/* RLIB - Convenience library for useful things
 * Copyright (C) 2016 Haakon Sporsheim <haakon.sporsheim@gmail.com>
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
#include <rlib/rsocketaddress.h>
#include "rnetworking-private.h"

#include <rlib/rmem.h>

RSocketAddress *
r_socket_address_new_from_native (rconstpointer addr, rsize addrsize)
{
  RSocketAddress * ret;

  if (R_UNLIKELY (addr == NULL)) return NULL;
  if (R_UNLIKELY (addrsize == 0 || addrsize > sizeof (struct sockaddr_storage))) return NULL;

  if ((ret = r_mem_new0 (RSocketAddress)) != NULL) {
    r_ref_init (ret, r_free);
    r_memcpy (&ret->addr, addr, addrsize);
  }

  return ret;
}

RSocketAddress *
r_socket_address_ipv4_new_uint32 (ruint32 addr, ruint16 port)
{
  RSocketAddress * ret;

  if ((ret = r_mem_new0 (RSocketAddress)) != NULL) {
    struct sockaddr_in * saddr = (struct sockaddr_in *)&ret->addr;

    r_ref_init (ret, r_free);

    ret->addrlen = sizeof (struct sockaddr_in);
    saddr->sin_family = R_SOCKET_FAMILY_IPV4;
    saddr->sin_port = r_htons (port);
    saddr->sin_addr.s_addr = r_htonl (addr);
  }

  return ret;
}

RSocketAddress *
r_socket_address_ipv4_new_uint8 (ruint8 a, ruint8 b, ruint8 c, ruint8 d, ruint16 port)
{
  RSocketAddress * ret;

  if ((ret = r_mem_new0 (RSocketAddress)) != NULL) {
    struct sockaddr_in * saddr = (struct sockaddr_in *)&ret->addr;

    r_ref_init (ret, r_free);

    ret->addrlen = sizeof (struct sockaddr_in);
    saddr->sin_family = R_SOCKET_FAMILY_IPV4;
    saddr->sin_port = r_htons (port);
    saddr->sin_addr.s_addr = ((ruint32)d << 24) | ((ruint32)c << 16) |
      ((ruint32)b << 8) | (ruint32)a;
  }

  return ret;
}

RSocketAddress *
r_socket_address_ipv4_new_from_string (const rchar * ip, ruint16 port)
{
  RSocketAddress * ret;
  struct in_addr in;

#if defined (HAVE_INET_PTON)
  if (inet_pton (AF_INET, ip, &in) < 1)
    return NULL;
#else
  if (inet_aton (ip, &in) < 1)
    return NULL;
#endif

  if ((ret = r_mem_new0 (RSocketAddress)) != NULL) {
    struct sockaddr_in * saddr = (struct sockaddr_in *)&ret->addr;

    r_ref_init (ret, r_free);

    ret->addrlen = sizeof (struct sockaddr_in);
    saddr->sin_family = R_SOCKET_FAMILY_IPV4;
    saddr->sin_port = r_htons (port);
    saddr->sin_addr = in;
  }

  return ret;
}

RSocketFamily
r_socket_address_get_family (const RSocketAddress * addr)
{
  return (RSocketFamily)addr->addr.ss_family;
}

int
r_socket_address_cmp (const RSocketAddress * a, const RSocketAddress * b)
{
  int ret;

  if (R_UNLIKELY (a == NULL)) return -(a != b);
  if (R_UNLIKELY (b == NULL)) return a != b;

  /* We can't do memcmp on the storage structure */

  if ((ret = (int)b->addr.ss_family - (int)a->addr.ss_family) == 0) {
    switch (a->addr.ss_family) {
      case R_SOCKET_FAMILY_IPV4:
        {
          struct sockaddr_in * ain = (struct sockaddr_in *)&a->addr;
          struct sockaddr_in * bin = (struct sockaddr_in *)&b->addr;

          if ((ret = ((int)bin->sin_port - (int)ain->sin_port)) == 0)
            ret = (int)bin->sin_addr.s_addr - (int)ain->sin_addr.s_addr;
        }
      case R_SOCKET_FAMILY_IPV6:
        {
          struct sockaddr_in6 * ain = (struct sockaddr_in6 *)&a->addr;
          struct sockaddr_in6 * bin = (struct sockaddr_in6 *)&b->addr;

          if ((ret = ((int)bin->sin6_port - (int)ain->sin6_port)) == 0)
            if ((ret = ((int)bin->sin6_flowinfo - (int)ain->sin6_flowinfo)) == 0)
              if ((ret = ((int)bin->sin6_scope_id - (int)ain->sin6_scope_id)) == 0)
                ret = r_memcmp (&bin->sin6_addr.s6_addr, &ain->sin6_addr.s6_addr,
                    sizeof (ain->sin6_addr.s6_addr));
        }
        break;
      default:
        ret = r_memcmp (&a->addr, &b->addr, sizeof (struct sockaddr_storage));
    }
  }

  return ret;
}

ruint16
r_socket_address_ipv4_get_port (const RSocketAddress * addr)
{
  struct sockaddr_in * addr_in;

  if (R_UNLIKELY (addr == NULL)) return RUINT16_MAX;

  addr_in = (struct sockaddr_in *)&addr->addr;
  return r_ntohs (addr_in->sin_port);
}

ruint32
r_socket_address_ipv4_get_ip (const RSocketAddress * addr)
{
  struct sockaddr_in * addr_in;

  if (R_UNLIKELY (addr == NULL)) return INADDR_NONE;

  addr_in = (struct sockaddr_in *)&addr->addr;
  return r_ntohl (addr_in->sin_addr.s_addr);
}
