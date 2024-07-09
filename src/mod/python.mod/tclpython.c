/*
 * tclpython.c -- tcl functions for python.mod
 */
/*
 * Copyright (C) 2000 - 2024 Eggheads Development Team
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
  FILE *fp;
  PyObject *pobj, *ptype, *pvalue, *ptraceback;
  PyObject *pystr;
  Py_ssize_t n;
  const char *res = NULL;
  int i;

  BADARGS(2, 2, " script");

  if (!(fp = fopen(argv[1], "r"))) {
    Tcl_AppendResult(irp, "Error: could not open file ", argv[1],": ", strerror(errno), NULL);
    return TCL_ERROR;
  }
  PyErr_Clear();
  // Always PyNone or NULL on exception
  pobj = PyRun_FileEx(fp, argv[1], Py_file_input, pglobals, pglobals, 1);
  Py_XDECREF(pobj);

  if (PyErr_Occurred()) {
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    pystr = PyObject_Str(pvalue);
    Tcl_AppendResult(irp, "Error loading python: ", PyUnicode_AsUTF8(pystr), NULL);
    Py_DECREF(pystr);
    // top level syntax errors do not have a traceback
    if (ptraceback) {
      PyObject *module_name, *pymodule, *pyfunc, *pyval, *item;

      module_name = PyUnicode_FromString("traceback");
      pymodule = PyImport_Import(module_name);
      Py_DECREF(module_name);
      // format backtrace and print
      pyfunc = PyObject_GetAttrString(pymodule, "format_exception");

      if (pyfunc && PyCallable_Check(pyfunc)) {
        pyval = PyObject_CallFunctionObjArgs(pyfunc, ptype, pvalue, ptraceback, NULL);

        if (pyval && PyList_Check(pyval)) {
          n = PyList_Size(pyval);
          for (i = 0; i < n; i++) {
            item = PyList_GetItem(pyval, i);
            pystr = PyObject_Str(item);
            res = PyUnicode_AsUTF8(pystr);
            // strip \n
            putlog(LOG_MISC, "*", "%.*s", (int)(strlen(res) - 1), res);
            Py_DECREF(pystr);
          }
        } else {
          putlog(LOG_MISC, "*", "Error fetching python traceback");
        }
        Py_XDECREF(pyval);
      }
      Py_XDECREF(pyfunc);
      Py_DECREF(pymodule);
    }
    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);
    return TCL_ERROR;
  }
  return TCL_OK;
}

static tcl_cmds my_tcl_cmds[] = {
  {"pysource",  tcl_pysource},
  {NULL,        NULL}
};
