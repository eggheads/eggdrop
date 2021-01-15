/*
 * explicit_bzero.h
 *   prototypes for explicit_bzero.c
 */
/*
 * Copyright (C) 2010 - 2021 Eggheads Development Team
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

#ifndef _EGG_COMPAT_EXPLICIT_BZERO_H_
#define _EGG_COMPAT_EXPLICIT_BZERO_H_

#ifndef HAVE_EXPLICIT_BZERO
void explicit_bzero(void *const, const size_t);
#endif /* HAVE_EXPLICIT_BZERO */

#endif /* _EGG_COMPAT_EXPLICIT_BZERO_H_ */
