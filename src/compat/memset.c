/*
 * memset.c -- provides memset() if necessary.
 *
 * $Id: memset.c,v 1.2 2000/09/09 11:39:10 fabian Exp $
 */
/* Copyright (C) 1997  Robey Pointer
 * Copyright (C) 1999, 2000  Eggheads
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
#include "memset.h"

#ifndef HAVE_MEMSET
void *egg_memset(void *dest, int c, size_t n)
{
  while (n--)
    *((u_8bit_t *) dest)++ = c;
  return dest;
}
#endif /* !HAVE_MEMSET */
