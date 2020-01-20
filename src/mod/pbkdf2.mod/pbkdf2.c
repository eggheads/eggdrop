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
#include <sys/resource.h>
#include <openssl/rand.h>
#include "src/mod/module.h"

/* TODO: move the following 2 macros to base64.h and change servmsg.c to also use them */
#define B64_NTOP_CALCULATE_SIZE(x) ((x + 2) / 3 * 4 + 1)
#define B64_PTON_CALCULATE_SIZE(x) (x * 3 / 4)
/* Default number of rounds if not explicitly specified */
#define PBKDF2_ROUNDS 5000
/* Salt string length */
#define PBKDF2_SALT_LEN 16

static Function *global = NULL;

/*  const EVP_MD *digest; */

static char pbkdf2_method[28] = "SHA512"; /* TODO: can we do this or do we have to use strlcpy before init()? */
static int pbkdf2_rounds = PBKDF2_ROUNDS;

static tcl_ints my_tcl_ints[] = {
  {"pbkdf2-rounds", &pbkdf2_rounds, 0},
  {NULL,           NULL,          0}
};

static tcl_strings my_tcl_strings[] = {
  {"pbkdf2-method", pbkdf2_method, 27, 0},
  {NULL,           NULL,          0}
};

static char *pbkdf2_close(void)
{
  return "You cannot unload the " MODULE_NAME " module.";
}

static int pbkdf2_get_size(const char* digest_name, const EVP_MD *digest, int saltlen)
{
  /* PHC string format */
  /* hash = "$pbkdf2-<digest>$rounds=<rounds>$<salt>$<hash>" */
  return strlen("$pbkdf2-") + strlen(digest_name) + strlen("$rounds=FFFFFFFF$") + B64_NTOP_CALCULATE_SIZE(saltlen) + 1 + B64_NTOP_CALCULATE_SIZE(EVP_MD_size(digest));
}

static void bufcount(char **buf, int *buflen, int bytes)
{
  *buf += bytes;
  *buflen -= bytes;
}

int b64_ntop_without_padding(u_char const *src, size_t srclength, char *target, size_t targsize) {
  char *c;

  if (b64_ntop(src, srclength, target, targsize) < 0)
    return -1;
  c = strchr(target, '=');
  if (c) {
    *c = 0;
    targsize = (c - target);
  }
  return targsize;
}

/* Write base64 PBKDF2 hash */
static int pbkdf2_make_base64_hash(const EVP_MD *digest, const char *pass, int passlen, const unsigned char *salt, int saltlen, int rounds, char *out, int outlen)
{
  static unsigned char *buf;
  int digestlen, r;
  struct rusage ru1, ru2;

  digestlen = EVP_MD_size(digest);
  if (!buf)
    buf = nmalloc(digestlen);
  if (outlen < B64_NTOP_CALCULATE_SIZE(digestlen))
    return -2;
  r = getrusage(RUSAGE_SELF, &ru1);
  if (!PKCS5_PBKDF2_HMAC(pass, passlen, salt, saltlen, rounds, digest, digestlen, buf))
    return -5;
  if (!r && !getrusage(RUSAGE_SELF, &ru2)) {
    debug4("pbkdf2 method %s rounds %i, user %.3fms sys %.3fms", pbkdf2_method, pbkdf2_rounds,
           (double) (ru2.ru_utime.tv_usec - ru1.ru_utime.tv_usec) / 1000 +
           (double) (ru2.ru_utime.tv_sec  - ru1.ru_utime.tv_sec ) * 1000,
           (double) (ru2.ru_stime.tv_usec - ru1.ru_stime.tv_usec) / 1000 +
           (double) (ru2.ru_stime.tv_sec  - ru1.ru_stime.tv_sec ) * 1000);
  }
  if (b64_ntop_without_padding(buf, digestlen, out, outlen) < 0) 
    return -5;
  return 0;
}

/* Encrypt a password with flexible settings for verification.
 * Returns 0 on success, -1 on digest not found, -2 on outbuffer too small, -3 on salt error, -4 on rounds outside range, -5 on pbkdf2 error
 */
static int pbkdf2crypt_verify_pass(const char *pass, const char *digest_name, const unsigned char *salt, int saltlen, int rounds, char *out, int outlen)
{
  int size, ret;
  const EVP_MD *digest;

  digest = EVP_get_digestbyname(digest_name);
  if (!digest)
    return -1;
  size = pbkdf2_get_size(digest_name, digest, saltlen);
  if (!out)
    return size;
  /* Sanity check */
  if (outlen < size)
    return -2;
  if (saltlen <= 0)
    return -3;
  if (rounds <= 0)
    return -4;

  bufcount(&out, &outlen, snprintf((char *) out, outlen, "$pbkdf2-%s$rounds=%i$", digest_name, (unsigned int) rounds));
  ret = b64_ntop_without_padding(salt, saltlen, out, outlen);
  if (ret < 0) {
    return -2;
  }
  bufcount(&out, &outlen, ret);
  bufcount(&out, &outlen, (out[0] = '$', 1));
  ret = pbkdf2_make_base64_hash(digest, pass, strlen(pass), salt, saltlen, rounds, out, outlen);
  if (ret != 0)
    return ret;
  return 0;
}

/* Encrypt a password with standard settings to store.
 * out = NULL returns necessary buffer size
 * Return values are the same as pbkdf2crypt_verify_pass
 */
static int pbkdf2_pass(const char *pass, char *out, int outlen)
{
  unsigned char salt[PBKDF2_SALT_LEN];
  if (!out)
    return pbkdf2_get_size(pbkdf2_method, EVP_get_digestbyname(pbkdf2_method), PBKDF2_SALT_LEN);
  if (RAND_bytes(salt, sizeof salt) != 1)
    return -3;
  return pbkdf2crypt_verify_pass(pass, pbkdf2_method, salt, sizeof salt, pbkdf2_rounds, out, outlen);
}

static char *pbkdf2_encrypt_pass(const char *pass)
{
  static int hashlen;
  static char *buf;

  if (!hashlen) {
    hashlen = pbkdf2_get_size(pbkdf2_method, EVP_get_digestbyname(pbkdf2_method), PBKDF2_SALT_LEN);
    buf = nmalloc(hashlen + 1);
  }
  if (pbkdf2_pass(pass, buf, hashlen))
    return NULL;
  buf[hashlen] = '\0';
  return buf;
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
  int bufsize, ret, b64saltlen, saltlen;
  long rounds;
  char *buf;
  unsigned char *salt;
  const char *b64salt, *hash = encrypted, *digest_name;
  const EVP_MD *digest;

  if (strncmp(hash, "$pbkdf2-", strlen("$pbkdf2-")))
    return -1;
  hash += strlen("$pbkdf2-");

  digest_name = hash;
  digest = EVP_get_digestbyname(digest_name);

  hash = strchr(hash, '$');
  if (!hash)
    return -1;
  /* TODO: check/skip "rounds=" ? */
  rounds = strtol(hash+1, (char **) &b64salt, 16);
  if (rounds > INT_MAX || rounds <= 0 || b64salt[0] != '$')
    return -1;

  hash = strchr(++b64salt, '$');
  b64saltlen = hash - b64salt;
  saltlen = B64_PTON_CALCULATE_SIZE(b64saltlen);
  if (!hash || !++hash)
    return -1;

  bufsize = pbkdf2_get_size(digest_name, digest, saltlen);
  buf = nmalloc(bufsize);
  salt = nmalloc(saltlen);
  b64saltlen = b64_pton(b64salt, salt, saltlen);
  if (b64saltlen == -1) {
    ret = -1;
    goto verify_pass_out;
  }

  if (pbkdf2crypt_verify_pass(pass, digest_name, (unsigned char *)salt, saltlen, rounds, buf, bufsize) != 0) {
    ret = -1;
    goto verify_pass_out;
  }
  if (strncmp(encrypted, buf, bufsize)) {
    ret = 0;
    goto verify_pass_out;
  }
  /* match, check if we suggest re-hashing */
  if (pbkdf2_rounds > rounds || pbkdf2_method != digest_name)
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
  const EVP_MD *digest;
  OpenSSL_add_all_digests();
  digest = EVP_get_digestbyname(pbkdf2_method);
  if (!digest) {
    putlog(LOG_MISC, "*", "Failed to initialize digest '%s'.", pbkdf2_method);
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
    add_tcl_ints(my_tcl_ints);
    add_tcl_strings(my_tcl_strings);
  }
  return NULL;
}
