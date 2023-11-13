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
  PyObject *pobj, *pstr;

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
    PyErr_Print();
  }
  return TCL_OK;
}

static tcl_cmds my_tcl_cmds[] = {
  {"pysource",  tcl_pysource},
  {NULL,        NULL}
};
