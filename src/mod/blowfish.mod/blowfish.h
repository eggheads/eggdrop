/*
 * blowfish.h -- part of blowfish.mod
 *
 * $Id: blowfish.h,v 1.17 2011/02/13 14:19:33 simple Exp $
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

#ifndef _EGG_MOD_BLOWFISH_BLOWFISH_H
#define _EGG_MOD_BLOWFISH_BLOWFISH_H

#define MAXKEYBYTES 56 /* 448 bits */
#define bf_N        16
#define noErr        0
#define DATAERROR   -1
#define KEYBYTES     8

union aword {
  u_32bit_t word;
  u_8bit_t byte[4];
  struct {
#ifdef WORDS_BIGENDIAN
    unsigned int byte0:8;
    unsigned int byte1:8;
    unsigned int byte2:8;
    unsigned int byte3:8;
#else /* !WORDS_BIGENDIAN */
    unsigned int byte3:8;
    unsigned int byte2:8;
    unsigned int byte1:8;
    unsigned int byte0:8;
#endif /* !WORDS_BIGENDIAN */
  } w;
};

#endif /* _EGG_MOD_BLOWFISH_BLOWFISH_H */
