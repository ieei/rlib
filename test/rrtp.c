#include <rlib/rlib.h>

static const ruint8 pkt_stun[] = {
  0x00, 0x01, 0x00, 0x00, 0x21, 0x12, 0xa4, 0x42, 0x46, 0x76, 0x41, 0x31, 0x65, 0x6d, 0x75, 0x49,
  0x73, 0x6b, 0x4e, 0x59
};

static const ruint8 pkt_rtp_pcmu[] = {
  0x80, 0x80, 0x92, 0xdb, 0x00, 0x00, 0x00, 0xa0, 0x34, 0x3d, 0xa9, 0x9b, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0x7f,
  0xff, 0x7f, 0x7f, 0xff, 0xff, 0x7f, 0x7f, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfe, 0x7e, 0xfd, 0x7d,
  0xfd, 0x7e, 0x75, 0xfc, 0x73, 0x75, 0xfe, 0x71, 0x7b, 0x7e, 0x7a, 0xfc, 0xfd, 0xf9, 0xfb, 0xfb,
  0xf6, 0xff, 0xf9, 0xf8, 0x7c, 0xfa, 0xfd, 0x7d, 0xfc, 0xff, 0x7e, 0xfe, 0xfe, 0xfe, 0x7e, 0xfd,
  0x7e, 0x7d, 0xfe, 0x7c, 0x7c, 0x7d, 0x7a, 0x7b, 0x7b, 0x7c, 0x7d, 0x7f, 0xfd, 0xfb, 0xf8, 0xf5,
  0xf4, 0xf1, 0xf0, 0xf1, 0xf0, 0xf2, 0xf5, 0xf7, 0xfb, 0xff, 0x7a, 0x76, 0x71, 0x6e, 0x6d, 0x6b,
  0x6b, 0x6b, 0x6b, 0x6c, 0x6e, 0x70, 0x75, 0x7c, 0xf9, 0xf2, 0xeb, 0xe8, 0xe3, 0xdf, 0xde, 0xdb,
  0xe3, 0xdf, 0xe4, 0x7e, 0xf4, 0x6f, 0x62, 0x66, 0x5e, 0x5e, 0x5f, 0x60
};

static const ruint8 pkt_rtcp_sr_sdes[] = {
  0x80, 0xc8, 0x00, 0x06, 0xf3, 0xcb, 0x20, 0x01, 0x83, 0xab, 0x03, 0xa1, 0xeb, 0x02, 0x0b, 0x3a,
  0x00, 0x00, 0x94, 0x20, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x00, 0x9b, 0x88,
  0x81, 0xca, 0x00, 0x05, 0xf3, 0xcb, 0x20, 0x01, 0x01, 0x0a, 0x6f, 0x75, 0x74, 0x43, 0x68, 0x61,
  0x6e, 0x6e, 0x65, 0x6c, 0x00, 0x00, 0x00, 0x00
};


RTEST (rrtp, is_valid_hdr, RTEST_FAST)
{
  r_assert (!r_rtp_is_valid_hdr (pkt_stun, sizeof (pkt_stun)));
  r_assert (r_rtp_is_valid_hdr (pkt_rtp_pcmu, sizeof (pkt_rtp_pcmu)));
  r_assert (!r_rtp_is_valid_hdr (pkt_rtcp_sr_sdes, sizeof (pkt_rtcp_sr_sdes)));
}
RTEST_END;

RTEST (rrtcp, is_valid_hdr, RTEST_FAST)
{
  r_assert (!r_rtcp_is_valid_hdr (pkt_stun, sizeof (pkt_stun)));
  r_assert (!r_rtcp_is_valid_hdr (pkt_rtp_pcmu, sizeof (pkt_rtp_pcmu)));
  r_assert (r_rtcp_is_valid_hdr (pkt_rtcp_sr_sdes, sizeof (pkt_rtcp_sr_sdes)));
}
RTEST_END;

