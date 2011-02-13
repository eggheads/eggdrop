/*
 * inet_aton.h
 *   prototypes for inet_aton.c
 *
 * $Id: inet_aton.h,v 1.16 2011/02/13 14:19:33 simple Exp $
 */
/*
 * Copyright (C) 2000 - 2011 Eggheads Development Team
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

#ifndef _EGG_COMPAT_INET_ATON_H
#define _EGG_COMPAT_INET_ATON_H

#include "src/main.h"
#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef HAVE_INET_ATON
/* Use our own implementation. */
int egg_inet_aton(const char *cp, struct in_addr *addr);
#else
#  define egg_inet_aton inet_aton
#endif

#endif /* !__EGG_COMPAT_INET_ATON_H */
