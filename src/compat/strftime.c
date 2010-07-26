/*
 * strftime.c
 *   Portable strftime implementation. Uses GNU's strftime().
 *
 * $Id: strftime.c,v 1.1 2010/07/26 21:11:06 simple Exp $
 */
/*
 * Copyright (C) 2000 - 2010 Eggheads Development Team
 * Written by Fabian Knittel
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

#include "src/main.h"
#include "strftime.h"

#ifndef HAVE_STRFTIME
#  undef emacs
#  undef _LIBC
#  define strftime egg_strftime

#  include "gnu_strftime.c"
#endif /* !HAVE_STRFTIME */
