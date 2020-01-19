/*
 * pbkdf2.c -- part of pbkdf2.mod
 */
/*
 * Copyright (C) 2017 - 2020 Eggheads Development Team
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

#define MODULE_NAME "encryption2"

#include <resolv.h> /* base64 encode b64_ntop() and base64 decode b64_pton() */
#include <openssl/rand.h>
#include "src/mod/module.h"

#define PBKDF2_BASE64_DEC_LEN(x, len) pbkdf2_base64_dec_len((x), (len))
#define PBKDF2_BASE64_ENC_LEN(x) (4*(1+((x)-1)/3))
/* Preferred digest as array index from struct digests */
#define PBKDF2_DIGEST_IDX 0
/* Skip "" entry at the end */
#define PBKDF2_DIGEST_IDX_INVALID(idx) ((idx) < 0 || (idx) > sizeof digests / sizeof *digests - 2)
/* Default number of rounds if not explicitly specified */
#define PBKDF2_ROUNDS 5000
/* Salt string length */
#define PBKDF2_SALT_LEN 16

static Function *global = NULL;

/* Only ever add to the end, never remove, last one must have empty name.
 * This MUST be the same on all bots, or you might render your userfile useless.
 * This COULD also be subject to change in Eggdrop itself, just don't touch this list.
 */
static struct {
  const char name[16];
  const EVP_MD *digest;
} digests[] = {{"sha512"}, {"sha256"}, {""}};

static int maxdigestlen = 1;
static int pbkdf2_rounds = PBKDF2_ROUNDS;

static tcl_ints pbkdf2_ints[] = {
  {"pbkdf2-rounds", &pbkdf2_rounds, 0},
  {NULL,           NULL,          0}
};

static int pbkdf2_base64_dec_len(const unsigned char *str, int len)
{
  int pad = (str[len-1] == '=');
  if (pad && str[len-2] == '=')
    pad++;
  return len / 4 * 3 - pad;
}

static char *pbkdf2_close(void)
{
  return "You cannot unload the " MODULE_NAME " module.";
}

static int pbkdf2_get_size(const char* digest_name, const EVP_MD *digest, int saltlen)
{
  /* PHC string format */
  /* hash = "$pbkdf2-<digest>$rounds=<rounds>$<salt>$<hash>" */
  return strlen("$pbkdf2-") + strlen(digest_name) + 1 + strlen("rounds=FFFFFFFF") + 1 + PBKDF2_BASE64_ENC_LEN(saltlen) + 1 + PBKDF2_BASE64_ENC_LEN(EVP_MD_size(digest));
}

static int pbkdf2_get_default_size(void)
{
  return pbkdf2_get_size(digests[PBKDF2_DIGEST_IDX].name, digests[PBKDF2_DIGEST_IDX].digest, PBKDF2_SALT_LEN);
}

static void bufcount(char **buf, int *buflen, int bytes)
{
  *buf += bytes;
  *buflen -= bytes;
}

/* Write base64 PBKDF2 hash */
static int pbkdf2_make_base64_hash(const EVP_MD *digest, const char *pass, int passlen, const unsigned char *salt, int saltlen, int rounds, char *out, int outlen)
{
  static unsigned char *buf;
  int digestlen;
  if (!buf)
    buf = nmalloc(maxdigestlen);
  digestlen = EVP_MD_size(digest);
  if (outlen < PBKDF2_BASE64_ENC_LEN(digestlen))
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

  if (PBKDF2_DIGEST_IDX_INVALID(digest_idx))
    return -1;
  digest = digests[digest_idx].digest;
  if (!digest)
    return -1;
  size = pbkdf2_get_size(digests[digest_idx].name, digest, saltlen);
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
  bufcount(&out, &outlen, PBKDF2_BASE64_ENC_LEN(saltlen));
  bufcount(&out, &outlen, (out[0] = '$', 1));
  ret = pbkdf2_make_base64_hash(digests[digest_idx].digest, pass, strlen(pass), salt, saltlen, rounds, out, outlen);
  if (ret != 0)
    return ret;
  return 0;
}

/* Encrypt a password with standard settings to store.
 * out = NULL returns necessary buffer size
 * Salt intentionally not static, so it gets overwritten.
 * Return values are the same as pbkdf2crypt_verify_pass
 */
static int pbkdf2_pass(const char *pass, char *out, int outlen)
{
  unsigned char salt[PBKDF2_SALT_LEN];
  if (!out)
    return pbkdf2_get_default_size();
  if (RAND_bytes(salt, sizeof salt) != 1)
    return -3;
  return pbkdf2crypt_verify_pass(pass, PBKDF2_DIGEST_IDX, salt, sizeof salt, pbkdf2_rounds, out, outlen);
}

static char *pbkdf2_encrypt_pass(const char *pass)
{
  static int hashlen;
  static char *buf;

  if (!hashlen) {
    hashlen = pbkdf2_get_default_size();
    buf = nmalloc(hashlen + 1);
  }
  if (pbkdf2_pass(pass, buf, hashlen))
    return NULL;
  buf[hashlen] = '\0';
  return buf;
}

static int pbkdf2_digestidx_by_string(const char *digest_idx_str)
{
  long digest_idx = strtol(digest_idx_str, NULL, 16);
  /* Skip "" entry at the end */
  if (PBKDF2_DIGEST_IDX_INVALID(digest_idx))
    return -1;
  return digest_idx;
}

/* Return values:
 * -1 - cannot parse hash
 * 0 - no match
 * 1 - match
 * 2 - match, suggest re-hashing password (more rounds, new algorithm, ...)
 * PHC string format
 * hash = "$pbkdf2-<digest>$rounds=<rounds>$<salt>$<hash>"
 */
static int pbkdf2_verify_pass(const char *pass, const char *encrypted)
{
  int digest_idx, bufsize, ret, b64saltlen, saltlen;
  long rounds;
  char *buf;
  unsigned char *salt;
  const char *b64salt, *hash = encrypted;

  if (strncmp(hash, "$pbkdf2", strlen("$pbkdf2")))
    return -1;
  hash += strlen("$pbkdf2");

  digest_idx = pbkdf2_digestidx_by_string(hash);
  if (PBKDF2_DIGEST_IDX_INVALID(digest_idx))
    return -1;

  hash = strchr(hash, '$');
  if (!hash)
    return -1;
  /* TODO: check/skip "rounds=" ? */
  rounds = strtol(hash+1, (char **) &b64salt, 16);
  if (rounds > INT_MAX || rounds <= 0 || b64salt[0] != '$')
    return -1;

  hash = strchr(++b64salt, '$');
  b64saltlen = hash - b64salt;
  saltlen = PBKDF2_BASE64_DEC_LEN((unsigned char *)b64salt, b64saltlen);
  if (!hash || !++hash)
    return -1;

  bufsize = pbkdf2_get_size(digests[digest_idx].name, digests[digest_idx].digest, saltlen);
  buf = nmalloc(bufsize);
  salt = nmalloc(saltlen);
  b64saltlen = b64_pton(b64salt, salt, saltlen);
  if (b64saltlen == -1) {
    ret = -1;
    goto verify_pass_out;
  }

  if (pbkdf2crypt_verify_pass(pass, digest_idx, (unsigned char *)salt, saltlen, rounds, buf, bufsize) != 0) {
    ret = -1;
    goto verify_pass_out;
  }
  if (strncmp(encrypted, buf, bufsize)) {
    ret = 0;
    goto verify_pass_out;
  }
  /* match, check if we suggest re-hashing */
  if (pbkdf2_rounds > rounds || PBKDF2_DIGEST_IDX != digest_idx)
    ret = 2;
  else
    ret = 1;

verify_pass_out:
  nfree(buf);
  nfree(salt);
  return ret;
}

EXPORT_SCOPE char *pbkdf2_start();

static Function pbkdf2_table[] = {
  (Function) pbkdf2_start,
  (Function) pbkdf2_close,
  NULL, /* expmem */
  NULL, /* report */
  (Function) pbkdf2_encrypt_pass,
  (Function) pbkdf2_verify_pass
};

/* Initializes API with hash algorithm
 * Returns -1 on digest not found, module should report and unload
 */
static int pbkdf2_init(void)
{
  int i;
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
  if (PBKDF2_DIGEST_IDX_INVALID(PBKDF2_DIGEST_IDX)) {
    putlog(LOG_MISC, "*", "Invalid Digest IDX.");
    return -1;
  }
  return 0;
}

char *pbkdf2_start(Function *global_funcs)
{
  /* `global_funcs' is NULL if eggdrop is recovering from a restart.
   *
   * As the encryption module is never unloaded, only initialise stuff
   * that got reset during restart, e.g. the tcl bindings.
   */
  if (global_funcs) {
    global = global_funcs;
    if (!module_rename("pbkdf2", MODULE_NAME))
      return "Already loaded.";
    module_register(MODULE_NAME, pbkdf2_table, 0, 1);
    if (!module_depend(MODULE_NAME, "eggdrop", 109, 0)) {
      module_undepend(MODULE_NAME);
      return "This module requires Eggdrop 1.9.0 or later.";
    }
    if (pbkdf2_init() != 0) {
      module_undepend(MODULE_NAME);
      return "Initialization failure";
    }
    add_hook(HOOK_ENCRYPT_PASS2, (Function) pbkdf2_encrypt_pass);
    add_hook(HOOK_VERIFY_PASS2, (Function) pbkdf2_verify_pass);
    add_tcl_ints(pbkdf2_ints);
  }
  return NULL;
}
