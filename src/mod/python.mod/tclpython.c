/*
 * tclpython.c -- part of python.mod
 *   contains all tcl functions
 *
 */
/*
 * Copyright (C) 2000 - 2023 Eggheads Development Team
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

static int tcl_pysource STDVAR
{
  BADARGS(2, 2, " script");

  FILE *fp;
  PyObject *pobj, *pstr, *ptype, *pvalue, *ptraceback;
  PyObject *pystr, *module_name, *pymodule, *pyfunc, *pyval, *item;
  Py_ssize_t n;
  const char *res = NULL;
  char *res2;
  int i;

  if (!(fp = fopen(argv[1], "r"))) {
    Tcl_AppendResult(irp, "Error: could not open file ", argv[1],": ", strerror(errno), NULL);
    return TCL_ERROR;
  }
  PyErr_Clear();
  pobj = PyRun_FileEx(fp, argv[1], Py_file_input, pglobals, pglobals, 1);
  if (pobj) {
    pstr = PyObject_Str(pobj);
    Tcl_AppendResult(irp, PyUnicode_AsUTF8(pstr), NULL);
    Py_DECREF(pstr);
    Py_DECREF(pobj);
  } else if (PyErr_Occurred()) {
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    pystr = PyObject_Str(pvalue);
    Tcl_AppendResult(irp, "Error loading python: ", PyUnicode_AsUTF8(pystr), NULL);
    module_name = PyUnicode_FromString("traceback");
    pymodule = PyImport_Import(module_name);
    Py_DECREF(module_name);
    // format backtrace and print
    pyfunc = PyObject_GetAttrString(pymodule, "format_exception");
    if (pyfunc && PyCallable_Check(pyfunc)) {
      pyval = PyObject_CallFunctionObjArgs(pyfunc, ptype, pvalue, ptraceback, NULL);
      // Check if traceback is a list and handle as such
      if (PyList_Check(pyval)) {
        n = PyList_Size(pyval);
        for (i = 0; i < n; i++) {
          item = PyList_GetItem(pyval, i);
          pystr = PyObject_Str(item);
          //Python returns a const char but we need to remove the \n
          res = PyUnicode_AsUTF8(pystr);
          if (res[strlen(res) - 1]) {
            res2 = (char*) nmalloc(strlen(res)+1);
            strncpy(res2, res, strlen(res));
            res2[strlen(res) - 1] = '\0';
          }
          putlog(LOG_MISC, "*", "%s", res);
          if (res2) {
            nfree(res2);
          }
        }
      } else {
        pystr = PyObject_Str(pyval);
        Tcl_AppendResult(irp, PyUnicode_AsUTF8(pystr), NULL);
      }
      Py_DECREF(pyval);
    }
    return TCL_ERROR;
  }
  return TCL_OK;
}

static tcl_cmds my_tcl_cmds[] = {
  {"pysource",  tcl_pysource},
  {NULL,        NULL}
};
