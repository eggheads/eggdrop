/*
 * gethostbyname2.h
 *   prototypes for gethostbyname2.c
 *
 * $Id: gethostbyname2.h,v 1.3 2010/10/06 19:07:47 pseudo Exp $
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

#ifndef _EGG_COMPAT_GETHOSTBYNAME2
#define _EGG_COMPAT_GETHOSTBYNAME2

#include "src/main.h"

#include <netdb.h>
#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif
#include <netinet/in.h>

#if defined IPV6 && !defined HAVE_GETHOSTBYNAME2
struct hostent *gethostbyname2(const char *name, int af);
#endif
#endif /* _EGG_COMPAT_GETHOSTBYNAME2 */
