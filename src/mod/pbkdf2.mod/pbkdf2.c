/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * pbkdf2.c -- part of pbkdf2.mod
 *
 * Written by thommey and Michael Ortmann
 *
 * Copyright (C) 2017 - 2020 Eggheads Development Team
 */

#include "src/mod/module.h"

#ifdef TLS
#define MODULE_NAME "encryption2"

#include <resolv.h> /* base64 encode b64_ntop() and base64 decode b64_pton() */
#include <sys/resource.h>
#include <openssl/err.h>
#include <openssl/rand.h>

/* Default number of rounds if not explicitly specified */
#define PBKDF2_ROUNDS 5000
/* Salt string length */
#define PBKDF2_SALT_LEN 16

static Function *global = NULL;

static char pbkdf2_method[28] = "SHA512";
static int pbkdf2_remove_old = 0; /* TODO: not implemented yet */
static int pbkdf2_rounds = PBKDF2_ROUNDS;

static tcl_ints my_tcl_ints[] = {
  {"pbkdf2-remove-old", &pbkdf2_remove_old, 0},
  {"pbkdf2-rounds",     &pbkdf2_rounds,     0},
  {NULL,                NULL,               0}
};

static tcl_strings my_tcl_strings[] = {
  {"pbkdf2-method", pbkdf2_method, 27, 0},
  {NULL,            NULL,          0,  0}
};

static char *pbkdf2_close(void)
{
  return "You cannot unload the " MODULE_NAME " module.";
}

static void bufcount(char **buf, int *buflen, int bytes)
{
  *buf += bytes;
  *buflen -= bytes;
}

int b64_ntop_without_padding(u_char const *src, size_t srclength, char *target, size_t targsize)
{
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

/* Encrypt a password with flexible settings for verification. */
static char *pbkdf2crypt_verify_pass(const char *pass, const char *digest_name, const unsigned char *salt, int saltlen, int rounds)
{
  const EVP_MD *digest;
  static int hashlen;  /* static object is initialized to zero (Standard C) */
  static char *out, *out2;
  int hashlen2, ret, digestlen;
  static unsigned char *buf;
  struct rusage ru1, ru2;

  digest = EVP_get_digestbyname(digest_name);
  if (!digest) {
    putlog(LOG_MISC, "*", "PBKDF2 error: Unknown message digest %s.", digest_name);
    return NULL;
  }
  /* Sanity check */
  if (saltlen <= 0) {
    putlog(LOG_MISC, "*", "PBKDF2 error: saltlen %i <= 0.", saltlen);
    return NULL;
  }
  if (rounds <= 0) {
    putlog(LOG_MISC, "*", "PBKDF2 error: rounds %i <= 0.", rounds);
    return NULL;
  }
  if (!hashlen) { /* TODO: hashlan may change if implemented */
    /* PHC string format
     * hash = "$pbkdf2-<digest>$rounds=<rounds>$<salt>$<hash>"
     */
    hashlen = strlen("$pbkdf2-") + strlen(digest_name) + strlen("$rounds=2147483647$") + B64_NTOP_CALCULATE_SIZE(saltlen) + 1 + B64_NTOP_CALCULATE_SIZE(EVP_MD_size(digest));
    out = nmalloc(hashlen + 1);
  }
  out2 = out;
  hashlen2 = hashlen;
  bufcount(&out2, &hashlen2, snprintf((char *) out2, hashlen2, "$pbkdf2-%s$rounds=%i$", digest_name, (unsigned int) rounds));
  ret = b64_ntop_without_padding(salt, saltlen, out2, hashlen2);
  if (ret < 0) {
    explicit_bzero(out, strlen(out));
    putlog(LOG_MISC, "*", "PBKDF2 error: Outbuffer too small (1).");
    return NULL;
  }
  bufcount(&out2, &hashlen2, ret);
  bufcount(&out2, &hashlen2, (out2[0] = '$', 1));
  /* Write base64 PBKDF2 hash */
  digestlen = EVP_MD_size(digest);
  debug3("DEBUG: hashlen %i B64_NTOP_CALCULATE_SIZE(digestlen) %i digestlen %i", hashlen2, B64_NTOP_CALCULATE_SIZE(digestlen), digestlen);
  if (hashlen2 < B64_NTOP_CALCULATE_SIZE(digestlen)) { /* TODO: do we need this? sanity check always true? does this code even make sense / is it correct ? */
    explicit_bzero(out, strlen(out));
    putlog(LOG_MISC, "*", "PBKDF2 error: hashlen2 < B64_NTOP_CALCULATE_SIZE(digestlen).");
    return NULL;
  }
  if (!buf) /* TODO ? */
    buf = nmalloc(digestlen); /* size ? */
  ret = getrusage(RUSAGE_SELF, &ru1);
  if (!PKCS5_PBKDF2_HMAC(pass, strlen(pass), salt, saltlen, rounds, digest, digestlen, buf)) {
    explicit_bzero(out, strlen(out));
    putlog(LOG_MISC, "*", "PBKDF2 error: PKCS5_PBKDF2_HMAC().");
    return NULL;
  }
  if (!ret && !getrusage(RUSAGE_SELF, &ru2)) {
    debug4("pbkdf2 method %s rounds %i, user %.3fms sys %.3fms", pbkdf2_method, pbkdf2_rounds,
           (double) (ru2.ru_utime.tv_usec - ru1.ru_utime.tv_usec) / 1000 +
           (double) (ru2.ru_utime.tv_sec  - ru1.ru_utime.tv_sec ) * 1000,
           (double) (ru2.ru_stime.tv_usec - ru1.ru_stime.tv_usec) / 1000 +
           (double) (ru2.ru_stime.tv_sec  - ru1.ru_stime.tv_sec ) * 1000);
  }
  else {
    debug1("PBKDF2 error: getrusage(): %s", strerror(errno));
  }
  if (b64_ntop_without_padding(buf, digestlen, out2, hashlen2) < 0) {
    explicit_bzero(out, strlen(out));
    putlog(LOG_MISC, "*", "PBKDF2 error: Outbuffer too small (2).");
    return NULL;
  }
  return out;
}

static char *pbkdf2_encrypt_pass(const char *pass)
{
  unsigned char salt[PBKDF2_SALT_LEN];
  static char *buf;

  /* Encrypt a password with standard settings to store. */
  if (RAND_bytes(salt, sizeof salt) != 1) {
    /* TODO: Do we need ERR_load_crypto_strings(), SSL_load_error_strings() and ERR_free_strings(void) ? */
    putlog(LOG_MISC, "*", "PBKDF2 error: %s.", ERR_error_string(ERR_get_error(), NULL));
    return NULL;
  }
  if (!(buf = pbkdf2crypt_verify_pass(pass, pbkdf2_method, salt, sizeof salt, pbkdf2_rounds)))
    return NULL;
  return buf;
}

/* PHC string format
 * hash = "$pbkdf2-<digest>$rounds=<rounds>$<salt>$<hash>"
 */
static int pbkdf2_verify_pass(const char *pass, const char *encrypted)
{
  char method[sizeof pbkdf2_method],
       b64salt[B64_NTOP_CALCULATE_SIZE(PBKDF2_SALT_LEN) + 1],
       b64hash[B64_NTOP_CALCULATE_SIZE(256) + 1];
  unsigned int rounds;
  const EVP_MD *digest;
  unsigned char salt[PBKDF2_SALT_LEN + 1];
  int saltlen;
  static char *buf;

  /* TODO: different salt lengths wont work yet i guess and overflow may still be possible */
  if (!sscanf(encrypted, "$pbkdf2-%27[^$]$rounds=%u$%24[^$]$%344s", method, &rounds, b64salt, b64hash)) {
    putlog(LOG_MISC, "*", "PBKDF2 error: could not parse hashed password.");
    return 1;
  }
  digest = EVP_get_digestbyname(method);
  if (!digest) {
    putlog(LOG_MISC, "*", "PBKDF2 error: EVP_get_digestbyname(%s).", method);
    return 1;
  }
  if (b64salt[22] == 0) {
    b64salt[22] = '=';
    b64salt[23] = '=';
    b64salt[24] = 0;
  }
  else if (b64salt[23] == 0) {
    b64salt[23] = '=';
    b64salt[24] = 0;
  }
  saltlen = b64_pton(b64salt, salt, sizeof salt);
  if (saltlen < 0) {
    putlog(LOG_MISC, "*", "PBKDF2 error: b64_pton(%s).", b64salt);
    return 1;
  }
  if (!(buf = pbkdf2crypt_verify_pass(pass, method, salt, saltlen, rounds)))
    return 1;
  if (strcmp(encrypted, buf)) {
    putlog(LOG_MISC, "*", "PBKDF2 error: strncmp(hashlen):\n  %s\n  %s.", encrypted, buf);
    /* TODO: re-hashing password (new method, more rounds)
     * if (strncmp(method, pbkdf2_method, sizeof pbkdf2_method) || rounds != pbkdf2_rounds)
     */
    explicit_bzero(buf, strlen(buf));
    return 1;
  }
  explicit_bzero(buf, strlen(buf));
  return 0;
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

/* Initializes API with hash algorithm */
static int pbkdf2_init(void)
{
  const EVP_MD *digest;
  OpenSSL_add_all_digests();
  digest = EVP_get_digestbyname(pbkdf2_method);
  if (!digest) {
    putlog(LOG_MISC, "*", "PBKDF2 error: Failed to initialize digest '%s'.", pbkdf2_method);
    return 1;
  }
  if (!RAND_status()) {
    putlog(LOG_MISC, "*", "PBKDF2 error: random generator has not been seeded with enough data.");
    return 1;
  }
  return 0;
}

#endif
char *pbkdf2_start(Function *global_funcs)
{
#ifdef TLS
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
    if (pbkdf2_init()) {
      module_undepend(MODULE_NAME);
      return "Initialization failure";
    }
    add_hook(HOOK_ENCRYPT_PASS2, (Function) pbkdf2_encrypt_pass);
    add_hook(HOOK_VERIFY_PASS2, (Function) pbkdf2_verify_pass);
    add_tcl_ints(my_tcl_ints);
    add_tcl_strings(my_tcl_strings);
  }
  return NULL;
#else
  return "Initialization failure: configured with --disable-TLS or openssl not found";
#endif
}
