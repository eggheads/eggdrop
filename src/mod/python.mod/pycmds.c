#define PY_SSIZE_T_CLEAN
#include <Python.h>


static PyObject *EggdropError;      //create static Python Exception object

static PyObject *testcmd(PyObject *self, PyObject *args) {
//PyDict_Type
  PyObject *testdict = PyDict_New();
//  dprintf(LOG_SERV, "this is a thing");
//  fatal("eggdrop");

  PyDict_SetItemString(testdict, "return", PyUnicode_FromString("this is the return"));
//  PyDict_SetItemString(testdict, "code", 0);

  return testdict;
}

static PyObject *py_putmsg(PyObject *self, PyObject *args) {
  char *chan = 0, *msg = 0, *p;

  if(!PyArg_ParseTuple(args, "ss", &chan, &msg)) {
//    PyErr_SetString(PyExc_SyntaxError, "wrong number of args");
    PyErr_SetString(EggdropError, "wrong number of args");
    return NULL;
  }

//  BADARGS(2, 3, " text ?options?");

//  if ((argc == 3) && strcasecmp(argv[2], "-next") &&
//      strcasecmp(argv[2], "-normal")) {
//    Tcl_AppendResult(irp, "unknown putserv option: should be one of: ",
//                     "-normal -next", NULL);
//    return TCL_ERROR;
//  }

  p = strchr(chan, '\n');
  if (p != NULL)
    *p = 0;
  p = strchr(chan, '\r');
  if (p != NULL)
    *p = 0;
//  if (!strcasecmp(t, "-next"))
//    dprintf(DP_SERVER_NEXT, "%s\n", s);
//  else
    dprintf(DP_SERVER, "PRIVMSG %s :%s\n", chan, msg);
  Py_RETURN_NONE;
}


static PyMethodDef MyPyMethods[] = {
    {"testcmd", testcmd, METH_VARARGS, "A test dict"},
    {"putmsg", py_putmsg, METH_VARARGS, "Send message to server"},
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

PyMODINIT_FUNC PyInit_eggdrop(void) {
  PyObject *pymodobj;

  pymodobj = PyModule_Create(&eggdrop);
  if (pymodobj == NULL)
    return NULL;

  EggdropError = PyErr_NewException("eggdrop.error", NULL, NULL);
  Py_XINCREF(EggdropError);
  if (PyModule_AddObject(pymodobj, "error", EggdropError) < 0) {
    Py_XDECREF(EggdropError);
    Py_CLEAR(EggdropError);
    Py_DECREF(pymodobj);
    return NULL;
  }

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
