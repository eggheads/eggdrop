/* 
 * wire.h -- part of wire.mod
 * 
 * $Id: wire.h,v 1.2 1999/12/15 02:33:00 guppy Exp $
 */
/* 
 * Copyright (C) 1997  Robey Pointer
 * Copyright (C) 1999  Eggheads
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

#define WIRE_IDLE		MISC_IDLE
#define WIRE_AWAY		MISC_AWAY
#define WIRE_NOTONWIRE		get_language(0xa000)
#define WIRE_CURRENTLYON	get_language(0xa001)
#define WIRE_NOLONGERWIRED	get_language(0xa002)
#define WIRE_CHANGINGKEY	get_language(0xa003)
#define WIRE_INFO1		get_language(0xa004)
#define WIRE_INFO2		get_language(0xa005)
#define WIRE_INFO3		get_language(0xa006)
#define WIRE_JOINED		get_language(0xa007)
#define WIRE_LEFT		get_language(0xa008)
#define WIRE_UNLOAD		get_language(0xa009)
#define WIRE_VERSIONERROR	get_language(0xa00a)
