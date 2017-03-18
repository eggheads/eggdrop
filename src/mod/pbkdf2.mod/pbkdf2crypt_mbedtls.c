/*
 * pbkdf2_mbedtls.c -- part of pbkdf2.mod
 *   mbedtls PBKDF2, included in pbkdf2.c
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

#ifndef PBKDF2_MBEDTLS_C
#define PBKDF2_MBEDTLS_C

#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/base64.h"

typedef mbedtls_md_info_t digest_t;

static size_t mbedtls_ignored;

#define PBKDF2CRYPT_GET_DIGEST_BY_NAME mbedtls_md_info_from_string
#define PBKDF2CRYPT_GET_DIGEST_SIZE mbedtls_md_get_size
#define PBKDF2CRYPT_MAKE_HASH pbkdf2crypt_mbedtls_pbkdf2
#define PBKDF2CRYPT_MAKE_SALT pbkdf2crypt_mbedtls_salt
#define PBKDF2CRYPT_INIT_MODULE pbkdf2crypt_mbedtls_init
#define PBKDF2CRYPT_BASE64_ENC(out, outlen, bytes, len) mbedtls_base64_encode((unsigned char *)(out), (outlen), &mbedtls_ignored, (bytes), (len))
#define PBKDF2CRYPT_BASE64_DEC(out, outlen, bytes, len) mbedtls_base64_decode((unsigned char *)(out), (outlen), &mbedtls_ignored, (bytes), (len))

#define PBKDF2CRYPT_MBEDTLS_RNG_DIGEST "SHA256"

static mbedtls_hmac_drbg_context rngctx;
static mbedtls_entropy_context entropyctx;

static int pbkdf2crypt_mbedtls_pbkdf2(const digest_t *digest, const char *pass, int passlen, const unsigned char *salt, int saltlen, int cycles, int outlen, unsigned char *out)
{
  mbedtls_md_context_t md_ctx;
  int ret;

  mbedtls_md_init(&md_ctx);
  if (mbedtls_md_setup(&md_ctx, digest, 1) != 0) {
      ret = -1;
      goto exit;
  }

  ret = mbedtls_pkcs5_pbkdf2_hmac(&md_ctx, (const unsigned char *)pass, passlen, salt, saltlen, cycles, outlen, out);
  if (ret != 0)
    return -2;

exit:
    mbedtls_md_free(&md_ctx);
    return ret;
}

static int pbkdf2crypt_mbedtls_salt(unsigned char *salt, int saltlen)
{
  return mbedtls_hmac_drbg_random(&rngctx, salt, saltlen);
}

static int pbkdf2crypt_mbedtls_init(void)
{
  const digest_t *digest = mbedtls_md_info_from_string(PBKDF2CRYPT_MBEDTLS_RNG_DIGEST);
  if (!digest)
    return -1;
  mbedtls_entropy_init(&entropyctx);
  mbedtls_hmac_drbg_init(&rngctx);
  if (!mbedtls_hmac_drbg_seed(&rngctx, digest, mbedtls_entropy_func, &entropyctx, NULL, 0))
    return -1;
  return 0;
}

#endif
