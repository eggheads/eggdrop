#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <datetime.h>
#include <tcl.h>
#include "src/mod/module.h"

struct py_bind {
  tcl_bind_list_t *bindtable;
  char tclcmdname[512];
  PyObject *callback;
  struct py_bind *next;
};

typedef struct {
  PyObject_HEAD
  char tclcmdname[512];
} TclFunc;
  
static PyTypeObject TclFuncType;

static struct py_bind *py_bindlist;

static PyObject *EggdropError;      //create static Python Exception object

static PyObject *py_ircsend(PyObject *self, PyObject *args) {
  char *text;
  int queuenum;

  if (!PyArg_ParseTuple(args, "si", &text, &queuenum)) {
    PyErr_SetString(EggdropError, "wrong number of args");
    return NULL;
  }

  dprintf(queuenum, "%s\n", text);
  Py_RETURN_NONE;
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
  struct py_bind *bindinfo = cd;

  // objc[0] is procname
  for (int i = 1; i < objc; i++) {
    PyTuple_SET_ITEM(args, i - 1, Py_BuildValue("s", Tcl_GetStringFromObj(objv[i], NULL)));
  }
  if (!PyObject_Call(bindinfo->callback, args, NULL)) {
    PyErr_Print();
    Tcl_SetResult(irp, "Error calling python code", NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

static PyObject *py_bind(PyObject *self, PyObject *args) {
  PyObject *callback;
  char *bindtype, *mask, *flags;
  struct py_bind *bindinfo;
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
  Py_IncRef(callback);

  bindinfo = nmalloc(sizeof *bindinfo);
  bindinfo->bindtable = tl;
  bindinfo->callback = callback;
  bindinfo->next = py_bindlist;
  snprintf(bindinfo->tclcmdname, sizeof bindinfo->tclcmdname, "*python:%s:%s:%" PRIxPTR, bindtype, mask, (uintptr_t)callback);
  py_bindlist = bindinfo;
  // TODO: deleteproc
  Tcl_CreateObjCommand(tclinterp, bindinfo->tclcmdname, tcl_call_python, bindinfo, NULL);
  // TODO: flags?
  bind_bind_entry(tl, flags, mask, bindinfo->tclcmdname);
  Py_RETURN_NONE;
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
    Tcl_DStringAppendElement(&ds, PyUnicode_AsUTF8(PyTuple_GetItem(args, i)));
  }
  retcode = Tcl_Eval(tclinterp, Tcl_DStringValue(&ds));

  if (retcode != TCL_OK) {
    PyErr_Format(EggdropError, "Tcl error: %s", Tcl_GetStringResult(tclinterp));
    return NULL;
  }
  result = Tcl_GetStringResult(tclinterp);
  putlog(LOG_MISC, "*", "Python called '%s' -> '%s'", Tcl_DStringValue(&ds), result);

  return PyUnicode_DecodeUTF8(result, strlen(result), NULL);
}

static PyObject *py_findtclfunc(PyObject *self, PyObject *args) {
  Tcl_Command tclcmd;
  char *cmdname;
  TclFunc *result;

  if (!PyArg_ParseTuple(args, "s", &cmdname)) {
    PyErr_SetString(EggdropError, "wrong arguments");
    return NULL;
  }
  // TODO: filter a bit better what is available to Python, specify return types ("list of string"), etc.
  if (!(tclcmd = Tcl_FindCommand(tclinterp, cmdname, NULL, TCL_GLOBAL_ONLY))) {
    PyErr_SetString(PyExc_AttributeError, cmdname);
    return NULL;
  }
  result = PyObject_New(TclFunc, &TclFuncType);
  strcpy(result->tclcmdname, cmdname);
  return (PyObject *)result;
}

static PyMethodDef MyPyMethods[] = {
    {"ircsend", py_ircsend, METH_VARARGS, "Send message to server"},
    {"bind", py_bind, METH_VARARGS, "register an eggdrop python bind"},
    {"findircuser", py_findircuser, METH_VARARGS, "find an IRC user by nickname and optional channel"},
    {"__getattr__", py_findtclfunc, METH_VARARGS, "fallback to call Tcl functions transparently"},
    // TODO: __dict__ with all valid Tcl commands
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef eggdrop = {
    PyModuleDef_HEAD_INIT,
    "eggdrop",   /* name of module */
    0,              /* module documentation, may be NULL */
    -1,             /* size of per-interpreter state of the module,
                    or -1 if the module keeps state in global variables. */
    MyPyMethods
};

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

PyMODINIT_FUNC PyInit_eggdrop(void) {
  PyObject *pymodobj;

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
  PyType_Ready(&TclFuncType);

  return pymodobj;
}
