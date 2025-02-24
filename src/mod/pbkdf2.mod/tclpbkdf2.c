/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * tclpbkdf2.c -- tcl functions for pbkdf2.mod
 *
 * Written by thommey and Michael Ortmann
 *
 * Copyright (C) 2017 - 2024 Eggheads Development Team
 */

#include <openssl/err.h>
#include <string.h>

static char *pbkdf2_encrypt(const char *);

static int tcl_encpass2 STDVAR
{
  BADARGS(2, 2, " string");
  Tcl_SetResult(irp, pbkdf2_encrypt(argv[1]), TCL_STATIC);
  return TCL_OK;
}

static int tcl_pbkdf2 STDVAR
{
  int hex, digestlen, i;
  unsigned int rounds;
  const EVP_MD *digest;
  unsigned char buf[256];
  char buf_hex[256];
  Tcl_Obj *result = 0;

  BADARGS(5, 6, " ?-bin? pass salt rounds digest");
  if (argc == 6) {
    if (!strcmp(argv[1], "-bin"))
      hex = 0;
    else {
      Tcl_AppendResult(irp, "bad option ", argv[1], ": must be -bin", NULL);
      return TCL_ERROR;
    }
  }
  else
    hex = 1;
  rounds = atoi(argv[3 + !hex]);
  digest = EVP_get_digestbyname(argv[4 + !hex]);
  if (!digest) {
    Tcl_AppendResult(irp, "PBKDF2 error: Unknown message digest '", argv[4 + !hex], "'.", NULL);
    return TCL_ERROR;
  }
  digestlen = EVP_MD_size(digest);
  if (!PKCS5_PBKDF2_HMAC(argv[1 + !hex], strlen(argv[1 + !hex]), (const unsigned char *) argv[2+ !hex], strlen(argv[2 + !hex]), rounds, digest, digestlen, buf)) {
    Tcl_AppendResult(irp, "PBKDF2 key derivation error: ", ERR_error_string(ERR_get_error(), NULL), ".", NULL);
    return TCL_ERROR;
  }
  if (hex) {
    for (i = 0; i < digestlen; i++)
      sprintf(buf_hex + (i * 2), "%.2X", buf[i]);
    result = Tcl_NewByteArrayObj((unsigned char *) buf_hex, digestlen * 2);
    explicit_bzero(buf_hex, digestlen * 2);
  }
  else
    result = Tcl_NewByteArrayObj(buf, digestlen);
  explicit_bzero(buf, digestlen);
  Tcl_SetObjResult(irp, result);
  return TCL_OK;
}

static tcl_cmds my_tcl_cmds[] = {
  {"encpass2", tcl_encpass2},
  {"pbkdf2",   tcl_pbkdf2},
  {NULL,       NULL}
};
