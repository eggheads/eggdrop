/*
 * pbkdf2.c -- part of pbkdf2.mod
 *   PBKDF2 password hashing, module API
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

#define MODULE_NAME "encryption2"

#include "src/mod/module.h"

static Function *global = NULL;

/* Cryptographic functions */
#include "pbkdf2crypt.c"

EXPORT_SCOPE char *pbkdf2_start();
static char *pbkdf2_close(void);
static char *pbkdf2_encrypt_pass(const char *pass);
static int pbkdf2_verify_pass(const char *pass, const char *hash);

static Function exported_functions[] = {
  (Function) pbkdf2_start,
  (Function) pbkdf2_close,
  (Function) NULL, /* expmem */
  (Function) NULL, /* report */
  (Function) pbkdf2_encrypt_pass,
  (Function) pbkdf2_verify_pass
};

static char *pbkdf2_encrypt_pass(const char *pass)
{
  static int hashlen;
  static char *buf;

  if (!hashlen) {
    hashlen = pbkdf2crypt_get_default_size();
    buf = nmalloc(hashlen + 1);
  }
  if (pbkdf2crypt_pass(pass, buf, hashlen))
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
  int digest_idx, bufsize, ret, b64saltlen, saltlen;
  long rounds;
  char *buf;
  unsigned char *salt;
  const char *b64salt, *hash = encrypted;

  if (strncmp(hash, "$pbkdf2", strlen("$pbkdf2")))
    return -1;
  hash += strlen("$pbkdf2");

  digest_idx = pbkdf2crypt_digestidx_by_string(hash);
  if (PBKDF2CRYPT_DIGEST_IDX_INVALID(digest_idx))
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
  saltlen = PBKDF2CRYPT_BASE64_DEC_LEN((unsigned char *)b64salt, b64saltlen);
  if (!hash || !++hash)
    return -1;

  bufsize = pbkdf2crypt_get_size(digests[digest_idx].name, digests[digest_idx].digest, saltlen);
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
  if (PBKDF2CONF_ROUNDS > rounds || PBKDF2CONF_DIGESTIDX != digest_idx)
    ret = 2;
  else
    ret = 1;

verify_pass_out:
  nfree(buf);
  nfree(salt);
  return ret;
}

/* Encrypt a password for verification.
 * out = NULL returns necessary buffer size
 * Returns 0 on success, -1 on invalid digest, -2 on outbuffer too small
 */
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
    module_register(MODULE_NAME, exported_functions, 0, 1);
    if (!module_depend(MODULE_NAME, "eggdrop", 109, 0)) {
      module_undepend(MODULE_NAME);
      return "This module requires Eggdrop 1.9.0 or later.";
    }
    if (pbkdf2crypt_init() != 0) {
      module_undepend(MODULE_NAME);
      return "Initialization failure";
    }
    add_hook(HOOK_ENCRYPT_PASS2, (Function) pbkdf2_encrypt_pass);
    add_hook(HOOK_VERIFY_PASS2, (Function) pbkdf2_verify_pass);
  }
  return NULL;
}

static char *pbkdf2_close(void)
{
  return "You cannot unload the PBKDF2 module.";
}
