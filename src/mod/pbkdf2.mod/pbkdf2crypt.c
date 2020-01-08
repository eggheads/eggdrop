/*
 * pbkdf2crypt.c -- part of pbkdf2.mod
 *   PBKDF2 password hashing, cryptographic functions
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2020 Eggheads Development Team
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

#include <resolv.h> /* base64 encode b64_ntop() and base64 decode b64_pton() */
#include <openssl/rand.h>

static int pbkdf2crypt_base64_dec_len(const unsigned char *str, int len);

#define PBKDF2CRYPT_BASE64_ENC_LEN(x) (4*(1+((x)-1)/3))
#define PBKDF2CRYPT_BASE64_DEC_LEN(x, len) pbkdf2crypt_base64_dec_len((x), (len))

/* Global functions exported to other modules (egg_malloc, egg_free, ..) */
/* Also, undefine some random stuff that collides by accident */
#undef global
#undef ver
#define global pbkdf2_global

/* Only ever add to the end, never remove, last one must have empty name.
 * This MUST be the same on all bots, or you might render your userfile useless.
 * This COULD also be subject to change in Eggdrop itself, just don't touch this list.
 */
static struct {
  const char name[16];
  const EVP_MD *digest;
} digests[] = {{"sha512"}, {"sha256"}, {""}};

static int maxdigestlen = 1;

/* Preferred digest as array index from above struct */
#define PBKDF2CONF_DIGESTIDX 0
/* Salt length in bytes, will be base64 encoded after (so, pick something divisible by 3) */
#define PBKDF2CONF_SALTLEN 24
/* Rounds for PBKDF2 */
#define PBKDF2CONF_ROUNDS 5000

/* Skip "" entry at the end */
#define PBKDF2CRYPT_DIGEST_IDX_INVALID(idx) ((idx) < 0 || (idx) > sizeof digests / sizeof *digests - 2)

/* Initializes API with hash algorithm
 * Returns -1 on digest not found, module should report and unload
 */
static int pbkdf2crypt_init(void)
{
  int i;
  if (PBKDF2CONF_ROUNDS <= 0) {
    putlog(LOG_MISC, "*", "rounds must be greater than 0");
    return -1;
  }
  if (PBKDF2CONF_ROUNDS > INT_MAX) {
    putlog(LOG_MISC, "*", "rounds must be equal to or less than %i", INT_MAX);
    return -1;
  }
  OpenSSL_add_all_digests();
  for (i = 0; digests[i].name[0]; i++) {
    digests[i].digest = EVP_get_digestbyname(digests[i].name);
    if (!digests[i].digest) {
      putlog(LOG_MISC, "*", "Failed to initialize digests '%s'.", digests[i].name);
      return -1;
    }
    if (EVP_MD_size(digests[i].digest) > maxdigestlen)
      maxdigestlen = EVP_MD_size(digests[i].digest);
  }
  if (PBKDF2CRYPT_DIGEST_IDX_INVALID(PBKDF2CONF_DIGESTIDX)) {
    putlog(LOG_MISC, "*", "Invalid Digest IDX.");
    return -1;
  }
  return 0;
}

static int pbkdf2crypt_base64_dec_len(const unsigned char *str, int len)
{
  int pad = (str[len-1] == '=');
  if (pad && str[len-2] == '=')
    pad++;
  return len / 4 * 3 - pad;
}

static int pbkdf2crypt_get_size(const char* digest_name, const EVP_MD *digest, int saltlen)
{
  /* PHC string format */
  /* hash = "$pbkdf2-<digest>$rounds=<rounds>$<salt>$<hash>" */
  return strlen("$pbkdf2-") + strlen(digest_name) + 1 + strlen("rounds=FFFFFFFF") + 1 + PBKDF2CRYPT_BASE64_ENC_LEN(saltlen) + 1 + PBKDF2CRYPT_BASE64_ENC_LEN(EVP_MD_size(digest));
}

static int pbkdf2crypt_get_default_size(void)
{
  return pbkdf2crypt_get_size(digests[PBKDF2CONF_DIGESTIDX].name, digests[PBKDF2CONF_DIGESTIDX].digest, PBKDF2CONF_SALTLEN);
}

static void bufcount(char **buf, int *buflen, int bytes)
{
  *buf += bytes;
  *buflen -= bytes;
}

static int pbkdf2crypt_digestidx_by_string(const char *digest_idx_str)
{
  long digest_idx = strtol(digest_idx_str, NULL, 16);
  /* Skip "" entry at the end */
  if (PBKDF2CRYPT_DIGEST_IDX_INVALID(digest_idx))
    return -1;
  return digest_idx;
}

/* Write base64 PBKDF2 hash */
static int pbkdf2crypt_make_base64_hash(const EVP_MD *digest, const char *pass, int passlen, const unsigned char *salt, int saltlen, int rounds, char *out, int outlen)
{
  static unsigned char *buf;
  int digestlen;
  if (!buf)
    buf = nmalloc(maxdigestlen);
  digestlen = EVP_MD_size(digest);
  if (outlen < PBKDF2CRYPT_BASE64_ENC_LEN(digestlen))
    return -2;
  if (!PKCS5_PBKDF2_HMAC(pass, passlen, salt, saltlen, rounds, digest, maxdigestlen, buf))
    return -5;
  if (b64_ntop(buf, digestlen, out, outlen) < 0)
    return -5;
  return 0;
}

/* Encrypt a password with flexible settings for verification.
 * Returns 0 on success, -1 on digest not found, -2 on outbuffer too small, -3 on salt error, -4 on rounds outside range, -5 on pbkdf2 error
 */
static int pbkdf2crypt_verify_pass(const char *pass, int digest_idx, const unsigned char *salt, int saltlen, int rounds, char *out, int outlen)
{
  int size, ret;
  const EVP_MD *digest;

  if (PBKDF2CRYPT_DIGEST_IDX_INVALID(digest_idx))
    return -1;
  digest = digests[digest_idx].digest;
  if (!digest)
    return -1;
  size = pbkdf2crypt_get_size(digests[digest_idx].name, digest, saltlen);
  if (!out)
    return size;
  /* Sanity check */
  if (outlen < size)
    return -2;
  if (saltlen <= 0)
    return -3;
  if (rounds <= 0)
    return -4;

  bufcount(&out, &outlen, snprintf((char *) out, outlen, "$pbkdf2-%s$rounds=%i$", digests[digest_idx].name, (unsigned int) rounds));
  ret = b64_ntop(salt, saltlen, out, outlen);
  if (ret < 0) {
    return -2;
  }
  bufcount(&out, &outlen, PBKDF2CRYPT_BASE64_ENC_LEN(saltlen));
  bufcount(&out, &outlen, (out[0] = '$', 1));
  ret = pbkdf2crypt_make_base64_hash(digests[digest_idx].digest, pass, strlen(pass), salt, saltlen, rounds, out, outlen);
  if (ret != 0)
    return ret;
  return 0;
}

/* Encrypt a password with standard settings to store.
 * out = NULL returns necessary buffer size
 * Salt intentionally not static, so it gets overwritten.
 * Return values are the same as pbkdf2crypt_verify_pass
 */
static int pbkdf2crypt_pass(const char *pass, char *out, int outlen)
{
  unsigned char salt[PBKDF2CONF_SALTLEN];
  if (!out)
    return pbkdf2crypt_get_default_size();
  if (RAND_bytes(salt, sizeof salt) != 1)
    return -3;
  return pbkdf2crypt_verify_pass(pass, PBKDF2CONF_DIGESTIDX, salt, sizeof salt, PBKDF2CONF_ROUNDS, out, outlen);
}
