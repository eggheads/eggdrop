/* 
 * chancmds.c - handles commands direclty relating to channel interaction
 */
/* 
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

/* Do we have any flags that will allow us ops on a channel? */
static struct chanset_t *has_op(int idx, char *chname)
{
  struct chanset_t *chan;

  context;
  if (chname && chname[0]) {
    chan = findchan(chname);
    if (!chan) {
      dprintf(idx, "No such channel.\n");
      return 0;
    }
  } else {
    chname = dcc[idx].u.chat->con_chan;
    chan = findchan(chname);
    if (!chan) {
      dprintf(idx, "Invalid console channel.\n");
      return 0;
    }
  }
  get_user_flagrec(dcc[idx].user, &user, chname);
  if (chan_op(user) || (glob_op(user) && !chan_deop(user)))
    return chan;
  dprintf(idx, "You are not a channel op on %s.\n", chan->name);
  return 0;
}

static void cmd_act(struct userrec *u, int idx, char *par)
{
  char *chname;
  struct chanset_t *chan;
  memberlist *m;

  if (!par[0]) {
    dprintf(idx, "Usage: act [channel] <action>\n");
    return;
  }
  if (strchr(CHANMETA, par[0]) != NULL)
    chname = newsplit(&par);
  else
    chname = 0;
  if (!(chan = has_op(idx, chname)))
    return;
  m = ismember(chan, botname);
  if (!m) {
    dprintf(idx, "Cannot say to %s: I'm not on that channel.\n", chan->name);
    return;
  }
  if ((chan->channel.mode & CHANMODER) && !(m->flags & (CHANOP | CHANVOICE))) {
    dprintf(idx, "Cannot say to %s, it is moderated.\n", chan->name);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# (%s) act %s", dcc[idx].nick,
	 chan->name, par);
  dprintf(DP_HELP, "PRIVMSG %s :\001ACTION %s\001\n",
	  chan->name, par);
  dprintf(idx, "Action to %s: %s\n", chan->name, par);
}

static void cmd_msg(struct userrec *u, int idx, char *par)
{
  char *nick;

  if (!par[0]) {
    dprintf(idx, "Usage: msg <nick> <message>\n");
  } else {
    nick = newsplit(&par);
    putlog(LOG_CMDS, "*", "#%s# msg %s %s", dcc[idx].nick, nick, par);
    dprintf(DP_HELP, "PRIVMSG %s :%s\n", nick, par);
    dprintf(idx, "Msg to %s: %s\n", nick, par);
  }
}

static void cmd_say(struct userrec *u, int idx, char *par)
{
  char *chname;
  struct chanset_t *chan;
  memberlist *m;

  if (!par[0]) {
    dprintf(idx, "Usage: say [channel] <message>\n");
    return;
  }
  if (strchr(CHANMETA, par[0]) != NULL)
    chname = newsplit(&par);
  else
    chname = 0;
  if (!(chan = has_op(idx, chname)))
    return;
  m = ismember(chan, botname);
  if (!m) {
    dprintf(idx, "Cannot say to %s: I'm not on that channel.\n", chan->name);
    return;
  }
  if ((chan->channel.mode & CHANMODER) && !(m->flags & (CHANOP | CHANVOICE))) {
    dprintf(idx, "Cannot say to %s, it is moderated.\n", chan->name);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# (%s) say %s", dcc[idx].nick, chan->name, par);
  dprintf(DP_HELP, "PRIVMSG %s :%s\n", chan->name, par);
  dprintf(idx, "Said to %s: %s\n", chan->name, par);
}

static void cmd_kickban(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *chname, *nick, *s1;
  memberlist *m;
  char s[1024];
  char bantype = 0;

  if (!par[0]) {
    dprintf(idx, "Usage: kickban [channel] <nick> [reason]\n");
    return;
  }
  if (strchr(CHANMETA, par[0]) != NULL)
    chname = newsplit(&par);
  else
    chname = 0;
  if (!(chan = has_op(idx, chname)))
    return;
  if (!me_op(chan)) {
    dprintf(idx, "I can't help you now because I'm not a channel op on %s.\n",
	    chan->name);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# (%s) kickban %s", dcc[idx].nick,
	 chan->name, par);
  nick = newsplit(&par);
  if ((nick[0] == '@') || (nick[0] == '!')) {
    bantype = nick[0];
    nick++;
  }
  if (match_my_nick(nick)) {
    dprintf(idx, "You're trying to pull a Run?\n");
    return;
  } else {
    m = ismember(chan, nick);
    if (!m) {
      dprintf(idx, "%s is not on %s\n", nick, chan->name);
    } else {
      simple_sprintf(s, "%s!%s", m->nick, m->userhost);
      u = get_user_by_host(s);
      get_user_flagrec(u, &victim, chan->name);
      if ((chan_op(victim) || (glob_op(victim) && !chan_deop(victim))) &&
	  !(chan_master(user) || glob_master(user))) {
	dprintf(idx, "%s is a legal op.\n", nick);
	return;
      }
      if ((chan_master(victim) || glob_master(victim)) &&
	  !(glob_owner(user) || chan_owner(user))) {
	dprintf(idx, "%s is a %s master.\n", nick, chan->name);
	return;
      }
      if (glob_bot(victim) && !(glob_owner(victim) || chan_owner(victim))) {
	dprintf(idx, "%s is another channel bot!\n", nick);
	return;
      }
      if (m->flags & CHANOP)
	add_mode(chan, '-', 'o', m->nick);
      switch (bantype) {
	case '@':
	  s1 = strchr(s, '@');
	  s1 -= 3;
	  s1[0] = '*';
	  s1[1] = '!';
	  s1[2] = '*';
	  break;
	case '!':
	  s1 = strchr(s, '!');
	  s1[1] = '*';
	  s1--;
	  s1[0] = '*';
	  break;
	default:
	  s1 = quickban(chan, m->userhost);
	  break;
      }
      if (!bantype)
	do_ban(chan, s1);
      if (!par[0])
	par = "requested";
      dprintf(DP_SERVER, "KICK %s %s :%s\n", chan->name, m->nick, par);
      u_addban(chan, s1, dcc[idx].nick, par, now + (60 * ban_time), 0);
      dprintf(idx, "Okay, done.\n");
    }
  }
}

static void cmd_voice(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *nick;
  memberlist *m;
  char s[1024];

  if (!par[0]) {
    dprintf(idx, "Usage: voice <nick> [channel]\n");
    return;
  }
  nick = newsplit(&par);
  if (!(chan = has_op(idx, par)))
    return;
  if (!me_op(chan)) {
    dprintf(idx, "I can't help you now because I'm not a chan op on %s.\n",
	    chan->name);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# (%s) voice %s %s", dcc[idx].nick,
	 dcc[idx].u.chat->con_chan, nick, par);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s.\n", nick, chan->name);
    return;
  }
  simple_sprintf(s, "%s!%s", m->nick, m->userhost);
  add_mode(chan, '+', 'v', nick);
  dprintf(idx, "Gave voice to %s on %s\n", nick, chan->name);
}

static void cmd_devoice(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *nick;
  memberlist *m;
  char s[1024];

  if (!par[0]) {
    dprintf(idx, "Usage: devoice <nick> [channel]\n");
    return;
  }
  nick = newsplit(&par);
  if (!(chan = has_op(idx, par)))
    return;
  if (!me_op(chan)) {
    dprintf(idx, "I can't do that right now I'm not a chan op on %s.\n",
	    chan->name);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# (%s) devoice %s %s", dcc[idx].nick,
	 dcc[idx].u.chat->con_chan, nick, par);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s.\n", nick, chan->name);
    return;
  }
  simple_sprintf(s, "%s!%s", m->nick, m->userhost);
  add_mode(chan, '-', 'v', nick);
  dprintf(idx, "Devoiced %s on %s\n", nick, chan->name);
}

static void cmd_op(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *nick;
  memberlist *m;
  char s[1024];

  if (!par[0]) {
    dprintf(idx, "Usage: op <nick> [channel]\n");
    return;
  }
  nick = newsplit(&par);
  if (!(chan = has_op(idx, par)))
    return;
  if (!me_op(chan)) {
    dprintf(idx, "I can't help you now because I'm not a chan op on %s.\n",
	    chan->name);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# (%s) op %s %s", dcc[idx].nick,
	 dcc[idx].u.chat->con_chan, nick, par);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s.\n", nick, chan->name);
    return;
  }
  simple_sprintf(s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->name);
  if (chan_deop(victim) || (glob_deop(victim) && !glob_op(victim))) {
    dprintf(idx, "%s is currently being auto-deopped.\n", m->nick);
    return;
  }
  if (channel_bitch(chan)
      && !(chan_op(victim) || (glob_op(victim) && !chan_deop(victim)))) {
    dprintf(idx, "%s is not a registered op.\n", m->nick);
    return;
  }
  add_mode(chan, '+', 'o', nick);
  dprintf(idx, "Gave op to %s on %s\n", nick, chan->name);
}

static void cmd_deop(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *nick;
  memberlist *m;
  char s[121];

  if (!par[0]) {
    dprintf(idx, "Usage: deop <nick> [channel]\n");
    return;
  }
  nick = newsplit(&par);
  if (!(chan = has_op(idx, par)))
    return;
  if (!me_op(chan)) {
    dprintf(idx, "I can't help you now because I'm not a chan op on %s.\n",
	    chan->name);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# (%s) deop %s %s", dcc[idx].nick,
	 dcc[idx].u.chat->con_chan, nick, par);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s.\n", nick, chan->name);
    return;
  }
  if (match_my_nick(nick)) {
    dprintf(idx, "I'm not going to deop myself.\n");
    return;
  }
  simple_sprintf(s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->name);
  if ((chan_master(victim) || glob_master(victim)) &&
      !(chan_owner(user) || glob_owner(user))) {
    dprintf(idx, "%s is a master for %s\n", m->nick, chan->name);
    return;
  }
  if ((chan_op(victim) || (glob_op(victim) && !chan_deop(victim))) &&
      !(chan_master(user) || glob_master(user))) {
    dprintf(idx, "%s has the op flag for %s\n", m->nick, chan->name);
    return;
  }
  add_mode(chan, '-', 'o', nick);
  dprintf(idx, "Took op from %s on %s\n", nick, chan->name);
}

static void cmd_kick(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *chname, *nick;
  memberlist *m;
  char s[121];

  if (!par[0]) {
    dprintf(idx, "Usage: kick [channel] <nick> [reason]\n");
    return;
  }
  if (strchr(CHANMETA, par[0]) != NULL)
    chname = newsplit(&par);
  else
    chname = 0;
  if (!(chan = has_op(idx, chname)))
    return;
  if (!me_op(chan)) {
    dprintf(idx, "I can't help you now because I'm not a channel op on %s.\n",
	    chan->name);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# (%s) kick %s", dcc[idx].nick,
	 chan->name, par);
  nick = newsplit(&par);
  if (!par[0])
    par = "request";
  if (match_my_nick(nick)) {
    dprintf(idx, "But I don't WANT to kick myself!\n");
    return;
  }
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s\n", nick, chan->name);
    return;
  }
  simple_sprintf(s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->name);
  if ((chan_op(victim) || (glob_op(victim) && !chan_deop(victim))) &&
      !(chan_master(user) || glob_master(user))) {
    dprintf(idx, "%s is a legal op.\n", nick);
    return;
  }
  if ((chan_master(victim) || glob_master(victim)) &&
      !(glob_owner(user) || chan_owner(user))) {
    dprintf(idx, "%s is a %s master.\n", nick, chan->name);
    return;
  }
  if (glob_bot(victim) && !(glob_owner(victim) || chan_owner(victim))) {
    dprintf(idx, "%s is another channel bot!\n", nick);
    return;
  }
  dprintf(DP_SERVER, "KICK %s %s :%s\n", chan->name, m->nick, par);
  dprintf(idx, "Okay, done.\n");
}

static void cmd_invite(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  memberlist *m;
  char *nick;

  if (!par[0])
    par = dcc[idx].nick;	/* doh, it's been without this since .9 ! */
  /* (1.2.0+pop3) - poptix */
  nick = newsplit(&par);
  if (!(chan = has_op(idx, par)))
    return;
  putlog(LOG_CMDS, "*", "#%s# (%s) invite %s", dcc[idx].nick, chan->name, nick);
  if (!me_op(chan)) {
    if (chan->channel.mode & CHANINV) {
      dprintf(idx, "I'm not chop on %s, so I can't invite anyone.\n",
	      chan->name);
      return;
    }
    if (!channel_active(chan)) {
      dprintf(idx, "I'm not on %s right now!\n", chan->name);
      return;
    }
  }
  m = ismember(chan, nick);
  if (m && !chan_issplit(m)) {
    dprintf(idx, "%s is already on %s!\n", nick, chan->name);
    return;
  }
  dprintf(DP_SERVER, "INVITE %s %s\n", nick, chan->name);
  dprintf(idx, "Inviting %s to %s.\n", nick, chan->name);
}

static void cmd_channel(struct userrec *u, int idx, char *par)
{
  char handle[20], s[121], s1[121], atrflag, chanflag, *chname;
  struct chanset_t *chan;
  int i;
  memberlist *m;
  static char spaces[33] = "                              ";
  static char spaces2[33] = "                              ";
  int len, len2;

  if (!has_op(idx, par))
    return;
  chname = newsplit(&par);
  putlog(LOG_CMDS, "*", "#%s# (%s) channel %s", dcc[idx].nick,
	 dcc[idx].u.chat->con_chan, chname);
  if (!chname[0])
    chan = findchan(dcc[idx].u.chat->con_chan);
  else
    chan = findchan(chname);
  if (chan == NULL) {
    dprintf(idx, "%s %s\n", IRC_NOTACTIVECHAN, chname);
    return;
  }
  strcpy(s, getchanmode(chan));
  if (channel_pending(chan))
    sprintf(s1, "%s %s", IRC_PROCESSINGCHAN, chan->name);
  else if (channel_active(chan))
    sprintf(s1, "%s %s", IRC_CHANNEL, chan->name);
  else
    sprintf(s1, "%s %s", IRC_DESIRINGCHAN, chan->name);
  dprintf(idx, "%s, %d member%s, mode %s:\n", s1, chan->channel.members,
	  chan->channel.members == 1 ? "" : "s", s);
  if (chan->channel.topic)
    dprintf(idx, "%s: %s\n", IRC_CHANNELTOPIC, chan->channel.topic);
  m = chan->channel.member;
  i = 0;
  if (channel_active(chan)) {
    dprintf(idx, "(n = owner, m = master, o = op, d = deop, b = bot)\n");
    spaces[NICKMAX - 9] = 0;
    spaces2[HANDLEN - 9] = 0;
    dprintf(idx, " NICKNAME %s HANDLE   %s JOIN   IDLE  USER@HOST\n",
	    spaces, spaces2);
    spaces[NICKMAX - 9] = ' ';
    spaces2[HANDLEN - 9] = ' ';
    while (m->nick[0]) {
      if (m->joined > 0) {
	strcpy(s, ctime(&(m->joined)));
	if ((now - (m->joined)) > 86400) {
	  strcpy(s1, &s[4]);
	  strcpy(s, &s[8]);
	  strcpy(&s[2], s1);
	  s[5] = 0;
	} else {
	  strcpy(s, &s[11]);
	  s[5] = 0;
	}
      } else
	strcpy(s, " --- ");
      if (m->user == NULL) {
	sprintf(s1, "%s!%s", m->nick, m->userhost);
	m->user = get_user_by_host(s1);
      }
      if (m->user == NULL) {
	strcpy(handle, "*");
      } else {
	strcpy(handle, m->user->handle);
      }
      get_user_flagrec(m->user, &user, chan->name);
      /* determine status char to use */
      get_user_flagrec(m->user, &user, chan->name);
      /* determine status char to use */
      if (glob_bot(user))
	atrflag = 'b';
      else if (glob_owner(user))
	atrflag = 'N';
      else if (chan_owner(user))
	atrflag = 'n';
      else if (glob_master(user))
	atrflag = 'M';
      else if (chan_master(user))
	atrflag = 'm';
      else if (glob_op(user) && !chan_deop(user))
	atrflag = 'O';
      else if (chan_op(user) && !chan_deop(user))
	atrflag = 'o';
      else if (glob_autoop(user) && !chan_deop(user))
	atrflag = 'A';
      else if (chan_autoop(user) && !chan_deop(user))
	atrflag = 'a';
      else if (glob_deop(user) && !chan_op(user))
	atrflag = 'D';
      else if (chan_deop(user))
	atrflag = 'd';
      else if (glob_voice(user) && !chan_quiet(user))
	atrflag = 'V';
      else if (chan_voice(user))
	atrflag = 'v';
      else if (glob_gvoice(user) && !chan_quiet(user))
	atrflag = 'G';
      else if (chan_gvoice(user))
	atrflag = 'g';
      else if (glob_quiet(user) && !chan_voice(user))
	atrflag = 'Q';
      else if (chan_quiet(user))
	atrflag = 'q';
      else
	atrflag = ' ';
      if (chan_hasop(m))
	chanflag = '@';
      else if (chan_hasvoice(m))
	chanflag = '+';
      else
	chanflag = ' ';
      spaces[len = (NICKMAX - strlen(m->nick))] = 0;
      spaces2[len2 = (HANDLEN - strlen(handle))] = 0;
      if (chan_issplit(m))
	dprintf(idx, "%c%s%s %s%s %s %c     <- netsplit, %lus\n", chanflag,
		m->nick, spaces, handle, spaces2, s, atrflag,
		now - (m->split));
      else if (!rfc_casecmp(m->nick, botname))
	dprintf(idx, "%c%s%s %s%s %s %c     <- it's me!\n", chanflag, m->nick,
		spaces, handle, spaces2, s, atrflag);
      else {
	/* determine idle time */
	if (now - (m->last) > 86400)
	  sprintf(s1, "%2lud", ((now - (m->last)) / 86400));
	else if (now - (m->last) > 3600)
	  sprintf(s1, "%2luh", ((now - (m->last)) / 3600));
	else if (now - (m->last) > 180)
	  sprintf(s1, "%2lum", ((now - (m->last)) / 60));
	else
	  strcpy(s1, "   ");
	dprintf(idx, "%c%s%s %s%s %s %c %s  %s\n", chanflag, m->nick,
		spaces, handle, spaces2, s, atrflag, s1, m->userhost);
      }
      spaces[len] = ' ';
      spaces2[len2] = ' ';
      if (chan_fakeop(m))
	dprintf(idx, "    (%s)\n", IRC_FAKECHANOP);
      if (chan_sentop(m))
	dprintf(idx, "    (%s)\n", IRC_PENDINGOP);
      if (chan_sentdeop(m))
	dprintf(idx, "    (%s)\n", IRC_PENDINGDEOP);
      if (chan_sentkick(m))
	dprintf(idx, "    (%s)\n", IRC_PENDINGKICK);
      m = m->next;
    }
  }
  dprintf(idx, "%s\n", IRC_ENDCHANINFO);
}

static void cmd_topic(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;

  if (strchr("&#+", par[0])) {
    char *chname = newsplit(&par);

    chan = has_op(idx, chname);
  } else
    chan = has_op(idx, "");
  if (chan) {
    if (!par[0]) {
      if (chan->channel.topic) {
	dprintf(idx, "The topic for %s is: %s\n", chan->name,
		chan->channel.topic);
      } else {
	dprintf(idx, "No topic is set for %s\n", chan->name);
      }
    } else if (channel_optopic(chan) && !me_op(chan)) {
      dprintf(idx, "I'm not a channel op on %s and the channel is +t.\n",
	      chan->name);
    } else {
      dprintf(DP_SERVER, "TOPIC %s :%s\n", chan->name, par);
      dprintf(idx, "Changing topic...\n");
      putlog(LOG_CMDS, "*", "#%s# (%s) topic %s", dcc[idx].nick,
	     chan->name, par);
    }
  }
}

static void cmd_resetbans(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan = findchan(dcc[idx].u.chat->con_chan);

  get_user_flagrec(u, &user, dcc[idx].u.chat->con_chan);
  if (!chan)
    dprintf(idx, "Invalid console channel.\n");
  else if (glob_op(user) || chan_op(user)) {
    putlog(LOG_CMDS, "*", "#%s# (%s) resetbans", dcc[idx].nick, chan->name);
    dprintf(idx, "Resetting bans on %s...\n", chan->name);
    resetbans(chan);
  }
}

static void cmd_resetexempts(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan = findchan(dcc[idx].u.chat->con_chan);

  get_user_flagrec(u, &user, dcc[idx].u.chat->con_chan);
  if (!chan)
    dprintf(idx, "Invalid console channel.\n");
  else if (glob_op(user) || chan_op(user)) {
    putlog(LOG_CMDS, "*", "#%s# (%s) resetexempts", dcc[idx].nick, chan->name);
    dprintf(idx, "Resetting exemptions on %s...\n", chan->name);
    resetexempts(chan);
  }
}

static void cmd_resetinvites(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan = findchan(dcc[idx].u.chat->con_chan);

  get_user_flagrec(u, &user, dcc[idx].u.chat->con_chan);
  if (!chan)
    dprintf(idx, "Invalid console channel.\n");
  else if (glob_op(user) || chan_op(user)) {
    putlog(LOG_CMDS, "*", "#%s# (%s) resetinvites", dcc[idx].nick, chan->name);
    dprintf(idx, "Resetting invitations on %s...\n", chan->name);
    resetinvites(chan);
  }
}

static void cmd_adduser(struct userrec *u, int idx, char *par)
{
  char *nick, *hand;
  struct chanset_t *chan;
  memberlist *m = 0;
  char s[121], s1[121];
  int atr = u ? u->flags : 0;
  int statichost = 0;
  char *p1 = (char*) &s1;

  context;
  if ((!par[0]) || ((par[0]=='!') && (!par[1]))) {
    dprintf(idx, "Usage: adduser <nick> [handle]\n");
    return;
  }
  nick = newsplit(&par);
  /* patch that allow to create users with static host (drummer,20Apr99) */
  if (nick[0] == '!') {
    statichost = 1;
    nick++;
  }
  if (!par[0]) {
    hand = nick;
  } else {
    char *p;
    int ok = 1;

    for (p = par; *p; p++)
      if ((*p <= 32) || (*p >= 127))
	ok = 0;
    if (!ok) {
      dprintf(idx, "You can't have strange characters in a nick.\n");
      return;
    } else if (strchr("-,+*=:!.@#;$", par[0]) != NULL) {
      dprintf(idx, "You can't start a nick with '%c'.\n", par[0]);
      return;
    }
    hand = par;
  }

  context;
  chan = chanset;
  while (chan != NULL) {
    m = ismember(chan, nick);
    if (m!=NULL)
      break;
    chan=chan->next;
  }
  if (!m) {
    dprintf(idx, "%s is not on any channels I monitor\n", nick);
    return;
  } /* else
    dprintf(idx,"I found %s on %s as %s\n", nick, chan->name, m->userhost);
    */
  if (strlen(hand) > HANDLEN)
    hand[HANDLEN] = 0;
  simple_sprintf(s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  if (u) {
    dprintf(idx, "%s is already known as %s.\n", nick, u->handle);
    return;
  }
  u = get_user_by_handle(userlist, hand);
  if (u && (u->flags & USER_OWNER) &&
      !(atr & USER_OWNER) && !strcasecmp(dcc[idx].nick, hand)) {
    dprintf(idx, "You can't add hostmasks to the bot owner.\n");
    return;
  }
  if (!statichost)
    maskhost(s, s1);
  else {
    strcpy(s1,s);
    p1 = strchr(s1,'!');
    if (strchr("~^+=-",p1[1]))
      p1[1] = '*';
    p1--;
    p1[0] = '*';
  }
  if (!u) {
    dprintf(idx, "Added [%s]%s with no password.\n", hand, p1);
    userlist = adduser(userlist, hand, p1, "-", USER_DEFAULT);
  } else {
    dprintf(idx, "Added hostmask %s to %s.\n", p1, u->handle);
    addhost_by_handle(hand,p1);
    get_user_flagrec(u, &user, chan->name);
    if (!(m->flags & CHANOP) &&
	(chan_op(user) || (glob_op(user) && !chan_deop(user))) &&
	(channel_autoop(chan) || glob_autoop(user) || chan_autoop(user)))
      add_mode(chan, '+', 'o', m->nick);
  }
  putlog(LOG_CMDS, "*", "#%s# adduser %s %s", dcc[idx].nick, nick,
	 hand == nick ? "" : hand);
}

static void cmd_deluser(struct userrec *u, int idx, char *par)
{
  char *nick, s[1024];
  struct chanset_t *chan;
  memberlist *m;
  struct flag_record victim =
  {
    FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0
  };

  if (!par[0]) {
    dprintf(idx, "Usage: deluser <nick>\n");
    return;
  }
  chan = findchan(dcc[idx].u.chat->con_chan);
  if (!chan) {
    dprintf(idx, "Your console channel is invalid.\n");
    return;
  }
  if (!channel_active(chan)) {
    dprintf(idx, "I'm not on %s!\n", chan->name);
    return;
  }
  nick = newsplit(&par);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s.\n", nick, chan->name);
    return;
  }
  get_user_flagrec(u, &user, chan->name);
  simple_sprintf(s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  if (!u) {
    dprintf(idx, "%s is not in a valid user.\n", nick);
    return;
  }
  get_user_flagrec(u, &victim, NULL);
  /* this maybe should allow glob +n's to deluser glob +n's but I don't
   * like that - beldin */
  /* checks vs channel owner/master ANYWHERE now -
   * so deluser on a channel they're not on should work */
  /* Shouldn't allow people to remove permanent owners (guppy 9Jan1999) */
  if ((glob_owner(victim) && strcasecmp(dcc[idx].nick, nick)) ||
      isowner(u->handle)) {
    dprintf(idx, "Can't remove the bot owner!\n");
  } else if (glob_botmast(victim) && !glob_owner(user)) {
    dprintf(idx, "Can't delete a master!\n");
  } else if (chan_owner(victim) && !glob_owner(user)) {
    dprintf(idx, "Can't remove a channel owner!\n");
  } else if (chan_master(victim) && !(glob_owner(user) || chan_owner(user))) {
    dprintf(idx, "Can't delete a channel master!\n");
  } else if (glob_bot(victim) && !glob_owner(user)) {
    dprintf(idx, "Can't delete a bot!\n");
  } else {
    char buf[HANDLEN + 1];
    strncpy(buf, u->handle, HANDLEN);
    buf[HANDLEN] = 0;
    if (deluser(u->handle)) {
      dprintf(idx, "Deleted %s.\n", buf); /* ?!?! :) */
      putlog(LOG_CMDS, "*", "#%s# deluser %s [%s]", dcc[idx].nick, nick, buf);
    } else {
      dprintf(idx, "Failed.\n");
    }
  }
}

static void cmd_reset(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;

  if (par[0]) {
    chan = findchan(par);
    if (!chan)
      dprintf(idx, "%s\n", IRC_NOMONITOR);
    else {
      get_user_flagrec(u, &user, par);
      if (!glob_master(user) && !chan_master(user)) {
	dprintf(idx, "You are not a master on %s.\n", chan->name);
      } else if (!channel_active(chan)) {
	dprintf(idx, "Im not on %s at the moment!\n", chan->name);
      } else {
	putlog(LOG_CMDS, "*", "#%s# reset %s", dcc[idx].nick, par);
	dprintf(idx, "Resetting channel info for %s...\n", par);
	reset_chan_info(chan);
      }
    }
  } else if (!(u->flags & USER_MASTER)) {
    dprintf(idx, "You are not a Bot Master.\n");
  } else {
    putlog(LOG_CMDS, "*", "#%s# reset all", dcc[idx].nick);
    dprintf(idx, "Resetting channel info for all channels...\n");
    chan = chanset;
    while (chan != NULL) {
      if (channel_active(chan))
	reset_chan_info(chan);
      chan = chan->next;
    }
  }
}

/* update the add/rem_builtins in irc.c if you add to this list!! */
static cmd_t irc_dcc[18] =
{
  {"adduser", "m|m", (Function) cmd_adduser, NULL},
  {"deluser", "m|m", (Function) cmd_deluser, NULL},
  {"reset", "m|m", (Function) cmd_reset, NULL},
  {"resetbans", "o|o", (Function) cmd_resetbans, NULL},
  {"resetexempts", "o|o", (Function) cmd_resetexempts, NULL},
  {"resetinvites", "o|o", (Function) cmd_resetinvites, NULL},
  {"act", "o|o", (Function) cmd_act, NULL},
  {"channel", "o|o", (Function) cmd_channel, NULL},
  {"deop", "o|o", (Function) cmd_deop, NULL},
  {"invite", "o|o", (Function) cmd_invite, NULL},
  {"kick", "o|o", (Function) cmd_kick, NULL},
  {"kickban", "o|o", (Function) cmd_kickban, NULL},
  {"msg", "o", (Function) cmd_msg, NULL},
  {"voice", "o|o", (Function) cmd_voice, NULL},
  {"devoice", "o|o", (Function) cmd_devoice, NULL},
  {"op", "o|o", (Function) cmd_op, NULL},
  {"say", "o|o", (Function) cmd_say, NULL},
  {"topic", "o|o", (Function) cmd_topic, NULL},
};
