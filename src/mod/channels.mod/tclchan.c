/* 
 * tclchan.c - part of channels.mod
 * 
 */
/* 
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

static int tcl_killban STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " ban");
  if (u_delban(NULL, argv[1], 1) > 0) {
    chan = chanset;
    while (chan != NULL) {
      add_mode(chan, '-', 'b', argv[1]);
      chan = chan->next;
    }
    Tcl_AppendResult(irp, "1", NULL);
  } else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_killchanban STDVAR
{
  struct chanset_t *chan;

  BADARGS(3, 3, " channel ban");
  chan = findchan(argv[1]);
  if (!chan) {
    Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if (u_delban(chan, argv[2], 1) > 0) {
    add_mode(chan, '-', 'b', argv[2]);
    Tcl_AppendResult(irp, "1", NULL);
  } else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_killexempt STDVAR
{
  struct chanset_t *chan;
  BADARGS(2, 2, " exempt");
  if (u_delexempt(NULL,argv[1],1) > 0) {
    chan = chanset;
    while (chan != NULL) {
      add_mode(chan, '-', 'e', argv[1]);
      chan = chan->next;
    }
    Tcl_AppendResult(irp, "1", NULL);
  } else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_killchanexempt STDVAR
{
  struct chanset_t *chan;
  BADARGS(3, 3, " channel exempt");
  chan = findchan(argv[1]);
  if (!chan) {
    Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if (u_delexempt(chan, argv[2],1) > 0) {
    add_mode(chan, '-', 'e', argv[2]);
    Tcl_AppendResult(irp, "1", NULL);
  } else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_killinvite STDVAR
{
  struct chanset_t *chan;
  BADARGS(2, 2, " invite");
  if (u_delinvite(NULL,argv[1],1) > 0) {
    chan = chanset;
    while (chan != NULL) {
      add_mode(chan, '-', 'I', argv[1]);
      chan = chan->next;
    }
    Tcl_AppendResult(irp, "1", NULL);
  } else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_killchaninvite STDVAR
{
  struct chanset_t *chan;
  BADARGS(3, 3, " channel invite");
  chan = findchan(argv[1]);
  if (!chan) {
    Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if (u_delinvite(chan, argv[2],1) > 0) {
    add_mode(chan, '-', 'I', argv[2]);
    Tcl_AppendResult(irp, "1", NULL);
   } else
     Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_stick STDVAR
{
  struct chanset_t *chan;
  int ok = 0;
  int yn = 1;
  
  BADARGS(2, 3, " ban ?channel?");
  if (argc == 3) {
    chan = findchan(argv[2]);
    if (!chan) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
    if (u_setsticky_ban(chan, argv[1], yn))
      ok = 1;
  }
  if (u_setsticky_ban(NULL, argv[1], yn))
    ok = 1;
  if (ok)
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_unstick STDVAR
{
  struct chanset_t *chan;
  int ok = 0;
  int yn = 0;

  BADARGS(2, 3, " ban ?channel?");
  if (argc == 3) {
    chan = findchan(argv[2]);
    if (!chan) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
    if (u_setsticky_ban(chan, argv[1], yn))
      ok = 1;
  }
  if (u_setsticky_ban(NULL, argv[1], yn))
    ok = 1;
  if (ok)
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_isban STDVAR
{
  struct chanset_t *chan;
  int ok = 0;

  BADARGS(2, 3, " ban ?channel?");
  if (argc == 3) {
    chan = findchan(argv[2]);
    if (!chan) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
    if (u_equals_ban(chan->bans, argv[1]))
      ok = 1;
  }
  if (u_equals_ban(global_bans, argv[1]))
    ok = 1;
  if (ok)
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_isexempt STDVAR
{
  struct chanset_t *chan;
  int ok = 0;
  
  BADARGS(2, 3, " exempt ?channel?");
  if (argc == 3) {
    chan = findchan(argv[2]);
    if (!chan) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
    if (u_equals_exempt(chan->exempts, argv[1]))
      ok = 1;
  }
  if (u_equals_exempt(global_exempts,argv[1]))
    ok = 1;
  if (ok)
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_isinvite STDVAR
{
  struct chanset_t *chan;
  int ok = 0;
  
  BADARGS(2, 3, " invite ?channel?");
  if (argc == 3) {
    chan = findchan(argv[2]);
    if (!chan) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
    if (u_equals_invite(chan->invites, argv[1]))
      ok = 1;
  }
  if (u_equals_invite(global_invites,argv[1]))
    ok = 1;
  if (ok)
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}


static int tcl_isbansticky STDVAR
{
  struct chanset_t *chan;
  int ok = 0;

  BADARGS(2, 3, " ban ?channel?");
  if (argc == 3) {
    chan = findchan(argv[2]);
    if (!chan) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
    if (u_sticky_ban(chan->bans, argv[1]))
      ok = 1;
  }
  if (u_sticky_ban(global_bans, argv[1]))
    ok = 1;
  if (ok)
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_isexemptsticky STDVAR
{
  struct chanset_t *chan;
  int ok = 0;
  
  BADARGS(2, 3, " exempt ?channel?");
  if (argc == 3) {
    chan = findchan(argv[2]);
    if (!chan) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
       }
    if (u_sticky_exempt(chan->exempts, argv[1]))
      ok = 1;
  }
  if (u_sticky_exempt(global_exempts,argv[1]))
    ok = 1;
  if (ok)
       Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_isinvitesticky STDVAR
{
  struct chanset_t *chan;
  int ok = 0;
  
  BADARGS(2, 3, " invite ?channel?");
  if (argc == 3) {
    chan = findchan(argv[2]);
    if (!chan) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
    if (u_sticky_invite(chan->invites, argv[1]))
      ok = 1;
  }
  if (u_sticky_invite(global_invites,argv[1]))
    ok = 1;
  if (ok)
    Tcl_AppendResult(irp, "1", NULL);
  else
       Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_ispermban STDVAR
{
  struct chanset_t *chan;
  int ok = 0;

  BADARGS(2, 3, " ban ?channel?");
  if (argc == 3) {
    chan = findchan(argv[2]);
    if (chan == NULL) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
    if (u_equals_ban(chan->bans, argv[1]) == 2)
      ok = 1;
  }
  if (u_equals_ban(global_bans, argv[1]) == 2)
    ok = 1;
  if (ok)
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_ispermexempt STDVAR
{
  struct chanset_t *chan;
  int ok = 0;
  BADARGS(2, 3, " exempt ?channel?");
  if (argc == 3) {
    chan = findchan(argv[2]);
    if (chan == NULL) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
      if (u_equals_exempt(chan->exempts, argv[1]) == 2)
	ok = 1;
  }
  if (u_equals_exempt(global_exempts,argv[1]) == 2)
    ok = 1;
  if (ok)
    Tcl_AppendResult(irp, "1", NULL);
   else
     Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_isperminvite STDVAR
{
  struct chanset_t *chan;
  int ok = 0;
  BADARGS(2, 3, " invite ?channel?");
  if (argc == 3) {
    chan = findchan(argv[2]);
    if (chan == NULL) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
    if (u_equals_invite(chan->invites, argv[1]) == 2)
      ok = 1;
  }
  if (u_equals_invite(global_invites,argv[1]) == 2)
    ok = 1;
  if (ok)
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_matchban STDVAR
{
  struct chanset_t *chan;
  int ok = 0;

  BADARGS(2, 3, " user!nick@host ?channel?");
  if (argc == 3) {
    chan = findchan(argv[2]);
    if (chan == NULL) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
    if (u_match_ban(chan->bans, argv[1]))
      ok = 1;
  }
  if (u_match_ban(global_bans, argv[1]))
    ok = 1;
  if (ok)
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_matchexempt STDVAR
{
   struct chanset_t *chan;
   int ok = 0;
    BADARGS(2, 3, " user!nick@host ?channel?");
   if (argc == 3) {
      chan = findchan(argv[2]);
      if (chan == NULL) {
     Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
     return TCL_ERROR;
      }
      if (u_match_exempt(chan->exempts, argv[1]))
      ok = 1;
   }
   if (u_match_exempt(global_exempts,argv[1]))
      ok = 1;
   if (ok)
      Tcl_AppendResult(irp, "1", NULL);
   else
      Tcl_AppendResult(irp, "0", NULL);
   return TCL_OK;
}

static int tcl_matchinvite STDVAR
{
   struct chanset_t *chan;
   int ok = 0;
    BADARGS(2, 3, " user!nick@host ?channel?");
   if (argc == 3) {
      chan = findchan(argv[2]);
      if (chan == NULL) {
     Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
     return TCL_ERROR;
      }
      if (u_match_invite(chan->invites, argv[1]))
      ok = 1;
   }
   if (u_match_invite(global_invites,argv[1]))
      ok = 1;
   if (ok)
      Tcl_AppendResult(irp, "1", NULL);
   else
      Tcl_AppendResult(irp, "0", NULL);
   return TCL_OK;
}

static int tcl_newchanban STDVAR
{
  time_t expire_time;
  struct chanset_t *chan;
  char ban[161], cmt[66], from[HANDLEN + 1];
  int sticky = 0;

  BADARGS(5, 7, " channel ban creator comment ?lifetime? ?options?");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if (argc == 7) {
    if (!strcasecmp(argv[6], "none"));
    else if (!strcasecmp(argv[6], "sticky"))
      sticky = 1;
    else {
      Tcl_AppendResult(irp, "invalid option ", argv[6], " (must be one of: ",
		       "sticky, none)", NULL);
      return TCL_ERROR;
    }
  }
  strncpy(ban, argv[2], 160);
  ban[160] = 0;
  strncpy(from, argv[3], HANDLEN);
  from[HANDLEN] = 0;
  strncpy(cmt, argv[4], 65);
  cmt[65] = 0;
  if (argc == 5)
    expire_time = now + (60 * ban_time);
  else {
    if (atoi(argv[5]) == 0)
      expire_time = 0L;
    else
      expire_time = now + (atoi(argv[5]) * 60);
  }
  if (u_addban(chan, ban, from, cmt, expire_time, sticky))
    add_mode(chan, '+', 'b', ban);
  return TCL_OK;
}



static int tcl_newban STDVAR
{
  time_t expire_time;
  struct chanset_t *chan;
  char ban[UHOSTLEN], cmt[66], from[HANDLEN + 1];
  int sticky = 0;

  BADARGS(4, 6, " ban creator comment ?lifetime? ?options?");
  if (argc == 6) {
    if (!strcasecmp(argv[5], "none"));
    else if (!strcasecmp(argv[5], "sticky"))
      sticky = 1;
    else {
      Tcl_AppendResult(irp, "invalid option ", argv[5], " (must be one of: ",
		       "sticky, none)", NULL);
      return TCL_ERROR;
    }
  }
  strncpy(ban, argv[1], UHOSTLEN - 1);
  ban[UHOSTLEN - 1] = 0;
  strncpy(from, argv[2], HANDLEN);
  from[HANDLEN] = 0;
  strncpy(cmt, argv[3], 65);
  cmt[65] = 0;
  if (argc == 4)
    expire_time = now + (60 * ban_time);
  else {
    if (atoi(argv[4]) == 0)
      expire_time = 0L;
    else
      expire_time = now + (atoi(argv[4]) * 60);
  }
  u_addban(NULL, ban, from, cmt, expire_time, sticky);
  chan = chanset;
  while (chan != NULL) {
    add_mode(chan, '+', 'b', ban);
    chan = chan->next;
  }
  return TCL_OK;
}

static int tcl_newchanexempt STDVAR
{
  time_t expire_time;
  struct chanset_t *chan;
  char exempt[161], cmt[66], from[HANDLEN+1];
  int sticky = 0;
 
  BADARGS(5, 7, " channel exempt creator comment ?lifetime? ?options?");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if (argc == 7) {
    if (!strcasecmp(argv[6], "none"));
    else if (!strcasecmp(argv[6], "sticky"))
      sticky = 1;
    else {
      Tcl_AppendResult(irp, "invalid option ", argv[6], " (must be one of: ",
		       "sticky, none)", NULL);
      return TCL_ERROR;
    }
  }
  strncpy(exempt, argv[2], 160);
  exempt[160] = 0;
  strncpy(from, argv[3], HANDLEN);
  from[HANDLEN] = 0;
  strncpy(cmt, argv[4], 65);
  cmt[65] = 0;
  if (argc == 5)
    expire_time = now + (60 * exempt_time);
  else {
    if (atoi(argv[5]) == 0)
      expire_time = 0L;
    else
      expire_time = now + (atoi(argv[5]) * 60);
  }
  if (u_addexempt(chan, exempt, from, cmt, expire_time,sticky))
    add_mode(chan, '+', 'e', exempt);
  return TCL_OK;
}
 
static int tcl_newexempt STDVAR
{
  time_t expire_time;
  struct chanset_t *chan;
  char exempt[UHOSTLEN], cmt[66], from[HANDLEN+1];
  int sticky = 0;
 
  BADARGS(4, 6, " exempt creator comment ?lifetime? ?options?");
  if (argc == 6) {
    if (!strcasecmp(argv[5], "none"));
    else if (strcasecmp(argv[5], "sticky") == 0)
      sticky = 1;
    else {
      Tcl_AppendResult(irp, "invalid option ", argv[5], " (must be one of: ",
		       "sticky, none)", NULL);
      return TCL_ERROR;
    }
  }
  strncpy(exempt, argv[1], UHOSTLEN - 1);
  exempt[UHOSTLEN - 1] = 0;
  strncpy(from, argv[2], HANDLEN);
  from[HANDLEN] = 0;
  strncpy(cmt, argv[3], 65);
  cmt[65] = 0;
  if (argc == 4)
    expire_time = now + (60 * exempt_time);
  else {
    if (atoi(argv[4]) == 0)
      expire_time = 0L;
    else
      expire_time = now + (atoi(argv[4]) * 60);
  }
  u_addexempt(NULL,exempt, from, cmt, expire_time,sticky);
  chan = chanset;
  while (chan != NULL) {
    add_mode(chan, '+', 'e', exempt);
    chan = chan->next;
  }
  return TCL_OK;
}
 
static int tcl_newchaninvite STDVAR
{
  time_t expire_time;
  struct chanset_t *chan;
  char invite[161], cmt[66], from[HANDLEN+1];
  int sticky = 0;
  
  BADARGS(5, 7, " channel invite creator comment ?lifetime? ?options?");
  chan = findchan(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if (argc == 7) {
    if (!strcasecmp(argv[6], "none"));
    else if (!strcasecmp(argv[6], "sticky"))
      sticky = 1;
    else {
      Tcl_AppendResult(irp, "invalid option ", argv[6], " (must be one of: ",
		       "sticky, none)", NULL);
      return TCL_ERROR;
    }
  }
  strncpy(invite, argv[2], 160);
  invite[160] = 0;
  strncpy(from, argv[3], HANDLEN);
  from[HANDLEN] = 0;
  strncpy(cmt, argv[4], 65);
  cmt[65] = 0;
  if (argc == 5)
    expire_time = now + (60 * invite_time);
  else {
    if (atoi(argv[5]) == 0)
      expire_time = 0L;
    else
      expire_time = now + (atoi(argv[5]) * 60);
  }
  if (u_addinvite(chan, invite, from, cmt, expire_time,sticky))
    add_mode(chan, '+', 'I', invite);
  return TCL_OK;
}
 
static int tcl_newinvite STDVAR
{
  time_t expire_time;
  struct chanset_t *chan;
  char invite[UHOSTLEN], cmt[66], from[HANDLEN+1];
  int sticky = 0;

  BADARGS(4, 6, " invite creator comment ?lifetime? ?options?");
  if (argc == 6) {
    if (!strcasecmp(argv[5], "none"));
    else if (strcasecmp(argv[5], "sticky") == 0)
      sticky = 1;
    else {
      Tcl_AppendResult(irp, "invalid option ", argv[5], " (must be one of: ",
		       "sticky, none)", NULL);
      return TCL_ERROR;
    }
  }
  strncpy(invite, argv[1], UHOSTLEN - 1);
  invite[UHOSTLEN - 1] = 0;
  strncpy(from, argv[2], HANDLEN);
  from[HANDLEN] = 0;
  strncpy(cmt, argv[3], 65);
  cmt[65] = 0;
  if (argc == 4)
     expire_time = now + (60 * invite_time);
  else {
    if (atoi(argv[4]) == 0)
      expire_time = 0L;
    else
      expire_time = now + (atoi(argv[4]) * 60);
  }
  u_addinvite(NULL,invite, from, cmt, expire_time,sticky);
  chan = chanset;
  while (chan != NULL) {
     add_mode(chan, '+', 'I', invite);
     chan = chan->next;
  }
  return TCL_OK;
}

static int tcl_channel_info(Tcl_Interp * irp, struct chanset_t *chan)
{
  char s[121];

  get_mode_protect(chan, s);
  Tcl_AppendElement(irp, s);
  simple_sprintf(s, "%d", chan->idle_kick);
  Tcl_AppendElement(irp, s);
  Tcl_AppendElement(irp, chan->need_op);
  Tcl_AppendElement(irp, chan->need_invite);
  Tcl_AppendElement(irp, chan->need_key);
  Tcl_AppendElement(irp, chan->need_unban);
  Tcl_AppendElement(irp, chan->need_limit);
  simple_sprintf(s, "%d:%d", chan->flood_pub_thr, chan->flood_pub_time);
  Tcl_AppendElement(irp, s);
  simple_sprintf(s, "%d:%d", chan->flood_ctcp_thr, chan->flood_ctcp_time);
  Tcl_AppendElement(irp, s);
  simple_sprintf(s, "%d:%d", chan->flood_join_thr, chan->flood_join_time);
  Tcl_AppendElement(irp, s);
  simple_sprintf(s, "%d:%d", chan->flood_kick_thr, chan->flood_kick_time);
  Tcl_AppendElement(irp, s);
  simple_sprintf(s, "%d:%d", chan->flood_deop_thr, chan->flood_deop_time);
  Tcl_AppendElement(irp, s);
  if (chan->status & CHAN_CLEARBANS)
    Tcl_AppendElement(irp, "+clearbans");
  else
    Tcl_AppendElement(irp, "-clearbans");
  if (chan->status & CHAN_ENFORCEBANS)
    Tcl_AppendElement(irp, "+enforcebans");
  else
    Tcl_AppendElement(irp, "-enforcebans");
  if (chan->status & CHAN_DYNAMICBANS)
    Tcl_AppendElement(irp, "+dynamicbans");
  else
    Tcl_AppendElement(irp, "-dynamicbans");
  if (chan->status & CHAN_NOUSERBANS)
    Tcl_AppendElement(irp, "-userbans");
  else
    Tcl_AppendElement(irp, "+userbans");
  if (chan->status & CHAN_OPONJOIN)
    Tcl_AppendElement(irp, "+autoop");
  else
    Tcl_AppendElement(irp, "-autoop");
  if (chan->status & CHAN_BITCH)
    Tcl_AppendElement(irp, "+bitch");
  else
    Tcl_AppendElement(irp, "-bitch");
  if (chan->status & CHAN_GREET)
    Tcl_AppendElement(irp, "+greet");
  else
    Tcl_AppendElement(irp, "-greet");
  if (chan->status & CHAN_PROTECTOPS)
    Tcl_AppendElement(irp, "+protectops");
  else
    Tcl_AppendElement(irp, "-protectops");
  if (chan->status & CHAN_PROTECTFRIENDS)
    Tcl_AppendElement(irp, "+protectfriends");
  else
    Tcl_AppendElement(irp, "-protectfriends");
  if (chan->status & CHAN_DONTKICKOPS)
    Tcl_AppendElement(irp, "+dontkickops");
  else
    Tcl_AppendElement(irp, "-dontkickops");
  if (chan->status & CHAN_WASOPTEST)
    Tcl_AppendElement(irp, "+wasoptest");
  else
    Tcl_AppendElement(irp, "-wasoptest");
  if (chan->status& CHAN_INACTIVE)
    Tcl_AppendElement(irp, "+inactive");
  else
    Tcl_AppendElement(irp, "-inactive");
  if (chan->status & CHAN_LOGSTATUS)
    Tcl_AppendElement(irp, "+statuslog");
  else
    Tcl_AppendElement(irp, "-statuslog");
  if (chan->status & CHAN_STOPNETHACK)
    Tcl_AppendElement(irp, "+stopnethack");
  else
    Tcl_AppendElement(irp, "-stopnethack");
  if (chan->status & CHAN_REVENGE)
    Tcl_AppendElement(irp, "+revenge");
  else
    Tcl_AppendElement(irp, "-revenge");
  if (chan->status & CHAN_REVENGEBOT)
    Tcl_AppendElement(irp, "+revengebot");
  else
    Tcl_AppendElement(irp, "-revengebot");
  if (chan->status & CHAN_SECRET)
    Tcl_AppendElement(irp, "+secret");
  else
    Tcl_AppendElement(irp, "-secret");
  if (chan->status & CHAN_SHARED)
    Tcl_AppendElement(irp, "+shared");
  else
    Tcl_AppendElement(irp, "-shared");
  if (chan->status & CHAN_AUTOVOICE)
    Tcl_AppendElement(irp, "+autovoice");
  else
    Tcl_AppendElement(irp, "-autovoice");
  if (chan->status & CHAN_CYCLE)
    Tcl_AppendElement(irp, "+cycle");
  else
    Tcl_AppendElement(irp, "-cycle");
  if (chan->status & CHAN_SEEN)
    Tcl_AppendElement(irp, "+seen");
  else
    Tcl_AppendElement(irp, "-seen");
  if (chan->ircnet_status& CHAN_DYNAMICEXEMPTS)
    Tcl_AppendElement(irp, "+dynamicexempts");
  else
    Tcl_AppendElement(irp, "-dynamicexempts");
  if (chan->ircnet_status& CHAN_NOUSEREXEMPTS)
    Tcl_AppendElement(irp, "-userexempts");
  else
    Tcl_AppendElement(irp, "+userexempts");
  if (chan->ircnet_status& CHAN_DYNAMICINVITES)
    Tcl_AppendElement(irp, "+dynamicinvites");
  else
    Tcl_AppendElement(irp, "-dynamicinvites");
  if (chan->ircnet_status& CHAN_NOUSERINVITES)
    Tcl_AppendElement(irp, "-userinvites");
  else
    Tcl_AppendElement(irp, "+userinvites");
  return TCL_OK;
}

static int tcl_channel STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 999, " command ?options?");
  if (!strcmp(argv[1], "add")) {
    BADARGS(3, 4, " add channel-name ?options-list?");
    if (argc == 3)
      return tcl_channel_add(irp, argv[2], "");
    return tcl_channel_add(irp, argv[2], argv[3]);
  }
  if (!strcmp(argv[1], "set")) {
    BADARGS(3, 999, " set channel-name ?options?");
    chan = findchan(argv[2]);
    if (chan == NULL) {
      if (chan_hack == 1)
	return TCL_OK;		/* ignore channel settings for a static
				 * channel which has been removed from
				 * the config */
      Tcl_AppendResult(irp, "no such channel record", NULL);
      return TCL_ERROR;
    }
    return tcl_channel_modify(irp, chan, argc - 3, &argv[3]);
  }
  if (!strcmp(argv[1], "info")) {
    BADARGS(3, 3, " info channel-name");
    chan = findchan(argv[2]);
    if (chan == NULL) {
      Tcl_AppendResult(irp, "no such channel record", NULL);
      return TCL_ERROR;
    }
    return tcl_channel_info(irp, chan);
  }
  if (!strcmp(argv[1], "remove")) {
    BADARGS(3, 3, " remove channel-name");
    chan = findchan(argv[2]);
    if (chan == NULL) {
      Tcl_AppendResult(irp, "no such channel record", NULL);
      return TCL_ERROR;
    }
    dprintf(DP_SERVER, "PART %s\n", chan->name);
    clear_channel(chan, 0);
    noshare = 1;
    while (chan->bans)
      u_delban(chan, chan->bans->banmask, 1);
    noshare = 0;
    killchanset(chan);
    return TCL_OK;
  }
  Tcl_AppendResult(irp, "unknown channel command: should be one of: ",
		   "add, set, info, remove", NULL);
  return TCL_ERROR;
}

/* parse options for a channel */
static int tcl_channel_modify(Tcl_Interp * irp, struct chanset_t *chan,
			      int items, char **item)
{
  int i;
  int oldstatus;
  int x=0;
  module_entry *me;

  oldstatus = chan->status;
  for (i = 0; i < items; i++) {
    if (!strcmp(item[i], "need-op")) {
      i++;
      if (i >= items) {
	if (irp)
	  Tcl_AppendResult(irp, "channel need-op needs argument", NULL);
	return TCL_ERROR;
      }
      strncpy(chan->need_op, item[i], 120);
      chan->need_op[120] = 0;
    } else if (!strcmp(item[i], "need-invite")) {
      i++;
      if (i >= items) {
	if (irp)
	  Tcl_AppendResult(irp, "channel need-invite needs argument", NULL);
	return TCL_ERROR;
      }
      strncpy(chan->need_invite, item[i], 120);
      chan->need_invite[120] = 0;
    } else if (!strcmp(item[i], "need-key")) {
      i++;
      if (i >= items) {
	if (irp)
	  Tcl_AppendResult(irp, "channel need-key needs argument", NULL);
	return TCL_ERROR;
      }
      strncpy(chan->need_key, item[i], 120);
      chan->need_key[120] = 0;
    } else if (!strcmp(item[i], "need-limit")) {
      i++;
      if (i >= items) {
	if (irp)
	  Tcl_AppendResult(irp, "channel need-limit needs argument", NULL);
	return TCL_ERROR;
      }
      strncpy(chan->need_limit, item[i], 120);
      chan->need_limit[120] = 0;
    } else if (!strcmp(item[i], "need-unban")) {
      i++;
      if (i >= items) {
	if (irp)
	  Tcl_AppendResult(irp, "channel need-unban needs argument", NULL);
	return TCL_ERROR;
      }
      strncpy(chan->need_unban, item[i], 120);
      chan->need_unban[120] = 0;
    } else if (!strcmp(item[i], "chanmode")) {
      i++;
      if (i >= items) {
	if (irp)
	  Tcl_AppendResult(irp, "channel chanmode needs argument", NULL);
	return TCL_ERROR;
      }
      if (strlen(item[i]) > 120)
	item[i][120] = 0;
      set_mode_protect(chan, item[i]);
    } else if (!strcmp(item[i], "idle-kick")) {
      i++;
      if (i >= items) {
	if (irp)
	  Tcl_AppendResult(irp, "channel idle-kick needs argument", NULL);
	return TCL_ERROR;
      }
      chan->idle_kick = atoi(item[i]);
    } else if (!strcmp(item[i], "dont-idle-kick"))
      chan->idle_kick = 0;
    else if (!strcmp(item[i], "+clearbans"))
      chan->status |= CHAN_CLEARBANS;
    else if (!strcmp(item[i], "-clearbans"))
      chan->status &= ~CHAN_CLEARBANS;
    else if (!strcmp(item[i], "+enforcebans"))
      chan->status |= CHAN_ENFORCEBANS;
    else if (!strcmp(item[i], "-enforcebans"))
      chan->status &= ~CHAN_ENFORCEBANS;
    else if (!strcmp(item[i], "+dynamicbans"))
      chan->status |= CHAN_DYNAMICBANS;
    else if (!strcmp(item[i], "-dynamicbans"))
      chan->status &= ~CHAN_DYNAMICBANS;
    else if (!strcmp(item[i], "-userbans"))
      chan->status |= CHAN_NOUSERBANS;
    else if (!strcmp(item[i], "+userbans"))
      chan->status &= ~CHAN_NOUSERBANS;
    else if (!strcmp(item[i], "+autoop"))
      chan->status |= CHAN_OPONJOIN;
    else if (!strcmp(item[i], "-autoop"))
      chan->status &= ~CHAN_OPONJOIN;
    else if (!strcmp(item[i], "+bitch"))
      chan->status |= CHAN_BITCH;
    else if (!strcmp(item[i], "-bitch"))
      chan->status &= ~CHAN_BITCH;
    else if (!strcmp(item[i], "+greet"))
      chan->status |= CHAN_GREET;
    else if (!strcmp(item[i], "-greet"))
      chan->status &= ~CHAN_GREET;
    else if (!strcmp(item[i], "+protectops"))
      chan->status |= CHAN_PROTECTOPS;
    else if (!strcmp(item[i], "-protectops"))
      chan->status &= ~CHAN_PROTECTOPS;
    else if (!strcmp(item[i], "+protectfriends"))
      chan->status |= CHAN_PROTECTFRIENDS;
    else if (!strcmp(item[i], "-protectfriends"))
      chan->status &= ~CHAN_PROTECTFRIENDS;   
    else if (!strcmp(item[i], "+dontkickops"))
      chan->status |= CHAN_DONTKICKOPS;
    else if (!strcmp(item[i], "-dontkickops"))
      chan->status &= ~CHAN_DONTKICKOPS;
    else if (!strcmp(item[i], "+wasoptest"))
      chan->status |= CHAN_WASOPTEST;
    else if (!strcmp(item[i], "-wasoptest"))
      chan->status &= ~CHAN_WASOPTEST;
    else if (!strcmp(item[i], "+inactive"))
      chan->status |= CHAN_INACTIVE;
    else if (!strcmp(item[i], "-inactive"))
      chan->status&= ~CHAN_INACTIVE;
    else if (!strcmp(item[i], "+statuslog"))
      chan->status |= CHAN_LOGSTATUS;
    else if (!strcmp(item[i], "-statuslog"))
      chan->status &= ~CHAN_LOGSTATUS;
    else if (!strcmp(item[i], "+stopnethack"))
      chan->status |= CHAN_STOPNETHACK;
    else if (!strcmp(item[i], "-stopnethack"))
      chan->status &= ~CHAN_STOPNETHACK;
    else if (!strcmp(item[i], "+revenge"))
      chan->status |= CHAN_REVENGE;
    else if (!strcmp(item[i], "-revenge"))
      chan->status &= ~CHAN_REVENGE;
    else if (!strcmp(item[i], "+revengebot"))
      chan->status |= CHAN_REVENGEBOT;
    else if (!strcmp(item[i], "-revengebot"))
      chan->status &= ~CHAN_REVENGEBOT;
    else if (!strcmp(item[i], "+secret"))
      chan->status |= CHAN_SECRET;
    else if (!strcmp(item[i], "-secret"))
      chan->status &= ~CHAN_SECRET;
    else if (!strcmp(item[i], "+shared"))
      chan->status |= CHAN_SHARED;
    else if (!strcmp(item[i], "-shared"))
      chan->status &= ~CHAN_SHARED;
    else if (!strcmp(item[i], "+autovoice"))
      chan->status |= CHAN_AUTOVOICE;
    else if (!strcmp(item[i], "-autovoice"))
      chan->status &= ~CHAN_AUTOVOICE;
    else if (!strcmp(item[i], "+cycle"))
      chan->status |= CHAN_CYCLE;
    else if (!strcmp(item[i], "-cycle"))
      chan->status &= ~CHAN_CYCLE;
    else if (!strcmp(item[i], "+seen"))
      chan->status |= CHAN_SEEN;
    else if (!strcmp(item[i], "-seen"))
      chan->status &= ~CHAN_SEEN;
    else if (!strcmp(item[i], "+dynamicexempts"))
      chan->ircnet_status|= CHAN_DYNAMICEXEMPTS;
    else if (!strcmp(item[i], "-dynamicexempts"))
      chan->ircnet_status&= ~CHAN_DYNAMICEXEMPTS;
    else if (!strcmp(item[i], "-userexempts"))
      chan->ircnet_status|= CHAN_NOUSEREXEMPTS;
    else if (!strcmp(item[i], "+userexempts"))
      chan->ircnet_status&= ~CHAN_NOUSEREXEMPTS;
    else if (!strcmp(item[i], "+dynamicinvites"))
      chan->ircnet_status|= CHAN_DYNAMICINVITES;
    else if (!strcmp(item[i], "-dynamicinvites"))
      chan->ircnet_status&= ~CHAN_DYNAMICINVITES;
    else if (!strcmp(item[i], "-userinvites"))
      chan->ircnet_status|= CHAN_NOUSERINVITES;
    else if (!strcmp(item[i], "+userinvites"))
      chan->ircnet_status&= ~CHAN_NOUSERINVITES;
    else if (!strncmp(item[i], "flood-", 6)) {
      int *pthr = 0, *ptime;
      char *p;
      
      if (!strcmp(item[i] + 6, "chan")) {
	pthr = &chan->flood_pub_thr;
	ptime = &chan->flood_pub_time;
      } else if (!strcmp(item[i] + 6, "join")) {
	pthr = &chan->flood_join_thr;
	ptime = &chan->flood_join_time;
      } else if (!strcmp(item[i] + 6, "ctcp")) {
	pthr = &chan->flood_ctcp_thr;
	ptime = &chan->flood_ctcp_time;
      } else if (!strcmp(item[i] + 6, "kick")) {
	pthr = &chan->flood_kick_thr;
	ptime = &chan->flood_kick_time;
      } else if (!strcmp(item[i] + 6, "deop")) {
	pthr = &chan->flood_deop_thr;
	ptime = &chan->flood_deop_time;
      } else {
	if (irp)
	  Tcl_AppendResult(irp, "illegal channel flood type: ", item[i], NULL);
	return TCL_ERROR;
      }
      i++;
      if (i >= items) {
	if (irp)
	  Tcl_AppendResult(irp, item[i - 1], " needs argument", NULL);
	return TCL_ERROR;
      }
      p = strchr(item[i], ':');
      if (p) {
	*p++ = 0;
	*pthr = atoi(item[i]);
	*ptime = atoi(p);
	*--p = ':';
      } else {
	*pthr = atoi(item[i]);
	*ptime = 1;
      }
    } else {
      if (irp && item[i][0]) /* ignore "" */
	Tcl_AppendResult(irp, "illegal channel option: ", item[i], NULL);
	x++;
    }
  }
  if (((oldstatus ^ chan->status) & CHAN_INACTIVE) && module_find("irc", 0, 0)) {
    if (channel_inactive(chan) && (chan->status & (CHAN_ACTIVE + CHAN_PEND)))
      dprintf(DP_SERVER, "PART %s\n", chan->name);
    if (!channel_inactive(chan) && !(chan->status & (CHAN_ACTIVE + CHAN_PEND)))
      dprintf(DP_SERVER, "JOIN %s %s\n", chan->name, chan->key_prot);
  }
  if ((oldstatus ^ chan->status) & (CHAN_ENFORCEBANS+CHAN_OPONJOIN+CHAN_BITCH+CHAN_AUTOVOICE)) {
    if ((me = module_find("irc", 0, 0))) {
      (me->funcs[15])(chan, 1);
    }
  }
  if (x > 0) { return TCL_ERROR; }
  return TCL_OK;
}

static int tcl_banlist STDVAR
{
  struct chanset_t *chan;
  struct banrec *u;
  char ts[21], ts1[21], ts2[21];
  char *list[6], *p;

  BADARGS(1, 2, " ?channel?");
  if (argc == 2) {
    chan = findchan(argv[1]);
    if (chan == NULL) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
      return TCL_ERROR;
    }
    u = chan->bans;
  } else
    u = global_bans;
  if (u == NULL)
    return TCL_OK;
  for (; u; u = u->next) {
    list[0] = u->banmask;
    list[1] = u->desc;
    sprintf(ts, "%lu", u->expire);
    list[2] = ts;
    sprintf(ts1, "%lu", u->added);
    list[3] = ts1;
    sprintf(ts2, "%lu", u->lastactive);
    list[4] = ts2;
    list[5] = u->user;
    p = Tcl_Merge(6, list);
    Tcl_AppendElement(irp, p);
    n_free(p, "", 0);
  }
  return TCL_OK;
}

static int tcl_exemptlist STDVAR
{
  struct chanset_t *chan;
  struct exemptrec *u;
  char ts[21], ts1[21], ts2[21];
  char *list[6], *p;
  
  BADARGS(1, 2, " ?channel?");
  if (argc == 2) {
    chan = findchan(argv[1]);
    if (chan == NULL) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
      return TCL_ERROR;
    }
    u = chan->exempts;
  } else
    u = global_exempts;
  if (u == NULL)
    return TCL_OK;
  for (;u;u=u->next) {
    list[0] = u->exemptmask;
    list[1] = u->desc;
    sprintf(ts,"%lu",u->expire);
    list[2] = ts;
    sprintf(ts1,"%lu",u->added);
    list[3] = ts1;
    sprintf(ts2,"%lu",u->lastactive);
    list[4] = ts2;
    list[5] = u->user;
    p = Tcl_Merge(6, list);
    Tcl_AppendElement(irp, p);
    n_free(p, "", 0);
  }
  return TCL_OK;
}

static int tcl_invitelist STDVAR
{
  struct chanset_t *chan;
  struct inviterec *u;
  char ts[21], ts1[21], ts2[21];
  char *list[6], *p;
  
  BADARGS(1, 2, " ?channel?");
  if (argc == 2) {
    chan = findchan(argv[1]);
    if (chan == NULL) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
      return TCL_ERROR;
    }
    u = chan->invites;
  } else
    u = global_invites;
  if (u == NULL)
    return TCL_OK;
  for (;u;u=u->next) {
    list[0] = u->invitemask;
    list[1] = u->desc;
    sprintf(ts,"%lu",u->expire);
    list[2] = ts;
    sprintf(ts1,"%lu",u->added);
    list[3] = ts1;
    sprintf(ts2,"%lu",u->lastactive);
    list[4] = ts2;
    list[5] = u->user;
    p = Tcl_Merge(6, list);
    Tcl_AppendElement(irp, p);
    n_free(p, "", 0);
  }
  return TCL_OK;
}


static int tcl_channels STDVAR
{
  struct chanset_t *chan;

  BADARGS(1, 1, "");
  chan = chanset;
  while (chan != NULL) {
    Tcl_AppendElement(irp, chan->name);
    chan = chan->next;
  } return TCL_OK;
}

static int tcl_savechannels STDVAR
{
  context;
  BADARGS(1, 1, "");
  if (!chanfile[0]) {
    Tcl_AppendResult(irp, "no channel file");
    return TCL_ERROR;
  }
  write_channels();
  return TCL_OK;
}

static int tcl_loadchannels STDVAR
{
  context;
  BADARGS(1, 1, "");
  if (!chanfile[0]) {
    Tcl_AppendResult(irp, "no channel file");
    return TCL_ERROR;
  }
  setstatic = 0;
  read_channels(1);
  return TCL_OK;
}

static int tcl_validchan STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan == NULL)
    Tcl_AppendResult(irp, "0", NULL);
  else
    Tcl_AppendResult(irp, "1", NULL);
  return TCL_OK;
}

static int tcl_isdynamic STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");
  chan = findchan(argv[1]);
  if (chan != NULL)
    if (!channel_static(chan)) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_getchaninfo STDVAR
{
  char s[161];
  struct userrec *u;

  BADARGS(3, 3, " handle channel");
  u = get_user_by_handle(userlist, argv[1]);
  if (!u || (u->flags & USER_BOT))
    return TCL_OK;
  get_handle_chaninfo(argv[1], argv[2], s);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_setchaninfo STDVAR
{
  struct chanset_t *chan;

  BADARGS(4, 4, " handle channel info");
  chan = findchan(argv[2]);
  if (chan == NULL) { 
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if (!strcasecmp(argv[3], "none")) {
    set_handle_chaninfo(userlist, argv[1], argv[2], NULL);
    return TCL_OK;
  }
  set_handle_chaninfo(userlist, argv[1], argv[2], argv[3]);
  return TCL_OK;
}

static int tcl_setlaston STDVAR
{
  time_t t = now;
  struct userrec *u;

  BADARGS(2, 4, " handle ?channel? ?timestamp?");
  u = get_user_by_handle(userlist, argv[1]);
  if (!u) {
    Tcl_AppendResult(irp, "No such user: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if (argc == 4)
    t = (time_t) atoi(argv[3]);
  if (argc == 3 && ((argv[2][0] != '#') && (argv[2][0] != '&')))
    t = (time_t) atoi(argv[2]);
  if (argc == 2 || (argc == 3 && ((argv[2][0] != '#') && (argv[2][0] != '&'))))
    set_handle_laston("*", u, t);
  else
    set_handle_laston(argv[2], u, t);
  return TCL_OK;
}

static int tcl_addchanrec STDVAR
{
  struct userrec *u;

  context;
  BADARGS(3, 3, " handle channel");
  u = get_user_by_handle(userlist, argv[1]);
  if (!u) {
    Tcl_AppendResult(irp, "0", NULL);
    return TCL_OK;
  }
  if (!findchan(argv[2])) {
    Tcl_AppendResult(irp, "0", NULL);
    return TCL_OK;
  }
  if (get_chanrec(u, argv[2]) != NULL) {
    Tcl_AppendResult(irp, "0", NULL);
    return TCL_OK;
  }
  add_chanrec(u, argv[2]);
  Tcl_AppendResult(irp, "1", NULL);
  return TCL_OK;
}

static int tcl_delchanrec STDVAR
{
  struct userrec *u;

  context;
  BADARGS(3, 3, " handle channel");
  u = get_user_by_handle(userlist, argv[1]);
  if (!u) {
    Tcl_AppendResult(irp, "0", NULL);
    return TCL_OK;
  }
  if (get_chanrec(u, argv[2]) == NULL) {
    Tcl_AppendResult(irp, "0", NULL);
    return TCL_OK;
  }
  del_chanrec(u, argv[2]);
  Tcl_AppendResult(irp, "1", NULL);
  return TCL_OK;
}

/* initialize out the channel record */
static void init_channel(struct chanset_t *chan)
{
  chan->channel.maxmembers = (-1);
  chan->channel.mode = 0;
  chan->channel.members = 0;
  chan->channel.key = (char *) nmalloc(1);
  chan->channel.key[0] = 0;
  chan->channel.ban = (banlist *) nmalloc(sizeof(banlist));
  chan->channel.ban->ban = (char *) nmalloc(1);
  chan->channel.ban->ban[0] = 0;
  chan->channel.ban->who = NULL;
  chan->channel.ban->next = NULL;
  chan->channel.exempt = (exemptlist *) nmalloc(sizeof(exemptlist));
  chan->channel.exempt->exempt = (char *) nmalloc(1);
  chan->channel.exempt->exempt[0] = 0;
  chan->channel.exempt->who = NULL;
  chan->channel.exempt->next = NULL;
  chan->channel.invite = (invitelist *) nmalloc(sizeof(invitelist));
  chan->channel.invite->invite = (char *) nmalloc(1);
  chan->channel.invite->invite[0] = 0;
  chan->channel.invite->who = NULL;
  chan->channel.invite->next = NULL;
  chan->channel.member = (memberlist *) nmalloc(sizeof(memberlist));
  chan->channel.member->nick[0] = 0;
  chan->channel.member->next = NULL;
  chan->channel.topic = NULL;
}

/* clear out channel data from memory */
static void clear_channel(struct chanset_t *chan, int reset)
{
  memberlist *m, *m1;
  banlist *b, *b1;
  exemptlist *e, *e1;
  invitelist *inv, *inv1;

  nfree(chan->channel.key);
  if (chan->channel.topic)
    nfree(chan->channel.topic);
  m = chan->channel.member;
  while (m != NULL) {
    m1 = m->next;
    nfree(m);
    m = m1;
  }
  b = chan->channel.ban;
  while (b != NULL) {
    b1 = b->next;
    if (b->ban[0])
      nfree(b->who);
    nfree(b->ban);
    nfree(b);
    b = b1;
  }
  e = chan->channel.exempt;
  while (e != NULL) {
    e1 = e->next;
    if (e->exempt[0])
      nfree(e->who);
    nfree(e->exempt);
    nfree(e);
    e = e1;
  }
  inv = chan->channel.invite;
  while (inv != NULL) {
    inv1 = inv->next;
    if (inv->invite[0])
      nfree(inv->who);
    nfree(inv->invite);
    nfree(inv);
    inv = inv1;
  }
  if (reset)
    init_channel(chan);
}

/* create new channel and parse commands */
static int tcl_channel_add(Tcl_Interp * irp, char *newname, char *options)
{
  struct chanset_t *chan;
  int items;
  char **item;
  int ret = TCL_OK;
  int join = 0;
  char buf[2048];

  if ((newname[0] != '#') && (newname[0] != '&'))
    return TCL_ERROR;
  context;
  simple_sprintf(buf, "chanmode \"%s\" ", glob_chanmode);
  strncat(buf, glob_chanset, 2047 - strlen(buf));
  strncat(buf, options, 2047 - strlen(buf));
  buf[2047] = 0;
  /* drummer: Tcl8.0: Tcl_SplitList does not use irp, can be NULL */
  if (Tcl_SplitList(NULL, buf, &items, &item) != TCL_OK)
    return TCL_ERROR;
  context;
  if ((chan = findchan(newname))) {
    /* already existing channel, maybe a reload of the channel file */
    chan->status &= ~CHAN_FLAGGED;	/* don't delete me! :) */
  } else {
    chan = (struct chanset_t *) nmalloc(sizeof(struct chanset_t));

    /* hells bells, why set *every* variable to 0 when we have bzero ? */
    bzero(chan, sizeof(struct chanset_t));

    chan->limit_prot = (-1);
    chan->limit = (-1);
    chan->flood_pub_thr = gfld_chan_thr;
    chan->flood_pub_time = gfld_chan_time;
    chan->flood_ctcp_thr = gfld_ctcp_thr;
    chan->flood_ctcp_time = gfld_ctcp_time;
    chan->flood_join_thr = gfld_join_thr;
    chan->flood_join_time = gfld_join_time;
    chan->flood_deop_thr = gfld_deop_thr;
    chan->flood_deop_time = gfld_deop_time;
    chan->flood_kick_thr = gfld_kick_thr;
    chan->flood_kick_time = gfld_kick_time;
    strncpy(chan->name, newname, 80);
    chan->name[80] = 0;
    /* initialize chan->channel info */
    init_channel(chan);
    list_append((struct list_type **) &chanset, (struct list_type *) chan);
    /* channel name is stored in xtra field for sharebot stuff */
    join = 1;
  }
  if (setstatic)
    chan->status |= CHAN_STATIC;
  /* if chan_hack is set, we're loading the userfile. Ignore errors while
   * reading userfile and just return TCL_OK. This is for compatability
   * if a user goes back to an eggdrop that no-longer supports certain
   * (channel) options. */
  if ((tcl_channel_modify(irp, chan, items, item) != TCL_OK) && !chan_hack) {
    ret = TCL_ERROR;
  }
  if (join && !channel_inactive(chan) && module_find("irc", 0, 0))
    dprintf(DP_SERVER, "JOIN %s %s\n", chan->name, chan->key_prot);
  return ret; 
}

static tcl_cmds channels_cmds[] =
{
  {"killban", tcl_killban},
  {"killchanban", tcl_killchanban},
  {"isbansticky", tcl_isbansticky},
  {"isban", tcl_isban},
  {"ispermban", tcl_ispermban},
  {"matchban", tcl_matchban},
  {"newchanban", tcl_newchanban},
  {"newban", tcl_newban},
  {"killexempt", tcl_killexempt},
  {"killchanexempt", tcl_killchanexempt},
  {"isexemptsticky", tcl_isexemptsticky},
  {"isexempt", tcl_isexempt},
  {"ispermexempt", tcl_ispermexempt},
  {"matchexempt", tcl_matchexempt},
  {"newchanexempt", tcl_newchanexempt},
  {"newexempt", tcl_newexempt},
  {"killinvite", tcl_killinvite},
  {"killchaninvite", tcl_killchaninvite},
  {"isinvitesticky", tcl_isinvitesticky},
  {"isinvite", tcl_isinvite},
  {"isperminvite", tcl_isperminvite},
  {"matchinvite", tcl_matchinvite},
  {"newchaninvite", tcl_newchaninvite},
  {"newinvite", tcl_newinvite},
  {"channel", tcl_channel},
  {"channels", tcl_channels},
  {"exemptlist", tcl_exemptlist},
  {"invitelist", tcl_invitelist},
  {"banlist", tcl_banlist},
  {"savechannels", tcl_savechannels},
  {"loadchannels", tcl_loadchannels},
  {"validchan", tcl_validchan},
  {"isdynamic", tcl_isdynamic},
  {"getchaninfo", tcl_getchaninfo},
  {"setchaninfo", tcl_setchaninfo},
  {"setlaston", tcl_setlaston},
  {"addchanrec", tcl_addchanrec},
  {"delchanrec", tcl_delchanrec},
  {"stick", tcl_stick},
  {"unstick", tcl_unstick},
  {0, 0}
};
