/* 
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

static time_t last_ctcp = (time_t) 0L;
static int count_ctcp = 0;

/* shrug (guppy:24Feb1999) */
static int gotfake433(char *from)
{
  char c, *oknicks = "^-_\\[]`", *p, *alt = get_altbotnick();

  context;
  /* alternate nickname defined? */
  if ((alt[0]) && (rfc_casecmp(alt, botname)))
    strcpy(botname, alt);
  /* if alt nickname failed, drop thru to here */
  else {
    int l = strlen(botname) - 1;

    c = botname[l];
    p = strchr(oknicks, c);
    if (((c >= '0') && (c <= '9')) || (p != NULL)) {
      if (p == NULL) {
	if (c == '9')
	  botname[l] = oknicks[0];
	else
	  botname[l] = c + 1;
      } else {
	p++;
	if (!*p)
	  botname[l] = 'a' + random() % 26;
	else
	  botname[l] = (*p);
      }
    } else {
      if (l + 1 == NICKLEN)
	botname[l] = '0';
      else {
	botname[++l] = '0';
	botname[++l] = 0;
      }
    }
  }
  putlog(LOG_MISC, "*", IRC_BOTNICKINUSE, botname);
  dprintf(DP_MODE, "NICK %s\n", botname);
  return 0;
}

/* check for tcl-bound msg command, return 1 if found */
/* msg: proc-name <nick> <user@host> <handle> <args...> */
static int check_tcl_msg(char *cmd, char *nick, char *uhost,
			 struct userrec *u, char *args)
{
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};
  char *hand = u ? u->handle : "*";
  int x;

  context;
  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_msg1", nick, 0);
  Tcl_SetVar(interp, "_msg2", uhost, 0);
  Tcl_SetVar(interp, "_msg3", hand, 0);
  Tcl_SetVar(interp, "_msg4", args, 0);
  context;
  x = check_tcl_bind(H_msg, cmd, &fr, " $_msg1 $_msg2 $_msg3 $_msg4",
		     MATCH_PARTIAL | BIND_HAS_BUILTINS | BIND_USE_ATTR);
  context;
  if (x == BIND_EXEC_LOG)
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! %s %s", nick, uhost, hand,
	   cmd, args);
  return ((x == BIND_MATCHED) || (x == BIND_EXECUTED) || (x == BIND_EXEC_LOG));
}

static void check_tcl_notc(char *nick, char *uhost, struct userrec *u, char *arg)
{
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

  context;
  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_notc1", nick, 0);
  Tcl_SetVar(interp, "_notc2", uhost, 0);
  Tcl_SetVar(interp, "_notc3", u ? u->handle : "*", 0);
  Tcl_SetVar(interp, "_notc4", arg, 0);
  context;
  check_tcl_bind(H_notc, arg, &fr, " $_notc1 $_notc2 $_notc3 $_notc4",
		 MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE);
  context;
}

static void check_tcl_msgm(char *cmd, char *nick, char *uhost,
			   struct userrec *u, char *arg)
{
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};
  char args[1024];

  context;
  if (arg[0])
    simple_sprintf(args, "%s %s", cmd, arg);
  else
    strcpy(args, cmd);
  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_msgm1", nick, 0);
  Tcl_SetVar(interp, "_msgm2", uhost, 0);
  Tcl_SetVar(interp, "_msgm3", u ? u->handle : "*", 0);
  Tcl_SetVar(interp, "_msgm4", args, 0);
  context;
  check_tcl_bind(H_msgm, args, &fr, " $_msgm1 $_msgm2 $_msgm3 $_msgm4",
		 MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE);
  context;
}

/* return 1 if processed */
static int check_tcl_raw(char *from, char *code, char *msg)
{
  int x;

  context;
  Tcl_SetVar(interp, "_raw1", from, 0);
  Tcl_SetVar(interp, "_raw2", code, 0);
  Tcl_SetVar(interp, "_raw3", msg, 0);
  context;
  x = check_tcl_bind(H_raw, code, 0, " $_raw1 $_raw2 $_raw3",
		     MATCH_EXACT | BIND_STACKABLE | BIND_WANTRET);
  context;
  return (x == BIND_EXEC_LOG);
}

static int check_tcl_ctcpr(char *nick, char *uhost, struct userrec *u,
			   char *dest, char *keyword, char *args,
			   p_tcl_bind_list table)
{
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};
  int x;

  context;
  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_ctcpr1", nick, 0);
  Tcl_SetVar(interp, "_ctcpr2", uhost, 0);
  Tcl_SetVar(interp, "_ctcpr3", u ? u->handle : "*", 0);
  Tcl_SetVar(interp, "_ctcpr4", dest, 0);
  Tcl_SetVar(interp, "_ctcpr5", keyword, 0);
  Tcl_SetVar(interp, "_ctcpr6", args, 0);
  x = check_tcl_bind(table, keyword, &fr,
		" $_ctcpr1 $_ctcpr2 $_ctcpr3 $_ctcpr4 $_ctcpr5 $_ctcpr6",
		     (lowercase_ctcp ? MATCH_EXACT : MATCH_CASE)
		     | BIND_USE_ATTR | BIND_STACKABLE |
		     ((table == H_ctcp) ? BIND_WANTRET : 0));
  context;
  return (x == BIND_EXEC_LOG) || (table == H_ctcr);
}

static int check_tcl_wall(char *from, char *msg)
{
  int x;

  context;
  Tcl_SetVar(interp, "_wall1", from, 0);
  Tcl_SetVar(interp, "_wall2", msg, 0);
  context;
  x = check_tcl_bind(H_wall, msg, 0, " $_wall1 $_wall2", MATCH_MASK | BIND_STACKABLE);
  context;
  if (x == BIND_EXEC_LOG) {
    putlog(LOG_WALL, "*", "!%s! %s", from, msg);
    return 1;
  } else
    return 0;
}


static int check_tcl_flud(char *nick, char *uhost, struct userrec *u,
			  char *ftype, char *chname)
{
  int x;

  context;
  Tcl_SetVar(interp, "_flud1", nick, 0);
  Tcl_SetVar(interp, "_flud2", uhost, 0);
  Tcl_SetVar(interp, "_flud3", u ? u->handle : "*", 0);
  Tcl_SetVar(interp, "_flud4", ftype, 0);
  Tcl_SetVar(interp, "_flud5", chname, 0);
  context;
  x = check_tcl_bind(H_flud, ftype, 0,
		     " $_flud1 $_flud2 $_flud3 $_flud4 $_flud5",
		     MATCH_MASK | BIND_STACKABLE | BIND_WANTRET);
  context;
  return (x == BIND_EXEC_LOG);
}

static int match_my_nick(char *nick)
{
  if (!rfc_casecmp(nick, botname))
    return 1;
  return 0;
}

/* 001: welcome to IRC (use it to fix the server name) */
static int got001(char *from, char *msg)
{
  struct server_list *x;
  int i, servidx = findanyidx(serv);
  struct chanset_t *chan;

  /* ok...param #1 of 001 = what server thinks my nick is */
  server_online = now;
  fixcolon(msg);
  strncpy(botname, msg, NICKMAX);
  botname[NICKMAX] = 0;
  /* init-server */
  if (initserver[0])
    do_tcl("init-server", initserver);
  x = serverlist;
  if (x == NULL)
    return 0;			/* uh, no server list */
  /* below makes a mess of DEBUG_OUTPUT can we do something else?
   * buggerit, 1 channel at a time is much neater
   * only do it if the IRC module is loaded */
  if (module_find("irc", 0, 0))
    for (chan = chanset; chan; chan = chan->next) {
      chan->status &= ~(CHAN_ACTIVE | CHAN_PEND);
      if (!channel_inactive(chan))
	dprintf(DP_SERVER, "JOIN %s %s\n", chan->name, chan->key_prot);
    }
  if (strcasecmp(from, dcc[servidx].host)) {
    putlog(LOG_MISC, "*", "(%s claims to be %s; updating server list)",
	   dcc[servidx].host, from);
    for (i = curserv; i > 0 && x != NULL; i--)
      x = x->next;
    if (x == NULL) {
      putlog(LOG_MISC, "*", "Invalid server list!");
      return 0;
    }
    if (x->realname)
      nfree(x->realname);
    if (strict_servernames == 1) {
      x->realname = NULL;
      if (x->name)
	nfree(x->name);
      x->name = nmalloc(strlen(from) + 1);
      strcpy(x->name, from);
    } else {
      x->realname = nmalloc(strlen(from) + 1);
      strcpy(x->realname, from);
    }
  }
  return 0;
}

/* close the current server connection */
static void nuke_server(char *reason)
{
  if (serv >= 0) {
    int servidx = findanyidx(serv);

    server_online = 0;
    if (reason && (servidx > 0))
      dprintf(servidx, "QUIT :%s\n", reason);
    lostdcc(servidx);
  }
}

static char ctcp_reply[1024] = "";

static int lastmsgs[FLOOD_GLOBAL_MAX];
static char lastmsghost[FLOOD_GLOBAL_MAX][81];
static time_t lastmsgtime[FLOOD_GLOBAL_MAX];

/* do on NICK, PRIVMSG, and NOTICE -- and JOIN */
static int detect_flood(char *floodnick, char *floodhost,
			char *from, int which)
{
  char *p;
  char ftype[10], h[1024];
  struct userrec *u;
  int thr = 0, lapse = 0;
  int atr;

  context;
  u = get_user_by_host(from);
  atr = u ? u->flags : 0;
  if (atr & (USER_BOT | USER_FRIEND))
    return 0;
  /* determine how many are necessary to make a flood */
  switch (which) {
  case FLOOD_PRIVMSG:
  case FLOOD_NOTICE:
    thr = flud_thr;
    lapse = flud_time;
    strcpy(ftype, "msg");
    break;
  case FLOOD_CTCP:
    thr = flud_ctcp_thr;
    lapse = flud_ctcp_time;
    strcpy(ftype, "ctcp");
    break;
  }
  if ((thr == 0) || (lapse == 0))
    return 0;			/* no flood protection */
  /* okay, make sure i'm not flood-checking myself */
  if (match_my_nick(floodnick))
    return 0;
  if (!strcasecmp(floodhost, botuserhost))
    return 0;			/* my user@host (?) */
  p = strchr(floodhost, '@');
  if (p) {
    p++;
    if (strcasecmp(lastmsghost[which], p)) {	/* new */
      strcpy(lastmsghost[which], p);
      lastmsgtime[which] = now;
      lastmsgs[which] = 0;
      return 0;
    }
  } else
    return 0;			/* uh... whatever. */
  if (lastmsgtime[which] < now - lapse) {
    /* flood timer expired, reset it */
    lastmsgtime[which] = now;
    lastmsgs[which] = 0;
    return 0;
  }
  lastmsgs[which]++;
  if (lastmsgs[which] >= thr) {	/* FLOOD */
    /* reset counters */
    lastmsgs[which] = 0;
    lastmsgtime[which] = 0;
    lastmsghost[which][0] = 0;
    u = get_user_by_host(from);
    if (check_tcl_flud(floodnick, from, u, ftype, "*"))
      return 0;
    /* private msg */
    simple_sprintf(h, "*!*@%s", p);
    putlog(LOG_MISC, "*", IRC_FLOODIGNORE1, p);
    addignore(h, origbotname, (which == FLOOD_CTCP) ? "CTCP flood" :
	      "MSG/NOTICE flood", now + (60 * ignore_time));
    if (use_silence)
      /* attempt to use ircdu's SILENCE command */
      dprintf(DP_MODE, "SILENCE +*@%s\n", p);
  }
  return 0;
}

/* check for more than 8 control characters in a line
 * this could indicate: beep flood CTCP avalanche */
static int detect_avalanche(char *msg)
{
  int count = 0;
  unsigned char *p;

  for (p = (unsigned char *) msg; (*p) && (count < 8); p++)
    if ((*p == 7) || (*p == 1))
      count++;
  if (count >= 8)
    return 1;
  else
    return 0;
}

/* private message */
static int gotmsg(char *from, char *msg)
{
  char *to, buf[UHOSTLEN], *nick, ctcpbuf[512], *uhost = buf, *ctcp;
  char *p, *p1, *code;
  struct userrec *u;
  int ctcp_count = 0;
  int ignoring;

 if ((strchr(CHANMETA, *msg) != NULL) ||
     (*msg == '@'))           /* notice to a channel, not handled here */
    return 0;
  ignoring = match_ignore(from);
  to = newsplit(&msg);
  fixcolon(msg);
  /* only check if flood-ctcp is active */
  strcpy(uhost, from);
  nick = splitnick(&uhost);
  if (flud_ctcp_thr && detect_avalanche(msg)) {
    if (!ignoring) {
      putlog(LOG_MODES, "*", "Avalanche from %s - ignoring", from);
      p = strchr(uhost, '@');
      if (p != NULL)
	p++;
      else
	p = uhost;
      simple_sprintf(ctcpbuf, "*!*@%s", p);
      addignore(ctcpbuf, origbotname, "ctcp avalanche",
		now + (60 * ignore_time));
    }
  }
  /* check for CTCP: */
  ctcp_reply[0] = 0;
  p = strchr(msg, 1);
  while ((p != NULL) && (*p)) {
    p++;
    p1 = p;
    while ((*p != 1) && (*p != 0))
      p++;
    if (*p == 1) {
      *p = 0;
      ctcp = strcpy(ctcpbuf, p1);
      strcpy(p1 - 1, p + 1);
      if (!ignoring)
	detect_flood(nick, uhost, from,
		     strncmp(ctcp, "ACTION ", 7) ? FLOOD_CTCP : FLOOD_PRIVMSG);
      /* respond to the first answer_ctcp */
      p = strchr(msg, 1);
      if (ctcp_count < answer_ctcp) {
	ctcp_count++;
	if (ctcp[0] != ' ') {
	  code = newsplit(&ctcp);
	  if ((to[0] == '$') || strchr(to, '.')) {
	    if (!ignoring)
	      /* don't interpret */
	      putlog(LOG_PUBLIC, to, "CTCP %s: %s from %s (%s) to %s",
		     code, ctcp, nick, uhost, to);
	  } else {
	    u = get_user_by_host(from);
	    if (!ignoring || trigger_on_ignore) {
	      if (!check_tcl_ctcp(nick, uhost, u, to, code, ctcp) &&
		  !ignoring) {
		if ((lowercase_ctcp && !strcasecmp(code, "DCC")) ||
		    (!lowercase_ctcp && !strcmp(code, "DCC"))) {
		  /* if it gets this far unhandled, it means that
		   * the user is totally unknown */
		  code = newsplit(&ctcp);
		  if (!strcmp(code, "CHAT")) {
		    if (!quiet_reject) {
		      if (u)
			dprintf(DP_HELP, "NOTICE %s :%s\n", nick,
				"I'm not accepting call at the moment.");
		      else
			dprintf(DP_HELP, "NOTICE %s :%s\n",
				nick, DCC_NOSTRANGERS);
		    }
		    putlog(LOG_MISC, "*", "%s: %s",
			   DCC_REFUSED, from);
		  } else
		    putlog(LOG_MISC, "*", "Refused DCC %s: %s",
			   code, from);
		}
	      }
	      if (!strcmp(code, "ACTION")) {
		putlog(LOG_MSGS, "*", "Action to %s: %s %s",
		       to, nick, ctcp);
	      } else {
		putlog(LOG_MSGS, "*", "CTCP %s: %s from %s (%s)",
		       code, ctcp, nick, uhost);
	      }			/* I love a good close cascade ;) */
	    }
	  }
	}
      }
    }
  }
  /* send out possible ctcp responses */
  if (ctcp_reply[0]) {
    if (ctcp_mode != 2) {
      dprintf(DP_SERVER, "NOTICE %s :%s\n", nick, ctcp_reply);
    } else {
      if (now - last_ctcp > flud_ctcp_time) {
	dprintf(DP_SERVER, "NOTICE %s :%s\n", nick, ctcp_reply);
	count_ctcp = 1;
      } else if (count_ctcp < flud_ctcp_thr) {
	dprintf(DP_SERVER, "NOTICE %s :%s\n", nick, ctcp_reply);
	count_ctcp++;
      }
      last_ctcp = now;
    }
  }
  if (msg[0]) {
    if ((to[0] == '$') || (strchr(to, '.') != NULL)) {
      /* msg from oper */
      if (!ignoring) {
	detect_flood(nick, uhost, from, FLOOD_PRIVMSG);
	/* do not interpret as command */
	putlog(LOG_MSGS | LOG_SERV, "*", "[%s!%s to %s] %s",
	       nick, uhost, to, msg);
      }
    } else {
      char *code;
      struct userrec *u;

      if (!ignoring)
	detect_flood(nick, uhost, from, FLOOD_PRIVMSG);
      u = get_user_by_host(from);
      code = newsplit(&msg);
      if (!ignoring || trigger_on_ignore)
	check_tcl_msgm(code, nick, uhost, u, msg);
      if (!ignoring)
	if (!check_tcl_msg(code, nick, uhost, u, msg))
	  putlog(LOG_MSGS, "*", "[%s] %s %s", from, code, msg);
    }
  }
  return 0;
}

/* private notice */
static int gotnotice(char *from, char *msg)
{
  char *to, *nick, ctcpbuf[512], *p, *p1, buf[512], *uhost = buf, *ctcp;
  struct userrec *u;
  int ignoring;

  if ((strchr(CHANMETA, *msg) != NULL) ||
      (*msg == '@'))           /* notice to a channel, not handled here */
    return 0;
  ignoring = match_ignore(from);
  to = newsplit(&msg);
  fixcolon(msg);
  strcpy(uhost, from);
  nick = splitnick(&uhost);
  if (flud_ctcp_thr && detect_avalanche(msg)) {
    /* discard -- kick user if it was to the channel */
    if (!ignoring)
      putlog(LOG_MODES, "*", "Avalanche from %s", from);
    return 0;
  }
  /* check for CTCP: */
  p = strchr(msg, 1);
  while ((p != NULL) && (*p)) {
    p++;
    p1 = p;
    while ((*p != 1) && (*p != 0))
      p++;
    if (*p == 1) {
      *p = 0;
      ctcp = strcpy(ctcpbuf, p1);
      strcpy(p1 - 1, p + 1);
      if (!ignoring)
	detect_flood(nick, uhost, from, FLOOD_CTCP);
      p = strchr(msg, 1);
      if (ctcp[0] != ' ') {
	char *code = newsplit(&ctcp);

	if ((to[0] == '$') || strchr(to, '.')) {
	  if (!ignoring)
	    putlog(LOG_PUBLIC, "*",
		   "CTCP reply %s: %s from %s (%s) to %s", code, ctcp,
		   nick, from, to);
	} else {
	  u = get_user_by_host(from);
	  if (!ignoring || trigger_on_ignore) {
	    check_tcl_ctcr(nick, from, u, to, code, ctcp);
	    if (!ignoring)
	      /* who cares? */
	      putlog(LOG_MSGS, "*",
		     "CTCP reply %s: %s from %s (%s) to %s",
		     code, ctcp, nick, from, to);
	  }
	}
      }
    }
  }
  if (msg[0]) {
    if (((to[0] == '$') || strchr(to, '.')) && !ignoring) {
      detect_flood(nick, uhost, from, FLOOD_NOTICE);
      putlog(LOG_MSGS | LOG_SERV, "*", "-%s (%s) to %s- %s",
	     nick, uhost, to, msg);
    } else {
      detect_flood(nick, uhost, from, FLOOD_NOTICE);
      u = get_user_by_host(from);
      /* server notice? */
      if ((from[0] == 0) || (nick[0] == 0)) {
	/* hidden `250' connection count message from server */
	if (strncmp(msg, "Highest connection count:", 25))
	  putlog(LOG_SERV, "*", "-NOTICE- %s", msg);
      } else if (!ignoring) {
	check_tcl_notc(nick, from, u, msg);
	putlog(LOG_MSGS, "*", "-%s (%s)- %s", nick, from, msg);
      }
    }
  }
  return 0;
}

/* got 251: lusers
 * <server> 251 <to> :there are 2258 users and 805 invisible on 127 servers */
static int got251(char *from, char *msg)
{
  int i;
  char *servs;

  if (min_servs == 0)
    return 0;			/* no minimum limit on servers */
  newsplit(&msg);
  fixcolon(msg);		/* NOTE!!! If servlimit is not set or is 0 */
  for (i = 0; i < 8; i++)
    newsplit(&msg);		/* lusers IS NOT SENT AT ALL!! */
  servs = newsplit(&msg);
  if (strncmp(msg, "servers", 7))
    return 0;			/* was invalid format */
  while (*servs && (*servs < 32))
    servs++;			/* I've seen some lame nets put bolds &
				 * stuff in here :/ */
  i = atoi(servs);
  if (i < min_servs) {
    putlog(LOG_SERV, "*", IRC_AUTOJUMP, min_servs, i);
    nuke_server(IRC_CHANGINGSERV);
  }
  return 0;
}

/* WALLOPS: oper's nuisance */
static int gotwall(char *from, char *msg)
{
  char *nick;
  char *p;
  int r;

  context;

  fixcolon(msg);
  p = strchr(from, '!');
  if (p && (p == strrchr(from, '!'))) {
    nick = splitnick(&from);
    r = check_tcl_wall(nick, msg);
    if (r == 0)
      putlog(LOG_WALL, "*", "!%s(%s)! %s", nick, from, msg);
  } else {
    r = check_tcl_wall(from, msg);
    if (r == 0)
      putlog(LOG_WALL, "*", "!%s! %s", from, msg);
  }
  return 0;
}

static void minutely_checks()
{
  /* called once a minute... but if we're the only one on the
   * channel, we only wanna send out "lusers" once every 5 mins */
  static int count = 4;
  int ok = 0;
  struct chanset_t *chan;

  if (keepnick) {
    /* NOTE: now that botname can but upto NICKLEN bytes long,
     * check that it's not just a truncation of the full nick */
    if (strncmp(botname, origbotname, strlen(botname))) {
      /* see if my nickname is in use and if if my nick is right */
      if (use_ison)
	/* save space and use the same ISON :P */
        dprintf(DP_MODE, "ISON :%s %s\n", origbotname, get_altbotnick());
      else
	dprintf(DP_MODE, "TRACE %s\n", origbotname);
      /* will return 206(undernet), 401(other), or 402(efnet) numeric if
       * not online */
    }
  }
  if (min_servs == 0)
    return;
  chan = chanset;
  while (chan != NULL) {
    if (channel_active(chan) && (chan->channel.members == 1))
      ok = 1;
    chan = chan->next;
  }
  if (!ok)
    return;
  count++;
  if (count >= 5) {
    dprintf(DP_SERVER, "LUSERS\n");
    count = 0;
  }
}

/* pong from server */
static int gotpong(char *from, char *msg)
{
  newsplit(&msg);
  fixcolon(msg);		/* scrap server name */
  waiting_for_awake = 0;
  server_lag = now - my_atoul(msg);
  if (server_lag > 99999) {
    /* IRCnet lagmeter support by drummer */
    server_lag = now - lastpingtime;
  }
  return 0;
}

/* cleaned up the ison reply code .. (guppy) */
static void got303(char *from, char *msg)
{
  char *tmp, *alt;

  if (keepnick) {
    newsplit(&msg);
    fixcolon(msg);
    tmp = newsplit(&msg);
    if (strncmp(botname, origbotname, strlen(botname))) {
      alt = get_altbotnick();
      if (!tmp[0] || (alt[0] && !rfc_casecmp(tmp, alt))) {
	putlog(LOG_MISC, "*", IRC_GETORIGNICK, origbotname);
	dprintf(DP_MODE, "NICK %s\n", origbotname);
      } else if (alt[0] && !msg[0] && strcasecmp(botname, alt)) {
	putlog(LOG_MISC, "*", IRC_GETALTNICK, alt);
	dprintf(DP_MODE, "NICK %s\n", alt);
      }
    }
  }
}

/* trace failed! meaning my nick is not in use! 206 (undernet)
 * 401 (other non-efnet) 402 (Efnet) */
static void trace_fail(char *from, char *msg)
{
  if (!use_ison) {
    if (!strcasecmp(botname, origbotname)) {
      putlog(LOG_MISC, "*", IRC_GETORIGNICK, origbotname);
      dprintf(DP_MODE, "NICK %s\n", origbotname);
    }
  }
}

/* 432 : bad nickname */
static int got432(char *from, char *msg)
{
  char *erroneus;

  newsplit(&msg);
  erroneus = newsplit(&msg);
  if (server_online)
    putlog(LOG_MISC, "*", "NICK IN INVALID: %s (keeping '%s').", erroneus, botname);
  else {
    putlog(LOG_MISC, "*", IRC_BADBOTNICK);
    if (!keepnick) {
      makepass(erroneus);
      erroneus[NICKMAX] = 0;
      dprintf(DP_MODE, "NICK %s\n", erroneus);
    }
    return 0;
  }
  return 0;
}

/* 433 : nickname in use
 * change nicks till we're acceptable or we give up */
static int got433(char *from, char *msg)
{
  char *tmp;
  context;
  if (server_online) {
    /* we are online and have a nickname, we'll keep it */
	newsplit(&msg);
    tmp = newsplit(&msg);
    putlog(LOG_MISC, "*", "NICK IN USE: %s (keeping '%s').", tmp, botname);
    return 0;
  }
  context;
  gotfake433(from);
  return 0;
}

/* 437 : nickname juped (Euronet) */
static int got437(char *from, char *msg)
{
  char *s;
  struct chanset_t *chan;

  newsplit(&msg);
  s = newsplit(&msg);
  if (strchr(CHANMETA, s[0]) != NULL) {
    chan = findchan(s);
    if (chan) {
      if (chan->status & CHAN_ACTIVE) {
	putlog(LOG_MISC, "*", IRC_CANTCHANGENICK, s);
      } else {
	putlog(LOG_MISC, "*", IRC_CHANNELJUPED, s);
      }
    }
  } else if (server_online) {
    putlog(LOG_MISC, "*", "NICK IS JUPED: %s (keeping '%s').\n", s, botname);
  } else {
    putlog(LOG_MISC, "*", "%s: %s", IRC_BOTNICKJUPED, s);
    gotfake433(from);
  }
  return 0;
}

/* 438 : nick change too fast */
static int got438(char *from, char *msg)
{
  context;
  newsplit(&msg);
  newsplit(&msg);
  fixcolon(msg);
  putlog(LOG_MISC, "*", "%s", msg);
  return 0;
}



static int got451(char *from, char *msg)
{
  /* usually if we get this then we really messed up somewhere
   * or this is a non-standard server, so we log it and kill the socket
   * hoping the next server will work :) -poptix */
  /* um, this does occur on a lagged anti-spoof server connection if the
   * (minutely) sending of joins occurs before the bot does its ping reply
   * probably should do something about it some time - beldin */
  putlog(LOG_MISC, "*", IRC_NOTREGISTERED1, from);
  nuke_server(IRC_NOTREGISTERED2);
  return 0;
}

/* error notice */
static int goterror(char *from, char *msg)
{
  fixcolon(msg);
  putlog(LOG_SERV | LOG_MSGS, "*", "-ERROR from server- %s", msg);
  if (serverror_quit) {
    putlog(LOG_BOTS, "*", "Disconnecting from server.");
    nuke_server("Bah, stupid error messages.");
  }
  return 1;
}

/* make nick!~user@host into nick!user@host if necessary */
/* also the new form: nick!+user@host or nick!-user@host */
static void fixfrom(char *s)
{
  char *p;

  if (strict_host)
    return;
  if (s == NULL)
    return;
  if ((p = strchr(s, '!')))
    p++;
  else
    p = s;			/* sometimes we get passed just a
				 * user@host here... */
  /* these are ludicrous. */
  if (strchr("~+-^=", *p) && (p[1] != '@')) /* added check for @ - drummer */
    strcpy(p, p + 1);
  /* bug was: n!~@host -> n!@host  now: n!~@host */
}

/* nick change */
static int gotnick(char *from, char *msg)
{
  char *nick, *alt = get_altbotnick();
  struct userrec *u;

  u = get_user_by_host(from);
  nick = splitnick(&from);
  fixcolon(msg);
  if (match_my_nick(nick)) {
    /* regained nick! */
    strncpy(botname, msg, NICKMAX);
    botname[NICKMAX] = 0;
    waiting_for_awake = 0;
    if (!strcmp(msg, origbotname))
      putlog(LOG_SERV | LOG_MISC, "*", "Regained nickname '%s'.", msg);
    else if (alt[0] && !strcmp(msg, alt))
      putlog(LOG_SERV | LOG_MISC, "*", "Regained alternate nickname '%s'.", msg);
    else if (keepnick && strcmp(nick, msg)) {
      putlog(LOG_SERV | LOG_MISC, "*", "Nickname changed to '%s'???", msg);
      if (!rfc_casecmp(nick, origbotname)) {
        putlog(LOG_MISC, "*", IRC_GETORIGNICK, origbotname);
        dprintf(DP_MODE, "NICK %s\n", origbotname);
      } else if (alt[0] && !rfc_casecmp(nick, alt)
		 && strcasecmp(botname, origbotname)) {
        putlog(LOG_MISC, "*", IRC_GETALTNICK, alt);
        dprintf(DP_MODE, "NICK %s\n", alt);
      }
    } else
      putlog(LOG_SERV | LOG_MISC, "*", "Nickname changed to '%s'???", msg);
  } else if ((keepnick) && (rfc_casecmp(nick, msg))) {
    /* only do the below if there was actual nick change, case doesn't count */
    if (!rfc_casecmp(nick, origbotname)) {
      putlog(LOG_MISC, "*", IRC_GETORIGNICK, origbotname);
      dprintf(DP_MODE, "NICK %s\n", origbotname);
    } else if (alt[0] && !rfc_casecmp(nick, alt) &&
	    strcasecmp(botname, origbotname)) {
      putlog(LOG_MISC, "*", IRC_GETALTNICK, altnick);
      dprintf(DP_MODE, "NICK %s\n", altnick);
    }
  }
  return 0;
}

static int gotmode(char *from, char *msg)
{
  char *ch;

  ch = newsplit(&msg);
  /* usermode changes? */
  if (strchr(CHANMETA, ch[0]) == NULL) {
    if (match_my_nick(ch) && check_mode_r) {
      /* umode +r? - D0H dalnet uses it to mean something different */
      fixcolon(msg);
      if ((msg[0] == '+') && strchr(msg, 'r')) {
	int servidx = findanyidx(serv);

	putlog(LOG_MISC | LOG_JOIN, "*",
	       "%s has me i-lined (jumping)", dcc[servidx].host);
	nuke_server("i-lines suck");
      }
    }
  }
  return 0;
}

static void eof_server(int idx)
{
  putlog(LOG_SERV, "*", "%s %s", IRC_DISCONNECTED, dcc[idx].host);
  lostdcc(idx);
}

static void display_server(int idx, char *buf)
{
  sprintf(buf, "%s  (lag: %d)", trying_server ? "conn" : "serv",
	  server_lag);
}
static void connect_server(void);

static void kill_server(int idx, void *x)
{
  module_entry *me;

  server_online = 0;
  killsock(dcc[idx].sock);
  serv = -1;
  if ((me = module_find("channels", 0, 0)) && me->funcs) {
    struct chanset_t *chan;

    for (chan = chanset; chan; chan = chan->next)
      (me->funcs[15]) (chan, 1);
  }
  connect_server();
}

static void timeout_server(int idx)
{
  putlog(LOG_SERV, "*", "Timeout: connect to %s", dcc[idx].host);
  lostdcc(idx);
}

static void server_activity(int idx, char *msg, int len);

static struct dcc_table SERVER_SOCKET =
{
  "SERVER",
  0,
  eof_server,
  server_activity,
  0,
  timeout_server,
  display_server,
  0,
  kill_server,
  0
};

static void server_activity(int idx, char *msg, int len)
{
  char *from, *code;

  if (trying_server) {
    strcpy(dcc[idx].nick, "(server)");
    putlog(LOG_SERV, "*", "Connected to %s", dcc[idx].host);
    trying_server = 0;
    waiting_for_awake = 0;
    SERVER_SOCKET.timeout_val = 0;
  }
  from = "";
  if (msg[0] == ':') {
    msg++;
    from = newsplit(&msg);
  }
  code = newsplit(&msg);
  if (use_console_r) {
    if (!strcmp(code, "PRIVMSG") ||
	!strcmp(code, "NOTICE")) {
      if (!match_ignore(from))
	putlog(LOG_RAW, "*", "[@] %s %s %s", from, code, msg);
    } else
      putlog(LOG_RAW, "*", "[@] %s %s %s", from, code, msg);
  }
  if (from[0])
    fixfrom(from);
  context;
  /* this has GOT to go into the raw binding table, * merely because this
   * is less effecient */
  check_tcl_raw(from, code, msg);
}

static int gotping(char *from, char *msg)
{
  fixcolon(msg);
  dprintf(DP_MODE, "PONG :%s\n", msg);
  return 0;
}

/* update the add/rem_builtins in server.c if you add to this list!! */
static cmd_t my_raw_binds[19] =
{
  {"PRIVMSG", "", (Function) gotmsg, NULL},
  {"NOTICE", "", (Function) gotnotice, NULL},
  {"MODE", "", (Function) gotmode, NULL},
  {"PING", "", (Function) gotping, NULL},
  {"PONG", "", (Function) gotpong, NULL},
  {"WALLOPS", "", (Function) gotwall, NULL},
  {"001", "", (Function) got001, NULL},
  {"206", "", (Function) trace_fail, NULL},
  {"251", "", (Function) got251, NULL},
  {"303", "", (Function) got303, NULL},
  {"401", "", (Function) trace_fail, NULL},
  {"402", "", (Function) trace_fail, NULL},
  {"432", "", (Function) got432, NULL},
  {"433", "", (Function) got433, NULL},
  {"437", "", (Function) got437, NULL},
  {"438", "", (Function) got438, NULL},
  {"451", "", (Function) got451, NULL},
  {"NICK", "", (Function) gotnick, NULL},
  {"ERROR", "", (Function) goterror, NULL},
};

/* hook up to a server */
/* works a little differently now... async i/o is your friend */
static void connect_server(void)
{
  char s[121], pass[121], botserver[UHOSTLEN + 1];
  static int oldserv = -1;
  int servidx, botserverport;

  waiting_for_awake = 0;
  trying_server = now;
  empty_msgq();
  /* start up the counter (always reset it if "never-give-up" is on) */
  if ((oldserv < 0) || (never_give_up))
    oldserv = curserv;
  if (newserverport) {		/* jump to specified server */
    curserv = (-1);		/* reset server list */
    strcpy(botserver, newserver);
    botserverport = newserverport;
    strcpy(pass, newserverpass);
    newserver[0] = 0;
    newserverport = 0;
    newserverpass[0] = 0;
    oldserv = (-1);
  }
  if (!cycle_time) {
    if (connectserver[0])	/* drummer */
      do_tcl("connect-server", connectserver);
    next_server(&curserv, botserver, &botserverport, pass);
    putlog(LOG_SERV, "*", "%s %s:%d", IRC_SERVERTRY, botserver, botserverport);
    serv = open_telnet(botserver, botserverport);
    if (serv < 0) {
      if (serv == (-2))
	strcpy(s, IRC_DNSFAILED);
      else
	neterror(s);
      putlog(LOG_SERV, "*", "%s %s (%s)", IRC_FAILEDCONNECT, botserver, s);
      if ((oldserv == curserv) && !(never_give_up))
	fatal("NO SERVERS WILL ACCEPT MY CONNECTION.", 0);
    } else {
      /* queue standard login */
      servidx = new_dcc(&SERVER_SOCKET, 0);
      dcc[servidx].sock = serv;
      dcc[servidx].port = botserverport;
      strcpy(dcc[servidx].nick, "(server)");
      strncpy(dcc[servidx].host, botserver, UHOSTLEN);
      dcc[servidx].host[UHOSTLEN] = 0;
      dcc[servidx].timeval = now;
      SERVER_SOCKET.timeout_val = &server_timeout;
      strcpy(botname, origbotname);
      /* another server may have truncated it :/ */
      dprintf(DP_MODE, "NICK %s\n", botname);
      if (pass[0])
	dprintf(DP_MODE, "PASS %s\n", pass);
      dprintf(DP_MODE, "USER %s %s %s :%s\n",
	      botuser, bothost, botserver, botrealname);
      cycle_time = 0;
      /* We join channels AFTER getting the 001 -Wild */
      /* wait for async result now */
    }
    if (server_cycle_wait)
      /* back to 1st server & set wait time */
      /* put it here, just in case the server quits on us quickly */
      cycle_time = server_cycle_wait;
  }
}
