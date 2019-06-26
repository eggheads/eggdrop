/*
 * inet_ntop.h
 *   prototypes for inet_ntop.c
 */
/*
 * Copyright (C) 2010 - 2019 Eggheads Development Team
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

#ifndef _EGG_COMPAT_INET_NTOP_H
#define _EGG_COMPAT_INET_NTOP_H

#include "src/main.h"

#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef HAVE_INET_NTOP
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
#endif
#endif /* _EGG_COMPAT_INET_NTOP_H */
