/*
 * python.c -- python interpreter handling for python.mod
 */

/*
 * Copyright (C) 2020 - 2024 Eggheads Development Team
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
#define PY_SSIZE_T_CLEAN /* Not required for 3.13+ but here for back compat */

#define ARRAYCOUNT(x) (sizeof (x) / sizeof *(x))

#include "src/mod/module.h"
// HACK, but stable API
#undef interp
#define tclinterp (*(Tcl_Interp **)(global[128]))
#undef days
#include <Python.h>
#include <datetime.h>
#include "src/mod/server.mod/server.h"
#include "python.h"

//static PyObject *pymodobj;
static PyObject *pirp, *pglobals;

#undef global
static Function *global = NULL;
static PyThreadState *_pythreadsave;
#include "pycmds.c"
#include "tclpython.c"

EXPORT_SCOPE char *python_start(Function *global_funcs);

static int python_expmem()
{
  return 0; // TODO
}

static int python_gil_unlock() {
  _pythreadsave = PyEval_SaveThread();
  return 0;
}

static int python_gil_lock() {
  PyEval_RestoreThread(_pythreadsave);
  return 0;
}

static char *init_python() {
  const char *venv;
  char venvpython[PATH_MAX];
  PyObject *pmodule;
  PyStatus status;
  PyConfig config;

  PyConfig_InitPythonConfig(&config);
  config.install_signal_handlers = 0;
  config.parse_argv = 0;
  if ((venv = getenv("VIRTUAL_ENV"))) {
    snprintf(venvpython, sizeof venvpython, "%s/bin/python3", venv);
    status = PyConfig_SetBytesString(&config, &config.executable, venvpython);
    if (PyStatus_Exception(status)) {
      PyConfig_Clear(&config);
      return "Python: Fatal error: Could not set venv executable";
    }
  }
  status = PyConfig_SetBytesString(&config, &config.program_name, argv0);
  if (PyStatus_Exception(status)) {
    PyConfig_Clear(&config);
    return "Python: Fatal error: Could not set program base path";
  }
  if (PyImport_AppendInittab("eggdrop", &PyInit_eggdrop) == -1) {
    PyConfig_Clear(&config);
    return "Python: Error: could not extend in-built modules table";
  }
  status = Py_InitializeFromConfig(&config);
  if (PyStatus_Exception(status)) {
    PyConfig_Clear(&config);
    return "Python: Fatal error: Could not initialize config";
  }
  PyConfig_Clear(&config);
  PyDateTime_IMPORT;
  pmodule = PyImport_ImportModule("eggdrop");
  if (!pmodule) {
    return "Error: could not import module 'eggdrop'";
  }

  pirp = PyImport_AddModule("__main__");
  pglobals = PyModule_GetDict(pirp);

  PyRun_SimpleString("import sys");
  // TODO: Relies on pwd() staying eggdrop main dir
  PyRun_SimpleString("sys.path.append(\".\")");
  PyRun_SimpleString("import eggdrop");
  PyRun_SimpleString("sys.displayhook = eggdrop.__displayhook__");

  return NULL;
}

static void python_report(int idx, int details)
{
  if (details)
    dprintf(idx, "    python version: %s (header version " PY_VERSION ")\n", Py_GetVersion());
}

static char *python_close()
{
  /* Forbid unloading, because:
   * - Reloading (Reexecuting PyDateTime_IMPORT) would crash
   * - Py_FinalizeEx() does not clean up everything
   * - Complexity regarding running python threads
   * see https://bugs.python.org/issue34309 for details
   */
  return "The " MODULE_NAME " module is not allowed to be unloaded.";
}

static Function python_table[] = {
  (Function) python_start,
  (Function) python_close,
  (Function) python_expmem,
  (Function) python_report
};

char *python_start(Function *global_funcs)
{
  char *s;

  /* Assign the core function table. After this point you use all normal
   * functions defined in src/mod/modules.h
   */
  if (global_funcs) {
    global = global_funcs;

    /* Register the module. */
    module_register(MODULE_NAME, python_table, 0, 1);
    if (!module_depend(MODULE_NAME, "eggdrop", 109, 0)) {
      module_undepend(MODULE_NAME);
      return "This module requires Eggdrop 1.9.0 or later.";
    }
    if ((s = init_python()))
      return s;
  }

  /* Add command table to bind list */
  add_builtins(H_dcc, mydcc);
  add_tcl_commands(my_tcl_cmds);
  add_hook(HOOK_PRE_SELECT, (Function)python_gil_unlock);
  add_hook(HOOK_POST_SELECT, (Function)python_gil_lock);
  return NULL;
}
