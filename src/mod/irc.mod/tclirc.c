/*
 * tclirc.c -- part of irc.mod
 *
 * $Id: tclirc.c,v 1.59 2011/02/13 14:19:34 simple Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2011 Eggheads Development Team
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

/* Streamlined by answer.
 */
static int tcl_chanlist STDVAR
{
  char nuh[1024];
  int f;
  memberlist *m;
  struct chanset_t *chan;
  struct flag_record plus = { FR_CHAN | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0 },
                     minus = { FR_CHAN | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0},
                     user = { FR_CHAN | FR_GLOBAL | FR_BOT, 0, 0, 0, 0, 0 };

  BADARGS(2, 3, " channel ?flags?");

  chan = findchan_by_dname(argv[1]);
  if (!chan) {
    Tcl_AppendResult(irp, "invalid channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if (argc == 2) {
    /* No flag restrictions so just whiz it thru quick */
    for (m = chan->channel.member; m && m->nick[0]; m = m->next)
      Tcl_AppendElement(irp, m->nick);
    return TCL_OK;
  }
  break_down_flags(argv[2], &plus, &minus);
  f = (minus.global || minus.udef_global || minus.chan || minus.udef_chan ||
       minus.bot);
  /* Return empty set if asked for flags but flags don't exist */
  if (!plus.global && !plus.udef_global && !plus.chan && !plus.udef_chan &&
      !plus.bot && !f)
    return TCL_OK;
  minus.match = plus.match ^ (FR_AND | FR_OR);

  for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
    if (!m->user) {
      egg_snprintf(nuh, sizeof nuh, "%s!%s", m->nick, m->userhost);
      m->user = get_user_by_host(nuh);
    }
    get_user_flagrec(m->user, &user, argv[1]);
    user.match = plus.match;
    if (flagrec_eq(&plus, &user)) {
      if (!f || !flagrec_eq(&minus, &user))
        Tcl_AppendElement(irp, m->nick);
    }
  }
  return TCL_OK;
}

static int tcl_botisop STDVAR
{
  struct chanset_t *chan, *thechan = NULL;

  BADARGS(1, 2, " ?channel?");

  if (argc > 1) {
    chan = findchan_by_dname(argv[1]);
    thechan = chan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[1], NULL);
      return TCL_ERROR;
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if (me_op(chan)) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_botishalfop STDVAR
{
  struct chanset_t *chan, *thechan = NULL;

  BADARGS(1, 2, " ?channel?");

  if (argc > 1) {
    chan = findchan_by_dname(argv[1]);
    thechan = chan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[1], NULL);
      return TCL_ERROR;
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if (me_halfop(chan)) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_ischanjuped STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");

  chan = findchan_by_dname(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if (channel_juped(chan))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_botisvoice STDVAR
{
  struct chanset_t *chan, *thechan = NULL;
  memberlist *mx;

  BADARGS(1, 2, " ?channel?");

  if (argc > 1) {
    chan = findchan_by_dname(argv[1]);
    thechan = chan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[1], NULL);
      return TCL_ERROR;
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if ((mx = ismember(chan, botname)) && chan_hasvoice(mx)) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_botonchan STDVAR
{
  struct chanset_t *chan, *thechan = NULL;

  BADARGS(1, 2, " ?channel?");

  if (argc > 1) {
    chan = findchan_by_dname(argv[1]);
    thechan = chan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[1], NULL);
      return TCL_ERROR;
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if (ismember(chan, botname)) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_isop STDVAR
{
  struct chanset_t *chan, *thechan = NULL;
  memberlist *mx;

  BADARGS(2, 3, " nick ?channel?");

  if (argc > 2) {
    chan = findchan_by_dname(argv[2]);
    thechan = chan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if ((mx = ismember(chan, argv[1])) && chan_hasop(mx)) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_ishalfop STDVAR
{
  struct chanset_t *chan, *thechan = NULL;
  memberlist *mx;

  BADARGS(2, 3, " nick ?channel?");

  if (argc > 2) {
    chan = findchan_by_dname(argv[2]);
    thechan = chan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if ((mx = ismember(chan, argv[1])) && chan_hashalfop(mx)) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_isvoice STDVAR
{
  struct chanset_t *chan, *thechan = NULL;
  memberlist *mx;

  BADARGS(2, 3, " nick ?channel?");

  if (argc > 2) {
    chan = findchan_by_dname(argv[2]);
    thechan = chan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if ((mx = ismember(chan, argv[1])) && chan_hasvoice(mx)) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_wasop STDVAR
{
  struct chanset_t *chan;
  memberlist *mx;

  BADARGS(3, 3, " nick channel");

  chan = findchan_by_dname(argv[2]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if ((mx = ismember(chan, argv[1])) && chan_wasop(mx))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_washalfop STDVAR
{
  struct chanset_t *chan;
  memberlist *mx;

  BADARGS(3, 3, " nick channel");

  chan = findchan_by_dname(argv[2]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  if ((mx = ismember(chan, argv[1])) && chan_washalfop(mx))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_onchan STDVAR
{
  struct chanset_t *chan, *thechan = NULL;

  BADARGS(2, 3, " nickname ?channel?");

  if (argc > 2) {
    chan = findchan_by_dname(argv[2]);
    thechan = chan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    if (ismember(chan, argv[1])) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_handonchan STDVAR
{
  char nuh[1024];
  struct chanset_t *chan, *thechan = NULL;
  memberlist *m;

  BADARGS(2, 3, " handle ?channel?");

  if (argc > 2) {
    chan = findchan_by_dname(argv[2]);
    thechan = chan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
      if (!m->user) {
        egg_snprintf(nuh, sizeof nuh, "%s!%s", m->nick, m->userhost);
        m->user = get_user_by_host(nuh);
      }
      if (m->user && !rfc_casecmp(m->user->handle, argv[1])) {
        Tcl_AppendResult(irp, "1", NULL);
        return TCL_OK;
      }
    }
    chan = chan->next;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_ischanban STDVAR
{
  struct chanset_t *chan;

  BADARGS(3, 3, " ban channel");

  chan = findchan_by_dname(argv[2]);
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

  chan = findchan_by_dname(argv[2]);
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

  chan = findchan_by_dname(argv[2]);
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
  struct chanset_t *chan, *thechan = NULL;
  memberlist *m;

  BADARGS(2, 3, " nickname ?channel?");

  if (argc > 2) {
    chan = findchan_by_dname(argv[2]);
    thechan = chan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    m = ismember(chan, argv[1]);
    if (m) {
      Tcl_AppendResult(irp, m->userhost, NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  return TCL_OK;
}

static int tcl_onchansplit STDVAR
{
  struct chanset_t *chan, *thechan = NULL;
  memberlist *m;

  BADARGS(2, 3, " nickname ?channel?");

  if (argc > 2) {
    chan = findchan_by_dname(argv[2]);
    thechan = chan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    m = ismember(chan, argv[1]);
    if (m && chan_issplit(m)) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_maskhost STDVAR
{
  char *new;

  BADARGS(2, 3, " nick!user@host ?type?");

  new = nmalloc(strlen(argv[1]) + 5);
  if (argc == 3)
    maskaddr(argv[1], new, atoi(argv[2]));
  else
    maskban(argv[1], new);
  Tcl_AppendResult(irp, new, NULL);
  nfree(new);
  return TCL_OK;
}

static int tcl_getchanidle STDVAR
{
  memberlist *m;
  struct chanset_t *chan;
  char s[20];
  int x;

  BADARGS(3, 3, " nickname channel");

  if (!(chan = findchan_by_dname(argv[2]))) {
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

static inline int tcl_chanmasks(masklist *m, Tcl_Interp *irp)
{
  char work[20], *p;
  EGG_CONST char *list[3];

  for (; m && m->mask && m->mask[0]; m = m->next) {
    list[0] = m->mask;
    list[1] = m->who;
    simple_sprintf(work, "%d", now - m->timer);
    list[2] = work;
    p = Tcl_Merge(3, list);
    Tcl_AppendElement(irp, p);
    Tcl_Free((char *) p);
  }
  return TCL_OK;
}

static int tcl_chanbans STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");

  chan = findchan_by_dname(argv[1]);
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

  chan = findchan_by_dname(argv[1]);
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

  chan = findchan_by_dname(argv[1]);
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

  chan = findchan_by_dname(argv[1]);
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

  chan = findchan_by_dname(argv[2]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
    return TCL_ERROR;
  }
  m = ismember(chan, argv[1]);

  if (m == NULL) {
    Tcl_AppendResult(irp, argv[1], " is not on ", argv[2], NULL);
    return TCL_ERROR;
  }
  sprintf(s, "%lu", (unsigned long) m->joined);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_channame2dname STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel-name");

  chan = findchan(argv[1]);
  if (chan) {
    Tcl_AppendResult(irp, chan->dname, NULL);
    return TCL_OK;
  } else {
    Tcl_AppendResult(irp, "invalid channel-name: ", argv[1], NULL);
    return TCL_ERROR;
  }
}

static int tcl_chandname2name STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel-dname");

  chan = findchan_by_dname(argv[1]);
  if (chan) {
    Tcl_AppendResult(irp, chan->name, NULL);
    return TCL_OK;
  } else {
    Tcl_AppendResult(irp, "invalid channel-dname: ", argv[1], NULL);
    return TCL_ERROR;
  }
}

static int tcl_flushmode STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");

  chan = findchan_by_dname(argv[1]);
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

  chan = findchan_by_dname(argv[1]);
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

  chan = findchan_by_dname(argv[1]);
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

  chan = findchan_by_dname(argv[1]);
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

  chan = findchan_by_dname(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel ", argv[1], NULL);
    return TCL_ERROR;
  }
  resetinvites(chan);
  return TCL_OK;
}

static int tcl_resetchanidle STDVAR
{
  memberlist *m;
  struct chanset_t *chan;

  BADARGS(2, 3, " ?nick? channel");

  chan = findchan_by_dname((argc == 2) ? argv[1] : argv[2]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel ",
                     (argc == 2) ? argv[1] : argv[2], NULL);
    return TCL_ERROR;
  }

  if (argc == 2)
    for (m = chan->channel.member; m; m = m->next)
      m->last = now;
  else {
    if (!(m = ismember(chan, argv[1]))) {
      Tcl_AppendResult(irp, argv[1], " is not on ", argv[2], NULL);
      return TCL_ERROR;
    }
    m->last = now;
  }
  return TCL_OK;
}

static int tcl_resetchanjoin STDVAR
{
  memberlist *m;
  struct chanset_t *chan;

  BADARGS(2, 3, " ?nick? channel");

  chan = findchan_by_dname((argc == 2) ? argv[1] : argv[2]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel ",
                     (argc == 2) ? argv[1] : argv[2], NULL);
    return TCL_ERROR;
  }

  if (argc == 2)
    for (m = chan->channel.member; m; m = m->next)
      m->joined = now;
  else {
    if (!(m = ismember(chan, argv[1]))) {
      Tcl_AppendResult(irp, argv[1], " is not on ", argv[2], NULL);
      return TCL_ERROR;
    }
    m->joined = now;
  }
  return TCL_OK;
}

static int tcl_resetchan STDVAR
{
  char *c;
  int flags = 0;
  struct chanset_t *chan;

  BADARGS(2, 3, " channel ?flags?");

  chan = findchan_by_dname(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel ", argv[1], NULL);
    return TCL_ERROR;
  }

  if (argc == 2) {
    reset_chan_info(chan, CHAN_RESETALL);
    return TCL_OK;
  }
  for (c = argv[2]; *c; c++) {
    switch(*c) {
    case 'w':
      flags |= CHAN_RESETWHO;
      break;
    case 'm':
      flags |= CHAN_RESETMODES;
      break;
    case 'b':
      flags |= CHAN_RESETBANS;
      break;
    case 'e':
      flags |= CHAN_RESETEXEMPTS;
      break;
    case 'I':
      flags |= CHAN_RESETINVITED;
      break;
    case 't':
      flags |= CHAN_RESETTOPIC;
      break;
    default:
      Tcl_AppendResult(irp, "invalid reset flags: ", argv[2], NULL);
      return TCL_ERROR;
    }
  }
  if (flags)
    reset_chan_info(chan, flags);
  return TCL_OK;
}

static int tcl_topic STDVAR
{
  struct chanset_t *chan;

  BADARGS(2, 2, " channel");

  chan = findchan_by_dname(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "invalid channel ", argv[1], NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(irp, chan->channel.topic, NULL);
  return TCL_OK;
}

static int tcl_hand2nick STDVAR
{
  char nuh[1024];
  memberlist *m;
  struct chanset_t *chan, *thechan = NULL;

  BADARGS(2, 3, " handle ?channel?");

  if (argc > 2) {
    chan = findchan_by_dname(argv[2]);
    thechan = chan;
    if (chan == NULL) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
      if (!m->user && !m->tried_getuser) {
        egg_snprintf(nuh, sizeof nuh, "%s!%s", m->nick, m->userhost);
        m->tried_getuser = 1;
        m->user = get_user_by_host(nuh);
      }
      if (m->user && !rfc_casecmp(m->user->handle, argv[1])) {
        Tcl_AppendResult(irp, m->nick, NULL);
        return TCL_OK;
      }
    }
    chan = chan->next;
  }
  return TCL_OK;
}

static int tcl_nick2hand STDVAR
{
  char nuh[1024];
  memberlist *m;
  struct chanset_t *chan, *thechan = NULL;

  BADARGS(2, 3, " nick ?channel?");

  if (argc > 2) {
    chan = findchan_by_dname(argv[2]);
    thechan = chan;
    if (chan == NULL) {
      Tcl_AppendResult(irp, "invalid channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else
    chan = chanset;

  while (chan && (thechan == NULL || thechan == chan)) {
    m = ismember(chan, argv[1]);
    if (m) {
      if (!m->user) {
        egg_snprintf(nuh, sizeof nuh, "%s!%s", m->nick, m->userhost);
        m->user = get_user_by_host(nuh);
      }
      Tcl_AppendResult(irp, m->user ? m->user->handle : "*", NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  return TCL_OK;
}

static int tcl_putkick STDVAR
{
  struct chanset_t *chan;
  int k = 0, l;
  char kicknick[512], *nick, *p, *comment = NULL;
  memberlist *m;

  BADARGS(3, 4, " channel nick?s? ?comment?");

  chan = findchan_by_dname(argv[1]);
  if (chan == NULL) {
    Tcl_AppendResult(irp, "illegal channel: ", argv[1], NULL);
    return TCL_ERROR;
  }
  if (argc == 4)
    comment = argv[3];
  else
    comment = "";
  if (!me_op(chan) && !me_halfop(chan)) {
    Tcl_AppendResult(irp, "need op or halfop", NULL);
    return TCL_ERROR;
  }

  kicknick[0] = 0;
  p = argv[2];
  /* Loop through all given nicks */
  while (p) {
    nick = p;
    p = strchr(nick, ',');      /* Search for beginning of next nick */
    if (p) {
      *p = 0;
      p++;
    }

    m = ismember(chan, nick);
    if (!me_op(chan) && !(me_halfop(chan) && !chan_hasop(m))) {
      Tcl_AppendResult(irp, "need op", NULL);
      return TCL_ERROR;
    }
    if (!m)
      continue;                 /* Skip non-existant nicks */
    m->flags |= SENTKICK;       /* Mark as pending kick */
    if (kicknick[0])
      strcat(kicknick, ",");
    strcat(kicknick, nick);     /* Add to local queue */
    k++;

    /* Check if we should send the kick command yet */
    l = strlen(chan->name) + strlen(kicknick) + strlen(comment);
    if (((kick_method != 0) && (k == kick_method)) || (l > 480)) {
      dprintf(DP_SERVER, "KICK %s %s :%s\n", chan->name, kicknick, comment);
      k = 0;
      kicknick[0] = 0;
    }
  }
  /* Clear out all pending kicks in our local kick queue */
  if (k > 0)
    dprintf(DP_SERVER, "KICK %s %s :%s\n", chan->name, kicknick, comment);
  return TCL_OK;
}

static tcl_cmds tclchan_cmds[] = {
  {"chanlist",       tcl_chanlist},
  {"botisop",        tcl_botisop},
  {"botishalfop",    tcl_botishalfop},
  {"botisvoice",     tcl_botisvoice},
  {"isop",           tcl_isop},
  {"wasop",          tcl_wasop},
  {"ishalfop",       tcl_ishalfop},
  {"washalfop",      tcl_washalfop},
  {"isvoice",        tcl_isvoice},
  {"onchan",         tcl_onchan},
  {"handonchan",     tcl_handonchan},
  {"ischanban",      tcl_ischanban},
  {"ischanexempt",   tcl_ischanexempt},
  {"ischaninvite",   tcl_ischaninvite},
  {"ischanjuped",    tcl_ischanjuped},
  {"getchanhost",    tcl_getchanhost},
  {"onchansplit",    tcl_onchansplit},
  {"maskhost",       tcl_maskhost},
  {"getchanidle",    tcl_getchanidle},
  {"chanbans",       tcl_chanbans},
  {"chanexempts",    tcl_chanexempts},
  {"chaninvites",    tcl_chaninvites},
  {"hand2nick",      tcl_hand2nick},
  {"nick2hand",      tcl_nick2hand},
  {"getchanmode",    tcl_getchanmode},
  {"getchanjoin",    tcl_getchanjoin},
  {"flushmode",      tcl_flushmode},
  {"pushmode",       tcl_pushmode},
  {"resetbans",      tcl_resetbans},
  {"resetexempts",   tcl_resetexempts},
  {"resetinvites",   tcl_resetinvites},
  {"resetchanidle",  tcl_resetchanidle},
  {"resetchanjoin",  tcl_resetchanjoin},
  {"resetchan",      tcl_resetchan},
  {"topic",          tcl_topic},
  {"botonchan",      tcl_botonchan},
  {"putkick",        tcl_putkick},
  {"channame2dname", tcl_channame2dname},
  {"chandname2name", tcl_chandname2name},
  {NULL,             NULL}
};
