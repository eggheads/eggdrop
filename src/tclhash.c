/*
 * tclhash.c -- rewritten tclhash.c & hash.c, handles (from tclhash.c):
 * bind and unbind
 * checking and triggering the various in-bot bindings
 * listing current bindings
 * adding/removing new binding tables
 * 
 * dprintf'ized, 4feb1996
 * Now includes FREE OF CHARGE everything from hash.c, 'cause they
 * were exporting functions to each other and only for each other.
 * 
 * hash.c -- handles:
 * (non-Tcl) procedure lookups for msg/dcc/file commands
 * (Tcl) binding internal procedures to msg/dcc/file commands
 *
 *  dprintf'ized, 15nov1995
 */
/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

#include "main.h"
#include "chan.h"
#include "users.h"
#include "match.c"

extern Tcl_Interp *interp;
extern struct dcc_t *dcc;
extern struct userrec *userlist;
extern int debug_tcl;
extern int dcc_total;
extern time_t now;

static p_tcl_bind_list bind_table_list;
p_tcl_bind_list H_chat, H_act, H_bcst, H_chon, H_chof;
p_tcl_bind_list H_load, H_unld, H_link, H_disc, H_dcc, H_chjn, H_chpt;
p_tcl_bind_list H_bot, H_time, H_nkch, H_away, H_note, H_filt, H_event;

static int builtin_2char();
static int builtin_3char();
static int builtin_5int();
static int builtin_char();
static int builtin_chpt();
static int builtin_chjn();
static int builtin_idxchar();
static int builtin_charidx();
static int builtin_chat();
static int builtin_dcc();

int expmem_tclhash()
{
  struct tcl_bind_list *p = bind_table_list;
  struct tcl_bind_mask *q;
  tcl_cmd_t *c;
  int tot = 0;

  while (p) {
    tot += sizeof(struct tcl_bind_list);

    for (q = p->first; q; q = q->next) {
      tot += sizeof(struct tcl_bind_mask);

      tot += strlen(q->mask) + 1;
      for (c = q->first; c; c = c->next) {
	tot += sizeof(tcl_cmd_t);
	tot += strlen(c->func_name) + 1;
      }
    }
    p = p->next;
  }
  return tot;
}

extern cmd_t C_dcc[];
static int tcl_bind();

void init_bind()
{
  bind_table_list = NULL;
  context;
  Tcl_CreateCommand(interp, "bind", tcl_bind, (ClientData) 0, NULL);
  Tcl_CreateCommand(interp, "unbind", tcl_bind, (ClientData) 1, NULL);
  H_unld = add_bind_table("unld", HT_STACKABLE, builtin_char);
  H_time = add_bind_table("time", HT_STACKABLE, builtin_5int);
  H_note = add_bind_table("note", 0, builtin_3char);
  H_nkch = add_bind_table("nkch", HT_STACKABLE, builtin_2char);
  H_load = add_bind_table("load", HT_STACKABLE, builtin_char);
  H_link = add_bind_table("link", HT_STACKABLE, builtin_2char);
  H_filt = add_bind_table("filt", HT_STACKABLE, builtin_idxchar);
  H_disc = add_bind_table("disc", HT_STACKABLE, builtin_char);
  H_dcc = add_bind_table("dcc", 0, builtin_dcc);
  H_chpt = add_bind_table("chpt", HT_STACKABLE, builtin_chpt);
  H_chon = add_bind_table("chon", HT_STACKABLE, builtin_charidx);
  H_chof = add_bind_table("chof", HT_STACKABLE, builtin_charidx);
  H_chjn = add_bind_table("chjn", HT_STACKABLE, builtin_chjn);
  H_chat = add_bind_table("chat", HT_STACKABLE, builtin_chat);
  H_bot = add_bind_table("bot", 0, builtin_3char);
  H_bcst = add_bind_table("bcst", HT_STACKABLE, builtin_chat);
  H_away = add_bind_table("away", HT_STACKABLE, builtin_chat);
  H_act = add_bind_table("act", HT_STACKABLE, builtin_chat);
  context;
  H_event = add_bind_table("evnt", HT_STACKABLE, builtin_char);
  add_builtins(H_dcc, C_dcc, 64);
}

void kill_bind()
{
  rem_builtins(H_dcc, C_dcc, 64);
  while (bind_table_list) {
    del_bind_table(bind_table_list);
  }
}

p_tcl_bind_list add_bind_table(char *nme, int flg, Function f)
{
  p_tcl_bind_list p = bind_table_list, o = NULL;

  if (strlen(nme) > 4)
    nme[4] = 0;
  while (p) {
    int v = strcasecmp(p->name, nme);

    if (v == 0)
      /* repeat, just return old value */
      return p;
    /* insert at start of list */
    if (v > 0) {
      break;
    } else {
      o = p;
      p = p->next;
    }
  }
  p = nmalloc(sizeof(struct tcl_bind_list));

  p->first = NULL;
  strcpy(p->name, nme);
  p->flags = flg;
  p->func = f;
  if (o) {
    p->next = o->next;
    o->next = p;
  } else {
    p->next = bind_table_list;
    bind_table_list = p;
  }
  putlog(LOG_DEBUG, "*", "Allocated bind table %s (flags %d)", nme, flg);
  return p;
}

void del_bind_table(p_tcl_bind_list which)
{
  p_tcl_bind_list p = bind_table_list, o = NULL;

  while (p) {
    if (p == which) {
      tcl_cmd_t *tt, *tt1;
      struct tcl_bind_mask *ht, *ht1;

      if (o) {
	o->next = p->next;
      } else {
	bind_table_list = p->next;
      }
      /* cleanup code goes here */
      for (ht = p->first; ht; ht = ht1) {
	ht1 = ht->next;
	for (tt = ht->first; tt; tt = tt1) {
	  tt1 = tt->next;
	  nfree(tt->func_name);
	  nfree(tt);
	}
	nfree(ht->mask);
	nfree(ht);
      }
      putlog(LOG_DEBUG, "*", "De-Allocated bind table %s", p->name);
      nfree(p);
      return;
    }
    o = p;
    p = p->next;
  }
  putlog(LOG_DEBUG, "*", "??? Tried to delete no listed bind table ???");
}

p_tcl_bind_list find_bind_table(char *nme)
{
  p_tcl_bind_list p = bind_table_list;

  while (p) {
    int v = strcasecmp(p->name, nme);

    if (v == 0)
      return p;
    if (v > 0)
      return NULL;
    p = p->next;
  }
  return NULL;
}

static void dump_bind_tables(Tcl_Interp * irp)
{
  p_tcl_bind_list p = bind_table_list;
  int i = 0;

  while (p) {
    if (i)
      Tcl_AppendResult(irp, ", ", NULL);
    else
      i++;
    Tcl_AppendResult(irp, p->name, NULL);
    p = p->next;
  }
}

static int unbind_bind_entry(p_tcl_bind_list typ, char *flags, char *cmd,
			     char *proc)
{
  tcl_cmd_t *tt, *last;
  struct tcl_bind_mask *ma, *ma1 = NULL;

  for (ma = typ->first; ma; ma = ma->next) {
    int i = strcmp(cmd, ma->mask);

    if (!i)
      break;			/* found it! fall out! */
    ma1 = ma;
  }
  if (ma) {
    last = NULL;
    for (tt = ma->first; tt; tt = tt->next) {
      /* if procs are same, erase regardless of flags */
      if (!strcasecmp(tt->func_name, proc)) {
	/* erase it */
	if (last) {
	  last->next = tt->next;
	} else if (tt->next) {
	  ma->first = tt->next;
	} else {
	  if (ma1)
	    ma1->next = ma->next;
	  else
	    typ->first = ma->next;
	  nfree(ma->mask);
	  nfree(ma);
	}
	nfree(tt->func_name);
	nfree(tt);
	return 1;
      }
      last = tt;
    }
  }
  return 0;			/* no match */
}

/* add command (remove old one if necessary) */
static int bind_bind_entry(p_tcl_bind_list typ, char *flags, char *cmd,
			   char *proc)
{
  tcl_cmd_t *tt;
  struct tcl_bind_mask *ma, *ma1 = NULL;

  if (proc[0] == '#') {
    putlog(LOG_MISC, "*", "Note: binding to '#' is obsolete.");
    return 0;
  }
  context;
  for (ma = typ->first; ma; ma = ma->next) {
    int i = strcmp(cmd, ma->mask);

    if (!i)
      break;			/* found it! fall out! */
    ma1 = ma;
  }
  context;
  if (!ma) {
    ma = nmalloc(sizeof(struct tcl_bind_mask));

    ma->mask = nmalloc(strlen(cmd) + 1);
    strcpy(ma->mask, cmd);
    ma->first = NULL;
    ma->next = typ->first;
    typ->first = ma;
  }
  context;
  for (tt = ma->first; tt; tt = tt->next) {
    /* already defined? if so replace */
    if (!strcasecmp(tt->func_name, proc)) {
      tt->flags.match = FR_GLOBAL | FR_CHAN;
      break_down_flags(flags, &(tt->flags), NULL);
      return 1;
    }
  }
  context;
  if (!(typ->flags & HT_STACKABLE) && ma->first) {
    nfree(ma->first->func_name);
    nfree(ma->first);
    ma->first = NULL;
  }
  context;
  tt = nmalloc(sizeof(tcl_cmd_t));
  tt->func_name = nmalloc(strlen(proc) + 1);
  tt->next = NULL;
  tt->hits = 0;
  tt->flags.match = FR_GLOBAL | FR_CHAN;
  break_down_flags(flags, &(tt->flags), NULL);
  strcpy(tt->func_name, proc);
  tt->next = ma->first;
  ma->first = tt;
  context;
  return 1;
}

static int tcl_getbinds(p_tcl_bind_list kind, char *name)
{
  tcl_cmd_t *tt;
  struct tcl_bind_mask *be;

  for (be = kind->first; be; be = be->next) {
    if (!strcasecmp(be->mask, name)) {
      for (tt = be->first; tt; tt = tt->next)
	Tcl_AppendElement(interp, tt->func_name);
      return TCL_OK;
    }
  }
  return TCL_OK;
}

static int tcl_bind STDVAR
{
  p_tcl_bind_list tp;

  if ((long int) cd == 1) {
    BADARGS(5, 5, " type flags cmd/mask procname")
  } else {
    BADARGS(4, 5, " type flags cmd/mask ?procname?")
  }
  tp = find_bind_table(argv[1]);
  if (!tp) {
    Tcl_AppendResult(irp, "bad type, should be one of: ", NULL);
    dump_bind_tables(irp);
    return TCL_ERROR;
  }
  if ((long int) cd == 1) {
    if (!unbind_bind_entry(tp, argv[2], argv[3], argv[4])) {
      /* don't error if trying to re-unbind a builtin */
      if ((strcmp(argv[3], &argv[4][5]) != 0) || (argv[4][0] != '*') ||
	  (strncmp(argv[1], &argv[4][1], 3) != 0) ||
	  (argv[4][4] != ':')) {
	Tcl_AppendResult(irp, "no such binding", NULL);
	return TCL_ERROR;
      }
    }
  } else {
    if (argc == 4)
      return tcl_getbinds(tp, argv[3]);
    bind_bind_entry(tp, argv[2], argv[3], argv[4]);
  }
  Tcl_AppendResult(irp, argv[3], NULL);
  return TCL_OK;
}

int check_validity(char *nme, Function f)
{
  char *p;
  p_tcl_bind_list t;

  if (*nme != '*')
    return 0;
  if (!(p = strchr(nme + 1, ':')))
    return 0;
  *p = 0;
  t = find_bind_table(nme + 1);
  *p = ':';
  if (!t)
    return 0;
  if (t->func != f)
    return 0;
  return 1;
}

static int builtin_3char STDVAR {
  Function F = (Function) cd;

  BADARGS(4, 4, " from to args");
  CHECKVALIDITY(builtin_3char);
  F(argv[1], argv[2], argv[3]);
  return TCL_OK;
}

static int builtin_2char STDVAR {
  Function F = (Function) cd;

  BADARGS(3, 3, " nick msg");
  CHECKVALIDITY(builtin_2char);
  F(argv[1], argv[2]);
  return TCL_OK;
}

static int builtin_5int STDVAR {
  Function F = (Function) cd;

  BADARGS(6, 6, " min hrs dom mon year");
  CHECKVALIDITY(builtin_5int);
  F(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
  return TCL_OK;
}

static int builtin_char STDVAR {
  Function F = (Function) cd;

  BADARGS(2, 2, " handle");
  CHECKVALIDITY(builtin_char);
  F(argv[1]);
  return TCL_OK;
}

static int builtin_chpt STDVAR {
  Function F = (Function) cd;

  BADARGS(3, 3, " bot nick sock");
  CHECKVALIDITY(builtin_chpt);
  F(argv[1], argv[2], atoi(argv[3]));
  return TCL_OK;
}

static int builtin_chjn STDVAR {
  Function F = (Function) cd;

  BADARGS(6, 6, " bot nick chan# flag&sock host");
  CHECKVALIDITY(builtin_chjn);
  F(argv[1], argv[2], atoi(argv[3]), argv[4][0],
    argv[4][0] ? atoi(argv[4] + 1) : 0, argv[5]);
  return TCL_OK;
}

static int builtin_idxchar STDVAR {
  Function F = (Function) cd;
  int idx;
  char *r;

  BADARGS(3, 3, " idx args");
  CHECKVALIDITY(builtin_idxchar);
  idx = findidx(atoi(argv[1]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  r = (((char *(*)()) F) (idx, argv[2]));

  Tcl_ResetResult(irp);
  Tcl_AppendResult(irp, r, NULL);
  return TCL_OK;
}

int findanyidx(int z)
{
  int j;

  for (j = 0; j < dcc_total; j++)
    if (dcc[j].sock == z)
      return j;
  return -1;
}

static int builtin_charidx STDVAR {
  Function F = (Function) cd;
  int idx;

  BADARGS(3, 3, " handle idx");
  CHECKVALIDITY(builtin_charidx);
  idx = findanyidx(atoi(argv[2]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(irp, int_to_base10(F(argv[1], idx)), NULL);
  return TCL_OK;
}

static int builtin_chat STDVAR {
  Function F = (Function) cd;
  int ch;

  BADARGS(4, 4, " handle idx text");
  CHECKVALIDITY(builtin_chat);
  ch = atoi(argv[2]);
  F(argv[1], ch, argv[3]);
  return TCL_OK;
}

static int builtin_dcc STDVAR {
  int idx;
  Function F = (Function) cd;

  context;
  BADARGS(4, 4, " hand idx param");
  idx = findidx(atoi(argv[2]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (F == 0) {
    Tcl_AppendResult(irp, "break", NULL);
    return TCL_OK;
  }
  /* check if it's a password change, if so, don't show the password */
  /* lets clean this up, it's debugging, we dont need pretty formats, just
   * a cover up - and dont even bother with .tcl */
  debug4("tcl: builtin dcc call: %s %s %s %s", argv[0], argv[1], argv[2],
	 (!strcmp(argv[0] + 5, "newpass") ||
	  !strcmp(argv[0] + 5, "chpass")) ? "[something]" : argv[3]);
  context;
  (F) (dcc[idx].user, idx, argv[3]);
  context;
  Tcl_ResetResult(irp);
  Tcl_AppendResult(irp, "0", NULL);
  context;
  return TCL_OK;
}

/* trigger (execute) a proc */
static int trigger_bind(char *proc, char *param)
{
  int x;
  FILE *f = 0;

  if (debug_tcl) {
    f = fopen("DEBUG.TCL", "a");
    if (f != NULL)
      fprintf(f, "eval: %s%s\n", proc, param);
  }
  set_tcl_vars();
  context;
  x = Tcl_VarEval(interp, proc, param, NULL);
  context;
  if (x == TCL_ERROR) {
    if (debug_tcl && (f != NULL)) {
      fprintf(f, "done eval. error.\n");
      fclose(f);
    }
    if (strlen(interp->result) > 400)
      interp->result[400] = 0;
    putlog(LOG_MISC, "*", "Tcl error [%s]: %s", proc, interp->result);
    return BIND_EXECUTED;
  } else {
    if (debug_tcl && (f != NULL)) {
      fprintf(f, "done eval. ok.\n");
      fclose(f);
    }
    if (!strcmp(interp->result, "break"))
      return BIND_EXEC_BRK;
    return (atoi(interp->result) > 0) ? BIND_EXEC_LOG : BIND_EXECUTED;
  }
}

int check_tcl_bind(p_tcl_bind_list bind, char *match,
		   struct flag_record *atr, char *param, int match_type)
{
  struct tcl_bind_mask *hm, *ohm = NULL, *hmp = NULL;
  int cnt = 0;
  char *proc = NULL;
  tcl_cmd_t *tt, *htt = NULL;
  int f = 0, atrok, x;

  context;
  for (hm = bind->first; hm && !f; ohm = hm, hm = hm->next) {
    int ok = 0;

    switch (match_type & 0x03) {
    case MATCH_PARTIAL:
      ok = !strncasecmp(match, hm->mask, strlen(match));
      break;
    case MATCH_EXACT:
      ok = !strcasecmp(match, hm->mask);
      break;
    case MATCH_CASE:
      ok = !strcmp(match, hm->mask);
      break;
    case MATCH_MASK:
      ok = wild_match_per((unsigned char *) hm->mask,
			  (unsigned char *) match);
      break;
    }
    if (ok) {
      tt = hm->first;
      if (match_type & BIND_STACKABLE) {
	/* could be multiple triggers */
	while (tt) {
	  if (match_type & BIND_USE_ATTR) {
	    if (match_type & BIND_HAS_BUILTINS)
	      atrok = flagrec_ok(&tt->flags, atr);
	    else
	      atrok = flagrec_eq(&tt->flags, atr);
	  } else
	    atrok = 1;
	  if (atrok) {
	    cnt++;
	    tt->hits++;
	    hmp = ohm;
	    Tcl_SetVar(interp, "lastbind", match, TCL_GLOBAL_ONLY);
	    x = trigger_bind(tt->func_name, param);
	    if ((match_type & BIND_WANTRET) &&
		!(match_type & BIND_ALTER_ARGS) && (x == BIND_EXEC_LOG))
	      return x;
	    if (match_type & BIND_ALTER_ARGS) {
	      if ((interp->result == NULL) || !(interp->result[0]))
		return x;
	      /* this is such an amazingly ugly hack: */
	      Tcl_SetVar(interp, "_a", interp->result, 0);
	    }
	  }
	  tt = tt->next;
	}
	if ((match_type & 3) != MATCH_MASK)
	  f = 1;		/* this will suffice until we have
				 * stackable partials */
      } else {
	if (match_type & BIND_USE_ATTR) {
	  if (match_type & BIND_HAS_BUILTINS)
	    atrok = flagrec_ok(&tt->flags, atr);
	  else
	    atrok = flagrec_eq(&tt->flags, atr);
	} else
	  atrok = 1;
	if (atrok) {
	  cnt++;
	  proc = tt->func_name;
	  htt = tt;
	  hmp = ohm;
	  if (((match_type & 3) != MATCH_PARTIAL) ||
	      !strcasecmp(match, hm->mask))
	    cnt = f = 1;
	}
      }
    }
  }
  context;
  if (cnt == 0)
    return BIND_NOMATCH;
  if (((match_type & 0x03) == MATCH_MASK) ||
      ((match_type & 0x03) == MATCH_CASE))
    return BIND_EXECUTED;
  if ((match_type & 0x3) != MATCH_CASE) {
    if (htt)
      htt->hits++;
    if (hmp) {
      ohm = hmp->next;
      hmp->next = ohm->next;
      ohm->next = bind->first;
      bind->first = ohm;
    }
  }
  if (cnt > 1)
    return BIND_AMBIGUOUS;
  Tcl_SetVar(interp, "lastbind", match, TCL_GLOBAL_ONLY);
  return trigger_bind(proc, param);
}

/* check for tcl-bound dcc command, return 1 if found */
/* dcc: proc-name <handle> <sock> <args...> */
int check_tcl_dcc(char *cmd, int idx, char *args)
{
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  int x;
  char s[5];

  context;
  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);
  sprintf(s, "%ld", dcc[idx].sock);
  Tcl_SetVar(interp, "_dcc1", dcc[idx].nick, 0);
  Tcl_SetVar(interp, "_dcc2", s, 0);
  Tcl_SetVar(interp, "_dcc3", args, 0);
  context;
  x = check_tcl_bind(H_dcc, cmd, &fr, " $_dcc1 $_dcc2 $_dcc3",
		     MATCH_PARTIAL | BIND_USE_ATTR | BIND_HAS_BUILTINS);
  context;
  if (x == BIND_AMBIGUOUS) {
    dprintf(idx, MISC_AMBIGUOUS);
    return 0;
  }
  if (x == BIND_NOMATCH) {
    dprintf(idx, MISC_NOSUCHCMD);
    return 0;
  }
  if (x == BIND_EXEC_BRK)
    return 1;			/* quit */
  if (x == BIND_EXEC_LOG)
    putlog(LOG_CMDS, "*", "#%s# %s %s", dcc[idx].nick, cmd, args);
  return 0;
}

void check_tcl_bot(char *nick, char *code, char *param)
{
  context;
  Tcl_SetVar(interp, "_bot1", nick, 0);
  Tcl_SetVar(interp, "_bot2", code, 0);
  Tcl_SetVar(interp, "_bot3", param, 0);
  check_tcl_bind(H_bot, code, 0, " $_bot1 $_bot2 $_bot3", MATCH_EXACT);
}

void check_tcl_chonof(char *hand, int sock, p_tcl_bind_list table)
{
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  char s[20];
  struct userrec *u = get_user_by_handle(userlist, hand);

  context;
  touch_laston(u, "partyline", now);
  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_chonof1", hand, 0);
  simple_sprintf(s, "%d", sock);
  Tcl_SetVar(interp, "_chonof2", s, 0);
  context;
  check_tcl_bind(table, hand, &fr, " $_chonof1 $_chonof2", MATCH_MASK |
		 BIND_USE_ATTR | BIND_STACKABLE | BIND_WANTRET);
  context;
}

void check_tcl_chatactbcst(char *from, int chan, char *text,
			   p_tcl_bind_list ht)
{
  char s[10];

  context;
  simple_sprintf(s, "%d", chan);
  Tcl_SetVar(interp, "_cab1", from, 0);
  Tcl_SetVar(interp, "_cab2", s, 0);
  Tcl_SetVar(interp, "_cab3", text, 0);
  check_tcl_bind(ht, text, 0, " $_cab1 $_cab2 $_cab3",
		 MATCH_MASK | BIND_STACKABLE);
  context;
}

void check_tcl_nkch(char *ohand, char *nhand)
{
  context;
  Tcl_SetVar(interp, "_nkch1", ohand, 0);
  Tcl_SetVar(interp, "_nkch2", nhand, 0);
  check_tcl_bind(H_nkch, ohand, 0, " $_nkch1 $_nkch2",
		 MATCH_MASK | BIND_STACKABLE);
  context;
}

void check_tcl_link(char *bot, char *via)
{
  context;
  Tcl_SetVar(interp, "_link1", bot, 0);
  Tcl_SetVar(interp, "_link2", via, 0);
  context;
  check_tcl_bind(H_link, bot, 0, " $_link1 $_link2",
		 MATCH_MASK | BIND_STACKABLE);
  context;
}

void check_tcl_disc(char *bot)
{
  context;
  Tcl_SetVar(interp, "_disc1", bot, 0);
  context;
  check_tcl_bind(H_disc, bot, 0, " $_disc1", MATCH_MASK | BIND_STACKABLE);
  context;
}

void check_tcl_loadunld(char *mod, p_tcl_bind_list table)
{
  context;
  Tcl_SetVar(interp, "_lu1", mod, 0);
  context;
  check_tcl_bind(table, mod, 0, " $_lu1", MATCH_MASK | BIND_STACKABLE);
  context;
}

char *check_tcl_filt(int idx, char *text)
{
  char s[10];
  int x;
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  context;
  sprintf(s, "%ld", dcc[idx].sock);
  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);
  Tcl_SetVar(interp, "_filt1", s, 0);
  Tcl_SetVar(interp, "_filt2", text, 0);
  context;
  x = check_tcl_bind(H_filt, text, &fr, " $_filt1 $_filt2",
		     MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE |
		     BIND_WANTRET | BIND_ALTER_ARGS);
  context;
  if ((x == BIND_EXECUTED) || (x == BIND_EXEC_LOG)) {
    if ((interp->result == NULL) || (!interp->result[0]))
      return "";
    else
      return interp->result;
  } else
    return text;
}

int check_tcl_note(char *from, char *to, char *text)
{
  int x;

  context;
  Tcl_SetVar(interp, "_note1", from, 0);
  Tcl_SetVar(interp, "_note2", to, 0);
  Tcl_SetVar(interp, "_note3", text, 0);
  x = check_tcl_bind(H_note, to, 0, " $_note1 $_note2 $_note3", MATCH_EXACT);
  return ((x == BIND_MATCHED) || (x == BIND_EXECUTED) ||
	  (x == BIND_EXEC_LOG));
}

void check_tcl_listen(char *cmd, int idx)
{
  char s[10];
  int x;

  context;
  simple_sprintf(s, "%d", idx);
  Tcl_SetVar(interp, "_n", s, 0);
  set_tcl_vars();
  context;
  x = Tcl_VarEval(interp, cmd, " $_n", NULL);
  context;
  if (x == TCL_ERROR)
    putlog(LOG_MISC, "*", "error on listen: %s", interp->result);
}

void check_tcl_chjn(char *bot, char *nick, int chan, char type,
		    int sock, char *host)
{
  struct flag_record fr =
  {FR_GLOBAL, 0, 0, 0, 0, 0};
  char s[20], t[2], u[20];

  context;
  t[0] = type;
  t[1] = 0;
  switch (type) {
  case '*':
    fr.global = USER_OWNER;

    break;
  case '+':
    fr.global = USER_MASTER;

    break;
  case '@':
    fr.global = USER_OP;

    break;
  case '%':
    fr.global = USER_BOTMAST;
  }
  simple_sprintf(s, "%d", chan);
  simple_sprintf(u, "%d", sock);
  Tcl_SetVar(interp, "_chjn1", bot, 0);
  Tcl_SetVar(interp, "_chjn2", nick, 0);
  Tcl_SetVar(interp, "_chjn3", s, 0);
  Tcl_SetVar(interp, "_chjn4", t, 0);
  Tcl_SetVar(interp, "_chjn5", u, 0);
  Tcl_SetVar(interp, "_chjn6", host, 0);
  context;
  check_tcl_bind(H_chjn, s, &fr,
		 " $_chjn1 $_chjn2 $_chjn3 $_chjn4 $_chjn5 $_chjn6",
		 MATCH_MASK | BIND_STACKABLE);
  context;
}

void check_tcl_chpt(char *bot, char *hand, int sock, int chan)
{
  char u[20], v[20];

  context;
  simple_sprintf(u, "%d", sock);
  simple_sprintf(v, "%d", chan);
  Tcl_SetVar(interp, "_chpt1", bot, 0);
  Tcl_SetVar(interp, "_chpt2", hand, 0);
  Tcl_SetVar(interp, "_chpt3", u, 0);
  Tcl_SetVar(interp, "_chpt4", v, 0);
  context;
  check_tcl_bind(H_chpt, v, 0, " $_chpt1 $_chpt2 $_chpt3 $_chpt4",
		 MATCH_MASK | BIND_STACKABLE);
  context;
}

void check_tcl_away(char *bot, int idx, char *msg)
{
  char u[20];

  context;
  simple_sprintf(u, "%d", idx);
  Tcl_SetVar(interp, "_away1", bot, 0);
  Tcl_SetVar(interp, "_away2", u, 0);
  Tcl_SetVar(interp, "_away3", msg ? msg : "", 0);
  context;
  check_tcl_bind(H_away, bot, 0, " $_away1 $_away2 $_away3",
		 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_time(struct tm *tm)
{
  char y[100];

  context;
  sprintf(y, "%02d", tm->tm_min);
  Tcl_SetVar(interp, "_time1", y, 0);
  sprintf(y, "%02d", tm->tm_hour);
  Tcl_SetVar(interp, "_time2", y, 0);
  sprintf(y, "%02d", tm->tm_mday);
  Tcl_SetVar(interp, "_time3", y, 0);
  sprintf(y, "%02d", tm->tm_mon);
  Tcl_SetVar(interp, "_time4", y, 0);
  sprintf(y, "%04d", tm->tm_year + 1900);
  Tcl_SetVar(interp, "_time5", y, 0);
  sprintf(y, "%02d %02d %02d %02d %04d", tm->tm_min, tm->tm_hour, tm->tm_mday,
	  tm->tm_mon, tm->tm_year + 1900);
  context;
  check_tcl_bind(H_time, y, 0,
		 " $_time1 $_time2 $_time3 $_time4 $_time5",
		 MATCH_MASK | BIND_STACKABLE);
  context;
}

void check_tcl_event(char *event)
{
  context;
  Tcl_SetVar(interp, "_event1", event, 0);
  context;
  check_tcl_bind(H_event, event, 0, " $_event1", MATCH_EXACT | BIND_STACKABLE);
  context;
}

void tell_binds(int idx, char *name)
{
  struct tcl_bind_mask *hm;
  p_tcl_bind_list p, kind;
  int fnd = 0;
  tcl_cmd_t *tt;
  char *s, *proc, flg[100];
  int showall = 0;

  context;
  s = strchr(name, ' ');
  if (s) {
    *s = 0;
    s++;
  } else {
    s = name;
  }
  kind = find_bind_table(name);
  if (!strcasecmp(s, "all"))
    showall = 1;
  for (p = kind ? kind : bind_table_list; p; p = kind ? 0 : p->next) {
    for (hm = p->first; hm; hm = hm->next) {
      if (!fnd) {
	dprintf(idx, MISC_CMDBINDS);
	fnd = 1;
	dprintf(idx, "  TYPE FLGS     COMMAND              HITS BINDING (TCL)\n");
      }
      for (tt = hm->first; tt; tt = tt->next) {
	proc = tt->func_name;
	build_flags(flg, &(tt->flags), NULL);
	context;
	if ((showall) || (proc[0] != '*') || !strchr(proc, ':'))
	  dprintf(idx, "  %-4s %-8s %-20s %4d %s\n", p->name, flg,
		  hm->mask, tt->hits, tt->func_name);
      }
    }
  }
  if (!fnd) {
    if (!kind)
      dprintf(idx, "No command bindings.\n");
    else
      dprintf(idx, "No bindings for %s.\n", name);
  }
}

/* bring the default msg/dcc/fil commands into the Tcl interpreter */
void add_builtins(p_tcl_bind_list table, cmd_t * cc, int count)
{
  int k;
  char p[1024], *l;

  context;
  while (count) {
    count--;
    simple_sprintf(p, "*%s:%s", table->name,
	       cc[count].funcname ? cc[count].funcname : cc[count].name);
    l = (char *) nmalloc(Tcl_ScanElement(p, &k));
    Tcl_ConvertElement(p, l, k | TCL_DONT_USE_BRACES);
    Tcl_CreateCommand(interp, p, table->func,
		      (ClientData) cc[count].func, NULL);
    bind_bind_entry(table, cc[count].flags, cc[count].name, l);
    nfree(l);
    /* create command entry in Tcl interpreter */
  }
}

/* bring the default msg/dcc/fil commands into the Tcl interpreter */
void rem_builtins(p_tcl_bind_list table, cmd_t * cc, int count)
{
  int k;
  char p[1024], *l;

  while (count) {
    count--;
    simple_sprintf(p, "*%s:%s", table->name,
		   cc[count].funcname ? cc[count].funcname :
		   cc[count].name);
    l = (char *) nmalloc(Tcl_ScanElement(p, &k));
    Tcl_ConvertElement(p, l, k | TCL_DONT_USE_BRACES);
    Tcl_DeleteCommand(interp, p);
    unbind_bind_entry(table, cc[count].flags, cc[count].name, l);
    nfree(l);
  }
}
