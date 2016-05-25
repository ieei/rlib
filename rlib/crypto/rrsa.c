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
#include <rlib/crypto/rrsa.h>
#include <rlib/rmem.h>

typedef struct {
  RCryptoKey key;

  rmpint n;
  rmpint e;
} RRsaPubKey;

typedef struct {
  RRsaPubKey pub;

  rint32 ver;

  rmpint d;
  rmpint p;
  rmpint q;

  rmpint dp;
  rmpint dq;
  rmpint qp;
} RRsaPrivKey;

static void
r_rsa_pub_key_free (rpointer data)
{
  RRsaPubKey * key;

  if ((key = data) != NULL) {
    r_mpint_clear (&key->n);
    r_mpint_clear (&key->e);
    r_free (key);
  }
}

static void
r_rsa_pub_key_init (RCryptoKey * key)
{
  r_ref_init (key, r_rsa_pub_key_free);
  key->type = R_CRYPTO_PUBLIC_KEY;
  key->algo = R_CRYPTO_ALGO_RSA;
  key->strtype = R_RSA_STR;
}

static void
r_rsa_priv_key_free (rpointer data)
{
  RRsaPrivKey * key;

  if ((key = data) != NULL) {
    r_mpint_clear (&key->d);
    r_mpint_clear (&key->p);
    r_mpint_clear (&key->q);
    r_mpint_clear (&key->dp);
    r_mpint_clear (&key->dq);
    r_mpint_clear (&key->qp);

    r_rsa_pub_key_free (data);
  }
}

static void
r_rsa_priv_key_init (RCryptoKey * key)
{
  r_ref_init (key, r_rsa_priv_key_free);
  key->type = R_CRYPTO_PRIVATE_KEY;
  key->algo = R_CRYPTO_ALGO_RSA;
  key->strtype = R_RSA_STR;
}

RCryptoKey *
r_rsa_pub_key_new (const rmpint * n, const rmpint * e)
{
  RRsaPubKey * ret;

  if (n != NULL && e != NULL) {
    if ((ret = r_mem_new (RRsaPubKey)) != NULL) {
      r_rsa_pub_key_init (&ret->key);
      r_mpint_init_copy (&ret->n, n);
      r_mpint_init_copy (&ret->e, e);
    }
  } else {
    ret = NULL;
  }

  return (RCryptoKey *)ret;
}

RCryptoKey *
r_rsa_pub_key_new_binary (rconstpointer n, rsize nsize,
    rconstpointer e, rsize esize)
{
  RRsaPubKey * ret;

  if (n != NULL && nsize > 0 && e != NULL && esize > 0) {
    if ((ret = r_mem_new (RRsaPubKey)) != NULL) {
      r_rsa_pub_key_init (&ret->key);
      r_mpint_init_binary (&ret->n, n, nsize);
      r_mpint_init_binary (&ret->e, e, esize);
    }
  } else {
    ret = NULL;
  }

  return (RCryptoKey *)ret;
}

RCryptoKey *
r_rsa_priv_key_new (const rmpint * n, const rmpint * e, const rmpint * d)
{
  RRsaPrivKey * ret;

  if (n != NULL && e != NULL && d != NULL) {
    if ((ret = r_mem_new0 (RRsaPrivKey)) != NULL) {
      r_rsa_priv_key_init (&ret->pub.key);
      r_mpint_init_copy (&ret->pub.n, n);
      r_mpint_init_copy (&ret->pub.e, e);
      r_mpint_init_copy (&ret->d, d);
      r_mpint_init (&ret->p);
      r_mpint_init (&ret->q);
      r_mpint_init (&ret->dp);
      r_mpint_init (&ret->dq);
      r_mpint_init (&ret->qp);
    }
  } else {
    ret = NULL;
  }

  return (RCryptoKey *)ret;
}

RCryptoKey *
r_rsa_priv_key_new_full (rint32 ver, const rmpint * n, const rmpint * e,
    const rmpint * d, const rmpint * p, const rmpint * q,
    const rmpint * dp, const rmpint * dq, const rmpint * qp)
{
  RRsaPrivKey * ret;

  if (n != NULL && e != NULL && d != NULL) {
    if ((ret = r_mem_new (RRsaPrivKey)) != NULL) {
      r_rsa_priv_key_init (&ret->pub.key);
      ret->ver = ver;
      r_mpint_init_copy (&ret->pub.n, n);
      r_mpint_init_copy (&ret->pub.e, e);
      r_mpint_init_copy (&ret->d, d);
      if (p != NULL)  r_mpint_init_copy (&ret->p, p);
      else            r_mpint_init (&ret->p);
      if (q != NULL)  r_mpint_init_copy (&ret->q, q);
      else            r_mpint_init (&ret->q);
      if (dp != NULL) r_mpint_init_copy (&ret->dp, dp);
      else            r_mpint_init (&ret->dp);
      if (dq != NULL) r_mpint_init_copy (&ret->dq, dq);
      else            r_mpint_init (&ret->dq);
      if (qp != NULL) r_mpint_init_copy (&ret->qp, qp);
      else            r_mpint_init (&ret->qp);
    }
  } else {
    ret = NULL;
  }

  return (RCryptoKey *)ret;
}

RCryptoKey *
r_rsa_priv_key_new_binary (rconstpointer n, rsize nsize,
    rconstpointer e, rsize esize, rconstpointer d, rsize dsize)
{
  RRsaPrivKey * ret;

  if (n != NULL && e != NULL && d != NULL) {
    if ((ret = r_mem_new0 (RRsaPrivKey)) != NULL) {
      r_rsa_priv_key_init (&ret->pub.key);
      r_mpint_init_binary (&ret->pub.n, n, nsize);
      r_mpint_init_binary (&ret->pub.e, e, esize);
      r_mpint_init_binary (&ret->d, d, dsize);
      r_mpint_init (&ret->p);
      r_mpint_init (&ret->q);
      r_mpint_init (&ret->dp);
      r_mpint_init (&ret->dq);
      r_mpint_init (&ret->qp);
    }
  } else {
    ret = NULL;
  }

  return (RCryptoKey *)ret;
}

RCryptoKey *
r_rsa_priv_key_new_from_asn1 (RAsn1BinDecoder * dec)
{
  RRsaPrivKey * ret;

  if ((ret = r_mem_new (RRsaPrivKey)) != NULL) {
    RAsn1BinTLV tlv = R_ASN1_BIN_TLV_INIT;
    r_rsa_priv_key_init (&ret->pub.key);

    r_mpint_init (&ret->pub.n);
    r_mpint_init (&ret->pub.e);
    r_mpint_init (&ret->d);
    r_mpint_init (&ret->p);
    r_mpint_init (&ret->q);
    r_mpint_init (&ret->dp);
    r_mpint_init (&ret->dq);
    r_mpint_init (&ret->qp);

    if (r_asn1_bin_decoder_next (dec, &tlv) != R_ASN1_DECODER_OK ||
        r_asn1_bin_decoder_into (dec, &tlv) != R_ASN1_DECODER_OK ||
        r_asn1_bin_tlv_parse_integer (&tlv, &ret->ver) != R_ASN1_DECODER_OK ||
        r_asn1_bin_decoder_next (dec, &tlv) != R_ASN1_DECODER_OK ||
        r_asn1_bin_tlv_parse_mpint (&tlv, &ret->pub.n) != R_ASN1_DECODER_OK ||
        r_asn1_bin_decoder_next (dec, &tlv) != R_ASN1_DECODER_OK ||
        r_asn1_bin_tlv_parse_mpint (&tlv, &ret->pub.e) != R_ASN1_DECODER_OK ||
        r_asn1_bin_decoder_next (dec, &tlv) != R_ASN1_DECODER_OK ||
        r_asn1_bin_tlv_parse_mpint (&tlv, &ret->d) != R_ASN1_DECODER_OK ||
        r_asn1_bin_decoder_next (dec, &tlv) != R_ASN1_DECODER_OK ||
        r_asn1_bin_tlv_parse_mpint (&tlv, &ret->p) != R_ASN1_DECODER_OK ||
        r_asn1_bin_decoder_next (dec, &tlv) != R_ASN1_DECODER_OK ||
        r_asn1_bin_tlv_parse_mpint (&tlv, &ret->q) != R_ASN1_DECODER_OK ||
        r_asn1_bin_decoder_next (dec, &tlv) != R_ASN1_DECODER_OK ||
        r_asn1_bin_tlv_parse_mpint (&tlv, &ret->dp) != R_ASN1_DECODER_OK ||
        r_asn1_bin_decoder_next (dec, &tlv) != R_ASN1_DECODER_OK ||
        r_asn1_bin_tlv_parse_mpint (&tlv, &ret->dq) != R_ASN1_DECODER_OK ||
        r_asn1_bin_decoder_next (dec, &tlv) != R_ASN1_DECODER_OK ||
        r_asn1_bin_tlv_parse_mpint (&tlv, &ret->qp) != R_ASN1_DECODER_OK) {
      r_crypto_key_unref ((RCryptoKey *)ret);
      ret = NULL;
    }
  }

  return (RCryptoKey *)ret;
}

rboolean
r_rsa_key_get_exponent (RCryptoKey * key, rmpint * e)
{
  if (key != NULL && e != NULL && key->algo == R_CRYPTO_ALGO_RSA) {
    r_mpint_set (e, &((RRsaPubKey *)key)->e);
    return TRUE;
  }

  return FALSE;
}

rboolean
r_rsa_pub_key_get_modulus (RCryptoKey * key, rmpint * n)
{
  if (key != NULL && n != NULL && key->algo == R_CRYPTO_ALGO_RSA) {
    r_mpint_set (n, &((RRsaPubKey *)key)->n);
    return TRUE;
  }

  return FALSE;
}

rboolean
r_rsa_priv_key_get_modulus (RCryptoKey * key, rmpint * d)
{
  if (key != NULL && d != NULL &&
      key->type == R_CRYPTO_PRIVATE_KEY && key->algo == R_CRYPTO_ALGO_RSA) {
    r_mpint_set (d, &((RRsaPrivKey *)key)->d);
    return TRUE;
  }

  return FALSE;
}

rboolean
r_rsa_pub_key_encrypt (RCryptoKey * key,
    rconstpointer data, rsize size, ruint8 * out, rsize * outsize)
{
  if (R_UNLIKELY (key == NULL || data == NULL || size == 0 ||
        out == NULL || outsize == NULL || *outsize == 0))
    return FALSE;

  /* TODO: implment... */
  return FALSE;
}

rboolean
r_rsa_pub_key_decrypt (RCryptoKey * key,
    rconstpointer data, rsize size, ruint8 * out, rsize * outsize)
{
  if (R_UNLIKELY (key == NULL || data == NULL || size == 0 ||
        out == NULL || outsize == NULL || *outsize == 0))
    return FALSE;

  /* TODO: implment... */
  return FALSE;
}

