/*
 * strcasecmp.h
 *   prototypes for strcasecmp.c
 *
 * $Id: strcasecmp.h,v 1.7 2003/01/29 05:48:41 wcc Exp $
 */
/*
 * Copyright (C) 2000, 2001, 2002, 2003 Eggheads Development Team
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

#ifndef _EGG_COMPAT_STRCASECMP_H
#define _EGG_COMPAT_STRCASECMP_H

#include "src/main.h"
#include <ctype.h>


#ifndef HAVE_STRCASECMP
/* Use our own implementation. */
int egg_strcasecmp(const char *, const char *);
#else
#  define egg_strcasecmp strcasecmp
#endif

#ifndef HAVE_STRNCASECMP
/* Use our own implementation. */
int egg_strncasecmp(const char *, const char *, size_t);
#else
#  define egg_strncasecmp strncasecmp
#endif

#endif /* !__EGG_COMPAT_STRCASECMP_H */
