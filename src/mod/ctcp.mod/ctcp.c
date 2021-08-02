/*
 * ctcp.c -- part of ctcp.mod
 *   all the ctcp handling (except DCC, it's special ;)
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2021 Eggheads Development Team
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

#define MODULE_NAME "ctcp"
#define MAKING_CTCP

#include "ctcp.h"
#include "src/mod/module.h"
#include "server.mod/server.h"
#include <netinet/in.h>
#include <arpa/inet.h>

static Function *global = NULL, *server_funcs = NULL;

static char ctcp_version[121];
static char ctcp_finger[121];
static char ctcp_userinfo[121];
static int ctcp_mode = 0;


static int ctcp_FINGER(char *nick, char *uhost, char *handle,
                       char *object, char *keyword, char *text)
{
  if (ctcp_mode != 1 && ctcp_finger[0])
    simple_sprintf(ctcp_reply, "%s\001FINGER %s\001", ctcp_reply, ctcp_finger);
  return 1;
}

static int ctcp_ECHOERR(char *nick, char *uhost, char *handle,
                        char *object, char *keyword, char *text)
{
  if (ctcp_mode != 1 && strlen(text) <= 80)
    simple_sprintf(ctcp_reply, "%s\001%s %s\001", ctcp_reply, keyword, text);
  return 1;
}

static int ctcp_PING(char *nick, char *uhost, char *handle,
                     char *object, char *keyword, char *text)
{
  struct userrec *u = get_user_by_handle(userlist, handle);
  int atr = u ? u->flags : 0;

  if ((ctcp_mode != 1 || (atr & USER_OP)) && strlen(text) <= 80)
    simple_sprintf(ctcp_reply, "%s\001%s %s\001", ctcp_reply, keyword, text);
  return 1;
}

static int ctcp_VERSION(char *nick, char *uhost, char *handle,
                        char *object, char *keyword, char *text)
{
  if (ctcp_mode != 1 && ctcp_version[0])
    simple_sprintf(ctcp_reply, "%s\001VERSION %s\001", ctcp_reply,
                   ctcp_version);
  return 1;
}

static int ctcp_USERINFO(char *nick, char *uhost, char *handle,
                         char *object, char *keyword, char *text)
{
  if (ctcp_mode != 1 && ctcp_userinfo[0])
    simple_sprintf(ctcp_reply, "%s\001USERINFO %s\001", ctcp_reply,
                   ctcp_userinfo);
  return 1;
}

static int ctcp_CLIENTINFO(char *nick, char *uhosr, char *handle,
                           char *object, char *keyword, char *msg)
{
  char *p = NULL;

  if (ctcp_mode == 1)
    return 1;
  else if (!msg[0])
    p = CLIENTINFO;
  else if (!strcasecmp(msg, "sed"))
    p = CLIENTINFO_SED;
  else if (!strcasecmp(msg, "version"))
    p = CLIENTINFO_VERSION;
  else if (!strcasecmp(msg, "clientinfo"))
    p = CLIENTINFO_CLIENTINFO;
  else if (!strcasecmp(msg, "userinfo"))
    p = CLIENTINFO_USERINFO;
  else if (!strcasecmp(msg, "errmsg"))
    p = CLIENTINFO_ERRMSG;
  else if (!strcasecmp(msg, "finger"))
    p = CLIENTINFO_FINGER;
  else if (!strcasecmp(msg, "time"))
    p = CLIENTINFO_TIME;
  else if (!strcasecmp(msg, "action"))
    p = CLIENTINFO_ACTION;
  else if (!strcasecmp(msg, "dcc"))
    p = CLIENTINFO_DCC;
  else if (!strcasecmp(msg, "utc"))
    p = CLIENTINFO_UTC;
  else if (!strcasecmp(msg, "ping"))
    p = CLIENTINFO_PING;
  else if (!strcasecmp(msg, "echo"))
    p = CLIENTINFO_ECHO;
  if (p == NULL) {
    simple_sprintf(ctcp_reply,
                   "%s\001ERRMSG CLIENTINFO: %s is not a valid function\001",
                   ctcp_reply, msg);
  } else
    simple_sprintf(ctcp_reply, "%s\001CLIENTINFO %s\001", ctcp_reply, p);
  return 1;
}

static int ctcp_TIME(char *nick, char *uhost, char *handle, char *object,
                     char *keyword, char *text)
{
  char tms[25];

  if (ctcp_mode == 1)
    return 1;
  strlcpy(tms, ctime(&now), sizeof tms);
  simple_sprintf(ctcp_reply, "%s\001TIME %s\001", ctcp_reply, tms);
  return 1;
}

static int ctcp_CHAT(char *nick, char *uhost, char *handle, char *object,
                     char *keyword, char *text)
{
  struct userrec *u = get_user_by_handle(userlist, handle);
  int atr = u ? u->flags : 0, i;
  int chatv = AF_UNSPEC;
  char s[INET6_ADDRSTRLEN];
#ifdef TLS
  int ssl = 0;
#endif

  if ((atr & (USER_PARTY | USER_XFER)) || ((atr & USER_OP) && !require_p)) {

    if (u_pass_match(u, "-")) {
      simple_sprintf(ctcp_reply, "%s\001ERROR no password set\001",
                     ctcp_reply);
      return 1;
    }

/* Check if SSL, IPv4, or IPv6 were requested */
    if (
#ifdef IPV6
    (!strcasecmp(keyword, "CHAT6")) ||
        (!strcasecmp(keyword, "SCHAT6"))) {
      chatv = AF_INET6;
    } else if (
#endif
#ifdef TLS
    (!strcasecmp(keyword, "SCHAT")) ||
#ifdef IPV6
        (!strcasecmp(keyword, "SCHAT6")) ||
#endif
        (!strcasecmp(keyword, "SCHAT4"))) {
      ssl = 1;
    } else if (
#endif
    (!strcasecmp(keyword, "CHAT4")) ||
        (!strcasecmp(keyword, "SCHAT4"))) {
      chatv = AF_INET;
    }
  
    for (i = 0; i < dcc_total; i++) {
      if ((dcc[i].type->flags & DCT_LISTEN) &&
#ifdef TLS
          (ssl == dcc[i].ssl) &&
#endif
          (!strcmp(dcc[i].nick, "(telnet)") ||
           !strcmp(dcc[i].nick, "(users)")) &&
          getdccfamilyaddr(NULL, s, sizeof s, chatv)) {
        /* Do me a favour and don't change this back to a CTCP reply,
         * CTCP replies are NOTICE's this has to be a PRIVMSG
         * -poptix 5/1/1997 */
#ifdef TLS
          dprintf(DP_SERVER, "PRIVMSG %s :\001DCC %sCHAT chat %s %u\001\n",
                  nick, (ssl ? "S" : ""), s, dcc[i].port);
#else
          dprintf(DP_SERVER, "PRIVMSG %s :\001DCC CHAT chat %s %u\001\n",
                  nick, s, dcc[i].port);
#endif
        return 1;
      }
    }
#ifdef TLS
    simple_sprintf(ctcp_reply, "%s\001ERROR no %stelnet port\001", ctcp_reply,
                   (ssl ? "SSL enabled " : ""));
#else
    simple_sprintf(ctcp_reply, "%s\001ERROR no telnet port\001", ctcp_reply);
#endif
  }
  return 1;
}

static cmd_t myctcp[] = {
  {"FINGER",     "",   ctcp_FINGER,     NULL},
  {"ECHO",       "",   ctcp_ECHOERR,    NULL},
  {"PING",       "",   ctcp_PING,       NULL},
  {"ERRMSG",     "",   ctcp_ECHOERR,    NULL},
  {"VERSION",    "",   ctcp_VERSION,    NULL},
  {"USERINFO",   "",   ctcp_USERINFO,   NULL},
  {"CLIENTINFO", "",   ctcp_CLIENTINFO, NULL},
  {"TIME",       "",   ctcp_TIME,       NULL},
  {"CHAT",       "",   ctcp_CHAT,       NULL},
  {"CHAT4",      "",   ctcp_CHAT,       NULL},
#ifdef IPV6
  {"CHAT6",      "",   ctcp_CHAT,       NULL},
#endif
#ifdef TLS
  {"SCHAT",      "",   ctcp_CHAT,       NULL},
  {"SCHAT4",     "",   ctcp_CHAT,       NULL},
#ifdef IPV6
  {"SCHAT6",     "",   ctcp_CHAT,       NULL},
#endif
#endif
  {NULL,         NULL, NULL,            NULL}
};

static tcl_strings mystrings[] = {
  {"ctcp-version",  ctcp_version,  120, 0},
  {"ctcp-finger",   ctcp_finger,   120, 0},
  {"ctcp-userinfo", ctcp_userinfo, 120, 0},
  {NULL,            NULL,          0,   0}
};

static tcl_ints myints[] = {
  {"ctcp-mode", &ctcp_mode},
  {NULL,              NULL}
};

static char *ctcp_close()
{
  rem_tcl_strings(mystrings);
  rem_tcl_ints(myints);
  rem_builtins(H_ctcp, myctcp);
  rem_help_reference("ctcp.help");
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *ctcp_start();

static Function ctcp_table[] = {
  (Function) ctcp_start,
  (Function) ctcp_close,
  (Function) NULL,
  (Function) NULL,
};

char *ctcp_start(Function *global_funcs)
{
  global = global_funcs;

  module_register(MODULE_NAME, ctcp_table, 1, 1);
  if (!module_depend(MODULE_NAME, "eggdrop", 108, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.8.0 or later.";
  }
  if (!(server_funcs = module_depend(MODULE_NAME, "server", 1, 0))) {
    module_undepend(MODULE_NAME);
    return "This module requires server module 1.0 or later.";
  }
  add_tcl_strings(mystrings);
  add_tcl_ints(myints);
  add_builtins(H_ctcp, myctcp);
  add_help_reference("ctcp.help");
  if (!ctcp_version[0])
    strlcpy(ctcp_version, ver, sizeof ctcp_version);
  if (!ctcp_finger[0])
    strlcpy(ctcp_finger, ver, sizeof ctcp_finger);
  if (!ctcp_userinfo[0])
    strlcpy(ctcp_userinfo, ver, sizeof ctcp_userinfo);
  return NULL;
}
