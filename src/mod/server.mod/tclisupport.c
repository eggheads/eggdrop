/*
 * isupport.c -- part of server.mod
 *   handles raw 005 (isupport)
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2021 Eggheads Development Team
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

#include <ctype.h>

int tcl_isupport STDOBJVAR;
static int tcl_isupport_get STDOBJVAR;
static int tcl_isupport_isset STDOBJVAR;

static struct {
  const char *subcmd;
  Tcl_ObjCmdProc *proc;
} subcmds[] = {
  {"get", tcl_isupport_get},
  {"isset", tcl_isupport_isset},
};

#define TCL_ERR_NOTSET(irp, keyobj) do {                                 \
  Tcl_Obj *_tclobj = Tcl_NewStringObj("key \"", -1);                     \
  Tcl_AppendObjToObj(_tclobj, (keyobj));                                 \
  Tcl_AppendStringsToObj(_tclobj, "\" is not set", NULL);                \
  Tcl_SetObjResult((irp), _tclobj);                                      \
  return TCL_ERROR;                                                      \
} while (0)

int tcl_isupport STDOBJVAR
{
  int i;
  const char *subcmd;
  Tcl_Obj *str;

  BADOBJARGS(2, -1, 1, "subcommand ?args?");

  subcmd = Tcl_GetString(objv[1]);
  for (i = 0; i < sizeof subcmds / sizeof *subcmds; i++) {
    if (!strcmp(subcmds[i].subcmd, subcmd))
      return (subcmds[i].proc)(cd, irp, objc, objv);
  }

  str = Tcl_NewStringObj("", 0);
  Tcl_AppendStringsToObj(str, "Invalid subcommand, must be one of:", NULL);
  for (i = 0; i < sizeof subcmds / sizeof *subcmds; i++)
    Tcl_AppendStringsToObj(str, " ", subcmds[i].subcmd, NULL);
  Tcl_SetObjResult(interp, str);
  return TCL_ERROR;
}

static int tcl_isupport_get STDOBJVAR
{
  int keylen;
  const char *key, *value;
  Tcl_Obj *tclres;

  BADOBJARGS(2, 3, 2, "?setting?");
  if (objc == 2) {
    tclres = Tcl_NewListObj(0, NULL);
 
   for (struct isupport *data = isupport_list; data; data = data->next) {
      Tcl_ListObjAppendElement(irp, tclres, Tcl_NewStringObj(data->key, -1));
      Tcl_ListObjAppendElement(irp, tclres, Tcl_NewStringObj(isupport_get_from_record(data), -1));
    }
    Tcl_SetObjResult(irp, tclres);
    return TCL_OK;
  }

  /* objc >= 3 */
  key = Tcl_GetStringFromObj(objv[2], &keylen);
  value = isupport_get(key, keylen);
  if (!value) {
    TCL_ERR_NOTSET(irp, objv[2]);
  }
  Tcl_SetObjResult(irp, Tcl_NewStringObj(value, -1));
  return TCL_OK;
}

static int tcl_isupport_isset STDOBJVAR
{
  int keylen;
  const char *key, *value;

  BADOBJARGS(3, 3, 2, "setting");
  key = Tcl_GetStringFromObj(objv[2], &keylen);
  value = isupport_get(key, keylen);
  Tcl_SetResult(interp, value ? "1" : "0", NULL);
  return TCL_OK;
}

#if 0
/* not exposed for now */
static int tcl_isupport_set STDOBJVAR
{
  int keylen, valuelen;
  const char *key, *value;

  /* First one to check for type validity only */
  BADOBJARGS(4, 4, 2, "setting value");

  key = Tcl_GetStringFromObj(objv[2], &keylen);
  value = Tcl_GetStringFromObj(objv[3], &valuelen);
  isupport_set(key, keylen, value, valuelen);

  Tcl_SetObjResult(irp, objv[3]);
  return TCL_OK;
}

static int tcl_isupport_unset STDOBJVAR
{
  struct isupport *data;
  int keylen;
  const char *key;

  BADOBJARGS(3, 3, 2, "setting");

  key = Tcl_GetStringFromObj(objv[2], &keylen);
  data = find_record(key, keylen);

  if (!data) {
    TCL_ERR_NOTSET(irp, objv[2]);
  }
  if (!data->value) {
    Tcl_SetResult(interp, "no server value set, cannot unset default values, change 'set isupport-default' instead", NULL);
    return TCL_ERROR;
  }
  isupport_unset(key, keylen);
  /* data might be invalidated by isupport_unset */
  data = find_record(key, keylen);
  Tcl_SetObjResult(irp, Tcl_NewStringObj(data ? isupport_get_from_record(data) : "", -1));
  return TCL_OK;
}
#endif

char *traced_isupport(ClientData cdata, Tcl_Interp *irp,
                            EGG_CONST char *name1,
                            EGG_CONST char *name2, int flags)
{
  if (flags & (TCL_TRACE_READS | TCL_TRACE_UNSETS)) {
    Tcl_SetVar2(interp, name1, name2, isupport_default, TCL_GLOBAL_ONLY);
    const char *value;
    Tcl_DString ds;
    Tcl_DStringInit(&ds);

    for (struct isupport *data = isupport_list; data; data = data->next) {
      if (data->defaultvalue) {
        value = isupport_encode(data->defaultvalue);
        Tcl_DStringAppend(&ds, data->key, strlen(data->key));
        Tcl_DStringAppend(&ds, "=", 1);
        Tcl_DStringAppend(&ds, value, strlen(value));
        Tcl_DStringAppend(&ds, " ", 1);
      }
    }
    /* remove trailing space */
    if (Tcl_DStringLength(&ds))
      Tcl_DStringTrunc(&ds, Tcl_DStringLength(&ds) - 1);
    Tcl_SetVar2(interp, name1, name2, Tcl_DStringValue(&ds), TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&ds);

    if (flags & TCL_TRACE_UNSETS)
      Tcl_TraceVar(irp, name1, TCL_TRACE_READS | TCL_TRACE_WRITES |
                   TCL_TRACE_UNSETS, traced_isupport, cdata);
  } else {
    EGG_CONST char *cval = Tcl_GetVar2(interp, name1, name2, TCL_GLOBAL_ONLY);
    isupport_clear_values(1);
    isupport_parse(cval, isupport_setdefault);
  }
  return NULL;
}

int isupport_bind STDVAR
{
  Function F = (Function) cd;
  BADARGS(4, 4, " key isset value");
  CHECKVALIDITY(isupport_bind);
  F(argv[1], argv[2], argv[3]);
  return TCL_OK;
}

/* Bind before values are final, so they can be changed by scripts */
int check_tcl_isupport(struct isupport *data, const char *key, const char *value)
{
  Tcl_SetVar(interp, "_isupport1", key, 0);
  Tcl_SetVar(interp, "_isupport2", value ? "1" : "0", 0);
  Tcl_SetVar(interp, "_isupport3", value ? value : "", 0);

  return BIND_EXEC_LOG == check_tcl_bind(H_isupport, key, 0, " $_isupport1 $_isupport2 $_isupport3",
      MATCH_MASK | BIND_STACKABLE | BIND_WANTRET);
}
