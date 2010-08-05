/*
 * gethostbyname2.c -- provide a dummy gethostbyname2 replacement
 *
 * $Id: gethostbyname2.c,v 1.1 2010/08/05 18:12:05 pseudo Exp $
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

#include "gethostbyname2.h"

#ifndef HAVE_GETHOSTBYNAME2
struct hostent *gethostbyname2(const char *name, int af)
{
  if(af != AF_INET) {
    h_errno = NO_RECOVERY;
    return 0;
  }
  return gethostbyname(name);
}
#endif
