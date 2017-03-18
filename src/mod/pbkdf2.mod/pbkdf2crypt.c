/*
 * pbkdf2crypt.c -- part of pbkdf2.mod
 *   PBKDF2 password hashing, cryptographic functions
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

static int pbkdf2crypt_base64_dec_len(const unsigned char *str, int len);

#define PBKDF2CRYPT_BASE64_ENC_LEN(x) (4*(1+((x)-1)/3))
#define PBKDF2CRYPT_BASE64_DEC_LEN(x, len) pbkdf2crypt_base64_dec_len((x), (len))

/* Load crypto module dependent routines */
#if defined(MAKING_DEPEND) || defined(PBKDF2_OPENSSL)
#  include "pbkdf2crypt_openssl.c"
#endif

#if defined(MAKING_DEPEND) || !defined(PBKDF2_OPENSSL)
#  include "pbkdf2crypt_mbedtls.c"
#endif

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
  const digest_t *digest;
} digests[] = {{"SHA256"}, {""}};

static int maxdigestlen = 1;

/* Preferred digest as array index from above struct */
#define PBKDF2CONF_DIGESTIDX 0
/* Salt length in bytes, will be base64 encoded after (so, pick something divisible by 3) */
#define PBKDF2CONF_SALTLEN 24
/* Rounds for PBKDF2 */
#define PBKDF2CONF_CYCLES 16000

/* Skip "" entry at the end */
#define PBKDF2CRYPT_DIGEST_IDX_INVALID(idx) ((idx) < 0 || (idx) > sizeof digests / sizeof *digests - 2)

/* Initializes API with hash algorithm
 * Returns -1 on digest not found, module should report and unload
 */
static int pbkdf2crypt_init(void)
{
  int i;
  if (PBKDF2CONF_CYCLES <= 0 || PBKDF2CONF_CYCLES > INT_MAX) {
    putlog(LOG_MISC, "*", "Cycle value outside its allowed range.");
    return -1;
  }
  if (PBKDF2CRYPT_INIT_MODULE() != 0) {
    putlog(LOG_MISC, "*", "Failed to initialize RNG.");
    return -1;
  }
  for (i = 0; digests[i].name[0]; i++) {
    digests[i].digest = PBKDF2CRYPT_GET_DIGEST_BY_NAME(digests[i].name);
    if (!digests[i].digest) {
      putlog(LOG_MISC, "*", "Failed to initialize digests '%s'.", digests[i].name);
      return -1;
    }
    if (PBKDF2CRYPT_GET_DIGEST_SIZE(digests[i].digest) > maxdigestlen)
      maxdigestlen = PBKDF2CRYPT_GET_DIGEST_SIZE(digests[i].digest);
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

static int pbkdf2crypt_get_size(const digest_t *digest, int saltlen)
{
  /* "$PBKDF2$<digestID>$<cycles>$<salt>$<hash>$" */
  return strlen("$PBKDF2$") + strlen("FF") + 1 + strlen("FFFFFFFF") + 1 + PBKDF2CRYPT_BASE64_ENC_LEN(saltlen) + 1 + PBKDF2CRYPT_BASE64_ENC_LEN(PBKDF2CRYPT_GET_DIGEST_SIZE(digest)) + 1;
}

static int pbkdf2crypt_get_default_size(void)
{
  return pbkdf2crypt_get_size(digests[PBKDF2CONF_DIGESTIDX].digest, PBKDF2CONF_SALTLEN);
}

static void bufcount(unsigned char **buf, int *buflen, int bytes)
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
static int pbkdf2crypt_make_base64_hash(const digest_t *digest, const char *pass, int passlen, const unsigned char *salt, int saltlen, int cycles, unsigned char *out, int outlen)
{
  static unsigned char *buf;
  int digestlen;
  if (!buf)
    buf = nmalloc(maxdigestlen);
  digestlen = PBKDF2CRYPT_GET_DIGEST_SIZE(digest);
  if (outlen < PBKDF2CRYPT_BASE64_ENC_LEN(digestlen))
    return -2;
  if (PBKDF2CRYPT_MAKE_HASH(digest, pass, passlen, salt, saltlen, cycles, maxdigestlen, buf) != 0)
    return -5;
  if (PBKDF2CRYPT_BASE64_ENC(out, outlen, buf, digestlen) != 0)
    return -5;
  return 0;
}

/* Encrypt a password with flexible settings for verification.
 * Returns 0 on success, -1 on digest not found, -2 on outbuffer too small, -3 on salt error, -4 on cycles outside range, -5 on pbkdf2 error
 */
static int pbkdf2crypt_verify_pass(const char *pass, int digest_idx, const unsigned char *salt, int saltlen, int cycles, unsigned char *out, int outlen)
{
  int size, ret;
  const digest_t *digest;

  if (PBKDF2CRYPT_DIGEST_IDX_INVALID(digest_idx))
    return -1;
  digest = digests[digest_idx].digest;
  size = pbkdf2crypt_get_size(digest, saltlen);
  if (!digest)
    return -1;
  if (!out)
    return size;
  /* Sanity check */
  if (outlen < size)
    return -2;
  if (saltlen <= 0)
    return -3;
  if (cycles <= 0)
    return -4;

  bufcount(&out, &outlen, snprintf((char *)out, outlen, "$PBKDF2$%02lX$%08X$", (unsigned long)digest_idx, (unsigned int)cycles));
  ret = PBKDF2CRYPT_BASE64_ENC(out, outlen, salt, saltlen);
  if (ret != 0)
    return -2;
  bufcount(&out, &outlen, PBKDF2CRYPT_BASE64_ENC_LEN(saltlen));
  bufcount(&out, &outlen, (out[0] = '$', 1));
  ret = pbkdf2crypt_make_base64_hash(digests[digest_idx].digest, pass, strlen(pass), salt, saltlen, cycles, out, outlen);
  if (ret != 0)
    return ret;
  out[PBKDF2CRYPT_BASE64_ENC_LEN(PBKDF2CRYPT_GET_DIGEST_SIZE(digest))] = '$';
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
  if (PBKDF2CRYPT_MAKE_SALT(salt, sizeof salt) != 0)
    return -3;
  return pbkdf2crypt_verify_pass(pass, PBKDF2CONF_DIGESTIDX, salt, sizeof salt, PBKDF2CONF_CYCLES, (unsigned char *)out, outlen);
}

