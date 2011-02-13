/*
 * assoc.c -- part of assoc.mod
 *   the assoc code, moved here mainly from botnet.c for module work
 *
 * $Id: assoc.c,v 1.37 2011/02/13 14:19:33 simple Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
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

#define MODULE_NAME "assoc"
#define MAKING_ASSOC

#include "src/mod/module.h"
#include "src/tandem.h"
#include <stdlib.h>
#include "assoc.h"

#undef global
static Function *global = NULL;

/* Keep track of channel associations */
typedef struct assoc_t_ {
  char name[21];
  unsigned int channel;
  struct assoc_t_ *next;
} assoc_t;

/* Channel name-number associations */
static assoc_t *assoc;

static void botnet_send_assoc(int idx, int chan, char *nick, char *buf)
{
  char x[1024];
  int idx2;

  simple_sprintf(x, "assoc %D %s %s", chan, nick, buf);
  for (idx2 = 0; idx2 < dcc_total; idx2++)
    if ((dcc[idx2].type == &DCC_BOT) && (idx2 != idx) &&
        (b_numver(idx2) >= NEAT_BOTNET) &&
        !(bot_flags(dcc[idx2].user) & BOT_ISOLATE))
      botnet_send_zapf(idx2, botnetnick, dcc[idx2].nick, x);
}

static int assoc_expmem()
{
  assoc_t *a;
  int size = 0;

  for (a = assoc; a; a = a->next)
    size += sizeof(assoc_t);
  return size;
}

static void link_assoc(char *bot, char *via)
{
  char x[1024];

  if (!egg_strcasecmp(via, botnetnick)) {
    int idx = nextbot(bot);
    assoc_t *a;

    if (!(bot_flags(dcc[idx].user) & BOT_ISOLATE)) {
      for (a = assoc; a && a->name[0]; a = a->next) {
        simple_sprintf(x, "assoc %D %s %s", (int) a->channel, botnetnick,
                       a->name);
        botnet_send_zapf(idx, botnetnick, dcc[idx].nick, x);
      }
    }
  }
}

static void kill_assoc(int chan)
{
  assoc_t *a = assoc, *last = NULL;

  while (a) {
    if (a->channel == chan) {
      if (last != NULL)
        last->next = a->next;
      else
        assoc = a->next;
      nfree(a);
      a = NULL;
    } else {
      last = a;
      a = a->next;
    }
  }
}

static void kill_all_assoc()
{
  assoc_t *a, *x;

  for (a = assoc; a; a = x) {
    x = a->next;
    nfree(a);
  }
  assoc = NULL;
}

static void add_assoc(char *name, int chan)
{
  assoc_t *a, *b, *old = NULL;

  for (a = assoc; a; a = a->next) {
    if (name[0] != 0 && !egg_strcasecmp(a->name, name)) {
      kill_assoc(a->channel);
      add_assoc(name, chan);
      return;
    }
    if (a->channel == chan) {
      strncpyz(a->name, name, sizeof a->name);
      return;
    }
  }
  /* Add in numerical order */
  for (a = assoc; a; old = a, a = a->next) {
    if (a->channel > chan) {
      b = nmalloc(sizeof *b);
      b->next = a;
      b->channel = chan;
      strncpyz(b->name, name, sizeof b->name);
      if (old == NULL)
        assoc = b;
      else
        old->next = b;
      return;
    }
  }
  /* Add at the end */
  b = nmalloc(sizeof *b);
  b->next = NULL;
  b->channel = chan;
  strncpyz(b->name, name, sizeof b->name);
  if (old == NULL)
    assoc = b;
  else
    old->next = b;
}

static int get_assoc(char *name)
{
  assoc_t *a;

  for (a = assoc; a; a = a->next)
    if (!egg_strcasecmp(a->name, name))
      return a->channel;
  return -1;
}

static char *get_assoc_name(int chan)
{
  assoc_t *a;

  for (a = assoc; a; a = a->next)
    if (a->channel == chan)
      return a->name;
  return NULL;
}

static void dump_assoc(int idx)
{
  assoc_t *a = assoc;

  if (a == NULL) {
    dprintf(idx, "%s\n", ASSOC_NOCHNAMES);
    return;
  }
  dprintf(idx, " %s  %s\n", ASSOC_CHAN, ASSOC_NAME);
  for (; a && a->name[0]; a = a->next)
    dprintf(idx, "%c%5d %s\n", (a->channel < GLOBAL_CHANS) ? ' ' : '*',
            a->channel % GLOBAL_CHANS, a->name);
  return;
}

static int cmd_assoc(struct userrec *u, int idx, char *par)
{
  char *num;
  int chan;

  if (!par[0]) {
    putlog(LOG_CMDS, "*", "#%s# assoc", dcc[idx].nick);
    dump_assoc(idx);
  } else if (!u || !(u->flags & USER_BOTMAST))
    dprintf(idx, "%s", MISC_NOSUCHCMD);
  else {
    num = newsplit(&par);
    if (num[0] == '*') {
      chan = GLOBAL_CHANS + atoi(num + 1);
      if ((chan < GLOBAL_CHANS) || (chan > 199999)) {
        dprintf(idx, "%s\n", ASSOC_LCHAN_RANGE);
        return 0;
      }
    } else {
      chan = atoi(num);
      if (chan == 0) {
        dprintf(idx, "%s\n", ASSOC_PARTYLINE);
        return 0;
      } else if ((chan < 1) || (chan >= GLOBAL_CHANS)) {
        dprintf(idx, "%s\n", ASSOC_CHAN_RANGE);
        return 0;
      }
    }
    if (!par[0]) {
      /* Remove an association */
      if (get_assoc_name(chan) == NULL) {
        dprintf(idx, ASSOC_NONAME_CHAN, (chan < GLOBAL_CHANS) ? "" : "*",
                chan % GLOBAL_CHANS);
        return 0;
      }
      kill_assoc(chan);
      putlog(LOG_CMDS, "*", "#%s# assoc %d", dcc[idx].nick, chan);
      dprintf(idx, ASSOC_REMNAME_CHAN, (chan < GLOBAL_CHANS) ? "" : "*",
              chan % GLOBAL_CHANS);
      chanout_but(-1, chan, ASSOC_REMOUT_CHAN, dcc[idx].nick);
      if (chan < GLOBAL_CHANS)
        botnet_send_assoc(-1, chan, dcc[idx].nick, "0");
      return 0;
    }
    if (strlen(par) > 20) {
      dprintf(idx, "%s\n", ASSOC_CHNAME_TOOLONG);
      return 0;
    }
    if ((par[0] >= '0') && (par[0] <= '9')) {
      dprintf(idx, "%s\n", ASSOC_CHNAME_FIRSTCHAR);
      return 0;
    }
    add_assoc(par, chan);
    putlog(LOG_CMDS, "*", "#%s# assoc %d %s", dcc[idx].nick, chan, par);
    dprintf(idx, ASSOC_NEWNAME_CHAN, (chan < GLOBAL_CHANS) ? "" : "*",
            chan % GLOBAL_CHANS, par);
    chanout_but(-1, chan, ASSOC_NEWOUT_CHAN, dcc[idx].nick, par);
    if (chan < GLOBAL_CHANS)
      botnet_send_assoc(-1, chan, dcc[idx].nick, par);
  }
  return 0;
}

static int tcl_killassoc STDVAR
{
  int chan;

  BADARGS(2, 2, " chan");

  if (argv[1][0] == '&')
    kill_all_assoc();
  else {
    chan = atoi(argv[1]);
    if ((chan < 1) || (chan > 199999)) {
      Tcl_AppendResult(irp, "invalid channel #", NULL);
      return TCL_ERROR;
    }
    kill_assoc(chan);

    botnet_send_assoc(-1, chan, "*script*", "0");
  }
  return TCL_OK;
}

static int tcl_assoc STDVAR
{
  int chan;
  char name[21], *p;

  BADARGS(2, 3, " chan ?name?");

  if ((argc == 2) && ((argv[1][0] < '0') || (argv[1][0] > '9'))) {
    chan = get_assoc(argv[1]);
    if (chan == -1)
      Tcl_AppendResult(irp, "", NULL);
    else {
      simple_sprintf(name, "%d", chan);
      Tcl_AppendResult(irp, name, NULL);
    }
    return TCL_OK;
  }
  chan = atoi(argv[1]);
  if ((chan < 1) || (chan > 199999)) {
    Tcl_AppendResult(irp, "invalid channel #", NULL);
    return TCL_ERROR;
  }
  if (argc == 3) {
    strncpy(name, argv[2], 20);
    name[20] = 0;
    add_assoc(name, chan);
    botnet_send_assoc(-1, chan, "*script*", name);
  }
  p = get_assoc_name(chan);
  if (p == NULL)
    name[0] = 0;
  else
    strcpy(name, p);
  Tcl_AppendResult(irp, name, NULL);
  return TCL_OK;
}

static void zapf_assoc(char *botnick, char *code, char *par)
{
  int idx = nextbot(botnick);
  char *s, *s1, *nick;
  int linking = 0, chan;

  if ((idx >= 0) && !(bot_flags(dcc[idx].user) & BOT_ISOLATE)) {
    if (!egg_strcasecmp(dcc[idx].nick, botnick))
      linking = b_status(idx) & STAT_LINKING;
    s = newsplit(&par);
    chan = base64_to_int(s);
    if ((chan > 0) || (chan < GLOBAL_CHANS)) {
      nick = newsplit(&par);
      s1 = get_assoc_name(chan);
      if (linking && ((s1 == NULL) || (s1[0] == 0) ||
          (((intptr_t) get_user(find_entry_type("BOTFL"),
          dcc[idx].user) & BOT_HUB)))) {
        add_assoc(par, chan);
        botnet_send_assoc(idx, chan, nick, par);
        chanout_but(-1, chan, ASSOC_CHNAME_NAMED, nick, par);
      } else if (par[0] == '0') {
        kill_assoc(chan);
        chanout_but(-1, chan, ASSOC_CHNAME_REM, botnick, nick);
      } else if (get_assoc(par) != chan) {
        /* New one i didn't know about -- pass it on */
        s1 = get_assoc_name(chan);
        add_assoc(par, chan);
        chanout_but(-1, chan, ASSOC_CHNAME_NAMED2, botnick, nick, par);
      }
    }
  }
}

static void assoc_report(int idx, int details)
{
  if (details) {
    assoc_t *a;
    int size = 0, count = 0;

    for (a = assoc; a; a = a->next) {
      count++;
      size += sizeof(assoc_t);
    }

    dprintf(idx, "    %d current association%s\n", count,
            (count != 1) ? "s" : "");
    dprintf(idx, "    Using %d byte%s of memory\n", size,
            (size != 1) ? "s" : "");
  }
}

static cmd_t mydcc[] = {
  {"assoc", "",   cmd_assoc, NULL},
  {NULL,    NULL, NULL,      NULL}
};

static cmd_t mybot[] = {
  {"assoc", "",   (IntFunc) zapf_assoc, NULL},
  {NULL,    NULL, NULL,                  NULL}
};

static cmd_t mylink[] = {
  {"*",  "",   (IntFunc) link_assoc, "assoc"},
  {NULL, NULL, NULL,                     NULL}
};

static tcl_cmds mytcl[] = {
  {"assoc",         tcl_assoc},
  {"killassoc", tcl_killassoc},
  {NULL,                 NULL}
};

static char *assoc_close()
{
  kill_all_assoc();
  rem_builtins(H_dcc, mydcc);
  rem_builtins(H_bot, mybot);
  rem_builtins(H_link, mylink);
  rem_tcl_commands(mytcl);
  rem_help_reference("assoc.help");
  del_lang_section("assoc");
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *assoc_start();

static Function assoc_table[] = {
  (Function) assoc_start,
  (Function) assoc_close,
  (Function) assoc_expmem,
  (Function) assoc_report,
};

char *assoc_start(Function *global_funcs)
{
  global = global_funcs;

  module_register(MODULE_NAME, assoc_table, 2, 0);
  if (!module_depend(MODULE_NAME, "eggdrop", 106, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.6.0 or later.";
  }
  assoc = NULL;
  add_builtins(H_dcc, mydcc);
  add_builtins(H_bot, mybot);
  add_builtins(H_link, mylink);
  add_lang_section("assoc");
  add_tcl_commands(mytcl);
  add_help_reference("assoc.help");
  return NULL;
}
