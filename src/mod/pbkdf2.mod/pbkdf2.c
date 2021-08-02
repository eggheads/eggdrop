/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * pbkdf2.c -- part of pbkdf2.mod
 *
 * Written by thommey and Michael Ortmann
 *
 * Copyright (C) 2017 - 2021 Eggheads Development Team
 */

#include "src/mod/module.h"

#if OPENSSL_VERSION_NUMBER >= 0x1000000fL /* 1.0.0 */
#define MODULE_NAME "encryption2"

#include <resolv.h> /* base64 encode b64_ntop() and base64 decode b64_pton() */
#include <sys/resource.h>
#include <openssl/err.h>
#include <openssl/rand.h>

/* Salt string length */
#define PBKDF2_SALT_LEN 16 /* DO NOT TOUCH! */

static Function *global = NULL;

/* Cryptographic hash function used. openssl list -digest-algorithms */
static char pbkdf2_method[28] = "SHA256";
/* Enable re-encoding of password if pbkdf2-method and / or pbkdf2-rounds
 * change.
 */
static int pbkdf2_re_encode = 1;
/* Number of rounds (iterations) */
static int pbkdf2_rounds = 16000;

static char *pbkdf2_close(void)
{
  return "You cannot unload the " MODULE_NAME " module.";
}

static void bufcount(char **buf, int *buflen, int bytes)
{
  *buf += bytes;
  *buflen -= bytes;
}

static int b64_ntop_without_padding(u_char const *src, size_t srclength,
                                    char *target, size_t targsize)
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

/* Return
 *   hash = "$pbkdf2-<digest>$rounds=<rounds>$<salt>$<hash>" (PHC string format)
 *     salt and hash = base64
 *   NULL = error
 */
static char *pbkdf2_hash(const char *pass, const char *digest_name,
                         const unsigned char *salt, unsigned int saltlen,
                         unsigned int rounds)
{
  const EVP_MD *digest;
  int digestlen, ret;
  int outlen, restlen;
  static char out[256]; /* static object is initialized to zero (Standard C) */
  char *out2;
  unsigned char *buf;
  struct rusage ru1, ru2;

  digest = EVP_get_digestbyname(digest_name);
  if (!digest) {
    putlog(LOG_MISC, "*", "PBKDF2 error: Unknown message digest '%s'.",
           digest_name);
    return NULL;
  }
  digestlen = EVP_MD_size(digest);
  outlen = strlen("$pbkdf2-") + strlen(digest_name) +
           strlen("$rounds=4294967295$i") + B64_NTOP_CALCULATE_SIZE(saltlen) +
           1 + B64_NTOP_CALCULATE_SIZE(digestlen);
  if ((outlen + 1) > sizeof out) {
    putlog(LOG_MISC, "*", "PBKDF2 error: outlen %i > sizeof out %i.", outlen,
           sizeof out);
    return NULL;
  }
  out2 = out;
  restlen = outlen;
  bufcount(&out2, &restlen, snprintf((char *) out2, restlen,
           "$pbkdf2-%s$rounds=%u$", digest_name, rounds));
  ret = b64_ntop_without_padding(salt, saltlen, out2, restlen);
  if (ret < 0) {
    explicit_bzero(out, outlen);
    putlog(LOG_MISC, "*", "PBKDF2 error: b64_ntop(salt).");
    return NULL;
  }
  bufcount(&out2, &restlen, ret);
  out2[0] = '$';
  bufcount(&out2, &restlen, 1);
  buf = nmalloc(digestlen);
  ret = getrusage(RUSAGE_SELF, &ru1);
  if (!PKCS5_PBKDF2_HMAC(pass, strlen(pass), salt, saltlen, rounds, digest,
                         digestlen, buf)) {
    explicit_bzero(buf, digestlen);
    explicit_bzero(out, outlen);
    putlog(LOG_MISC, "*", "PBKDF2 error: PKCS5_PBKDF2_HMAC(): %s.",
           ERR_error_string(ERR_get_error(), NULL));
    nfree(buf);
    return NULL;
  }
  if (!ret && !getrusage(RUSAGE_SELF, &ru2)) {
    debug4("pbkdf2 method %s rounds %i, user %.3fms sys %.3fms", digest_name,
           rounds,
           (double) (ru2.ru_utime.tv_usec - ru1.ru_utime.tv_usec) / 1000 +
           (double) (ru2.ru_utime.tv_sec  - ru1.ru_utime.tv_sec ) * 1000,
           (double) (ru2.ru_stime.tv_usec - ru1.ru_stime.tv_usec) / 1000 +
           (double) (ru2.ru_stime.tv_sec  - ru1.ru_stime.tv_sec ) * 1000);
  }
  else {
    debug1("PBKDF2 error: getrusage(): %s", strerror(errno));
  }
  if (b64_ntop_without_padding(buf, digestlen, out2, restlen) < 0) {
    explicit_bzero(out, outlen);
    putlog(LOG_MISC, "*", "PBKDF2 error: b64_ntop(hash).");
    nfree(buf);
    return NULL;
  }
  nfree(buf);
  return out;
}

/* Return
 *   hash = "$pbkdf2-<digest>$rounds=<rounds>$<salt>$<hash>" (PHC string format)
 *     salt and hash = base64
 *   NULL = error
 */
static char *pbkdf2_encrypt(const char *pass)
{
  unsigned char salt[PBKDF2_SALT_LEN];
  static char *buf;

  if (RAND_bytes(salt, sizeof salt) != 1) {
    putlog(LOG_MISC, "*", "PBKDF2 error: RAND_bytes(): %s.",
           ERR_error_string(ERR_get_error(), NULL));
    return NULL;
  }
  if (!(buf = pbkdf2_hash(pass, pbkdf2_method, salt, sizeof salt,
                          pbkdf2_rounds))) {
    explicit_bzero(salt, sizeof salt);
    return NULL;
  }
  explicit_bzero(salt, sizeof salt);
  return buf;
}

/* Return
 *   hash = "$pbkdf2-<digest>$rounds=<rounds>$<salt>$<hash>" (PHC string format)
 *     salt and hash = base64
 *     old encrypted = verify successful
 *     new encrypted = verify successful, reenrypted with new parameters
 *   NULL = verify failed
 */
static char *pbkdf2_verify(const char *pass, const char *encrypted)
{
  char method[sizeof pbkdf2_method],
       b64salt[B64_NTOP_CALCULATE_SIZE(PBKDF2_SALT_LEN) + 1],
       b64hash[B64_NTOP_CALCULATE_SIZE(256) + 1];
  char format[40];
  unsigned int rounds;
  const EVP_MD *digest;
  unsigned char salt[PBKDF2_SALT_LEN + 1];
  int saltlen;
  static char *buf;

  if (snprintf(format, sizeof format,
               "$pbkdf2-%%%zu[^$]$rounds=%%u$%%%zu[^$]$%%%zus",
               (sizeof method) - 1, (sizeof b64salt) - 1, (sizeof b64hash) - 1)
      != (sizeof format) - 1) {
    putlog(LOG_MISC, "*", "PBKDF2 error: could not initialize parser for hashed password.");
    return NULL;
  }
  if (sscanf(encrypted, format, method, &rounds, b64salt, b64hash) != 4) {
    putlog(LOG_MISC, "*", "PBKDF2 error: could not parse hashed password.");
    return NULL;
  }
  digest = EVP_get_digestbyname(method);
  if (!digest) {
    putlog(LOG_MISC, "*", "PBKDF2 error: Unknown message digest '%s'.", method);
    return NULL;
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
    return NULL;
  }
  if (!(buf = pbkdf2_hash(pass, method, salt, saltlen, rounds))) {
    explicit_bzero(salt, saltlen);
    return NULL;
  }
  explicit_bzero(salt, saltlen);
  if (crypto_verify(encrypted, buf)) {
    explicit_bzero(buf, strlen(buf));
    return NULL;
  }
  explicit_bzero(buf, strlen(buf));
  if (pbkdf2_re_encode &&
      ((rounds != pbkdf2_rounds) || strcmp(method, pbkdf2_method)))
    return pbkdf2_encrypt(pass);
  return (char *) encrypted;
}

static int tcl_encpass2 STDVAR
{
  BADARGS(2, 2, " string");
  if (strlen(argv[1]) > 0)
    Tcl_AppendResult(irp, pbkdf2_encrypt(argv[1]), NULL);
  else
    Tcl_AppendResult(irp, "", NULL);
  return TCL_OK;
}

static tcl_cmds my_tcl_cmds[] = {
  {"encpass2", tcl_encpass2},
  {NULL,       NULL}
};

static tcl_ints my_tcl_ints[] = {
  {"pbkdf2-re-encode", &pbkdf2_re_encode, 0},
  {"pbkdf2-rounds",    &pbkdf2_rounds,    0},
  {NULL,               NULL,              0}
};

static tcl_strings my_tcl_strings[] = {
  {"pbkdf2-method", pbkdf2_method, 27, 0},
  {NULL,            NULL,          0,  0}
};

EXPORT_SCOPE char *pbkdf2_start();

static Function pbkdf2_table[] = {
  (Function) pbkdf2_start,
  (Function) pbkdf2_close,
  NULL, /* expmem */
  NULL, /* report */
  (Function) pbkdf2_encrypt,
  (Function) pbkdf2_verify
};

/* Initializes API with hash algorithm */
static int pbkdf2_init(void)
{
  const EVP_MD *digest;
  /* OpenSSL library initialization
   * If you are using 1.1.0 or above then you don't need to take any further
   * steps. */
#if OPENSSL_VERSION_NUMBER < 0x10100000L /* 1.1.0 */
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();
#endif
  digest = EVP_get_digestbyname(pbkdf2_method);
  if (!digest) {
    putlog(LOG_MISC, "*", "PBKDF2 error: Unknown message digest '%s'.",
           pbkdf2_method);
    return 1;
  }
  if (!RAND_status()) {
    putlog(LOG_MISC, "*", "PBKDF2 error: openssl random generator has not been seeded with enough data.");
    return 1;
  }
  return 0;
}

#endif
char *pbkdf2_start(Function *global_funcs)
{
#if OPENSSL_VERSION_NUMBER >= 0x1000000fL /* 1.0.0 */

  /* `global_funcs' is NULL if eggdrop is recovering from a restart.
   *
   * As the encryption module is never unloaded, only initialise stuff
   * that got reset during restart, e.g. the tcl bindings.
   */
  if (global_funcs) {
    global = global_funcs;
    if (!module_rename("pbkdf2", MODULE_NAME))
      return "Already loaded.";
    module_register(MODULE_NAME, pbkdf2_table, 1, 0);
    if (!module_depend(MODULE_NAME, "eggdrop", 109, 0)) {
      module_undepend(MODULE_NAME);
      return "This module requires Eggdrop 1.9.0 or later.";
    }
    if (pbkdf2_init()) {
      module_undepend(MODULE_NAME);
      return "Initialization failure";
    }
    add_hook(HOOK_ENCRYPT_PASS2, (Function) pbkdf2_encrypt);
    add_hook(HOOK_VERIFY_PASS2, (Function) pbkdf2_verify);
    add_tcl_commands(my_tcl_cmds);
    add_tcl_ints(my_tcl_ints);
    add_tcl_strings(my_tcl_strings);
  }
  return NULL;
#else
  #ifdef TLS
    return "Initialization failure: compiled with openssl version < 1.0.0";
  #else
    return "Initialization failure: configured with --disable-TLS or openssl not found";
  #endif
#endif
}
