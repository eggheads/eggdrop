/*
 * python.c -- part of twitch.mod
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
// HACK, but stable API
#undef interp
#define tclinterp (*(Tcl_Interp **)(global[128]))
#undef days
#include <stdlib.h>
#include <Python.h>
#include <datetime.h>
#include "src/mod/irc.mod/irc.h"
#include "src/mod/server.mod/server.h"
#include "src/mod/python.mod/python.h"

//static PyObject *pymodobj;
static PyObject *pirp, *pglobals;

#undef global
static Function *global = NULL, *irc_funcs = NULL;
#include "src/mod/python.mod/pycmds.c"
#include "src/mod/python.mod/tclpython.c"

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
  PyDateTime_IMPORT;
  pmodule = PyImport_ImportModule("eggdrop");
  if (!pmodule) {
    PyErr_Print();
    fprintf(stderr, "Error: could not import module 'eggdrop'\n");
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

static char *python_close()
{
  Context;
  kill_python();
  rem_builtins(H_dcc, mydcc);
  rem_tcl_commands(my_tcl_cmds);
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
  add_builtins(H_dcc, mydcc);
  add_tcl_commands(my_tcl_cmds);
  return NULL;
}
