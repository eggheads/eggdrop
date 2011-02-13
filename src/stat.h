/*
 * stat.h
 *  file attributes
 *
 * $Id: stat.h,v 1.13 2011/02/13 14:19:33 simple Exp $
 */
/*
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

#ifndef _EGG_STAT_H
#define _EGG_STAT_H

#ifndef S_ISDIR
#  ifndef S_IFMT
#    define S_IFMT   0170000 /* Bitmask for the file type bitfields */
#  endif
#  ifndef S_IFDIR
#    define S_IFDIR  0040000 /* Directory                           */
#  endif
#  define S_ISDIR(m) (((m)&(S_IFMT)) == (S_IFDIR))
#endif
#ifndef S_IFREG
#  define S_IFREG    0100000 /* Regular file                        */
#endif
#ifndef S_IFLNK
#  define S_IFLNK    0120000 /* Symbolic link                       */
#endif

#endif /* _EGG_STAT_H */
