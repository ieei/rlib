/* RLIB - Convenience library for useful things
 * Copyright (C) 2016-2017 Haakon Sporsheim <haakon.sporsheim@gmail.com>
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
#ifndef __R_ASN1_ASN1_H__
#define __R_ASN1_ASN1_H__

#if !defined(__RLIB_H_INCLUDE_GUARD__) && !defined(RLIB_COMPILATION)
#error "#include <rlib.h> only pelase."
#endif

#include <rlib/rtypes.h>
#include <rlib/rref.h>

#include <rlib/data/rbitset.h>
#include <rlib/data/rmpint.h>

#include <rlib/rbuffer.h>
#include <rlib/rmsgdigest.h>

R_BEGIN_DECLS

/* ASN.1 identifier */
#define R_ASN1_ID(c, pc, tag) (((c) & R_ASN1_ID_CLASS_MASK) | \
    ((pc) & R_ASN1_ID_PC_MASK) | ((tag) & R_ASN1_ID_TAG_MASK))
#define R_ASN1_UNIVERSAL_ID(pc, tag)  R_ASN1_ID (R_ASN1_ID_UNIVERSAL, pc, tag)

#define R_ASN1_ID_CLASS_MASK              0xc0
typedef enum {
  R_ASN1_ID_UNIVERSAL                   = 0x00,
  R_ASN1_ID_APPLICATION                 = 0x40,
  R_ASN1_ID_CONTEXT                     = 0x80,
  R_ASN1_ID_PRIVATE                     = 0xc0,
} RAsn1IdClass;

#define R_ASN1_ID_PC_MASK                 0x20
#define R_ASN1_ID_PRIMITIVE               0x00
#define R_ASN1_ID_CONSTRUCTED             0x20

#define R_ASN1_ID_TAG_MASK                0x1F
typedef enum {
  R_ASN1_ID_EOC                         = 0x00,
  R_ASN1_ID_BOOLEAN                     = 0x01,
  R_ASN1_ID_INTEGER                     = 0x02,
  R_ASN1_ID_BIT_STRING                  = 0x03,
  R_ASN1_ID_OCTET_STRING                = 0x04,
  R_ASN1_ID_NULL                        = 0x05,
  R_ASN1_ID_OBJECT_IDENTIFIER           = 0x06,
  R_ASN1_ID_OBJECT_DESCRIPTOR           = 0x07,
  R_ASN1_ID_EXTERNAL                    = 0x08,
  R_ASN1_ID_REAL                        = 0x09,
  R_ASN1_ID_ENUMERATED                  = 0x0a,
  R_ASN1_ID_EMBEDDED_PDV                = 0x0b,
  R_ASN1_ID_UTF8_STRING                 = 0x0c,
  R_ASN1_ID_RELATIVE_OID                = 0x0d,
  R_ASN1_ID_RESERVED_0x0e               = 0x0e,
  R_ASN1_ID_RESERVED_0x0f               = 0x0f,
  R_ASN1_ID_SEQUENCE                    = 0x10,
  R_ASN1_ID_SET                         = 0x11,
  R_ASN1_ID_NUMERIC_STRING              = 0x12,
  R_ASN1_ID_PRINTABLE_STRING            = 0x13,
  R_ASN1_ID_T61_STRING                  = 0x14,
  R_ASN1_ID_VIDEOTEX_STRING             = 0x15,
  R_ASN1_ID_IA5_STRING                  = 0x16,
  R_ASN1_ID_UTC_TIME                    = 0x17,
  R_ASN1_ID_GENERALIZED_TIME            = 0x18,
  R_ASN1_ID_GRAPHIC_STRING              = 0x19,
  R_ASN1_ID_VISIBLE_STRING              = 0x1a,
  R_ASN1_ID_GENERAL_STRING              = 0x1b,
  R_ASN1_ID_UNIVERSAL_STRING            = 0x1c,
  R_ASN1_ID_CHARACTER_STRING            = 0x1d,
  R_ASN1_ID_BMP_STRING                  = 0x1e,
} RAsn1IdTag;

/* ASN.1 binary length octets */
#define R_ASN1_BIN_LENGTH_INDEFINITE      0x80

/* ASN.1 Decoder status */
typedef enum {
  R_ASN1_DECODER_EOS                    = -2,
  R_ASN1_DECODER_EOC                    = -1,
  R_ASN1_DECODER_OK                     = 0,
  R_ASN1_DECODER_INVALID_ARG,
  R_ASN1_DECODER_OOM,
  R_ASN1_DECODER_PARSE_ERROR,
  R_ASN1_DECODER_WRONG_TYPE,
  R_ASN1_DECODER_OVERFLOW,
  R_ASN1_DECODER_INDEFINITE,
  R_ASN1_DECODER_NOT_CONSTRUCTED,
} RAsn1DecoderStatus;
#define R_ASN1_DECODER_STATUS_ERROR(s)      ((s) >  R_ASN1_DECODER_OK)
#define R_ASN1_DECODER_STATUS_SUCCESS(s)    ((s) <= R_ASN1_DECODER_OK)

/* ASN.1 Encoder status */
typedef enum {
  R_ASN1_ENCODER_OK                     = 0,
  R_ASN1_ENCODER_INVALID_ARG,
  R_ASN1_ENCODER_OOM,
  R_ASN1_ENCODER_INDEFINITE,
  R_ASN1_ENCODER_NOT_CONSTRUCTED,
} RAsn1EncoderStatus;

/* ASN.1 binary Type-Length-Value */
#define R_ASN1_BIN_TLV_INIT           { NULL, 0, NULL }
#define R_ASN1_BIN_TLV_ID_CLASS(tlv)  (*(tlv)->start & R_ASN1_ID_CLASS_MASK)
#define R_ASN1_BIN_TLV_ID_PC(tlv)     (*(tlv)->start & R_ASN1_ID_PC_MASK)
#define R_ASN1_BIN_TLV_ID_TAG(tlv)    (*(tlv)->start & R_ASN1_ID_TAG_MASK)
#define R_ASN1_BIN_TLV_IS_ID(tlv, tag)(*(tlv)->start == (tag))
#define R_ASN1_BIN_TLV_ID_IS_TAG(tlv, tag)  (R_ASN1_BIN_TLV_ID_TAG (tlv) == (tag))

typedef struct _RAsn1BinTLV {
  const ruint8 *  start;  /* Type   (first octet of tlv) */
  rsize           len;    /* Length (parsed) */
  const ruint8 *  value;  /* Value  (pointer to first value octet) */
} RAsn1BinTLV;

R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_boolean (const RAsn1BinTLV * tlv, rboolean * value);
R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_integer_bits (const RAsn1BinTLV * tlv, ruint * bits, rboolean * unsign);
R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_integer_i32 (const RAsn1BinTLV * tlv, rint32 * value);
R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_integer_u32 (const RAsn1BinTLV * tlv, ruint32 * value);
R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_integer_i64 (const RAsn1BinTLV * tlv, rint64 * value);
R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_integer_u64 (const RAsn1BinTLV * tlv, ruint64 * value);
R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_integer_mpint (const RAsn1BinTLV * tlv, rmpint * value);
R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_oid (const RAsn1BinTLV * tlv, ruint32 * varray, rsize * size);
R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_oid_to_dot (const RAsn1BinTLV * tlv, rchar ** dot);
R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_oid_to_msg_digest_type (const RAsn1BinTLV * tlv, RMsgDigestType * mdtype);
R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_time (const RAsn1BinTLV * tlv, ruint64 * time);
R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_bit_string (const RAsn1BinTLV * tlv, RBitset ** bitset);
R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_bit_string_bits (const RAsn1BinTLV * tlv, rsize * bits);
#define r_asn1_bin_tlv_bit_string_value(tlv) (&(tlv)->value[1])
/* TODO: Add parsing of strings, time and ... */


typedef enum {
  R_ASN1_BER,
  R_ASN1_DER,
  R_ASN1_ENCODING_RULES_COUNT
} RAsn1EncodingRules;

typedef struct _RAsn1BinDecoder RAsn1BinDecoder;

R_API RAsn1BinDecoder * r_asn1_bin_decoder_new_file (RAsn1EncodingRules enc,
    const rchar * file);
R_API RAsn1BinDecoder * r_asn1_bin_decoder_new_with_data (RAsn1EncodingRules enc,
    ruint8 * data, rsize size);
R_API RAsn1BinDecoder * r_asn1_bin_decoder_new (RAsn1EncodingRules enc,
    const ruint8 * data, rsize size);
#define r_asn1_bin_decoder_ref r_ref_ref
#define r_asn1_bin_decoder_unref r_ref_unref

R_API RAsn1DecoderStatus r_asn1_bin_decoder_next (RAsn1BinDecoder * dec, RAsn1BinTLV * tlv);
R_API RAsn1DecoderStatus r_asn1_bin_decoder_into (RAsn1BinDecoder * dec, RAsn1BinTLV * tlv);
R_API RAsn1DecoderStatus r_asn1_bin_decoder_out (RAsn1BinDecoder * dec, RAsn1BinTLV * tlv);

R_API RAsn1DecoderStatus r_asn1_bin_tlv_parse_distinguished_name (RAsn1BinDecoder * dec,
    RAsn1BinTLV * tlv, rchar ** name);

/* TODO: Add callback/events based decoder/parser (like a SAX parser) */
/*R_API rboolean r_asn1_bin_decoder_decode_events (RAsn1BinDecoder * dec,*/
    /*RFunc primary, RFunc start, RFunc end);*/


typedef struct _RAsn1BinEncoder RAsn1BinEncoder;

R_API RAsn1BinEncoder * r_asn1_bin_encoder_new (RAsn1EncodingRules enc);
#define r_asn1_bin_encoder_ref r_ref_ref
#define r_asn1_bin_encoder_unref r_ref_unref

R_API RAsn1EncoderStatus r_asn1_bin_encoder_begin_constructed (RAsn1BinEncoder * enc, ruint8 id, rsize sizehint);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_begin_bit_string (RAsn1BinEncoder * enc, rsize sizehint);
#define r_asn1_bin_encoder_begin_octet_string(enc, sizehint)                  \
  r_asn1_bin_encoder_begin_constructed (enc,                                  \
      R_ASN1_ID (R_ASN1_ID_UNIVERSAL, R_ASN1_ID_PRIMITIVE, R_ASN1_ID_OCTET_STRING), \
      sizehint)
R_API RAsn1EncoderStatus r_asn1_bin_encoder_end_constructed (RAsn1BinEncoder * enc);
#define r_asn1_bin_encoder_end_bit_string  r_asn1_bin_encoder_end_constructed
#define r_asn1_bin_encoder_end_octet_string  r_asn1_bin_encoder_end_constructed

R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_raw (RAsn1BinEncoder * enc, ruint8 id, rconstpointer data, rsize size);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_null (RAsn1BinEncoder * enc);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_boolean (RAsn1BinEncoder * enc, rboolean value);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_integer_i32 (RAsn1BinEncoder * enc, rint32 value);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_integer_u32 (RAsn1BinEncoder * enc, ruint32 value);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_integer_i64 (RAsn1BinEncoder * enc, rint64 value);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_integer_u64 (RAsn1BinEncoder * enc, ruint64 value);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_integer_mpint (RAsn1BinEncoder * enc, const rmpint * value);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_oid_rawsz (RAsn1BinEncoder * enc, const rchar * rawsz);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_bit_string_raw (RAsn1BinEncoder * enc, const ruint8 * bits, rsize size);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_utc_time (RAsn1BinEncoder * enc, ruint64 time);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_ia5_string (RAsn1BinEncoder * enc, const rchar * dn, rssize size);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_utf8_string (RAsn1BinEncoder * enc, const rchar * dn, rssize size);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_printable_string (RAsn1BinEncoder * enc, const rchar * dn, rssize size);
R_API RAsn1EncoderStatus r_asn1_bin_encoder_add_distinguished_name (RAsn1BinEncoder * enc, const rchar * dn);

R_API ruint8 * r_asn1_bin_encoder_get_data (RAsn1BinEncoder * enc, rsize * size) R_ATTR_MALLOC;
R_API RAsn1EncoderStatus r_asn1_bin_encoder_get_buffer (RAsn1BinEncoder * enc, RBuffer ** buf);



const rchar * r_asn1_x500_name_from_oid (rconstpointer p, rsize size);
const rchar * r_asn1_x500_name_to_oid (const rchar * name, rsize size);

R_END_DECLS

#endif /* __R_ASN1_ASN1_H__ */

