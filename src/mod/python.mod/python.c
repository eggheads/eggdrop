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

EXPORT_SCOPE char *python_start(Function *global_funcs);

static int python_expmem()
{
  return 0; // TODO
}

// TODO: Do we really have to exit eggdrop on module load failure?
static void init_python() {
  PyObject *pmodule;
  PyStatus status;
  PyConfig config;

  if (PY_VERSION_HEX < 0x0308) {
    putlog(LOG_MISC, "*", "Python: Python version %d is lower than 3.8, not loading Python module", PY_VERSION_HEX);
    return;
  }
  PyConfig_InitIsolatedConfig(&config);
  config.install_signal_handlers = 0;
  config.parse_argv = 0;
  status = PyConfig_SetBytesString(&config, &config.program_name, argv0);
  if (PyStatus_Exception(status)) {
    PyConfig_Clear(&config);
    putlog(LOG_MISC, "*", "Python: Fatal error: Could not set program base path");
    Py_ExitStatusException(status);
  }
  if (PyImport_AppendInittab("eggdrop", &PyInit_eggdrop) == -1) {
    putlog(LOG_MISC, "*", "Python: Error: could not extend in-built modules table");
    exit(1);
  }
  status = Py_InitializeFromConfig(&config);
  if (PyStatus_Exception(status)) {
    PyConfig_Clear(&config);
    putlog(LOG_MISC, "*", "Python: Fatal error: Could not initialize config");
    fatal(1);
  }
  PyConfig_Clear(&config);
  PyDateTime_IMPORT;
  pmodule = PyImport_ImportModule("eggdrop");
  if (!pmodule) {
    PyErr_Print();
    putlog(LOG_MISC, "*", "Error: could not import module 'eggdrop'");
    fatal(1);
  }

  pirp = PyImport_AddModule("__main__");
  pglobals = PyModule_GetDict(pirp);

  PyRun_SimpleString("import sys");
  // TODO: Relies on pwd() staying eggdrop main dir
  PyRun_SimpleString("sys.path.append(\".\")");
  PyRun_SimpleString("import eggdrop");
  PyRun_SimpleString("sys.displayhook = eggdrop.__displayhook__");

  return;
}

static void kill_python() {
  if (Py_FinalizeEx() < 0) {
    exit(120);
  }
  return;
}

static void python_report(int idx, int details)
{
  // TODO
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
  // TODO: Is this dependency necessary? It auto-loads irc.mod, that might be undesired
  if (!(irc_funcs = module_depend(MODULE_NAME, "irc", 1, 5))) {
    module_undepend(MODULE_NAME);
    return "This module requires irc module 1.5 or later.";
  }
  // irc.mod depends on server.mod and channels.mod, so those were implicitely loaded

  init_python();

  /* Add command table to bind list */
  add_builtins(H_dcc, mydcc);
  add_tcl_commands(my_tcl_cmds);
  return NULL;
}