/*
 * strdup.h
 *   prototypes for strdup.c
 *
 * $Id: strdup.h,v 1.1 2011/07/09 15:07:48 thommey Exp $
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

#ifndef _EGG_COMPAT_STRDUP_H
#define _EGG_COMPAT_STRDUP_H

#include "src/main.h"
#include <string.h>

#ifndef HAVE_STRDUP
/* Use our own implementation. */
char *egg_strdup(const char *s);
#else
#  define egg_strdup strdup
#endif

#endif /* !__EGG_COMPAT_STRDUP_H */
