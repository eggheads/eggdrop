/* 
 * console.c - part of console.mod
 * saved console settings based on console.tcl by
 * cmwagner/billyjoe/D. Senso
 */

#define MAKING_CONSOLE
#define MODULE_NAME "console"
#include "../module.h"
#include <stdlib.h>

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

static int console_unpack(struct userrec *u, struct user_entry *e)
{
  context;
  if (e->name) {
    char *p, *w, *o;
    struct console_info *i;

    p = e->u.list->extra;
    e->u.list->extra = NULL;
    list_type_kill(e->u.list);
    e->u.extra = i = user_malloc(sizeof(struct console_info));

    o = p;
    w = newsplit(&p);
    i->channel = user_malloc(strlen(w) + 1);
    strcpy(i->channel, w);
    w = newsplit(&p);
    i->conflags = logmodes(w);
    w = newsplit(&p);
    i->stripflags = stripmodes(w);
    w = newsplit(&p);
    i->echoflags = (w[0] == '1') ? 1 : 0;
    w = newsplit(&p);
    i->page = atoi(w);
    w = newsplit(&p);
    i->conchan = atoi(w);
    nfree(o);
  }
  return 1;
}

static int console_pack(struct userrec *u, struct user_entry *e)
{
  if (!e->name) {
    char work[1024];
    struct console_info *i = e->u.extra;
    int c = simple_sprintf(work, "%s %s %s %d %d %d",
			   i->channel, masktype(i->conflags),
			   stripmasktype(i->stripflags), i->echoflags,
			   i->page, i->conchan);

    e->u.list = user_malloc(sizeof(struct list_type));

    e->u.list->next = NULL;
    e->u.list->extra = user_malloc(c + 1);
    strcpy(e->u.list->extra, work);
    nfree(i->channel);
    nfree(i);
  }
  return 1;
}

static int console_kill(struct user_entry *e)
{
  struct console_info *i = e->u.extra;

  context;
  nfree(i->channel);
  nfree(i);
  nfree(e);
  return 1;
}

static int console_write_userfile(FILE * f, struct userrec *u, struct user_entry *e)
{
  struct console_info *i = e->u.extra;

  context;
  if (fprintf(f, "--CONSOLE %s %s %s %d %d %d\n",
	      i->channel, masktype(i->conflags),
	      stripmasktype(i->stripflags), i->echoflags,
	      i->page, i->conchan) == EOF)
    return 0;
  return 1;
}

static int console_set(struct userrec *u, struct user_entry *e, void *buf)
{
  e->u.extra = buf;
  context;
  return 1;
}

static int console_tcl_get(Tcl_Interp * irp, struct userrec *u,
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

int console_tcl_set(Tcl_Interp * irp, struct userrec *u,
		    struct user_entry *e, int argc, char **argv)
{
  struct console_info *i = e->u.extra;
  int l;

  BADARGS(4, 9, " handle CONSOLE channel flags strip echo page conchan");
  if (!i) {
    i = user_malloc(sizeof(struct console_info));
    bzero(i, sizeof(struct console_info));
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
  return TCL_OK;
}

int console_expmem(struct user_entry *e)
{
  struct console_info *i = e->u.extra;

  context;
  return sizeof(struct console_info) + strlen(i->channel) + 1;
}

void console_display(int idx, struct user_entry *e)
{
  struct console_info *i = e->u.extra;

  context;
  if (dcc[idx].user && (dcc[idx].user->flags & USER_MASTER)) {
    dprintf(idx, "  Saved Console Settings:\n");
    dprintf(idx, "    Channel: %s\n", i->channel);
    dprintf(idx, "    Console flags: %s, Strip flags: %s, Echo: %s\n",
	    masktype(i->conflags), stripmasktype(i->stripflags),
	    i->echoflags ? "yes" : "no");
    dprintf(idx, "    Page setting: %d, Console channel: %s%d\n",
	    i->page, (i->conchan < 100000) ? "" : "*", i->conchan % 100000);
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

static struct user_entry_type USERENTRY_CONSOLE =
{
  0,				/* always 0 ;) */
  0,
  console_dupuser,
  console_unpack,
  console_pack,
  console_write_userfile,
  console_kill,
  0,
  console_set,
  console_tcl_get,
  console_tcl_set,
  console_expmem,
  console_display,
  "CONSOLE"
};

static int console_chon(char *handle, int idx)
{
  struct console_info *i = get_user(&USERENTRY_CONSOLE, dcc[idx].user);

  if (dcc[idx].type == &DCC_CHAT) {
    if (i) {
      if (i->channel && i->channel[0])
	strcpy(dcc[idx].u.chat->con_chan, i->channel);
      if (i->conflags)
	dcc[idx].u.chat->con_flags = i->conflags;
      if (i->stripflags)
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
	    botnet_send_chan(-1, botnetnick, NULL,
			     dcc[idx].u.chat->channel, x);
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
    bzero(i, sizeof(struct console_info));
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
    dprintf(idx, "Saved Console your Settings:\n");
    dprintf(idx, "  Channel: %s\n", i->channel);
    dprintf(idx, "  Console flags: %s, Strip flags: %s, Echo: %s\n",
	    masktype(i->conflags), stripmasktype(i->stripflags),
	    i->echoflags ? "yes" : "no");
    dprintf(idx, "  Page setting: %d, Console channel: %d\n",
	    i->page, i->conchan);
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

static tcl_ints myints[] =
{
  {"console-autosave", &console_autosave, 0},
  {"force-channel", &force_channel, 0},
  {"info-party", &info_party, 0},
  {0, 0, 0}
};

static cmd_t mychon[] =
{
  {"*", "", console_chon, "console:chon"},
  {0, 0, 0, 0}
};

static cmd_t mydcc[] =
{
  {"store", "", console_store, NULL},
  {0, 0, 0, 0}
};

static char *console_close()
{
  context;
  rem_builtins(H_chon, mychon);
  rem_builtins(H_dcc, mydcc);
  rem_tcl_ints(myints);
  rem_help_reference("console.help");
  del_entry_type(&USERENTRY_CONSOLE);
  module_undepend(MODULE_NAME);
  return NULL;
}

char *console_start();

static Function console_table[] =
{
  (Function) console_start,
  (Function) console_close,
  (Function) 0,
  (Function) 0,
  (Function) console_dostore,
};

char *console_start(Function * global_funcs)
{
  global = global_funcs;

  context;
  module_register(MODULE_NAME, console_table, 1, 1);
  if (!module_depend(MODULE_NAME, "eggdrop", 103, 0))
    return "This module requires eggdrop1.3.0 or later";
  add_builtins(H_chon, mychon);
  add_builtins(H_dcc, mydcc);
  add_tcl_ints(myints);
  add_help_reference("console.help");
  USERENTRY_CONSOLE.get = def_get;
  add_entry_type(&USERENTRY_CONSOLE);
  return NULL;
}
