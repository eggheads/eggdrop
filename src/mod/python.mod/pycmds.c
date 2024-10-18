/*
 * pycmds.c -- python.mod python functions
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
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <datetime.h>
#include <tcl.h>
#include "src/mod/module.h"

typedef struct {
  PyObject_HEAD
  char tclcmdname[128];
} TclFunc;

typedef struct {
  PyObject_HEAD
  char tclcmdname[128];
  char *flags;
  char *mask;
  tcl_bind_list_t *bindtable;
  PyObject *callback;
} PythonBind;
  
static PyTypeObject TclFuncType, PythonBindType;
static int eval_idx = -1;

static PyObject *EggdropError;      //create static Python Exception object

static Tcl_Obj *py_to_tcl_obj(PyObject *o); // generic conversion function

static PyObject *py_displayhook(PyObject *self, PyObject *o) {
  PyObject *pstr;

  if (o) {
    pstr = PyObject_Repr(o);
    if (pstr) {
      dprintf(eval_idx, "Python: %s\n", PyUnicode_AsUTF8(pstr));
      Py_DECREF(pstr);
    }
  }
  Py_RETURN_NONE;
}

static void cmd_python(struct userrec *u, int idx, char *par) {
  PyObject *pobj, *ptype, *pvalue, *ptraceback;
  PyObject *pystr, *module_name, *pymodule, *pyfunc, *pyval, *item;
  Py_ssize_t n;
  int i;

  PyErr_Clear();

  // Expression output redirection via sys.displayhook
  eval_idx = idx;
  pobj = PyRun_String(par, Py_single_input, pglobals, pglobals);

  if (pobj) {
    // always None
    Py_DECREF(pobj);
  } else if (PyErr_Occurred()) {
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    pystr = PyObject_Str(pvalue);
    // Get "pretty" error result
    dprintf(eval_idx, "Python Error: %s\n", PyUnicode_AsUTF8(pystr));
    module_name = PyUnicode_FromString("traceback");
    pymodule = PyImport_Import(module_name);
    Py_DECREF(module_name);
    // format backtrace and print
    pyfunc = PyObject_GetAttrString(pymodule, "format_exception");
    if (pyfunc && PyCallable_Check(pyfunc)) {
      pyval = PyObject_CallFunctionObjArgs(pyfunc, ptype, pvalue, ptraceback, NULL);
      // Check if traceback is a list and handle as such
      if (pyval && PyList_Check(pyval)) {
        n = PyList_Size(pyval);
        for (i = 0; i < n; i++) {
          item = PyList_GetItem(pyval, i);
          pystr = PyObject_Str(item);
          dprintf(idx, "%s", PyUnicode_AsUTF8(pystr));
        }
      } else {
        pystr = PyObject_Str(pyval);
        dprintf(idx, "%s", PyUnicode_AsUTF8(pystr));
      }
      Py_XDECREF(pyval);
    }
  }
  return;
}

static PyObject *make_ircuser_dict(memberlist *m) {
  PyObject *result = PyDict_New();
  PyDict_SetItemString(result, "nick", PyUnicode_FromString(m->nick));
  PyDict_SetItemString(result, "host", PyUnicode_FromString(m->userhost));
  if (m->joined) {
    PyObject *tmp = PyTuple_New(1);
    PyTuple_SET_ITEM(tmp, 0, PyFloat_FromDouble((double)m->joined));
    PyDict_SetItemString(result, "joined", PyDateTime_FromTimestamp(tmp));
  }
  if (m->last) {
    PyObject *tmp = PyTuple_New(1);
    PyTuple_SET_ITEM(tmp, 0, PyFloat_FromDouble((double)m->last));
    PyDict_SetItemString(result, "lastseen", PyDateTime_FromTimestamp(tmp));
  }
  PyDict_SetItemString(result, "account", m->account[0] ? PyUnicode_FromString(m->account) : Py_None);
  return result;
}

static PyObject *py_findircuser(PyObject *self, PyObject *args) {
  char *nick, *chan = NULL;

  if (!PyArg_ParseTuple(args, "s|s", &nick, &chan)) {
    PyErr_SetString(EggdropError, "wrong number of args");
    return NULL;
  }
  for (struct chanset_t *ch = chan ? findchan_by_dname(chan) : chanset; ch; ch = chan ? NULL : ch->next) {
    memberlist *m = ismember(ch, nick);
    if (m) {
      return make_ircuser_dict(m);
    }
  }
  Py_RETURN_NONE;
}

static int tcl_call_python(ClientData cd, Tcl_Interp *irp, int objc, Tcl_Obj *const objv[])
{
  PyObject *args = PyTuple_New(objc > 1 ? objc - 1: 0);
  PythonBind *bind = cd;

  // objc[0] is procname
  for (int i = 1; i < objc; i++) {
    PyTuple_SET_ITEM(args, i - 1, Py_BuildValue("s", Tcl_GetStringFromObj(objv[i], NULL)));
  }
  if (!PyObject_Call(bind->callback, args, NULL)) {
    PyErr_Print();
    Tcl_SetResult(irp, "Error calling python code", TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

static PyObject *py_parse_tcl_list(PyObject *self, PyObject *args) {
  Tcl_Size max;
  const char *str;
  Tcl_Obj *strobj;
  PyObject *result;

  if (!PyArg_ParseTuple(args, "s", &str)) {
    PyErr_SetString(PyExc_TypeError, "Argument is not a unicode string");
    return NULL;
  }
  strobj = Tcl_NewStringObj(str, -1);
  Tcl_IncrRefCount(strobj);
  if (Tcl_ListObjLength(tclinterp, strobj, &max) != TCL_OK) {
    Tcl_DecrRefCount(strobj);
    PyErr_SetString(EggdropError, "Supplied string is not a Tcl list");
  }
  result = PyList_New(max);
  for (int i = 0; i < max; i++) {
    Tcl_Obj *tclobj;
    const char *tclstr;
    Tcl_Size tclstrlen;

    Tcl_ListObjIndex(tclinterp, strobj, i, &tclobj);
    tclstr = Tcl_GetStringFromObj(tclobj, &tclstrlen);
    PyList_SetItem(result, i, PyUnicode_DecodeUTF8(tclstr, tclstrlen, NULL));
  }
  Tcl_DecrRefCount(strobj);
  return result;
}

static PyObject *py_parse_tcl_dict(PyObject *self, PyObject *args) {
  int done;
  const char *str;
  Tcl_Obj *strobj, *key, *value;
  Tcl_DictSearch search;
  PyObject *result;

  if (!PyArg_ParseTuple(args, "s", &str)) {
    PyErr_SetString(PyExc_TypeError, "Argument is not a unicode string");
    return NULL;
  }
  strobj = Tcl_NewStringObj(str, -1);
  if (Tcl_DictObjFirst(tclinterp, strobj, &search, &key, &value, &done) != TCL_OK) {
    PyErr_SetString(EggdropError, "Supplied string is not a Tcl dictionary");
    return NULL;
  }
  result = PyDict_New();
  while (!done) {
    Tcl_Size len;
    const char *valstr = Tcl_GetStringFromObj(value, &len);
    PyObject *pyval = PyUnicode_DecodeUTF8(valstr, len, NULL);
    PyDict_SetItemString(result, Tcl_GetString(key), pyval);
    Tcl_DictObjNext(&search, &key, &value, &done);
  }
  Tcl_DictObjDone(&search);
  return result;
}

static PyObject *py_unbind(PyObject *self, PyObject *args) {
  PythonBind *bind;

  if (!PyObject_TypeCheck(self, &PythonBindType)) {
    PyErr_SetString(EggdropError, "Invalid argument for unbind method");
    return NULL;
  }
 
  bind = (PythonBind *)self;
  unbind_bind_entry(bind->bindtable, bind->flags, bind->mask, bind->tclcmdname);
  // cleanup in python_bind_destroyed callback when Tcl command is destroyed
  Py_RETURN_NONE;
}

void python_bind_destroyed(ClientData cd) {
  PythonBind *bind = cd;

  Py_DECREF(bind->callback);
  nfree(bind->mask);
  nfree(bind->flags);
  Py_DECREF((PyObject *)bind);
}

static PyObject *py_bind(PyObject *self, PyObject *args) {
  PyObject *callback;
  PythonBind *bind;
  Py_hash_t hash;
  char *bindtype, *mask, *flags;
  tcl_bind_list_t *tl;
 
  // type flags mask callback
  if (!PyArg_ParseTuple(args, "sssO", &bindtype, &flags, &mask, &callback) || !callback) {
    PyErr_SetString(EggdropError, "wrong arguments");
    return NULL;
  }
  if (!(tl = find_bind_table(bindtype))) {
    PyErr_SetString(EggdropError, "unknown bind type");
    return NULL;
  }
  if (callback == Py_None) {
    PyErr_SetString(EggdropError, "callback is None");
    return NULL;
  }
  if (!PyCallable_Check(callback)) {
    PyErr_SetString(EggdropError, "callback is not callable");
    return NULL;
  }
  Py_INCREF(callback);

  bind = PyObject_New(PythonBind, &PythonBindType);
  bind->mask = strdup(mask);
  bind->flags = strdup(flags);
  bind->bindtable = tl;
  bind->callback = callback;
  hash = PyObject_Hash((PyObject *)bind);
  snprintf(bind->tclcmdname, sizeof bind->tclcmdname, "*python:%s:%" PRIx64, bindtype, (int64_t)hash);

  Tcl_CreateObjCommand(tclinterp, bind->tclcmdname, tcl_call_python, bind, python_bind_destroyed);
  bind_bind_entry(tl, flags, mask, bind->tclcmdname);

  Py_INCREF((PyObject *)bind);
  return (PyObject *)bind;  
}

static Tcl_Obj *py_list_to_tcl_obj(PyObject *o) {
  int max = PyList_GET_SIZE(o);
  Tcl_Obj *result = Tcl_NewListObj(0, NULL);

  for (int i = 0; i < max; i++) {
    Tcl_ListObjAppendElement(tclinterp, result, py_to_tcl_obj(PyList_GET_ITEM(o, i)));
  }
  return result;
}

static Tcl_Obj *py_tuple_to_tcl_obj(PyObject *o) {
  int max = PyTuple_GET_SIZE(o);
  Tcl_Obj *result = Tcl_NewListObj(0, NULL);

  for (int i = 0; i < max; i++) {
    Tcl_ListObjAppendElement(tclinterp, result, py_to_tcl_obj(PyTuple_GET_ITEM(o, i)));
  }
  return result;
}

static Tcl_Obj *py_dict_to_tcl_obj(PyObject *o) {
  int max;
  Tcl_Obj *result = Tcl_NewDictObj();

  /* operate on list of (key, value) tuples instead */
  o = PyDict_Items(o);
  max = PyList_GET_SIZE(o);
  for (int i = 0; i < max; i++) {
    PyObject *key = PyTuple_GET_ITEM(PyList_GET_ITEM(o, i), 0);
    PyObject *val = PyTuple_GET_ITEM(PyList_GET_ITEM(o, i), 1);
    Tcl_Obj *keyobj = py_to_tcl_obj(key);
    Tcl_Obj *valobj = py_to_tcl_obj(val);
    Tcl_DictObjPut(tclinterp, result, keyobj, valobj);
  }
  return result;
}

static Tcl_Obj *py_str_to_tcl_obj(PyObject *o) {
  Tcl_Obj *ret;
  PyObject *strobj = PyObject_Str(o);

  if (strobj) {
    ret = Tcl_NewStringObj(PyUnicode_AsUTF8(strobj), -1);
    Py_DECREF(strobj);
  } else {
    ret = Tcl_NewObj();
  }
  return ret;
}

static Tcl_Obj *py_to_tcl_obj(PyObject *o) {
  if (PyList_Check(o)) {
    return py_list_to_tcl_obj(o);
  } else if (PyDict_Check(o)) {
    return py_dict_to_tcl_obj(o);
  } else if (PyTuple_Check(o)) {
    return py_tuple_to_tcl_obj(o);
  } else if (o == Py_None) {
    return Tcl_NewObj();
  } else {
    return py_str_to_tcl_obj(o);
  }
}

static PyObject *python_call_tcl(PyObject *self, PyObject *args, PyObject *kwargs) {
  TclFunc *tf = (TclFunc *)self;
  Py_ssize_t argc = PyTuple_Size(args);
  Tcl_DString ds;
  const char *result;
  int retcode;

  Tcl_DStringInit(&ds);
  Tcl_DStringAppendElement(&ds, tf->tclcmdname);
  for (int i = 0; i < argc; i++) {
    PyObject *o = PyTuple_GetItem(args, i);
    Tcl_DStringAppendElement(&ds, Tcl_GetString(py_to_tcl_obj(o)));
  }
  retcode = Tcl_Eval(tclinterp, Tcl_DStringValue(&ds));

  if (retcode != TCL_OK) {
    PyErr_Format(EggdropError, "Tcl error: %s", Tcl_GetStringResult(tclinterp));
    return NULL;
  }
  result = Tcl_GetStringResult(tclinterp);
  //putlog(LOG_MISC, "*", "Python called '%s' -> '%s'", Tcl_DStringValue(&ds), result);

  if (!*result) {
    // Empty string means okay
    Py_RETURN_NONE;
  }

  return PyUnicode_DecodeUTF8(result, strlen(result), NULL);
}


static PyObject *py_dir(PyObject *self, PyObject *args) {
  PyObject *py_list, *py_s;
  size_t i;
  int j;
  const char *info[] = {"info commands", "info procs"}, *s, *value;
  Tcl_Obj *tcl_list, **objv;
  Tcl_Size objc;

  py_list = PyList_New(0);
  for (i = 0; i < sizeof info / sizeof info[0]; i++) {
    s = info[i];
    if (Tcl_VarEval(tclinterp, s, NULL, NULL) == TCL_ERROR)
      putlog(LOG_MISC, "*", "python error: Tcl_VarEval(%s)", s);
    else {
      tcl_list = Tcl_GetObjResult(tclinterp);
      if (Tcl_ListObjGetElements(tclinterp, tcl_list, &objc, &objv) == TCL_ERROR)
        putlog(LOG_MISC, "*", "python error: Tcl_VarEval(%s)", s);
      else {
        for (j = 0; j < objc; j++) {
          value = Tcl_GetString(objv[j]);
          if (*value != '*') {
            py_s = PyUnicode_FromString(value);
            PyList_Append(py_list, py_s);
            Py_DECREF(py_s);
          }
        }
      }
    }
  }
  return py_list;
}

static PyObject *py_findtclfunc(PyObject *self, PyObject *args) {
  char *cmdname;
  TclFunc *result;

  if (!PyArg_ParseTuple(args, "s", &cmdname)) {
    PyErr_SetString(EggdropError, "wrong arguments");
    return NULL;
  }
  // TODO: filter a bit better what is available to Python, specify return types ("list of string"), etc.
  if (!(Tcl_FindCommand(tclinterp, cmdname, NULL, TCL_GLOBAL_ONLY))) {
    PyErr_SetString(PyExc_AttributeError, cmdname);
    return NULL;
  }
  result = PyObject_New(TclFunc, &TclFuncType);
  strlcpy(result->tclcmdname, cmdname, sizeof result->tclcmdname);
  return (PyObject *)result;
}

static PyMethodDef MyPyMethods[] = {
    {"bind", py_bind, METH_VARARGS, "register an eggdrop python bind"},
    {"findircuser", py_findircuser, METH_VARARGS, "find an IRC user by nickname and optional channel"},
    {"parse_tcl_list", py_parse_tcl_list, METH_VARARGS, "convert a Tcl list string to a Python list"},
    {"parse_tcl_dict", py_parse_tcl_dict, METH_VARARGS, "convert a Tcl dict string to a Python dict"},
    {"__displayhook__", py_displayhook, METH_O, "display hook for python expressions"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static PyMethodDef EggTclMethods[] = {
    {"__dir__", py_dir, METH_VARARGS, ""},
    {"__getattr__", py_findtclfunc, METH_VARARGS, "fallback to call Tcl functions transparently"},
    {NULL, NULL, 0, NULL}
};  

static cmd_t mydcc[] = {
  /* command  flags  function     tcl-name */
  {"python",    "",     (IntFunc) cmd_python,   NULL},
  {NULL,        NULL,   NULL,                   NULL}  /* Mark end. */
};

static struct PyModuleDef eggdrop = {
    PyModuleDef_HEAD_INIT,
    "eggdrop",      /* name of module */
    0,              /* module documentation, may be NULL */
    -1,             /* size of per-interpreter state of the module,
                    or -1 if the module keeps state in global variables. */
    MyPyMethods
};

static struct PyModuleDef eggdrop_tcl = { PyModuleDef_HEAD_INIT, "eggdrop.tcl", NULL, -1, EggTclMethods };

static PyTypeObject TclFuncType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "eggdrop.TclFunc",
    .tp_doc = "Tcl function that is callable from Python.",
    .tp_basicsize = sizeof(TclFunc),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_call = python_call_tcl,
};

static PyMethodDef PythonBindMethods[] = {
    {"unbind", py_unbind, METH_VARARGS, "deregister an eggdrop python bind"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static PyTypeObject PythonBindType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "eggdrop.PythonBind",
    .tp_doc = "Eggdrop bind to a python callback",
    .tp_basicsize = sizeof(PythonBind),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_methods = PythonBindMethods
};

PyMODINIT_FUNC PyInit_eggdrop(void) {
  PyObject *pymodobj, *eggtclmodobj, *pymoddict;

  pymodobj = PyModule_Create(&eggdrop);
  if (pymodobj == NULL)
    return NULL;

  EggdropError = PyErr_NewException("eggdrop.error", NULL, NULL);
  Py_INCREF(EggdropError);
  if (PyModule_AddObject(pymodobj, "error", EggdropError) < 0) {
    Py_DECREF(EggdropError);
    Py_CLEAR(EggdropError);
    Py_DECREF(pymodobj);
    return NULL;
  }
  eggtclmodobj = PyModule_Create(&eggdrop_tcl);
  PyModule_AddObject(pymodobj, "tcl", eggtclmodobj);

  pymoddict = PyModule_GetDict(pymodobj);
  PyDict_SetItemString(pymoddict, "tcl", eggtclmodobj);

  pymoddict = PyImport_GetModuleDict();
  PyDict_SetItemString(pymoddict, "eggdrop.tcl", eggtclmodobj);

  PyType_Ready(&TclFuncType);
  PyType_Ready(&PythonBindType);

  return pymodobj;
}
