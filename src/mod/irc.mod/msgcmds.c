/* 
 * msgcmds.c -- handles:
 * all commands entered via /MSG
 * 
 * dprintf'ized, 4feb1996
 */
/* 
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

static int msg_hello(char *nick, char *h, struct userrec *u, char *p)
{
  char host[161], s[161], s1[161];
  char *p1;
  int common = 0;
  int atr = 0;
  struct chanset_t *chan;
  struct flag_record fr =
  {FR_GLOBAL, 0, 0, 0, 0, 0};

  if ((!learn_users) && (!make_userfile))
    return 0;
  if (match_my_nick(nick))
    return 1;
  if (u)
    atr = u->flags;
  if (u && !(atr & USER_COMMON)) {
    dprintf(DP_HELP, "NOTICE %s :%s, %s.\n", IRC_HI, nick, u->handle);
    return 1;
  }
  if (get_user_by_handle(userlist, nick)) {
    struct flag_record fr =
    {FR_GLOBAL, 0, 0, 0, 0, 0};
    fr.global = atr;

    dprintf(DP_HELP, IRC_BADHOST1, IRC_BADHOST1_ARGS);
    dprintf(DP_HELP, IRC_BADHOST2, IRC_BADHOST2_ARGS);
    return 1;
  }
  simple_sprintf(s, "%s!%s", nick, h);
  if (u_match_mask(global_bans, s)) {
    dprintf(DP_HELP, "NOTICE %s :%s.\n", nick, IRC_BANNED2);
    return 1;
  }
  if (strlen(nick) > HANDLEN) {
    /* crazy dalnet */
    dprintf(DP_HELP, "NOTICE %s :%s.\n", nick, IRC_NICKTOOLONG);
    return 1;
  }
  if (atr & USER_COMMON) {
    maskhost(s, host);
    strcpy(s, host);
    simple_sprintf(host, "%s!%s", nick, s + 2);
    userlist = adduser(userlist, nick, host, "-", USER_DEFAULT);
    putlog(LOG_MISC, "*", "%s %s (%s) -- %s",
	   IRC_INTRODUCED, nick, host, IRC_COMMONSITE);
    common = 1;
  } else {
    maskhost(s, host);
    if (make_userfile) {
      userlist = adduser(userlist, nick, host, "-",
		 sanity_check(default_flags | USER_MASTER | USER_OWNER));
      set_user(&USERENTRY_HOSTS, get_user_by_handle(userlist, nick),
	       "telnet!*@*");
    } else
      userlist = adduser(userlist, nick, host, "-",
			 sanity_check(default_flags));
    putlog(LOG_MISC, "*", "%s %s (%s)", IRC_INTRODUCED, nick, host);
  }
  for (chan = chanset; chan; chan = chan->next) {
    if (ismember(chan, nick))
      add_chanrec_by_handle(userlist, nick, chan->name);
  }
  dprintf(DP_HELP, IRC_SALUT1, IRC_SALUT1_ARGS);
  dprintf(DP_HELP, IRC_SALUT2, IRC_SALUT2_ARGS);
  if (common) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_SALUT2A);
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_SALUT2B);
  }
  if (make_userfile) {
    fr.global = sanity_check(default_flags | USER_OWNER);

    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_INITOWNER1);
    dprintf(DP_HELP, IRC_NEWBOT1, IRC_NEWBOT1_ARGS);
    dprintf(DP_HELP, IRC_NEWBOT2, IRC_NEWBOT2_ARGS);
    putlog(LOG_MISC, "*", IRC_INIT1, IRC_INIT1_ARGS);
    make_userfile = 0;
    write_userfile(-1);
    add_note(nick, origbotname, IRC_INITNOTE, -1, 0);
  } else {
    fr.global = default_flags;

    dprintf(DP_HELP, IRC_INTRO1, IRC_INTRO1_ARGS);
  }
  if (notify_new[0]) {
    sprintf(s, IRC_INITINTRO, nick, host);
    strcpy(s1, notify_new);
    while (s1[0]) {
      p1 = strchr(s1, ',');
      if (p1 != NULL) {
	*p1 = 0;
	p1++;
	rmspace(p1);
      }
      rmspace(s1);
      add_note(s1, origbotname, s, -1, 0);
      if (p1 == NULL)
	s1[0] = 0;
      else
	strcpy(s1, p1);
    }
  }
  return 1;
}

static int msg_pass(char *nick, char *host, struct userrec *u, char *par)
{
  char *old, *new;

  if (match_my_nick(nick))
    return 1;
  if (!u)
    return 1;
  if (u->flags & (USER_BOT | USER_COMMON))
    return 1;
  if (!par[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick,
	    u_pass_match(u, "-") ? IRC_NOPASS : IRC_PASS);
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! PASS?", nick, host, u->handle);
    return 1;
  }
  old = newsplit(&par);
  if (!u_pass_match(u, "-") && !par[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_EXISTPASS);
    return 1;
  }
  if (par[0]) {
    if (!u_pass_match(u, old)) {
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_FAILPASS);
      return 1;
    }
    new = newsplit(&par);
  } else {
    new = old;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! PASS...", nick, host, u->handle);
  if (strlen(new) > 15)
    new[15] = 0;
  if (strlen(new) < 6) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_PASSFORMAT);
    return 0;
  }
  set_user(&USERENTRY_PASS, u, new);
  dprintf(DP_HELP, "NOTICE %s :%s '%s'.\n", nick,
	  new == old ? IRC_SETPASS : IRC_CHANGEPASS, new);
  return 1;
}

static int msg_ident(char *nick, char *host, struct userrec *u, char *par)
{
  char s[121], s1[121], *pass, who[NICKLEN];
  struct userrec *u2;
  memberlist *mx;
  if (match_my_nick(nick))
    return 1;
  if (u && (u->flags & USER_BOT))
    return 1;
  if (u && (u->flags & USER_COMMON)) {
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_FAILCOMMON);
    return 1;
  }
  pass = newsplit(&par);
  if (!par[0])
    strcpy(who, nick);
  else {
    strncpy(who, par, NICKMAX);
    who[NICKMAX] = 0;
  }
  u2 = get_user_by_handle(userlist, who);
  if (!u2) {
    if (u && !quiet_reject) {
      dprintf(DP_HELP, IRC_MISIDENT, IRC_MISIDENT_ARGS);
    }
  } else if (rfc_casecmp(who, origbotname) && !(u2->flags & USER_BOT)) {
    /* This could be used as detection... */
    if (u_pass_match(u2, "-")) {
      putlog(LOG_CMDS, "*", "(%s!%s) !*! IDENT %s", nick, host, who);
      if (!quiet_reject)
	dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_NOPASS);
    } else if (!u_pass_match(u2, pass)) {
      if (!quiet_reject)
	dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_DENYACCESS);
    } else if (u == u2) {
      if (!quiet_reject)
	dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_RECOGNIZED);
      return 1;
    } else if (u) {
      if (!quiet_reject)
	dprintf(DP_HELP, IRC_MISIDENT, IRC_MISIDENT_ARGS2);
      return 1;
    } else {
      struct chanset_t *chan = chanset;
      struct flag_record fr =
      {
	FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0
      };

      putlog(LOG_CMDS, "*", "(%s!%s) !*! IDENT %s", nick, host, who);
      simple_sprintf(s, "%s!%s", nick, host);
      maskhost(s, s1);
      dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, IRC_ADDHOSTMASK, s1);
      addhost_by_handle(who, s1);
      while (chan) {
	get_user_flagrec(u2, &fr, chan->name);
	/* is the channel or the user marked auto-op? */
	if ((channel_autoop(chan) || glob_autoop(fr) || chan_autoop(fr)) &&
           (mx = ismember(chan, nick)) && !chan_hasop(mx) && !chan_sentop(mx) &&	
           /* are they actually validly +o ? */
	   (chan_op(fr) || (glob_op(fr) && !chan_deop(fr)))) {
	  add_mode(chan, '+', 'o', nick);
          mx->flags |= SENTOP;
        }
	chan = chan->next;
      }
      return 1;
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !*! failed IDENT %s", nick, host, who);
  return 1;
}

static int msg_addhost(char *nick, char *host, struct userrec *u, char *par)
{
  char *pass;

  if (match_my_nick(nick))
    return 1;
  if (!u || (u->flags & USER_BOT))
    return 1;
  if (u->flags & USER_COMMON) {
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_FAILCOMMON);
    return 1;
  }
  pass = newsplit(&par);
  if (!par[0]) {
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :You must supply a hostmask\n", nick);
  } else if (rfc_casecmp(u->handle, origbotname)) {
    /* This could be used as detection... */
    if (u_pass_match(u, "-")) {
      if (!quiet_reject)
	dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_NOPASS);
    } else if (!u_pass_match(u, pass)) {
      if (!quiet_reject)
	dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_DENYACCESS);
    } else if (get_user_by_host(par)) {
      if (!quiet_reject)
	dprintf(DP_HELP, "NOTICE %s :That hostmask clashes with another already in use.\n", nick);
    } else {
      putlog(LOG_CMDS, "*", "(%s!%s) !*! ADDHOST %s", nick, host, par);
      dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, IRC_ADDHOSTMASK, par);
      addhost_by_handle(u->handle, par);
      return 1;
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !*! failed ADDHOST %s", nick, host, par);
  return 1;
}

static int msg_info(char *nick, char *host, struct userrec *u, char *par)
{
  char s[121], *pass, *chname, *p;

  int locked = 0;

  if (match_my_nick(nick))
    return 1;
  if (!use_info)
    return 1;
  if (!u)
    return 0;
  if (u->flags & (USER_COMMON | USER_BOT))
    return 1;
  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed INFO", nick, host, u->handle);
      return 1;
    }
  }
  if (par[0] && (strchr(CHANMETA, par[0]) != NULL)) {
    if (!findchan(chname = newsplit(&par))) {
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_NOMONITOR);
      return 1;
    }
  } else
    chname = 0;
  if (par[0]) {
    p = get_user(&USERENTRY_INFO, u);
    if (p && (p[0] == '@'))
      locked = 1;
    if (chname) {
      get_handle_chaninfo(u->handle, chname, s);
      if (s[0] == '@')
	locked = 1;
    }
    if (locked) {
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_INFOLOCKED);
      return 1;
    }
    if (!strcasecmp(par, "none")) {
      par[0] = 0;
      if (chname) {
	set_handle_chaninfo(userlist, u->handle, chname, NULL);
	dprintf(DP_HELP, "NOTICE %s :%s %s.\n", nick, IRC_REMINFOON, chname);
	putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO %s NONE", nick, host,
	       u->handle, chname);
      } else {
	set_user(&USERENTRY_INFO, u, NULL);
	dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_REMINFO);
	putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO NONE", nick, host, u->handle);
      }
      return 1;
    }
    if (par[0] == '@')
      par++;
    dprintf(DP_HELP, "NOTICE %s :%s %s\n", nick, IRC_FIELDCHANGED, par);
    if (chname) {
      set_handle_chaninfo(userlist, u->handle, chname, par);
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO %s ...", nick, host, u->handle, chname);
    } else {
      set_user(&USERENTRY_INFO, u, par);
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO ...", nick, host, u->handle);
    }
    return 1;
  }
  if (chname) {
    get_handle_chaninfo(u->handle, chname, s);
    p = s;
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO? %s", nick, host, u->handle,
	   chname);
  } else {
    p = get_user(&USERENTRY_INFO, u);
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO?", nick, host, u->handle);
  }
  if (p && p[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s %s\n", nick, IRC_FIELDCURRENT, p);
    dprintf(DP_HELP, "NOTICE %s :%s /msg %s info <pass>%s%s none\n",
	    nick, IRC_FIELDTOREMOVE, botname, chname ? " " : "", chname
	    ? chname : "");
  } else {
    if (chname)
      dprintf(DP_HELP, "NOTICE %s :%s %s.\n", nick, IRC_NOINFOON, chname);
    else
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_NOINFO);
  }
  return 1;
}

static int msg_who(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  memberlist *m;
  char s[UHOSTLEN], also[512], *info;
  int i;

  if (match_my_nick(nick))
    return 1;
  if (!u)
    return 0;
  if (!use_info)
    return 1;
  if (!par[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s: /msg %s who <channel>\n", nick,
	    USAGE, botname);
    return 0;
  }
  chan = findchan(par);
  if (!chan) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_NOMONITOR);
    return 0;
  }
  get_user_flagrec(u, &fr, par);
  if (channel_hidden(chan) && !hand_on_chan(chan, u) &&
      !glob_op(fr) && !glob_friend(fr) &&
      !chan_op(fr) && !chan_friend(fr)) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_CHANHIDDEN);
    return 1;
  }
  context;
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! WHO", nick, host, u->handle);
  also[0] = 0;
  i = 0;
  m = chan->channel.member;
  while (m->nick[0]) {
    struct userrec *u;

    simple_sprintf(s, "%s!%s", m->nick, m->userhost);
    u = get_user_by_host(s);
    info = get_user(&USERENTRY_INFO, u);
    if (u && (u->flags & USER_BOT))
      info = 0;
    if (info && (info[0] == '@'))
      info++;
    else if (u) {
      get_handle_chaninfo(u->handle, chan->name, s);
      if (s[0]) {
	info = s;
	if (info[0] == '@')
	  info++;
      }
    }
    if (info && info[0])
      dprintf(DP_HELP, "NOTICE %s :[%9s] %s\n", nick, m->nick, info);
    else {
      if (match_my_nick(m->nick))
	dprintf(DP_HELP, "NOTICE %s :[%9s] <-- I'm the bot, of course.\n",
		nick, m->nick);
      else if (u && (u->flags & USER_BOT)) {
	if (bot_flags(u) & BOT_SHARE)
	  dprintf(DP_HELP, "NOTICE %s :[%9s] <-- a twin of me\n",
		  nick, m->nick);
	else
	  dprintf(DP_HELP, "NOTICE %s :[%9s] <-- another bot\n",
		  nick, m->nick);
      } else {
	if (i) {
	  also[i++] = ',';
	  also[i++] = ' ';
	}
	i += my_strcpy(also + i, m->nick);
	if (i > 400) {
	  dprintf(DP_HELP, "NOTICE %s :No info: %s\n", nick, also);
	  i = 0;
	  also[0] = 0;
	}
      }
    }
    m = m->next;
  }
  if (i) {
    dprintf(DP_HELP, "NOTICE %s :No info: %s\n", nick, also);
  }
  context;
  return 1;
}

static int msg_whois(char *nick, char *host, struct userrec *u, char *par)
{
  char s[161], s1[81], *s2;
  int ok;
  struct chanset_t *chan;
  memberlist *m;
  struct chanuserrec *cr;
  struct userrec *u2;
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  struct xtra_key *xk;
  time_t tt = 0;

  if (match_my_nick(nick))
    return 1;
  if (!u)
    return 0;
  if (strlen(par) > NICKMAX)
    par[NICKMAX] = 0;
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! WHOIS %s", nick, host, u->handle, par);
  u2 = get_user_by_handle(userlist, par);
  if (!u2) {
    /* no such handle -- maybe it's a nickname of someone on a chan? */
    ok = 0;
    chan = chanset;
    while (chan && !ok) {
      m = ismember(chan, par);
      if (m) {
	simple_sprintf(s, "%s!%s", par, m->userhost);
	u2 = get_user_by_host(s);
	if (u2) {
	  ok = 1;
	  dprintf(DP_HELP, "NOTICE %s :[%s] AKA '%s':\n", nick,
		  par, u2->handle);
	}
      }
      chan = chan->next;
    }
    if (!ok) {
      dprintf(DP_HELP, "NOTICE %s :[%s] %s\n", nick, par, USERF_NOUSERREC);
      return 1;
    }
  }
  s2 = get_user(&USERENTRY_INFO, u2);
  if (s2 && (s2[0] == '@'))
    s2++;
  if (s2 && s2[0] && !(u2->flags & USER_BOT))
    dprintf(DP_HELP, "NOTICE %s :[%s] %s\n", nick, u2->handle, s2);
  for (xk = get_user(&USERENTRY_XTRA, u2); xk; xk = xk->next)
    if (!strcasecmp(xk->key, "EMAIL"))
      dprintf(DP_HELP, "NOTICE %s :[%s] email: %s\n", nick, u2->handle,
	      xk->data);
  ok = 0;
  chan = chanset;
  while (chan != NULL) {
    if (hand_on_chan(chan, u2)) {
      sprintf(s1, "NOTICE %s :[%s] %s %s.", nick, u2->handle,
	      IRC_ONCHANNOW, chan->name);
      ok = 1;
    } else {
      get_user_flagrec(u, &fr, chan->name);
      cr = get_chanrec(u2, chan->name);
      if (cr && (cr->laston > tt) &&
	  (!channel_hidden(chan) ||
	   hand_on_chan(chan, u) ||
	   (glob_op(fr) && !chan_deop(fr)) ||
	   glob_friend(fr) || chan_op(fr) || chan_friend(fr))) {
	tt = cr->laston;
	strcpy(s, ctime(&tt));
	strcpy(s, &s[4]);
	s[12] = 0;
	ok = 1;
	sprintf(s1, "NOTICE %s :[%s] %s %s on %s", nick, u2->handle,
		IRC_LASTSEENAT, s, chan->name);
      }
    }
    chan = chan->next;
  }
  if (!ok)
    sprintf(s1, "NOTICE %s :[%s] %s", nick, u2->handle, IRC_NEVERJOINED);
  if (u2->flags & USER_OP)
    strcat(s1, USER_ISGLOBALOP);
  if (u2->flags & USER_BOT)
    strcat(s1, USER_ISBOT);
  if (u2->flags & USER_MASTER)
    strcat(s1, USER_ISMASTER);
  dprintf(DP_HELP, "%s\n", s1);
  return 1;
}

static int msg_help(char *nick, char *host, struct userrec *u, char *par)
{
  char *p;

  if (match_my_nick(nick))
    return 1;
  if (!u) {
    if (!quiet_reject) {
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_DONTKNOWYOU);
      dprintf(DP_HELP, "NOTICE %s :/MSG %s hello\n", nick, botname);
    }
    return 0;
  }
  if (helpdir[0]) {
    struct flag_record fr =
    {FR_ANYWH | FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

    get_user_flagrec(u, &fr, 0);
    if (!par[0])
      showhelp(nick, "help", &fr, 0);
    else {
      for (p = par; *p != 0; p++)
	if ((*p >= 'A') && (*p <= 'Z'))
	  *p += ('a' - 'A');
      showhelp(nick, par, &fr, 0);
    }
  } else
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_NOHELP);
  return 1;
}

/* i guess just op them on every channel they're on */
static int msg_op(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  if (match_my_nick(nick))
    return 1;
  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    /* Prevent people from gaining ops when no password set */
    if (!u_pass_match(u, "-")) {
      if (par[0]) {
	chan = findchan(par);
	if (chan && channel_active(chan)) {
	  get_user_flagrec(u, &fr, par);
	  if (chan_op(fr) || (glob_op(fr) && !chan_deop(fr)))
	    add_mode(chan, '+', 'o', nick);
	    putlog(LOG_CMDS, "*", "(%s!%s) !%s! OP %s",
		   nick, host, u->handle, par);
	  return 1;
	}
      } else {
	chan = chanset;
	while (chan != NULL) {
	  get_user_flagrec(u, &fr, chan->name);
	  if (chan_op(fr) || (glob_op(fr) && !chan_deop(fr)))
	    add_mode(chan, '+', 'o', nick);
	  chan = chan->next;
	}
	putlog(LOG_CMDS, "*", "(%s!%s) !%s! OP", nick, host, u->handle);
	return 1;
      }
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed OP", nick, host, nick);
  return 1;
}

static int msg_key(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  if (match_my_nick(nick))
    return 1;
  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    /* Prevent people from getting key with no pass set */
    if (!u_pass_match(u, "-")) {
      if (!(chan = findchan(par))) {
	dprintf(DP_HELP, "NOTICE %s :%s: /MSG %s key <pass> <channel>\n",
		nick, USAGE, botname);
	return 1;
      }
      if (!channel_active(chan)) {
	dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, par, IRC_NOTONCHAN);
	return 1;
      }
      chan = findchan(par);
      if (chan && channel_active(chan)) {
	get_user_flagrec(u, &fr, par);
	if (chan_op(fr) || (glob_op(fr) && !chan_deop(fr))) {
	  if (chan->channel.key[0]) {
	    dprintf(DP_SERVER, "NOTICE %s :%s: key is %s\n", nick, par,
		    chan->channel.key);
	    if (invite_key && (chan->channel.mode & CHANINV)) {
	      dprintf(DP_SERVER, "INVITE %s %s\n", nick, par);
	      putlog(LOG_CMDS, "*", "(%s!%s) !%s! KEY %s",
		     nick, host, u->handle, par);
	    }
	  } else {
	    dprintf(DP_HELP, "NOTICE %s :%s: no key set for this channel\n", nick, par);
	    putlog(LOG_CMDS, "*", "(%s!%s) !%s! KEY %s",
	         nick, host, u->handle, par);
	  }
	}
	return 1;
      }
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed KEY", nick, host, nick);
  return 1;
}

/* dont have to specify a channel now and can use this command
 * regardless of +autovoice or being a chanop. (guppy 7Jan1999) */
static int msg_voice(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  if (match_my_nick(nick))
    return 1;
  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    if (!u_pass_match(u, "-")) {
      if (par[0]) {
	chan = findchan(par);
	if (chan && channel_active(chan)) {
	  get_user_flagrec(u, &fr, par);
	  if (chan_voice(fr) || glob_voice(fr))
	    add_mode(chan, '+', 'v', nick);
	    putlog(LOG_CMDS, "*", "(%s!%s) !%s! VOICE %s",
		   nick, host, u->handle, par);
	  return 1;
	}
      } else {
	chan = chanset;
	while (chan != NULL) {
	  get_user_flagrec(u, &fr, chan->name);
	  if (chan_voice(fr) || glob_voice(fr))
	    add_mode(chan, '+', 'v', nick);
	  chan = chan->next;
	}
	putlog(LOG_CMDS, "*", "(%s!%s) !%s! VOICE", nick, host, u->handle);
	return 1;
      }
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed VOICE", nick, host, nick);
  return 1;
}

static int msg_invite(char *nick, char *host, struct userrec *u, char *par)
{
  char *pass;
  struct chanset_t *chan;
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  if (match_my_nick(nick))
    return 1;
  if (!u)
    return 0;
  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    if (par[0] == '*') {
      for (chan = chanset; chan; chan = chan->next) {
	get_user_flagrec(u, &fr, chan->name);
	if ((chan_op(fr) || (glob_op(fr) && !chan_deop(fr))) &&
	    (chan->channel.mode & CHANINV))
	  dprintf(DP_SERVER, "INVITE %s %s\n", nick, chan->name);
      }
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! INVITE ALL", nick, host,
	     u->handle);
      return 1;
    }
    if (!(chan = findchan(par))) {
      dprintf(DP_HELP, "NOTICE %s :%s: /MSG %s invite <pass> <channel>\n",
	      nick, USAGE, botname);
      return 1;
    }
    if (!channel_active(chan)) {
      dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, par, IRC_NOTONCHAN);
      return 1;
    }
    dprintf(DP_SERVER, "INVITE %s %s\n", nick, par);
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! INVITE %s", nick, host,
	   u->handle, par);
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed INVITE %s", nick, host,
	 u->handle, par);
  return 1;
}

static int msg_status(char *nick, char *host, struct userrec *u, char *par)
{
  char s[256];
  char *ve_t, *un_t;
  char *pass;
  int i, l;
  struct chanset_t *chan;

#ifdef HAVE_UNAME
  struct utsname un;

  if (!uname(&un) < 0) {
#endif
    ve_t = " ";
    un_t = "*unkown*";
#ifdef HAVE_UNAME
  } else {
    ve_t = un.release;
    un_t = un.sysname;
  }
#endif

  if (match_my_nick(nick))
    return 1;
  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed STATUS", nick, host,
	     u->handle);
      return 1;
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! STATUS", nick, host, u->handle);
  dprintf(DP_HELP, "NOTICE %s :I am %s, running %s.\n", nick, botnetnick,
	  Version);
  dprintf(DP_HELP, "NOTICE %s :Running on %s %s\n", nick, un_t, ve_t);
  if (admin[0])
    dprintf(DP_HELP, "NOTICE %s :Admin: %s\n", nick, admin);
  /* Fixed previous lame code. Well it's still lame, will overflow the
   * buffer with a long channel-name. <cybah> */
  context;
  strcpy(s, "Channels: ");
  l = 10;
  for (chan = chanset; chan; chan = chan->next) {
    l += my_strcpy(s + l, chan->name);
    if (!channel_active(chan))
      l += my_strcpy(s + l, " (trying)");
    else if (channel_pending(chan))
      l += my_strcpy(s + l, " (pending)");
    else if (!me_op(chan))
      l += my_strcpy(s + l, " (want ops!)");
    s[l++] = ',';
    s[l++] = ' ';
    if (l > 70) {
      s[l] = 0;
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, s);
      strcpy(s, "          ");
      l = 10;
    }
  }
  if (l > 10) {
    s[l] = 0;
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, s);
  }
  context;
  i = count_users(userlist);
  dprintf(DP_HELP, "NOTICE %s :%d user%s  (mem: %uk)\n", nick, i, i == 1 ? "" : "s",
	  (int) (expected_memory() / 1024));
  /* waiting for beldin to make it extern */
  daysdur(now, server_online, s);
  dprintf(DP_HELP, "NOTICE %s :Connected %s\n", nick, s);
  dprintf(DP_HELP, "NOTICE %s :Online as: %s!%s\n", nick, botname,
	  botuserhost);
  return 1;
}

static int msg_memory(char *nick, char *host, struct userrec *u, char *par)
{
  char *pass;

  if (match_my_nick(nick))
    return 1;
  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed MEMORY", nick, host,
	     u->handle);
      return 1;
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! MEMORY", nick, host, u->handle);
  tell_mem_status(nick);
  return 1;
}

static int msg_die(char *nick, char *host, struct userrec *u, char *par)
{
  char s[1024];
  char *pass;

  if (match_my_nick(nick))
    return 1;
  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed DIE", nick, host, u->handle);
      return 1;
    }
  } else {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed DIE", nick, host, u->handle);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_NOPASS);
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! DIE", nick, host, u->handle);
  dprintf(-serv, "NOTICE %s :%s\n", nick, BOT_MSGDIE);
  simple_sprintf(s, "BOT SHUTDOWN (authorized by %s)", u->handle);
  chatout("*** %s\n", s);
  botnet_send_chat(-1, botnetnick, s);
  botnet_send_bye();
  if (!par[0]) {
    nuke_server(nick);
  } else {
    nuke_server(par);
  }
  write_userfile(-1);
  sleep(1);			/* give the server time to understand */
  sprintf(s, "DEAD BY REQUEST OF %s!%s", nick, host);
  fatal(s, 0);
  return 1;
}

static int msg_rehash(char *nick, char *host, struct userrec *u, char *par)
{
  if (match_my_nick(nick))
    return 1;
  if (u_pass_match(u, par)) {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! REHASH", nick, host, u->handle);
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, USERF_REHASHING);
    if (make_userfile)
      make_userfile = 0;
    write_userfile(-1);
    do_restart = -2;
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed REHASH", nick, host, u->handle);
  return 1;
}

static int msg_save(char *nick, char *host, struct userrec *u, char *par)
{
  if (match_my_nick(nick))
    return 1;
  if (u_pass_match(u, par)) {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! SAVE", nick, host, u->handle);
    dprintf(DP_HELP, "NOTICE %s :Saving user file...\n", nick);
    write_userfile(-1);
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed SAVE", nick, host, u->handle);
  return 1;
}

static int msg_reset(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;

  if (match_my_nick(nick))
    return 1;
  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed RESET", nick, host,
	     u->handle);
      return 1;
    }
  } else {
    putlog(LOG_CMDS, "*", "(%s!%s) !*! failed RESET", nick, host, u->handle);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_NOPASS);
    return 1;
  }
  if (par[0]) {
    chan = findchan(par);
    if (!chan) {
      dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, par, IRC_NOMONITOR);
      return 0;
    }
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! RESET %s", nick, host, u->handle, par);
    dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, par, IRC_RESETCHAN);
    reset_chan_info(chan);
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! RESET ALL", nick, host, u->handle);
  dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_RESETCHAN);
  chan = chanset;
  while (chan != NULL) {
    reset_chan_info(chan);
    chan = chan->next;
  }
  return 1;
}

static int msg_go(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  int ok = 0, ok2 = 0;
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  if (match_my_nick(nick))
    return 1;
  if (!u)
    return 0;
  if (par[0]) {
    chan = findchan(par);
    if (!chan)
      return 0;
    if (!(chan->status & CHAN_ACTIVE)) {
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed GO (i'm blind)", nick, host,
	     u->handle);
      return 1;
    }
    get_user_flagrec(u, &fr, par);
    if (!chan_op(fr) && !(glob_op(fr) && !chan_deop(fr))) {
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed GO (not op)", nick, host,
	     u->handle);
      return 1;
    }
    if (!me_op(chan)) {
      dprintf(DP_SERVER, "PART %s\n", chan->name);
      putlog(LOG_CMDS, chan->name, "(%s!%s) !%s! GO %s", nick, host,
	     u->handle, par);
      return 1;
    }
    putlog(LOG_CMDS, chan->name, "(%s!%s) !%s! failed GO %s (i'm chop)",
	   nick, host, u->handle, par);
    return 1;
  }
  chan = chanset;
  while (chan != NULL) {
    if (ismember(chan, nick)) {
      get_user_flagrec(u, &fr, par);
      if (chan_op(fr) || (glob_op(fr) && !chan_deop(fr))) {
	ok2 = 1;
	if (!me_op(chan)) {
	  dprintf(DP_SERVER, "PART %s\n", chan->name);
	  ok = 1;
	}
      }
    }
    chan = chan->next;
  }
  if (ok) {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! GO", nick, host, u->handle);
  } else if (ok2) {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed GO (i'm chop)", nick, host,
	   u->handle);
  } else {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed GO (not op)", nick, host,
	   u->handle);
  }
  return 1;
}

static int msg_jump(char *nick, char *host, struct userrec *u, char *par)
{
  char *s;
  int port;

  if (match_my_nick(nick))
    return 1;
  if (u_pass_match(u, "-")) {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed JUMP", nick, host, u->handle);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, IRC_NOPASS);
    return 1;
  }
  s = newsplit(&par);		/* password */
  if (u_pass_match(u, s)) {
    if (par[0]) {
      s = newsplit(&par);
      port = atoi(newsplit(&par));
      if (!port)
	port = default_port;
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! JUMP %s %d %s", nick, host,
	     u->handle, s, port, par);
      strcpy(newserver, s);
      newserverport = port;
      strcpy(newserverpass, par);
    } else
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! JUMP", nick, host, u->handle);
    dprintf(-serv, "NOTICE %s :%s\n", nick, IRC_JUMP);
    cycle_time = 0;
    nuke_server("changing servers");
  } else
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed JUMP", nick, host, u->handle);
  return 1;
}

/* MSG COMMANDS */
/* function call should be:
 * int msg_cmd("handle","nick","user@host","params");
 * function is responsible for any logging
 * (return 1 if successful, 0 if not) */
/* update the add/rem_builtins in irc.c if you add to this list!! */
static cmd_t C_msg[] =
{
  {"addhost", "", (Function) msg_addhost, NULL},
  {"die", "n", (Function) msg_die, NULL},
  {"go", "", (Function) msg_go, NULL},
  {"hello", "", (Function) msg_hello, NULL},
  {"help", "", (Function) msg_help, NULL},
  {"ident", "", (Function) msg_ident, NULL},
  {"info", "", (Function) msg_info, NULL},
  {"invite", "o|o", (Function) msg_invite, NULL},
  {"jump", "m", (Function) msg_jump, NULL},
  {"key", "o|o", (Function) msg_key, NULL},
  {"memory", "m", (Function) msg_memory, NULL},
  {"op", "", (Function) msg_op, NULL},
  {"pass", "", (Function) msg_pass, NULL},
  {"rehash", "m", (Function) msg_rehash, NULL},
  {"reset", "m", (Function) msg_reset, NULL},
  {"save", "m", (Function) msg_save, NULL},
  {"status", "m|m", (Function) msg_status, NULL},
  {"voice", "", (Function) msg_voice, NULL},
  {"who", "", (Function) msg_who, NULL},
  {"whois", "", (Function) msg_whois, NULL},
  {0, 0, 0, 0}
};
