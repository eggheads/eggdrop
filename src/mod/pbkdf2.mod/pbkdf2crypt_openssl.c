/*
 * pbkdf2_openssl.c -- part of pbkdf2.mod
 *   OpenSSL PBKDF2, included in pbkdf2.c
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

#ifndef PBKDF2_OPENSSL_C
#define PBKDF2_OPENSSL_C

#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

typedef EVP_MD digest_t;

#define INVERT_ERROR(x) ((x) != 1)
#define PBKDF2CRYPT_GET_DIGEST_BY_NAME EVP_get_digestbyname
#define PBKDF2CRYPT_GET_DIGEST_SIZE EVP_MD_size
#define PBKDF2CRYPT_MAKE_HASH(digest, pass, passlen, salt, saltlen, cycles, outlen, out) \
  INVERT_ERROR(PKCS5_PBKDF2_HMAC((pass), (passlen), (salt), (saltlen), (cycles), (digest), (outlen), (out)))
#define PBKDF2CRYPT_MAKE_SALT(buf, num) INVERT_ERROR(RAND_bytes((buf), (num)))
#define PBKDF2CRYPT_BASE64_ENC pbkdf2crypt_openssl_base64_enc
#define PBKDF2CRYPT_BASE64_DEC pbkdf2crypt_openssl_base64_dec
#define PBKDF2CRYPT_INIT_MODULE pbkdf2crypt_openssl_init 

static int pbkdf2crypt_openssl_base64_enc(unsigned char *out, int outlen, const unsigned char *bytes, int len)
{
  BIO *bio, *b64;
  BUF_MEM *memptr;

  if (outlen < PBKDF2CRYPT_BASE64_ENC_LEN(len))
    return -1;

  bio = BIO_new(BIO_s_mem());
  b64 = BIO_new(BIO_f_base64());
  b64 = BIO_push(b64, bio);
  if (BIO_write(b64, bytes, len) <= 0)
    return -1;
  BIO_flush(b64);
  BIO_get_mem_ptr(b64, &memptr);
  if (outlen < memptr->length)
    return -1;
  /* Skip EOF */
  memcpy(out, memptr->data, memptr->length-1);
  BIO_free_all(b64);
  return 0;
}

static int pbkdf2crypt_openssl_base64_dec(unsigned char *out, int outlen, const unsigned char *bytes, int len)
{
  BIO *bio, *b64;
  int expectedlen;

  expectedlen = PBKDF2CRYPT_BASE64_DEC_LEN(bytes, len);

  if (outlen < expectedlen)
    return -1;

  bio = BIO_new_mem_buf(bytes, len);
  b64 = BIO_new(BIO_f_base64());
  bio = BIO_push(b64, bio);
  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
  if (BIO_read(bio, out, outlen) != expectedlen)
    return -1;
  BIO_free_all(bio);
  return 0;
}

static int pbkdf2crypt_openssl_init(void)
{
  OpenSSL_add_all_digests();
  return 0;
}

#endif
