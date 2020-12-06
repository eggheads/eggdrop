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
      putlog(LOG_MISC, "*", "Ident error: Wrote %i bytes instead of %i bytes.", i, count);
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
  FILE *fd;
  long filesize;
  char *data = NULL;
  char path[121], line[256], buf[256], identstr[256];
#ifdef IPV6
  char s[INET6_ADDRSTRLEN];
#else
  char s[INET_ADDRSTRLEN];
#endif
  int ret, prevtime, servidx;
  unsigned int size;
  struct sockaddr_storage ss;

  snprintf(identstr, sizeof identstr, "### eggdrop_%s", pid_file);

  if (!home) {
    putlog(LOG_MISC, "*",
           "Ident error: variable HOME is not in the current environment.");
    return;
  }
  if (snprintf(path, sizeof path, "%s/.oidentd.conf", home) >= sizeof path) {
    putlog(LOG_MISC, "*", "Ident error: path too long.");
    return;
  }
  fd = fopen(path, "r");
  if (fd != NULL) {
    /* Calculate file size for buffer */
    if (fseek(fd, 0, SEEK_END) == 0) {
      filesize = ftell(fd);
      if (filesize == -1) {
        putlog(LOG_MISC, "*", "IDENT: Error reading oident.conf");
      }
      data = nmalloc(filesize + 256); /* Room for Eggdrop adds */
      data[0] = '\0';

      /* Read the file into buffer */
      if (fseek(fd, 0, SEEK_SET) != 0) {
        putlog(LOG_MISC, "*", "IDENT: Error setting oident.conf file pointer");
      } else {
        while (fgets(line, 255, fd)) {
          /* If it is not an Eggdrop entry, don't mess with it */
          if (!strstr(line, "### eggdrop_")) {
            strncat(data, line, ((filesize + 256) - strlen(data)));
          } else {
            /* If it is Eggdrop but not me, check for expiration and remove */
            if (!strstr(line, identstr)) {
              strncpy(buf, line, sizeof buf);
              strtok(buf, "!");
              prevtime = atoi(strtok(NULL, "!"));
              if ((now - prevtime) > 300) {
                putlog(LOG_DEBUG, "*", "IDENT: Removing expired oident.conf entry: \"%s\"", buf);
              } else {
                strncat(data, line, ((filesize + 256) - strlen(data)));
              }
            }
          }
        }
      }
    }
    fclose(fd);
  } else {
    putlog(LOG_MISC, "*", "IDENT: Error opening oident.conf for reading");
  }
  servidx = findanyidx(serv);
  size = sizeof ss;
  ret = getsockname(dcc[servidx].sock, (struct sockaddr *) &ss, &size);
  if (ret) {
    putlog(LOG_DEBUG, "*", "IDENT: Error getting socket info for writing");
  }
  fd = fopen(path, "w");
  if (fd != NULL) {
    fprintf(fd, "%s", data);
    if (ss.ss_family == AF_INET) {
      struct sockaddr_in *saddr = (struct sockaddr_in *)&ss;
      fprintf(fd, "lport %i from %s { reply \"%s\" } "
                "### eggdrop_%s !%" PRId64 "\n", ntohs(saddr->sin_port),
                inet_ntop(AF_INET, &(saddr->sin_addr), s, INET_ADDRSTRLEN),
                botuser, pid_file, (int64_t) now);
#ifdef IPV6
    } else if (ss.ss_family == AF_INET6) {
      struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)&ss;
      fprintf(fd, "lport %i from %s { reply \"%s\" } "
                "### eggdrop_%s !%" PRId64 "\n", ntohs(saddr->sin6_port),
                inet_ntop(AF_INET6, &(saddr->sin6_addr), s, INET6_ADDRSTRLEN),
                botuser, pid_file, (int64_t) now);
#endif
    } else {
      putlog(LOG_MISC, "*", "IDENT: Error writing oident.conf line");
    }
    fclose(fd);
  } else {
    putlog(LOG_MISC, "*", "IDENT: Error opening oident.conf for writing");
  }
  if (data) {
    nfree(data);
  }
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
