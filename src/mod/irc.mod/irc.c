/*
 * irc.c - part of channels.mod
 * support for channels withing the bot 
 */

#define MODULE_NAME "irc"
#define MAKING_IRC
#include "../module.h"
#include "irc.h"
#include "../server.mod/server.h"
#include "../channels.mod/channels.h"
#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif

static p_tcl_bind_list H_topc, H_splt, H_sign, H_rejn, H_part, H_pub, H_pubm;
static p_tcl_bind_list H_nick, H_mode, H_kick, H_join;
static Function *global = NULL, *channels_funcs = NULL, *server_funcs = NULL;

static int ctcp_mode;
static int net_type;
static int strict_host;

/* time to wait for user to return for net-split */
static int wait_split = 300;
static int max_bans = 20;
static int max_exempts = 20;
static int max_invites = 20;
static int max_modes = 30;
static int bounce_bans = 1;
static int bounce_exempts = 0;
static int bounce_invites = 0;
static int bounce_modes = 0;
static int bounce_bogus_bans = 1;
static int kick_bogus_bans = 1;
static int bounce_bogus_exempts = 0;
static int kick_bogus_exempts = 0;
static int bounce_bogus_invites = 0;
static int kick_bogus_invites = 0;
static int learn_users = 0;
static int wait_info = 15;
static int invite_key = 1;
static int no_chanrec_info = 0;
static int modesperline = 3;	/* number of modes per line to send */
static int mode_buf_len = 200;	/* maximum bytes to send in 1 mode */
static int use_354 = 0;		/* use ircu's short 354 /who responses */
static int kick_method = 1;	/* how many kicks does the irc network
				 * support at once? */
				/* 0 = as many as possible. Ernst 18/3/1998 */
static int kick_bogus = 0;
static int ban_bogus = 0;
static int kick_fun = 0;
static int ban_fun = 0;
static int allow_desync = 0;
static int keepnick = 1;	/* keepnick */
static char altnick[NICKLEN];	/* dang it, we need this here too */

#include "chan.c"
#include "mode.c"
#include "cmdsirc.c"
#include "msgcmds.c"
#include "tclirc.c"

/* revenge tactic: person did something bad, if they are oplisted,
 * remove them from the op list otherwise, deop them */
static void take_revenge(struct chanset_t *chan, char *who, char *reason)
{
  char *nick, s[UHOSTLEN], s1[UHOSTLEN], ct[81];
  int i;
  time_t tm;
  struct userrec *u;
  memberlist *mx = NULL;
  struct flag_record fr =
  {
    FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0
  };

  u = get_user_by_host(who);
  get_user_flagrec(u, &fr, chan->name);
  if (glob_friend(fr) || chan_friend(fr)) {
    putlog(LOG_MISC, "*", "%s is a friend (%s)", who, reason);
    return;			/* argh! */
  }
  if (chan_op(fr) || (glob_op(fr) && !chan_deop(fr))) {
    fr.match = FR_CHAN;
    if (chan_op(fr)) {
      fr.chan &= ~USER_OP;
    } else {
      fr.chan |= USER_DEOP;
    }
    set_user_flagrec(u, &fr, chan->name);
    putlog(LOG_MISC, "*", "No longer opping %s[%s] (%s)", u->handle, who,
	   reason);
    recheck_channel(chan, 0);
    return;
  }
  if (u_match_ban(global_bans, who) || u_match_ban(chan->bans, who)) {
    /* what more can we do? */
    return;
  }
  /* get current time into a string */
  tm = now;
  strcpy(ct, ctime(&tm));
  ct[10] = 0;
  ct[16] = 0;
  strcpy(ct, &ct[8]);
  strcpy(&ct[2], &ct[3]);
  strcpy(&ct[7], &ct[10]);
  if (chan_deop(fr) || glob_deop(fr)) {
    /* this is out of control: BAN THEM */
    putlog(LOG_MISC, "*", "Now banning %s (%s)", who, reason);
    nick = splitnick(&who);
    maskhost(who, s1);
    simple_sprintf(s, "(%s) %s", ct, reason);
    u_addban(chan, s1, origbotname, s, now + (60 * ban_time), 0);
    if (me_op(chan))
      add_mode(chan, '+', 'b', s1);
    return;
  }
  if (u) {
    /* in the user list already, cool :) */
    fr.match = FR_CHAN;
    fr.chan |= USER_DEOP;
    set_user_flagrec(u, &fr, chan->name);
    simple_sprintf(s, "(%s) %s", ct, reason);
    putlog(LOG_MISC, "*", "Now deopping %s[%s] (%s)", u->handle, who, s);
    recheck_channel(chan, 0);
    return;
  }
  nick = splitnick(&who);
  strcpy(s1, who);
  maskhost(s1, s);
  strcpy(s1, nick);
  while (get_user_by_handle(userlist, s1)) {
    if (!strncmp(s1, "bad", 3)) {
      i = atoi(s1 + 3);
      simple_sprintf(s1 + 3, "%d", i + 1);
    } else
      strcpy(s1, "bad1");
  }
  userlist = adduser(userlist, s1, s, "-", 0);
  fr.match = FR_CHAN;
  fr.chan = USER_DEOP;
  fr.udef_chan = 0;
  u = get_user_by_handle(userlist, s1);
  if ((mx = ismember(chan, nick)))
    mx->user = u;
  set_user_flagrec(u, &fr, chan->name);
  simple_sprintf(s, "(%s) %s (%s)", ct, reason, who);
  set_user(&USERENTRY_COMMENT, u, (void *) &s);
  putlog(LOG_MISC, "*", "Now deopping %s (%s)", who, reason);
  recheck_channel(chan, 0);
  recheck_bans(chan);
}

/* set the key */
static void set_key(struct chanset_t *chan, char *k)
{
  nfree(chan->channel.key);
  if (k == NULL) {
    chan->channel.key = (char *) channel_malloc(1);
    chan->channel.key[0] = 0;
    return;
  }
  chan->channel.key = (char *) channel_malloc(strlen(k) + 1);
  strcpy(chan->channel.key, k);
}

static int hand_on_chan(struct chanset_t *chan, struct userrec *u)
{
  char s[UHOSTLEN];
  memberlist *m = chan->channel.member;

  while (m->nick[0]) {
    sprintf(s, "%s!%s", m->nick, m->userhost);
    if (u == get_user_by_host(s))
      return 1;
    m = m->next;
  }
  return 0;
}

/* adds a ban to the list */
static void newban(struct chanset_t *chan, char *s, char *who)
{
  banlist *b;

  b = chan->channel.ban;
  while (b->ban[0] && rfc_casecmp(b->ban, s))
    b = b->next;
  if (b->ban[0])
    return;			/* already existent ban */
  b->next = (banlist *) channel_malloc(sizeof(banlist));
  b->next->next = NULL;
  b->next->ban = (char *) channel_malloc(1);
  b->next->ban[0] = 0;
  nfree(b->ban);
  b->ban = (char *) channel_malloc(strlen(s) + 1);
  strcpy(b->ban, s);
  b->who = (char *) channel_malloc(strlen(who) + 1);
  strcpy(b->who, who);
  b->timer = now;
}

/* add a ban exemption to the list - Daemus - 2/3/1999 */
static void newexempt(struct chanset_t *chan, char *s, char *who)
{
  exemptlist *e;

  context;
  e = chan->channel.exempt;
  while (e->exempt[0] && rfc_casecmp(e->exempt, s))
    e = e->next;
  if (e->exempt[0])
    return;			/* already existent exemption */
  e->next = (exemptlist *) channel_malloc(sizeof(exemptlist));
  e->next->next = NULL;
  e->next->exempt = (char *) channel_malloc(1);
  e->next->exempt[0] = 0;
  nfree(e->exempt);
  e->exempt = (char *) channel_malloc(strlen(s) + 1);
  strcpy(e->exempt, s);
  e->who = (char *) channel_malloc(strlen(who) + 1);
  strcpy(e->who, who);
  e->timer = now;
}

/* add an invite exemption to the list - Daemus - 2/3/1999 */
static void newinvite(struct chanset_t *chan, char *s, char *who)
{
  invitelist *inv;
  
  context;
  inv = chan->channel.invite;
  while (inv->invite[0] && rfc_casecmp(inv->invite, s))
    inv = inv->next;
  if (inv->invite[0])
    return;			/* already existent invitation */
  inv->next = (invitelist *) channel_malloc(sizeof(invitelist));
  inv->next->next = NULL;
  inv->next->invite = (char *) channel_malloc(1);
  inv->next->invite[0] = 0;
  nfree(inv->invite);
  inv->invite = (char *) channel_malloc(strlen(s) + 1);
  strcpy(inv->invite, s);
  inv->who = (char *) channel_malloc(strlen(who) + 1);
  strcpy(inv->who, who);
  inv->timer = now;
}

/* removes a nick from the channel member list (returns 1 if successful) */
static int killmember(struct chanset_t *chan, char *nick)
{
  memberlist *x, *old;

  x = chan->channel.member;
  old = NULL;
  while (x->nick[0] && rfc_casecmp(x->nick, nick)) {
    old = x;
    x = x->next;
  }
  if (!x->nick[0] && !channel_pending(chan)) {
    putlog(LOG_MISC, "*", "(!) killmember(%s) -> nonexistent", nick);
    return 0;
  }
  if (old)
    old->next = x->next;
  else
    chan->channel.member = x->next;
  nfree(x);
  chan->channel.members--;
  return 1;
}

/* am i a chanop? */
static int me_op(struct chanset_t *chan)
{
  memberlist *mx = NULL;

  mx = ismember(chan, botname);
  if (!mx)
    return 0;
  if (chan_hasop(mx))
    return 1;
  else
    return 0;
}

/* are there any ops on the channel? */
static int any_ops(struct chanset_t *chan)
{
  memberlist *x = chan->channel.member;

  while (x->nick[0] && !chan_hasop(x))
    x = x->next;
  if (!x->nick[0])
    return 0;
  return 1;
}

/* reset the channel information */
static void reset_chan_info(struct chanset_t *chan)
{
  /* don't reset the channel if we're already resetting it */
  context;
  if (channel_inactive(chan)) {
    dprintf(DP_MODE,"PART %s\n",chan->name);
    return;
  }
  if (!channel_pending(chan)) {
    clear_channel(chan, 1);
    chan->status |= CHAN_PEND;
    chan->status &= ~CHAN_ACTIVE;
    if (!(chan->status & CHAN_ASKEDBANS)) {
      chan->status |= CHAN_ASKEDBANS;
      dprintf(DP_MODE, "MODE %s +b\n", chan->name);
    }
    if (!(chan->ircnet_status & CHAN_ASKED_EXEMPTS) &&
	use_exempts == 1) {
      chan->ircnet_status |= CHAN_ASKED_EXEMPTS;
      dprintf(DP_MODE, "MODE %s +e\n", chan->name);
    }
    if (!(chan->ircnet_status & CHAN_ASKED_INVITED) &&
	use_invites == 1) {
      chan->ircnet_status |= CHAN_ASKED_INVITED;
      dprintf(DP_MODE, "MODE %s +I\n", chan->name);
    }
    /* these 2 need to get out asap, so into the mode queue */
    if (use_354)
      dprintf(DP_MODE, "WHO %s %%c%%h%%n%%u%%f\n", chan->name);
    else
      dprintf(DP_MODE, "WHO %s\n", chan->name);
    dprintf(DP_MODE, "MODE %s\n", chan->name);
    /* this is not so critical, so slide it into the standard q */
    dprintf(DP_SERVER, "TOPIC %s\n", chan->name);
    /* clear_channel nuked the data...so */
  }
}

/* log the channel members */
static void log_chans()
{
  banlist *b;
  exemptlist *e;
  invitelist *i;
  memberlist *m;
  struct chanset_t *chan;
  int chops, bans, invites,exempts;
  chan = chanset;
  while (chan != NULL) {
    if (channel_active(chan) && channel_logstatus(chan) && !channel_inactive(chan)) {    
      m = chan->channel.member;
      chops = 0;
      while (m->nick[0]) {
	if (chan_hasop(m))
	  chops++;
	m = m->next;
      }
      b = chan->channel.ban;
      bans = 0;
      while (b->ban[0]) {
	bans++;
	b = b->next;
      }
      e = chan->channel.exempt;
      exempts = 0;
      while (e->exempt[0]) {
	exempts++;
	e = e->next;
      }
      i = chan->channel.invite;
      invites = 0;
      while (i->invite[0]) {
	invites++;
	i = i->next;
      }
      putlog(LOG_MISC, chan->name, "%-10s: %d member%c (%d chop%s, %2d ba%s %s",
	     chan->name, chan->channel.members, chan->channel.members == 1 ? ' ' : 's',
	     chops, chops == 1 ? ")" : "s)", bans, bans == 1 ? "n" : "ns", me_op(chan) ? "" :
	     "(not op'd)");
      if ((use_invites == 1) || (use_exempts == 1)) {
	putlog(LOG_MISC, chan->name, "%-10s: %d exemptio%s, %d invit%s",
	       chan->name, exempts,
	       exempts == 1 ? "n" : "ns", invites, invites == 1 ? "e" : "es");
      }
    }
    chan = chan->next;
  }
}

/*  if i'm the only person on the channel, and i'm not op'd,
 *  might as well leave and rejoin. If i'm NOT the only person
 *  on the channel, but i'm still not op'd, demand ops */
static void check_lonely_channel(struct chanset_t *chan)
{
  memberlist *m;
  char s[UHOSTLEN];
  int i = 0;
  static int whined = 0;

  context;
  if (channel_pending(chan) || !channel_active(chan) || me_op(chan) ||
      channel_inactive(chan))
    return;
  m = chan->channel.member;
  /* count non-split channel members */
  while (m->nick[0]) {
    if (!chan_issplit(m))
      i++;
    m = m->next;
  }
  if ((i == 1) && channel_cycle(chan)) {
    if (chan->name[0] != '+') {	/* Its pointless to cycle + chans for ops */
      putlog(LOG_MISC, "*", "Trying to cycle %s to regain ops.", chan->name);
      dprintf(DP_MODE, "PART %s\n", chan->name);
      dprintf(DP_MODE, "JOIN %s %s\n", chan->name, chan->key_prot);
      whined = 0;
    }
  } else if (any_ops(chan)) {
    whined = 0;
    if (chan->need_op[0])
      do_tcl("need-op", chan->need_op);
  } else {
    /* other people here, but none are ops */
    /* are there other bots?  make them LEAVE. */
    int ok = 1;

    if (!whined) {
      if (chan->name[0] != '+')	/* Once again, + is opless.
				 * complaining about no ops when without
				 * special help(services), we cant get
				 * them - Raist */
	putlog(LOG_MISC, "*", "%s is active but has no ops :(", chan->name);
      whined = 1;
    }
    m = chan->channel.member;
    while (m->nick[0]) {
      struct userrec *u;

      sprintf(s, "%s!%s", m->nick, m->userhost);
      u = get_user_by_host(s);
      if (!match_my_nick(m->nick) && (!u || !(u->flags & USER_BOT)))
	ok = 0;
      m = m->next;
    }
    if (ok) {
      /* ALL bots!  make them LEAVE!!! */
      m = chan->channel.member;
      while (m->nick[0]) {
	if (!match_my_nick(m->nick))
	  dprintf(DP_SERVER, "PRIVMSG %s :go %s\n", m->nick, chan->name);
	m = m->next;
      }
    } else {
      /* some humans on channel, but still op-less */
      if (chan->need_op[0])
	do_tcl("need-op", chan->need_op);
    }
  }
}

static void check_expired_chanstuff()
{
  banlist *b;
  exemptlist *e;
  invitelist *inv;
  memberlist *m, *n;
  char s[UHOSTLEN], *snick, *sfrom;
  struct chanset_t *chan;
  struct flag_record fr =
  {
    FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0
  };
  static int count = 4;
  int ok = 0;

  if (!server_online)
    return;
  for (chan = chanset; chan; chan = chan->next) {
    if (!(chan->status & (CHAN_ACTIVE | CHAN_PEND)) && !channel_inactive(chan) && server_online) {
      dprintf(DP_MODE, "JOIN %s %s\n", chan->name, chan->key_prot);      
    }
    if ((chan->status & (CHAN_ACTIVE | CHAN_PEND)) && channel_inactive(chan)) {
      dprintf(DP_MODE, "PART %s\n", chan->name);
    }
    if (channel_dynamicbans(chan) && me_op(chan) && !channel_inactive(chan) && ismember(chan, botname)) {
      for (b = chan->channel.ban; b->ban[0]; b = b->next) {
	if ((ban_time != 0) && (((now - b->timer) > (60 * ban_time)) &&
				!u_sticky_ban(chan->bans, b->ban) &&
				!u_sticky_ban(global_bans, b->ban))) {
	  strcpy(s, b->who); sfrom = s; snick = splitnick(&sfrom);
	  if (force_expire || channel_clearbans(chan) ||
	      !(snick[0] && strcasecmp(sfrom, botuserhost) &&
		(m=ismember(chan, snick)) &&
		m->user && (m->user->flags & USER_BOT) && chan_hasop(m))) {
	  putlog(LOG_MODES, chan->name,
		 "(%s) Channel ban on %s expired.",
		 chan->name, b->ban);
	  add_mode(chan, '-', 'b', b->ban);
	  b->timer = now;
	}
      }
    }
    }
    if (use_exempts == 1) {
      if (channel_dynamicexempts(chan) && me_op(chan)) {
	for (e = chan->channel.exempt; e->exempt[0]; e = e->next) {
	  if ((exempt_time != 0) && 
	      (((now - e->timer) > (60 * exempt_time)) &&
	       !u_sticky_exempt(chan->exempts, e->exempt) && 
	       !u_sticky_exempt(global_exempts,e->exempt))) {
 	    strcpy(s, e->who); sfrom = s; snick = splitnick(&sfrom);
	    if (force_expire || channel_clearbans(chan) ||
		!(snick[0] && strcasecmp(sfrom, botuserhost) &&
		  (m=ismember(chan, snick)) &&
		  m->user && (m->user->flags & USER_BOT) && chan_hasop(m))) {
	    /* Check to see if it matches a ban */
        /* Leave this extra logging in for now. Can be removed later
         * Jason */
        int match = 0;
        b = chan->channel.ban;
        while (b->ban[0] && !match) {
          if (wild_match(b->ban, e->exempt) ||
              wild_match(e->exempt,b->ban))
            match=1;
          else
            b = b->next;
        }
        if (match) {
		  putlog(LOG_MODES, chan->name,
             "(%s) Channel exemption %s NOT expired. Ban still set!",
		     chan->name, e->exempt);
	    } else {
	      putlog(LOG_MODES, chan->name,
		     "(%s) Channel exemption on %s expired.",
		     chan->name, e->exempt);
	      add_mode(chan, '-', 'e', e->exempt);
	    }
	    e->timer = now;
	  }
	}
      }
    }
    }

    if (use_invites == 1) {
      if (channel_dynamicinvites(chan) && me_op(chan)) {
	for (inv = chan->channel.invite; inv->invite[0]; inv = inv->next) {
	  if ((invite_time != 0) &&
	      (((now - inv->timer) > (60 * invite_time)) &&
	       !u_sticky_invite(chan->invites, inv->invite) && 
	       !u_sticky_invite(global_invites,inv->invite))) {
 	    strcpy(s, inv->who); sfrom = s; snick = splitnick(&sfrom);
	    if (force_expire || channel_clearbans(chan) ||
		!(snick[0] && strcasecmp(sfrom, botuserhost) &&
		  (m=ismember(chan, snick)) &&
		  m->user && (m->user->flags & USER_BOT) && chan_hasop(m))) {
        if ((chan->channel.mode & CHANINV) && isinvited(chan,inv->invite)) {
	      /*Leave this extra logging in for now. Can be removed later
	       * Jason */
	      putlog(LOG_MODES, chan->name,
             "(%s) Channel invitation %s NOT expired. i mode still set!",
		     chan->name, inv->invite);
	    } else {
	      putlog(LOG_MODES, chan->name,
		     "(%s) Channel invitation on %s expired.",
		     chan->name, inv->invite);
	      add_mode(chan, '-', 'I', inv->invite);
	    }
	    inv->timer = now;
	    }
	  }
	}
      }
    }
    m = chan->channel.member;
    while (m->nick[0]) {
      if (m->split) {
	n = m->next;
	if (!channel_active(chan))
	  killmember(chan, m->nick);
	else if ((now - m->split) > wait_split) {
	  sprintf(s, "%s!%s", m->nick, m->userhost);
	  m->user = get_user_by_host(s);
	  check_tcl_sign(m->nick, m->userhost, m->user, chan->name,
			 "lost in the netsplit");
	  putlog(LOG_JOIN, chan->name,
		 "%s (%s) got lost in the net-split.",
		 m->nick, m->userhost);
	  killmember(chan, m->nick);
	}
	m = n;
      } else
	m = m->next;
    }
    if (channel_active(chan) && (chan->idle_kick)) {
      m = chan->channel.member;
      while (m->nick[0]) {
	if ((now - m->last) >= (chan->idle_kick * 60) &&
	    !match_my_nick(m->nick)) {
	  sprintf(s, "%s!%s", m->nick, m->userhost);
	  m->user = get_user_by_host(s);
	  get_user_flagrec(m->user, &fr, chan->name);
	  if (!(glob_bot(fr) || glob_friend(fr) ||
		(glob_op(fr) && !glob_deop(fr)) ||
		chan_friend(fr) || chan_op(fr)))
	    dprintf(DP_SERVER, "KICK %s %s :idle %d min\n", chan->name,
		    m->nick, chan->idle_kick);
	}
	m = m->next;
      }
    }
    check_lonely_channel(chan);
  }
  if (min_servs > 0) {
    for (chan = chanset; chan; chan = chan->next)
      if (channel_active(chan) && (chan->channel.members == 1))
	ok = 1;
    if (ok) {
      count++;
      if (count >= 5) {
	dprintf(DP_SERVER, "LUSERS\n");
	count = 0;
      }
    }
  }
}

static int channels_6char STDVAR {
  Function F = (Function) cd;
  char x[20];

  BADARGS(7, 7, " nick user@host handle desto/chan keyword/nick text");
  CHECKVALIDITY(channels_6char);
  sprintf(x, "%d", F(argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]));
  Tcl_AppendResult(irp, x, NULL);
  return TCL_OK;
}

static int channels_5char STDVAR {
  Function F = (Function) cd;

  BADARGS(6, 6, " nick user@host handle channel text");
  CHECKVALIDITY(channels_5char);
  F(argv[1], argv[2], argv[3], argv[4], argv[5]);
  return TCL_OK;
}

static int channels_4char STDVAR {
  Function F = (Function) cd;

  BADARGS(5, 5, " nick uhost hand chan/param");
  CHECKVALIDITY(channels_4char);
  F(argv[1], argv[2], argv[3], argv[4]);
  return TCL_OK;
}

static void check_tcl_joinpart(char *nick, char *uhost, struct userrec *u,
			       char *chname, p_tcl_bind_list table)
{
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  char args[1024];

  context;
  simple_sprintf(args, "%s %s!%s", chname, nick, uhost);
  get_user_flagrec(u, &fr, chname);
  Tcl_SetVar(interp, "_jp1", nick, 0);
  Tcl_SetVar(interp, "_jp2", uhost, 0);
  Tcl_SetVar(interp, "_jp3", u ? u->handle : "*", 0);
  Tcl_SetVar(interp, "_jp4", chname, 0);
  context;
  check_tcl_bind(table, args, &fr, " $_jp1 $_jp2 $_jp3 $_jp4",
		 MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE);
  context;
}

static void check_tcl_signtopcnick(char *nick, char *uhost, struct userrec *u,
				   char *chname, char *reason,
				   p_tcl_bind_list table)
{
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  char args[1024];

  context;
  if (table == H_sign)
    simple_sprintf(args, "%s %s!%s", chname, nick, uhost);
  else
    simple_sprintf(args, "%s %s", chname, reason);
  get_user_flagrec(u, &fr, chname);
  Tcl_SetVar(interp, "_stnm1", nick, 0);
  Tcl_SetVar(interp, "_stnm2", uhost, 0);
  Tcl_SetVar(interp, "_stnm3", u ? u->handle : "*", 0);
  Tcl_SetVar(interp, "_stnm4", chname, 0);
  Tcl_SetVar(interp, "_stnm5", reason, 0);
  context;
  check_tcl_bind(table, args, &fr, " $_stnm1 $_stnm2 $_stnm3 $_stnm4 $_stnm5",
		 MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE);
  context;
}

static void check_tcl_kickmode(char *nick, char *uhost, struct userrec *u,
			       char *chname, char *dest, char *reason,
			       p_tcl_bind_list table)
{
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  char args[512];

  context;
  get_user_flagrec(u, &fr, chname);
  if (table == H_mode)
    simple_sprintf(args, "%s %s", chname, dest);
  else
    simple_sprintf(args, "%s %s %s", chname, dest, reason);
  Tcl_SetVar(interp, "_kick1", nick, 0);
  Tcl_SetVar(interp, "_kick2", uhost, 0);
  Tcl_SetVar(interp, "_kick3", u ? u->handle : "*", 0);
  Tcl_SetVar(interp, "_kick4", chname, 0);
  Tcl_SetVar(interp, "_kick5", dest, 0);
  Tcl_SetVar(interp, "_kick6", reason, 0);
  context;
  check_tcl_bind(table, args, &fr, " $_kick1 $_kick2 $_kick3 $_kick4 $_kick5 $_kick6",
		 MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE);
  context;
}

static int check_tcl_pub(char *nick, char *from, char *chname, char *msg)
{
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  int x;
  char buf[512], *args = buf, *cmd, host[161], *hand;
  struct userrec *u;

  context;
  strcpy(args, msg);
  cmd = newsplit(&args);
  simple_sprintf(host, "%s!%s", nick, from);
  u = get_user_by_host(host);
  hand = u ? u->handle : "*";
  get_user_flagrec(u, &fr, chname);
  Tcl_SetVar(interp, "_pub1", nick, 0);
  Tcl_SetVar(interp, "_pub2", from, 0);
  Tcl_SetVar(interp, "_pub3", hand, 0);
  Tcl_SetVar(interp, "_pub4", chname, 0);
  Tcl_SetVar(interp, "_pub5", args, 0);
  context;
  x = check_tcl_bind(H_pub, cmd, &fr, " $_pub1 $_pub2 $_pub3 $_pub4 $_pub5",
		     MATCH_EXACT | BIND_USE_ATTR | BIND_HAS_BUILTINS);
  context;
  if (x == BIND_NOMATCH)
    return 0;
  if (x == BIND_EXEC_LOG)
    putlog(LOG_CMDS, chname, "<<%s>> !%s! %s %s", nick, hand, cmd, args);
  return 1;
}

static void check_tcl_pubm(char *nick, char *from, char *chname, char *msg)
{
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  char buf[1024], host[161];
  struct userrec *u;

  context;
  simple_sprintf(buf, "%s %s", chname, msg);
  simple_sprintf(host, "%s!%s", nick, from);
  u = get_user_by_host(host);
  get_user_flagrec(u, &fr, chname);
  Tcl_SetVar(interp, "_pubm1", nick, 0);
  Tcl_SetVar(interp, "_pubm2", from, 0);
  Tcl_SetVar(interp, "_pubm3", u ? u->handle : "*", 0);
  Tcl_SetVar(interp, "_pubm4", chname, 0);
  Tcl_SetVar(interp, "_pubm5", msg, 0);
  context;
  check_tcl_bind(H_pubm, buf, &fr, " $_pubm1 $_pubm2 $_pubm3 $_pubm4 $_pubm5",
		 MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE);
  context;
}

static tcl_ints myints[] =
{
  {"learn-users", &learn_users, 0},	/* arthur2 */
  {"wait-split", &wait_split, 0},
  {"wait-info", &wait_info, 0},
  {"bounce-bans", &bounce_bans, 0},
  {"bounce-exempts", &bounce_exempts, 0},
  {"bounce-invites", &bounce_invites, 0},
  {"bounce-modes", &bounce_modes, 0},
  {"bounce-bogus-bans", &bounce_bogus_bans, 0},
  {"kick-bogus-bans", &kick_bogus_bans, 0},
  {"bounce-bogus-exempts", &bounce_bogus_exempts, 0},
  {"kick-bogus-exempts", &kick_bogus_exempts, 0},
  {"bounce-bogus-invites", &bounce_bogus_invites, 0},
  {"kick-bogus-invites", &kick_bogus_invites, 0},
  {"modes-per-line", &modesperline, 0},
  {"mode-buf-length", &mode_buf_len, 0},
  {"use-354", &use_354, 0},
  {"kick-method", &kick_method, 0},
  {"kick-bogus", &kick_bogus, 0},
  {"ban-bogus", &ban_bogus, 0},
  {"kick-fun", &kick_fun, 0},
  {"ban-fun", &ban_fun, 0},
  {"invite-key", &invite_key, 0},
  {"allow-desync", &allow_desync, 0},
  {"no-chanrec-info", &no_chanrec_info, 0},
  {"max-bans", &max_bans, 0},
  {"max-exempts", &max_exempts, 0},
  {"max-invites", &max_invites, 0},
  {"max-modes", &max_modes, 0},
  {"net-type", &net_type, 0},
  {"strict-host", &strict_host, 0},	/* arthur2 */
  {"ctcp-mode", &ctcp_mode, 0},	/* arthur2 */
  {"keep-nick", &keepnick, 0},	/* guppy */
  {0, 0, 0}			/* arthur2 */
};

static tcl_strings mystrings[] =
{
  {"altnick", altnick, NICKMAX, 0},
  {0, 0, 0, 0}
};

/* for EVERY channel */
static void flush_modes()
{
  struct chanset_t *chan;

  chan = chanset;
  while (chan != NULL) {
    flush_mode(chan, NORMAL);
    chan = chan->next;
  }
}

static void irc_report(int idx, int details)
{
  struct flag_record fr =
  {
    FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0
  };
  char ch[1024], q[160], *p;
  int k, l;
  struct chanset_t *chan;

  strcpy(q, "Channels: ");
  k = 10;
  for (chan = chanset; chan; chan = chan->next) {
    if (idx != DP_STDOUT)
      get_user_flagrec(dcc[idx].user, &fr, chan->name);
    if ((idx == DP_STDOUT) || glob_master(fr) || chan_master(fr)) {
      p = NULL;
      if (!channel_inactive(chan)) {
	if (!(chan->status & CHAN_ACTIVE))
	  p = MISC_TRYING;
	else if (chan->status & CHAN_PEND)
	  p = MISC_PENDING;
	else if (!me_op(chan))
	  p = MISC_WANTOPS;
      }
 /*     else
	     p = MISC_INACTIVE; */
      l = simple_sprintf(ch, "%s%s%s%s, ", chan->name, p ? "(" : "",
			 p ? p : "", p ? ")" : "");
      if ((k + l) > 70) {
	dprintf(idx, "   %s\n", q);
	strcpy(q, "          ");
	k = 10;
      }
      k += my_strcpy(q + k, ch);
    }
  }
  if (k > 10) {
    q[k - 2] = 0;
    dprintf(idx, "    %s\n", q);
  }
}

static void do_nettype()
{
  switch (net_type) {
  case 0:		/* EfNet except new +e/+I hybrid */
    kick_method = 1;
    modesperline = 4;
    use_354 = 0;
    use_silence = 0;
    use_exempts = 0;
    use_invites = 0;
    break;
  case 1:		/* Ircnet */
    kick_method = 4;
    modesperline = 3;
    use_354 = 0;
    use_silence = 0;
    use_exempts = 1;
    use_invites = 1;
    break;
  case 2:		/* Undernet */
    kick_method = 1;
    modesperline = 6;
    use_354 = 1;
    use_silence = 1;
    use_exempts = 0;
    use_invites = 0;
    break;
  case 3:		/* Dalnet */
    kick_method = 1;
    modesperline = 6;
    use_354 = 0;
    use_silence = 0;
    use_exempts = 0;
    use_invites = 0;
    break;
  case 4:		/* new +e/+I Efnet hybrid */
    kick_method = 1;
    modesperline = 4;
    use_354 = 0;
    use_silence = 0;
    use_exempts = 1;
    use_invites = 1;
    break;
  default:
    break;
  }
}

static char *traced_nettype(ClientData cdata, Tcl_Interp * irp, char *name1,
			    char *name2, int flags)
{
  do_nettype();
  return NULL;
}

static int irc_expmem()
{
  return 0;
}

static char *irc_close()
{
  struct chanset_t *chan;

  context;
  for (chan = chanset; chan; chan = chan->next) {
    dprintf(DP_MODE, "PART %s\n", chan->name);
    clear_channel(chan, 1);
  }
  context;
  del_bind_table(H_topc);
  del_bind_table(H_splt);
  del_bind_table(H_sign);
  del_bind_table(H_rejn);
  del_bind_table(H_part);
  del_bind_table(H_nick);
  del_bind_table(H_mode);
  del_bind_table(H_kick);
  del_bind_table(H_join);
  del_bind_table(H_pubm);
  del_bind_table(H_pub);
  context;
  rem_tcl_ints(myints);
  rem_tcl_strings(mystrings);
  rem_builtins(H_dcc, irc_dcc, 18);
  rem_builtins(H_msg, C_msg, 20);
  rem_builtins(H_raw, irc_raw, 28);
  rem_tcl_commands(tclchan_cmds);
  rem_help_reference("irc.help");
  context;
  del_hook(HOOK_MINUTELY, check_expired_chanstuff);
  del_hook(HOOK_5MINUTELY, log_chans);
  del_hook(HOOK_ADD_MODE, real_add_mode);
  del_hook(HOOK_IDLE, flush_modes);
  Tcl_UntraceVar(interp, "net-type",
		 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		 traced_nettype, NULL);
  context;
  module_undepend(MODULE_NAME);
  context;
  return NULL;
}

char *irc_start();

static Function irc_table[] =
{
  /* 0 - 3 */
  (Function) irc_start,
  (Function) irc_close,
  (Function) irc_expmem,
  (Function) irc_report,
  /* 4 - 7 */
  (Function) & H_splt,
  (Function) & H_rejn,
  (Function) & H_nick,
  (Function) & H_sign,
  /* 8 - 11 */
  (Function) & H_join,
  (Function) & H_part,
  (Function) & H_mode,
  (Function) & H_kick,
  /* 12 - 15 */
  (Function) & H_pubm,
  (Function) & H_pub,
  (Function) & H_topc,
  (Function) recheck_channel,
  /* 16 - 19 */
  (Function) me_op,
};

char *server_start();

char *irc_start(Function * global_funcs)
{
  struct chanset_t *chan;

  global = global_funcs;

  context;
  module_register(MODULE_NAME, irc_table, 1, 1);
  if (!(server_funcs = module_depend(MODULE_NAME, "server", 1, 0)))
    return "You need the server module to use the irc module.";
  if (!(channels_funcs = module_depend(MODULE_NAME, "channels", 1, 0))) {
    module_undepend(MODULE_NAME);
    return "You need the channels module to use the irc module.";
  }
  context;
  for (chan = chanset; chan; chan = chan->next) {
    if (!channel_inactive(chan))
      dprintf(DP_MODE, "JOIN %s %s\n", chan->name, chan->key_prot);      
    chan->status &= ~(CHAN_ACTIVE | CHAN_PEND | CHAN_ASKEDBANS);
    chan->ircnet_status &= ~(CHAN_ASKED_INVITED | CHAN_ASKED_EXEMPTS);
  }
  context;
  add_hook(HOOK_MINUTELY, check_expired_chanstuff);
  add_hook(HOOK_5MINUTELY, log_chans);
  add_hook(HOOK_ADD_MODE, real_add_mode);
  add_hook(HOOK_IDLE, flush_modes);
  Tcl_TraceVar(interp, "net-type",
	       TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
	       traced_nettype, NULL);
  context;
  add_tcl_ints(myints);
  add_tcl_strings(mystrings);
  add_builtins(H_dcc, irc_dcc, 18);
  add_builtins(H_msg, C_msg, 20);
  add_builtins(H_raw, irc_raw, 28);
  add_tcl_commands(tclchan_cmds);
  add_help_reference("irc.help");
  context;
  H_topc = add_bind_table("topc", HT_STACKABLE, channels_5char);
  H_splt = add_bind_table("splt", HT_STACKABLE, channels_4char);
  H_sign = add_bind_table("sign", HT_STACKABLE, channels_5char);
  H_rejn = add_bind_table("rejn", HT_STACKABLE, channels_4char);
  H_part = add_bind_table("part", HT_STACKABLE, channels_4char);
  H_nick = add_bind_table("nick", HT_STACKABLE, channels_5char);
  H_mode = add_bind_table("mode", HT_STACKABLE, channels_6char);
  H_kick = add_bind_table("kick", HT_STACKABLE, channels_6char);
  H_join = add_bind_table("join", HT_STACKABLE, channels_4char);
  H_pubm = add_bind_table("pubm", HT_STACKABLE, channels_5char);
  H_pub = add_bind_table("pub", 0, channels_5char);
  context;
  do_nettype();
  context;
  return NULL;
}
