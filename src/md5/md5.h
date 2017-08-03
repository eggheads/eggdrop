/*
 * This is an OpenSSL-compatible implementation of the RSA Data Security,
 * Inc. MD5 Message-Digest Algorithm.
 *
 * Written by Solar Designer <solar@openwall.com> in 2001, and placed in
 * the public domain.  See md5c.c for more information.
 */

#ifndef _MD5_H
#define _MD5_H

#include "src/main.h"

#ifdef HAVE_OPENSSL_MD5
#include <openssl/md5.h>
#else

/* Any 32-bit or wider integer data type will do */
typedef unsigned long MD5_u32plus;

typedef struct {
  MD5_u32plus lo, hi;
  MD5_u32plus a, b, c, d;
  unsigned char buffer[64];
  MD5_u32plus block[16];
} MD5_CTX;

extern void MD5_Init(MD5_CTX *ctx);
extern void MD5_Update(MD5_CTX *ctx, void *data, unsigned long size);
extern void MD5_Final(unsigned char *result, MD5_CTX *ctx);

#endif /* HAVE_OPENSSL_MD5 */
#endif /* _MD5_H */
