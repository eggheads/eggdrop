/*
 * base64.h
 *   prototypes for base64.c
 */
/*
 * Copyright (C) 2010 - 2021 Eggheads Development Team
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

#ifndef _EGG_COMPAT_BASE64_H_
#define _EGG_COMPAT_BASE64_H_

#define B64_NTOP_CALCULATE_SIZE(x) ((x + 2) / 3 * 4)
#define B64_PTON_CALCULATE_SIZE(x) (x * 3 / 4)

#ifndef HAVE_BASE64
int b64_ntop(uint8_t const *, size_t, char *, size_t);
int b64_pton(const char *, uint8_t *, size_t);
#endif /* HAVE_BASE64 */

#endif /* _EGG_COMPAT_BASE64_H_ */
