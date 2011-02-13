/*
 * strcasecmp.c -- provides strcasecmp() and strncasecmp if necessary.
 *
 * $Id: strcasecmp.c,v 1.11 2011/02/13 14:19:33 simple Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2011 Eggheads Development Team
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

#include "main.h"
#include "memcpy.h"

#ifndef HAVE_STRCASECMP
int egg_strcasecmp(const char *s1, const char *s2)
{
  while ((*s1) && (*s2) && (toupper(*s1) == toupper(*s2))) {
    s1++;
    s2++;
  }
  return toupper(*s1) - toupper(*s2);
}
#endif /* !HAVE_STRCASECMP */

#ifndef HAVE_STRNCASECMP
int egg_strncasecmp(const char *s1, const char *s2, size_t n)
{
  if (!n)
    return 0;
  while (--n && (*s1) && (*s2) && (toupper(*s1) == toupper(*s2))) {
    s1++;
    s2++;
  }
  return toupper(*s1) - toupper(*s2);
}
#endif /* !HAVE_STRNCASECMP */
