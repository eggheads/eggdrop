/* 
 * userchan.c - part of channels.mod
 *
 */
/* 
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

struct chanuserrec *get_chanrec(struct userrec *u, char *chname)
{
  struct chanuserrec *ch = u->chanrec;

  while (ch != NULL) {
    if (!rfc_casecmp(ch->channel, chname))
      return ch;
    ch = ch->next;
  }
  return NULL;
}

static struct chanuserrec *add_chanrec(struct userrec *u, char *chname)
{
  struct chanuserrec *ch = 0;

  if (findchan(chname)) {
    ch = user_malloc(sizeof(struct chanuserrec));

    ch->next = u->chanrec;
    u->chanrec = ch;
    ch->info = NULL;
    ch->flags = 0;
    ch->flags_udef = 0;
    ch->laston = 0;
    strncpy(ch->channel, chname, 80);
    ch->channel[80] = 0;
    if (!noshare && !(u->flags & USER_UNSHARED))
      shareout(findchan(chname), "+cr %s %s\n", u->handle, chname);
  }
  return ch;
}

static void add_chanrec_by_handle(struct userrec *bu, char *hand, char *chname)
{
  struct userrec *u;

  u = get_user_by_handle(bu, hand);
  if (!u)
    return;
  if (!get_chanrec(u, chname))
    add_chanrec(u, chname);
}

static void get_handle_chaninfo(char *handle, char *chname, char *s)
{
  struct userrec *u;
  struct chanuserrec *ch;

  u = get_user_by_handle(userlist, handle);
  if (u == NULL) {
    s[0] = 0;
    return;
  }
  ch = get_chanrec(u, chname);
  if (ch == NULL) {
    s[0] = 0;
    return;
  }
  if (ch->info == NULL) {
    s[0] = 0;
    return;
  }
  strcpy(s, ch->info);
  return;
}

static void set_handle_chaninfo(struct userrec *bu, char *handle,
				char *chname, char *info)
{
  struct userrec *u;
  struct chanuserrec *ch;
  char *p;
  struct chanset_t *cst;

  u = get_user_by_handle(bu, handle);
  if (!u)
    return;
  ch = get_chanrec(u, chname);
  if (!ch) {
    add_chanrec_by_handle(bu, handle, chname);
    ch = get_chanrec(u, chname);
  }
  if (info) {
    if (strlen(info) > 80)
      info[80] = 0;
    for (p = info; *p;) {
      if ((*p < 32) || (*p == 127))
	strcpy(p, p + 1);
      else
	p++;
    }
  }
  if (ch->info != NULL)
    nfree(ch->info);
  if (info && info[0]) {
    ch->info = (char *) nmalloc(strlen(info) + 1);
    strcpy(ch->info, info);
  } else
    ch->info = NULL;
  cst = findchan(chname);
  if ((!noshare) && (bu == userlist) &&
      !(u->flags & (USER_UNSHARED | USER_BOT)) && share_greet) {
    shareout(cst, "chchinfo %s %s %s\n", handle, chname, info ? info : "");
  }
}

static void del_chanrec(struct userrec *u, char *chname)
{
  struct chanuserrec *ch, *lst;

  lst = NULL;
  ch = u->chanrec;
  while (ch) {
    if (!rfc_casecmp(chname, ch->channel)) {
      if (lst == NULL)
	u->chanrec = ch->next;
      else
	lst->next = ch->next;
      if (ch->info != NULL)
	nfree(ch->info);
      nfree(ch);
      if (!noshare && !(u->flags & USER_UNSHARED))
	shareout(findchan(chname), "-cr %s %s\n", u->handle, chname);
      return;
    }
    lst = ch;
    ch = ch->next;
  }
}

static void set_handle_laston(char *chan, struct userrec *u, time_t n)
{
  struct chanuserrec *ch;

  if (!u)
    return;
  touch_laston(u, chan, n);
  ch = get_chanrec(u, chan);
  if (!ch)
    return;
  ch->laston = n;
}

/* is this ban sticky? */
static int u_sticky_ban(struct banrec *u, char *uhost)
{
  for (; u; u = u->next)
    if (!rfc_casecmp(u->banmask, uhost))
      return (u->flags & BANREC_STICKY);
  return 0;
}

/* is this exempt sticky? */
static int u_sticky_exempt (struct exemptrec * u, char * uhost)
{
  for (;u;u=u->next) 
    if (!rfc_casecmp(u->exemptmask,uhost))
      return (u->flags & EXEMPTREC_STICKY);
  return 0;
}

/* is this invite sticky? */
static int u_sticky_invite (struct inviterec * u, char * uhost)
{
  for (;u;u=u->next) 
    if (!rfc_casecmp(u->invitemask,uhost))
      return (u->flags & INVITEREC_STICKY);
  return 0;
}

/* set sticky attribute for a ban */
static int u_setsticky_ban(struct chanset_t *chan, char *uhost, int sticky)
{
  int j;
  struct banrec *u = chan ? chan->bans : global_bans;

  j = atoi(uhost);
  if (!j)
    j = (-1);
  for (; u; u = u->next) {
    if (j >= 0)
      j--;
    if (!j || ((j < 0) && !rfc_casecmp(u->banmask, uhost))) {
      if (sticky > 0)
	u->flags |= BANREC_STICKY;
      else if (!sticky)
	u->flags &= ~BANREC_STICKY;
      else			/* we don't actually want to change,
				 * just skip over */
	return 0;
      if (!j)
	strcpy(uhost, u->banmask);
      if (!noshare) {
	shareout(chan, "s %s %d %s\n", uhost, sticky,
		 chan ? chan->name : "");
      }
      return 1;
    }
  }
  if (j >= 0)
    return -j;
  else
    return 0;
}

/* set sticky attribute for an exempt */
static int u_setsticky_exempt (struct chanset_t * chan, char * uhost, int sticky)
{
  int j;
  struct exemptrec * u = chan ? chan->exempts : global_exempts;
  
  j = atoi(uhost);
  if (!j)
    j = (-1);
  for (;u;u=u->next) {
    if (j >= 0)
      j--;
    if (!j || ((j < 0) && !rfc_casecmp(u->exemptmask,uhost))) {
      if (sticky > 0)
	u->flags |= EXEMPTREC_STICKY;
      else if (!sticky)
	u->flags &= ~EXEMPTREC_STICKY;
      else /* we don't actually want to change, just skip over */
	return 0;
      if (!j)
	strcpy(uhost,u->exemptmask);
      if (!noshare) {
	shareout(chan,"se %s %d %s\n", uhost, sticky,
		 chan?chan->name:"");
      }
      return 1;
    }
    
  }
  if (j >= 0)
    return -j;
  else
    return 0;
}

/* set sticky attribute for a invite */
static int u_setsticky_invite (struct chanset_t * chan, char * uhost, int sticky)
{
  int j;
  struct inviterec * u = chan ? chan->invites : global_invites;
  
  j = atoi(uhost);
  if (!j)
    j = (-1);
  for (;u;u=u->next) {
    if (j >= 0)
      j--;
    if (!j || ((j < 0) && !rfc_casecmp(u->invitemask,uhost))) {
      if (sticky > 0)
	u->flags |= INVITEREC_STICKY;
      else if (!sticky)
	u->flags &= ~INVITEREC_STICKY;
      else /* we don't actually want to change, just skip over */
	return 0;
      if (!j)
	strcpy(uhost,u->invitemask);
      if (!noshare) {
	shareout(chan,"sInv %s %d %s\n", uhost, sticky,
		 chan?chan->name:"");
      }
      return 1;
    }
  }
  if (j >= 0)
    return -j;
  else
    return 0;
}

/* returns 1 if temporary ban, 2 if permban, 0 if not a ban at all */
static int u_equals_ban(struct banrec *u, char *uhost)
{
  for (; u; u = u->next)
    if (!rfc_casecmp(u->banmask, uhost)) {
      if (u->flags & BANREC_PERM)
	return 2;
      else
	return 1;
    }
  return 0;			/* not equal */
}

/* returns 1 if temporary exempt, 2 if permexempt, 0 if not a exempt at all */
static int u_equals_exempt (struct exemptrec * u, char * uhost)
{
  for (;u;u=u->next)
    if (!rfc_casecmp(u->exemptmask, uhost)) {
      if (u->flags &EXEMPTREC_PERM)
	return 2;
      else
	return 1;
    }
  return 0;			/* not equal */
}

/* returns 1 if temporary invite, 2 if perminvite, 0 if not a invite at all */
static int u_equals_invite (struct inviterec * u, char * uhost)
{
  for (;u;u=u->next)
    if (!rfc_casecmp(u->invitemask, uhost)) {
      if (u->flags & INVITEREC_PERM)
	return 2;
      else
	return 1;
    }
  return 0;			/* not equal */
}

static int u_match_exempt (struct exemptrec * u, char * uhost)
{
  for (;u;u=u->next)
    if (wild_match(u->exemptmask,uhost))
      return 1;
  return 0;
}
 
static int u_match_invite (struct inviterec * u, char * uhost)
{
  for (;u;u=u->next)
    if (wild_match(u->invitemask,uhost))
      return 1;
  return 0;
}

static int u_match_ban(struct banrec *u, char *uhost)
{
  for (; u; u = u->next)
    if (wild_match(u->banmask, uhost))
      return 1;
  return 0;
}

static int u_delban(struct chanset_t *c, char *who, int doit)
{
  int j, i = 0;
  struct banrec *t;
  struct banrec **u = c ? &(c->bans) : &global_bans;

  if (!strchr(who, '!') && (j = atoi(who))) {
    j--;
    for (; (*u) && j; u = &((*u)->next), j--);
    if (*u) {
      strcpy(who, (*u)->banmask);
      i = 1;
    } else
      return -j - 1;
  } else {
    /* find matching host, if there is one */
    for (; *u && !i; u = &((*u)->next))
      if (!rfc_casecmp((*u)->banmask, who)) {
	i = 1;
	break;
      }
    if (!*u)
      return 0;
  }
  if (i && doit) {
    if (!noshare) {
      /* distribute chan bans differently */
      if (c)
	shareout(c, "-bc %s %s\n", c->name, who);
      else
	shareout(NULL, "-b %s\n", who);
    }
    if (!c)
      gban_total--;
    nfree((*u)->banmask);
    if ((*u)->desc)
      nfree((*u)->desc);
    if ((*u)->user)
      nfree((*u)->user);
    t = *u;
    *u = (*u)->next;
    nfree(t);
  }
  return i;
}

static int u_delexempt (struct chanset_t * c, char * who, int doit)
{
  int j, i = 0;
  struct exemptrec * t;
  struct exemptrec ** u = c ? &(c->exempts) : &global_exempts;
  
  if (!strchr(who,'!') && (j = atoi(who))) {
    j--;
    for (;(*u) && j;u=&((*u)->next),j--);
    if (*u) {
      strcpy(who, (*u)->exemptmask);
      i = 1;
    } else
      return -j-1;
  } else {
    /* find matching host, if there is one */
    for (;*u && !i;u=&((*u)->next)) 
      if (!rfc_casecmp((*u)->exemptmask,who)) {
	i = 1;
	break;
      }
    if (!*u)
      return 0;
  }
  if (i && doit) {
    if (!noshare) {
      /* distribute chan exempts differently */
      if (c)
	shareout(c,"-ec %s %s\n", c->name, who);
      else 
	shareout(NULL,"-e %s\n", who);
    }
    if (!c)
      gexempt_total --;
    nfree((*u)->exemptmask);
    if ((*u)->desc)
      nfree((*u)->desc);
    if ((*u)->user)
      nfree((*u)->user);
    t = *u;
    *u = (*u)->next;
    nfree(t);
  }
  return i;
}

static int u_delinvite (struct chanset_t * c, char * who, int doit)
{
  int j, i = 0;
  struct inviterec * t;
  struct inviterec ** u = c ? &(c->invites) : &global_invites;
  
  if (!strchr(who,'!') && (j = atoi(who))) {
    j--;
    for (;(*u) && j;u=&((*u)->next),j--);
    if (*u) {
      strcpy(who, (*u)->invitemask);
      i = 1;
    } else
      return -j-1;
  } else {
    /* find matching host, if there is one */
    for (;*u && !i;u=&((*u)->next))
      if (!rfc_casecmp((*u)->invitemask,who)) {
	i = 1;
	break;
      }
    if (!*u)
      return 0;
  }
  if (i && doit) {
    if (!noshare) {
      /* distribute chan invites differently */
      if (c)
	shareout(c,"-invc %s %s\n", c->name, who);
      else 
	shareout(NULL,"-inv %s\n", who);
    }
    if (!c)
      ginvite_total --;
    nfree((*u)->invitemask);
    if ((*u)->desc)
      nfree((*u)->desc);
    if ((*u)->user)
      nfree((*u)->user);
    t = *u;
    *u = (*u)->next;
    nfree(t);
  }
  return i;
}

/* new method of creating bans */
/* if first char of note is '*' it's a sticky ban */
static int u_addban(struct chanset_t *chan, char *ban, char *from, char *note,
		    time_t expire_time, int flags)
{
  struct banrec *p;
  struct banrec **u = chan ? &chan->bans : &global_bans;
  char host[1024], s[1024];
  module_entry *me;

  strcpy(host, ban);
  /* choke check: fix broken bans (must have '!' and '@') */
  if ((strchr(host, '!') == NULL) && (strchr(host, '@') == NULL))
    strcat(host, "!*@*");
  else if (strchr(host, '@') == NULL)
    strcat(host, "@*");
  else if (strchr(host, '!') == NULL) {
    char *i = strchr(host, '@');

    strcpy(s, i);
    *i = 0;
    strcat(host, "!*");
    strcat(host, s);
  }
  if ((me = module_find("server", 0, 0)) && me->funcs)
    simple_sprintf(s, "%s!%s", me->funcs[4], me->funcs[5]);
  else
    simple_sprintf(s, "%s!%s@%s", origbotname, botuser, hostname);
  if (wild_match(host, s)) {
    putlog(LOG_MISC, "*", IRC_IBANNEDME);
    return 0;
  }
  if (u_equals_ban(*u, host))
    u_delban(chan, host, 1);	/* remove old ban */
  /* it shouldn't expire and be sticky also */
  if (note[0] == '*') {
    flags |= BANREC_STICKY;
    note++;
  }
  if ((expire_time == 0L) || (flags & BANREC_PERM)) {
    flags |= BANREC_PERM;
    expire_time = 0L;
  }
  /* new format: */
  p = user_malloc(sizeof(struct banrec));

  p->next = *u;
  *u = p;
  p->expire = expire_time;
  p->added = now;
  p->lastactive = 0;
  p->flags = flags;
  p->banmask = user_malloc(strlen(host) + 1);
  strcpy(p->banmask, host);
  p->user = user_malloc(strlen(from) + 1);
  strcpy(p->user, from);
  p->desc = user_malloc(strlen(note) + 1);
  strcpy(p->desc, note);
  if (!noshare) {
    if (!chan)
      shareout(NULL, "+b %s %lu %s%s %s %s\n", host, expire_time - now,
	       (flags & BANREC_STICKY) ? "s" : "",
	       (flags & BANREC_PERM) ? "p" : "-", from, note);
    else
      shareout(chan, "+bc %s %lu %s %s%s %s %s\n", host, expire_time - now,
	       chan->name, (flags & BANREC_STICKY) ? "s" : "",
	       (flags & BANREC_PERM) ? "p" : "-", from, note);
  }
  return 1;
}

/* new method of creating invites - Hell invites are new!! - Jason */
/* if first char of note is '*' it's a sticky invite */
static int u_addinvite (struct chanset_t * chan, char * invite, char * from,
			char * note, time_t expire_time, int flags)
{
  struct inviterec * p;
  struct inviterec ** u = chan ? &chan->invites : &global_invites;
  char host[1024], s[1024];
  module_entry * me;
  
  strcpy(host, invite);
  /* choke check: fix broken invites (must have '!' and '@') */
  if ((strchr(host, '!') == NULL) && (strchr(host, '@') == NULL))
    strcat(host, "!*@*");
  else if (strchr(host, '@') == NULL)
    strcat(host, "@*");
  else if (strchr(host, '!') == NULL) {
    char * i = strchr(host, '@');
    strcpy(s, i);
    *i = 0;
    strcat(host, "!*");
    strcat(host, s);
  }
  if ((me = module_find("server",0,0)) && me->funcs)
    simple_sprintf(s, "%s!%s", me->funcs[4], me->funcs[5]);
  else
    simple_sprintf(s, "%s!%s@%s", origbotname, botuser, hostname);
  
  if (u_equals_invite(*u, host))
    u_delinvite(chan, host,1);	/* remove old invite */
  /* it shouldn't expire and be sticky also */
  if (note[0] == '*') {
    flags |= INVITEREC_STICKY;
    note++;
  }
  if (expire_time != 0L)
    flags &= ~INVITEREC_STICKY;
  else
    flags |= INVITEREC_PERM;
  /* new format: */
  p = user_malloc(sizeof(struct inviterec));
  p->next = *u;
  *u = p;
  p->expire = expire_time;
  p->added = now;
  p->lastactive = 0;
  p->flags = flags;
  p->invitemask = user_malloc(strlen(host)+1);
  strcpy(p->invitemask,host);
  p->user = user_malloc(strlen(from)+1);
  strcpy(p->user,from);
  p->desc = user_malloc(strlen(note)+1);
  strcpy(p->desc,note);
  if (!noshare) {
    if (!chan)
      shareout(NULL,"+inv %s %lu %s%s %s %s\n", host, expire_time - now,
	       (flags & INVITEREC_STICKY) ? "s" : "",
	       (flags & INVITEREC_PERM) ? "p": "-", from, note);
    else 
      shareout(chan,"+invc %s %lu %s %s%s %s %s\n", host, expire_time - now,
	       chan->name, (flags & INVITEREC_STICKY) ? "s" : "",
	       (flags & INVITEREC_PERM) ? "p": "-", from, note);
  }
  return 1;
}

/* new method of creating exempts- new... yeah whole bleeding lot is new! */
/* if first char of note is '*' it's a sticky exempt */
static int u_addexempt (struct chanset_t * chan, char * exempt, char * from,
			char * note, time_t expire_time, int flags)
{
  struct exemptrec * p;
  struct exemptrec ** u = chan ? &chan->exempts : &global_exempts;
  char host[1024], s[1024];
  module_entry * me;
  
  strcpy(host, exempt);
  /* choke check: fix broken exempts (must have '!' and '@') */
  if ((strchr(host, '!') == NULL) && (strchr(host, '@') == NULL))
    strcat(host, "!*@*");
  else if (strchr(host, '@') == NULL)
    strcat(host, "@*");
  else if (strchr(host, '!') == NULL) {
    char * i = strchr(host, '@');
    strcpy(s, i);
    *i = 0;
    strcat(host, "!*");
    strcat(host, s);
  }
  if ((me = module_find("server",0,0)) && me->funcs)
    simple_sprintf(s, "%s!%s", me->funcs[4], me->funcs[5]);
  else
    simple_sprintf(s, "%s!%s@%s", origbotname, botuser, hostname);
  
  if (u_equals_exempt(*u, host))
    u_delexempt(chan, host,1);	/* remove old exempt */
  /* it shouldn't expire and be sticky also */
  if (note[0] == '*') {
    flags |= EXEMPTREC_STICKY;
    note++;
  }
  if (expire_time != 0L)
    flags &= ~EXEMPTREC_STICKY;
  else
    flags |= EXEMPTREC_PERM;
  /* new format: */
  p = user_malloc(sizeof(struct exemptrec));
  p->next = *u;
  *u = p;
  p->expire = expire_time;
  p->added = now;
  p->lastactive = 0;
  p->flags = flags;
  p->exemptmask = user_malloc(strlen(host)+1);
  strcpy(p->exemptmask,host);
  p->user = user_malloc(strlen(from)+1);
  strcpy(p->user,from);
  p->desc = user_malloc(strlen(note)+1);
  strcpy(p->desc,note);
  if (!noshare) {
    if (!chan)
      shareout(NULL,"+e %s %lu %s%s %s %s\n", host, expire_time - now,
	       (flags & EXEMPTREC_STICKY) ? "s" : "",
	       (flags & EXEMPTREC_PERM) ? "p": "-", from, note);
    else 
      shareout(chan,"+ec %s %lu %s %s%s %s %s\n", host, expire_time - now,
	       chan->name, (flags & EXEMPTREC_STICKY) ? "s" : "",
	       (flags & EXEMPTREC_PERM) ? "p": "-", from, note);
  }
  return 1;
}


/* take host entry from ban list and display it ban-style */
static void display_ban(int idx, int number, struct banrec *ban,
			struct chanset_t *chan, int show_inact)
{
  char dates[81], s[41];

  if (ban->added) {
    daysago(now, ban->added, s);
    sprintf(dates, "%s %s", BANS_CREATED, s);
    if (ban->added < ban->lastactive) {
      strcat(dates, ", ");
      strcat(dates, BANS_LASTUSED);
      strcat(dates, " ");
      daysago(now, ban->lastactive, s);
      strcat(dates, s);
    }
  } else
    dates[0] = 0;
  if (ban->flags & BANREC_PERM)
    strcpy(s, "(perm)");
  else {
    char s1[41];

    days(ban->expire, now, s1);
    sprintf(s, "(expires %s)", s1);
  }
  if (ban->flags & BANREC_STICKY)
    strcat(s, " (sticky)");
  if (!chan || isbanned(chan, ban->banmask)) {
    if (number >= 0) {
      dprintf(idx, "  [%3d] %s %s\n", number, ban->banmask, s);
    } else {
      dprintf(idx, "BAN: %s %s\n", ban->banmask, s);
    }
  } else if (show_inact) {
    if (number >= 0) {
      dprintf(idx, "! [%3d] %s %s\n", number, ban->banmask, s);
    } else {
      dprintf(idx, "BAN (%s): %s %s\n", BANS_INACTIVE, ban->banmask, s);
    }
  } else
    return;
  dprintf(idx, "        %s: %s\n", ban->user, ban->desc);
  if (dates[0])
    dprintf(idx, "        %s\n", dates);
}

/* take host entry from exempt list and display it ban-style */
static void display_exempt (int idx, int number, struct exemptrec * exempt,
			    struct chanset_t * chan, int show_inact)
{
  char dates[81], s[41];
  
  if (exempt->added) {
    daysago(now, exempt->added, s);
    sprintf(dates, "%s %s", EXEMPTS_CREATED, s);
    if (exempt->added < exempt->lastactive) {
      strcat(dates, ", ");
      strcat(dates, EXEMPTS_LASTUSED);
      strcat(dates, " ");
      daysago(now, exempt->lastactive, s);
      strcat(dates, s);
    }
  } else
    dates[0] = 0;
  if (exempt->flags & EXEMPTREC_PERM)
    strcpy(s, "(perm)");
  else {
    char s1[41];
    days(exempt->expire, now, s1);
    sprintf(s, "(expires %s)", s1);
  }
  if (exempt->flags & EXEMPTREC_STICKY)
    strcat(s, " (sticky)");
  if (!chan || isexempted(chan, exempt->exemptmask)) {
    if (number >= 0) {
      dprintf(idx, "  [%3d] %s %s\n", number, exempt->exemptmask, s);
    } else {
      dprintf(idx, "EXEMPT: %s %s\n", exempt->exemptmask, s);
    }
  } else if (show_inact) {
    if (number >= 0) {
      dprintf(idx, "! [%3d] %s %s\n", number, exempt->exemptmask, s);
    } else {
      dprintf(idx, "EXEMPT (%s): %s %s\n", EXEMPTS_INACTIVE, exempt->exemptmask, s);
    }
  } else 
    return;
  dprintf(idx, "        %s: %s\n", exempt->user, exempt->desc);
  if (dates[0])
    dprintf(idx, "        %s\n", dates);
}
 
/* take host entry from invite list and display it ban-style */
static void display_invite (int idx, int number, struct inviterec * invite,
			    struct chanset_t * chan, int show_inact)
{
  char dates[81], s[41];
  
  if (invite->added) {
    daysago(now, invite->added, s);
    sprintf(dates, "%s %s", INVITES_CREATED, s);
    if (invite->added < invite->lastactive) {
      strcat(dates, ", ");
      strcat(dates, INVITES_LASTUSED);
      strcat(dates, " ");
      daysago(now, invite->lastactive, s);
      strcat(dates, s);
    }
  } else
    dates[0] = 0;
  if (invite->flags & INVITEREC_PERM)
    strcpy(s, "(perm)");
  else {
    char s1[41];
    days(invite->expire, now, s1);
    sprintf(s, "(expires %s)", s1);
  }
  if (invite->flags & INVITEREC_STICKY)
    strcat(s, " (sticky)");
  if (!chan || isinvited(chan, invite->invitemask)) {
    if (number >= 0) {
      dprintf(idx, "  [%3d] %s %s\n", number, invite->invitemask, s);
    } else {
      dprintf(idx, "INVITE: %s %s\n", invite->invitemask, s);
    }
  } else if (show_inact) {
    if (number >= 0) {
      dprintf(idx, "! [%3d] %s %s\n", number, invite->invitemask, s);
    } else {
      dprintf(idx, "INVITE (%s): %s %s\n", INVITES_INACTIVE, invite->invitemask, s);
    }
  } else 
    return;
  dprintf(idx, "        %s: %s\n", invite->user, invite->desc);
  if (dates[0])
    dprintf(idx, "        %s\n", dates);
}

static void tell_bans(int idx, int show_inact, char *match)
{
  int k = 1;
  char *chname;
  struct chanset_t *chan = NULL;
  struct banrec *u;

  /* was channel given? */
  context;
  if (match[0]) {
    chname = newsplit(&match);
    if (strchr(CHANMETA, chname[0]) != NULL) {
      chan = findchan(chname);
      if (!chan) {
	dprintf(idx, "%s.\n", CHAN_NOSUCH);
	return;
      }
    } else
      match = chname;
  }
  if (!chan && !(chan = findchan(dcc[idx].u.chat->con_chan)) &&
      !(chan = chanset))
    return;
  if (show_inact)
    dprintf(idx, "%s:   (! = %s %s)\n", BANS_GLOBAL,
	    BANS_NOTACTIVE, chan->name);
  else
    dprintf(idx, "%s:\n", BANS_GLOBAL);
  context;
  u = global_bans;
  for (; u; u = u->next) {
    if (match[0]) {
      if ((wild_match(match, u->banmask)) ||
	  (wild_match(match, u->desc)) ||
	  (wild_match(match, u->user)))
	display_ban(idx, k, u, chan, 1);
      k++;
    } else
      display_ban(idx, k++, u, chan, show_inact);
  }
  if (show_inact)
    dprintf(idx, "%s %s:   (! = %s, * = %s)\n",
	    BANS_BYCHANNEL, chan->name,
	    BANS_NOTACTIVE2, BANS_NOTBYBOT);
  else
    dprintf(idx, "%s %s:  (* = %s)\n",
	    BANS_BYCHANNEL, chan->name,
	    BANS_NOTBYBOT);
  u = chan->bans;
  for (; u; u = u->next) {
    if (match[0]) {
      if ((wild_match(match, u->banmask)) ||
	  (wild_match(match, u->desc)) ||
	  (wild_match(match, u->user)))
	display_ban(idx, k, u, chan, 1);
      k++;
    } else
      display_ban(idx, k++, u, chan, show_inact);
  }
  if (chan->status & CHAN_ACTIVE) {
    banlist *b = chan->channel.ban;
    char s[UHOSTLEN], *s1, *s2, fill[256];
    int min, sec;

    while (b->ban[0]) {
      if ((!u_equals_ban(global_bans, b->ban)) &&
	  (!u_equals_ban(chan->bans, b->ban))) {
	strcpy(s, b->who);
	s2 = s;
	s1 = splitnick(&s2);
	if (s1[0])
	  sprintf(fill, "%s (%s!%s)", b->ban, s1, s2);
	else if (!strcasecmp(s, "existant"))
	  sprintf(fill, "%s (%s)", b->ban, s2);
	else
	  sprintf(fill, "%s (server %s)", b->ban, s2);
	if (b->timer != 0) {
	  min = (now - b->timer) / 60;
	  sec = (now - b->timer) - (min * 60);
	  sprintf(s, " (active %02d:%02d)", min, sec);
	  strcat(fill, s);
	}
	if ((!match[0]) || (wild_match(match, b->ban)))
	  dprintf(idx, "* [%3d] %s\n", k, fill);
	k++;
      }
      b = b->next;
    }
  }
  if (k == 1)
    dprintf(idx, "(There are no bans, permanent or otherwise.)\n");
  if ((!show_inact) && (!match[0]))
    dprintf(idx, "%s.\n", BANS_USEBANSALL);
}

static void tell_exempts (int idx, int show_inact, char * match)
{
  int k = 1;
  char *chname;
  struct chanset_t *chan = NULL;
  struct exemptrec * u;
  
  /* was channel given? */
  context;
  if (match[0]) {
    chname = newsplit(&match);
    if ((chname[0] == '#') || (chname[0] == '+') || (chname[0] == '&')) {
      chan = findchan(chname);
      if (!chan) {
	dprintf(idx, "%s.\n", CHAN_NOSUCH);
	return;
      }
    } else
      match = chname;
  }
  if (!chan && !(chan = findchan(dcc[idx].u.chat->con_chan))
      && !(chan = chanset))
    return;	 
  if (show_inact)
    dprintf(idx, "%s:   (! = %s %s)\n", EXEMPTS_GLOBAL, 
	    EXEMPTS_NOTACTIVE, chan->name);
  else
    dprintf(idx, "%s:\n", EXEMPTS_GLOBAL);
  context;
  u = global_exempts;
  for (;u;u=u->next) {
    if (match[0]) {
      if ((wild_match(match, u->exemptmask)) ||
	  (wild_match(match, u->desc)) ||
	  (wild_match(match, u->user)))
	display_exempt(idx, k, u, chan, 1);
      k++;
    } else
      display_exempt(idx, k++, u, chan, show_inact);
  }
  if (show_inact)
    dprintf(idx, "%s %s:   (! = %s, * = %s)\n",
	    EXEMPTS_BYCHANNEL, chan->name, 
	    EXEMPTS_NOTACTIVE2,
	    EXEMPTS_NOTBYBOT);
  else
    dprintf(idx, "%s %s:  (* = %s)\n",
	    EXEMPTS_BYCHANNEL, chan->name, 
	    EXEMPTS_NOTBYBOT);
  u = chan->exempts;
  for (;u;u=u->next) {
    if (match[0]) {
      if ((wild_match(match, u->exemptmask)) ||
	  (wild_match(match, u->desc)) ||
	  (wild_match(match, u->user)))
	display_exempt(idx, k, u, chan, 1);
      k++;
    } else
      display_exempt(idx, k++, u, chan, show_inact);
  }
  if (chan->status & CHAN_ACTIVE) {
    exemptlist *e = chan->channel.exempt;
    char s[UHOSTLEN], * s1, *s2,fill[256];
    int min, sec;
    while (e->exempt[0]) {
      if ((!u_equals_exempt(global_exempts,e->exempt)) &&
	  (!u_equals_exempt(chan->exempts, e->exempt))) {
	strcpy(s, e->who);
	s2 = s;
	s1 = splitnick(&s2);
	if (s1[0])
	  sprintf(fill, "%s (%s!%s)", e->exempt, s1, s2);
	else if (!strcasecmp(s, "existant"))
	  sprintf(fill, "%s (%s)", e->exempt, s2);
	else
	  sprintf(fill, "%s (server %s)", e->exempt, s2);
	if (e->timer != 0) {
	  min = (now - e->timer) / 60;
	  sec = (now - e->timer) - (min * 60);
	  sprintf(s, " (active %02d:%02d)", min, sec);
	  strcat(fill, s);
	}
	if ((!match[0]) || (wild_match(match, e->exempt)))
	  dprintf(idx, "* [%3d] %s\n", k, fill);
	k++;
      }
      e = e->next;
    }
  }
  if (k == 1)
    dprintf(idx, "(There are no ban exempts, permanent or otherwise.)\n");
  if ((!show_inact) && (!match[0]))
    dprintf(idx, "%s.\n", EXEMPTS_USEEXEMPTSALL);
}

static void tell_invites (int idx, int show_inact, char * match)
{
  int k = 1;
  char *chname;
  struct chanset_t *chan = NULL;
  struct inviterec * u;
  
  /* was channel given? */
  context;
  if (match[0]) {
    chname = newsplit(&match);
    if ((chname[0] == '#') || (chname[0] == '+') || (chname[0] == '&')) {
      chan = findchan(chname);
      if (!chan) {
	dprintf(idx, "%s.\n", CHAN_NOSUCH);
	return;
      }
    } else
      match = chname;
  }
  if (!chan && !(chan = findchan(dcc[idx].u.chat->con_chan))
      && !(chan = chanset))
    return;	 
  if (show_inact)
    dprintf(idx, "%s:   (! = %s %s)\n", INVITES_GLOBAL, 
	    INVITES_NOTACTIVE, chan->name);
  else
    dprintf(idx, "%s:\n", INVITES_GLOBAL);
  context;
  u = global_invites;
  for (;u;u=u->next) {
    if (match[0]) {
      if ((wild_match(match, u->invitemask)) ||
	  (wild_match(match, u->desc)) ||
	  (wild_match(match, u->user)))
	display_invite(idx, k, u, chan, 1);
      k++;
    } else
      display_invite(idx, k++, u, chan, show_inact);
  }
  if (show_inact)
    dprintf(idx, "%s %s:   (! = %s, * = %s)\n",
	    INVITES_BYCHANNEL, chan->name, 
	    INVITES_NOTACTIVE2,
	    INVITES_NOTBYBOT);
  else
    dprintf(idx, "%s %s:  (* = %s)\n",
	    INVITES_BYCHANNEL, chan->name, 
	    INVITES_NOTBYBOT);
  u = chan->invites;
  for (;u;u=u->next) {
    if (match[0]) {
      if ((wild_match(match, u->invitemask)) ||
	  (wild_match(match, u->desc)) ||
	  (wild_match(match, u->user)))
	display_invite(idx, k, u, chan, 1);
      k++;
    } else
      display_invite(idx, k++, u, chan, show_inact);
  }
  if (chan->status & CHAN_ACTIVE) {
    invitelist *i = chan->channel.invite;
    char s[UHOSTLEN], * s1, *s2,fill[256];
    int min, sec;
    while (i->invite[0]) {
      if ((!u_equals_invite(global_invites,i->invite)) &&
	  (!u_equals_invite(chan->invites, i->invite))) {
	strcpy(s, i->who);
	s2 = s;
	s1 = splitnick(&s2);
	if (s1[0])
	  sprintf(fill, "%s (%s!%s)", i->invite, s1, s2);
	else if (!strcasecmp(s, "existant"))
	  sprintf(fill, "%s (%s)", i->invite, s2);
	else
	  sprintf(fill, "%s (server %s)", i->invite, s2);
	if (i->timer != 0) {
	  min = (now - i->timer) / 60;
	  sec = (now - i->timer) - (min * 60);
	  sprintf(s, " (active %02d:%02d)", min, sec);
	  strcat(fill, s);
	}
	if ((!match[0]) || (wild_match(match, i->invite)))
	  dprintf(idx, "* [%3d] %s\n", k, fill);
	k++;
      }
      i = i->next;
    }
  }
  if (k == 1)
    dprintf(idx, "(There are no invites, permanent or otherwise.)\n");
  if ((!show_inact) && (!match[0]))
    dprintf(idx, "%s.\n", INVITES_USEINVITESALL);
}


/* write channel's local banlist to a file */
static int write_bans(FILE * f, int idx)
{
  struct chanset_t *chan;
  struct banrec *b;
  struct igrec *i;

  if (global_ign)
    if (fprintf(f, IGNORE_NAME " - -\n") == EOF)	/* Daemus */
      return 0;
  for (i = global_ign; i; i = i->next)
    if (fprintf(f, "- %s:%s%lu:%s:%lu:%s\n", i->igmask,
		(i->flags & IGREC_PERM) ? "+" : "", i->expire,
		i->user ? i->user : botnetnick, i->added, i->msg ? i->msg : "") == EOF)
      return 0;
  if (global_bans)
    if (fprintf(f, BAN_NAME " - -\n") == EOF)	/* Daemus */
      return 0;
  for (b = global_bans; b; b = b->next)
    if (fprintf(f, "- %s:%s%lu%s:+%lu:%lu:%s:%s\n", b->banmask,
		(b->flags & BANREC_PERM) ? "+" : "", b->expire,
		(b->flags & BANREC_STICKY) ? "*" : "", b->added,
		b->lastactive, b->user ? b->user : botnetnick,
		b->desc ? b->desc : "requested") == EOF)
      return 0;
  for (chan = chanset; chan; chan = chan->next)
    if ((idx < 0) || (chan->status & CHAN_SHARED)) {
      struct flag_record fr =
      {FR_CHAN | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0};

      if ((idx >= 0) && !(fr.bot & BOT_GLOBAL))
	get_user_flagrec(dcc[idx].user, &fr, chan->name);
      else
	fr.chan = BOT_SHARE;
      if (fr.chan & BOT_SHARE) {
	if (fprintf(f, "::%s bans\n", chan->name) == EOF)
	  return 0;
	for (b = chan->bans; b; b = b->next)
	  if (fprintf(f, "- %s:%s%lu%s:+%lu:%lu:%s:%s\n", b->banmask,
		      (b->flags & BANREC_PERM) ? "+" : "", b->expire,
		      (b->flags & BANREC_STICKY) ? "*" : "", b->added,
		      b->lastactive, b->user ? b->user : botnetnick,
		      b->desc ? b->desc : "requested") == EOF)
	    return 0;
      }
    }
  return 1;
}

/* write channel's local exemptlist to a file */
static int write_exempts (FILE * f, int idx)
{
  struct chanset_t *chan;
  struct exemptrec * e;
  struct igrec * i;
  
  if (global_ign)
    if (fprintf(f, IGNORE_NAME " - -\n")==EOF) /* Daemus */
      return 0;
  for (i = global_ign;i;i=i->next) 
    if (fprintf(f,"%s %s:%s%lu:%s:%lu:%s\n","%", i->igmask,
		(i->flags & IGREC_PERM) ? "+" : "", i->expire,
		i->user?i->user:botnetnick, i->added, i->msg?i->msg:"")==EOF)
      return 0;
  if (global_exempts)
    if (fprintf(f, EXEMPT_NAME " - -\n")==EOF) /* Daemus */
      return 0;
  for (e = global_exempts;e;e=e->next) 
    if (fprintf(f,"%s %s:%s%lu%s:+%lu:%lu:%s:%s\n","%",e->exemptmask,
		(e->flags & EXEMPTREC_PERM) ? "+" : "", e->expire,
		(e->flags & EXEMPTREC_STICKY) ? "*" : "", e->added,
		e->lastactive, e->user?e->user:botnetnick,
		e->desc?e->desc:"requested")==EOF)
      return 0;
  for (chan = chanset;chan;chan=chan->next) 
    if ((idx < 0) || (chan->status & CHAN_SHARED)) {
      struct flag_record fr = {FR_CHAN|FR_GLOBAL|FR_BOT,0,0,0,0,0};
      if ((idx >= 0) && !(fr.bot & BOT_GLOBAL))
	get_user_flagrec(dcc[idx].user,&fr,chan->name);
      else
	fr.chan = BOT_SHARE;
      if (fr.chan & BOT_SHARE) {
	if (fprintf(f, "&&%s exempts\n", chan->name) == EOF)
	  return 0;
	for (e = chan->exempts;e;e=e->next) 
	  if (fprintf(f,"%s %s:%s%lu%s:+%lu:%lu:%s:%s\n","%",e->exemptmask,
		      (e->flags & EXEMPTREC_PERM) ? "+" : "", e->expire,
		      (e->flags & EXEMPTREC_STICKY) ? "*" : "", e->added,
		      e->lastactive, e->user?e->user:botnetnick,
		      e->desc?e->desc:"requested")==EOF)
	    return 0;
      }
    }
  return 1;
}

/* write channel's local invitelist to a file */
static int write_invites (FILE * f, int idx)
{
  struct chanset_t *chan;
  struct inviterec * ir;
  struct igrec * i;
  
  if (global_ign)
    if (fprintf(f, IGNORE_NAME " - -\n")==EOF) /* Daemus */
      return 0;
  for (i = global_ign;i;i=i->next) 
    if (fprintf(f,"@ %s:%s%lu:%s:%lu:%s\n", i->igmask,
		(i->flags & IGREC_PERM) ? "+" : "", i->expire,
		i->user?i->user:botnetnick, i->added, i->msg?i->msg:"")==EOF)
      return 0;
  if (global_invites)
    if (fprintf(f, INVITE_NAME " - -\n")==EOF) /* Daemus */
      return 0;
  for (ir = global_invites;ir;ir=ir->next) 
    if (fprintf(f,"@ %s:%s%lu%s:+%lu:%lu:%s:%s\n",ir->invitemask,
		(ir->flags & INVITEREC_PERM) ? "+" : "", ir->expire,
		(ir->flags & INVITEREC_STICKY) ? "*" : "", ir->added,
		ir->lastactive, ir->user?ir->user:botnetnick,
		ir->desc?ir->desc:"requested")==EOF)
      return 0;
  for (chan = chanset;chan;chan=chan->next) 
    if ((idx < 0) || (chan->status & CHAN_SHARED)) {
      struct flag_record fr = {FR_CHAN|FR_GLOBAL|FR_BOT,0,0,0,0,0};
      if ((idx >= 0) && !(fr.bot & BOT_GLOBAL))
	get_user_flagrec(dcc[idx].user,&fr,chan->name);
      else
	fr.chan = BOT_SHARE;
      if (fr.chan & BOT_SHARE) {
	if (fprintf(f, "$$%s invites\n", chan->name) == EOF)
	  return 0;
	for (ir = chan->invites;ir;ir=ir->next) 
	  if (fprintf(f,"@ %s:%s%lu%s:+%lu:%lu:%s:%s\n",ir->invitemask,
		      (ir->flags & INVITEREC_PERM) ? "+" : "", ir->expire,
		      (ir->flags & INVITEREC_STICKY) ? "*" : "", ir->added,
		      ir->lastactive, ir->user?ir->user:botnetnick,
		      ir->desc?ir->desc:"requested")==EOF)
	    return 0;
      }
    }
  return 1;
}

static void channels_writeuserfile()
{
  char s[1024];
  FILE *f;

  simple_sprintf(s, "%s~new", userfile);
  f = fopen(s, "a");
  write_bans(f, -1);
  write_exempts(f,-1);
  write_invites(f,-1);
  fclose(f);
  write_channels();
}

/* check for expired timed-bans */
static void check_expired_bans()
{
  struct banrec **u;
  struct chanset_t *chan;

  u = &global_bans;
  while (*u) {
    if (!((*u)->flags & BANREC_PERM) && (now >= (*u)->expire)) {
      putlog(LOG_MISC, "*", "%s %s (%s)", BANS_NOLONGER,
	     (*u)->banmask, MISC_EXPIRED);
      chan = chanset;
      while (chan != NULL) {
	add_mode(chan, '-', 'b', (*u)->banmask);
	chan = chan->next;
      }
      u_delban(NULL, (*u)->banmask, 1);
    } else
      u = &((*u)->next);
  }
  /* check for specific channel-domain bans expiring */
  for (chan = chanset; chan; chan = chan->next) {
    u = &chan->bans;
    while (*u) {
      if (!((*u)->flags & BANREC_PERM) && (now >= (*u)->expire)) {
	putlog(LOG_MISC, "*", "%s %s %s %s (%s)", BANS_NOLONGER,
	       (*u)->banmask, MISC_ONLOCALE, chan->name, MISC_EXPIRED);
	add_mode(chan, '-', 'b', (*u)->banmask);
	u_delban(chan, (*u)->banmask, 1);
      } else
	u = &((*u)->next);
    }
  }
}

/* check for expired timed-exemptions */
static void check_expired_exempts()
{
  struct exemptrec ** u;
  struct chanset_t *chan;
  banlist *b;
  int match;

  u = &global_exempts;
  while (*u) {
    if (!((*u)->flags & EXEMPTREC_PERM) && (now >= (*u)->expire)) {
      putlog(LOG_MISC, "*", "%s %s (%s)", EXEMPTS_NOLONGER,
	     (*u)->exemptmask, MISC_EXPIRED);
      chan = chanset;
      while (chan != NULL) {
	match=0;
	b = chan->channel.ban;
	while (b->ban[0] && !match) {
	  if (wild_match(b->ban, (*u)->exemptmask) ||
	      wild_match((*u)->exemptmask,b->ban))
	    match=1;
	  else
	    b = b->next;
	}
	if (match) {
	  putlog(LOG_MISC, chan->name,
		 "Exempt not expired on channel %s. Ban still set!",
		 chan->name); 
	} else {
	  add_mode(chan, '-', 'e', (*u)->exemptmask);
	}
	chan = chan->next;
      }
      u_delexempt(NULL,(*u)->exemptmask,1);
    } else
      u = &((*u)->next);
  }
  /* check for specific channel-domain exempts expiring */
  for (chan = chanset;chan;chan = chan->next) {
    u = &chan->exempts;
    while (*u) {
      if (!((*u)->flags & EXEMPTREC_PERM) && (now >= (*u)->expire)) {
	match=0;
	b = chan->channel.ban;
	while (b->ban[0] && !match) {
	  if (wild_match(b->ban, (*u)->exemptmask) ||
	      wild_match((*u)->exemptmask,b->ban))
	    match=1;
	  else
	    b = b->next;
	}
	if (match) {
	  putlog(LOG_MISC, chan->name,
		 "Exempt not expired on channel %s. Ban still set!",
		 chan->name);
	} else {
	  putlog(LOG_MISC, "*", "%s %s %s %s (%s)", EXEMPTS_NOLONGER,
		 (*u)->exemptmask, MISC_ONLOCALE, chan->name, MISC_EXPIRED);
	  add_mode(chan, '-', 'e', (*u)->exemptmask);
	  u_delexempt(chan,(*u)->exemptmask,1);
	}
      }
      u = &((*u)->next);
    }
  }
}

/* check for expired timed-invites */
static void check_expired_invites()
{
  struct inviterec ** u;
  struct chanset_t *chan;
  
  u = &global_invites;
  while (*u) {
    if (!((*u)->flags & INVITEREC_PERM) && (now >= (*u)->expire)) {
      putlog(LOG_MISC, "*", "%s %s (%s)", INVITES_NOLONGER,
	     (*u)->invitemask, MISC_EXPIRED);
      chan = chanset;
      while (chan != NULL && !(chan->channel.mode & CHANINV)){
 	    add_mode(chan, '-', 'I', (*u)->invitemask);
 	    chan = chan->next;
      }
      u_delinvite(NULL,(*u)->invitemask,1);
    } else
      u = &((*u)->next);
  }
  /* check for specific channel-domain invites expiring */
  for (chan = chanset;chan;chan = chan->next) {
    u = &chan->invites;
    while (*u) {
      if (!((*u)->flags & INVITEREC_PERM) && (now >= (*u)->expire) 
	  && !(chan->channel.mode & CHANINV)) {
	putlog(LOG_MISC, "*", "%s %s %s %s (%s)", INVITES_NOLONGER,
	       (*u)->invitemask, MISC_ONLOCALE, chan->name, MISC_EXPIRED); 	    add_mode(chan, '-', 'I', (*u)->invitemask);
	u_delinvite(chan,(*u)->invitemask,1);
      } else
	u = &((*u)->next);
    }
  }
}
