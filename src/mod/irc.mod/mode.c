/* 
 * mode.c -- part of irc.mod
 *   queueing and flushing mode changes made by the bot
 *   channel mode changes and the bot's reaction to them
 *   setting and getting the current wanted channel modes
 * 
 * $Id: mode.c,v 1.32 2000/09/02 19:34:36 fabian Exp $
 */
/* 
 * Copyright (C) 1997  Robey Pointer
 * Copyright (C) 1999, 2000  Eggheads
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

/* Reversing this mode? */
static int reversing = 0;

#define PLUS    1
#define MINUS   2
#define CHOP    4
#define BAN     8
#define VOICE   16
#define EXEMPT  32
#define INVITE  64

static struct flag_record user   = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
static struct flag_record victim = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

static void flush_mode(struct chanset_t *chan, int pri)
{
  char *p, out[512], post[512];
  int i, ok = 0;

  p = out;
  post[0] = 0;
  if (chan->pls[0])
    *p++ = '+';
  for (i = 0; i < strlen(chan->pls); i++)
    *p++ = chan->pls[i];
  if (chan->mns[0])
    *p++ = '-';
  for (i = 0; i < strlen(chan->mns); i++)
    *p++ = chan->mns[i];
  chan->pls[0] = 0;
  chan->mns[0] = 0;
  chan->bytes = 0;
  chan->compat = 0;
  ok = 0;
  /* +k or +l ? */
  if (chan->key != NULL) {
    if (!ok) {
      *p++ = '+';
      ok = 1;
    }
    *p++ = 'k';
    strcat(post, chan->key);
    strcat(post, " ");
    nfree(chan->key);
    chan->key = NULL;
  }
  if (chan->limit != -1) {
    if (!ok) {
      *p++ = '+';
      ok = 1;
    }
    *p++ = 'l';
    sprintf(&post[strlen(post)], "%d ", chan->limit);
    chan->limit = -1;
  }
  /* Do -b before +b to avoid server ban overlap ignores */
  for (i = 0; i < modesperline; i++)
    if ((chan->cmode[i].type & MINUS) && (chan->cmode[i].type & BAN)) {
      if (ok < 2) {
	*p++ = '-';
	ok = 2;
      }
      *p++ = 'b';
      strcat(post, chan->cmode[i].op);
      strcat(post, " ");
      nfree(chan->cmode[i].op);
      chan->cmode[i].op = NULL;
    }
  ok &= 1;
  for (i = 0; i < modesperline; i++)
    if (chan->cmode[i].type & PLUS) {
      if (!ok) {
	*p++ = '+';
	ok = 1;
      }
      *p++ = ((chan->cmode[i].type & BAN) ? 'b' :
	      ((chan->cmode[i].type & CHOP) ? 'o' : 
	       ((chan->cmode[i].type & EXEMPT) ? 'e' : 
		((chan->cmode[i].type & INVITE) ? 'I' : 'v'))));
      strcat(post, chan->cmode[i].op);
      strcat(post, " ");
      nfree(chan->cmode[i].op);
      chan->cmode[i].op = NULL;
    }
  ok = 0;
  /* -k ? */
  if (chan->rmkey != NULL) {
    if (!ok) {
      *p++ = '-';
      ok = 1;
    }
    *p++ = 'k';
    strcat(post, chan->rmkey);
    strcat(post, " ");
    nfree(chan->rmkey);
    chan->rmkey = NULL;
  }
  for (i = 0; i < modesperline; i++)
    if ((chan->cmode[i].type & MINUS) && !(chan->cmode[i].type & BAN)) {
      if (!ok) {
	*p++ = '-';
	ok = 1;
      }
	 *p++ = ((chan->cmode[i].type & CHOP) ? 'o' : 
		 ((chan->cmode[i].type & EXEMPT) ? 'e' : 
		  ((chan->cmode[i].type & INVITE) ? 'I' : 'v')));
      strcat(post, chan->cmode[i].op);
      strcat(post, " ");
      nfree(chan->cmode[i].op);
      chan->cmode[i].op = NULL;
    }
  *p = 0;
  for (i = 0; i < modesperline; i++)
    chan->cmode[i].type = 0;
  if (post[0] && post[strlen(post) - 1] == ' ')
    post[strlen(post) - 1] = 0;
  if (post[0]) {
    strcat(out, " ");
    strcat(out, post);
  }
  if (out[0]) {
    if (pri == QUICK)
      dprintf(DP_MODE, "MODE %s %s\n", chan->name, out);
    else
      dprintf(DP_SERVER, "MODE %s %s\n", chan->name, out);
  }
}

/* Queue a channel mode change
 */
static void real_add_mode(struct chanset_t *chan,
			  char plus, char mode, char *op)
{
  int i, type, ok, l;
  masklist *m;
  memberlist *mx;
  char s[21];
  
  Context;
  if (!me_op(chan))
    return;			/* No point in queueing the mode */

  if (mode == 'o' || mode == 'v') {
    mx = ismember(chan, op);
    if (!mx)
      return;

    if (plus == '-' && mode == 'o') {
      if (chan_sentdeop(mx) || !chan_hasop(mx))
	return;
      mx->flags |= SENTDEOP;
    }
    if (plus == '+' && mode == 'o') {
      if (chan_sentop(mx) || chan_hasop(mx))
	return;
      mx->flags |= SENTOP;
    }
    if (plus == '-' && mode == 'v') {
      if (chan_sentdevoice(mx) || !chan_hasvoice(mx))
	return;
      mx->flags |= SENTDEVOICE;
    }
    if (plus == '+' && mode == 'v') {
      if (chan_sentvoice(mx) || chan_hasvoice(mx))
	return;
      mx->flags |= SENTVOICE;
    }
  }

  if (chan->compat == 0) {
    if (mode == 'e' || mode == 'I')
      chan->compat = 2;
    else
      chan->compat = 1;
  } else if (mode == 'e' || mode == 'I') {
    if (prevent_mixing && chan->compat == 1)
      flush_mode(chan, NORMAL);
  } else if (prevent_mixing && chan->compat == 2)
    flush_mode(chan, NORMAL);

  if (mode == 'o' || mode == 'b' || mode == 'v' || mode == 'e' || mode == 'I') {
    type = (plus == '+' ? PLUS : MINUS) |
	   (mode == 'o' ? CHOP :
	    (mode == 'b' ? BAN :
	     (mode == 'v' ? VOICE :
	      (mode == 'e' ? EXEMPT : INVITE))));

    /* 
     * FIXME: Some networks remove overlapped bans, IrcNet does not
     *        (poptix/drummer)
     *
     * Note:  On ircnet ischanXXX() should be used, otherwise isXXXed().
     */
    /* If removing a non-existant mask... */
    if ((plus == '-' &&
         ((mode == 'b' && !ischanban(chan, op)) ||
          (mode == 'e' && !ischanexempt(chan, op)) ||
          (mode == 'I' && !ischaninvite(chan, op)))) ||
        /* or adding an existant mask... */
        (plus == '+' &&
         ((mode == 'b' && ischanban(chan, op)) ||
          (mode == 'e' && ischanexempt(chan, op)) ||
          (mode == 'I' && ischaninvite(chan, op)))))
      return;	/* ...nuke it */

    /* If there are already max_bans bans, max_exempts exemptions,
     * max_invites invitations or max_modes +b/+e/+I modes on the
     * channel, don't try to add one more.
     */
    if (plus == '+' && (mode == 'b' || mode == 'e' || mode == 'I')) {
      int	bans = 0, exempts = 0, invites = 0;

      for (m = chan->channel.ban; m && m->mask[0]; m = m->next)
	bans++;
      if (mode == 'b')
	if (bans >= max_bans)
	  return;

      for (m = chan->channel.exempt; m && m->mask[0]; m = m->next)
	exempts++;
      if (mode == 'e')
	if (exempts >= max_exempts)
	  return;

      for (m = chan->channel.invite; m && m->mask[0]; m = m->next)
	invites++;
      if (mode == 'I')
	if (invites >= max_invites)
	  return;

      if (bans + exempts + invites >= max_modes)
	return;
    }

    /* op-type mode change */
    for (i = 0; i < modesperline; i++)
      if (chan->cmode[i].type == type && chan->cmode[i].op != NULL &&
	  !rfc_casecmp(chan->cmode[i].op, op))
	return;			/* Already in there :- duplicate */
    ok = 0;			/* Add mode to buffer */
    l = strlen(op) + 1;
    if (chan->bytes + l > mode_buf_len)
      flush_mode(chan, NORMAL);
    for (i = 0; i < modesperline; i++)
      if (chan->cmode[i].type == 0 && !ok) {
	chan->cmode[i].type = type;
	chan->cmode[i].op = (char *) channel_malloc(l);
	chan->bytes += l;	/* Add 1 for safety */
	strcpy(chan->cmode[i].op, op);
	ok = 1;
      }
    ok = 0;			/* Check for full buffer */
    for (i = 0; i < modesperline; i++)
      if (chan->cmode[i].type == 0)
	ok = 1;
    if (!ok)
      flush_mode(chan, NORMAL);	/* Full buffer!  flush modes */
    return;
  }
  /* +k ? store key */
  if (plus == '+' && mode == 'k') {
    chan->key = (char *) channel_malloc(strlen(op) + 1);
    if (chan->key)
      strcpy(chan->key, op);
  }
  /* -k ? store removed key */
  else if (plus == '-' && mode == 'k') {
    chan->rmkey = (char *) channel_malloc(strlen(op) + 1);
    if (chan->rmkey)
      strcpy(chan->rmkey, op);
  }
  /* +l ? store limit */
  else if (plus == '+' && mode == 'l')
    chan->limit = atoi(op);
  else {
    /* Typical mode changes */
    if (plus == '+')
      strcpy(s, chan->pls);
    else
      strcpy(s, chan->mns);
    if (!strchr(s, mode)) {
      if (plus == '+') {
	chan->pls[strlen(chan->pls) + 1] = 0;
	chan->pls[strlen(chan->pls)] = mode;
      } else {
	chan->mns[strlen(chan->mns) + 1] = 0;
	chan->mns[strlen(chan->mns)] = mode;
      }
    }
  }
  ok = modesperline;			/* Check for full buffer. */
  for (i = 0; i < modesperline; i++)
    if (chan->cmode[i].type)
      ok--;
  if (include_lk && chan->limit != -1)
    ok--;
  if (include_lk && chan->rmkey)
    ok--;
  if (include_lk && chan->key)
    ok--;
  if (ok <= 0)
    flush_mode(chan, NORMAL);		/* Full buffer! Flush modes. */
}


/*
 *    Mode parsing functions
 */

static void got_key(struct chanset_t *chan, char *nick, char *from,
		    char *key)
{
  if ((!nick[0]) && (bounce_modes))
    reversing = 1;
  set_key(chan, key);
  if (((reversing) && !(chan->key_prot[0])) ||
      ((chan->mode_mns_prot & CHANKEY) &&
       !(glob_master(user) || glob_bot(user) || chan_master(user)))) {
    if (strlen(key) != 0) {
      add_mode(chan, '-', 'k', key);
    } else {
      add_mode(chan, '-', 'k', "");
    }
  }
}

static void got_op(struct chanset_t *chan, char *nick, char *from,
		   char *who, struct userrec *opu, struct flag_record *opper)
{
  memberlist *m;
  char s[UHOSTLEN];
  struct userrec *u;
  int check_chan = 0;
  int snm = chan->stopnethack_mode;

  m = ismember(chan, who);
  if (!m) {
    putlog(LOG_MISC, chan->dname, CHAN_BADCHANMODE, CHAN_BADCHANMODE_ARGS);
    dprintf(DP_MODE, "WHO %s\n", who);
    return;
  }
  /* Did *I* just get opped? */
  if (!me_op(chan) && match_my_nick(who))
    check_chan = 1;

  if (!m->user) {
    simple_sprintf(s, "%s!%s", m->nick, m->userhost);
    u = get_user_by_host(s);
  } else
    u = m->user;

  get_user_flagrec(u, &victim, chan->dname);
  /* Flags need to be set correctly right from the beginning now, so that
   * add_mode() doesn't get irritated.
   */
  m->flags |= CHANOP;
  check_tcl_mode(nick, from, opu, chan->dname, "+o", who);
  /* Added new meaning of WASOP:
   * in mode binds it means: was he op before get (de)opped
   * (stupid IrcNet allows opped users to be opped again and
   *  opless users to be deopped)
   * script now can use [wasop nick chan] proc to check
   * if user was op or wasnt  (drummer)
   */
  m->flags &= ~SENTOP;

  /* I'm opped, and the opper isn't me */
  if (me_op(chan) && !match_my_nick(who) &&
    /* and it isn't a server op */
	   nick[0]) {
    /* Channis is +bitch, and the opper isn't a global master or a bot */
    if (channel_bitch(chan) && !(glob_master(*opper) || glob_bot(*opper)) &&
	/* ... and the *opper isn't a channel master */
	!chan_master(*opper) &&
	/* ... and the oppee isn't global op/master/bot */
	!(glob_op(victim) || glob_bot(victim)) &&
	/* ... and the oppee isn't a channel op/master */
	!chan_op(victim))
      add_mode(chan, '-', 'o', who);
      /* Opped is channel +d or global +d */
    else if ((chan_deop(victim) ||
		(glob_deop(victim) && !chan_op(victim))) &&
	       !glob_master(*opper) && !chan_master(*opper))
      add_mode(chan, '-', 'o', who);
    else if (reversing)
      add_mode(chan, '-', 'o', who);
  } else if (reversing && !match_my_nick(who))
    add_mode(chan, '-', 'o', who);
  if (!nick[0] && me_op(chan) && !match_my_nick(who)) {
    if (chan_deop(victim) || (glob_deop(victim) && !chan_op(victim))) {
      m->flags |= FAKEOP;
      add_mode(chan, '-', 'o', who);
    } else if (snm > 0 && snm < 7 && !(m->delay)) {
      if (snm == 5) snm = channel_bitch(chan) ? 1 : 3;
      if (snm == 6) snm = channel_bitch(chan) ? 4 : 2;
      if (chan_wasoptest(victim) || glob_wasoptest(victim) ||
      snm == 2) {
        if (!chan_wasop(m)) {
          m->flags |= FAKEOP;
          add_mode(chan, '-', 'o', who);
        }
      } else if (!(chan_op(victim) ||
		 (glob_op(victim) && !chan_deop(victim)))) {
		  if (snm == 1 || snm == 4 || (snm == 3 && !chan_wasop(m))) {
		    add_mode(chan, '-', 'o', who);
		    m->flags |= FAKEOP;
        }
      } else if (snm == 4 && !chan_wasop(m)) {
        add_mode(chan, '-', 'o', who);
        m->flags |= FAKEOP;
      }
    }
  }
  m->flags |= WASOP;
  if (check_chan)
    recheck_channel(chan, 1);
}

static void got_deop(struct chanset_t *chan, char *nick, char *from,
		     char *who, struct userrec *opu)
{
  memberlist *m;
  char s[UHOSTLEN], s1[UHOSTLEN];
  struct userrec *u;
  int had_op;

  m = ismember(chan, who);
  if (!m) {
    putlog(LOG_MISC, chan->dname, CHAN_BADCHANMODE, CHAN_BADCHANMODE_ARGS);
    dprintf(DP_MODE, "WHO %s\n", who);
    return;
  }

  simple_sprintf(s, "%s!%s", m->nick, m->userhost);
  simple_sprintf(s1, "%s!%s", nick, from);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->dname);

  had_op = chan_hasop(m);
  /* Flags need to be set correctly right from the beginning now, so that
   * add_mode() doesn't get irritated.
   */
  m->flags &= ~(CHANOP | SENTDEOP | FAKEOP);
  check_tcl_mode(nick, from, opu, chan->dname, "-o", who);
  /* Check comments in got_op()  (drummer) */
  m->flags &= ~WASOP;

  /* Deop'd someone on my oplist? */
  if (me_op(chan)) {
    int ok = 1;

    if (glob_master(victim) || chan_master(victim))
      ok = 0;
    else if ((glob_op(victim) || glob_friend(victim)) && !chan_deop(victim))
      ok = 0;
    else if (chan_op(victim) || chan_friend(victim))
      ok = 0;
    if (!ok && !match_my_nick(nick) &&
       rfc_casecmp(who, nick) && had_op &&
	!match_my_nick(who)) {	/* added 25mar1996, robey */
      /* Do we want to reop? */
      /* Is the deopper NOT a master or bot? */
      if (!glob_master(user) && !chan_master(user) && !glob_bot(user) &&
	  /* and is the channel protectops? */
	  ((channel_protectops(chan) &&
	    /* and it's not +bitch ... */
	    (!channel_bitch(chan) ||
	    /* or the user's a valid op? */
	     chan_op(victim) || (glob_op(victim) && !chan_deop(victim)))) ||
	   /* or is the channel protectfriends? */
           (channel_protectfriends(chan) &&
	     /* and the users a valid friend? */
             (chan_friend(victim) || (glob_friend(victim) &&
				      !chan_deop(victim))))) &&
       /* and the users not a de-op? */
       !(chan_deop(victim) || (glob_deop(victim) && !chan_op(victim))))
	/* Then we'll bless them */
	add_mode(chan, '+', 'o', who);
      else if (reversing)
	add_mode(chan, '+', 'o', who);
    }
  }

  if (!nick[0])
    putlog(LOG_MODES, chan->dname, "TS resync (%s): %s deopped by %s",
	   chan->dname, who, from);
  /* Check for mass deop */
  if (nick[0])
    detect_chan_flood(nick, from, s1, chan, FLOOD_DEOP, who);
  /* Having op hides your +v status -- so now that someone's lost ops,
   * check to see if they have +v
   */
  if (!(m->flags & (CHANVOICE | STOPWHO))) {
    dprintf(DP_HELP, "WHO %s\n", m->nick);
    m->flags |= STOPWHO;
  }
  /* Was the bot deopped? */
  if (match_my_nick(who)) {
    /* Cancel any pending kicks and modes */
    memberlist *m2 = chan->channel.member;

    while (m2 && m2->nick[0]) {
	m2->flags &= ~(SENTKICK | SENTDEOP | SENTOP | SENTVOICE | SENTDEVOICE);
      m2 = m2->next;
    }
    check_tcl_need(chan->dname, "op");
    if (chan->need_op[0])
      do_tcl("need-op", chan->need_op);
    if (!nick[0])
      putlog(LOG_MODES, chan->dname, "TS resync deopped me on %s :(",
	     chan->dname);
  }
  if (nick[0])
    maybe_revenge(chan, s1, s, REVENGE_DEOP);
}


static void got_ban(struct chanset_t *chan, char *nick, char *from,
		    char *who)
{
  char me[UHOSTLEN], s[UHOSTLEN], s1[UHOSTLEN];
  int check = 1;
  memberlist *m;
  struct userrec *u;

  simple_sprintf(me, "%s!%s", botname, botuserhost);
  simple_sprintf(s, "%s!%s", nick, from);
  newban(chan, who, s);
  if (wild_match(who, me) && me_op(chan)) {
    /* First of all let's check whether some luser banned us ++rtc */
    if (match_my_nick(nick))
      /* Bot banned itself -- doh! ++rtc */
      putlog(LOG_MISC, "*", "Uh, banned myself on %s, reversing...",
	     chan->dname);
    reversing = 1;
    check = 0;
  } else if (!match_my_nick(nick)) {	/* It's not my ban */
    if (channel_nouserbans(chan) && nick[0] && !glob_bot(user) &&
	!glob_master(user) && !chan_master(user)) {
      /* No bans made by users */
      add_mode(chan, '-', 'b', who);
      return;
    }
    /* Don't enforce a server ban right away -- give channel users a chance
     * to remove it, in case it's fake
     */
    if (!nick[0]) {
      check = 0;
      if (bounce_modes)
	reversing = 1;
    }
    /* Does this remotely match against any of our hostmasks?
     * just an off-chance...
     */
    u = get_user_by_host(who);
    if (u) {
      get_user_flagrec(u, &victim, chan->dname);
      if (glob_friend(victim) || (glob_op(victim) && !chan_deop(victim)) ||
	  chan_friend(victim) || chan_op(victim)) {
	if (!glob_master(user) && !glob_bot(user) && !chan_master(user))
	  /* commented out: reversing = 1; */ /* arthur2 - 99/05/31 */
	  check = 0;
	if (glob_master(victim) || chan_master(victim))
	  check = 0;
      }
    } else {
      /* Banning an oplisted person who's on the channel? */
      for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
	sprintf(s1, "%s!%s", m->nick, m->userhost);
	if (wild_match(who, s1)) {
	  u = get_user_by_host(s1);
	  if (u) {
	    get_user_flagrec(u, &victim, chan->dname);
	    if (glob_friend(victim) ||
		(glob_op(victim) && !chan_deop(victim))
		|| chan_friend(victim) || chan_op(victim)) {
	      /* Remove ban on +o/f/m user, unless placed by another +m/b */
	      if (!glob_master(user) && !glob_bot(user) &&
		  !chan_master(user)) {
		if (!isexempted(chan, s1)) {
		  /* Crotale - if the victim is not +e, then ban is removed */
		  add_mode(chan, '-', 'b', who);
		  check = 0;
		}
	      }
	      if (glob_master(victim) || chan_master(victim))
		check = 0;
	    }
	  }
	}
      }
    }
  }
  /* If a ban is set on an exempted user then we might as well set exemption
   * at the same time.
   */
  refresh_exempt(chan,who);
  if (check && channel_enforcebans(chan))
    kick_all(chan, who, IRC_BANNED, match_my_nick(nick) ? 0 : 1);
  /* Is it a server ban from nowhere? */
  if (reversing ||
      (bounce_bans && (!nick[0]) &&
       (!u_equals_mask(global_bans, who) ||
	!u_equals_mask(chan->bans, who)) && (check)))
    add_mode(chan, '-', 'b', who);
}

static void got_unban(struct chanset_t *chan, char *nick, char *from,
		      char *who, struct userrec *u)
{
  masklist *b, *old;

  old = NULL;
  for (b = chan->channel.ban; b->mask[0] && rfc_casecmp(b->mask, who);
       old = b, b = b->next)
    ;
  if (b->mask[0]) {
    if (old)
      old->next = b->next;
    else
      chan->channel.ban = b->next;
    nfree(b->mask);
    nfree(b->who);
    nfree(b);
  }
  if (u_sticky_mask(chan->bans, who) || u_sticky_mask(global_bans, who)) {
    /* That's a sticky ban! No point in being
     * sticky unless we enforce it!!
     */
    add_mode(chan, '+', 'b', who);
  }
  if ((u_equals_mask(global_bans, who) || u_equals_mask(chan->bans, who)) &&
      me_op(chan) && !channel_dynamicbans(chan)) {
    /* That's a permban! */
    if ((!glob_bot(user) || !(bot_flags(u) & BOT_SHARE)) &&
        ((!glob_op(user) || chan_deop(user)) && !chan_op(user)))
      add_mode(chan, '+', 'b', who);
  }
}

static void got_exempt(struct chanset_t *chan, char *nick, char *from,
		       char *who)
{
  char s[UHOSTLEN];

  Context;
  simple_sprintf(s, "%s!%s", nick, from);
  newexempt(chan, who, s);
  if (!match_my_nick(nick)) {	/* It's not my exemption */
    if (channel_nouserexempts(chan) && nick[0] && !glob_bot(user) &&
	!glob_master(user) && !chan_master(user)) {
      /* No exempts made by users */
      add_mode(chan, '-', 'e', who);
      return;
    }
    if (!nick[0] && bounce_modes)
      reversing = 1;
  }
  if (reversing || (bounce_exempts && !nick[0] && 
		   (!u_equals_mask(global_exempts, who) ||
		    !u_equals_mask(chan->exempts, who))))
    add_mode(chan, '-', 'e', who);
}

static void got_unexempt(struct chanset_t *chan, char *nick, char *from,
			 char *who, struct userrec *u)
{
  masklist *e, *old;
  masklist *b ;
  int match = 0;

  Context;
  e = chan->channel.exempt;
  old = NULL;
  while (e && e->mask[0] && rfc_casecmp(e->mask, who)) {
    old = e;
    e = e->next;
  }
  if (e && e->mask[0]) {
    if (old)
      old->next = e->next;
    else
      chan->channel.exempt = e->next;
    nfree(e->mask);
    nfree(e->who);
    nfree(e);
  }
  if (u_sticky_mask(chan->exempts, who) || u_sticky_mask(global_exempts, who)) {
    /* That's a sticky exempt! No point in being sticky unless we enforce it!!
     */
    add_mode(chan, '+', 'e', who);
  }
  /* If exempt was removed by master then leave it else check for bans */
  if (!nick[0] && glob_bot(user) && !glob_master(user) && !chan_master(user)) {
    b = chan->channel.ban;
    while (b->mask[0] && !match) {
      if (wild_match(b->mask, who) ||
          wild_match(who, b->mask)) {
        add_mode(chan, '+', 'e', who);
        match = 1;
      } else
        b = b->next;
    }
  }
  if ((u_equals_mask(global_exempts, who) || u_equals_mask(chan->exempts, who)) &&
      me_op(chan) && !channel_dynamicexempts(chan)) {
    /* That's a permexempt! */
    if (glob_bot(user) && (bot_flags(u) & BOT_SHARE)) {
      /* Sharebot -- do nothing */
    } else
      add_mode(chan, '+', 'e', who);
  }
}

static void got_invite(struct chanset_t *chan, char *nick, char *from,
		       char *who)
{
  char s[UHOSTLEN];

  simple_sprintf(s, "%s!%s", nick, from);
  newinvite(chan, who, s);
  if (!match_my_nick(nick)) {	/* It's not my invitation */
    if (channel_nouserinvites(chan) && nick[0] && !glob_bot(user) &&
	!glob_master(user) && !chan_master(user)) {
      /* No exempts made by users */
      add_mode(chan, '-', 'I', who);
      return;
    }
    if ((!nick[0]) && (bounce_modes))
      reversing = 1;
  }
  if (reversing || (bounce_invites && (!nick[0])  && 
		    (!u_equals_mask(global_invites, who) ||
		     !u_equals_mask(chan->invites, who))))
    add_mode(chan, '-', 'I', who);
}

static void got_uninvite(struct chanset_t *chan, char *nick, char *from,
			 char *who, struct userrec *u)
{
  masklist *inv, *old;

  inv = chan->channel.invite;
  old = NULL;
  while (inv->mask[0] && rfc_casecmp(inv->mask, who)) {
    old = inv;
    inv = inv->next;
  }
  if (inv->mask[0]) {
    if (old)
      old->next = inv->next;
    else
      chan->channel.invite = inv->next;
    nfree(inv->mask);
    nfree(inv->who);
    nfree(inv);
  }
  if (u_sticky_mask(chan->invites, who) || u_sticky_mask(global_invites, who)) {
    /* That's a sticky invite! No point in being sticky unless we enforce it!!
     */
    add_mode(chan, '+', 'I', who);
  }
  if (!nick[0] && glob_bot(user) && !glob_master(user) && !chan_master(user)
      &&  (chan->channel.mode & CHANINV))
    add_mode(chan, '+', 'I', who);
  if ((u_equals_mask(global_invites, who) ||
       u_equals_mask(chan->invites, who)) && me_op(chan) &&
      !channel_dynamicinvites(chan)) {
    /* That's a perminvite! */
    if (glob_bot(user) && (bot_flags(u) & BOT_SHARE)) {
      /* Sharebot -- do nothing */
    } else
      add_mode(chan, '+', 'I', who);
  }
}

static int gotmode(char *from, char *origmsg)
{
  char *nick, *ch, *op, *chg, *msg;
  char s[UHOSTLEN], buf[511];
  char ms2[3];
  int z;
  struct userrec *u;
  memberlist *m;
  struct chanset_t *chan;

  strncpy(buf, origmsg, 510);
  buf[510] = 0;
  msg = buf;
  /* Usermode changes? */
  if (msg[0] && (strchr(CHANMETA, msg[0]) != NULL)) {
    ch = newsplit(&msg);
    chg = newsplit(&msg);
    reversing = 0;
    chan = findchan(ch);
    if (!chan) {
      putlog(LOG_MISC, "*", CHAN_FORCEJOIN, ch);
      dprintf(DP_SERVER, "PART %s\n", ch);
    } else if (!channel_pending(chan)) {
      z = strlen(msg);
      if (msg[--z] == ' ')	/* I hate cosmetic bugs :P -poptix */
	msg[z] = 0;
      putlog(LOG_MODES, chan->dname, "%s: mode change '%s %s' by %s",
	     ch, chg, msg, from);
      u = get_user_by_host(from);
      get_user_flagrec(u, &user, ch);
      nick = splitnick(&from);
      m = ismember(chan, nick);
      if (m)
	m->last = now;
      if (!(glob_friend(user) || chan_friend(user) ||
	    (channel_dontkickops(chan) &&
	     (chan_op(user) || (glob_op(user) && !chan_deop(user)))))) {
	if (m && me_op(chan)) {
	  if (chan_fakeop(m)) {
	    putlog(LOG_MODES, ch, CHAN_FAKEMODE, ch);
	    dprintf(DP_MODE, "KICK %s %s :%s\n", ch, nick,
		    CHAN_FAKEMODE_KICK);
	    m->flags |= SENTKICK;
	    reversing = 1;
	  } else if (!chan_hasop(m) && !channel_nodesynch(chan)) {
	    putlog(LOG_MODES, ch, CHAN_DESYNCMODE, ch);
	    dprintf(DP_MODE, "KICK %s %s :%s\n", ch, nick,
		    CHAN_DESYNCMODE_KICK);
	    m->flags |= SENTKICK;
	    reversing = 1;
	  }
	}
      }
      ms2[0] = '+';
      ms2[2] = 0;
      while ((ms2[1] = *chg)) {
	int todo = 0;

	switch (*chg) {
	case '+':
	  ms2[0] = '+';
	  break;
	case '-':
	  ms2[0] = '-';
	  break;
	case 'i':
	  todo = CHANINV;
	  if ((!nick[0]) && (bounce_modes))
	    reversing = 1;
	  break;
	case 'p':
	  todo = CHANPRIV;
	  if ((!nick[0]) && (bounce_modes))
	    reversing = 1;
	  break;
	case 's':
	  todo = CHANSEC;
	  if ((!nick[0]) && (bounce_modes))
	    reversing = 1;
	  break;
	case 'm':
	  todo = CHANMODER;
	  if ((!nick[0]) && (bounce_modes))
	    reversing = 1;
	  break;
	case 'c':
	  todo = CHANNOCLR;
	  if ((!nick[0]) && (bounce_modes))
	    reversing = 1;
	  break;
	case 'R':
	  todo = CHANREGON;
	  if ((!nick[0]) && (bounce_modes))
	    reversing = 1;
	  break;
	case 't':
	  todo = CHANTOPIC;
	  if ((!nick[0]) && (bounce_modes))
	    reversing = 1;
	  break;
	case 'n':
	  todo = CHANNOMSG;
	  if ((!nick[0]) && (bounce_modes))
	    reversing = 1;
	  break;
	case 'a':
	  todo = CHANANON;
	  if ((!nick[0]) && (bounce_modes))
	    reversing = 1;
	  break;
	case 'q':
	  todo = CHANQUIET;
	  if ((!nick[0]) && (bounce_modes))
	    reversing = 1;
	  break;
	case 'l':
	  if ((!nick[0]) && (bounce_modes))
	    reversing = 1;
	  if (ms2[0] == '-') {
	    check_tcl_mode(nick, from, u, chan->dname, ms2, "");
	    if ((reversing) && (chan->channel.maxmembers != (-1))) {
	      simple_sprintf(s, "%d", chan->channel.maxmembers);
	      add_mode(chan, '+', 'l', s);
	    } else if ((chan->limit_prot != (-1)) && !glob_master(user) &&
		       !chan_master(user)) {
	      simple_sprintf(s, "%d", chan->limit_prot);
	      add_mode(chan, '+', 'l', s);
	    }
	    chan->channel.maxmembers = (-1);
	  } else {
	    op = newsplit(&msg);
	    fixcolon(op);
	    if (op == '\0') {
	      break;
	    }
	    chan->channel.maxmembers = atoi(op);
	    check_tcl_mode(nick, from, u, chan->dname, ms2,
			   int_to_base10(chan->channel.maxmembers));
	    if (((reversing) &&
		 !(chan->mode_pls_prot & CHANLIMIT)) ||
		((chan->mode_mns_prot & CHANLIMIT) &&
		 !glob_master(user) && !chan_master(user))) {
	      if (chan->channel.maxmembers == 0)
		add_mode(chan, '+', 'l', "23");		/* wtf? 23 ??? */
	      add_mode(chan, '-', 'l', "");
	    }
	    if ((chan->limit_prot != chan->channel.maxmembers) &&
		(chan->mode_pls_prot & CHANLIMIT) &&
		(chan->limit_prot != (-1)) &&	/* arthur2 */
		!glob_master(user) && !chan_master(user)) {
	      simple_sprintf(s, "%d", chan->limit_prot);
	      add_mode(chan, '+', 'l', s);
	    }
	  }
	  break;
	case 'k':
 	  if (ms2[0] == '+')
	    chan->channel.mode |= CHANKEY;
	  else
            chan->channel.mode &= ~CHANKEY;
          op = newsplit(&msg);
	  fixcolon(op);
	  if (op == '\0') {
	    break;
	  }
	  check_tcl_mode(nick, from, u, chan->dname, ms2, op);
	  if (ms2[0] == '+')
	    got_key(chan, nick, from, op);
	  else {
	    if ((reversing) && (chan->channel.key[0]))
	      add_mode(chan, '+', 'k', chan->channel.key);
	    else if ((chan->key_prot[0]) && !glob_master(user)
		     && !chan_master(user))
	      add_mode(chan, '+', 'k', chan->key_prot);
	    set_key(chan, NULL);
	  }
	  break;
	case 'o':
	  op = newsplit(&msg);
	  fixcolon(op);
	  if (ms2[0] == '+')
	    got_op(chan, nick, from, op, u, &user);
	  else
	    got_deop(chan, nick, from, op, u);
	  break;
	case 'v':
	  op = newsplit(&msg);
	  fixcolon(op);
	  m = ismember(chan, op);
	  if (!m) {
	    putlog(LOG_MISC, chan->dname,
		   CHAN_BADCHANMODE, CHAN_BADCHANMODE_ARGS2);
	    dprintf(DP_MODE, "WHO %s\n", op);
	  } else {
	    get_user_flagrec(m->user, &victim, chan->dname);
	    if (ms2[0] == '+') {
	      m->flags &= ~SENTVOICE;
	      m->flags |= CHANVOICE;
	      check_tcl_mode(nick, from, u, chan->dname, ms2, op);
	      if (!glob_master(user) && !chan_master(user)) {
		if (channel_autovoice(chan) &&
		    (chan_quiet(victim) ||
		     (glob_quiet(victim) && !chan_voice(victim)))) {
		  add_mode(chan, '-', 'v', op);
		} else if (reversing)
		  add_mode(chan, '-', 'v', op);
	      }
	    } else {
	      m->flags &= ~SENTDEVOICE;
	      m->flags &= ~CHANVOICE;
	      check_tcl_mode(nick, from, u, chan->dname, ms2, op);
	      if (!glob_master(user) && !chan_master(user)) {
		if ((channel_autovoice(chan) && !chan_quiet(victim) &&
		    (chan_voice(victim) || glob_voice(victim))) ||
		    (!chan_quiet(victim) && 
		    (glob_gvoice(victim) || chan_gvoice(victim)))) {
		  add_mode(chan, '+', 'v', op);
		} else if (reversing)
		  add_mode(chan, '+', 'v', op);
	      }
	    }
	  }
	  break;
	case 'b':
	  op = newsplit(&msg);
	  fixcolon(op);
	  check_tcl_mode(nick, from, u, chan->dname, ms2, op);
	  if (ms2[0] == '+')
	    got_ban(chan, nick, from, op);
	  else
	    got_unban(chan, nick, from, op, u);
	  break;
	case 'e':
	  op = newsplit(&msg);
	  fixcolon(op);
	  check_tcl_mode(nick, from, u, chan->dname, ms2, op);
	  if (ms2[0] == '+')
	    got_exempt(chan, nick, from, op);
	  else
	    got_unexempt(chan, nick, from, op, u);
	  break;
	case 'I':
	  op = newsplit(&msg);
	  fixcolon(op);
	  check_tcl_mode(nick, from, u, chan->dname, ms2, op);
	  if (ms2[0] == '+')
	    got_invite(chan, nick, from, op);
	  else
	    got_uninvite(chan, nick, from, op, u);
	  break;
	}
	if (todo) {
	  check_tcl_mode(nick, from, u, chan->dname, ms2, "");
	  if (ms2[0] == '+')
	    chan->channel.mode |= todo;
	  else
	    chan->channel.mode &= ~todo;
	  if ((((ms2[0] == '+') && (chan->mode_mns_prot & todo)) ||
	       ((ms2[0] == '-') && (chan->mode_pls_prot & todo))) &&
	      !glob_master(user) && !chan_master(user))
	    add_mode(chan, ms2[0] == '+' ? '-' : '+', *chg, "");
	  else if (reversing &&
		   ((ms2[0] == '+') || (chan->mode_pls_prot & todo)) &&
		   ((ms2[0] == '-') || (chan->mode_mns_prot & todo)))
	    add_mode(chan, ms2[0] == '+' ? '-' : '+', *chg, "");
	}
	chg++;
      }
      if (!me_op(chan) && !nick[0])
        chan->status |= CHAN_ASKEDMODES;
    }
  }
  return 0;
}
