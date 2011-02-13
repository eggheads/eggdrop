/*
 * assoc.h -- part of assoc.mod
 *
 * $Id: assoc.h,v 1.14 2011/02/13 14:19:33 simple Exp $
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

#ifndef _EGG_MOD_ASSOC_ASSOC_H
#define _EGG_MOD_ASSOC_ASSOC_H

#define ASSOC_NOCHNAMES        get_language(0xb000)
#define ASSOC_CHAN             get_language(0xb001)
#define ASSOC_NAME             get_language(0xb002)
#define ASSOC_LCHAN_RANGE      get_language(0xb003)
#define ASSOC_CHAN_RANGE       get_language(0xb004)
#define ASSOC_PARTYLINE        get_language(0xb005)
#define ASSOC_NONAME_CHAN      get_language(0xb006)
#define ASSOC_REMNAME_CHAN     get_language(0xb007)
#define ASSOC_REMOUT_CHAN      get_language(0xb008)
#define ASSOC_NEWNAME_CHAN     get_language(0xb009)
#define ASSOC_NEWOUT_CHAN      get_language(0xb00a)
#define ASSOC_CHNAME_NAMED     get_language(0xb00b)
#define ASSOC_CHNAME_NAMED2    get_language(0xb00c)
#define ASSOC_CHNAME_REM       get_language(0xb00d)
#define ASSOC_CHNAME_TOOLONG   get_language(0xb00e)
#define ASSOC_CHNAME_FIRSTCHAR get_language(0xb00f)

#endif /* _EGG_MOD_ASSOC_ASSOC_H */
