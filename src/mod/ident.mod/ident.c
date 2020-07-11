/*
 * ident.c -- part of ident.mod
 */
/*
 * Copyright (c) 2018 - 2019 Michael Ortmann MIT License
 * Copyright (C) 2019 - 2020 Eggheads Development Team
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

#define MODULE_NAME "ident"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "src/mod/module.h"
#include "server.mod/server.h"

#define IDENT_METHOD_OIDENT     0
#define IDENT_METHOD_BUILTIN    1
#define IDENT_SIZE           1000 /* rfc1413 */

static Function *global = NULL, *server_funcs = NULL;

static int ident_method = IDENT_METHOD_OIDENT;
static int ident_port = 113;

static tcl_ints identints[] = {
  {"ident-method", &ident_method, 0},
  {"ident-port",   &ident_port,   0},
  {NULL,           NULL,          0}
};

static void ident_builtin_off();

static cmd_t ident_raw[] = {
  {"001", "",   (IntFunc) ident_builtin_off, "ident:001"},
  {NULL,  NULL, NULL,                        NULL}
};

static void ident_activity(int idx, char *buf, int len)
{
  int s;
  char buf2[IDENT_SIZE + sizeof " : USERID : UNIX : \r\n" + NICKLEN], *pos;
  size_t count;
  ssize_t i;

  s = answer(dcc[idx].sock, &dcc[idx].sockname, 0, 0);
  killsock(dcc[idx].sock);
  dcc[idx].sock = s;
  if ((i = read(s, buf2, IDENT_SIZE)) < 0) {
    putlog(LOG_MISC, "*", "Ident error: %s", strerror(errno));
    return;
  }
  buf2[i - 1] = 0;
  if (!(pos = strpbrk(buf2, "\r\n"))) {
    putlog(LOG_MISC, "*", "Ident error: could not read request.");
    return;
  } 
  snprintf(pos, (sizeof buf2) - (pos - buf2), " : USERID : UNIX : %s\r\n", botname);
  count = strlen(buf2) + 1;
  if ((i = write(s, buf2, count)) != count) {
    if (i < 0)
      putlog(LOG_MISC, "*", "Ident error: %s", strerror(errno));
    else
      putlog(LOG_MISC, "*", "Ident error: could not write response. Wrote %i instead of %i bytes.", i, count);
    return;
  }
  putlog(LOG_MISC, "*", "Ident: Responded.");
  ident_builtin_off();
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
  int idx, s;

  debug0("Ident: Starting ident server.");
  for (idx = 0; idx < dcc_total; idx++)
    if (dcc[idx].type == &DCC_IDENTD)
      return;
  idx = new_dcc(&DCC_IDENTD, 0);
  if (idx < 0) {
    putlog(LOG_MISC, "*", "Ident error: could not get new dcc.");
    return;
  }
  s = open_listen(&ident_port);
  if (s == -2) {
    lostdcc(idx);
    putlog(LOG_MISC, "*", "Ident error: could not bind socket port %i.", ident_port);
    return;
  } else if (s == -1) {
    lostdcc(idx);
    putlog(LOG_MISC, "*", "Ident error: could not get socket.");
    return;
  }
  dcc[idx].sock = s;
  dcc[idx].port = ident_port;
  strcpy(dcc[idx].nick, "(ident)");
  add_builtins(H_raw, ident_raw);
}

static void ident_builtin_off()
{
  int idx;

  for (idx = 0; idx < dcc_total; idx++)
    if (dcc[idx].type == &DCC_IDENTD) {
      debug0("Ident: Stopping ident server.");
      killsock(dcc[idx].sock);
      lostdcc(idx);
      break;
    }
  rem_builtins(H_raw, ident_raw);
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
  ident_builtin_off();
  rem_builtins(H_event, ident_event);
  rem_tcl_ints(identints);
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

  module_register(MODULE_NAME, ident_table, 0, 9);

  if (!module_depend(MODULE_NAME, "eggdrop", 109, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.9.0 or later.";
  }
  if (!(server_funcs = module_depend(MODULE_NAME, "server", 1, 0))) {
    module_undepend(MODULE_NAME);
    return "This module requires server module 1.0 or later.";
  }

  add_builtins(H_event, ident_event);
  add_tcl_ints(identints);

  return NULL;
}
