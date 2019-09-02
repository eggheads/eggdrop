/*
 * pbkdf2.h -- part of pbkdf2.mod
 *   PBKDF2 config
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2019 Eggheads Development Team
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

#ifndef PBKDF2_H
#define PBKDF2_H

#define MODULE_NAME "encryption2"

/* Include first because of ncalloc/nmalloc/nfree */
#include "src/mod/module.h"

/* Global functions exported to other modules (egg_malloc, egg_free, ..) */
/* Also, undefine some random stuff that collides by accident */
#undef global
#undef ver
#define global pbkdf2_global

#endif

