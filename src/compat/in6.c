/*
 * in6.c -- provide some IPv6 constants if not available
 */
/*
 * Copyright (C) 2007 plazmus
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

#include "in6.h"

#ifdef IPV6

#ifndef HAVE_IN6ADDR_ANY
static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
#endif

#ifndef HAVE_IN6ADDR_LOOPBACK
static const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
#endif

#endif /* IPV6 */
