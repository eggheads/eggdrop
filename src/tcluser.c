/*
 * tcluser.c -- handles:
 *   Tcl stubs for the user-record-oriented commands
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

#include "main.h"
#include "users.h"
#include "chan.h"
#include "tandem.h"
#include "modules.h"
#include "string.h"

extern Tcl_Interp *interp;
extern struct userrec *userlist;
extern int default_flags, dcc_total, ignore_time;
extern struct dcc_t *dcc;
extern char botnetnick[];
extern time_t now;
extern struct user_entry_type *entry_type_list;

static int tcl_countusers STDVAR
{
  BADARGS(1, 1, "");

  Tcl_AppendResult(irp, int_to_base10(count_users(userlist)), NULL);
  return TCL_OK;
}

static int tcl_validuser STDVAR
{
  BADARGS(2, 2, " handle");

  Tcl_AppendResult(irp, get_user_by_handle(userlist, argv[1]) ? "1" : "0",
                   NULL);
  return TCL_OK;
}

static int tcl_finduser STDVAR
{
  struct userrec *u;

  BADARGS(2, 2, " nick!user@host");

  u = get_user_by_host(argv[1]);
  Tcl_AppendResult(irp, u ? u->handle : "*", NULL);
  return TCL_OK;
}

static int tcl_passwdOk STDVAR
{
  struct userrec *u;

  BADARGS(3, 3, " handle passwd");

  Tcl_AppendResult(irp, ((u = get_user_by_handle(userlist, argv[1])) &&
                   u_pass_match(u, argv[2])) ? "1" : "0", NULL);
  return TCL_OK;
}

static int tcl_chattr STDVAR
{
  int of, ocf = 0;
  char *chan, *chg, work[100];
  struct flag_record pls, mns, user;
  struct userrec *u;

  BADARGS(2, 4, " handle ?changes? ?channel?");

  if ((argv[1][0] == '*') || !(u = get_user_by_handle(userlist, argv[1]))) {
    Tcl_AppendResult(irp, "*", NULL);
    return TCL_OK;
  }
  if (argc == 4) {
    user.match = FR_GLOBAL | FR_CHAN;
    chan = argv[3];
    chg = argv[2];
  } else if (argc == 3 && argv[2][0]) {
    int ischan = (findchan_by_dname(argv[2]) != NULL);

    if (strchr(CHANMETA, argv[2][0]) && !ischan && argv[2][0] != '+' &&
        argv[2][0] != '-') {
      Tcl_AppendResult(irp, "no such channel", NULL);
      return TCL_ERROR;
    } else if (ischan) {
      /* Channel exists */
      user.match = FR_GLOBAL | FR_CHAN;
      chan = argv[2];
      chg = NULL;
    } else {
      /* 3rd possibility... channel doesnt exist, does start with a +.
       * In this case we assume the string is flags.
       */
      user.match = FR_GLOBAL;
      chan = NULL;
      chg = argv[2];
    }
  } else {
    user.match = FR_GLOBAL;
    chan = NULL;
    chg = NULL;
  }
  if (chan && !findchan_by_dname(chan)) {
    Tcl_AppendResult(irp, "no such channel", NULL);
    return TCL_ERROR;
  }
  /* Retrieve current flags */
  get_user_flagrec(u, &user, chan);
  /* Make changes */
  if (chg) {
    of = user.global;
    pls.match = user.match;
    break_down_flags(chg, &pls, &mns);
    /* No-one can change these flags on-the-fly */
    pls.global &=~(USER_BOT);
    mns.global &=~(USER_BOT);

    if (chan) {
      pls.chan &= ~(BOT_AGGRESSIVE);
      mns.chan &= ~(BOT_AGGRESSIVE);
    }
    user_sanity_check(&(user.global), pls.global, mns.global);

    user.udef_global = (user.udef_global | pls.udef_global)
                       & ~mns.udef_global;
    if (chan) {
      ocf = user.chan;
      chan_sanity_check(&(user.chan), pls.chan, mns.chan, user.global);
      user.udef_chan = (user.udef_chan | pls.udef_chan) & ~mns.udef_chan;

    }
    set_user_flagrec(u, &user, chan);
    check_dcc_attrs(u, of);
    if (chan)
      check_dcc_chanattrs(u, chan, user.chan, ocf);
  }
  user.chan &= ~BOT_AGGRESSIVE; /* actually not a user flag, hide it */
  /* Build flag string */
  build_flags(work, &user, NULL);
  Tcl_AppendResult(irp, work, NULL);
  return TCL_OK;
}

static int tcl_botattr STDVAR
{
  char *chan, *chg, work[100];
  struct flag_record pls, mns, user;
  struct userrec *u;

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
  } else if (argc == 3 && argv[2][0] && strchr(CHANMETA, argv[2][0]) != NULL) {
    /* We need todo extra checking here to stop us mixing up +channel's
     * with flags. <cybah>
     */
    if (!findchan_by_dname(argv[2]) && argv[2][0] != '+') {
      /* Channel doesnt exist, and it cant possibly be flags as there
       * is no + at the start of the string.
       */
      Tcl_AppendResult(irp, "no such channel", NULL);
      return TCL_ERROR;
    } else if (findchan_by_dname(argv[2])) {
      /* Channel exists */
      user.match = FR_BOT | FR_CHAN;
      chan = argv[2];
      chg = NULL;
    } else {
      /* 3rd possibility... channel doesnt exist, does start with a +.
       * In this case we assume the string is flags.
       */
      user.match = FR_BOT;
      chan = NULL;
      chg = argv[2];
    }
  } else {
    user.match = FR_BOT;
    chan = NULL;
    if (argc < 3)
      chg = NULL;
    else
      chg = argv[2];
  }
  if (chan && !findchan_by_dname(chan)) {
    Tcl_AppendResult(irp, "no such channel", NULL);
    return TCL_ERROR;
  }
  /* Retrieve current flags */
  get_user_flagrec(u, &user, chan);
  /* Make changes */
  if (chg) {
    pls.match = user.match;
    break_down_flags(chg, &pls, &mns);
    /* No-one can change these flags on-the-fly */
    if (chan) {
      pls.chan &= BOT_AGGRESSIVE;
      mns.chan &= BOT_AGGRESSIVE;
    }
    /* this merges user.bot with pls.bot and mns.bot in user.bot
     * squelch the msgids as this is the tcl version of botattr */
    bot_sanity_check(&(user.bot), pls.bot, mns.bot);
    if (chan) {
      user.chan = (user.chan | pls.chan) & ~mns.chan;
      user.udef_chan = (user.udef_chan | pls.udef_chan) & ~mns.udef_chan;
    }
    set_user_flagrec(u, &user, chan);
  }
  /* Only user flags can be set per channel, not bot ones,
     so BOT_AGGRESSIVE is a hack to allow botattr |+s */
  user.chan &= BOT_AGGRESSIVE;
  user.udef_chan = 0; /* User definable bot flags are global only,
                         anything here is a regular flag, so hide it. */
  /* Build flag string */
  build_flags(work, &user, NULL);
  Tcl_AppendResult(irp, work, NULL);
  return TCL_OK;
}

static int tcl_matchattr STDVAR
{
  struct userrec *u;
  struct flag_record plus = {0}, minus = {0}, user = {0};
  int ok = 0, nom = 0;

  BADARGS(3, 4, " handle flags ?channel?");

  if ((u = get_user_by_handle(userlist, argv[1]))) {
    user.match = FR_GLOBAL | (argc == 4 ? FR_CHAN : 0) | FR_BOT;
    get_user_flagrec(u, &user, argv[3]);
    plus.match = user.match;
    break_down_flags(argv[2], &plus, &minus);
    minus.match = plus.match ^ (FR_AND | FR_OR);
    if (!minus.global && !minus.udef_global && !minus.chan &&
        !minus.udef_chan && !minus.bot) {
      nom = 1;
      if (!plus.global && !plus.udef_global && !plus.chan &&
          !plus.udef_chan && !plus.bot) {
        Tcl_AppendResult(irp, "Unknown flag specified for matching", NULL);
        return TCL_ERROR;
      }
    }
    if (flagrec_eq(&plus, &user)) {
      if (nom || !flagrec_eq(&minus, &user)) {
        ok = 1;
      }
    }
  }
  Tcl_AppendResult(irp, ok ? "1" : "0", NULL);
  return TCL_OK;
}

static int tcl_adduser STDVAR
{
  unsigned char *p;

  BADARGS(2, 3, " handle ?hostmask?");

  if (strlen(argv[1]) > HANDLEN)
    argv[1][HANDLEN] = 0;
  for (p = (unsigned char *) argv[1]; *p; p++)
    if (*p <= 32 || *p == '@')
      *p = '?';

  if ((argv[1][0] == '*') || strchr(BADHANDCHARS, argv[1][0]) ||
      get_user_by_handle(userlist, argv[1]))
    Tcl_AppendResult(irp, "0", NULL);
  else {
    userlist = adduser(userlist, argv[1], argv[2], "-", default_flags);
    Tcl_AppendResult(irp, "1", NULL);
  }
  return TCL_OK;
}

static int tcl_addbot STDVAR
{
  struct bot_addr *bi;
  /* Max addr len is 60 ? (see cmd_pls_bot in cmds.c) + brackets + delims + ports + 0 */
  char *p, *q, addr[75], hand[HANDLEN + 1];
  int i, colon=0, braced = 0, ipv6 = 0, count = 0;


  BADARGS(3, 5, " handle address ?telnet-port ?relay-port??");

  /* Copy to adjustable char*'s */
  strlcpy(hand, argv[1], sizeof hand);
  strlcpy(addr, argv[2], sizeof addr);

  for (p = hand; *p; p++)
    if ((unsigned char) *p <= 32 || *p == '@') {
      Tcl_AppendResult(irp, "Invalid character in handle", NULL);
      return TCL_ERROR;
    }

  if ((argv[1][0] == '*') || strchr(BADHANDCHARS, argv[1][0]) ||
      get_user_by_handle(userlist, hand))
    Tcl_AppendResult(irp, "0", NULL);
  else {
    for (i=0; addr[i]; i++) {
      if (addr[i] == ':') {
        count++;
        colon=i;
      }
      if (addr[i] == ']') {
        braced = i;
      }
    }
    if (count > 1) {
      ipv6 = 1;
    }
    if ((!ipv6 && braced)
#ifndef IPV6
        || (ipv6)
#endif
      ) {
      Tcl_AppendResult(irp, "0", NULL);
      return TCL_OK;
    }
    /* Check that the char following the / is not null */
    if ((q = strrchr(addr, '/'))) {
      if (!q[1]) {
        *q = 0;
        q = NULL;
      }
    }

    /* Find ports, still allowing them in address for backward compat */
    if (!ipv6) {
      q = strchr(addr, ':');
    } else if (braced && (colon > braced)) {
      q = strrchr(addr, ':');
    } else {
      /* IPv6 address with no braces or braces without port(s) after */
      q = NULL;
    }

    if (q) {
      /* Split and count, ignore any following args */
      *q++ = 0;
      p = strchr(q, '/');
      if (p)
        *p++ = 0;
    } else {
      /* No port in address field, check next args */
      p = NULL;
      if (argc == 4 || argc == 5) {
        q = argv[3];
      }
      if (argc == 5) {
        p = argv[4];
      }
    }

    /* Verify ports */
#ifndef TLS
    /* Check TLS */
    if ((q && *q == '+') || (p && *p == '+')) {
      Tcl_AppendResult(irp, "0", NULL);
      return TCL_OK;
    }

#endif
    /* Verify */
    /* check_int_range returns 0 if q or p is NULL */
    if (q && !check_int_range(q, 0, 65536)) {
      Tcl_AppendResult(irp, "0", NULL);
      return TCL_OK;
    }
    if (p && !check_int_range(p, 0, 65536)) {
      Tcl_AppendResult(irp, "0", NULL);
      return TCL_OK;
    }

    userlist = adduser(userlist, hand, "none", "-", USER_BOT);
    bi = user_malloc(sizeof(struct bot_addr));
#ifdef TLS
    bi->ssl = 0;
#endif

    if ((count = strlen(addr)) > 60) {
      count = 60;
      addr[count] = 0;
    }
    /* Trim IPv6 []s out if present */
    if (braced) {
      --count;
      addr[count] = 0;
      memmove(addr, addr + 1, count);
    }

    if (!q) {
      bi->address = user_malloc(count + 1);
      strcpy(bi->address, addr);
      bi->telnet_port = 3333;
      bi->relay_port = 3333;
    } else {
      bi->address = user_malloc(count + 1);
      strcpy(bi->address, addr);
      bi->telnet_port = atoi(q);
#ifdef TLS
      if (*q == '+')
        bi->ssl = TLS_BOT;
#endif
      if (!p) {
        bi->relay_port = bi->telnet_port;
#ifdef TLS
        bi->ssl *= TLS_BOT + TLS_RELAY;
#endif
      } else {
        bi->relay_port = atoi(p);
#ifdef TLS
        if (*p == '+')
          bi->ssl |= TLS_RELAY;
#endif
      }
    }
    set_user(&USERENTRY_BOTADDR, get_user_by_handle(userlist, hand), bi);
    Tcl_AppendResult(irp, "1", NULL);
  }
  return TCL_OK;
}

static int tcl_deluser STDVAR
{
  BADARGS(2, 2, " handle");

  Tcl_AppendResult(irp, (argv[1][0] == '*') ? "0" :
                   int_to_base10(deluser(argv[1])), NULL);
  return TCL_OK;
}

static int tcl_delhost STDVAR
{
  BADARGS(3, 3, " handle hostmask");

  if ((!get_user_by_handle(userlist, argv[1])) || (argv[1][0] == '*')) {
    Tcl_AppendResult(irp, "non-existent user", NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(irp, delhost_by_handle(argv[1], argv[2]) ? "1" : "0", NULL);
  return TCL_OK;
}

static int tcl_userlist STDVAR
{
  struct userrec *u;
  struct flag_record user, plus, minus;
  int ok = 1, f = 0;

  BADARGS(1, 3, " ?flags ?channel??");

  if (argc == 3 && !findchan_by_dname(argv[2])) {
    Tcl_AppendResult(irp, "Invalid channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (argc >= 2) {
    plus.match = FR_GLOBAL | FR_CHAN | FR_BOT;
    break_down_flags(argv[1], &plus, &minus);
    f = (minus.global ||minus.udef_global || minus.chan || minus.udef_chan ||
         minus.bot);
    minus.match = plus.match ^ (FR_AND | FR_OR);
  }
  for (u = userlist; u; u = u->next) {
    if (argc >= 2) {
      user.match = FR_GLOBAL | FR_CHAN | FR_BOT | (argc == 3 ? 0 : FR_ANYWH);
      if (argc == 3)
        get_user_flagrec(u, &user, argv[2]);
      else
        get_user_flagrec(u, &user, NULL);
      if (flagrec_eq(&plus, &user) && !(f && flagrec_eq(&minus, &user)))
        ok = 1;
      else
        ok = 0;
    }
    if (ok)
      Tcl_AppendElement(interp, u->handle);
  }
  return TCL_OK;
}

static int tcl_save STDVAR
{
  write_userfile(-1);
  return TCL_OK;
}

static int tcl_reload STDVAR
{
  reload();
  return TCL_OK;
}

static int tcl_chhandle STDVAR
{
  struct userrec *u;
  char newhand[HANDLEN + 1];
  int x = 1, i;

  BADARGS(3, 3, " oldnick newnick");

  u = get_user_by_handle(userlist, argv[1]);
  if (!u)
    x = 0;
  else {
    strlcpy(newhand, argv[2], sizeof newhand);
    for (i = 0; i < strlen(newhand); i++)
      if (((unsigned char) newhand[i] <= 32) || (newhand[i] == '@'))
        newhand[i] = '?';
    if (strchr(BADHANDCHARS, newhand[0]) != NULL)
      x = 0;
    else if (strlen(newhand) < 1)
      x = 0;
    else if (get_user_by_handle(userlist, newhand))
      x = 0;
    else if (!strcasecmp(botnetnick, newhand) && (!(u->flags & USER_BOT) ||
             nextbot(argv[1]) != -1))
      x = 0;
    else if (newhand[0] == '*')
      x = 0;
  }
  if (x)
    x = change_handle(u, newhand);

  Tcl_AppendResult(irp, x ? "1" : "0", NULL);
  return TCL_OK;
}

static int tcl_getting_users STDVAR
{
  int i;

  BADARGS(1, 1, "");

  for (i = 0; i < dcc_total; i++) {
    if (dcc[i].type == &DCC_BOT && dcc[i].status & STAT_GETTING) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_isignore STDVAR
{
  BADARGS(2, 2, " nick!user@host");

  Tcl_AppendResult(irp, match_ignore(argv[1]) ? "1" : "0", NULL);
  return TCL_OK;
}

static int tcl_newignore STDVAR
{
  time_t expire_time;
  char ign[UHOSTLEN], cmt[66], from[HANDLEN + 1];

  BADARGS(4, 5, " hostmask creator comment ?lifetime?");

  strlcpy(ign, argv[1], sizeof ign);
  strlcpy(from, argv[2], sizeof from);
  strlcpy(cmt, argv[3], sizeof cmt);
  if (argc == 4)
    expire_time = now + 60 * ignore_time;
  else if ((expire_time = get_expire_time(irp, argv[4])) == -1)
    return TCL_ERROR;
  addignore(ign, from, cmt, expire_time);
  return TCL_OK;
}

static int tcl_killignore STDVAR
{
  BADARGS(2, 2, " hostmask");

  Tcl_AppendResult(irp, delignore(argv[1]) ? "1" : "0", NULL);
  return TCL_OK;
}

static int tcl_ignorelist STDVAR
{
  char expire[11], added[11], *p;
  long tv;
  EGG_CONST char *list[5];
  struct igrec *i;

  BADARGS(1, 1, "");

  for (i = global_ign; i; i = i->next) {
    list[0] = i->igmask;
    list[1] = i->msg;

    tv = i->expire;
    egg_snprintf(expire, sizeof expire, "%lu", tv);
    list[2] = expire;

    tv = i->added;
    egg_snprintf(added, sizeof added, "%lu", tv);
    list[3] = added;

    list[4] = i->user;
    p = Tcl_Merge(5, list);
    Tcl_AppendElement(irp, p);
    Tcl_Free((char *) p);
  }
  return TCL_OK;
}

static int tcl_getuser STDVAR
{
  struct user_entry_type *et = NULL;
  struct userrec *u;
  struct user_entry *e;

  BADARGS(2, -1, " handle ?type?");

  if (!(u = get_user_by_handle(userlist, argv[1]))) {
    if (argv[1][0] != '*') {
      Tcl_AppendResult(irp, "No such user.", NULL);
      return TCL_ERROR;
    } else
      return TCL_OK; /* silently ignore user */
  }
  if (argc >= 3) {
    if (!(et = find_entry_type(argv[2])) &&
        strcasecmp(argv[2], "HANDLE")) {
      Tcl_AppendResult(irp, "No such info type: ", argv[2], NULL);
      return TCL_ERROR;
    }
    if (!strcasecmp(argv[2], "HANDLE"))
      Tcl_AppendResult(irp, u->handle, NULL);
    else {
      e = find_user_entry(et, u);
      if (e)
        return et->tcl_get(irp, u, e, argc, argv);
    }
  } else {
    for (et = entry_type_list; et; et = et->next) {
      if (!et->tcl_append)
        continue;
      e = find_user_entry(et, u);
      if (e && e->name) {
        Tcl_AppendElement(irp, e->name);
      } else if (et->name) {
        Tcl_AppendElement(irp, et->name);
      } else {
        continue;
      }
      if (e) {
        et->tcl_append(irp, u, e);
      } else {
        Tcl_AppendElement(irp, "");
      }
    }
  }
  return TCL_OK;
}

static int tcl_setuser STDVAR
{
  struct user_entry_type *et;
  struct userrec *u;
  struct user_entry *e;
  int r;
  module_entry *me;

  BADARGS(3, -1, " handle type ?setting....?");

  if (!(et = find_entry_type(argv[2]))) {
    Tcl_AppendResult(irp, "No such info type: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (!(u = get_user_by_handle(userlist, argv[1]))) {
    if (argv[1][0] != '*') {
      Tcl_AppendResult(irp, "No such user.", NULL);
      return TCL_ERROR;
    } else
      return TCL_OK; /* Silently ignore user * */
  }
  me = module_find("irc", 0, 0);
  if (me && !strcasecmp(argv[2], "hosts") && argc == 3) {
    Function *func = me->funcs;

    (func[IRC_CHECK_THIS_USER]) (argv[1], 1, NULL);
  }
  if (!(e = find_user_entry(et, u))) {
    e = user_malloc(sizeof(struct user_entry));
    e->type = et;
    e->name = NULL;
    e->u.list = NULL;
    list_insert((&(u->entries)), e);
  }
  r = et->tcl_set(irp, u, e, argc, argv);
  /* Yeah... e is freed, and we read it... (tcl: setuser hand HOSTS none) */
  if ((!e->u.list) && (egg_list_delete((struct list_type **) &(u->entries),
      (struct list_type *) e)))
    nfree(e);
    /* else maybe already freed... (entry_type==HOSTS) <drummer> */
  if (me && !strcasecmp(argv[2], "hosts") && argc == 4) {
    Function *func = me->funcs;

    (func[IRC_CHECK_THIS_USER]) (argv[1], 0, NULL);
  }
  return r;
}

tcl_cmds tcluser_cmds[] = {
  {"countusers",       tcl_countusers},
  {"validuser",         tcl_validuser},
  {"finduser",           tcl_finduser},
  {"passwdok",           tcl_passwdOk},
  {"chattr",               tcl_chattr},
  {"botattr",             tcl_botattr},
  {"matchattr",         tcl_matchattr},
  {"matchchanattr",     tcl_matchattr},
  {"adduser",             tcl_adduser},
  {"addbot",               tcl_addbot},
  {"deluser",             tcl_deluser},
  {"delhost",             tcl_delhost},
  {"userlist",           tcl_userlist},
  {"save",                   tcl_save},
  {"reload",               tcl_reload},
  {"chhandle",           tcl_chhandle},
  {"chnick",             tcl_chhandle},
  {"getting-users", tcl_getting_users},
  {"isignore",           tcl_isignore},
  {"newignore",         tcl_newignore},
  {"killignore",       tcl_killignore},
  {"ignorelist",       tcl_ignorelist},
  {"getuser",             tcl_getuser},
  {"setuser",             tcl_setuser},
  {NULL,                         NULL}
};
