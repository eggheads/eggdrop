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

static PyObject *pymodobj;
static PyObject *pirp, *pglobals;

//extern p_tcl_bind_list H_pubm;

int runPythonArgs(const char *script, const char *method, int argc, char **argv);


/* Calculate the memory we keep allocated.
 */
static int python_expmem()
{
  int size = 0;

  Context;
  return size;
}

int py_pubm (char *nick, char *hand, char *host, char *chan, char *text) {
  struct flag_record fr = { FR_CHAN | FR_ANYWH | FR_GLOBAL, 0, 0, 0, 0, 0 };
  char *argv[] = {"pubm", nick, host, hand, chan, text};

  runPythonArgs("binds", "on_event", ARRAYCOUNT(argv), argv);
  return 0;
}

int py_msgm (char *nick, char *hand, char *host, char *text) {
  struct flag_record fr = { FR_CHAN | FR_ANYWH | FR_GLOBAL, 0, 0, 0, 0, 0 };
  char *argv[] = {"msgm", nick, host, hand, text};

  runPythonArgs("binds", "on_event", ARRAYCOUNT(argv), argv);
  return 0;
}


static PyObject* py_numargs(PyObject *self, PyObject *args) {
  int numargs = 2;

  if(!PyArg_ParseTuple(args, "i:numargs", &numargs))
    return NULL;
  return PyLong_FromLong(numargs);
}

static PyObject* py_putserv(PyObject *self, PyObject *args) {
  char *s = 0, *t = 0, *p;

  if(!PyArg_ParseTuple(args, "s|s", &s, &t)) {
    return NULL;
  }

//  BADARGS(2, 3, " text ?options?");

//  if ((argc == 3) && strcasecmp(argv[2], "-next") &&
//      strcasecmp(argv[2], "-normal")) {
//    Tcl_AppendResult(irp, "unknown putserv option: should be one of: ",
//                     "-normal -next", NULL);
//    return TCL_ERROR;
//  }

  p = strchr(s, '\n');
  if (p != NULL)
    *p = 0;
  p = strchr(s, '\r');
  if (p != NULL)
    *p = 0;
//  if (!strcasecmp(t, "-next"))
//    dprintf(DP_SERVER_NEXT, "%s\n", s);
//  else
    dprintf(DP_SERVER, "%s\n", s);
  return Py_None;
}


static PyMethodDef PyMethods[] = {
  {"numargs", py_numargs, METH_VARARGS, "Return the number of arguments received by the process."},
  {"putserv", py_putserv, METH_VARARGS, "text ?options"},
  {NULL, NULL, 0, NULL}
};

static PyModuleDef PyModule = {
  PyModuleDef_HEAD_INIT, "eggdrop", NULL, -1, PyMethods,
  NULL, NULL, NULL, NULL
};

static PyObject* PyInit_py(void) {
  pymodobj = PyModule_Create(&PyModule);

  PyModule_AddIntConstant(pymodobj, "USER_AUTOOP", 0x00000001);
  PyModule_AddIntConstant(pymodobj, "USER_BOT", 0x00000002);
  PyModule_AddIntConstant(pymodobj, "USER_COMMON", 0x00000004);
  PyModule_AddIntConstant(pymodobj, "USER_DEOP", 0x00000008);
  PyModule_AddIntConstant(pymodobj, "USER_EXEMPT", 0x00000010);
  PyModule_AddIntConstant(pymodobj, "USER_FRIEND", 0x00000020);
  PyModule_AddIntConstant(pymodobj, "USER_GVOICE", 0x00000040);
  PyModule_AddIntConstant(pymodobj, "USER_HIGHLITE", 0x00000080);
  PyModule_AddIntConstant(pymodobj, "USER_JANITOR", 0x00000200);
  PyModule_AddIntConstant(pymodobj, "USER_KICK", 0x00000400);
  PyModule_AddIntConstant(pymodobj, "USER_HALFOP", 0x00000800);
  PyModule_AddIntConstant(pymodobj, "USER_MASTER", 0x00001000);
  PyModule_AddIntConstant(pymodobj, "USER_OWNER", 0x00002000);
  PyModule_AddIntConstant(pymodobj, "USER_OP", 0x00004000);
  PyModule_AddIntConstant(pymodobj, "USER_PARTY", 0x00008000);
  PyModule_AddIntConstant(pymodobj, "USER_QUIET", 0x00010000);
  PyModule_AddIntConstant(pymodobj, "USER_DEHALFOP", 0x00020000);
  PyModule_AddIntConstant(pymodobj, "USER_BOTMAST", 0x00080000);
  PyModule_AddIntConstant(pymodobj, "USER_UNSHARED", 0x00100000);
  PyModule_AddIntConstant(pymodobj, "USER_VOICE", 0x00200000);
  PyModule_AddIntConstant(pymodobj, "USER_WASOPTEST", 0x00400000);
  PyModule_AddIntConstant(pymodobj, "USER_XFER", 0x00800000);
  PyModule_AddIntConstant(pymodobj, "USER_AUTOHALFOP", 0x01000000);
  PyModule_AddIntConstant(pymodobj, "USER_WASHALFOPTEST", 0x02000000);

  return pymodobj;
}


static void init_python() {
  wchar_t *program = Py_DecodeLocale("eggdrop", NULL);

  if (program == NULL) {
    fprintf(stderr, "Python: Fatal error: This should never happen\n");
    fatal(1);
  }
  Py_SetProgramName(program);  /* optional but recommended */
  PyImport_AppendInittab("eggdrop", &PyInit_py);
  Py_Initialize();

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
  PyMem_RawFree("eggdrop");
  return;
}

int runPythonArgs(const char *script, const char *method, int argc, char **argv)
{
  PyObject *pName, *pModule, *pFunc;
  PyObject *pArgs, *pValue;
  char *pyv[argc];

  pName = PyUnicode_DecodeFSDefault(script);
  /* Error checking of pName left out */
  pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  for (int i = 0; i < argc; i++) {
    fprintf(stderr, "pyv[%d] is %s\n", i, argv[i]);
  }
  if (pModule != NULL) {
    pFunc = PyObject_GetAttrString(pModule, method);
    /* pFunc is a new reference */
    if (pFunc && PyCallable_Check(pFunc)) {
      pArgs = PyTuple_New(argc);
      for (int i = 0; i < argc; i++) {
          pValue = Py_BuildValue("s", argv[i]);
          if (!pValue) {
            Py_DECREF(pArgs);
            Py_DECREF(pModule);
            fprintf(stderr, "Cannot convert argument at position %d: '%s'\n", i, argv[i]);
            return 1;
          }
          /* pValue reference stolen here: */
          PyTuple_SetItem(pArgs, i, pValue);
      }
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
    fprintf(stderr, "Failed to load \"%s\"\n", pyv[0]);
    return 1;
  }
  if (Py_FinalizeEx() < 0) {
    return 120;
  }
  return 0;
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
  char *result;
  PyObject *pobj, *pstr;

  PyErr_Clear();

  pobj = PyRun_String(par, Py_file_input, pglobals, pglobals);

  if (PyErr_Occurred()) {
    PyErr_Print();
  }
  return;
}

static void cmd_pyexpr(struct userrec *u, int idx, char *par) {
  char *result;
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
  {NULL,        NULL,   NULL,                   NULL}  /* Mark end. */
};

static cmd_t mypy_pubm[] = {
    {"*",   "",     py_pubm, "python:pubm"},
    {NULL,  NULL,   NULL,     NULL}
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
  add_builtins(H_dcc, mydcc);
//  add_tcl_commands(mytcl);
//  add_tcl_ints(my_tcl_ints);
//  add_tcl_strings(my_tcl_strings);
  return NULL;
}
