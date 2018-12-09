/*
 * ident.c -- part of ident.mod
 */
/*
 * MIT License
 *
 * Copyright (c) 2018 Michael Ortmann
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define MODULE_NAME "ident"

#include <errno.h>
#include <fcntl.h>
#include "src/mod/module.h"
#include "server.mod/server.h"

#define IDENT_METHOD_OIDENT  0
#define IDENT_METHOD_BUILTIN 1

static Function *global = NULL, *server_funcs = NULL;;

static int ident_method = IDENT_METHOD_OIDENT;

static tcl_ints identints[] = {
  {"ident-method", &ident_method, 0},
  {NULL,           NULL,          0}
};

static void ident_builtin_off()
{
  putlog(LOG_MISC, "*", "Ident: ident_builtin_off() not implemented yet.");
}

static cmd_t ident_raw[] = {
  {"001", "",   (IntFunc) ident_builtin_off, "ident:001"},
  {NULL,  NULL, NULL,                        NULL}
};

static void ident_activity(int idx, char *buf, int len)
{
  putlog(LOG_MISC, "*", "Ident: ident_activity() not implemented yet.");
}

static void ident_display(int idx, char *buf)
{
  strcpy(buf, "ident (ready)");
}

static struct dcc_table DCC_IDENTD = {
  "IDENTD",
  DCT_LISTEN,
  NULL,
  ident_activity,
  NULL,
  NULL,
  ident_display,
  NULL,
  NULL,
  NULL
};

static void ident_oidentd()
{
  char *home = getenv("HOME");
  char path[121], buf[(sizeof "global{reply \"\"}") + USERLEN];
  int nbytes;
  int fd;

  if (!home) {
    putlog(LOG_MISC, "*",
           "Ident error: variable HOME is not in the current environment.");
    return;
  }
  if (snprintf(path, sizeof path, "%s/.oidentd.conf", home) >= sizeof path) {
    putlog(LOG_MISC, "*", "Ident error: path too long.");
    return;
  }
  if ((fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IROTH)) < 0) {
    putlog(LOG_MISC, "*", "Ident error: %s", strerror(errno));
    return;
  }
  nbytes = snprintf(buf, sizeof buf, "global{reply \"%s\"}", botuser);
  if (write(fd, buf, nbytes) < 0)
    putlog(LOG_MISC, "*", "Ident error: %s", strerror(errno));
  close(fd);
}

static void ident_builtin_on()
{
  int idx;

  idx = new_dcc(&DCC_IDENTD, 0);
  if (idx < 0) {
    putlog(LOG_MISC, "*", "Ident error: could not get new dcc.");
    return;
  }
  putlog(LOG_MISC, "*", "Ident: ident_builtin_on() not implemented yet.");
}

static void ident_ident()
{
  if (ident_method == IDENT_METHOD_OIDENT)
    ident_oidentd();
  else if (ident_method == IDENT_METHOD_BUILTIN)
    ident_builtin_on();
}

static cmd_t ident_event[] = {
  {"ident", "",   (IntFunc) ident_ident, "ident:ident"},
  {NULL,    NULL, NULL,                  NULL}
};

static char *ident_close()
{
  rem_builtins(H_event, ident_event);
  rem_builtins(H_raw, ident_raw);
  rem_tcl_ints(identints);
  del_hook(HOOK_IDENT, (Function) ident_ident);
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *ident_start();

static Function ident_table[] = {
  (Function) ident_start,
  (Function) ident_close,
  NULL,
  NULL,
};

char *ident_start(Function *global_funcs)
{
  global = global_funcs;

  module_register(MODULE_NAME, ident_table, 0, 1);

  if (!module_depend(MODULE_NAME, "eggdrop", 108, 4)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.8.4 or later.";
  }
  if (!(server_funcs = module_depend(MODULE_NAME, "server", 1, 0))) {
    module_undepend(MODULE_NAME);
    return "This module requires server module 1.0 or later.";
  }

  add_builtins(H_event, ident_event);
  add_builtins(H_raw, ident_raw);
  add_tcl_ints(identints);
  add_hook(HOOK_IDENT, (Function) ident_ident);

  return NULL;
}
