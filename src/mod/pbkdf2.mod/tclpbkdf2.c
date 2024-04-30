/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * tclpbkdf2.c -- tcl functions for pbkdf2.mod
 *
 * Written by thommey and Michael Ortmann
 *
 * Copyright (C) 2017 - 2024 Eggheads Development Team
 */

#include <openssl/err.h>

static char *pbkdf2_encrypt(const char *);

static int tcl_encpass2 STDVAR
{
  BADARGS(2, 2, " string");
  Tcl_SetResult(irp, pbkdf2_encrypt(argv[1]), TCL_STATIC);
  return TCL_OK;
}

static int tcl_pbkdf2 STDVAR
{
  unsigned int rounds;
  const EVP_MD *digest;
  int digestlen;
  unsigned char buf[256];

  BADARGS(5, 5, " pass salt rounds digest");
  rounds = atoi(argv[3]);
  digest = EVP_get_digestbyname(argv[4]);
  if (!digest) {
    Tcl_AppendResult(irp, "PBKDF2 error: Unknown message digest '", argv[4], "'.", NULL);
    return TCL_ERROR;
  }
  digestlen = EVP_MD_size(digest);
  if (!PKCS5_PBKDF2_HMAC(argv[1], strlen(argv[1]), (const unsigned char *) argv[2], strlen(argv[2]), rounds, digest, digestlen, buf)) {
    Tcl_AppendResult(irp, "PBKDF2 key derivation error: ", ERR_error_string(ERR_get_error(), NULL), ".", NULL);
    return TCL_ERROR;
  }
  buf[digestlen] = 0;
  Tcl_Obj *result = Tcl_NewByteArrayObj(buf, digestlen);
  explicit_bzero(buf, digestlen);
  Tcl_SetObjResult(irp, result);
  return TCL_OK;
}

static tcl_cmds my_tcl_cmds[] = {
  {"encpass2", tcl_encpass2},
  {"pbkdf2",   tcl_pbkdf2},
  {NULL,       NULL}
};
