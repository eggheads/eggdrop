/*
 * twitch.c -- part of twitch.mod
 *   will cease to work. Buyer beware.
 *
 * Originally written by Geo              April 2020
 */

/*
 * Copyright (C) 2020 - 2021 Eggheads Development Team
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

#define MODULE_NAME "python"
#define MAKING_PYTHON

#define ARRAYCOUNT(x) (sizeof (x) / sizeof *(x))

#include "src/mod/module.h"
#undef interp
#include <stdlib.h>
#include <Python.h>
#include "src/mod/irc.mod/irc.h"
#include "src/mod/server.mod/server.h"
#include "src/mod/python.mod/python.h"

#undef global
static Function *global = NULL, *irc_funcs = NULL;
#include "src/mod/python.mod/pycmds.c"

//static PyObject *pymodobj;
static PyObject *pirp, *pglobals;

//extern p_tcl_bind_list H_pubm;

int runPythonArgs(const char *script, const char *method, int argc, char **argv);
int runPythonPyArgs(const char *script, const char *method, PyObject *pArgs);


/* Calculate the memory we keep allocated.
 */
static int python_expmem()
{
  int size = 0;

  Context;
  return size;
}

int py_pubm (char *nick, char *host, char *hand, char *chan, char *text) {
  PyObject *pArgs;
  char buf[UHOSTLEN];
  struct chanset_t *ch;
  struct userrec *u;
  struct flag_record fr = { FR_CHAN | FR_ANYWH | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0 };

  if (!(ch = findchan_by_dname(chan))) {
    putlog(LOG_MISC, "*", "Python: Cannot find pubm channel %s", chan);
    return 1;
  }
  snprintf(buf, sizeof buf, "%s!%s", nick, host);
  if ((u = get_user_by_host(buf))) {
    get_user_flagrec(u, &fr, chan);
  }
  if (!(pArgs = Py_BuildValue("(skkksssss)", "pubm", (unsigned long)fr.global, (unsigned long)fr.chan, (unsigned long)fr.bot, nick, host, hand, chan, text))) {
    putlog(LOG_MISC, "*", "Python: Cannot convert pubm arguments");
    return 1;
  }
  return runPythonPyArgs("eggdroppy.binds", "on_event", pArgs);
}

int py_join (char *nick, char *host, char *hand, char *chan) {
  PyObject *pArgs;
  char buf[UHOSTLEN];
  struct chanset_t *ch;
  struct userrec *u;
  struct flag_record fr = { FR_CHAN | FR_ANYWH | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0 };

  if (!(ch = findchan_by_dname(chan))) {
    putlog(LOG_MISC, "*", "Python: Cannot find pubm channel %s", chan);
    return 1;
  }
  snprintf(buf, sizeof buf, "%s!%s", nick, host);
  if ((u = get_user_by_host(buf))) {
    get_user_flagrec(u, &fr, chan);
  }
  if (!(pArgs = Py_BuildValue("(skkkssss)", "join", (unsigned long)fr.global, (unsigned long)fr.chan, (unsigned long)fr.bot, nick, host, hand, chan))) {
    putlog(LOG_MISC, "*", "Python: Cannot convert join arguments");
    return 1;
  }
  return runPythonPyArgs("eggdroppy.binds", "on_event", pArgs);
}

int py_msgm (char *nick, char *hand, char *host, char *text) {
//  struct flag_record fr = { FR_CHAN | FR_ANYWH | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0 };
  char *argv[] = {"msgm", nick, host, hand, text};

  runPythonArgs("eggdroppy.binds", "on_event", ARRAYCOUNT(argv), argv);
  return 0;
}

static void init_python() {
  PyObject *pmodule;
  wchar_t *program = Py_DecodeLocale("eggdrop", NULL);

  if (program == NULL) {
    fprintf(stderr, "Python: Fatal error: This should never happen\n");
    fatal(1);
  }
  Py_SetProgramName(program);  /* optional but recommended */
  if (PyImport_AppendInittab("eggdrop", &PyInit_eggdrop) == -1) {
    fprintf(stderr, "Error: could not extend in-built modules table\n");
    exit(1);
  }
  Py_Initialize();
  pmodule = PyImport_ImportModule("eggdrop");
  if (!pmodule) {
    PyErr_Print();
    fprintf(stderr, "Error: could not import module 'spam'\n");
  }

  pirp = PyImport_AddModule("__main__");
  pglobals = PyModule_GetDict(pirp);

  PyRun_SimpleString("import sys");
  PyRun_SimpleString("sys.path.append(\".\")");

  return;
}

static void kill_python() {
  if (Py_FinalizeEx() < 0) {
    exit(120);
  }
  PyMem_RawFree(Py_DecodeLocale("eggdrop", NULL));
  return;
}

int runPythonPyArgs(const char *script, const char *method, PyObject *pArgs)
{
  PyObject *pName, *pModule, *pFunc, *pValue;

  pName = PyUnicode_DecodeFSDefault(script);
  /* Error checking of pName left out */
  pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  if (pModule != NULL) {
    pFunc = PyObject_GetAttrString(pModule, method);
    /* pFunc is a new reference */
    if (pFunc && PyCallable_Check(pFunc)) {
      pValue = PyObject_CallObject(pFunc, pArgs);
      Py_DECREF(pArgs);
      if (pValue != NULL) {
        printf("Result of call: %ld\n", PyLong_AsLong(pValue));
        Py_DECREF(pValue);
      }
      else {
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        PyErr_Print();
        fprintf(stderr,"Call failed\n");
        return 1;
      }
    }
    else {
      if (PyErr_Occurred())
        PyErr_Print();
      fprintf(stderr, "Cannot find function \"%s\"\n", method);
    }
    Py_XDECREF(pFunc);
    Py_DECREF(pModule);
  }
  else {
    PyErr_Print();
    fprintf(stderr, "Failed to load \"%s\"\n", script);
    return 1;
  }
  return 0;
}

int runPythonArgs(const char *script, const char *method, int argc, char **argv)
{
  PyObject *pArgs, *pValue;

  pArgs = PyTuple_New(argc);
  for (int i = 0; i < argc; i++) {
    pValue = Py_BuildValue("s", argv[i]);
    if (!pValue) {
      Py_DECREF(pArgs);
      fprintf(stderr, "Cannot convert argument at position %d: '%s'\n", i, argv[i]);
      return 1;
    }
    /* pValue reference stolen here: */
    PyTuple_SetItem(pArgs, i, pValue);
  }
  return runPythonPyArgs(script, method, pArgs);
}

static void runPython(char *par) {
  int maxargc = 16;
  char **argv = NULL;
  char *script = NULL, *method = NULL;
  int argvidx = -2;
  char *word;

  if (!argv) {
    argv = nmalloc(maxargc * sizeof *argv);
  }
  for (word = strtok(par, " "); word; word = strtok(NULL, " ")) {
    if (argvidx == -2) {
      script = word;
    } else if (argvidx == -1) {
      method = word;
    } else {
      if (argvidx >= maxargc) {
        maxargc += 16;
        argv = nrealloc(argv, maxargc * sizeof *argv);
      }
      argv[argvidx] = word;
    }
    argvidx++;
  }
  if (argvidx < 0) {
    fprintf(stderr, "Missing script/method for runPython\n");
    return;
  }
  runPythonArgs(script, method, argvidx, argv);
}

static void cmd_python(struct userrec *u, int idx, char *par) {
  PyErr_Clear();
  PyRun_String(par, Py_file_input, pglobals, pglobals);
  if (PyErr_Occurred()) {
    PyErr_Print();
  }
  return;
}

static void cmd_pyexpr(struct userrec *u, int idx, char *par) {
//  char *result;
  PyObject *pobj, *pstr;

  PyErr_Clear();

  pobj = PyRun_String(par, Py_eval_input, pglobals, pglobals);
  if (pobj) {
    pstr = PyObject_Str(pobj);

    dprintf(idx, "%s\n", PyUnicode_AsUTF8(pstr));
    Py_DECREF(pstr);
    Py_DECREF(pobj);
  } else if (PyErr_Occurred()) {
    PyErr_Print();
  }
  return;
}

static void cmd_pysource(struct userrec *u, int idx, char *par) {
  if (!par[0]) {
    dprintf(idx, "Usage: pysource <file> <method> [args]\n");
    return;
  }
  runPython(par);

  return;
}

static void cmd_pybinds(struct userrec *u, int idx, char *par) {
  runPython("eggdroppy.binds print_all");
  return;
}

/* A report on the module status.
 *
 * details is either 0 or 1:
 *    0 - `.status'
 *    1 - `.status all'  or  `.module twitch'
 */
static void python_report(int idx, int details)
{
  if (details) {
    int size = python_expmem();

    dprintf(idx, "    Using %d byte%s of memory\n", size,
            (size != 1) ? "s" : "");
  }
}

static cmd_t mydcc[] = {
  /* command  flags  function     tcl-name */
  {"pysource",  "",     (IntFunc) cmd_pysource, NULL},
  {"python",    "",     (IntFunc) cmd_python,   NULL},
  {"pyexpr",    "",     (IntFunc) cmd_pyexpr,   NULL},
  {"pybinds",   "",     (IntFunc) cmd_pybinds,  NULL},
  {NULL,        NULL,   NULL,                   NULL}  /* Mark end. */
};

static cmd_t mypy_pubm[] = {
    {"*",   "",     py_pubm, "python:pubm"},
    {NULL,  NULL,   NULL,     NULL}
};

static cmd_t mypy_join[] = {
    {"*",   "",     py_join, "python:join"},
    {NULL,  NULL,   NULL,   NULL}
};

/*static tcl_cmds mytcl[] = {
  {"twcmd",           tcl_twcmd},
  {"roomstate",   tcl_roomstate},
  {"userstate",   tcl_userstate},
  {"twitchmods", tcl_twitchmods},
  {"twitchvips", tcl_twitchvips},
  {"ismod",           tcl_ismod},
  {"isvip",           tcl_isvip},
  {NULL,                   NULL}
}; */

/*
static tcl_ints my_tcl_ints[] = {
  {"keep-nick",         &keepnick,        STR_PROTECT},
  {NULL,                NULL,                       0}
}; */

/*
static tcl_strings my_tcl_strings[] = {
  {"cap-request",   cap_request,    55,     STR_PROTECT},
  {NULL,            NULL,           0,                0}
}; */

static char *python_close()
{
  Context;
  kill_python();
  rem_builtins(H_dcc, mydcc);
  rem_builtins(H_pubm, mypy_pubm);
//  rem_tcl_commands(mytcl);
//  rem_tcl_ints(my_tcl_ints);
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *python_start();

static Function python_table[] = {
  (Function) python_start,
  (Function) python_close,
  (Function) python_expmem,
  (Function) python_report
};

char *python_start(Function *global_funcs)
{
  /* Assign the core function table. After this point you use all normal
   * functions defined in src/mod/modules.h
   */
  global = global_funcs;

  Context;
  /* Register the module. */
  module_register(MODULE_NAME, python_table, 0, 1);

  if (!module_depend(MODULE_NAME, "eggdrop", 109, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.9.0 or later.";
  }
  if (!(irc_funcs = module_depend(MODULE_NAME, "irc", 1, 5))) {
    module_undepend(MODULE_NAME);
    return "This module requires irc module 1.5 or later.";
  }
/*
  if (!(server_funcs = module_depend(MODULE_NAME, "channels", 1, 1))) {
    module_undepend(MODULE_NAME);
    return "This module requires channels module 1.1 or later.";
  }
*/

  init_python();

  /* Add command table to bind list */
  add_builtins(H_pubm, mypy_pubm);
  add_builtins(H_join, mypy_join);
  add_builtins(H_dcc, mydcc);
//  add_tcl_commands(mytcl);
//  add_tcl_ints(my_tcl_ints);
//  add_tcl_strings(my_tcl_strings);
  return NULL;
}
