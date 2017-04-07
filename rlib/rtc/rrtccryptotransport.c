/* RLIB - Convenience library for useful things
 * Copyright (C) 2017  Haakon Sporsheim <haakon.sporsheim@gmail.com>
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
#include "rrtc-private.h"
#include <rlib/rtc/rrtcsession.h>

#include <rlib/rassert.h>
#include <rlib/rstr.h>

static void
r_rtc_crypto_transport_free (RRtcCryptoTransport * crypto)
{
  if (crypto->ice != NULL) {
    r_rtc_ice_transport_close (crypto->ice);
    r_rtc_ice_transport_clear_cb (crypto->ice);
    r_rtc_ice_transport_unref (crypto->ice);
  }

  if (crypto->dtlssrv != NULL)
    r_tls_server_unref (crypto->dtlssrv);
  /*if (crypto->dtlscli != NULL)*/
    /*r_tls_client_unref (crypto->dtlscli);*/
  if (crypto->srtp != NULL)
    r_srtp_ctx_unref (crypto->srtp);

  r_rtc_rtp_listener_unref (crypto->listener);

  if (crypto->loop != NULL)
    r_ev_loop_unref (crypto->loop);
  if (crypto->prng != NULL)
    r_prng_unref (crypto->prng);

  r_free (crypto);
}

static void
r_rtc_crypto_srv_hs_done (rpointer data, RTLSServer * srv)
{
  RRtcCryptoTransport * crypto = data;
  RSRTPCipherSuite cs;
  const RSRTPCipherSuiteInfo * csinfo;
  ruint8 * material;
  rsize msize;
  RTLSError tlserr;

  cs = r_tls_server_get_dtls_srtp_profile (srv);
  if ((csinfo = r_srtp_cipher_suite_get_info (cs)) == NULL) {
    R_LOG_WARNING ("0x%.04x DTLS-SRTP profile not supported", (ruint)cs);
    return;
  }

  msize = 2 * ((csinfo->cipher->keybits + csinfo->saltbits) / 8);
  material = r_alloca (msize);
  if ((tlserr = r_tls_server_export_keying_matierial (srv, material, msize,
      R_STR_WITH_SIZE_ARGS ("EXTRACTOR-dtls_srtp"), NULL, 0)) == R_TLS_ERROR_OK) {
    ruint8 * clikey = r_alloca (msize / 2);
    ruint8 * srvkey = r_alloca (msize / 2);
    ruint8 * ptr = material;
    RSRTPError srtperr;

    r_memcpy (clikey, ptr, csinfo->cipher->keybits / 8); ptr += csinfo->cipher->keybits / 8;
    r_memcpy (srvkey, ptr, csinfo->cipher->keybits / 8); ptr += csinfo->cipher->keybits / 8;
    r_memcpy (clikey + csinfo->cipher->keybits / 8, ptr, csinfo->saltbits / 8); ptr += csinfo->saltbits / 8;
    r_memcpy (srvkey + csinfo->cipher->keybits / 8, ptr, csinfo->saltbits / 8); ptr += csinfo->saltbits / 8;

    if ((srtperr = r_srtp_add_crypto_context_with_filter (crypto->srtp,
            R_SRTP_FILTER_ANY, cs, clikey)) == R_SRTP_ERROR_OK) {
      R_LOG_INFO ("Added crypto context %s for DTLS-SRTP", csinfo->str);
      R_LOG_MEM_DUMP (R_LOG_LEVEL_INFO, clikey, msize / 2);
      r_rtc_rtp_listener_notify_ready (crypto->listener, crypto);
    } else {
      R_LOG_WARNING ("Couldn't add crypto context for SRTP err %d",
          (ruint)srtperr);
    }
  } else {
    R_LOG_WARNING ("Couldn't export keying material for SRTP from DTLS err %d",
        (ruint)tlserr);
  }
}

static rboolean
r_rtc_crypto_srv_buffer_out (rpointer data, RBuffer * buf, RTLSServer * srv)
{
  RRtcCryptoTransport * crypto = data;
  (void) srv;

  R_LOG_TRACE ("RtcCryptoTransport %p %p %"RSIZE_FMT,
      crypto, buf, r_buffer_get_size (buf));
  r_rtc_ice_transport_send (crypto->ice, buf);
  return TRUE;
}

static rboolean
r_rtc_crypto_srv_buffer_appdata (rpointer data, RBuffer * buf, RTLSServer * srv)
{
  RRtcCryptoTransport * crypto = data;
  RMemMapInfo info = R_MEM_MAP_INFO_INIT;
  (void) crypto;
  (void) srv;

  if (r_buffer_map (buf, &info, R_MEM_MAP_READ)) {
    R_LOG_MEM_DUMP (R_LOG_LEVEL_TRACE, info.data, info.size);
    r_buffer_unmap (buf, &info);
  }

  return TRUE;
}

static void
r_rtc_crypto_transport_ice_ready (rpointer data, rpointer ctx)
{
  RRtcCryptoTransport * crypto = data;
  RRtcIceTransport * ice = ctx;

  r_assert_cmpptr (ice, ==, crypto->ice);

  R_LOG_TRACE ("RtcCryptoTransport %p ice ready %p", crypto, ice);
  /* FIXME: Do something? */
}

static void
r_rtc_crypto_transport_ice_close (rpointer data, rpointer ctx)
{
  RRtcCryptoTransport * crypto = data;
  RRtcIceTransport * ice = ctx;

  r_assert_cmpptr (ice, ==, crypto->ice);

  /* FIXME: State change? */
  r_rtc_ice_transport_clear_cb (crypto->ice);
  r_rtc_rtp_listener_notify_close (crypto->listener, crypto);
}

static void
r_rtc_crypto_transport_srv_ice_packet (rpointer data, RBuffer * buf, rpointer ctx)
{
  RRtcCryptoTransport * crypto = data;
  RRtcIceTransport * ice = ctx;
  RMemMapInfo info = R_MEM_MAP_INFO_INIT;
  RSRTPError err;
  RBuffer * decrypt;

  r_assert_cmpptr (ice, ==, crypto->ice);

  if (r_buffer_map (buf, &info, R_MEM_MAP_READ)) {
    if (r_rtp_is_valid_hdr (info.data, info.size)) {
      r_buffer_unmap (buf, &info);
      if ((decrypt = r_srtp_decrypt_rtp (crypto->srtp, buf, &err)) != NULL) {
        R_LOG_TRACE ("RtcCryptoTransport %p RTP packet", crypto);
        r_rtc_rtp_listener_handle_rtp (crypto->listener, decrypt, crypto);
        r_buffer_unref (decrypt);
      } else {
        R_LOG_WARNING ("Unable to decrypt SRTP buffer %p (err: %d)", buf, (int)err);
      }
    } else if (r_rtcp_is_valid_hdr (info.data, info.size)) {
      r_buffer_unmap (buf, &info);
      if ((decrypt = r_srtp_decrypt_rtcp (crypto->srtp, buf, &err)) != NULL) {
        R_LOG_TRACE ("RtcCryptoTransport %p RTCP packet", crypto);
        r_rtc_rtp_listener_handle_rtcp (crypto->listener, decrypt, crypto);
        r_buffer_unref (decrypt);
      } else {
        R_LOG_WARNING ("Unable to decrypt SRTCP buffer %p (err: %d)", buf, (int)err);
      }
    } else if (r_tls_version_is_dtls (r_tls_parse_data_shallow (info.data, info.size))) {
      R_LOG_TRACE ("RtcCryptoTransport %p DTLS packet %"RSIZE_FMT, crypto, info.size);
      r_buffer_unmap (buf, &info);
      if (!r_tls_server_incoming_data (crypto->dtlssrv, buf)) {
        R_LOG_WARNING ("r_tls_server_incoming_data failed!");
      }
    } else {
      R_LOG_WARNING ("Unknown packet received");
      r_buffer_unmap (buf, &info);
    }
  } else {
    R_LOG_WARNING ("Unable to map buffer %p", buf);
  }
}

RRtcCryptoTransport *
r_rtc_crypto_transport_new (RRtcIceTransport * ice, RPrng * prng,
    RRtcCryptoRole role, RCryptoCert * cert, RCryptoKey * privkey)
{
  RRtcCryptoTransport * ret;
  static RTLSCallbacks cbs = {
    NULL,
    r_rtc_crypto_srv_hs_done,
    r_rtc_crypto_srv_buffer_out,
    r_rtc_crypto_srv_buffer_appdata,
  };

  if (R_UNLIKELY (ice == NULL)) return NULL;
  if (R_UNLIKELY (prng == NULL)) return NULL;

  /* FIXME: support other roles */
  r_assert_cmpint (role, ==, R_RTC_CRYPTO_ROLE_SERVER);
  /* FIXME: support raw transport? */
  r_assert_cmpptr (cert, !=, NULL);
  r_assert_cmpptr (privkey, !=, NULL);

  if ((ret = r_mem_new0 (RRtcCryptoTransport)) != NULL) {
    r_ref_init (ret, r_rtc_crypto_transport_free);

    R_LOG_TRACE ("New RtcCryptoTransport %p", ret);
    ret->listener = r_rtc_rtp_listener_new ();
    ret->srtp = r_srtp_ctx_new ();

    ret->dtlssrv = r_tls_server_new (&cbs, ret, NULL);
    r_tls_server_set_cert (ret->dtlssrv, cert, privkey);
    ret->prng = r_prng_ref (prng);
    ret->ice = r_rtc_ice_transport_ref (ice);
    r_rtc_ice_transport_set_cb (ret->ice,
        r_rtc_crypto_transport_ice_ready, r_rtc_crypto_transport_ice_close,
        r_rtc_crypto_transport_srv_ice_packet, ret, NULL);
  }

  return ret;
}

RRtcError
r_rtc_crypto_transport_send (RRtcCryptoTransport * crypto, RBuffer * buf)
{
  RRtcError ret;
  RMemMapInfo info = R_MEM_MAP_INFO_INIT;

  if (r_buffer_map (buf, &info, R_MEM_MAP_READ)) {
    RSRTPError srtperr;

    if (r_rtp_is_valid_hdr (info.data, info.size)) {
      r_buffer_unmap (buf, &info);
      if ((buf = r_srtp_encrypt_rtp (crypto->srtp, buf, &srtperr)) != NULL)
        ret = r_rtc_ice_transport_send (crypto->ice, buf);
      else
        ret = R_RTC_ENCRYPT_ERROR;
    } else if (r_rtcp_is_valid_hdr (info.data, info.size)) {
      r_buffer_unmap (buf, &info);
      if ((buf = r_srtp_encrypt_rtcp (crypto->srtp, buf, &srtperr)) != NULL)
        ret = r_rtc_ice_transport_send (crypto->ice, buf);
      else
        ret = R_RTC_ENCRYPT_ERROR;
    } else {
      ret = R_RTC_INVALID_MEDIA;
    }
  } else {
    ret = R_RTC_MAP_ERROR;
  }

  return ret;
}

RRtcError
r_rtc_crypto_transport_start (RRtcCryptoTransport * crypto, REvLoop * loop)
{
  RRtcError ret = R_RTC_OK;

  if (R_UNLIKELY (loop == NULL)) return R_RTC_INVAL;
  if (R_UNLIKELY (crypto->loop != NULL)) return R_RTC_WRONG_STATE;

  crypto->loop = r_ev_loop_ref (loop);
  if (crypto->dtlssrv != NULL) {
    if (r_tls_server_start (crypto->dtlssrv, loop, crypto->prng) != R_TLS_ERROR_OK)
      ret = R_RTC_WRONG_STATE;
  }
  r_rtc_ice_transport_start (crypto->ice, loop);

  R_LOG_TRACE ("RtcCryptoTransport %p start %d", crypto, (int)ret);

  return ret;
}

#if 0
RRtcError
r_rtc_crypto_transport_close (RRtcCryptoTransport * crypto)
{
}
#endif

