/*
 * config.h -- part of pbkdf2.mod
 *   Config file for mbedtls
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2017 Eggheads Development Team
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

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

#include "../pbkdf2.h"

#include "src/mod/module.h"

/* ver collides with local variable */
#undef ver

#undef global
#define global pbkdf2_global
extern Function *pbkdf2_global;

#undef malloc
#undef free
#define malloc nmalloc
#define free nfree

#define MBEDTLS_SHA256_C
#define MBEDTLS_PKCS5_C
#define MBEDTLS_MD_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_HMAC_DRBG_C
#define MBEDTLS_BASE64_C

#endif
