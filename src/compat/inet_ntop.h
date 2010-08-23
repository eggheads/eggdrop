/*
 * inet_ntop.h
 *   prototypes for inet_ntop.c
 *
 * $Id: inet_ntop.h,v 1.2 2010/08/23 21:27:40 pseudo Exp $
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>

#if defined IPV6 && !defined HAVE_INET_NTOP
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
#endif
