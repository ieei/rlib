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
#ifndef __R_RTC_RTP_SENDER_H__
#define __R_RTC_RTP_SENDER_H__

#if !defined(__RLIB_H_INCLUDE_GUARD__) && !defined(RLIB_COMPILATION)
#error "#include <rlib.h> only pelase."
#endif

#include <rlib/rtypes.h>
#include <rlib/rtc/rrtctypes.h>
#include <rlib/rref.h>

#include <rlib/rtc/rrtcrtpparameters.h>

#include <rlib/ev/revloop.h>

R_BEGIN_DECLS

typedef struct {
  RRtcEventCb       ready; /* Handshake done, ready for data */
  RRtcEventCb       close;
} RRtcRtpSenderCallbacks;

typedef struct _RRtcRtpSender RRtcRtpSender;

#define r_rtc_rtp_sender_ref        r_ref_ref
#define r_rtc_rtp_sender_unref      r_ref_unref

R_API const rchar * r_rtc_rtp_sender_get_id (RRtcRtpSender * s);
R_API const rchar * r_rtc_rtp_sender_get_mid (RRtcRtpSender * s);
R_API RRtcError r_rtc_rtp_sender_start (RRtcRtpSender * s,
    RRtcRtpParameters * params, REvLoop * loop);
R_API RRtcError r_rtc_rtp_sender_stop (RRtcRtpSender * s);
R_API RRtcError r_rtc_rtp_sender_send (RRtcRtpSender * s, RBuffer * packet);

R_END_DECLS

#endif /* __R_RTC_RTP_SENDER_H__ */

