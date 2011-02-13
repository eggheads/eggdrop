/*
 * files.h -- part of filesys.mod
 *
 * $Id: files.h,v 1.16 2011/02/13 14:19:33 simple Exp $
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

#ifndef _EGG_MOD_FILESYS_FILES_H
#define _EGG_MOD_FILESYS_FILES_H

/* Language file additions for the file area */
#define FILES_CONVERT    get_language(0x300)
#define FILES_NOUPDATE   get_language(0x301)
#define FILES_NOCONVERT  get_language(0x302)
#define FILES_LSHEAD1    get_language(0x303)
#define FILES_LSHEAD2    get_language(0x304)
#define FILES_NOFILES    get_language(0x305)
#define FILES_NOMATCH    get_language(0x306)
#define FILES_DIRDNE     get_language(0x307)
#define FILES_FILEDNE    get_language(0x308)
#define FILES_NOSHARE    get_language(0x309)
#define FILES_REMOTE     get_language(0x30a)
#define FILES_SENDERR    get_language(0x30b)
#define FILES_SENDING    get_language(0x30c)
#define FILES_REMOTEREQ  get_language(0x30d)
#define FILES_BROKEN     get_language(0x30e)
#define FILES_INVPATH    get_language(0x30f)
#define FILES_CURDIR     get_language(0x310)
#define FILES_NEWCURDIR  get_language(0x311)
#define FILES_NOSUCHDIR  get_language(0x312)
#define FILES_ILLDIR     get_language(0x313)
#define FILES_BADNICK    get_language(0x314)
#define FILES_NOTAVAIL   get_language(0x315)
#define FILES_REQUESTED  get_language(0x316)
#define FILES_NORMAL     get_language(0x317)
#define FILES_CHGLINK    get_language(0x318)
#define FILES_NOTOWNER   get_language(0x319)
#define FILES_CREADIR    get_language(0x31a)
#define FILES_REQACCESS  get_language(0x31b)
#define FILES_CHGACCESS  get_language(0x31c)
#define FILES_CHGNACCESS get_language(0x31d)
#define FILES_REMDIR     get_language(0x31e)
#define FILES_ILLSOURCE  get_language(0x31f)
#define FILES_ILLDEST    get_language(0x320)
#define FILES_STUPID     get_language(0x321)
#define FILES_EXISTDIR   get_language(0x322)
#define FILES_SKIPSTUPID get_language(0x323)
#define FILES_DEST       get_language(0x324)
#define FILES_COPY       get_language(0x325)
#define FILES_COPIED     get_language(0x326)
#define FILES_MOVE       get_language(0x327)
#define FILES_MOVED      get_language(0x328)
#define FILES_CANTWRITE  get_language(0x329)
#define FILES_REQUIRES   get_language(0x32a)
#define FILES_HID        get_language(0x32b)
#define FILES_UNHID      get_language(0x32c)
#define FILES_SHARED     get_language(0x32d)
#define FILES_UNSHARED   get_language(0x32e)
#define FILES_ADDLINK    get_language(0x32f)
#define FILES_CHANGED    get_language(0x330)
#define FILES_BLANKED    get_language(0x331)
#define FILES_ERASED     get_language(0x332)
#define FILES_WELCOME    get_language(0x33a)
#define FILES_WELCOME1   get_language(0x33b)

#endif /* _EGG_MOD_FILESYS_FILES_H */
