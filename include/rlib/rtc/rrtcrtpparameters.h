/* RLIB - Convenience library for useful things
 * Copyright (C) 2017 Haakon Sporsheim <haakon.sporsheim@gmail.com>
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
#ifndef __R_RTC_RTP_PARAMETERS_H__
#define __R_RTC_RTP_PARAMETERS_H__

#if !defined(__RLIB_H_INCLUDE_GUARD__) && !defined(RLIB_COMPILATION)
#error "#include <rlib.h> only pelase."
#endif

#include <rlib/rtypes.h>

#include <rlib/rptrarray.h>
#include <rlib/rref.h>

#include <rlib/net/proto/rrtp.h>

#include <rlib/rtc/rrtc.h>

R_BEGIN_DECLS

typedef enum {
  R_RTC_RTCP_NONE       = 0,
  R_RTC_RTCP_MUX        = (1 << 0),
  R_RTC_RTCP_RSIZE      = (1 << 1),
} RRtcRtcpFlags;

typedef enum {
  R_RTC_PRIORITY_NONE = 0,
  R_RTC_PRIORITY_VERY_LOW,
  R_RTC_PRIORITY_LOW,
  R_RTC_PRIORITY_MEDIUM,
  R_RTC_PRIORITY_HIGH,
} RRtcPriorityType;

typedef struct {
  rchar *         name;
  rchar *         mime;
  RRTPPayloadType pt;
  ruint           rate;
  ruint           maxptime;
  ruint           ptime;
  ruint           channels;
  RPtrArray       rtcpfb; /* StrKV */
  RPtrArray       params; /* ??? */
} RRtcRtpCodecParameters;

R_API RRtcRtpCodecParameters * r_rtc_rtp_codec_parameters_new (const rchar * name,
    rssize size, RRTPPayloadType pt, ruint rate, ruint ch) R_ATTR_MALLOC;
R_API void r_rtc_rtp_codec_parameters_init (RRtcRtpCodecParameters * p);
R_API void r_rtc_rtp_codec_parameters_clear (RRtcRtpCodecParameters * p);
R_API RRtcRtpCodecParameters * r_rtc_rtp_codec_parameters_dup (
    const RRtcRtpCodecParameters * p);

typedef struct {
  ruint32 ssrc;
  rchar * mechanism;
} RRtcRtpFecParameters;

typedef struct {
  ruint32 ssrc;
} RRtcRtpRtxParameters;

typedef struct {
  ruint32 ssrc;
  RRTPPayloadType pt;
  RRtcPriorityType pri;

  RRtcRtpFecParameters fec;
  RRtcRtpRtxParameters rtx;

  rboolean active;
  rchar id[16 + 1];
  ruint64 maxbr;

  ruint maxfr;
  rdouble scaleres;
  rdouble scalefr;
} RRtcRtpEncodingParameters;

R_API RRtcRtpEncodingParameters * r_rtc_rtp_encoding_parameters_new (ruint32 ssrc,
    RRTPPayloadType pt) R_ATTR_MALLOC;
R_API void r_rtc_rtp_encoding_parameters_init (RRtcRtpEncodingParameters * p);
R_API void r_rtc_rtp_encoding_parameters_clear (RRtcRtpEncodingParameters * p);
R_API RRtcRtpEncodingParameters * r_rtc_rtp_encoding_parameters_dup (
    const RRtcRtpEncodingParameters * p);

typedef struct {
  rchar * uri;
  ruint16 id;
  rboolean prefencrypt;
  RPtrArray params; /* ??? */
} RRtcRtpHdrExtParameters;

R_API RRtcRtpHdrExtParameters * r_rtc_rtp_hdrext_parameters_new (
    const rchar * uri, rssize size, ruint16 id) R_ATTR_MALLOC;
R_API void r_rtc_rtp_hdrext_parameters_init (RRtcRtpHdrExtParameters * p);
R_API void r_rtc_rtp_hdrext_parameters_clear (RRtcRtpHdrExtParameters * p);
R_API RRtcRtpHdrExtParameters * r_rtc_rtp_hdrext_parameters_dup (
    const RRtcRtpHdrExtParameters * p);


typedef struct {
  RRef ref;

  /* RRtcRtcpParameters */
  rchar * cname;
  ruint32 ssrc;
  RRtcRtcpFlags flags;

  rchar *     mid;
  RPtrArray   codecs;     /* RRtcRtpCodecParameters */
  RPtrArray   extensions; /* RRtcRtpHdrExtParameters */
  RPtrArray   encodings;  /* RRtcRtpEncodingParameters */
} RRtcRtpParameters;


R_API RRtcRtpParameters * r_rtc_rtp_parameters_new (const rchar * mid, rssize size) R_ATTR_MALLOC;
#define r_rtc_rtp_parameters_ref                r_ref_ref
#define r_rtc_rtp_parameters_unref              r_ref_unref

#define r_rtc_rtp_parameters_mid(p)             ((p)->mid)
#define r_rtc_rtp_parameters_rtcp_cname(p)      ((p)->cname)
#define r_rtc_rtp_parameters_rtcp_ssrc(p)       ((p)->ssrc)
#define r_rtc_rtp_parameters_rtcp_flags(p)      ((p)->flags)
#define r_rtc_rtp_parameters_codec_count(p)     r_ptr_array_size (&(p)->codecs)
#define r_rtc_rtp_parameters_hdrext_count(p)    r_ptr_array_size (&(p)->extensions)
#define r_rtc_rtp_parameters_encoding_count(p)  r_ptr_array_size (&(p)->encodings)

R_API RRtcError r_rtc_rtp_parameters_set_rtcp (RRtcRtpParameters * params,
    const rchar * cname, rssize size, ruint32 ssrc, RRtcRtcpFlags flags);
R_API RRtcError r_rtc_rtp_parameters_take_codec (RRtcRtpParameters * params,
    RRtcRtpCodecParameters * codec);
#define r_rtc_rtp_parameters_add_codec(params, codec)                         \
  r_rtc_rtp_parameters_take_codec (params, r_rtc_rtp_codec_parameters_dup (codec))
#define r_rtc_rtp_parameters_add_codec_simple(params, name, pt, rate, ch)     \
  r_rtc_rtp_parameters_take_codec (params,                                    \
      r_rtc_rtp_codec_parameters_new (name, -1, pt, rate, ch))
R_API RRtcError r_rtc_rtp_parameters_take_encoding (RRtcRtpParameters * params,
    RRtcRtpEncodingParameters * encoding);
#define r_rtc_rtp_parameters_add_encoding(params, encoding)                   \
  r_rtc_rtp_parameters_take_encoding (params, r_rtc_rtp_encoding_parameters_dup (encoding))
#define r_rtc_rtp_parameters_add_encoding_simple(params, ssrc, pt)            \
  r_rtc_rtp_parameters_take_encoding (params,                                 \
      r_rtc_rtp_encoding_parameters_new (ssrc, pt))
R_API RRtcError r_rtc_rtp_parameters_take_hdrext (RRtcRtpParameters * params,
    RRtcRtpHdrExtParameters * extension);
#define r_rtc_rtp_parameters_add_hdrext(params, hdrext)                       \
  r_rtc_rtp_parameters_take_hdrext (params, r_rtc_rtp_hdrext_parameters_dup (hdrext))
#define r_rtc_rtp_parameters_add_hdrext_simple(params, uri, id)               \
  r_rtc_rtp_parameters_take_hdrext (params,                                   \
      r_rtc_rtp_hdrext_parameters_new (uri, -1, id))


R_END_DECLS

#endif /* __R_RTC_RTP_PARAMETERS_H__ */
