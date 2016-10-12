#include <rlib/rlib.h>

RTEST (rtls, parse_errors, RTEST_FAST)
{
  static const ruint8 pkt_bogus[] = { 0x16, 0x11, 0x12, 0x00, 0x04 };
  RTLSParser parser;

  r_assert_cmpint (R_TLS_ERROR_INVAL, ==, r_tls_parser_init (NULL, NULL, 0));
  r_assert_cmpint (R_TLS_ERROR_INVAL, ==, r_tls_parser_init (&parser, NULL, 0));
  r_assert_cmpint (R_TLS_ERROR_BUF_TOO_SMALL, ==, r_tls_parser_init (&parser, pkt_bogus, 0));
  r_assert_cmpint (R_TLS_ERROR_VERSION, ==, r_tls_parser_init (&parser, pkt_bogus, sizeof (pkt_bogus)));
}
RTEST_END;

RTEST (rtls, parse_dtls_client_hello, RTEST_FAST)
{
  static const ruint8 pkt_dtls_client_hallo[] = {
    0x16, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9e, 0x01, 0x00, 0x00,
    0x92, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x92, 0xfe, 0xfd, 0x69, 0x03, 0x27, 0x0d, 0xab,
    0x7e, 0x39, 0x4d, 0x78, 0x67, 0x5c, 0x98, 0x4b, 0x7b, 0x2e, 0xf5, 0xeb, 0x3f, 0x2a, 0xaf, 0x8f,
    0xf7, 0xfa, 0x55, 0xbd, 0x0b, 0x6b, 0x97, 0xf3, 0x91, 0x4a, 0x34, 0x00, 0x00, 0x00, 0x22, 0xc0,
    0x2b, 0xc0, 0x2f, 0x00, 0x9e, 0xcc, 0xa9, 0xcc, 0xa8, 0xcc, 0x14, 0xcc, 0x13, 0xc0, 0x09, 0xc0,
    0x13, 0x00, 0x33, 0xc0, 0x0a, 0xc0, 0x14, 0x00, 0x39, 0x00, 0x9c, 0x00, 0x2f, 0x00, 0x35, 0x00,
    0x0a, 0x01, 0x00, 0x00, 0x46, 0xff, 0x01, 0x00, 0x01, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x23,
    0x00, 0x00, 0x00, 0x0d, 0x00, 0x18, 0x00, 0x16, 0x08, 0x06, 0x06, 0x01, 0x06, 0x03, 0x08, 0x05,
    0x05, 0x01, 0x05, 0x03, 0x08, 0x04, 0x04, 0x01, 0x04, 0x03, 0x02, 0x01, 0x02, 0x03, 0x00, 0x0e,
    0x00, 0x07, 0x00, 0x04, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x0b, 0x00, 0x02, 0x01, 0x00, 0x00,
    0x0a, 0x00, 0x08, 0x00, 0x06, 0x00, 0x1d, 0x00, 0x17, 0x00, 0x18
  };
  RTLSParser parser;
  RTLSHandshakeType hstype;
  ruint32 hslen;
  RTLSHelloMsg msg;
  RTLSHelloExt ext;

  r_assert_cmpint (R_TLS_ERROR_OK, ==,
      r_tls_parser_init (&parser, pkt_dtls_client_hallo, sizeof (pkt_dtls_client_hallo)));
  r_assert_cmpuint (parser.content, ==, R_TLS_CONTENT_TYPE_HANDSHAKE);
  r_assert_cmpuint (parser.version, ==, R_TLS_VERSION_DTLS_1_0);
  r_assert_cmpuint (parser.epoch, ==, 0);
  r_assert_cmpuint (parser.seqno, ==, 0);
  r_assert_cmpuint (parser.fraglen, ==, 158);

  r_assert (r_tls_parser_dtls_is_complete_handshake_fragment (&parser));
  r_assert_cmpint (R_TLS_ERROR_OK, ==,
      r_tls_parser_parse_handshake (&parser, &hstype, &hslen));
  r_assert_cmpuint (hstype, ==, R_TLS_HANDSHAKE_TYPE_CLIENT_HELLO);
  r_assert_cmpuint (hslen, ==, 146);
  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_parser_parse_hello (&parser, &msg));
  r_assert_cmpuint (msg.version, ==, R_TLS_VERSION_DTLS_1_2);
  r_assert_cmpuint (msg.sidlen, ==, 0);
  r_assert_cmpuint (msg.cookielen, ==, 0);
  r_assert_cmpuint (msg.cslen, ==, 34);
  r_assert_cmpuint (msg.complen, ==, 1);
  r_assert_cmpuint (msg.extlen, ==, 70);

  r_assert_cmpuint (r_tls_hello_msg_cipher_suite_count (&msg), ==, 17);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg,  0), ==, R_CS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg,  1), ==, R_CS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg,  2), ==, R_CS_TLS_DHE_RSA_WITH_AES_128_GCM_SHA256);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg,  3), ==, R_CS_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg,  4), ==, R_CS_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg,  5), ==, R_CS_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256_OLD);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg,  6), ==, R_CS_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256_OLD);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg,  7), ==, R_CS_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg,  8), ==, R_CS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg,  9), ==, R_CS_TLS_DHE_RSA_WITH_AES_128_CBC_SHA);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg, 10), ==, R_CS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg, 11), ==, R_CS_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg, 12), ==, R_CS_TLS_DHE_RSA_WITH_AES_256_CBC_SHA);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg, 13), ==, R_CS_TLS_RSA_WITH_AES_128_GCM_SHA256);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg, 14), ==, R_CS_TLS_RSA_WITH_AES_128_CBC_SHA);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg, 15), ==, R_CS_TLS_RSA_WITH_AES_256_CBC_SHA);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg, 16), ==, R_CS_TLS_RSA_WITH_3DES_EDE_CBC_SHA);
  r_assert (r_tls_hello_msg_has_cipher_suite (&msg, R_CS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA));
  r_assert (!r_tls_hello_msg_has_cipher_suite (&msg, R_CS_TLS_ECDHE_PSK_WITH_3DES_EDE_CBC_SHA));
  r_assert_cmpuint (r_tls_hello_msg_compression_count (&msg), ==, 1);
  r_assert_cmpuint (r_tls_hello_msg_compression_method (&msg, 0), ==, R_TLS_COMPRESSION_NULL);

  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_hello_msg_extension_first (&msg, &ext));
  r_assert_cmpuint (ext.type, ==, R_TLS_EXT_TYPE_RENEGOTIATION_INFO);
  r_assert_cmpuint (ext.len, ==, 1);
  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_hello_msg_extension_next (&msg, &ext));
  r_assert_cmpuint (ext.type, ==, R_TLS_EXT_TYPE_EXTENDED_MASTER_SECRET);
  r_assert_cmpuint (ext.len, ==, 0);
  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_hello_msg_extension_next (&msg, &ext));
  r_assert_cmpuint (ext.type, ==, R_TLS_EXT_TYPE_SESSION_TICKET);
  r_assert_cmpuint (ext.len, ==, 0);
  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_hello_msg_extension_next (&msg, &ext));
  r_assert_cmpuint (ext.type, ==, R_TLS_EXT_TYPE_SIGNATURE_ALGORITHMS);
  r_assert_cmpuint (ext.len, ==, 24);
  r_assert_cmpuint (r_tls_hello_ext_sign_scheme_count (&ext), ==, 11);
  r_assert_cmphex (r_tls_hello_ext_sign_scheme (&ext,  0), ==, R_TLS_SIGN_SCHEME_RSA_PSS_SHA512);
  r_assert_cmphex (r_tls_hello_ext_sign_scheme (&ext,  1), ==, R_TLS_SIGN_SCHEME_RSA_PKCS1_SHA512);
  r_assert_cmphex (r_tls_hello_ext_sign_scheme (&ext,  2), ==, R_TLS_SIGN_SCHEME_ECDSA_SECP521R1_SHA512);
  r_assert_cmphex (r_tls_hello_ext_sign_scheme (&ext,  3), ==, R_TLS_SIGN_SCHEME_RSA_PSS_SHA384);
  r_assert_cmphex (r_tls_hello_ext_sign_scheme (&ext,  4), ==, R_TLS_SIGN_SCHEME_RSA_PKCS1_SHA384);
  r_assert_cmphex (r_tls_hello_ext_sign_scheme (&ext,  5), ==, R_TLS_SIGN_SCHEME_ECDSA_SECP384R1_SHA384);
  r_assert_cmphex (r_tls_hello_ext_sign_scheme (&ext,  6), ==, R_TLS_SIGN_SCHEME_RSA_PSS_SHA256);
  r_assert_cmphex (r_tls_hello_ext_sign_scheme (&ext,  7), ==, R_TLS_SIGN_SCHEME_RSA_PKCS1_SHA256);
  r_assert_cmphex (r_tls_hello_ext_sign_scheme (&ext,  8), ==, R_TLS_SIGN_SCHEME_ECDSA_SECP256R1_SHA256);
  r_assert_cmphex (r_tls_hello_ext_sign_scheme (&ext,  9), ==, R_TLS_SIGN_SCHEME_RSA_PKCS1_SHA1);
  r_assert_cmphex (r_tls_hello_ext_sign_scheme (&ext, 10), ==, R_TLS_SIGN_SCHEME_ECDSA_SHA1);
  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_hello_msg_extension_next (&msg, &ext));
  r_assert_cmpuint (ext.type, ==, R_TLS_EXT_TYPE_USE_SRTP);
  r_assert_cmpuint (ext.len, ==, 7);
  r_assert_cmpuint (r_tls_hello_ext_use_srtp_profile_count (&ext), ==, 2);
  r_assert_cmphex (r_tls_hello_ext_use_srtp_profile (&ext, 0), ==, R_DTLS_SRTP_AES128_CM_HMAC_SHA1_32);
  r_assert_cmphex (r_tls_hello_ext_use_srtp_profile (&ext, 1), ==, R_DTLS_SRTP_AES128_CM_HMAC_SHA1_80);
  r_assert_cmpuint (r_tls_hello_ext_use_srtp_mki_size (&ext), ==, 0);
  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_hello_msg_extension_next (&msg, &ext));
  r_assert_cmpuint (ext.type, ==, R_TLS_EXT_TYPE_EC_POINT_FORMATS);
  r_assert_cmpuint (ext.len, ==, 2);
  r_assert_cmpuint (r_tls_hello_ext_ec_point_format_count (&ext), ==, 1);
  r_assert_cmphex (r_tls_hello_ext_ec_point_format (&ext, 0), ==, R_TLS_EC_POINT_FORMAT_UNCOMPRESSED);
  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_hello_msg_extension_next (&msg, &ext));
  r_assert_cmpuint (ext.type, ==, R_TLS_EXT_TYPE_SUPPORTED_GROUPS);
  r_assert_cmpuint (ext.len, ==, 8);
  r_assert_cmpuint (r_tls_hello_ext_supported_groups_count (&ext), ==, 3);
  r_assert_cmphex (r_tls_hello_ext_supported_group (&ext, 0), ==, R_TLS_SUPPORTED_GROUP_X25519);
  r_assert_cmphex (r_tls_hello_ext_supported_group (&ext, 1), ==, R_TLS_SUPPORTED_GROUP_SECP256R1);
  r_assert_cmphex (r_tls_hello_ext_supported_group (&ext, 2), ==, R_TLS_SUPPORTED_GROUP_SECP384R1);
  r_assert_cmpint (R_TLS_ERROR_EOB, ==, r_tls_hello_msg_extension_next (&msg, &ext));
}
RTEST_END;

RTEST (rtls, parse_tls_server_hello, RTEST_FAST)
{
  static const ruint8 pkt_tls_server_hallo[] = {
    0x16, 0x03, 0x03, 0x00, 0x41, 0x02, 0x00, 0x00, 0x3d, 0x03, 0x03, 0xe2, 0xbf, 0xfe, 0xd7, 0x55,
    0x11, 0xa0, 0x9a, 0xa0, 0x63, 0x7b, 0x10, 0x01, 0xe7, 0xec, 0x32, 0x24, 0xe2, 0x94, 0x48, 0xfd,
    0x6e, 0x9b, 0x56, 0xa1, 0xc1, 0xfc, 0x6a, 0xd1, 0x60, 0x4b, 0x65, 0x00, 0xc0, 0x2f, 0x00, 0x00,
    0x15, 0x00, 0x00, 0x00, 0x00, 0xff, 0x01, 0x00, 0x01, 0x00, 0x00, 0x0b, 0x00, 0x04, 0x03, 0x00,
    0x01, 0x02, 0x00, 0x23, 0x00, 0x00
  };
  RTLSParser parser;
  RTLSHandshakeType hstype;
  ruint32 hslen;
  RTLSHelloMsg msg;
  RTLSHelloExt ext;

  r_assert_cmpint (R_TLS_ERROR_OK, ==,
      r_tls_parser_init (&parser, pkt_tls_server_hallo, sizeof (pkt_tls_server_hallo)));
  r_assert_cmpuint (parser.content, ==, R_TLS_CONTENT_TYPE_HANDSHAKE);
  r_assert_cmpuint (parser.version, ==, R_TLS_VERSION_TLS_1_2);
  r_assert_cmpuint (parser.fraglen, ==, 65);

  r_assert_cmpint (R_TLS_ERROR_OK, ==,
      r_tls_parser_parse_handshake (&parser, &hstype, &hslen));
  r_assert_cmpuint (hstype, ==, R_TLS_HANDSHAKE_TYPE_SERVER_HELLO);
  r_assert_cmpuint (hslen, ==, 61);
  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_parser_parse_hello (&parser, &msg));
  r_assert_cmpuint (msg.version, ==, R_TLS_VERSION_TLS_1_2);
  r_assert_cmpuint (msg.sidlen, ==, 0);
  r_assert_cmpuint (msg.cookielen, ==, 0);
  r_assert_cmpuint (msg.cslen, ==, 2);
  r_assert_cmpuint (msg.complen, ==, 1);
  r_assert_cmpuint (msg.extlen, ==, 21);

  r_assert_cmpuint (r_tls_hello_msg_cipher_suite_count (&msg), ==, 1);
  r_assert_cmphex (r_tls_hello_msg_cipher_suite (&msg,  0), ==, R_CS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256);
  r_assert_cmpuint (r_tls_hello_msg_compression_count (&msg), ==, 1);
  r_assert_cmpuint (r_tls_hello_msg_compression_method (&msg, 0), ==, R_TLS_COMPRESSION_NULL);

  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_hello_msg_extension_first (&msg, &ext));
  r_assert_cmpuint (ext.type, ==, R_TLS_EXT_TYPE_SERVER_NAME);
  r_assert_cmpuint (ext.len, ==, 0);
  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_hello_msg_extension_next (&msg, &ext));
  r_assert_cmpuint (ext.type, ==, R_TLS_EXT_TYPE_RENEGOTIATION_INFO);
  r_assert_cmpuint (ext.len, ==, 1);
  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_hello_msg_extension_next (&msg, &ext));
  r_assert_cmpuint (ext.type, ==, R_TLS_EXT_TYPE_EC_POINT_FORMATS);
  r_assert_cmpuint (ext.len, ==, 4);
  r_assert_cmpuint (r_tls_hello_ext_ec_point_format_count (&ext), ==, 3);
  r_assert_cmphex (r_tls_hello_ext_ec_point_format (&ext, 0), ==, R_TLS_EC_POINT_FORMAT_UNCOMPRESSED);
  r_assert_cmphex (r_tls_hello_ext_ec_point_format (&ext, 1), ==, R_TLS_EC_POINT_FORMAT_ANSIX962_COMPRESSED_PRIME);
  r_assert_cmphex (r_tls_hello_ext_ec_point_format (&ext, 2), ==, R_TLS_EC_POINT_FORMAT_ANSIX962_COMPRESSED_CHAR2);
  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_hello_msg_extension_next (&msg, &ext));
  r_assert_cmpuint (ext.type, ==, R_TLS_EXT_TYPE_SESSION_TICKET);
  r_assert_cmpuint (ext.len, ==, 0);
  r_assert_cmpint (R_TLS_ERROR_EOB, ==, r_tls_hello_msg_extension_next (&msg, &ext));
}
RTEST_END;

RTEST (rtls, parse_dtls_certificate, RTEST_FAST)
{
  static const ruint8 pkt_dtls_certificate[] = {
    0x16, 0xfe, 0xfd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xba, 0x0b, 0x00, 0x02,
    0xae, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0xae, 0x00, 0x02, 0xab, 0x00, 0x02, 0xa8, 0x30,
    0x82, 0x02, 0xa4, 0x30, 0x82, 0x01, 0x8c, 0x02, 0x09, 0x00, 0xa0, 0x7c, 0xaa, 0xd1, 0x5f, 0x10,
    0x9a, 0xef, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05,
    0x00, 0x30, 0x14, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x09, 0x6c, 0x6f,
    0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x30, 0x1e, 0x17, 0x0d, 0x31, 0x36, 0x31, 0x30, 0x31,
    0x31, 0x31, 0x33, 0x34, 0x35, 0x32, 0x38, 0x5a, 0x17, 0x0d, 0x31, 0x37, 0x31, 0x30, 0x31, 0x31,
    0x31, 0x33, 0x34, 0x35, 0x32, 0x38, 0x5a, 0x30, 0x14, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55,
    0x04, 0x03, 0x0c, 0x09, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x30, 0x82, 0x01,
    0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00,
    0x03, 0x82, 0x01, 0x0f, 0x00, 0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xc5, 0xe6,
    0x9c, 0xc4, 0x2c, 0x83, 0x7b, 0xe1, 0xaa, 0x3e, 0xa0, 0x83, 0x62, 0x86, 0x87, 0xea, 0x3b, 0xf9,
    0x6e, 0x3f, 0x3a, 0xdf, 0xb1, 0xf2, 0x5a, 0x3f, 0xa4, 0xf1, 0xef, 0x13, 0x3e, 0x93, 0xe9, 0x61,
    0x90, 0x2b, 0xe3, 0x90, 0x96, 0x2a, 0x53, 0x31, 0x54, 0x13, 0xe1, 0x76, 0xd7, 0xb8, 0xc6, 0xe5,
    0xbd, 0x97, 0xc5, 0x5c, 0x5f, 0xb2, 0xef, 0x7f, 0x2c, 0x02, 0x72, 0xc4, 0xd1, 0x8c, 0xd5, 0x43,
    0x29, 0x8d, 0xae, 0xae, 0xc2, 0x37, 0x35, 0xb6, 0x1e, 0x35, 0x6c, 0x17, 0x92, 0x26, 0x60, 0x8f,
    0xae, 0x42, 0x08, 0x2e, 0x8d, 0x8a, 0x43, 0x91, 0x64, 0xc7, 0x08, 0xfd, 0xb7, 0x07, 0x63, 0x6e,
    0x33, 0xca, 0x10, 0x53, 0x06, 0x4a, 0x74, 0x38, 0x8a, 0xe1, 0xc9, 0xdf, 0x8f, 0xb6, 0x68, 0xcb,
    0x31, 0xfb, 0x56, 0xc6, 0x39, 0x9e, 0x3b, 0xd7, 0xa9, 0xed, 0x6f, 0x6b, 0x8a, 0xb7, 0x2d, 0x9e,
    0x4a, 0x1d, 0xa1, 0x83, 0x1c, 0xe7, 0x21, 0x2f, 0x79, 0xad, 0x1a, 0x9f, 0x9d, 0xe8, 0x26, 0xc3,
    0xad, 0x69, 0xdc, 0x72, 0x83, 0x8c, 0x29, 0xd2, 0xe7, 0xfe, 0xaa, 0x85, 0x07, 0x96, 0x39, 0x3e,
    0x13, 0x04, 0x68, 0xd2, 0xa7, 0x03, 0x83, 0x2a, 0x1f, 0x3f, 0xab, 0x10, 0x03, 0x89, 0x39, 0xe5,
    0x84, 0xc9, 0xad, 0xa3, 0x6e, 0x5f, 0xaa, 0xd8, 0x2b, 0xbb, 0xf9, 0x71, 0xd8, 0x1c, 0x29, 0xca,
    0xf0, 0x71, 0xad, 0x4e, 0xa3, 0x43, 0x1b, 0x46, 0x73, 0x3e, 0x3a, 0x92, 0x01, 0x92, 0x65, 0xd4,
    0xf7, 0x56, 0x71, 0xad, 0xa2, 0x10, 0xb9, 0x54, 0x2d, 0x1f, 0xf5, 0xa1, 0x2b, 0xe3, 0x66, 0x7a,
    0xea, 0xfb, 0x51, 0x22, 0xf4, 0x67, 0x19, 0x52, 0x5e, 0xf0, 0xfe, 0x7d, 0xf7, 0x56, 0x4f, 0x11,
    0x0e, 0xcc, 0x5b, 0x2c, 0xdf, 0xce, 0xf8, 0x6a, 0x4e, 0xac, 0x7f, 0x06, 0x1b, 0x63, 0x02, 0x03,
    0x01, 0x00, 0x01, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b,
    0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x90, 0x3c, 0xae, 0x57, 0xa6, 0xa4, 0x80, 0x45, 0x9d,
    0x0e, 0x11, 0xc2, 0xeb, 0x77, 0x97, 0x66, 0x9a, 0x2c, 0xdb, 0x8a, 0x76, 0xaf, 0x19, 0x2f, 0x11,
    0x74, 0x1c, 0x2a, 0x7c, 0x61, 0x25, 0x17, 0xc0, 0x54, 0xe7, 0xae, 0x1a, 0xe0, 0x3e, 0x81, 0xaa,
    0x4c, 0x01, 0xcb, 0x16, 0x84, 0xe8, 0x74, 0x2c, 0xad, 0xff, 0xce, 0xaa, 0x49, 0x48, 0xcf, 0x21,
    0x45, 0x1d, 0xfc, 0x44, 0x5a, 0x60, 0xf5, 0x51, 0xaa, 0x2d, 0x63, 0x50, 0xb9, 0xca, 0x70, 0x48,
    0x06, 0x32, 0x5e, 0x9b, 0xf3, 0xfa, 0xec, 0x42, 0x01, 0x74, 0xc8, 0x7b, 0x74, 0xcb, 0xc8, 0x37,
    0xa7, 0xa3, 0x65, 0xca, 0x15, 0x98, 0xab, 0x4f, 0x95, 0xc2, 0xdb, 0xcf, 0xa2, 0x28, 0xe1, 0x99,
    0x87, 0xac, 0xd5, 0xa6, 0xcf, 0x4f, 0xad, 0xe7, 0xeb, 0x2f, 0xf2, 0xcc, 0xf8, 0xdc, 0xcf, 0xf0,
    0xef, 0xf9, 0xf6, 0x61, 0xcc, 0x1c, 0x84, 0x35, 0x4d, 0xff, 0x8f, 0x66, 0xda, 0x40, 0x46, 0x01,
    0x22, 0xf3, 0x1e, 0xf6, 0xe8, 0x48, 0x0f, 0x6e, 0xdd, 0x4a, 0x21, 0x61, 0x66, 0x62, 0xe2, 0xc1,
    0xe6, 0x10, 0x39, 0xe1, 0xaa, 0x38, 0x2c, 0x30, 0x80, 0x13, 0xd3, 0xa7, 0x17, 0x08, 0x07, 0x3a,
    0xa5, 0xf3, 0x0a, 0x6e, 0x00, 0xe1, 0x20, 0x58, 0x42, 0xd3, 0x45, 0x62, 0x04, 0x95, 0x1d, 0xa5,
    0xab, 0x26, 0x4d, 0x94, 0xde, 0xfd, 0x25, 0x6e, 0xd2, 0xc7, 0x82, 0x0f, 0x73, 0xe1, 0x7d, 0xeb,
    0x8f, 0x38, 0x72, 0xd9, 0xd6, 0x1b, 0x1f, 0x93, 0xe0, 0x2e, 0xf7, 0x77, 0xe5, 0x59, 0x24, 0xfe,
    0x83, 0xda, 0xe0, 0x71, 0x4a, 0xa2, 0x93, 0x28, 0x86, 0x8f, 0xa5, 0xd2, 0x00, 0xec, 0x67, 0x64,
    0xb4, 0x46, 0x54, 0xc0, 0xb5, 0xe8, 0x89, 0x8c, 0x3b, 0x77, 0xaa, 0xda, 0x4d, 0x20, 0x19, 0xa8,
    0xc1, 0x3e, 0x3c, 0x9e, 0x22, 0x0a, 0x85
  };

  RTLSParser parser;
  RTLSHandshakeType hstype;
  ruint32 hslen;
  RTLSCertificate tlscert = R_TLS_CERTIFICATE_INIT;
  RCryptoCert * cert;
  RCryptoKey * pk;

  r_assert_cmpint (R_TLS_ERROR_OK, ==,
      r_tls_parser_init (&parser, pkt_dtls_certificate, sizeof (pkt_dtls_certificate)));
  r_assert_cmpuint (parser.content, ==, R_TLS_CONTENT_TYPE_HANDSHAKE);
  r_assert_cmpuint (parser.version, ==, R_TLS_VERSION_DTLS_1_2);
  r_assert_cmpuint (parser.epoch, ==, 0);
  r_assert_cmpuint (parser.seqno, ==, 1);
  r_assert_cmpuint (parser.fraglen, ==, 698);

  r_assert (r_tls_parser_dtls_is_complete_handshake_fragment (&parser));
  r_assert_cmpint (R_TLS_ERROR_OK, ==,
      r_tls_parser_parse_handshake (&parser, &hstype, &hslen));
  r_assert_cmpuint (hstype, ==, R_TLS_HANDSHAKE_TYPE_CERTIFICATE);
  r_assert_cmpuint (hslen, ==, 686);

  r_assert_cmpint (R_TLS_ERROR_OK, ==,
      r_tls_parser_parse_certificate_next (&parser, &tlscert));
  r_assert_cmpptr ((cert = r_tls_certificate_get_cert (&tlscert)), !=, NULL);
  r_assert_cmpint (R_TLS_ERROR_EOB, ==,
      r_tls_parser_parse_certificate_next (&parser, &tlscert));

  r_assert_cmpint (r_crypto_cert_get_type (cert), ==, R_CRYPTO_CERT_X509);
  r_assert_cmpstr (r_crypto_x509_cert_issuer (cert), ==, "CN=localhost");
  r_assert_cmpstr (r_crypto_x509_cert_subject (cert), ==, "CN=localhost");
  r_assert (r_crypto_x509_cert_is_self_issued (cert));
  r_assert (r_crypto_x509_cert_is_self_signed (cert));

  r_assert_cmpptr ((pk = r_crypto_cert_get_public_key (cert)), !=, NULL);
  r_crypto_cert_unref (cert);

  r_assert (!r_crypto_key_has_private_key (pk));
  r_assert_cmpuint (r_crypto_key_get_algo (pk), ==, R_CRYPTO_ALGO_RSA);
  r_assert_cmpuint (r_crypto_key_get_bitsize (pk), ==, 2048);

  r_crypto_key_unref (pk);
}
RTEST_END;

RTEST (rtls, parse_dtls_certificate_request, RTEST_FAST)
{
  static const ruint8 pkt_dtls_certificate_request[] = {
    0x16, 0xfe, 0xfd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x32, 0x0d, 0x00, 0x00,
    0x26, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x03, 0x01, 0x02, 0x40, 0x00, 0x1e, 0x06,
    0x01, 0x06, 0x02, 0x06, 0x03, 0x05, 0x01, 0x05, 0x02, 0x05, 0x03, 0x04, 0x01, 0x04, 0x02, 0x04,
    0x03, 0x03, 0x01, 0x03, 0x02, 0x03, 0x03, 0x02, 0x01, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00
  };

  RTLSParser parser;
  RTLSHandshakeType hstype;
  ruint32 hslen;
  RTLSCertReq req;

  r_assert_cmpint (R_TLS_ERROR_OK, ==, r_tls_parser_init (&parser,
        pkt_dtls_certificate_request, sizeof (pkt_dtls_certificate_request)));
  r_assert_cmpuint (parser.content, ==, R_TLS_CONTENT_TYPE_HANDSHAKE);
  r_assert_cmpuint (parser.version, ==, R_TLS_VERSION_DTLS_1_2);
  r_assert_cmpuint (parser.epoch, ==, 0);
  r_assert_cmpuint (parser.seqno, ==, 3);
  r_assert_cmpuint (parser.fraglen, ==, 50);

  r_assert (r_tls_parser_dtls_is_complete_handshake_fragment (&parser));
  r_assert_cmpint (R_TLS_ERROR_OK, ==,
      r_tls_parser_parse_handshake (&parser, &hstype, &hslen));
  r_assert_cmpuint (hstype, ==, R_TLS_HANDSHAKE_TYPE_CERTIFICATE_REQUEST);
  r_assert_cmpuint (hslen, ==, 38);

  r_assert_cmpint (R_TLS_ERROR_OK, ==,
      r_tls_parser_parse_certificate_request (&parser, &req));

  r_assert_cmpuint (req.certtypecount, ==, 3);
  r_assert_cmphex (r_tls_cert_req_cert_type (&req, 0), ==, R_TLS_CLIENT_CERT_TYPE_RSA_SIGN);
  r_assert_cmphex (r_tls_cert_req_cert_type (&req, 1), ==, R_TLS_CLIENT_CERT_TYPE_DSS_SIGN);
  r_assert_cmphex (r_tls_cert_req_cert_type (&req, 2), ==, R_TLS_CLIENT_CERT_TYPE_ECDSA_SIGN);

  r_assert_cmpuint (req.signschemecount, ==, 15);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req,  0), ==, R_TLS_SIGN_SCHEME_RSA_PKCS1_SHA512);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req,  1), ==, R_TLS_SIGN_SCHEME_DSA_SHA512);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req,  2), ==, R_TLS_SIGN_SCHEME_ECDSA_SECP521R1_SHA512);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req,  3), ==, R_TLS_SIGN_SCHEME_RSA_PKCS1_SHA384);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req,  4), ==, R_TLS_SIGN_SCHEME_DSA_SHA384);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req,  5), ==, R_TLS_SIGN_SCHEME_ECDSA_SECP384R1_SHA384);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req,  6), ==, R_TLS_SIGN_SCHEME_RSA_PKCS1_SHA256);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req,  7), ==, R_TLS_SIGN_SCHEME_DSA_SHA256);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req,  8), ==, R_TLS_SIGN_SCHEME_ECDSA_SECP256R1_SHA256);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req,  9), ==, R_TLS_SIGN_SCHEME_RSA_PKCS1_SHA224);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req, 10), ==, R_TLS_SIGN_SCHEME_DSA_SHA224);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req, 11), ==, R_TLS_SIGN_SCHEME_ECDSA_SECP224R1_SHA224);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req, 12), ==, R_TLS_SIGN_SCHEME_RSA_PKCS1_SHA1);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req, 13), ==, R_TLS_SIGN_SCHEME_DSA_SHA1);
  r_assert_cmphex (r_tls_cert_req_sign_scheme (&req, 14), ==, R_TLS_SIGN_SCHEME_ECDSA_SHA1);

  r_assert_cmpuint (req.cacount, ==, 0);
}
RTEST_END;


