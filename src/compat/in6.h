/*
 * in6.h -- various IPv6 related definitions and macros
 *
 * $Id: in6.h,v 1.2 2010/10/06 19:07:47 pseudo Exp $
 */
/*
 * Copyright (C) 2010 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef IPV6

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#ifndef PF_INET6
#  define PF_INET6 PF_MAX
#endif

#ifndef AF_INET6
#  define AF_INET6 PF_INET6
#endif

#ifndef HAVE_STRUCT_IN6_ADDR
struct in6_addr {
  union {
    u_8bit_t	__u6_addr8[16];
    u_16bit_t	__u6_addr16[8];
    u_32bit_t	__u6_addr32[4];
  } __u6_addr;
#define	s6_addr	__u6_addr.__u6_addr8
};
#endif

#ifndef HAVE_STRUCT_SOCKADDR_IN6
struct sockaddr_in6 {
  u_16bit_t 	  sin6_family;
  u_16bit_t 	  sin6_port;
  u_32bit_t 	  sin6_flowinfo;
  struct in6_addr sin6_addr;
  u_32bit_t       sin6_scope_id;
};
#endif

#ifndef INET6_ADDRSTRLEN
#  define INET6_ADDRSTRLEN 46
#endif

#ifndef IN6ADDR_ANY_INIT
#  define IN6ADDR_ANY_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } }
#endif

#ifndef IN6ADDR_LOOPBACK_INIT
#  define IN6ADDR_LOOPBACK_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }
#endif

#ifndef HAVE_IN6ADDR_ANY
extern const struct in6_addr in6addr_any;
#endif

#ifndef HAVE_IN6ADDR_LOOPBACK
extern const struct in6_addr in6addr_loopback;
#endif

#ifndef IN6_IS_ADDR_UNSPECIFIED
# define IN6_IS_ADDR_UNSPECIFIED(a) \
  (((const u_32bit_t *) (a))[0] == 0                                   \
   && ((const u_32bit_t *) (a))[1] == 0                                \
   && ((const u_32bit_t *) (a))[2] == 0                                \
   && ((const u_32bit_t *) (a))[3] == 0)
#endif

#ifndef IN6_IS_ADDR_LOOPBACK
# define IN6_IS_ADDR_LOOPBACK(a) \
  (((const u_32bit_t *) (a))[0] == 0                                   \
   && ((const u_32bit_t *) (a))[1] == 0                                \
   && ((const u_32bit_t *) (a))[2] == 0                                \
   && ((const u_32bit_t *) (a))[3] == htonl (1))
#endif

#ifndef IN6_IS_ADDR_V4MAPPED
# define IN6_IS_ADDR_V4MAPPED(a) \
  ((((const u_32bit_t *) (a))[0] == 0)                                 \
   && (((const u_32bit_t *) (a))[1] == 0)                              \
   && (((const u_32bit_t *) (a))[2] == htonl (0xffff)))
#endif

#ifndef IN6_ARE_ADDR_EQUAL
# define IN6_ARE_ADDR_EQUAL(a,b) \
  ((((const u_32bit_t *) (a))[0] == ((const u_32bit_t *) (b))[0])     \
   && (((const u_32bit_t *) (a))[1] == ((const u_32bit_t *) (b))[1])  \
   && (((const u_32bit_t *) (a))[2] == ((const u_32bit_t *) (b))[2])  \
   && (((const u_32bit_t *) (a))[3] == ((const u_32bit_t *) (b))[3]))
#endif

#endif /* IPV6 */
