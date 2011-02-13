/*
 * console.c -- part of console.mod
 *   saved console settings based on console.tcl
 *   by cmwagner/billyjoe/D. Senso
 *
 * $Id: console.c,v 1.39 2011/02/13 14:19:33 simple Exp $
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

#define MODULE_NAME "console"
#define MAKING_CONSOLE

#include "src/mod/module.h"
#include <stdlib.h>
#include "console.h"

static Function *global = NULL;
static int console_autosave = 0;
static int force_channel = 0;
static int info_party = 0;

struct console_info {
  char *channel;
  int conflags;
  int stripflags;
  int echoflags;
  int page;
  int conchan;
};

static struct user_entry_type USERENTRY_CONSOLE;


static int console_unpack(struct userrec *u, struct user_entry *e)
{
  struct console_info *ci = user_malloc(sizeof(struct console_info));
  char *par, *arg;

  par = e->u.list->extra;
  arg = newsplit(&par);
  ci->channel = user_malloc(strlen(arg) + 1);
  strcpy(ci->channel, arg);
  arg = newsplit(&par);
  ci->conflags = logmodes(arg);
  arg = newsplit(&par);
  ci->stripflags = stripmodes(arg);
  arg = newsplit(&par);
  ci->echoflags = (arg[0] == '1') ? 1 : 0;
  arg = newsplit(&par);
  ci->page = atoi(arg);
  arg = newsplit(&par);
  ci->conchan = atoi(arg);
  list_type_kill(e->u.list);
  e->u.extra = ci;
  return 1;
}

static int console_pack(struct userrec *u, struct user_entry *e)
{
  char work[1024];
  struct console_info *ci;
  int l;

  ci = (struct console_info *) e->u.extra;

  l = simple_sprintf(work, "%s %s %s %d %d %d",
                     ci->channel, masktype(ci->conflags),
                     stripmasktype(ci->stripflags), ci->echoflags,
                     ci->page, ci->conchan);

  e->u.list = user_malloc(sizeof(struct list_type));
  e->u.list->next = NULL;
  e->u.list->extra = user_malloc(l + 1);
  strcpy(e->u.list->extra, work);

  nfree(ci->channel);
  nfree(ci);
  return 1;
}

static int console_kill(struct user_entry *e)
{
  struct console_info *i = e->u.extra;

  nfree(i->channel);
  nfree(i);
  nfree(e);
  return 1;
}

static int console_write_userfile(FILE *f, struct userrec *u,
                                  struct user_entry *e)
{
  struct console_info *i = e->u.extra;

  if (fprintf(f, "--CONSOLE %s %s %s %d %d %d\n",
              i->channel, masktype(i->conflags),
              stripmasktype(i->stripflags), i->echoflags,
              i->page, i->conchan) == EOF)
    return 0;
  return 1;
}

static int console_set(struct userrec *u, struct user_entry *e, void *buf)
{
  struct console_info *ci = (struct console_info *) e->u.extra;

  if (!ci && !buf)
    return 1;

  if (ci != buf) {
    if (ci) {
      nfree(ci->channel);
      nfree(ci);
    }
    ci = e->u.extra = buf;
  }

  /* Note: Do not share console info */
  return 1;
}

static int console_tcl_get(Tcl_Interp *irp, struct userrec *u,
                           struct user_entry *e, int argc, char **argv)
{
  char work[1024];
  struct console_info *i = e->u.extra;

  simple_sprintf(work, "%s %s %s %d %d %d",
                 i->channel, masktype(i->conflags),
                 stripmasktype(i->stripflags), i->echoflags,
                 i->page, i->conchan);
  Tcl_AppendResult(irp, work, NULL);
  return TCL_OK;
}

static int console_tcl_set(Tcl_Interp *irp, struct userrec *u,
                           struct user_entry *e, int argc, char **argv)
{
  struct console_info *i = e->u.extra;
  int l;

  BADARGS(4, 9, " handle CONSOLE channel flags strip echo page conchan");

  if (!i) {
    i = user_malloc(sizeof(struct console_info));
    egg_bzero(i, sizeof(struct console_info));
  }
  if (i->channel)
    nfree(i->channel);
  l = strlen(argv[3]);
  if (l > 80)
    l = 80;
  i->channel = user_malloc(l + 1);
  strncpy(i->channel, argv[3], l);
  i->channel[l] = 0;
  if (argc > 4) {
    i->conflags = logmodes(argv[4]);
    if (argc > 5) {
      i->stripflags = stripmodes(argv[5]);
      if (argc > 6) {
        i->echoflags = (argv[6][0] == '1') ? 1 : 0;
        if (argc > 7) {
          i->page = atoi(argv[7]);
          if (argc > 8)
            i->conchan = atoi(argv[8]);
        }
      }
    }
  }
  set_user(&USERENTRY_CONSOLE, u, i);
  return TCL_OK;
}

static int console_expmem(struct user_entry *e)
{
  struct console_info *i = e->u.extra;

  return sizeof(struct console_info) + strlen(i->channel) + 1;
}

static void console_display(int idx, struct user_entry *e)
{
  struct console_info *i = e->u.extra;

  if (dcc[idx].user && (dcc[idx].user->flags & USER_MASTER)) {
    dprintf(idx, "  %s\n", CONSOLE_SAVED_SETTINGS);
    dprintf(idx, "    %s %s\n", CONSOLE_CHANNEL, i->channel);
    dprintf(idx, "    %s %s, %s %s, %s %s\n", CONSOLE_FLAGS,
            masktype(i->conflags), CONSOLE_STRIPFLAGS,
            stripmasktype(i->stripflags), CONSOLE_ECHO,
            i->echoflags ? CONSOLE_YES : CONSOLE_NO);
    dprintf(idx, "    %s %d, %s %s%d\n", CONSOLE_PAGE_SETTING, i->page,
            CONSOLE_CHANNEL2, (i->conchan < GLOBAL_CHANS) ? "" : "*",
            i->conchan % GLOBAL_CHANS);
  }
}

static int console_dupuser(struct userrec *new, struct userrec *old,
                           struct user_entry *e)
{
  struct console_info *i = e->u.extra, *j;

  j = user_malloc(sizeof(struct console_info));
  my_memcpy(j, i, sizeof(struct console_info));

  j->channel = user_malloc(strlen(i->channel) + 1);
  strcpy(j->channel, i->channel);
  return set_user(e->type, new, j);
}

static struct user_entry_type USERENTRY_CONSOLE = {
  NULL,                         /* always 0 ;) */
  NULL,
  console_dupuser,
  console_unpack,
  console_pack,
  console_write_userfile,
  console_kill,
  NULL,
  console_set,
  console_tcl_get,
  console_tcl_set,
  console_expmem,
  console_display,
  "CONSOLE"
};

static int console_chon(char *handle, int idx)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };
  struct console_info *i = get_user(&USERENTRY_CONSOLE, dcc[idx].user);

  if (dcc[idx].type == &DCC_CHAT) {
    if (i) {
      if (i->channel && i->channel[0])
        strcpy(dcc[idx].u.chat->con_chan, i->channel);
      get_user_flagrec(dcc[idx].user, &fr, i->channel);
      dcc[idx].u.chat->con_flags = check_conflags(&fr, i->conflags);
      dcc[idx].u.chat->strip_flags = i->stripflags;
      if (i->echoflags)
        dcc[idx].status |= STAT_ECHO;
      else
        dcc[idx].status &= ~STAT_ECHO;
      if (i->page) {
        dcc[idx].status |= STAT_PAGE;
        dcc[idx].u.chat->max_line = i->page;
        if (!dcc[idx].u.chat->line_count)
          dcc[idx].u.chat->current_lines = 0;
      }
      dcc[idx].u.chat->channel = i->conchan;
    } else if (force_channel > -1)
      dcc[idx].u.chat->channel = force_channel;
    if ((dcc[idx].u.chat->channel >= 0) &&
        (dcc[idx].u.chat->channel < GLOBAL_CHANS)) {
      botnet_send_join_idx(idx, -1);
      check_tcl_chjn(botnetnick, dcc[idx].nick, dcc[idx].u.chat->channel,
                     geticon(idx), dcc[idx].sock, dcc[idx].host);
    }
    if (info_party) {
      char *p = get_user(&USERENTRY_INFO, dcc[idx].user);

      if (p) {
        if (dcc[idx].u.chat->channel >= 0) {
          char x[1024];

          chanout_but(-1, dcc[idx].u.chat->channel,
                      "*** [%s] %s\n", dcc[idx].nick, p);
          simple_sprintf(x, "[%s] %s", dcc[idx].nick, p);
          botnet_send_chan(-1, botnetnick, NULL, dcc[idx].u.chat->channel, x);
        }
      }
    }
  }
  return 0;
}

static int console_store(struct userrec *u, int idx, char *par)
{
  struct console_info *i = get_user(&USERENTRY_CONSOLE, u);

  if (!i) {
    i = user_malloc(sizeof(struct console_info));
    egg_bzero(i, sizeof(struct console_info));
  }
  if (i->channel)
    nfree(i->channel);
  i->channel = user_malloc(strlen(dcc[idx].u.chat->con_chan) + 1);
  strcpy(i->channel, dcc[idx].u.chat->con_chan);
  i->conflags = dcc[idx].u.chat->con_flags;
  i->stripflags = dcc[idx].u.chat->strip_flags;
  i->echoflags = (dcc[idx].status & STAT_ECHO) ? 1 : 0;
  if (dcc[idx].status & STAT_PAGE)
    i->page = dcc[idx].u.chat->max_line;
  else
    i->page = 0;
  i->conchan = dcc[idx].u.chat->channel;
  if (par) {
    dprintf(idx, "%s\n", CONSOLE_SAVED_SETTINGS2);
    dprintf(idx, "  %s %s\n", CONSOLE_CHANNEL, i->channel);
    dprintf(idx, "  %s %s, %s %s, %s %s\n", CONSOLE_FLAGS,
            masktype(i->conflags), CONSOLE_STRIPFLAGS,
            stripmasktype(i->stripflags), CONSOLE_ECHO,
            i->echoflags ? CONSOLE_YES : CONSOLE_NO);
    dprintf(idx, "  %s %d, %s %d\n", CONSOLE_PAGE_SETTING, i->page,
            CONSOLE_CHANNEL2, i->conchan);
  }
  set_user(&USERENTRY_CONSOLE, u, i);
  return 0;
}

/* cmds.c:cmd_console calls this, better than chof bind - drummer,07/25/1999 */
static int console_dostore(int idx)
{
  if (console_autosave)
    console_store(dcc[idx].user, idx, NULL);
  return 0;
}

static tcl_ints myints[] = {
  {"console-autosave", &console_autosave, 0},
  {"force-channel",    &force_channel,    0},
  {"info-party",       &info_party,       0},
  {NULL,               NULL,              0}
};

static cmd_t mychon[] = {
  {"*",  "",   console_chon, "console:chon"},
  {NULL, NULL, NULL,                   NULL}
};

static cmd_t mydcc[] = {
  {"store", "",   console_store, NULL},
  {NULL,    NULL, NULL,          NULL}
};

static char *console_close()
{
  rem_builtins(H_chon, mychon);
  rem_builtins(H_dcc, mydcc);
  rem_tcl_ints(myints);
  rem_help_reference("console.help");
  del_entry_type(&USERENTRY_CONSOLE);
  del_lang_section("console");
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *console_start();

static Function console_table[] = {
  (Function) console_start,
  (Function) console_close,
  (Function) NULL,
  (Function) NULL,
  (Function) console_dostore,
};

char *console_start(Function *global_funcs)
{
  global = global_funcs;

  module_register(MODULE_NAME, console_table, 1, 2);
  if (!module_depend(MODULE_NAME, "eggdrop", 106, 20)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.6.20 or later.";
  }
  add_builtins(H_chon, mychon);
  add_builtins(H_dcc, mydcc);
  add_tcl_ints(myints);
  add_help_reference("console.help");
  USERENTRY_CONSOLE.get = def_get;
  add_entry_type(&USERENTRY_CONSOLE);
  add_lang_section("console");
  return NULL;
}
