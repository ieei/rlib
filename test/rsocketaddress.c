#include <rlib/rlib.h>

#ifdef R_OS_WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

RTEST (rsocketaddress, ipv4_native, RTEST_FAST)
{
  struct sockaddr_in ipv4;
  RSocketAddress * addr;

  r_memset (&ipv4, 0, sizeof (ipv4));
  ipv4.sin_family = AF_INET;
  ipv4.sin_port = r_htons (42);
  ipv4.sin_addr.s_addr = INADDR_LOOPBACK;

  r_assert_cmpptr (r_socket_address_new_from_native (NULL, 0), ==, NULL);
  r_assert_cmpptr (r_socket_address_new_from_native (&ipv4, 0), ==, NULL);
  r_assert_cmpptr ((addr = r_socket_address_new_from_native (&ipv4, sizeof (ipv4))), !=, NULL);

  r_assert_cmpptr (r_socket_address_get_family (addr), ==, R_SOCKET_FAMILY_IPV4);

  r_socket_address_unref (addr);
}
RTEST_END;

RTEST (rsocketaddress, ipv4_new, RTEST_FAST)
{
  RSocketAddress * addr_u32, * addr_u8, * addr_str;

  r_assert_cmpptr ((addr_u32 = r_socket_address_ipv4_new_uint32 (INADDR_LOOPBACK, 42)), !=, NULL);
  r_assert_cmpptr (r_socket_address_get_family (addr_u32), ==, R_SOCKET_FAMILY_IPV4);
  r_assert_cmpuint (r_socket_address_ipv4_get_port (addr_u32), ==, 42);
  r_assert_cmpuint (r_socket_address_ipv4_get_ip (addr_u32), ==, INADDR_LOOPBACK);

  r_assert_cmpptr ((addr_u8 = r_socket_address_ipv4_new_uint8 (127, 0, 0, 1, 42)), !=, NULL);
  r_assert_cmpptr (r_socket_address_get_family (addr_u8), ==, R_SOCKET_FAMILY_IPV4);
  r_assert_cmpuint (r_socket_address_ipv4_get_port (addr_u8), ==, 42);
  r_assert_cmpuint (r_socket_address_ipv4_get_ip (addr_u8), ==, INADDR_LOOPBACK);

  r_assert_cmpptr ((addr_str = r_socket_address_ipv4_new_from_string ("127.0.0.1", 42)), !=, NULL);
  r_assert_cmpptr (r_socket_address_get_family (addr_str), ==, R_SOCKET_FAMILY_IPV4);
  r_assert_cmpuint (r_socket_address_ipv4_get_port (addr_str), ==, 42);
  r_assert_cmpuint (r_socket_address_ipv4_get_ip (addr_str), ==, INADDR_LOOPBACK);

  r_assert_cmpint (r_socket_address_cmp (addr_u32, addr_u8), ==, 0);
  r_assert_cmpint (r_socket_address_cmp (addr_str, addr_u8), ==, 0);

  r_socket_address_unref (addr_str);
  r_socket_address_unref (addr_u32);
  r_socket_address_unref (addr_u8);
}
RTEST_END;

