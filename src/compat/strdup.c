/*
 * strdup.c -- provides strdup() if necessary.
 *
 * $Id: strdup.c,v 1.1 2011/07/09 15:07:48 thommey Exp $
 */
/*
 * Copyright (C) 2011 Eggheads Development Team
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

#ifndef HAVE_STRDUP
char *egg_strdup(const char *s)
{
  char *d;
  size_t l = strlen(s)+1;
  d = nmalloc(l);
  return egg_memcpy(d, s, l);
}
#endif /* !HAVE_STRDUP */
