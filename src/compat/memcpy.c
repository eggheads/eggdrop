/*
 * memcpy.c -- provides memcpy() if necessary.
 *
 * $Id: memcpy.c,v 1.11 2011/02/13 14:19:33 simple Exp $
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

#ifndef HAVE_MEMCPY
void *egg_memcpy(void *dest, const void *src, size_t n)
{
  while (n--)
    *((char *) dest)++ = *((char *) src)++;
  return dest;
}
#endif /* !HAVE_MEMCPY */
