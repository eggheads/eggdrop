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
#define MAKING_TWITCH

#include "src/mod/module.h"
#undef interp
#include <stdlib.h>
#include <Python.h>
#include "src/mod/irc.mod/irc.h"
#include "src/mod/server.mod/server.h"
#include "src/mod/python.mod/python.h"


#undef global
static Function *global = NULL, *irc_funcs = NULL;

//extern p_tcl_bind_list H_pubm;

int runPython();


/* Calculate the memory we keep allocated.
 */
static int python_expmem()
{
  int size = 0;

  Context;
  return size;
}

int py_pubm (char *nick, char *hand, char *host, char *chan, char *text) {
  char arg[2048];

  strlcpy(arg, "binds ", sizeof arg);
  strncat(arg, "on_pub ", sizeof arg - strlen(arg));
  strncat(arg, nick, sizeof arg - strlen(arg));
  strncat(arg, " ", sizeof arg - strlen(arg));
  strncat(arg, host, sizeof arg - strlen(arg));
  strncat(arg, " ", sizeof arg - strlen(arg));
  strncat(arg, hand, sizeof arg - strlen(arg));
  strncat(arg, " ", sizeof arg - strlen(arg));
  strncat(arg, chan, sizeof arg - strlen(arg));
  strncat(arg, " ", sizeof arg - strlen(arg));
  strncat(arg, text, sizeof arg - strlen(arg));
  runPython(7, arg);
/* call "on_pub" in python */;
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
    PyModuleDef_HEAD_INIT, "py", NULL, -1, PyMethods,
    NULL, NULL, NULL, NULL
};

static PyObject* PyInit_py(void) {
    return PyModule_Create(&PyModule);
}


static void init_python() {
  wchar_t *program = Py_DecodeLocale("eggdrop", NULL);

  if (program == NULL) {
    fprintf(stderr, "Python: Fatal error: This should never happen\n");
    fatal(1);
  }
  Py_SetProgramName(program);  /* optional but recommended */
  PyImport_AppendInittab("py", &PyInit_py);
  Py_Initialize();
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

int runPython(int pyc, char *args)
{
    PyObject *pName, *pModule, *pFunc;
    PyObject *pArgs, *pValue;
    int i=0;
    char *tok;
    char script[256];
    char method[256];
    char *pyv[pyc];

    tok = strtok(args, " ");
    strlcpy(script, tok, sizeof script);
    pName = PyUnicode_DecodeFSDefault(script);
    /* Error checking of pName left out */
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    tok = strtok(NULL, " ");    
    strlcpy(method, tok, sizeof method);
    while (tok !=NULL) {
        pyv[i] = tok;
        tok = strtok(NULL, " ");
fprintf(stderr, "pyv[%d] is %s\n", i, pyv[i]);
        i++;
    }
    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, method);
        /* pFunc is a new reference */
        if (pFunc && PyCallable_Check(pFunc)) {
            pArgs = PyTuple_New(pyc-2);
            for (i = 0; i < pyc-2; ++i) {
                pValue = Py_BuildValue("s", pyv[i+1]);
fprintf(stderr, "i is %d, Adding %s\n", i, pyv[i+1]);
                if (!pValue) {
                    Py_DECREF(pArgs);
                    Py_DECREF(pModule);
                    fprintf(stderr, "Cannot convert argument\n");
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


static void cmd_python(struct userrec *u, int idx, char *par) {
  PyRun_SimpleString("import sys");
  PyRun_SimpleString("sys.path.append(\".\")");
  PyRun_SimpleString(par);
  return;
}

static void cmd_pysource(struct userrec *u, int idx, char *par) {
int i, pyc=-1;

  if (!par[0]) {
    dprintf(idx, "Usage: pysource <file> <method> [args]\n");
    return;
  }
  for (i = 0; par[i] != '\0'; i++) {
    if (par[i] == ' ') {
      pyc++;
    }
  }
  
  runPython(pyc, par);

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
  {NULL,        NULL,   NULL,                   NULL}  /* Mark end. */
};

static cmd_t mypy[] = {
    {"*",   "",     py_pubm,  "python:pubm"},
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
  rem_builtins(H_pubm, mypy);
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
  add_builtins(H_pubm, mypy);
  add_builtins(H_dcc, mydcc);
//  add_tcl_commands(mytcl);
//  add_tcl_ints(my_tcl_ints);
//  add_tcl_strings(my_tcl_strings);
  return NULL;
}
