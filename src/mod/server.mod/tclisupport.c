/*
 * isupport.c -- part of server.mod
 *   handles raw 005 (isupport)
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

#include <ctype.h>

static int tcl_isupport STDOBJVAR;
static int tcl_isupport_get STDOBJVAR;
static int tcl_isupport_isset STDOBJVAR;
static int tcl_isupport_set STDOBJVAR;
static int tcl_isupport_unset STDOBJVAR;

/* settypes are implicitly also gettypes */
static const char *settypes[] = {"forced", "ignored", "default"}, *gettypes[] = {"server", "current"};

static struct {
  const char *subcmd;
  Tcl_ObjCmdProc *proc;
} subcmds[] = {
  {"get", tcl_isupport_get},
  {"isset", tcl_isupport_isset},
  {"set", tcl_isupport_set},
  {"unset", tcl_isupport_unset},
};

static size_t tcl_isupport_subcmds_len(void) {
  size_t i, len = 0;
  for (i = 0; i < ARRAY_SIZE(subcmds); i++)
    len += strlen(subcmds[i].subcmd) + 1;
  return len;
}

static char *tcl_isupport_subcmds(void) {
  static char *str;
  if (!str) {
    char *s;
    int i;
    str = nmalloc(tcl_isupport_subcmds_len());
    s = str;
    for (i = 0; i < ARRAY_SIZE(subcmds); i++) {
      s += sprintf(s, "%s", subcmds[i].subcmd);
      *s++ = ' ';
    }
    *s = '\0';
  }
  return str;
}

static size_t tcl_isupport_syntaxstring_len(const char **words, size_t count) {
  size_t i, len = 0;
  for (i = 0; i < count; i++)
    len += strlen(words[i]) + 1;
  return len;
}

static char *tcl_isupport_get_syntaxstring(char **dst, const char **words, size_t count) {
  if (!*dst) {
    char *s;
    int i;
    *dst = nmalloc(tcl_isupport_syntaxstring_len(words, count));
    s = *dst;
    for (i = 0; i < count; i++) {
      s += sprintf(s, "%s", words[i]);
      *s++ = ' ';
    }
    *s = '\0';
  }
  return *dst;
}

static const char *tcl_isupport_settypes(void) {
  static char *str;
  return tcl_isupport_get_syntaxstring(&str, settypes, ARRAY_SIZE(settypes));
}

static const char *tcl_isupport_gettypes(void) {
  static char *str;
  return tcl_isupport_get_syntaxstring(&str, gettypes, ARRAY_SIZE(gettypes));
}

static int tcl_isupport_validtype(const char *type, int set) {
  int i;
  for (i = 0; i < ARRAY_SIZE(settypes); i++)
    if (!strcmp(type, settypes[i]))
      return 1;
  if (set)
    return 0;
  for (i = 0; i < ARRAY_SIZE(gettypes); i++)
    if (!strcmp(type, gettypes[i]))
      return 1;
  return 0;
}

static int tcl_isupport STDOBJVAR
{
  int i;
  const char *subcmd;
  Tcl_Obj *str;

  BADOBJARGS(2, -1, 1, "subcommand ?args?");

  subcmd = Tcl_GetString(objv[1]);
  for (i = 0; i < sizeof subcmds / sizeof *subcmds; i++)
    if (!strcmp(subcmds[i].subcmd, subcmd))
      return (subcmds[i].proc)(cd, irp, objc, objv);

  str = Tcl_NewStringObj("", 0);
  Tcl_AppendStringsToObj(str, "Invalid subcommand, must be one of: ", tcl_isupport_subcmds(), NULL);
  Tcl_SetObjResult(interp, str);
  return TCL_ERROR;
}

/* Caller must DecrRefCount on the result. */
static Tcl_Obj *tcl_isupport_type_list(Tcl_Interp *irp, const char *type) {
  struct isupportkeydata *e;
  struct isupport *data;
  const char *value;
  Tcl_Obj *result = Tcl_NewListObj(0, NULL);

  Tcl_IncrRefCount(result);
  LIST_FOREACH_DATA(isupportdata, e, data) {
    value = isupport_get_type_from(data, type);
    if (value) {
      Tcl_ListObjAppendElement(irp, result, Tcl_NewStringObj(data->key, -1));
      Tcl_ListObjAppendElement(irp, result, Tcl_NewStringObj(value, -1));
    }
  }
  return result;
}

#define TCL_ERR_NOTSET(irp, dstobj, keyobj, typeobj) do {       \
  (dstobj) = Tcl_NewStringObj("key \"", -1);                    \
  Tcl_AppendObjToObj((dstobj), (keyobj));                       \
  Tcl_AppendToObj((dstobj), "\" is not set for type \"", -1);   \
  Tcl_AppendObjToObj((dstobj), (typeobj));                      \
  Tcl_AppendToObj((dstobj), "\"", -1);                          \
  Tcl_SetObjResult((irp), (dstobj));                            \
  return TCL_ERROR;                                             \
} while (0)

#define ENSURE_VALIDTYPE(irp, dstobj, typeobj, set) do {                       \
  if (!tcl_isupport_validtype(Tcl_GetString((typeobj)), (set))) {              \
    (dstobj) = Tcl_NewStringObj("type \"", -1);                                \
    Tcl_AppendObjToObj((dstobj), (typeobj));                                   \
    Tcl_AppendToObj((dstobj), "\" is not a valid type, must be one of: ", -1); \
    Tcl_AppendToObj((dstobj), tcl_isupport_settypes(), -1);                    \
    if (!(set))                                                                \
      Tcl_AppendStringsToObj((dstobj), " ", tcl_isupport_gettypes(), NULL);    \
    Tcl_SetObjResult((irp), (dstobj));                                         \
    return TCL_ERROR;                                                          \
  }                                                                            \
} while (0)

static int tcl_isupport_get STDOBJVAR
{
  Tcl_Obj *tclres;

  BADOBJARGS(2, 4, 2, "?type? ?setting?");
  if (objc == 2) {
    int i;
    tclres = Tcl_NewListObj(0, NULL);
    for (i = 0; i < ARRAY_SIZE(settypes); i++) {
      Tcl_Obj *o = tcl_isupport_type_list(irp, settypes[i]);
      Tcl_ListObjAppendElement(irp, tclres, Tcl_NewStringObj(settypes[i], -1));
      Tcl_ListObjAppendElement(irp, tclres, o);
      Tcl_DecrRefCount(o);
    }
    for (i = 0; i < ARRAY_SIZE(gettypes); i++) {
      Tcl_Obj *o = tcl_isupport_type_list(irp, gettypes[i]);
      Tcl_ListObjAppendElement(irp, tclres, Tcl_NewStringObj(gettypes[i], -1));
      Tcl_ListObjAppendElement(irp, tclres, o);
      Tcl_DecrRefCount(o);
    }
    Tcl_SetObjResult(irp, tclres);
    return TCL_OK;
  }

  /* objc >= 3 */
  ENSURE_VALIDTYPE(irp, tclres, objv[2], 0);

  if (objc == 4) {
    int keylen;
    const char *key = Tcl_GetStringFromObj(objv[3], &keylen);
    struct isupport *data = find_record(key, keylen);
    if (data) {
      const char *result = isupport_get_type_from(data, Tcl_GetString(objv[2]));
      if (result) {
        Tcl_SetObjResult(irp, Tcl_NewStringObj(result, -1));
        return TCL_OK;
      }
      /* fall through, same error msg */
    }
    TCL_ERR_NOTSET(irp, tclres, objv[3], objv[2]);
  }
  /* objc == 3 */
  tclres = tcl_isupport_type_list(irp, Tcl_GetString(objv[2]));
  Tcl_SetObjResult(irp, tclres);
  Tcl_DecrRefCount(tclres);
  return TCL_OK;
}

static int tcl_isupport_set STDOBJVAR
{
  Tcl_Obj *tclres;
  struct isupport *data;
  int keylen;
  const char *key;

  /* First one to check for type validity only */
  BADOBJARGS(3, 5, 2, "type setting value");

  ENSURE_VALIDTYPE(irp, tclres, objv[2], 1);

  if (!strcmp(Tcl_GetString(objv[2]), "ignored"))
    BADOBJARGS(4, 5, 3, "setting ?value?");
  else {
    BADOBJARGS(4, 5, 3, "setting value");
    BADOBJARGS(5, 5, 4, "value");
  }

  key = Tcl_GetStringFromObj(objv[3], &keylen);
  data = get_record(key, keylen);
  if (!data) {
    TCL_ERR_NOTSET(irp, tclres, objv[3], objv[2]);
  }
  if (!strcmp(Tcl_GetString(objv[2]), "ignored")) {
    int truefalse;
    if (objc == 5) {
      int ret;
      ret = Tcl_GetBooleanFromObj(irp, objv[4], &truefalse);
      if (ret != TCL_OK)
        return ret;
    } else {
      truefalse = 1;
    }
    isupport_set_ignored_into(data, truefalse);
    Tcl_SetObjResult(irp, Tcl_NewBooleanObj(truefalse));
  } else {
    isupport_set_type_into(data, Tcl_GetString(objv[2]), Tcl_GetString(objv[4]));
    Tcl_SetObjResult(irp, objv[4]);
  }
  return TCL_OK;
}

static int tcl_isupport_unset STDOBJVAR
{
  Tcl_Obj *tclres;
  struct isupport *data;
  int keylen;
  const char *key;

  BADOBJARGS(4, 4, 2, "type setting");

  ENSURE_VALIDTYPE(irp, tclres, objv[2], 1);

  key = Tcl_GetStringFromObj(objv[3], &keylen);
  data = find_record(key, keylen);

  if (!data)
    TCL_ERR_NOTSET(irp, tclres, objv[3], objv[2]);

  isupport_unset_type_into(data, Tcl_GetString(objv[2]));
  Tcl_ResetResult(irp);
  return TCL_OK;
}

static int tcl_isupport_isset STDOBJVAR
{
  Tcl_Obj *tclres;
  struct isupport *data;
  int isset, keylen;
  const char *key;

  BADOBJARGS(4, 4, 2, "type setting");

  ENSURE_VALIDTYPE(irp, tclres, objv[2], 1);

  key = Tcl_GetStringFromObj(objv[3], &keylen);
  data = find_record(key, keylen);
  isset = (data && isupport_get_type_from(data, Tcl_GetString(objv[2])));
  Tcl_SetObjResult(irp, Tcl_NewBooleanObj(isset));
  return TCL_OK;
}

