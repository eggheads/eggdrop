
/* streamlined by answer */
static int tcl_chanlist STDVAR
{
  char s1[121];
  int f;
  memberlist *m;
  struct userrec *u;
  struct chanset_t *chan;
  struct flag_record plus = {FR_CHAN | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0},
 		     minus = {FR_CHAN | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0},
		     user = {FR_CHAN | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0};

  BADARGS(2, 3, " channel ?flags?");
  context;
  chan = findchan(argv[1]);
  if (!chan) {
    Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  context;
  m = chan->channel.member;
  if (argc == 2) {
    /* no flag restrictions so just whiz it thru quick */
    while (m->nick[0]) {
      Tcl_AppendElement(irp, m->nick);
      m = m->next;
    }
    return TCL_OK;
  }
  break_down_flags(argv[2], &plus, &minus);
  f = (minus.global || minus.udef_global ||
       minus.chan || minus.udef_chan || minus.bot);
  /* return empty set if asked for flags but flags don't exist */
  if (!plus.global && !plus.udef_global &&
      !plus.chan && !plus.udef_chan && !plus.bot && !f)
    return TCL_OK;
  minus.match = plus.match ^ (FR_AND | FR_OR);
  while (m->nick[0]) {
    simple_sprintf(s1, "%s!%s", m->nick, m->userhost);
    u = get_user_by_host(s1);
    get_user_flagrec(u, &user, argv[1]);
    user.match = plus.match;
    if (flagrec_eq(&plus, &user)) {
      if (!f || !flagrec_eq(&minus, &user))
	Tcl_AppendElement(irp, m->nick);
    }
    m = m->next;
  }
  return TCL_OK;
}

static int tcl_botisop STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if (me_op(chan))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_botisvoice STDVAR
{
  struct chanset_t *chan;
  memberlist *mx;

  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if ((mx = ismember(chan, botname)) && chan_hasvoice(mx))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_botonchan STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if (ismember(chan, botname))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_isop STDVAR
{
  struct chanset_t *chan;
  memberlist *mx;

  BADARGS(3, 3, " nick channel");
  chan = findchan(argv[2]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if ((mx = ismember(chan, argv[1])) && chan_hasop(mx))
    Tcl_AppendResult(irp, "1", NULL);

  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_isvoice STDVAR
{
  struct chanset_t *chan;
  memberlist *mx;

  BADARGS(3, 3, " nick channel");
  chan = findchan(argv[2]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if ((mx = ismember(chan, argv[1])) && chan_hasvoice(mx))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_onchan STDVAR
{
  struct chanset_t *chan;

  BADARGS(3, 3, " nickname channel");
  chan = findchan(argv[2]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (!ismember(chan, argv[1]))
    Tcl_AppendResult(irp, "0", NULL);
  else
    Tcl_AppendResult(irp, "1", NULL);
  return TCL_OK;
}

static int tcl_handonchan STDVAR
{
  struct chanset_t *chan;
  struct userrec *u;

  BADARGS(3, 3, " handle channel");
  chan = findchan(argv[2]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if ((u = get_user_by_handle(userlist, argv[1])))
    if (hand_on_chan(chan, u)) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_ischanban STDVAR
{
  struct chanset_t *chan;

  BADARGS(3, 3, " ban channel");
  chan = findchan(argv[2]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (ischanban(chan, argv[1]))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_ischanexempt STDVAR
{
  struct chanset_t *chan;

  BADARGS(3, 3, " exempt channel");
  chan = findchan(argv[2]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (ischanexempt(chan, argv[1]))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_ischaninvite STDVAR
{
  struct chanset_t *chan;

  BADARGS(3, 3, " invite channel");
  chan = findchan(argv[2]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (ischaninvite(chan, argv[1]))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_getchanhost STDVAR
{
  struct chanset_t *chan;
  struct chanset_t *thechan = NULL;
  memberlist *m;

  context;
  BADARGS(2, 3, " nickname ?channel?");	/* drummer */
  if (argv[2]) {
    thechan = findchan(argv[2]);
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  }
  context;
  chan = chanset;
  while (chan != NULL) {
    m = ismember(chan, argv[1]);
    if (m && ((chan == thechan) || (thechan == NULL))) {
      Tcl_AppendResult(irp, m->userhost, NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  return TCL_OK;
}

static int tcl_onchansplit STDVAR
{
  struct chanset_t *chan;
  memberlist *m;

  BADARGS(3, 3, " nickname channel");
  if (!(chan = findchan(argv[2]))) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  m = ismember(chan, argv[1]);
  if (m && chan_issplit(m))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_maskhost STDVAR
{
  char new[121];
  BADARGS(2, 2, " nick!user@host");
  maskhost(argv[1], new);
  Tcl_AppendResult(irp, new, NULL);
  return TCL_OK;
}

static int tcl_getchanidle STDVAR
{
  memberlist *m;
  struct chanset_t *chan;
  char s[20];
  int x;

  BADARGS(3, 3, " nickname channel");
  if (!(chan = findchan(argv[2]))) {
    Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  m = ismember(chan, argv[1]);
  if (m) {
    x = (now - (m->last)) / 60;
    simple_sprintf(s, "%d", x);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

inline int tcl_chanmasks(masklist *m, Tcl_Interp *irp)
{
  char *list[3], work[20], *p;
  
  while(m && m->mask && m->mask[0]) {
    list[0] = m->mask;
    list[1] = m->who;
    simple_sprintf(work, "%lu", now - m->timer);
    list[2] = work;
    p = Tcl_Merge(3, list);
    Tcl_AppendElement(irp, p);
    n_free(p, "", 0);
    
    m = m->next;
  }
  
  return TCL_OK;
}

static int tcl_chanbans STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }

  return tcl_chanmasks(chan->channel.ban, irp);
}

static int tcl_chanexempts STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }

  return tcl_chanmasks(chan->channel.exempt, irp);
}

static int tcl_chaninvites STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }

  return tcl_chanmasks(chan->channel.invite, irp);
}

static int tcl_getchanmode STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(irp, getchanmode(chan), NULL);
  return TCL_OK;
}

static int tcl_getchanjoin STDVAR
{
  struct chanset_t *chan;
  char s[21];
  memberlist *m;

  BADARGS(3, 3, " nick channel");
  chan = findchan(argv[2]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid cahnnel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  m = ismember(chan, argv[1]);
  if (m == NULL) {
    Tcl_AppendResult(irp, argv[1], " is not on ", argv[2], NULL);
    return TCL_ERROR;
  }
  sprintf(s, "%lu", m->joined);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

/* flushmode <chan> */
static int tcl_flushmode STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  flush_mode(chan, NORMAL);
  return TCL_OK;
}

static int tcl_pushmode STDVAR
{
  struct chanset_t *chan;
  char plus, mode;

  BADARGS(3, 4, " channel mode ?arg?");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  plus = argv[2][0];
  mode = argv[2][1];
  if ((plus != '+') && (plus != '-')) {
    mode = plus;
    plus = '+';
  }
  if (!(((mode >= 'a') && (mode <= 'z')) || ((mode >= 'A') && (mode <= 'Z')))) {
    Tcl_AppendResult(irp, "invalid mode: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (argc < 4) {
    if (strchr("bvoeIk", mode) != NULL) {
      Tcl_AppendResult(irp, "modes b/v/o/e/I/k/l require an argument", NULL);
      return TCL_ERROR;
    } else if (plus == '+' && mode == 'l') {
      Tcl_AppendResult(irp, "modes b/v/o/e/I/k/l require an argument", NULL);
      return TCL_ERROR;
    }
  }
  if (argc == 4)
    add_mode(chan, plus, mode, argv[3]);
  else
    add_mode(chan, plus, mode, "");
  return TCL_OK;
}

static int tcl_resetbans STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel ", argv[1], NULL);
    return TCL_ERROR;
  }
  resetbans(chan);
  return TCL_OK;
}

static int tcl_resetexempts STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel ", argv[1], NULL);
    return TCL_ERROR;
  }
  resetexempts(chan);
  return TCL_OK;
}

static int tcl_resetinvites STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel ", argv[1], NULL);
    return TCL_ERROR;
  }
  resetinvites(chan);
  return TCL_OK;
}

static int tcl_resetchan STDVAR
{
  struct chanset_t *chan;

  context;
  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel ", argv[1], NULL);
    return TCL_ERROR;
  }
  reset_chan_info(chan);
  return TCL_OK;
}

static int tcl_topic STDVAR
{
  struct chanset_t *chan;

  context;
  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel ", argv[1], NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(irp, chan->channel.topic, NULL);
  return TCL_OK;
}

static int tcl_hand2nick STDVAR
{
  memberlist *m;
  char s[161];
  struct chanset_t *chan;
  struct chanset_t *thechan = NULL;
  struct userrec *u;

  context;
  BADARGS(2, 3, " handle ?channel?");	/* drummer */
  if (argv[2]) {
    chan = findchan(argv[2]);
    thechan = chan;
    if (chan == NULL) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else {
    chan = chanset;
  }
  context;
  while ((chan != NULL) && ((thechan == NULL) || (thechan == chan))) {
    m = chan->channel.member;
    while (m->nick[0]) {
      simple_sprintf(s, "%s!%s", m->nick, m->userhost);
      u = get_user_by_host(s);
      if (u && !strcasecmp(u->handle, argv[1])) {
	Tcl_AppendResult(irp, m->nick, NULL);
	return TCL_OK;
      }
      m = m->next;
    }
    chan = chan->next;
  }
  return TCL_OK;		/* blank */
}

static int tcl_nick2hand STDVAR
{
  memberlist *m;
  char s[161];
  struct chanset_t *chan;
  struct chanset_t *thechan = NULL;
  struct userrec *u;

  context;
  BADARGS(2, 3, " nick ?channel?");	/* drummer */
  if (argv[2]) {
    chan = findchan(argv[2]);
    thechan = chan;
    if (chan == NULL) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else {
    chan = chanset;
  }
  context;
  while ((chan != NULL) && ((thechan == NULL) || (thechan == chan))) {
    m = ismember(chan, argv[1]);
    if (m) {
      simple_sprintf(s, "%s!%s", m->nick, m->userhost);
      u = get_user_by_host(s);
      Tcl_AppendResult(irp, u ? u->handle : "*", NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  context;
  return TCL_OK;		/* blank */
}

/*  sends an optimal number of kicks per command (as defined by
 *  kick_method) to the server, simialer to kick_all.  Fabian */
static int tcl_putkick STDVAR
{
  struct chanset_t *chan;
  int k = 0, l;
  char kicknick[512], *nick, *p, *comment = NULL;
  memberlist *m;

  context;
  BADARGS(3, 4, " channel nick?s? ?comment?");
  chan = findchan(argv[1]);
  if (chan == NULL) { 
    Tcl_AppendResult(irp, "illegal channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if (argc == 4)
    comment = argv[3];
  else
    comment = "";
  if (!me_op(chan)) {
    Tcl_AppendResult(irp, "need op", NULL);
    return TCL_ERROR;
  }
  
  kicknick[0] = 0;
  p = argv[2];
  /* loop through all given nicks */
  while(p) {
    nick = p;
    p = strchr(nick, ',');	/* search for beginning of next nick */
    if (p) {
      *p = 0;
      p++;
    }
    
    m = ismember(chan, nick);
    if (!m)
      continue;			/* skip non-existant nicks */
    m->flags |= SENTKICK;	/* mark as pending kick */
    if (kicknick[0])
      strcat(kicknick, ",");
    strcat(kicknick, nick);	/* add to local queue */
    k++;

    /* check if we should send the kick command yet */
    l = strlen(chan->name) + strlen(kicknick) + strlen(comment);
    if (((kick_method != 0) && (k == kick_method)) || (l > 480)) {
      dprintf(DP_SERVER, "KICK %s %s :%s\n", chan->name, kicknick, comment);
      k = 0;
      kicknick[0] = 0;
    }
  }
  /* clear out all pending kicks in our local kick queue */
  if (k > 0)
   dprintf(DP_SERVER, "KICK %s %s :%s\n", chan->name, kicknick, comment);
  context;
  return TCL_OK;
}

static tcl_cmds tclchan_cmds[] =
{
  {"chanlist", tcl_chanlist},
  {"botisop", tcl_botisop},
  {"botisvoice", tcl_botisvoice},
  {"isop", tcl_isop},
  {"isvoice", tcl_isvoice},
  {"onchan", tcl_onchan},
  {"handonchan", tcl_handonchan},
  {"ischanban", tcl_ischanban},
  {"ischanexempt", tcl_ischanexempt},
  {"ischaninvite", tcl_ischaninvite},
  {"getchanhost", tcl_getchanhost},
  {"onchansplit", tcl_onchansplit},
  {"maskhost", tcl_maskhost},
  {"getchanidle", tcl_getchanidle},
  {"chanbans", tcl_chanbans},
  {"chanexempts", tcl_chanexempts},
  {"chaninvites", tcl_chaninvites},
  {"hand2nick", tcl_hand2nick},
  {"nick2hand", tcl_nick2hand},
  {"getchanmode", tcl_getchanmode},
  {"getchanjoin", tcl_getchanjoin},
  {"flushmode", tcl_flushmode},
  {"pushmode", tcl_pushmode},
  {"resetbans", tcl_resetbans},
  {"resetexempts", tcl_resetexempts},
  {"resetinvites", tcl_resetinvites},
  {"resetchan", tcl_resetchan},
  {"topic", tcl_topic},
  {"botonchan", tcl_botonchan},
  {"putkick", tcl_putkick},
  {0, 0}
};
