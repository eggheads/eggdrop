/* 
 * mode.c -- handles:
 * queueing and flushing mode changes made by the bot
 * channel mode changes and the bot's reaction to them
 * setting and getting the current wanted channel modes
 * dprintf'ized, 12dec1995
 * multi-channel, 6feb1996
 * stopped the bot deopping masters and bots in bitch mode, pteron 23Mar1997
 */
/* 
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

/* reversing this mode? */
static int reversing = 0;

#define PLUS    1
#define MINUS   2
#define CHOP    4
#define BAN     8
#define VOICE   16
#define EXEMPT  32
#define INVITE  64

static struct flag_record user =
{FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

static struct flag_record victim =
{FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

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
  if (chan->key[0]) {
    if (!ok) {
      *p++ = '+';
      ok = 1;
    }
    *p++ = 'k';
    strcat(post, chan->key);
    strcat(post, " ");
  }
  if (chan->limit != (-1)) {
    if (!ok) {
      *p++ = '+';
      ok = 1;
    }
    *p++ = 'l';
    sprintf(&post[strlen(post)], "%d ", chan->limit);
  }
  chan->limit = (-1);
  chan->key[0] = 0;
  /* do -b before +b to avoid server ban overlap ignores */
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
  if (chan->rmkey[0]) {
    if (!ok) {
      *p++ = '-';
      ok = 1;
    }
    *p++ = 'k';
    strcat(post, chan->rmkey);
    strcat(post, " ");
  }
  chan->rmkey[0] = 0;
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

/* queue a channel mode change */
static void real_add_mode(struct chanset_t *chan,
			  char plus, char mode, char *op)
{
  int i, type, ok, l;
  int bans, exempts, invites, modes;
  masklist *m;
  char s[21];
  
  context;

  if (!me_op(chan))
    return;			/* no point in queueing the mode */
  if (chan->compat == 0) {
     if (mode == 'e' || mode == 'I') chan->compat = 2;
     else chan->compat = 1;
	} else if (mode == 'e' || mode == 'I') {
  	 if (prevent_mixing && chan->compat == 1)
           flush_mode(chan, NORMAL);
    } else if (prevent_mixing && chan->compat == 2)
           flush_mode(chan, NORMAL);
  if ((mode == 'o') || (mode == 'b') || (mode == 'v') ||
      (mode == 'e') || (mode == 'I')) {
    type = (plus == '+' ? PLUS : MINUS) |
      (mode == 'o' ? CHOP : (mode == 'b' ? BAN : (mode == 'v' ? VOICE : (mode == 'e' ? EXEMPT : INVITE))));
    /* if -b'n a non-existant ban...nuke it */
    if ((plus == '-') && (mode == 'b'))
      if (!isbanned(chan, op))
	return;
    /* if there are already max_bans bans on the channel, don't try to add 
     * one more */
    context;
    bans = 0;
    for (m = chan->channel.ban; m && m->mask[0]; m = m->next)
      bans++;
    context;
    if ((plus == '+') && (mode == 'b'))
      if (bans >= max_bans)
	return;
    /* if there are already max_exempts exemptions on the channel, don't
     * try to add one more */
    context;
    exempts = 0;
    for (m = chan->channel.exempt; m && m->mask[0]; m = m->next)
      exempts++;
    context;
    if ((plus == '+') && (mode == 'e'))
      if (exempts >= max_exempts)
	return;
    /* if there are already max_invites invitations on the channel, don't
     * try to add one more */
    context;
    invites = 0;
    for (m = chan->channel.invite; m && m->mask[0]; m = m->next)
      invites++;
    context;
    if ((plus == '+') && (mode == 'I'))
      if (invites >= max_invites)
	return;
    /* if there are already max_modes +b/+e/+I modes on the channel, don't 
     * try to add one more */
    modes = bans + exempts + invites;
    if ((modes >= max_modes) &&
	((mode == 'b') || (mode == 'e') || (mode == 'I')))
      return;
    /* op-type mode change */
    for (i = 0; i < modesperline; i++)
      if ((chan->cmode[i].type == type) && (chan->cmode[i].op != NULL) &&
	  (!rfc_casecmp(chan->cmode[i].op, op)))
	return;			/* already in there :- duplicate */
    ok = 0;			/* add mode to buffer */
    l = strlen(op) + 1;
    if ((chan->bytes + l) > mode_buf_len)
      flush_mode(chan, NORMAL);
    for (i = 0; i < modesperline; i++)
      if ((chan->cmode[i].type == 0) && (!ok)) {
	chan->cmode[i].type = type;
	chan->cmode[i].op = (char *) channel_malloc(l);
	chan->bytes += l;	/* add 1 for safety */
	strcpy(chan->cmode[i].op, op);
	ok = 1;
      }
    ok = 0;			/* check for full buffer */
    for (i = 0; i < modesperline; i++)
      if (chan->cmode[i].type == 0)
	ok = 1;
    if (!ok)
      flush_mode(chan, NORMAL);	/* full buffer!  flush modes */
    if ((mode == 'b') && (plus == '+') && channel_enforcebans(chan))
      enforce_bans(chan);
/* recheck_channel(chan,0); */
    return;
  }
  /* +k ? store key */
  if ((plus == '+') && (mode == 'k'))
    strcpy(chan->key, op);
  /* -k ? store removed key */
  else if ((plus == '-') && (mode == 'k'))
    strcpy(chan->rmkey, op);
  /* +l ? store limit */
  else if ((plus == '+') && (mode == 'l'))
    chan->limit = atoi(op);
  else {
    /* typical mode changes */
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
}

/**********************************************************************/
/* horrible code to parse mode changes */
/* no, it's not horrible, it just looks that way */

static void got_key(struct chanset_t *chan, char *nick, char *from,
		    char *key)
{
  int bogus = 0, i;
  memberlist *m;

  if ((!nick[0]) && (bounce_modes))
    reversing = 1;
  set_key(chan, key);
  for (i = 0; i < strlen(key); i++)
    if (((key[i] < 32) || (key[i] == 127)) &&
	(key[i] != 2) && (key[i] != 31) && (key[i] != 22))
      bogus = 1;
  if (bogus && !match_my_nick(nick)) {
    putlog(LOG_MODES, chan->name, "%s on %s!", CHAN_BADCHANKEY, chan->name);
    dprintf(DP_MODE, "KICK %s %s :%s\n", chan->name, nick, CHAN_BADCHANKEY);
    m = ismember(chan, nick);
    m->flags |= SENTKICK;
  }
  if (((reversing) && !(chan->key_prot[0])) || (bogus) ||
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
		   char *who, struct flag_record *opper)
{
  memberlist *m;
  char s[UHOSTLEN];
  struct userrec *u;
  int check_chan = 0;

  m = ismember(chan, who);
  if (!m) {
    putlog(LOG_MISC, chan->name, CHAN_BADCHANMODE, CHAN_BADCHANMODE_ARGS);
    dprintf(DP_MODE, "WHO %s\n", who);
    return;
  }
  if (!m->user) {
    simple_sprintf(s, "%s!%s", m->nick, m->userhost);
    u = get_user_by_host(s);
  } else
    u = m->user;
  get_user_flagrec(u, &victim, chan->name);
  /* Did *I* just get opped? */
  if (!me_op(chan) && match_my_nick(who))
    check_chan = 1;
  /* I'm opped, and the opper isn't me */
  else if (me_op(chan) && !match_my_nick(who) &&
    /* and deop hasn't been sent for this one and it isn't a server op */
	   !chan_sentdeop(m) && nick[0]) {
    /* Channis is +bitch, and the opper isn't a global master or a bot */
    if (channel_bitch(chan) && !(glob_master(*opper) || glob_bot(*opper)) &&
    /* and the *opper isn't a channel master */
	!chan_master(*opper) &&
    /* and the oppee isn't global op/master/bot */
	!(glob_op(victim) || glob_bot(victim)) &&
    /* and the oppee isn't a channel op/master */
	!chan_op(victim)) {
      add_mode(chan, '-', 'o', who);
      m->flags |= SENTDEOP;
      /* opped is channel +d or global +d */
    } else if ((chan_deop(victim) ||
		(glob_deop(victim) && !chan_op(victim))) &&
	       !glob_master(*opper) && !chan_master(*opper)) {
      add_mode(chan, '-', 'o', who);
      m->flags |= SENTDEOP;
    } else if (reversing) {
      add_mode(chan, '-', 'o', who);
      m->flags |= SENTDEOP;
    }
  } else if (reversing && !chan_sentdeop(m) && !match_my_nick(who)) {
    add_mode(chan, '-', 'o', who);
    m->flags |= SENTDEOP;
  }
  if ((chan_wasoptest(victim) || glob_wasoptest(victim) ||
      chan_autoop(victim) || glob_autoop(victim) ||
      channel_autoop(chan) ||	/* drummer */
      channel_wasoptest(chan)) && !match_my_nick(who)) {
    /* 1.3.21 behavior: wasop test needed for stopnethack */
    if (!nick[0] && !chan_wasop(m) && !chan_sentdeop(m) &&
	me_op(chan) && channel_stopnethack(chan)) {
      add_mode(chan, '-', 'o', who);
      m->flags |= (FAKEOP | SENTDEOP);
    } else {
      m->flags &= ~FAKEOP;
    }
  } else {			/* 1.3.20 behavior: wasop test unwanted
				 * for stopnethack */
    if (!nick[0] &&
	!(chan_op(victim) || (glob_op(victim) && !chan_deop(victim))) &&
	!chan_sentdeop(m) && me_op(chan) &&
	channel_stopnethack(chan)) {
      add_mode(chan, '-', 'o', who);
      m->flags |= (FAKEOP | SENTDEOP);
    } else {
      m->flags &= ~FAKEOP;
    }
  }
  m->flags |= CHANOP;
  m->flags &= ~(SENTOP | WASOP);
  if (check_chan)
    recheck_channel(chan, 1);
}

static void got_deop(struct chanset_t *chan, char *nick, char *from,
		     char *who)
{
  memberlist *m;
  char s[UHOSTLEN], s1[UHOSTLEN];
  struct userrec *u;

  m = ismember(chan, who);
  if (!m) {
    putlog(LOG_MISC, chan->name, CHAN_BADCHANMODE, CHAN_BADCHANMODE_ARGS);
    dprintf(DP_MODE, "WHO %s\n", who);
    return;
  }

  simple_sprintf(s, "%s!%s", m->nick, m->userhost);
  simple_sprintf(s1, "%s!%s", nick, from);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->name);
  /* deop'd someone on my oplist? */
  if (me_op(chan)) {
    int ok = 1;

    if (glob_master(victim) || chan_master(victim))
      ok = 0;
    else if ((glob_op(victim) || glob_friend(victim)) && !chan_deop(victim))
      ok = 0;
    else if (chan_op(victim) || chan_friend(victim))
      ok = 0;
    if (!ok && !match_my_nick(nick) &&
	rfc_casecmp(who, nick) && chan_hasop(m) &&
	!match_my_nick(who)) {	/* added 25mar1996, robey */
      /* reop? */
      /* let's break it down home boy... */
      /* is the deopper NOT a master or bot? */
      if (!glob_master(user) && !chan_master(user) && !glob_bot(user) &&
      /* is the channel protectops? */
	  ((channel_protectops(chan) &&
      /* provided it's not +bitch ... */
	    (!channel_bitch(chan) ||
      /* or the users a valid op */
	     chan_op(victim) || (glob_op(victim) && !chan_deop(victim)))) ||
      /* is the channel protectfriends? */
           (channel_protectfriends(chan) &&
      /* and the users a valid friend */
             (chan_friend(victim) || (glob_friend(victim) && !chan_deop(victim))))) &&
      /* and provided the users not a de-op */
       !(chan_deop(victim) || (glob_deop(victim) && !chan_op(victim))) &&
      /* and we havent sent it already */
	  !chan_sentop(m)) {
	/* then we'll bless them */
	add_mode(chan, '+', 'o', who);
	m->flags |= SENTOP;
      } else if (reversing) {
	add_mode(chan, '+', 'o', who);
	m->flags |= SENTOP;
      }
    }
  }

  if (!nick[0])
    putlog(LOG_MODES, chan->name, "TS resync (%s): %s deopped by %s",
	   chan->name, who, from);
  /* check for mass deop */
  if (nick[0])
    detect_chan_flood(nick, from, s1, chan, FLOOD_DEOP, who);
  /* having op hides your +v status -- so now that someone's lost ops,
   * check to see if they have +v */
  if (!(m->flags & (CHANVOICE | STOPWHO))) {
    dprintf(DP_MODE, "WHO %s\n", m->nick);
    m->flags |= STOPWHO;
  }
  /* was the bot deopped? */
  if (match_my_nick(who)) {
    /* cancel any pending kicks.  Ernst 18/3/1998 */
    memberlist *m2 = chan->channel.member;

    while (m2->nick[0]) {
      if (chan_sentkick(m))
	m2->flags &= ~SENTKICK;
      m2 = m2->next;
    }
    if (chan->need_op[0])
      do_tcl("need-op", chan->need_op);
    if (!nick[0])
      putlog(LOG_MODES, chan->name, "TS resync deopped me on %s :(",
	     chan->name);
  }
  m->flags &= ~(FAKEOP | CHANOP | SENTDEOP | WASOP);

  if (nick[0])
    maybe_revenge(chan, s1, s, REVENGE_DEOP);
}


static void got_ban(struct chanset_t *chan, char *nick, char *from,
		    char *who)
{
  char me[UHOSTLEN], s[UHOSTLEN], s1[UHOSTLEN];
  int check, i, bogus;
  memberlist *m;
  struct userrec *u;

  simple_sprintf(me, "%s!%s", botname, botuserhost);
  simple_sprintf(s, "%s!%s", nick, from);
  newban(chan, who, s);
  bogus = 0;
  check = 1;
  if (!match_my_nick(nick)) {	/* it's not my ban */
    if (channel_nouserbans(chan) && nick[0] && !glob_bot(user) &&
	!glob_master(user) && !chan_master(user)) {
      /* no bans made by users */
      add_mode(chan, '-', 'b', who);
      return;
    }
    for (i = 0; who[i]; i++)
      if (((who[i] < 32) || (who[i] == 127)) &&
	  (who[i] != 2) && (who[i] != 22) && (who[i] != 31))
	bogus = 1;
    if (bogus) {
      if (glob_bot(user) || glob_friend(user) || chan_friend(user) ||
	  (channel_dontkickops(chan) &&
	   (chan_op(user) || (glob_op(user) && !chan_deop(user))))) {	/* arthur2 */
	/* fix their bogus ban */
	if (bounce_bogus_bans) {
	  int ok = 0;

	  strcpy(s1, who);
	  for (i = 0; i < strlen(s1); i++) {
	    if (((s1[i] < 32) || (s1[i] == 127)) &&
		(s1[i] != 2) && (s1[i] != 22) && (s1[i] != 31))
	      s1[i] = '?';
	    if ((s1[i] != '?') && (s1[i] != '*') &&
		(s1[i] != '!') && (s1[i] != '@'))
	      ok = 1;
	  }
	  add_mode(chan, '-', 'b', who);
	  flush_mode(chan, NORMAL);
	  /* only re-add it if it has something besides wildcards */
	  if (ok)
	    add_mode(chan, '+', 'b', s1);
	}
      } else {
	if (bounce_bogus_bans) {
	  add_mode(chan, '-', 'b', who);
	  if (kick_bogus_bans) {
	    m = ismember(chan, nick);
	    if (!m || !chan_sentkick(m)) {
	      if (m)
		m->flags |= SENTKICK;
	      dprintf(DP_MODE, "KICK %s %s :%s\n", chan->name, nick,
		      CHAN_BOGUSBAN);
	    }
	  }
	}
      }
      return;
    }
    /* don't enforce a server ban right away -- give channel users a chance
     * to remove it, in case it's fake */
    if (!nick[0]) {
      check = 0;
      if (bounce_modes)
	reversing = 1;
    }
    /* does this remotely match against any of our hostmasks? */
    /* just an off-chance... */
    u = get_user_by_host(who);
    if (u) {
      get_user_flagrec(u, &victim, chan->name);
      if (glob_friend(victim) || (glob_op(victim) && !chan_deop(victim)) ||
	  chan_friend(victim) || chan_op(victim)) {
	if (!glob_master(user) && !glob_bot(user) && !chan_master(user)) {
	  /* reversing = 1; */ /* arthur2 - 99/05/31 */
	  check = 0;
	}
	if (glob_master(victim) || chan_master(victim))
	  check = 0;
      } else if (wild_match(who, me) && me_op(chan)) {
	/* ^ don't really feel like being banned today, thank you! */
	reversing = 1;
	check = 0;
      }
    } else {
      /* banning an oplisted person who's on the channel? */
      m = chan->channel.member;
      while (m->nick[0]) {
	sprintf(s1, "%s!%s", m->nick, m->userhost);
	if (wild_match(who, s1)) {
	  u = get_user_by_host(s1);
	  if (u) {
	    get_user_flagrec(u, &victim, chan->name);
	    if (glob_friend(victim) ||
		(glob_op(victim) && !chan_deop(victim))
		|| chan_friend(victim) || chan_op(victim)) {
	      /* remove ban on +o/f/m user, unless placed by another +m/b */
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
	m = m->next;
      }
    }
  }
  /* If a ban is set on an exempted user then we might as well set exemption
   * at the same time */
  refresh_exempt(chan,who);
  /* if ((u_equals_exempt(global_exempts,who) || u_equals_exempt(chan->exempts, who))) {
   * add_mode(chan, '+', 'e' , who);
   *} */
  if (check && channel_enforcebans(chan))
    kick_all(chan, who, IRC_BANNED);
  /* is it a server ban from nowhere? */
  if (reversing ||
      (bounce_bans && (!nick[0]) &&
       (!u_equals_mask(global_bans, who) ||
	!u_equals_mask(chan->bans, who)) && (check)))
    add_mode(chan, '-', 'b', who);
}

static void got_unban(struct chanset_t *chan, char *nick, char *from,
		      char *who, struct userrec *u)
{
  int i, bogus;
  masklist *b, *old;

  b = chan->channel.ban;
  old = NULL;
  while (b->mask[0] && rfc_casecmp(b->mask, who)) {
    old = b;
    b = b->next;
  }
  if (b->mask[0]) {
    if (old)
      old->next = b->next;
    else
      chan->channel.ban = b->next;
    nfree(b->mask);
    nfree(b->who);
    nfree(b);
  }
  bogus = 0;
  for (i = 0; i < strlen(who); i++)
    if (((who[i] < 32) || (who[i] == 127)) &&
	(who[i] != 2) && (who[i] != 22) && (who[i] != 31))
      bogus = 1;
  /* it's bogus, not by me, and in fact didn't exist anyway.. */
  if (bogus && !match_my_nick(nick) && !isbanned(chan, who) &&
  /* not by valid +f/+b/+o */
      !(glob_friend(user) || glob_bot(user) ||
	(glob_op(user) && !chan_deop(user))) &&
      !(chan_friend(user) || chan_op(user))) {
    /* then lets kick the weenie */
    memberlist *m = ismember(chan, nick);

    if (!m || !chan_sentkick(m)) {
      if (m)
	m->flags |= SENTKICK;
      dprintf(DP_MODE, "KICK %s %s :%s\n", chan->name, nick, CHAN_BADBAN);
    }
    return;
  }
  if (u_sticky_mask(chan->bans, who) || u_sticky_mask(global_bans, who)) {
    /* that's a sticky ban! No point in being
     * sticky unless we enforce it!! */
    add_mode(chan, '+', 'b', who);
  }
  if ((u_equals_mask(global_bans, who) || u_equals_mask(chan->bans, who)) &&
      me_op(chan) && !channel_dynamicbans(chan)) {
    /* that's a permban! */
    if (glob_bot(user) && (bot_flags(u) & BOT_SHARE)) {
      /* sharebot -- do nothing */
    } else if ((glob_op(user) && !chan_deop(user)) || chan_op(user)) {
      dprintf(DP_HELP, "NOTICE %s :%s %s", nick, who, CHAN_PERMBANNED);
    } else
      add_mode(chan, '+', 'b', who);
  }
}

static void got_exempt(struct chanset_t *chan, char *nick, char *from,
		       char *who)
{
  char me[UHOSTLEN], s[UHOSTLEN], s1[UHOSTLEN];
  int check, i, bogus;
  memberlist *m;

  context;
  simple_sprintf(me, "%s!%s", botname, botuserhost);
  simple_sprintf(s, "%s!%s", nick, from);
  newexempt(chan, who, s);
  bogus = 0;
  check = 1;
  if (!match_my_nick(nick)) {	/* it's not my exemption */
    if (channel_nouserexempts(chan) && nick[0] && !glob_bot(user) &&
	!glob_master(user) && !chan_master(user)) {
      /* no exempts made by users */
      add_mode(chan, '-', 'e', who);
      return;
    }
    for (i = 0; who[i]; i++)
      if (((who[i] < 32) || (who[i] == 127)) &&
	  (who[i] != 2) && (who[i] != 22) && (who[i] != 31))
	bogus = 1;
    if (bogus) {
      if (glob_bot(user) || glob_friend(user) || chan_friend(user) ||
	  (channel_dontkickops(chan) &&
	   (chan_op(user) || (glob_op(user) && !chan_deop(user))))) {	/* arthur2 */
	/* fix their bogus exemption */
	if (bounce_bogus_exempts) {
	  int ok = 0;
	  
	  strcpy(s1, who);
	  for (i = 0; i < strlen(s1); i++) {
	    if (((s1[i] < 32) || (s1[i] == 127)) &&
		(s1[i] != 2) && (s1[i] != 22) && (s1[i] != 31))
	      s1[i] = '?';
	    if ((s1[i] != '?') && (s1[i] != '*') &&
		(s1[i] != '!') && (s1[i] != '@'))
	      ok = 1;
	  }
	  add_mode(chan, '-', 'e', who);
	  flush_mode(chan, NORMAL);
	  /* only re-add it if it has something besides wildcards */
	  if (ok)
	    add_mode(chan, '+', 'e', s1);
	}
      } else {
	if (bounce_bogus_exempts) {
	  add_mode(chan, '-', 'e', who);
	  if (kick_bogus_exempts) {
	    m = ismember(chan, nick);
	    if (!m || !chan_sentkick(m)) {
	      if (m)
		m->flags |= SENTKICK;
	      dprintf(DP_MODE, "KICK %s %s :%s\n", chan->name, nick,
		      CHAN_BOGUSEXEMPT);
	    }
	  }
	}
      }
      return;
    }
    if ((!nick[0]) && (bounce_modes))
      reversing = 1;
  }
  if (reversing || (bounce_exempts && !nick[0] && 
		   (!u_equals_mask(global_exempts, who) || !u_equals_mask(chan->exempts, who))
		    && (check)))
    add_mode(chan, '-', 'e', who);
}

static void got_unexempt(struct chanset_t *chan, char *nick, char *from,
			 char *who, struct userrec *u)
{
  masklist *e, *old;
  masklist *b ;
  int match = 0;

  context;
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
    /* that's a sticky exempt! No point in being
     * sticky unless we enforce it!! */
    add_mode(chan, '+', 'e', who);
  }
  /* if exempt was removed by master then leave it else check for bans */
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
    /* that's a permexempt! */
    if (glob_bot(user) && (bot_flags(u) & BOT_SHARE)) {
      /* sharebot -- do nothing */
    } else
      add_mode(chan, '+', 'e', who);
  }
}

static void got_invite(struct chanset_t *chan, char *nick, char *from,
		       char *who)
{
  char me[UHOSTLEN], s[UHOSTLEN], s1[UHOSTLEN];
  int check, i, bogus;
  memberlist *m;

  simple_sprintf(me, "%s!%s", botname, botuserhost);
  simple_sprintf(s, "%s!%s", nick, from);
  newinvite(chan, who, s);
  bogus = 0;
  check = 1;
  if (!match_my_nick(nick)) {	/* it's not my invitation */
    if (channel_nouserinvites(chan) && nick[0] && !glob_bot(user) &&
	!glob_master(user) && !chan_master(user)) {
      /* no exempts made by users */
      add_mode(chan, '-', 'I', who);
      return;
    }
    for (i = 0; who[i]; i++)
      if (((who[i] < 32) || (who[i] == 127)) &&
	  (who[i] != 2) && (who[i] != 22) && (who[i] != 31))
	bogus = 1;
    if (bogus) {
      if (glob_bot(user) || glob_friend(user) || chan_friend(user) ||
	  (channel_dontkickops(chan) &&
	   (chan_op(user) || (glob_op(user) && !chan_deop(user))))) {	/* arthur2 */
	/* fix their bogus invitation */
	if (bounce_bogus_invites) {
	  int ok = 0;

	  strcpy(s1, who);
	  for (i = 0; i < strlen(s1); i++) {
	    if (((s1[i] < 32) || (s1[i] == 127)) &&
		(s1[i] != 2) && (s1[i] != 22) && (s1[i] != 31))
	      s1[i] = '?';
	    if ((s1[i] != '?') && (s1[i] != '*') &&
		(s1[i] != '!') && (s1[i] != '@'))
	      ok = 1;
	  }
	  add_mode(chan, '-', 'I', who);
	  flush_mode(chan, NORMAL);
	  /* only re-add it if it has something besides wildcards */
	  if (ok)
	    add_mode(chan, '+', 'I', s1);
	}
      } else {
	if (bounce_bogus_invites) {
	  add_mode(chan, '-', 'I', who);
	  if (kick_bogus_invites) {
	    m = ismember(chan, nick);
	    if (!m || !chan_sentkick(m)) {
	      if (m)
		m->flags |= SENTKICK;
	      dprintf(DP_MODE, "KICK %s %s :%s\n", chan->name, nick,
		      CHAN_BOGUSINVITE);
	    }
	  }
	}
      }
      return;
    }
    if ((!nick[0]) && (bounce_modes))
      reversing = 1;
  }
  if (reversing || (bounce_invites && (!nick[0])  && 
		    (!u_equals_mask(global_invites, who) || !u_equals_mask(chan->invites, who))
		    && (check)))
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
    /* that's a sticky invite! No point in being
     * sticky unless we enforce it!! */
    add_mode(chan, '+', 'I', who);
  }
  if (!nick[0] && glob_bot(user) && !glob_master(user) && !chan_master(user)
      &&  (chan->channel.mode & CHANINV))
    add_mode(chan, '+', 'I', who);
  if ((u_equals_mask(global_invites, who) || u_equals_mask(chan->invites, who)) &&
      me_op(chan) && !channel_dynamicinvites(chan)) {
    /* that's a perminvite! */
    if (glob_bot(user) && (bot_flags(u) & BOT_SHARE)) {
      /* sharebot -- do nothing */
    } else
      add_mode(chan, '+', 'I', who);
  }
}

/* a pain in the ass: mode changes */
static void gotmode(char *from, char *msg)
{
  char *nick, *ch, *op, *chg;
  char s[UHOSTLEN];
  char ms2[3];
  int z;
  struct userrec *u;
  memberlist *m;
  struct chanset_t *chan;

  /* usermode changes? */
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
      if (msg[--z] == ' ')	/* i hate cosmetic bugs :P -poptix */
	msg[z] = 0;
      putlog(LOG_MODES, chan->name, "%s: mode change '%s %s' by %s",
	     ch, chg, msg, from);
      u = get_user_by_host(from);
      get_user_flagrec(u, &user, ch);
      nick = splitnick(&from);
      m = ismember(chan, nick);
      if (m)
	m->last = now;
      if ((allow_desync == 0) &&	/* arthur2: added flag tests */
	  !(glob_friend(user) || chan_friend(user) ||
	    (channel_dontkickops(chan) &&
	     (chan_op(user) || (glob_op(user) && !chan_deop(user)))))) {
	if (m && me_op(chan)) {
	  if (chan_fakeop(m)) {
	    putlog(LOG_MODES, ch, CHAN_FAKEMODE, ch);
	    dprintf(DP_MODE, "KICK %s %s :%s\n", ch, nick,
		    CHAN_FAKEMODE_KICK);
	    m->flags |= SENTKICK;
	    reversing = 1;
	  } else if (!chan_hasop(m)) {
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
	    check_tcl_mode(nick, from, u, chan->name, ms2, "");
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
	    check_tcl_mode(nick, from, u, chan->name, ms2,
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
	  op = newsplit(&msg);
	  fixcolon(op);
	  if (op == '\0') {
	    break;
	  }
	  check_tcl_mode(nick, from, u, chan->name, ms2, op);
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
	  check_tcl_mode(nick, from, u, chan->name, ms2, op);
	  if (ms2[0] == '+')
	    got_op(chan, nick, from, op, &user);
	  else
	    got_deop(chan, nick, from, op);
	  break;
	case 'v':
	  op = newsplit(&msg);
	  fixcolon(op);
	  m = ismember(chan, op);
	  if (!m) {
	    putlog(LOG_MISC, chan->name,
		   CHAN_BADCHANMODE, CHAN_BADCHANMODE_ARGS2);
	    dprintf(DP_MODE, "WHO %s\n", op);
	  } else {
	    check_tcl_mode(nick, from, u, chan->name, ms2, op);
	    get_user_flagrec(m->user, &victim, chan->name);
	    if (ms2[0] == '+') {
	      m->flags &= ~SENTVOICE;
	      m->flags |= CHANVOICE;
	      if (!glob_master(user) && !chan_master(user)) {
		if (channel_autovoice(chan) &&
		    (chan_quiet(victim) ||
		     (glob_quiet(victim) && !chan_voice(victim)))) {
		  add_mode(chan, '-', 'v', op);
		  m->flags |= SENTDEVOICE;
		} else if (reversing) {
		  add_mode(chan, '-', 'v', op);
		  m->flags |= SENTDEVOICE;
		   }
	      }
	    } else {
	      m->flags &= ~SENTDEVOICE;
	      m->flags &= ~CHANVOICE;
	      if (!glob_master(user) && !chan_master(user)) {
		if ((channel_autovoice(chan) && !chan_quiet(victim) &&
		    (chan_voice(victim) || glob_voice(victim))) ||
		    (!chan_quiet(victim) && 
		    (glob_gvoice(victim) || chan_gvoice(victim)))) {
		  add_mode(chan, '+', 'v', op);
		  m->flags |= SENTVOICE;
		} else if (reversing) {
		  add_mode(chan, '+', 'v', op);
		  m->flags |= SENTVOICE;
		 }
	      }
	    }
	  }
	  break;
	case 'b':
	  op = newsplit(&msg);
	  fixcolon(op);
	  check_tcl_mode(nick, from, u, chan->name, ms2, op);
	  if (ms2[0] == '+')
	    got_ban(chan, nick, from, op);
	  else
	    got_unban(chan, nick, from, op, u);
	  break;
	case 'e':
	  op = newsplit(&msg);
	  fixcolon(op);
	  check_tcl_mode(nick, from, u, chan->name, ms2, op);
	  if (ms2[0] == '+')
	    got_exempt(chan, nick, from, op);
	  else
	    got_unexempt(chan, nick, from, op, u);
	  break;
	case 'I':
	  op = newsplit(&msg);
	  fixcolon(op);
	  check_tcl_mode(nick, from, u, chan->name, ms2, op);
	  if (ms2[0] == '+')
	    got_invite(chan, nick, from, op);
	  else
	    got_uninvite(chan, nick, from, op, u);
	  break;
	}
	if (todo) {
	  check_tcl_mode(nick, from, u, chan->name, ms2, "");
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
    }
  }
}
