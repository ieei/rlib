/* RLIB - Convenience library for useful things
 * Copyright (C) 2016  Haakon Sporsheim <haakon.sporsheim@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 * See the COPYING file at the root of the source repository.
 */

#include "config.h"
#include <rlib/crypto/rx509.h>
#include <rlib/crypto/rcrypto-private.h>

#include <rlib/asn1/roid.h>
#include <rlib/rlist.h>
#include <rlib/rmem.h>
#include <rlib/rstr.h>

typedef struct {
  ruint8 * value;
  rsize size;
} RX509Buf;

typedef struct {
  RCryptoCert cert;

  RX509Version version;
  ruint64 serial;
  rchar * issuer;
  rchar * subject;

  RX509Buf issuerUniqueID;
  RX509Buf subjectUniqueID;
  RX509Buf subjectKeyID;
  RX509Buf authorityKeyID;
  rchar * authority;
  ruint64 authorityCertSerialNumber;

  RX509KeyUsage keyUsage;
  RX509ExtKeyUsage extKeyUsage;
  RSList * policies;
  rboolean ca;
  rint32 pathLen;
  rint32 requireExplicitPolicy;
  rint32 inhibitPolicyMapping;
} RCryptoX509Cert;

typedef rboolean (*RCertX509ExtFunc) (RCryptoX509Cert * cert,
    RAsn1BinDecoder * dec, RAsn1BinTLV * tlv, rboolean critical);

static rboolean
r_crypto_x509_authority_key_id (RCryptoX509Cert * cert,
    RAsn1BinDecoder * dec, RAsn1BinTLV * tlv, rboolean critical)
{
  if (R_UNLIKELY (cert->authorityKeyID.value != NULL)) return FALSE;

  (void) critical; /* Warning if critical is TRUE! */

  r_asn1_bin_decoder_into (dec, tlv);
  if (R_ASN1_BIN_TLV_IS_ID (tlv, R_ASN1_ID_CONTEXT | 0)) {
    cert->authorityKeyID.value = r_memdup (tlv->value, tlv->len);
    cert->authorityKeyID.size = tlv->len;
    r_asn1_bin_decoder_next (dec, tlv);
  }
  if (R_ASN1_BIN_TLV_IS_ID (tlv, R_ASN1_ID_CONTEXT | 1)) {
    /* TODO: authorityCertIssuer -> GeneralNames */
    r_asn1_bin_decoder_next (dec, tlv);
  }
  if (R_ASN1_BIN_TLV_IS_ID (tlv, R_ASN1_ID_CONTEXT | 2)) {
    r_asn1_bin_tlv_parse_integer_u64 (tlv, &cert->authorityCertSerialNumber);
    r_asn1_bin_decoder_next (dec, tlv);
  }
  r_asn1_bin_decoder_out (dec, tlv);
  return TRUE;
}

static rboolean
r_crypto_x509_subject_key_id (RCryptoX509Cert * cert,
    RAsn1BinDecoder * dec, RAsn1BinTLV * tlv, rboolean critical)
{
  if (R_UNLIKELY (cert->subjectKeyID.value != NULL)) return FALSE;
  if (R_UNLIKELY (!R_ASN1_BIN_TLV_ID_IS_TAG (tlv, R_ASN1_ID_OCTET_STRING)))
    return FALSE;

  (void) dec;
  (void) critical; /* Warning if critical is TRUE! */

  cert->subjectKeyID.value = r_memdup (tlv->value, tlv->len);
  cert->subjectKeyID.size = tlv->len;
  return TRUE;
}

static rboolean
r_crypto_x509_basic_constraints (RCryptoX509Cert * cert,
    RAsn1BinDecoder * dec, RAsn1BinTLV * tlv, rboolean critical)
{
  (void) critical;

  r_asn1_bin_decoder_into (dec, tlv);
  if (R_ASN1_BIN_TLV_ID_IS_TAG (tlv, R_ASN1_ID_BOOLEAN)) {
    r_asn1_bin_tlv_parse_boolean (tlv, &cert->ca);
    r_asn1_bin_decoder_next (dec, tlv);
  }

  if (R_ASN1_BIN_TLV_ID_IS_TAG (tlv, R_ASN1_ID_INTEGER)) {
    r_asn1_bin_tlv_parse_integer_i32 (tlv, &cert->pathLen);
    r_asn1_bin_decoder_next (dec, tlv);
  }
  r_asn1_bin_decoder_out (dec, tlv);

  return TRUE;
}

static rboolean
r_crypto_x509_policy_constraints (RCryptoX509Cert * cert,
    RAsn1BinDecoder * dec, RAsn1BinTLV * tlv, rboolean critical)
{
  (void) critical;

  r_asn1_bin_decoder_into (dec, tlv);
  if (R_ASN1_BIN_TLV_IS_ID (tlv, R_ASN1_ID_CONTEXT | 0)) {
    r_asn1_bin_tlv_parse_integer_i32 (tlv, &cert->requireExplicitPolicy);
    r_asn1_bin_decoder_next (dec, tlv);
  }
  if (R_ASN1_BIN_TLV_IS_ID (tlv, R_ASN1_ID_CONTEXT | 1)) {
    r_asn1_bin_tlv_parse_integer_i32 (tlv, &cert->inhibitPolicyMapping);
    r_asn1_bin_decoder_next (dec, tlv);
  }
  r_asn1_bin_decoder_out (dec, tlv);

  return TRUE;
}

static rboolean
r_crypto_x509_key_usage (RCryptoX509Cert * cert,
    RAsn1BinDecoder * dec, RAsn1BinTLV * tlv, rboolean critical)
{
  (void) dec;
  (void) critical;

  if (R_ASN1_BIN_TLV_ID_IS_TAG (tlv, R_ASN1_ID_BIT_STRING)) {
    if (tlv->len == 2) {
      cert->keyUsage = (tlv->value[1] >> tlv->value[0]);
      return TRUE;
    } else if (tlv->len == 3) {
      cert->keyUsage = (RUINT16_FROM_BE (*(const ruint16 *)&tlv->value[1]) >> tlv->value[0]);
      return TRUE;
    }
  }

  return FALSE;
}

static rboolean
r_crypto_x509_ext_key_usage (RCryptoX509Cert * cert,
    RAsn1BinDecoder * dec, RAsn1BinTLV * tlv, rboolean critical)
{
  rchar * oid;

  (void) critical;

  do {
    if (!R_ASN1_BIN_TLV_ID_IS_TAG (tlv, R_ASN1_ID_SEQUENCE))
      break;
    if (r_asn1_bin_decoder_into (dec, tlv) != R_ASN1_DECODER_OK)
      break;

    if (r_asn1_bin_tlv_parse_oid_to_dot (tlv, &oid) == R_ASN1_DECODER_OK) {
      if (r_str_equals (oid, R_ID_CE_OID_EXT_KEY_USAGE".0"))
        cert->extKeyUsage |= R_X509_EXT_KEY_USAGE_ANY;
      else if (r_str_equals (oid, R_ID_KP_OID_SERVER_AUTH))
        cert->extKeyUsage |= R_X509_EXT_KEY_USAGE_SERVER_AUTH;
      else if (r_str_equals (oid, R_ID_KP_OID_CLIENT_AUTH))
        cert->extKeyUsage |= R_X509_EXT_KEY_USAGE_CLIENT_AUTH;
      else if (r_str_equals (oid, R_ID_KP_OID_CODE_SIGNING))
        cert->extKeyUsage |= R_X509_EXT_KEY_USAGE_CODE_SIGNING;
      else if (r_str_equals (oid, R_ID_KP_OID_EMAIL_PROTECTION))
        cert->extKeyUsage |= R_X509_EXT_KEY_USAGE_EMAIL_PROTECTION;
      else if (r_str_equals (oid, R_ID_KP_OID_TIME_STAMPING))
        cert->extKeyUsage |= R_X509_EXT_KEY_USAGE_TIME_STAMPING;
      else if (r_str_equals (oid, R_ID_KP_OID_OCSP_SIGNING))
        cert->extKeyUsage |= R_X509_EXT_KEY_USAGE_OCSP_SIGNING;

      r_free (oid);
    }
  } while (r_asn1_bin_decoder_out (dec, tlv) == R_ASN1_DECODER_OK);

  return TRUE;
}

static rboolean
r_crypto_x509_certificate_policies (RCryptoX509Cert * cert,
    RAsn1BinDecoder * dec, RAsn1BinTLV * tlv, rboolean critical)
{
  (void) critical;

  do {
    if (!R_ASN1_BIN_TLV_ID_IS_TAG (tlv, R_ASN1_ID_SEQUENCE))
      break;
    if (r_asn1_bin_decoder_into (dec, tlv) != R_ASN1_DECODER_OK)
      break;

    if (r_asn1_bin_decoder_into (dec, tlv) == R_ASN1_DECODER_OK) {
      rchar * oid;
      if (r_asn1_bin_tlv_parse_oid_to_dot (tlv, &oid) == R_ASN1_DECODER_OK)
        cert->policies = r_slist_prepend (cert->policies, oid);

      r_asn1_bin_decoder_out (dec, tlv);
    }
  } while (r_asn1_bin_decoder_out (dec, tlv) == R_ASN1_DECODER_OK);

  return TRUE;
}

static rboolean
r_crypto_x509_cert_v3_parse_extensions (RCryptoX509Cert * cert,
    RAsn1BinDecoder * dec, RAsn1BinTLV * tlv)
{
  RAsn1DecoderStatus res;
  rchar * oid;
  static const struct {
    const rchar * oid;
    RCertX509ExtFunc func;
  } exttbl[] = {
    { R_ID_CE_OID_AUTHORITY_KEY_ID, r_crypto_x509_authority_key_id },
    { R_ID_CE_OID_SUBJECT_KEY_ID, r_crypto_x509_subject_key_id },
    { R_ID_CE_OID_BASIC_CONSTRAINTS, r_crypto_x509_basic_constraints },
    /*{ R_ID_CE_OID_NAME_CONSTRAINTS, r_crypto_x509_name_constraints },*/
    { R_ID_CE_OID_POLICY_CONSTRAINTS, r_crypto_x509_policy_constraints },
    { R_ID_CE_OID_KEY_USAGE, r_crypto_x509_key_usage },
    { R_ID_CE_OID_EXT_KEY_USAGE, r_crypto_x509_ext_key_usage },
    { R_ID_CE_OID_CERTIFICATE_POLICIES, r_crypto_x509_certificate_policies },
    /*{ R_ID_CE_OID_SUBJECT_ALT_NAME, r_crypto_x509_subject_alt_name },*/
    /*{ R_ID_CE_OID_POLICY_MAPPINGS, r_crypto_x509_policy_mappings },*/
    /* TODO */
  };

  if (R_UNLIKELY (r_asn1_bin_decoder_into (dec, tlv) != R_ASN1_DECODER_OK))
    return FALSE;

  do {
    if (R_UNLIKELY (r_asn1_bin_decoder_into (dec, tlv) != R_ASN1_DECODER_OK))
      break;

    if (r_asn1_bin_tlv_parse_oid_to_dot (tlv, &oid) == R_ASN1_DECODER_OK) {
      rboolean critical = FALSE;
      ruint i;

      r_asn1_bin_decoder_next (dec, tlv);

      if (R_ASN1_BIN_TLV_ID_IS_TAG (tlv, R_ASN1_ID_BOOLEAN)) {
        r_asn1_bin_tlv_parse_boolean (tlv, &critical);
        r_asn1_bin_decoder_next (dec, tlv);
      }

      if (R_ASN1_BIN_TLV_ID_IS_TAG (tlv, R_ASN1_ID_OCTET_STRING)) {
        r_asn1_bin_decoder_into (dec, tlv);
        for (i = 0; i < R_N_ELEMENTS (exttbl); i++) {
          if (r_str_equals (oid, exttbl[i].oid)) {
            exttbl[i].func (cert, dec, tlv, critical);
            break;
          }
        }
        r_asn1_bin_decoder_out (dec, tlv);
      }
      r_free (oid);
    }
  } while ((res = r_asn1_bin_decoder_out (dec, tlv)) == R_ASN1_DECODER_OK);

  r_asn1_bin_decoder_out (dec, tlv);
  return res == R_ASN1_DECODER_EOC;
}

static rboolean
r_crypto_x509_cert_init (RCryptoX509Cert * cert, RAsn1BinDecoder * dec)
{
  RAsn1BinTLV tlv = R_ASN1_BIN_TLV_INIT;

  if (R_UNLIKELY (r_asn1_bin_decoder_next (dec, &tlv) != R_ASN1_DECODER_OK))
    return FALSE;

  /* X.509 Certificate */
  if (r_asn1_bin_decoder_into (dec, &tlv) == R_ASN1_DECODER_OK) {
    rchar * oid;
    /* TBSCertificate */
    if (r_asn1_bin_decoder_into (dec, &tlv) == R_ASN1_DECODER_OK) {
      /* version - Optional */
      if (r_asn1_bin_decoder_into (dec, &tlv) == R_ASN1_DECODER_OK) {
        rint32 ver;
        if (r_asn1_bin_tlv_parse_integer_i32 (&tlv, &ver) != R_ASN1_DECODER_OK ||
            ver > R_X509_VERSION_SUPPORTED)
          goto beach;
        cert->version = ver;
        r_asn1_bin_decoder_out (dec, &tlv);
      } else {
        cert->version = R_X509_VERSION_V1; /* Default value*/
      }
      /* serialNumber */
      if (r_asn1_bin_tlv_parse_integer_u64 (&tlv, &cert->serial) != R_ASN1_DECODER_OK ||
          r_asn1_bin_decoder_next (dec, &tlv) != R_ASN1_DECODER_OK)
        goto beach;
      /* signature - Skip */
      if (r_asn1_bin_decoder_next (dec, &tlv) != R_ASN1_DECODER_OK)
        goto beach;
      /* issuer */
      if (r_asn1_bin_tlv_parse_distinguished_name (dec, &tlv, &cert->issuer) != R_ASN1_DECODER_OK)
        goto beach;
      /* validity */
      if (r_asn1_bin_decoder_into (dec, &tlv) != R_ASN1_DECODER_OK ||
          r_asn1_bin_tlv_parse_time (&tlv, &cert->cert.valid_from) != R_ASN1_DECODER_OK ||
          r_asn1_bin_decoder_next (dec, &tlv) != R_ASN1_DECODER_OK ||
          r_asn1_bin_tlv_parse_time (&tlv, &cert->cert.valid_to) != R_ASN1_DECODER_OK ||
          r_asn1_bin_decoder_out (dec, &tlv) != R_ASN1_DECODER_OK)
        goto beach;
      /* subject */
      if (r_asn1_bin_tlv_parse_distinguished_name (dec, &tlv, &cert->subject) != R_ASN1_DECODER_OK)
        goto beach;
      /* subjectPublicKeyInfo */
      if ((cert->cert.pk = r_crypto_key_from_asn1_public_key (dec, &tlv)) == NULL)
        goto beach;

      if (cert->version > R_X509_VERSION_V1) {
        if (R_ASN1_BIN_TLV_IS_ID (&tlv, R_ASN1_ID_CONTEXT | R_ASN1_ID_CONSTRUCTED | 1)) {
          if (r_asn1_bin_decoder_into (dec, &tlv) == R_ASN1_DECODER_OK) {
            rsize bits;
            if (r_asn1_bin_tlv_parse_bit_string_bits (&tlv, &bits) == R_ASN1_DECODER_OK && (bits % 8) == 0) {
              cert->issuerUniqueID.value = r_memdup (r_asn1_bin_tlv_bit_string_value (&tlv), bits / 8);
              cert->issuerUniqueID.size = bits / 8;
            }
            r_asn1_bin_decoder_out (dec, &tlv);
          }
        }
        if (R_ASN1_BIN_TLV_IS_ID (&tlv, R_ASN1_ID_CONTEXT | R_ASN1_ID_CONSTRUCTED | 2)) {
          if (r_asn1_bin_decoder_into (dec, &tlv) == R_ASN1_DECODER_OK) {
            rsize bits;
            if (r_asn1_bin_tlv_parse_bit_string_bits (&tlv, &bits) == R_ASN1_DECODER_OK && (bits % 8) == 0) {
              cert->subjectUniqueID.value = r_memdup (r_asn1_bin_tlv_bit_string_value (&tlv), bits / 8);
              cert->subjectUniqueID.size = bits / 8;
            }
            r_asn1_bin_decoder_out (dec, &tlv);
          }
        }
        if (cert->version > R_X509_VERSION_V2) {
          if (R_ASN1_BIN_TLV_IS_ID (&tlv, R_ASN1_ID_CONTEXT | R_ASN1_ID_CONSTRUCTED | 3)) {
            if (r_asn1_bin_decoder_into (dec, &tlv) == R_ASN1_DECODER_OK) {
              /*rboolean ret = */r_crypto_x509_cert_v3_parse_extensions (cert, dec, &tlv);
              r_asn1_bin_decoder_out (dec, &tlv);
              /*if (!ret)*/
                /*goto beach;*/
            }
          }
        }
      }

      r_asn1_bin_decoder_out (dec, &tlv);
    } else goto beach;

    /* signatureAlgorithm */
    if (r_asn1_bin_decoder_into (dec, &tlv) == R_ASN1_DECODER_OK &&
      r_asn1_bin_tlv_parse_oid_to_dot (&tlv, &oid) == R_ASN1_DECODER_OK) {
      if (r_str_equals (oid, R_RSA_OID_MD5_WITH_RSA))
        cert->cert.signalgo = R_CRYPTO_SIGN_ALGO_RSA_MD5;
      else if (r_str_equals (oid, R_RSA_OID_SHA1_WITH_RSA))
        cert->cert.signalgo = R_CRYPTO_SIGN_ALGO_RSA_SHA1;
      else if (r_str_equals (oid, R_RSA_OID_SHA256_WITH_RSA))
        cert->cert.signalgo = R_CRYPTO_SIGN_ALGO_RSA_SHA256;
      else if (r_str_equals (oid, R_RSA_OID_SHA384_WITH_RSA))
        cert->cert.signalgo = R_CRYPTO_SIGN_ALGO_RSA_SHA384;
      else if (r_str_equals (oid, R_RSA_OID_SHA512_WITH_RSA))
        cert->cert.signalgo = R_CRYPTO_SIGN_ALGO_RSA_SHA512;
      else if (r_str_equals (oid, R_RSA_OID_SHA224_WITH_RSA))
        cert->cert.signalgo = R_CRYPTO_SIGN_ALGO_RSA_SHA224;
      else if (r_str_equals (oid, R_X9CM_OID_DSA_WITH_SHA1))
        cert->cert.signalgo = R_CRYPTO_SIGN_ALGO_DSA_SHA1;
      else
        cert->cert.signalgo = R_CRYPTO_SIGN_ALGO_UNKNOWN;
      r_free (oid);
      r_asn1_bin_decoder_out (dec, &tlv);
    } else goto beach;

    /* signature */
    if (R_ASN1_BIN_TLV_ID_IS_TAG (&tlv, R_ASN1_ID_BIT_STRING) &&
        r_asn1_bin_tlv_parse_bit_string_bits (&tlv, &cert->cert.signbits) == R_ASN1_DECODER_OK) {
      cert->cert.sign = r_asn1_bin_tlv_bit_string_value (&tlv);
    } else goto beach;

    return TRUE;
  }

beach:
  return FALSE;
}

static void
r_crypto_x509_cert_free (RCryptoX509Cert * cert)
{
  r_crypto_cert_destroy ((RCryptoCert *)cert);
  r_free (cert->issuer);
  r_free (cert->subject);
  r_free (cert->issuerUniqueID.value);
  r_free (cert->subjectUniqueID.value);
  r_free (cert->subjectKeyID.value);
  r_free (cert->authorityKeyID.value);
  r_free (cert->authority);
  r_slist_destroy_full (cert->policies, r_free);
  r_free (cert);
}

RCryptoCert *
r_crypto_x509_cert_new (rconstpointer data, rsize size)
{
  RAsn1BinDecoder * dec;
  RCryptoCert * ret;

  if ((dec = r_asn1_bin_decoder_new (R_ASN1_DER, data, size)) != NULL) {
    ret = r_crypto_x509_cert_new_from_asn1 (dec);
    r_asn1_bin_decoder_unref (dec);
  } else {
    ret = NULL;
  }

  return ret;
}

RCryptoCert *
r_crypto_x509_cert_new_from_asn1 (RAsn1BinDecoder * dec)
{
  RCryptoX509Cert * ret;

  if (R_UNLIKELY (dec == NULL)) return NULL;

  if ((ret = r_mem_new0 (RCryptoX509Cert)) != NULL) {
    r_ref_init (ret, r_crypto_x509_cert_free);
    ret->cert.type = R_CRYPTO_CERT_X509;
    ret->cert.strtype = "X.509";

    if (!r_crypto_x509_cert_init (ret, dec)) {
      r_crypto_cert_unref (ret);
      ret = NULL;
    }
  }

  return (RCryptoCert *)ret;
}

RX509Version
r_crypt_x509_cert_version (const RCryptoCert * cert)
{
  return ((const RCryptoX509Cert *)cert)->version;
}

ruint64
r_crypt_x509_cert_serial_number (const RCryptoCert * cert)
{
  return ((const RCryptoX509Cert *)cert)->serial;
}

const rchar *
r_crypt_x509_cert_issuer (const RCryptoCert * cert)
{
  return ((const RCryptoX509Cert *)cert)->issuer;
}

const rchar *
r_crypt_x509_cert_subject (const RCryptoCert * cert)
{
  return ((const RCryptoX509Cert *)cert)->subject;
}

const ruint8 *
r_crypt_x509_cert_issuer_unique_id (const RCryptoCert * cert, rsize * size)
{
  if (size != NULL)
    *size = ((const RCryptoX509Cert *)cert)->issuerUniqueID.size;
  return ((const RCryptoX509Cert *)cert)->issuerUniqueID.value;
}

const ruint8 *
r_crypt_x509_cert_subject_unique_id (const RCryptoCert * cert, rsize * size)
{
  if (size != NULL)
    *size = ((const RCryptoX509Cert *)cert)->subjectUniqueID.size;
  return ((const RCryptoX509Cert *)cert)->subjectUniqueID.value;
}

const ruint8 *
r_crypt_x509_cert_subject_key_id (const RCryptoCert * cert, rsize * size)
{
  if (size != NULL)
    *size = ((const RCryptoX509Cert *)cert)->subjectKeyID.size;
  return ((const RCryptoX509Cert *)cert)->subjectKeyID.value;
}

const ruint8 *
r_crypt_x509_cert_authority_key_id (const RCryptoCert * cert, rsize * size)
{
  if (size != NULL)
    *size = ((const RCryptoX509Cert *)cert)->authorityKeyID.size;
  return ((const RCryptoX509Cert *)cert)->authorityKeyID.value;
}

RX509KeyUsage
r_crypt_x509_cert_key_usage (const RCryptoCert * cert)
{
  return ((const RCryptoX509Cert *)cert)->keyUsage;
}

RX509ExtKeyUsage
r_crypt_x509_cert_ext_key_usage (const RCryptoCert * cert)
{
  return ((const RCryptoX509Cert *)cert)->extKeyUsage;
}

rboolean
r_crypt_x509_cert_is_ca (const RCryptoCert * cert)
{
  return ((const RCryptoX509Cert *)cert)->ca;
}

rboolean
r_crypt_x509_cert_is_self_issued (const RCryptoCert * cert)
{
  return r_str_equals (((const RCryptoX509Cert *)cert)->issuer,
      ((const RCryptoX509Cert *)cert)->subject);
}

rboolean
r_crypt_x509_cert_is_self_signed (const RCryptoCert * cert)
{
  const RCryptoX509Cert * c = (const RCryptoX509Cert *)cert;

  return c->authorityKeyID.value == NULL || (c->subjectKeyID.value != NULL &&
      c->authorityKeyID.size == c->subjectKeyID.size &&
      r_memcmp (c->authorityKeyID.value, c->subjectKeyID.value, c->subjectKeyID.size) == 0);
}

rboolean
r_crypt_x509_cert_has_policy (const RCryptoCert * cert, const rchar * policy)
{
  const RCryptoX509Cert * x509 = (const RCryptoX509Cert *)cert;
  RSList * it;

  for (it = x509->policies; it != NULL; it = r_slist_next (it)) {
    if (r_str_equals (policy, r_slist_data (it)))
      return TRUE;
  }

  return FALSE;
}
