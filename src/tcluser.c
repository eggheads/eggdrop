/*
 * tcluser.c -- handles:
 * Tcl stubs for the user-record-oriented commands
 * 
 * dprintf'ized, 1aug1996
 */
/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

#include "main.h"
#include "users.h"
#include "chan.h"
#include "tandem.h"

/* eggdrop always uses the same interpreter */
extern Tcl_Interp *interp;
extern struct userrec *userlist;
extern int default_flags;
extern struct dcc_t *dcc;
extern int dcc_total;
extern char origbotname[];
extern char botnetnick[];
extern int ignore_time;
extern time_t now;

/***********************************************************************/

static int tcl_countusers STDVAR {
  context;
  BADARGS(1, 1, "");
  Tcl_AppendResult(irp, int_to_base10(count_users(userlist)), NULL);
  return TCL_OK;
}

static int tcl_validuser STDVAR {
  context;
  BADARGS(2, 2, " handle");
  Tcl_AppendResult(irp, get_user_by_handle(userlist, argv[1]) ? "1" : "0",
		   NULL);
  return TCL_OK;
}
static int tcl_finduser STDVAR {
  struct userrec *u;

  context;
  BADARGS(2, 2, " nick!user@host");
  u = get_user_by_host(argv[1]);
  Tcl_AppendResult(irp, u ? u->handle : "*", NULL);
  return TCL_OK;
}

static int tcl_passwdOk STDVAR {
  struct userrec *u;

  context;
  BADARGS(3, 3, " handle passwd");
  Tcl_AppendResult(irp, ((u = get_user_by_handle(userlist, argv[1])) &&
			 u_pass_match(u, argv[2])) ? "1" : "0", NULL);
  return TCL_OK;
}

static int tcl_chattr STDVAR {
  char *chan, *chg, work[100];
  struct flag_record pls, mns, user;
  struct userrec *u;

  context;
  BADARGS(2, 4, " handle ?changes? ?channel?");
  if ((argv[1][0] == '*') || !(u = get_user_by_handle(userlist, argv[1]))) {
    Tcl_AppendResult(irp, "*", NULL);
    return TCL_OK;
  }
  if (argc == 4) {
    user.match = FR_GLOBAL | FR_CHAN;
    chan = argv[3];
    chg = argv[2];
  } else if ((argc == 3) && (strchr(CHANMETA, argv[2][0]) != NULL)) {
    /* We need todo extra checking here to stop us mixing up +channel's
     * with flags. <cybah> */
    if (!findchan(argv[2]) && argv[2][0] != '+') {
      /* Channel doesnt exist, and it cant possibly be flags as there
       * is no + at the start of the string. */
      Tcl_AppendResult(irp, "no such channel", NULL);
      return TCL_ERROR;
    } else if(findchan(argv[2])) {
      /* Channel exists */
      user.match = FR_GLOBAL | FR_CHAN;
      chan = argv[2];
      chg = NULL;
    } else {
      /* 3rd possibility... channel doesnt exist, does start with a +.
       * In this case we assume the string is flags. */
      user.match = FR_GLOBAL;
      chan = NULL;
      chg = argv[2];
    }
  } else {
    user.match = FR_GLOBAL;
    chan = NULL;
    chg = argv[2];
  }
  if (chan && !findchan(chan)) {
    Tcl_AppendResult(irp, "no such channel", NULL);
    return TCL_ERROR;
  }
  get_user_flagrec(u, &user, chan);
  /* make changes */
  if (chg) {
    pls.match = user.match;
    break_down_flags(chg, &pls, &mns);
    /* no-one can change these flags on-the-fly */
    pls.global &=~(USER_BOT);
    mns.global &=~(USER_BOT);
    if (chan) {
      pls.chan &= ~(BOT_SHARE);
      mns.chan &= ~(BOT_SHARE);
    }
    user.global = sanity_check((user.global |pls.global) &~mns.global);
    user.udef_global = (user.udef_global | pls.udef_global)
      & ~mns.udef_global;
    if (chan) {
      user.chan = chan_sanity_check((user.chan | pls.chan) & ~mns.chan,
				    user.global);
      user.udef_chan = (user.udef_chan | pls.udef_chan) & ~mns.udef_chan;
    }
    set_user_flagrec(u, &user, chan);
  }
  /* retrieve current flags and return them */
  build_flags(work, &user, NULL);
  Tcl_AppendResult(irp, work, NULL);
  return TCL_OK;
}

static int tcl_botattr STDVAR {
  char *chan, *chg, work[100];
  struct flag_record pls, mns, user;
  struct userrec *u;

  context;
  BADARGS(2, 4, " bot-handle ?changes? ?channel?");
  u = get_user_by_handle(userlist, argv[1]);
  if ((argv[1][0] == '*') || !u || !(u->flags & USER_BOT)) {
    Tcl_AppendResult(irp, "*", NULL);
    return TCL_OK;
  }
  if (argc == 4) {
    user.match = FR_BOT | FR_CHAN;
    chan = argv[3];
    chg = argv[2];
  } else if ((argc == 3) && (strchr(CHANMETA, argv[2][0]) != NULL)) {
    /* We need todo extra checking here to stop us mixing up +channel's
     * with flags. <cybah> */
    if (!findchan(argv[2]) && argv[2][0] != '+') {
      /* Channel doesnt exist, and it cant possibly be flags as there
       * is no + at the start of the string. */
      Tcl_AppendResult(irp, "no such channel", NULL);
      return TCL_ERROR;
    } else if(findchan(argv[2])) {
      /* Channel exists */
      user.match = FR_BOT | FR_CHAN;
      chan = argv[2];
      chg = NULL;
    } else {
      /* 3rd possibility... channel doesnt exist, does start with a +.
       * In this case we assume the string is flags. */
      user.match = FR_BOT;
      chan = NULL;
      chg = argv[2];
    }
  } else {
    user.match = FR_BOT;
    chan = NULL;
    chg = argv[2];
  }
  if (chan && !findchan(chan)) {
    Tcl_AppendResult(irp, "no such channel", NULL);
    return TCL_ERROR;
  }
  get_user_flagrec(u, &user, chan);
  /* make changes */
  if (chg) {
    pls.match = user.match;
    break_down_flags(chg, &pls, &mns);
    /* no-one can change these flags on-the-fly */
    if (chan) {
      pls.chan &= BOT_SHARE;
      mns.chan &= BOT_SHARE;
    }
    user.bot = sanity_check((user.bot | pls.bot) & ~mns.bot);
    if (chan) {
      user.chan = chan_sanity_check((user.chan | pls.chan) & ~mns.chan,
				    user.global);
      user.udef_chan = (user.udef_chan | pls.udef_chan) & ~mns.udef_chan;
    }
    set_user_flagrec(u, &user, chan);
  }
  /* retrieve current flags and return them */
  build_flags(work, &user, NULL);
  Tcl_AppendResult(irp, work, NULL);
  return TCL_OK;
}

static int tcl_matchattr STDVAR {
  struct userrec *u;
  struct flag_record plus, minus, user;
  int ok = 0, f;

  context;
  BADARGS(3, 4, " handle flags ?channel?");
  context;			/* a2 - Last context: tcluser.c/184 */
  if ((u = get_user_by_handle(userlist, argv[1])) &&
      ((argc == 3) || findchan(argv[3]))) {
    context;			/* a2 - Last context: tcluser.c/184 */
    user.match = FR_GLOBAL | (argc == 4 ? FR_CHAN : 0) | FR_BOT;
    get_user_flagrec(u, &user, argv[3]);
    plus.match = user.match;
    break_down_flags(argv[2], &plus, &minus);
    f = (minus.global || minus.udef_global || minus.chan ||
	 minus.udef_chan || minus.bot);
    if (flagrec_eq(&plus, &user)) {
      context;			/* a2 - Last context: tcluser.c/184 */
      if (!f)
	ok = 1;
      else {
	minus.match = plus.match ^ (FR_AND | FR_OR);
	if (!flagrec_eq(&minus, &user))
	  ok = 1;
      }
    }
  }
  context;			/* a2 - Last context: tcluser.c/184 */
  Tcl_AppendResult(irp, ok ? "1" : "0", NULL);
  return TCL_OK;
}

static int tcl_adduser STDVAR {
  context;
  BADARGS(3, 3, " handle hostmask");
  if (strlen(argv[1]) > HANDLEN)
    argv[1][HANDLEN] = 0;
  if ((argv[1][0] == '*') || get_user_by_handle(userlist, argv[1]))
    Tcl_AppendResult(irp, "0", NULL);
  else {
    userlist = adduser(userlist, argv[1], argv[2], "-", default_flags);
    Tcl_AppendResult(irp, "1", NULL);
  }
  return TCL_OK;
}

static int tcl_addbot STDVAR {
  struct bot_addr *bi;
  char *p, *q;

  context;
  BADARGS(3, 3, " handle address");
  if (strlen(argv[1]) > HANDLEN)
     argv[1][HANDLEN] = 0;
  if (get_user_by_handle(userlist, argv[1]))
     Tcl_AppendResult(irp, "0", NULL);
  else if (argv[1][0] == '*')
     Tcl_AppendResult(irp, "0", NULL);
  else {
    userlist = adduser(userlist, argv[1], "none", "-", USER_BOT);
    bi = user_malloc(sizeof(struct bot_addr));
     q = strchr(argv[2], ':');
    if (!q) {
      bi->address = user_malloc(strlen(argv[2]) + 1);
      strcpy(bi->address, argv[2]);
      bi->telnet_port = 3333;
      bi->relay_port = 3333;
    } else {
      bi->address = user_malloc(q - argv[2] + 1);
      strncpy(bi->address, argv[2], q - argv[2]);
      bi->address[q - argv[2]] = 0;
      p = q + 1;
      bi->telnet_port = atoi(p);
      q = strchr(p, '/');
      if (!q)
	bi->relay_port = bi->telnet_port;
      else
	bi->relay_port = atoi(q + 1);
    }
    set_user(&USERENTRY_BOTADDR, get_user_by_handle(userlist, argv[1]), bi);
    Tcl_AppendResult(irp, "1", NULL);
  }
  return TCL_OK;
}

static int tcl_deluser STDVAR {
  context;
  BADARGS(2, 2, " handle");
  Tcl_AppendResult(irp, (argv[1][0] == '*') ? "0" :
		   int_to_base10(deluser(argv[1])), NULL);
  return TCL_OK;
}

static int tcl_delhost STDVAR {
  context;
  BADARGS(3, 3, " handle hostmask");
  if ((!get_user_by_handle(userlist, argv[1])) || (argv[1][0] == '*')) {
    Tcl_AppendResult(irp, "non-existent user", NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(irp, delhost_by_handle(argv[1], argv[2]) ? "1" : "0",
		   NULL);
  return TCL_OK;
}

static int tcl_userlist STDVAR {
  struct userrec *u = userlist;
  struct flag_record user, plus, minus;
  int ok = 1, f = 0;

  context;
  BADARGS(1, 3, " ?flags ?channel??");
  if ((argc == 3) && !findchan(argv[2])) {
    Tcl_AppendResult(irp, "Invalid channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (argc >= 2) {
    plus.match = FR_GLOBAL | FR_CHAN | FR_BOT;
    break_down_flags(argv[1], &plus, &minus);
    f = (minus.global || minus.udef_global || minus.chan ||
	 minus.udef_chan || minus.bot);
  }
  minus.match = plus.match ^ (FR_AND | FR_OR);
  while (u) {
    if (argc >= 2) {
      user.match = FR_GLOBAL | FR_CHAN | FR_BOT | (argc == 3 ? 0 : FR_ANYWH);
      get_user_flagrec(u, &user, argv[2]);	/* argv[2] == NULL for argc = 2 ;) */
      if (flagrec_eq(&plus, &user) && !(f && flagrec_eq(&minus, &user)))
	ok = 1;
      else
	ok = 0;
    }
    if (ok)
      Tcl_AppendElement(interp, u->handle);
    u = u->next;
  }
  return TCL_OK;
}

static int tcl_save STDVAR {
  context;
  write_userfile(-1);
  return TCL_OK;
}

static int tcl_reload STDVAR {
  context;
  reload();
  return TCL_OK;
}

static int tcl_chnick STDVAR {
  struct userrec *u;
  char hand[HANDLEN + 1];
  int x = 1, i;

  context;
  BADARGS(3, 3, " oldnick newnick");
  u = get_user_by_handle(userlist, argv[1]);
  if (!u)
     x = 0;
  else {
    strncpy(hand, argv[2], HANDLEN);
    hand[HANDLEN] = 0;
    for (i = 0; i < strlen(hand); i++)
      if ((hand[i] <= 32) || (hand[i] >= 127) || (hand[i] == '@'))
	hand[i] = '?';
    if (strchr("-,+*=:!.@#;$", hand[0]) != NULL)
      x = 0;
    else if (strlen(hand) < 1)
      x = 0;
    else if (get_user_by_handle(userlist, hand))
      x = 0;
    else if (!strcasecmp(origbotname, hand) || !rfc_casecmp(botnetnick, hand))
      x = 0;
    else if (hand[0] == '*')
      x = 0;
  }
  if (x)
     x = change_handle(u, hand);

  Tcl_AppendResult(irp, x ? "1" : "0", NULL);
  return TCL_OK;
}

static int tcl_getting_users STDVAR {
  int i;

  context;
  BADARGS(1, 1, "");
  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type == &DCC_BOT) &&
	(dcc[i].status & STAT_GETTING)) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_isignore STDVAR {
  context;
  BADARGS(2, 2, " nick!user@host");
  Tcl_AppendResult(irp, match_ignore(argv[1]) ? "1" : "0", NULL);
  return TCL_OK;
}

static int tcl_newignore STDVAR {
  time_t expire_time;
  char ign[UHOSTLEN], cmt[66], from[HANDLEN + 1];

  context;
  BADARGS(4, 5, " hostmask creator comment ?lifetime?");
  strncpy(ign, argv[1], UHOSTLEN - 1);
  ign[UHOSTLEN - 1] = 0;
  strncpy(from, argv[2], HANDLEN);
  from[HANDLEN] = 0;
  strncpy(cmt, argv[3], 65);
  cmt[65] = 0;
  if (argc == 4)
     expire_time = now + (60 * ignore_time);
  else {
    if (atol(argv[4]) == 0)
      expire_time = 0L;
    else
      expire_time = now + (60 * atol(argv[4]));
  }
  addignore(ign, from, cmt, expire_time);

  return TCL_OK;
}

static int tcl_killignore STDVAR {
  context;
  BADARGS(2, 2, " hostmask");
  Tcl_AppendResult(irp, delignore(argv[1]) ? "1" : "0", NULL);
  return TCL_OK;
}

/* { hostmask note expire-time create-time creator } */
static int tcl_ignorelist STDVAR {
  struct igrec *i;
  char ts[21], ts1[21], *list[5], *p;

  context;
  BADARGS(1, 1, "");
  for (i = global_ign; i; i = i->next) {
    list[0] = i->igmask;
    list[1] = i->msg;
    sprintf(ts, "%lu", i->expire);
    list[2] = ts;
    sprintf(ts1, "%lu", i->added);
    list[3] = ts1;
    list[4] = i->user;
    p = Tcl_Merge(5, list);
    Tcl_AppendElement(irp, p);
    n_free(p, "", 0);
  }
  return TCL_OK;
}

static int tcl_getuser STDVAR {
  struct user_entry_type *et;
  struct userrec *u;
  struct user_entry *e;

  context;
  BADARGS(3, 999, " handle type");
  if (!(et = find_entry_type(argv[2]))) {
    Tcl_AppendResult(irp, "No such info type: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (!(u = get_user_by_handle(userlist, argv[1]))) {
    if (argv[1][0] != '*') {
      Tcl_AppendResult(irp, "No such user.", NULL);
      return TCL_ERROR;
    } else
      return TCL_OK;		/* silently ignore user * */
  }
  e = find_user_entry(et, u);

  if (e)
    return et->tcl_get(irp, u, e, argc, argv);
  return TCL_OK;
}

static int tcl_setuser STDVAR {
  struct user_entry_type *et;
  struct userrec *u;
  struct user_entry *e;
  int r;

  context;
  BADARGS(3, 999, " handle type ?setting....?");
  if (!(et = find_entry_type(argv[2]))) {
    Tcl_AppendResult(irp, "No such info type: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (!(u = get_user_by_handle(userlist, argv[1]))) {
    if (argv[1][0] != '*') {
      Tcl_AppendResult(irp, "No such user.", NULL);
      return TCL_ERROR;
    } else
      return TCL_OK;		/* silently ignore user * */
  }
  if (!(e = find_user_entry(et, u))) {
    e = user_malloc(sizeof(struct user_entry));

    e->type = et;
    e->name = NULL;
    e->u.list = NULL;
    list_insert((&(u->entries)), e);
  }
  r = et->tcl_set(irp, u, e, argc, argv);
  if (!e->u.list) {
    list_delete((struct list_type **) &(u->entries), (struct list_type *) e);
    nfree(e);
  }
  return r;
}

tcl_cmds tcluser_cmds[] =
{
  {"countusers", tcl_countusers},
  {"validuser", tcl_validuser},
  {"finduser", tcl_finduser},
  {"passwdok", tcl_passwdOk},
  {"chattr", tcl_chattr},
  {"botattr", tcl_botattr},
  {"matchattr", tcl_matchattr},
  {"matchchanattr", tcl_matchattr},
  {"adduser", tcl_adduser},
  {"addbot", tcl_addbot},
  {"deluser", tcl_deluser},
  {"delhost", tcl_delhost},
  {"userlist", tcl_userlist},
  {"save", tcl_save},
  {"reload", tcl_reload},
  {"chnick", tcl_chnick},
  {"getting-users", tcl_getting_users},
  {"isignore", tcl_isignore},
  {"newignore", tcl_newignore},
  {"killignore", tcl_killignore},
  {"ignorelist", tcl_ignorelist},
  {"getuser", tcl_getuser},
  {"setuser", tcl_setuser},
  {0, 0}
};
