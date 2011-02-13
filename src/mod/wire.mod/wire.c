/*
 * wire.c -- part of wire.mod
 *   An encrypted partyline communication.
 *   Compatible with wire.tcl.
 *   This module does not support wire usage in the files area.
 *
 * by ButchBub - Scott G. Taylor (staylor@mrynet.com)
 *
 * Version   Date            Req'd Eggver    Notes                    Who
 * .......   ..........      ............    ....................     ......
 * 1.0       1997-07-17      1.2.0           Initial.                 BB
 * 1.1       1997-07-28      1.2.0           Release version.         BB
 * 1.2       1997-08-19      1.2.1           Update and bugfixes.     BB
 * 1.3       1997-09-24      1.2.2.0         Reprogrammed for 1.2.2   BB
 * 1.4       1997-11-25      1.2.2.0         Added language addition  Kirk
 * 1.5       1998-07-12      1.3.0.0         Fixed ;me and updated    BB
 *
 * $Id: wire.c,v 1.42 2011/02/13 14:19:34 simple Exp $
 */
/*
 * Copyright (C) 1999 - 2011 Eggheads Development Team
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

#define MODULE_NAME "wire"
#define MAKING_WIRE

#include "src/mod/module.h"

#ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else
#    include <time.h>
#  endif
#endif

#include "src/users.h"
#include "src/chan.h"
#include "wire.h"

#undef global
static Function *global = NULL, *encryption_funcs = NULL;

typedef struct wire_t {
  int sock;
  char *crypt;
  char *key;
  struct wire_t *next;
} wire_list;

static wire_list *wirelist;

static cmd_t wire_bot[] = {
  {0, 0, 0, 0},                 /* Saves having to malloc :P */
  {0, 0, 0, 0}                  /* add_builtin expects a terminating elem */
};

static void wire_leave();
static void wire_join();
static void wire_display();

static int wire_expmem()
{
  wire_list *w = wirelist;
  int size = 0;

  while (w) {
    size += sizeof(wire_list);
    size += strlen(w->crypt) + 1;
    size += strlen(w->key) + 1;
    w = w->next;
  }
  return size;
}

static void nsplit(char *to, char *from)
{
  char *x, *y = from;

  x = newsplit(&y);
  strcpy(to, x);
  strcpy(from, y);
}

static void wire_filter(char *from, char *cmd, char *param)
{
  char wirecrypt[512];
  char wirewho[512];
  char wiretmp2[512];
  char wiretmp[512];
  char wirereq[512];
  wire_list *w = wirelist;
  char reqsock;
  time_t now2 = now;
  char idle[20];
  char *enctmp;

  strcpy(wirecrypt, &cmd[5]);
  strcpy(wiretmp, param);
  nsplit(wirereq, param);

/*
 * !wire<crypt"wire"> !wirereq <destbotsock> <crypt"destbotnick">
 * -----  wirecrypt    wirereq    wirewho         param
 */

  if (!strcmp(wirereq, "!wirereq")) {
    nsplit(wirewho, param);
    while (w) {
      if (!strcmp(w->crypt, wirecrypt)) {
        int idx = findanyidx(w->sock);

        reqsock = atoi(wirewho);
        if (now2 - dcc[idx].timeval > 300) {
          unsigned long Days, hrs, mins;

          Days = (now2 - dcc[idx].timeval) / 86400;
          hrs = ((now2 - dcc[idx].timeval) - (Days * 86400)) / 3600;
          mins = ((now2 - dcc[idx].timeval) - (hrs * 3600)) / 60;
          if (Days > 0)
            sprintf(idle, " [%s %lud%luh]", WIRE_IDLE, Days, hrs);
          else if (hrs > 0)
            sprintf(idle, " [%s %luh%lum]", WIRE_IDLE, hrs, mins);
          else
            sprintf(idle, " [%s %lum]", WIRE_IDLE, mins);
        } else
          idle[0] = 0;
        sprintf(wirereq, "----- %c%-9s %-9s  %s%s",
                geticon(idx), dcc[idx].nick, botnetnick, dcc[idx].host, idle);
        enctmp = encrypt_string(w->key, wirereq);
        strcpy(wiretmp, enctmp);
        nfree(enctmp);
        sprintf(wirereq, "zapf %s %s !wire%s !wireresp %s %s %s",
                botnetnick, from, wirecrypt, wirewho, param, wiretmp);
        dprintf(nextbot(from), "%s\n", wirereq);
        if (dcc[idx].u.chat->away) {
          sprintf(wirereq, "-----    %s: %s\n", WIRE_AWAY,
                  dcc[idx].u.chat->away);
          enctmp = encrypt_string(w->key, wirereq);
          strcpy(wiretmp, enctmp);
          nfree(enctmp);
          sprintf(wirereq, "zapf %s %s !wire%s !wireresp %s %s %s",
                  botnetnick, from, wirecrypt, wirewho, param, wiretmp);
          dprintf(nextbot(from), "%s\n", wirereq);
        }
      }
      w = w->next;
    }
    return;
  }
  if (!strcmp(wirereq, "!wireresp")) {
    nsplit(wirewho, param);
    reqsock = atoi(wirewho);
    w = wirelist;
    nsplit(wiretmp2, param);
    while (w) {
      if (w->sock == reqsock) {
        int idx = findanyidx(reqsock);

        enctmp = decrypt_string(w->key, wiretmp2);
        strcpy(wirewho, enctmp);
        nfree(enctmp);
        if (!strcmp(dcc[idx].nick, wirewho)) {
          enctmp = decrypt_string(w->key, param);
          dprintf(idx, "%s\n", enctmp);
          nfree(enctmp);
          return;
        }
      }
      w = w->next;
    }
    return;
  }
  while (w) {
    if (!strcmp(wirecrypt, w->crypt))
      wire_display(findanyidx(w->sock), w->key, wirereq, param);
    w = w->next;
  }
}

static void wire_display(int idx, char *key, char *from, char *message)
{
  char *enctmp;

  enctmp = decrypt_string(key, message);
  if (from[0] == '!')
    dprintf(idx, "----- > %s %s\n", &from[1], enctmp + 1);
  else
    dprintf(idx, "----- <%s> %s\n", from, enctmp);
  nfree(enctmp);
}

static int cmd_wirelist(struct userrec *u, int idx, char *par)
{
  wire_list *w = wirelist;
  int entry = 0;

  dprintf(idx, "Current Wire table:  (Base table address = %p)\n", w);
  while (w) {
    dprintf(idx, "entry %d: w=%p  idx=%d  sock=%d  next=%p\n",
            ++entry, w, findanyidx(w->sock), w->sock, w->next);
    w = w->next;
  }
  return 0;
}

static int cmd_onwire(struct userrec *u, int idx, char *par)
{
  wire_list *w, *w2;
  char wiretmp[512], wirecmd[512], idxtmp[512];
  char idle[20], *enctmp;
  time_t now2 = now;

  w = wirelist;
  while (w) {
    if (w->sock == dcc[idx].sock)
      break;
    w = w->next;
  }
  if (!w) {
    dprintf(idx, "%s\n", WIRE_NOTONWIRE);
    return 0;
  }
  dprintf(idx, "----- %s '%s':\n", WIRE_CURRENTLYON, w->key);
  dprintf(idx, "----- Nick       Bot        Host\n");
  dprintf(idx, "----- ---------- ---------- ------------------------------\n");
  enctmp = encrypt_string(w->key, "wire");
  sprintf(wirecmd, "!wire%s", enctmp);
  nfree(enctmp);
  enctmp = encrypt_string(w->key, dcc[idx].nick);
  strcpy(wiretmp, enctmp);
  nfree(enctmp);
  simple_sprintf(idxtmp, "!wirereq %d %s", dcc[idx].sock, wiretmp);
  botnet_send_zapf_broad(-1, botnetnick, wirecmd, idxtmp);
  w2 = wirelist;
  while (w2) {
    if (!strcmp(w2->key, w->key)) {
      int idx2 = findanyidx(w2->sock);

      if (now2 - dcc[idx2].timeval > 300) {
        unsigned long Days, hrs, mins;

        Days = (now2 - dcc[idx2].timeval) / 86400;
        hrs = ((now2 - dcc[idx2].timeval) - (Days * 86400)) / 3600;
        mins = ((now2 - dcc[idx2].timeval) - (hrs * 3600)) / 60;
        if (Days > 0)
          sprintf(idle, " [%s %lud%luh]", WIRE_IDLE, Days, hrs);
        else if (hrs > 0)
          sprintf(idle, " [%s %luh%lum]", WIRE_IDLE, hrs, mins);
        else
          sprintf(idle, " [%s %lum]", WIRE_IDLE, mins);
      } else
        idle[0] = 0;
      dprintf(idx, "----- %c%-9s %-9s  %s%s\n",
              geticon(idx2), dcc[idx2].nick, botnetnick, dcc[idx2].host, idle);
      if (dcc[idx2].u.chat->away)
        dprintf(idx, "-----    %s: %s\n", WIRE_AWAY, dcc[idx2].u.chat->away);
    }
    w2 = w2->next;
  }
  return 0;
}

static int cmd_wire(struct userrec *u, int idx, char *par)
{
  wire_list *w = wirelist;

  if (!par[0]) {
    dprintf(idx, "%s: .wire [<encrypt-key>|OFF|info]\n", MISC_USAGE);
    return 0;
  }
  while (w) {
    if (w->sock == dcc[idx].sock)
      break;
    w = w->next;
  }
  if (!egg_strcasecmp(par, "off")) {
    if (w) {
      wire_leave(w->sock);
      dprintf(idx, "%s\n", WIRE_NOLONGERWIRED);
      return 0;
    }
    dprintf(idx, "%s\n", WIRE_NOTONWIRE);
    return 0;
  }
  if (!egg_strcasecmp(par, "info")) {
    if (w)
      dprintf(idx, "%s '%s'.\n", WIRE_CURRENTLYON, w->key);
    else
      dprintf(idx, "%s\n", WIRE_NOTONWIRE);
    return 0;
  }
  if (w) {
    dprintf(idx, "%s %s...\n", WIRE_CHANGINGKEY, par);
    wire_leave(w->sock);
  } else {
    dprintf(idx, "----- %s\n", WIRE_INFO1);
    dprintf(idx, "----- %s\n", WIRE_INFO2);
    dprintf(idx, "----- %s\n", WIRE_INFO3);
  }
  wire_join(idx, par);
  cmd_onwire((struct userrec *) 0, idx, "");
  return 0;
}

static char *chof_wire(char *from, int idx)
{
  wire_leave(dcc[idx].sock);
  return NULL;
}

static void wire_join(int idx, char *key)
{
  char wirecmd[512];
  char wiremsg[512];
  char wiretmp[512];
  char *enctmp;
  wire_list *w = wirelist, *w2;

  while (w) {
    if (w->next == 0)
      break;
    w = w->next;
  }
  if (!wirelist) {
    wirelist = nmalloc(sizeof *wirelist);
    w = wirelist;
  } else {
    w->next = nmalloc(sizeof *w->next);
    w = w->next;
  }
  w->sock = dcc[idx].sock;
  w->key = nmalloc(strlen(key) + 1);
  strcpy(w->key, key);
  w->next = 0;
  enctmp = encrypt_string(w->key, "wire");
  strcpy(wiretmp, enctmp);
  nfree(enctmp);
  w->crypt = nmalloc(strlen(wiretmp) + 1);
  strcpy(w->crypt, wiretmp);
  sprintf(wirecmd, "!wire%s", wiretmp);
  sprintf(wiremsg, "%s joined wire '%s'", dcc[idx].nick, key);
  enctmp = encrypt_string(w->key, wiremsg);
  strcpy(wiretmp, enctmp);
  nfree(enctmp);
  {
    char x[1024];

    simple_sprintf(x, "%s %s", botnetnick, wiretmp);
    botnet_send_zapf_broad(-1, botnetnick, wirecmd, x);
  }
  w2 = wirelist;
  while (w2) {
    if (!strcmp(w2->key, w->key))
      dprintf(findanyidx(w2->sock), "----- %s %s '%s'.\n",
              dcc[findanyidx(w->sock)].nick, WIRE_JOINED, w2->key);
    w2 = w2->next;
  }
  w2 = wirelist;
  while (w2) {                  /* Is someone using this key here already? */
    if (w2 != w)
      if (!strcmp(w2->key, w->key))
        break;
    w2 = w2->next;
  }
  if (!w2) {                    /* Someone else is NOT using this key, so
                                 * we add a bind */
    wire_bot[0].name = wirecmd;
    wire_bot[0].flags = "";
    wire_bot[0].func = (IntFunc) wire_filter;
    add_builtins(H_bot, wire_bot);
  }
}

static void wire_leave(int sock)
{
  char wirecmd[513];
  char wiremsg[513];
  char wiretmp[513];
  char *enctmp;
  wire_list *w = wirelist;
  wire_list *w2 = wirelist;
  wire_list *wlast = wirelist;

  while (w) {
    if (w->sock == sock)
      break;
    w = w->next;
  }
  if (!w)
    return;
  enctmp = encrypt_string(w->key, "wire");
  strcpy(wirecmd, enctmp);
  nfree(enctmp);
  sprintf(wiretmp, "%s left the wire.", dcc[findanyidx(w->sock)].nick);
  enctmp = encrypt_string(w->key, wiretmp);
  strcpy(wiremsg, enctmp);
  nfree(enctmp);
  {
    char x[1024];

    simple_sprintf(x, "!wire%s %s", wirecmd, botnetnick);
    botnet_send_zapf_broad(-1, botnetnick, x, wiremsg);
  }
  w2 = wirelist;
  while (w2) {
    if (w2->sock != sock && !strcmp(w2->key, w->key)) {
      dprintf(findanyidx(w2->sock), "----- %s %s\n",
              dcc[findanyidx(w->sock)].nick, WIRE_LEFT);
    }
    w2 = w2->next;
  }
  /* Check to see if someone else is using this wire key.
   * If so, then don't remove the wire filter binding.
   */
  w2 = wirelist;
  while (w2) {
    if (w2 != w && !strcmp(w2->key, w->key))
      break;
    w2 = w2->next;
  }
  if (!w2) {                    /* Someone else is NOT using this key */
    wire_bot[0].name = wirecmd;
    wire_bot[0].flags = "";
    wire_bot[0].func = (IntFunc) wire_filter;
    rem_builtins(H_bot, wire_bot);
  }
  w2 = wirelist;
  wlast = 0;
  while (w2) {
    if (w2 == w)
      break;
    wlast = w2;
    w2 = w2->next;
  }
  if (wlast) {
    if (w->next)
      wlast->next = w->next;
    else
      wlast->next = 0;
  } else if (!w->next)
    wirelist = 0;
  else
    wirelist = w->next;
  nfree(w->crypt);
  nfree(w->key);
  nfree(w);
}

static char *cmd_putwire(int idx, char *message)
{
  wire_list *w = wirelist;
  wire_list *w2 = wirelist;
  int wiretype;
  char wirecmd[512];
  char wiremsg[512];
  char wiretmp[512];
  char wiretmp2[512];
  char *enctmp;

  while (w) {
    if (w->sock == dcc[idx].sock)
      break;
    w = w->next;
  }
  if (!w)
    return "";
  if (!message[1])
    return "";
  if ((strlen(message) > 3) && !strncmp(&message[1], "me", 2) &&
      (message[3] == ' ')) {
    sprintf(wiretmp2, "!%s@%s", dcc[idx].nick, botnetnick);
    enctmp = encrypt_string(w->key, &message[3]);
    wiretype = 1;
  } else {
    sprintf(wiretmp2, "%s@%s", dcc[idx].nick, botnetnick);
    enctmp = encrypt_string(w->key, &message[1]);
    wiretype = 0;
  }
  strcpy(wiremsg, enctmp);
  nfree(enctmp);
  enctmp = encrypt_string(w->key, "wire");
  strcpy(wiretmp, enctmp);
  nfree(enctmp);
  sprintf(wirecmd, "!wire%s", wiretmp);
  sprintf(wiretmp, "%s %s", wiretmp2, wiremsg);
  botnet_send_zapf_broad(-1, botnetnick, wirecmd, wiretmp);
  sprintf(wiretmp, "%s%s", wiretype ? "!" : "", dcc[findanyidx(w->sock)].nick);
  while (w2) {
    if (!strcmp(w2->key, w->key))
      wire_display(findanyidx(w2->sock), w2->key, wiretmp, wiremsg);
    w2 = w2->next;
  }
  return "";
}

/* a report on the module status */
static void wire_report(int idx, int details)
{
  if (details) {
    int count = 0, size = wire_expmem();
    wire_list *w = wirelist;

    while (w) {
      count++;
      w = w->next;
    }

    dprintf(idx, "    %d wire%s\n", count, (count != 1) ? "s" : "");
    dprintf(idx, "    Using %d byte%s of memory\n", size,
            (size != 1) ? "s" : "");
  }
}

static cmd_t wire_dcc[] = {
  {"wire",     "",   (IntFunc) cmd_wire,     NULL},
  {"onwire",   "",   (IntFunc) cmd_onwire,   NULL},
  {"wirelist", "n",  (IntFunc) cmd_wirelist, NULL},
  {NULL,       NULL, NULL,                    NULL}
};

static cmd_t wire_chof[] = {
  {"*", "",    (IntFunc) chof_wire, "wire:chof"},
  {NULL, NULL, NULL,                        NULL}
};

static cmd_t wire_filt[] = {
  {";*", "",   (IntFunc) cmd_putwire, "wire:filt"},
  {NULL, NULL, NULL,                          NULL}
};

static char *wire_close()
{
  wire_list *w = wirelist;
  char wiretmp[512];
  char *enctmp;
  p_tcl_bind_list H_temp;

  /* Remove any current wire encrypt bindings for now, don't worry
   * about duplicate unbinds.
   */
  while (w) {
    enctmp = encrypt_string(w->key, "wire");
    sprintf(wiretmp, "!wire%s", enctmp);
    nfree(enctmp);
    wire_bot[0].name = wiretmp;
    wire_bot[0].flags = "";
    wire_bot[0].func = (IntFunc) wire_filter;
    rem_builtins(H_bot, wire_bot);
    w = w->next;
  }
  w = wirelist;
  while (w && w->sock) {
    dprintf(findanyidx(w->sock), "----- %s\n", WIRE_UNLOAD);
    dprintf(findanyidx(w->sock), "----- %s\n", WIRE_NOLONGERWIRED);
    wire_leave(w->sock);
    w = wirelist;
  }
  rem_help_reference("wire.help");
  rem_builtins(H_dcc, wire_dcc);
  H_temp = find_bind_table("filt");
  rem_builtins(H_temp, wire_filt);
  H_temp = find_bind_table("chof");
  rem_builtins(H_temp, wire_chof);
  del_lang_section("wire");
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *wire_start();

static Function wire_table[] = {
  (Function) wire_start,
  (Function) wire_close,
  (Function) wire_expmem,
  (Function) wire_report,
};

char *wire_start(Function *global_funcs)
{
  p_tcl_bind_list H_temp;

  global = global_funcs;

  module_register(MODULE_NAME, wire_table, 2, 0);
  if (!module_depend(MODULE_NAME, "eggdrop", 106, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.6.0 or later.";
  }

  if (!(encryption_funcs = module_depend(MODULE_NAME, "encryption", 2, 1))) {
    module_undepend(MODULE_NAME);
    return "This module requires an encryption module.";
  }

  add_help_reference("wire.help");
  add_builtins(H_dcc, wire_dcc);
  H_temp = find_bind_table("filt");
  add_builtins(H_filt, wire_filt);
  H_temp = find_bind_table("chof");
  add_builtins(H_chof, wire_chof);
  wirelist = NULL;
  add_lang_section("wire");
  return NULL;
}
