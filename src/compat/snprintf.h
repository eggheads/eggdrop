/*
 * snprintf.h --
 *
 *	prototypes for snprintf.c
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
/*
 * $Id: snprintf.h,v 1.15 2003/03/04 22:14:03 wcc Exp $
 */

#ifndef _EGG_SNPRINTF_H
#define _EGG_SNPRINTF_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>		/* FIXME: possible varargs.h conflicts */

#if !defined(HAVE_VSNPRINTF) || !defined(HAVE_C99_VSNPRINTF)
int egg_vsnprintf(char *str, size_t count, const char *fmt, va_list args);
#else
#  define egg_vsnprintf vsnprintf
#endif

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_C99_VSNPRINTF)
int egg_snprintf(char *str, size_t count, const char *fmt, ...);
#else
#  define egg_snprintf snprintf
#endif

#ifndef HAVE_VASPRINTF
int egg_vasprintf(char **ptr, const char *format, va_list ap);
#else
#  define egg_vasprintf vasprintf
#endif

#ifndef HAVE_ASPRINTF
int egg_asprintf(char **ptr, const char *format, ...);
#else
#  define egg_asprintf asprintf
#endif

#endif				/* !_EGG_SNPRINTF_H */
