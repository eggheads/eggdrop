/*
 * memset.h
 *   prototypes for memset.c
 *
 * $Id: memset.h,v 1.14 2011/02/13 14:19:33 simple Exp $
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

#ifndef _EGG_COMPAT_MEMSET_H
#define _EGG_COMPAT_MEMSET_H

#include "src/main.h"
#include <string.h>

#ifndef HAVE_MEMSET
/* Use our own implementation. */
void *egg_memset(void *dest, int c, size_t n);
#else
#  define egg_memset memset
#endif

/* Use memset instead of bzero.
 */
#define egg_bzero(dest, n) egg_memset(dest, 0, n)

#endif /* !__EGG_COMPAT_MEMSET_H */
