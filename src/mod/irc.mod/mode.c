/*
 * mode.c -- part of irc.mod
 *   queuing and flushing mode changes made by the bot
 *   channel mode changes and the bot's reaction to them
 *   setting and getting the current wanted channel modes
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2025 Eggheads Development Team
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

#define PLUS    0x01
#define MINUS   0x02
#define CHOP    0x04
#define BAN     0x08
#define VOICE   0x10
#define EXEMPT  0x20
#define INVITE  0x40
#define CHHOP   0x80

static struct flag_record user = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };
static struct flag_record victim = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

/* This is called after a mode bind is processed. It will check if the
 * channel still exists and will refresh the user and victim flag records,
 * in case users were also modified.
 */
static struct chanset_t *modebind_refresh(char *chname,
                                          char *usrhost, struct flag_record *usr,
                                          char *vcrhost, struct flag_record *vcr)
{
  struct userrec *u;
  struct chanset_t *chan;

  if (!chname || !(chan = findchan(chname)))
    return NULL;
  if (usrhost) {
    u = lookup_user_record(NULL, NULL, usrhost); // TODO: get account from somewhere
    get_user_flagrec(u, usr, chan->dname);
  }
  if (vcrhost) {
    u = lookup_user_record(NULL, NULL, vcrhost); // TODO: get account from somewhere
    get_user_flagrec(u, vcr, chan->dname);
  }
  return chan;
}

static void flush_mode(struct chanset_t *chan, int pri)
{
  char *p, out[512], post[512];
  size_t postsize = sizeof(post);
  int i, plus = 2;              /* 0 = '-', 1 = '+', 2 = none */

  p = out;
  post[0] = 0, postsize--;

  if (chan->mns[0]) {
    *p++ = '-', plus = 0;
    for (i = 0; i < strlen(chan->mns); i++)
      *p++ = chan->mns[i];
    chan->mns[0] = 0;
  }

  if (chan->pls[0]) {
    *p++ = '+', plus = 1;
    for (i = 0; i < strlen(chan->pls); i++)
      *p++ = chan->pls[i];
    chan->pls[0] = 0;
  }

  chan->bytes = 0;
  chan->compat = 0;

  /* +k or +l ? */
  if (chan->key && !chan->rmkey) {
    if (plus != 1) {
      *p++ = '+', plus = 1;
    }
    *p++ = 'k';

    postsize -= egg_strcatn(post, chan->key, sizeof(post));
    postsize -= egg_strcatn(post, " ", sizeof(post));

    nfree(chan->key), chan->key = NULL;
  }

  /* max +l is signed 2^32 on IRCnet at least... so make sure we've got at least
   * a 13 char buffer for '-2147483647 \0'. We'll be overwriting the existing
   * terminating null in 'post', so make sure postsize >= 12.
   */
  if (chan->limit != 0 && postsize >= 12) {
    if (plus != 1) {
      *p++ = '+', plus = 1;
    }
    *p++ = 'l';

    /* 'sizeof(post) - 1' is used because we want to overwrite the old null */
    postsize -=
      sprintf(&post[(sizeof(post) - 1) - postsize], "%d ", chan->limit);

    chan->limit = 0;
  }

  /* -k ? */
  if (chan->rmkey) {
    if (plus) {
      *p++ = '-', plus = 0;
    }
    *p++ = 'k';

    postsize -= egg_strcatn(post, chan->rmkey, sizeof(post));
    postsize -= egg_strcatn(post, " ", sizeof(post));

    nfree(chan->rmkey), chan->rmkey = NULL;
  }

  /* Do -{b,e,I} before +{b,e,I} to avoid the server ignoring overlaps */
  for (i = 0; i < modesperline; i++) {
    if ((chan->cmode[i].type & MINUS) && postsize > strlen(chan->cmode[i].op)) {
      if (plus) {
        *p++ = '-', plus = 0;
      }

      *p++ = ((chan->cmode[i].type & BAN) ? 'b' :
              ((chan->cmode[i].type & CHOP) ? 'o' :
              ((chan->cmode[i].type & CHHOP) ? 'h' :
              ((chan->cmode[i].type & EXEMPT) ? 'e' :
              ((chan->cmode[i].type & INVITE) ? 'I' : 'v')))));

      postsize -= egg_strcatn(post, chan->cmode[i].op, sizeof(post));
      postsize -= egg_strcatn(post, " ", sizeof(post));

      nfree(chan->cmode[i].op), chan->cmode[i].op = NULL;
      chan->cmode[i].type = 0;
    }
  }

  /* now do all the + modes... */
  for (i = 0; i < modesperline; i++) {
    if ((chan->cmode[i].type & PLUS) && postsize > strlen(chan->cmode[i].op)) {
      if (plus != 1) {
        *p++ = '+', plus = 1;
      }

      *p++ = ((chan->cmode[i].type & BAN) ? 'b' :
              ((chan->cmode[i].type & CHOP) ? 'o' :
              ((chan->cmode[i].type & CHHOP) ? 'h' :
              ((chan->cmode[i].type & EXEMPT) ? 'e' :
              ((chan->cmode[i].type & INVITE) ? 'I' : 'v')))));

      postsize -= egg_strcatn(post, chan->cmode[i].op, sizeof(post));
      postsize -= egg_strcatn(post, " ", sizeof(post));

      nfree(chan->cmode[i].op), chan->cmode[i].op = NULL;
      chan->cmode[i].type = 0;
    }
  }

  /* remember to terminate the buffer ('out')... */
  *p = 0;

  if (post[0]) {
    /* remove the trailing space... */
    size_t index = (sizeof(post) - 1) - postsize;

    if (index > 0 && post[index - 1] == ' ')
      post[index - 1] = 0;

    egg_strcatn(out, " ", sizeof(out));
    egg_strcatn(out, post, sizeof(out));
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
  int i, type, modes, l;
  masklist *m;
  memberlist *mx;
  char s[21];

  /* Capability is rank-based for PREFIX modes, literal o/h for non-prefix. */
  if (!can_set_mode(chan, mode))
    return;

  if (mode == 'o' || mode == 'h' || mode == 'v') {
    mx = ismember(chan, op);
    if (!mx)
      return;
    if (plus == '-') {
      if (member_prefix_sentminus(mx, mode) || !member_has_prefixmode(mx, mode))
        return;
      member_set_prefix_sentminus(mx, mode, 1);
    }
    if (plus == '+') {
      if (member_prefix_sentplus(mx, mode) || member_has_prefixmode(mx, mode))
        return;
      member_set_prefix_sentplus(mx, mode, 1);
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

  if (mode == 'o' || mode == 'h' || mode == 'b' || mode == 'v' || mode == 'e' ||
      mode == 'I') {
    type = (plus == '+' ? PLUS : MINUS) | (mode == 'o' ? CHOP : (mode == 'h' ?
           CHHOP : (mode == 'b' ? BAN : (mode == 'v' ? VOICE : (mode == 'e' ?
           EXEMPT : INVITE)))));
    /*
     * FIXME: Some networks remove overlapped bans,
     *        IRCnet does not (poptix/drummer)
     *
     * Note:  On IRCnet ischanXXX() should be used, otherwise isXXXed().
     */
    if ((plus == '-' && ((mode == 'b' && !ischanban(chan, op)) ||
        (mode == 'e' && !ischanexempt(chan, op)) ||
        (mode == 'I' && !ischaninvite(chan, op)))) || (plus == '+' &&
        ((mode == 'b' && ischanban(chan, op)) ||
        (mode == 'e' && ischanexempt(chan, op)) ||
        (mode == 'I' && ischaninvite(chan, op)))))
      return;

    /* If there are already max_bans bans, max_exempts exemptions,
     * max_invites invitations or max_modes +b/+e/+I modes on the
     * channel, don't try to add one more.
     */
    if (plus == '+' && (mode == 'b' || mode == 'e' || mode == 'I')) {
      int bans = 0, exempts = 0, invites = 0;

      for (m = chan->channel.ban; m && m->mask[0]; m = m->next)
        bans++;
      if ((mode == 'b') && (bans >= max_bans))
        return;

      for (m = chan->channel.exempt; m && m->mask[0]; m = m->next)
        exempts++;
      if ((mode == 'e') && (exempts >= max_exempts))
        return;

      for (m = chan->channel.invite; m && m->mask[0]; m = m->next)
        invites++;
      if ((mode == 'I') && (invites >= max_invites))
        return;

      if (bans + exempts + invites >= max_modes)
        return;
    }

    /* op-type mode change */
    for (i = 0; i < modesperline; i++)
      if (chan->cmode[i].type == type && chan->cmode[i].op != NULL &&
          !rfc_casecmp(chan->cmode[i].op, op))
        return;                 /* Already in there :- duplicate */
    l = strlen(op) + 1;
    if (chan->bytes + l > mode_buf_len)
      flush_mode(chan, NORMAL);
    for (i = 0; i < modesperline; i++)
      if (chan->cmode[i].type == 0) {
        chan->cmode[i].type = type;
        chan->cmode[i].op = (char *) channel_malloc(l);
        chan->bytes += l;       /* Add 1 for safety */
        strcpy(chan->cmode[i].op, op);
        break;
      }
  }

  /* +k ? store key */
  else if (plus == '+' && mode == 'k') {
    if (chan->key)
      nfree(chan->key);
    chan->key = (char *) channel_malloc(strlen(op) + 1);
    if (chan->key)
      strcpy(chan->key, op);
  }
  /* -k ? store removed key */
  else if (plus == '-' && mode == 'k') {
    if (chan->rmkey)
      nfree(chan->rmkey);
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
  modes = modesperline;         /* Check for full buffer. */
  for (i = 0; i < modesperline; i++)
    if (chan->cmode[i].type)
      modes--;
  if (include_lk && chan->limit)
    modes--;
  if (include_lk && chan->rmkey)
    modes--;
  if (include_lk && chan->key)
    modes--;
  if (modes < 1)
    flush_mode(chan, NORMAL);   /* Full buffer! Flush modes. */
}


/*
 *    Mode parsing functions
 */

static void got_key(struct chanset_t *chan, char *nick, char *from, char *key)
{
  if (!nick[0] && bounce_modes)
    reversing = 1;

  if (!(glob_master(user) || glob_bot(user) || chan_master(user)) &&
      !match_my_nick(nick)) {
    if ((reversing && !chan->key_prot[0]) || (chan->mode_mns_prot & CHANKEY)) {
      if (strlen(key) != 0)
        add_mode(chan, '-', 'k', key);
      else
        add_mode(chan, '-', 'k', "");
    }
    if ((chan->mode_pls_prot & CHANKEY) && (chan->key_prot[0] != 0) &&
        strcmp(key, chan->key_prot)) {
      add_mode(chan, '+', 'k', chan->key_prot);
    }
  }
}

static struct chanset_t *got_prefixmode(struct chanset_t *chan, char *ch,
                                        char *nick, char *from, char mode,
                                        char *who, struct userrec *opu,
                                        struct flag_record *opper)
{
  memberlist *m;
  char s[UHOSTLEN], modechange[3];
  struct userrec *u;
  int check_chan = 0, is_op = (mode == 'o'), snm = chan->stopnethack_mode;

  if (!who)
    return chan;
  m = ismember(chan, who);
  if (!m) {
    if (channel_pending(chan))
      return chan;
    putlog(LOG_MISC, chan->dname, CHAN_BADCHANMODE, chan->dname, who);
    chan->status |= CHAN_PEND;
    refresh_who_chan(chan->name);
    return chan;
  }

  if (is_op) {
    if (!me_op(chan) && match_my_nick(who))
      check_chan = 1;
  } else if (!me_op(chan) && !me_halfop(chan) && match_my_nick(who))
    check_chan = 1;

  simple_sprintf(s, "%s!%s", m->nick, m->userhost);
  u = get_user_from_member(m);
  get_user_flagrec(u, &victim, chan->dname);
  member_set_prefixmode(m, mode, 1);
  modechange[0] = '+';
  modechange[1] = mode;
  modechange[2] = 0;
  check_tcl_mode(nick, from, opu, chan->dname, modechange, who);
  if (!(chan = modebind_refresh(ch, from, opper, s, &victim)) ||
      !(m = ismember(chan, who)))
    return NULL;
  member_set_prefix_sentplus(m, mode, 0);

  if (channel_pending(chan))
    return chan;

  if (nick[0] && can_set_mode(chan, mode) && !match_my_nick(who) &&
      !match_my_nick(nick)) {
    if (is_op) {
      if (channel_bitch(chan) && !(glob_master(*opper) || glob_bot(*opper)) &&
          !chan_master(*opper) && !(glob_op(victim) || glob_bot(victim)) &&
          !chan_op(victim))
        add_mode(chan, '-', mode, who);
      else if ((chan_deop(victim) ||
               (glob_deop(victim) && !chan_op(victim))) &&
               !glob_master(*opper) && !chan_master(*opper))
        add_mode(chan, '-', mode, who);
      else if (reversing)
        add_mode(chan, '-', mode, who);
    } else {
      if (channel_bitch(chan) && !(glob_master(*opper) || glob_bot(*opper)) &&
          !chan_master(*opper) && !(glob_halfop(victim) || glob_op(victim) ||
          glob_bot(victim)) && !chan_op(victim) && !chan_halfop(victim))
        add_mode(chan, '-', mode, who);
      else if ((chan_dehalfop(victim) || (glob_dehalfop(victim) &&
               !chan_halfop(victim))) && !glob_master(*opper) &&
               !chan_master(*opper))
        add_mode(chan, '-', mode, who);
      else if (reversing)
        add_mode(chan, '-', mode, who);
    }
  } else if (reversing && can_set_mode(chan, mode) && !match_my_nick(who) &&
             !match_my_nick(nick))
    add_mode(chan, '-', mode, who);

  if (!nick[0] && can_set_mode(chan, mode) && !match_my_nick(who)) {
    if (is_op) {
      if (chan_deop(victim) || (glob_deop(victim) && !chan_op(victim))) {
        m->flags |= FAKEOP;
        add_mode(chan, '-', mode, who);
      } else if (snm > 0 && snm < 7 && !((channel_autoop(chan) ||
               glob_autoop(victim) || chan_autoop(victim)) &&
               (chan_op(victim) || (glob_op(victim) && !chan_deop(victim)))) &&
               !glob_exempt(victim) && !chan_exempt(victim)) {
        if (snm == 5)
          snm = channel_bitch(chan) ? 1 : 3;
        if (snm == 6)
          snm = channel_bitch(chan) ? 4 : 2;
        if (chan_wasoptest(victim) || glob_wasoptest(victim) || snm == 2) {
          if (!chan_wasop(m)) {
            m->flags |= FAKEOP;
            add_mode(chan, '-', mode, who);
          }
        } else if (!(chan_op(victim) ||
                 (glob_op(victim) && !chan_deop(victim)))) {
          if (snm == 1 || snm == 4 || (snm == 3 && !chan_wasop(m))) {
            add_mode(chan, '-', mode, who);
            m->flags |= FAKEOP;
          }
        } else if (snm == 4 && !chan_wasop(m)) {
          add_mode(chan, '-', mode, who);
          m->flags |= FAKEOP;
        }
      }
    } else {
      if (chan_dehalfop(victim) || (glob_dehalfop(victim) &&
          !chan_halfop(victim))) {
        m->flags |= FAKEHALFOP;
        add_mode(chan, '-', mode, who);
      } else if (snm > 0 && snm < 7 && !((channel_autohalfop(chan) ||
               glob_autohalfop(victim) || chan_autohalfop(victim)) &&
               (chan_halfop(victim) || (glob_halfop(victim) &&
               !chan_dehalfop(victim)))) && !glob_exempt(victim) &&
               !chan_exempt(victim)) {
        if (snm == 5)
          snm = channel_bitch(chan) ? 1 : 3;
        if (snm == 6)
          snm = channel_bitch(chan) ? 4 : 2;
        if (chan_washalfoptest(victim) || glob_washalfoptest(victim) ||
            snm == 2) {
          if (!chan_washalfop(m)) {
            m->flags |= FAKEHALFOP;
            add_mode(chan, '-', mode, who);
          }
        } else if (!(chan_halfop(victim) || (glob_halfop(victim) &&
                 !chan_dehalfop(victim)))) {
          if (snm == 1 || snm == 4 || (snm == 3 && !chan_washalfop(m))) {
            add_mode(chan, '-', mode, who);
            m->flags |= FAKEHALFOP;
          }
        } else if (snm == 4 && !chan_washalfop(m)) {
          add_mode(chan, '-', mode, who);
          m->flags |= FAKEHALFOP;
        }
      }
    }
  }
  member_set_wasprefixmode(m, mode, 1);
  if (check_chan)
    recheck_channel(chan, 1);
  return chan;
}

static struct chanset_t *got_deprefixmode(struct chanset_t *chan, char *ch,
                                          char *nick, char *from, char mode,
                                          char *who, struct userrec *opu)
{
  memberlist *m;
  char s[UHOSTLEN], s1[UHOSTLEN], modechange[3];
  struct userrec *u;
  int had_prefix, is_op = (mode == 'o');

  if (!who)
    return chan;
  m = ismember(chan, who);
  if (!m) {
    if (channel_pending(chan))
      return chan;
    putlog(LOG_MISC, chan->dname, CHAN_BADCHANMODE, chan->dname, who);
    chan->status |= CHAN_PEND;
    refresh_who_chan(chan->name);
    return chan;
  }

  simple_sprintf(s, "%s!%s", m->nick, m->userhost);
  simple_sprintf(s1, "%s!%s", nick, from);
  u = get_user_from_member(m);
  get_user_flagrec(u, &victim, chan->dname);
  had_prefix = member_has_prefixmode(m, mode);
  member_set_prefixmode(m, mode, 0);
  member_set_prefix_sentminus(m, mode, 0);
  if (is_op)
    m->flags &= ~FAKEOP;
  else
    m->flags &= ~FAKEHALFOP;
  modechange[0] = '-';
  modechange[1] = mode;
  modechange[2] = 0;
  check_tcl_mode(nick, from, opu, chan->dname, modechange, who);
  if (!(chan = modebind_refresh(ch, from, &user, s, &victim)) ||
      !(m = ismember(chan, who)))
    return NULL;
  member_set_wasprefixmode(m, mode, 0);

  if (channel_pending(chan))
    return chan;

  if (can_set_mode(chan, mode)) {
    int ok = 1;

    if (is_op) {
      if (!glob_deop(victim) && !chan_deop(victim)) {
        if (channel_protectops(chan) && (glob_master(victim) ||
            chan_master(victim) || glob_op(victim) || chan_op(victim)))
          ok = 0;
        else if (channel_protectfriends(chan) && (glob_friend(victim) ||
                 chan_friend(victim)))
          ok = 0;
      }
      if ((reversing || !ok) && had_prefix && !match_my_nick(nick) &&
          rfc_casecmp(who, nick) && !match_my_nick(who) &&
          !glob_master(user) && !chan_master(user) && !glob_bot(user) &&
          ((chan_op(victim) || (glob_op(victim) && !chan_deop(victim))) ||
          !channel_bitch(chan)))
        add_mode(chan, '+', mode, who);
    } else {
      if (!glob_dehalfop(victim) && !chan_dehalfop(victim)) {
        if (channel_protecthalfops(chan) && (glob_master(victim) ||
            chan_master(victim) || glob_halfop(victim) || chan_halfop(victim)))
          ok = 0;
        else if (channel_protectfriends(chan) && (glob_friend(victim) ||
                 chan_friend(victim)))
          ok = 0;
      }
      if ((reversing || !ok) && had_prefix && !match_my_nick(nick) &&
          rfc_casecmp(who, nick) && !match_my_nick(who) &&
          !glob_master(user) && !chan_master(user) && !glob_bot(user) &&
          ((chan_halfop(victim) ||
          (glob_halfop(victim) && !chan_dehalfop(victim))) ||
          !channel_bitch(chan)))
        add_mode(chan, '+', mode, who);
    }
  }

  if (!nick[0])
    putlog(LOG_MODES, chan->dname, "TS resync (%s): %s deopped by %s",
           chan->dname, who, from);

  if (is_op) {
    if (nick[0])
      detect_chan_flood(nick, from, s1, chan, FLOOD_DEOP, who);
    if (!(m->flags & (CHANVOICE | CHANHALFOP | STOPWHO))) {
      chan->status |= CHAN_PEND;
      refresh_who_chan(chan->name);
      m->flags |= STOPWHO;
    }
    if (match_my_nick(who)) {
      memberlist *m2;

      for (m2 = chan->channel.member; m2 && m2->nick[0]; m2 = m2->next) {
        m2->sentplus = 0;
        m2->sentminus = 0;
        m2->flags &= ~(SENTKICK | SENTDEOP | SENTOP | SENTHALFOP |
                       SENTDEHALFOP | SENTVOICE | SENTDEVOICE);
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
  } else if (!(m->flags & (CHANVOICE | STOPWHO))) {
    chan->status |= CHAN_PEND;
    refresh_who_chan(chan->name);
    m->flags |= STOPWHO;
  }
  return chan;
}

static void got_ban(struct chanset_t *chan, char *nick, char *from, char *who,
                    char *ch, struct userrec *u)
{
  char me[UHOSTLEN], s[UHOSTLEN], s1[UHOSTLEN];
  memberlist *m;
  struct userrec *targ;

  egg_snprintf(me, sizeof me, "%s!%s", botname, botuserhost);
  egg_snprintf(s, sizeof s, "%s!%s", nick, from);
  newban(chan, who, s);
  check_tcl_mode(nick, from, u, chan->dname, "+b", who);
  if (!(chan = modebind_refresh(ch, from, &user, NULL, NULL)))
    return;

  if (channel_pending(chan) || !can_set_mode(chan, 'b'))
    return;

  if (match_addr(who, me) && !isexempted(chan, me)) {
    add_mode(chan, '-', 'b', who);
    reversing = 1;
    return;
  }
  if (!match_my_nick(nick)) {
    if (nick[0] && channel_nouserbans(chan) && !glob_bot(user) &&
        !glob_master(user) && !chan_master(user)) {
      add_mode(chan, '-', 'b', who);
      return;
    }
    for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
      egg_snprintf(s1, sizeof s1, "%s!%s", m->nick, m->userhost);
      if (match_addr(who, s1)) {
        targ = get_user_from_member(m);
        if (targ) {
          get_user_flagrec(targ, &victim, chan->dname);
          if ((glob_friend(victim) || (glob_op(victim) && !chan_deop(victim)) ||
               chan_friend(victim) || chan_op(victim)) && !glob_master(user) &&
              !glob_bot(user) && !chan_master(user) && !isexempted(chan, s1)) {
            add_mode(chan, '-', 'b', who);
            return;
          }
        }
      }
    }
  }
  refresh_exempt(chan, who);
  if (nick[0] && channel_enforcebans(chan)) {
    maskrec *b;
    int cycle;
    char resn[512] = "";

    for (cycle = 0; cycle < 2; cycle++) {
      for (b = cycle ? chan->bans : global_bans; b; b = b->next) {
        if (match_addr(b->mask, who)) {
          if (b->desc && b->desc[0] != '@')
            egg_snprintf(resn, sizeof resn, "%s %s", IRC_PREBANNED, b->desc);
          else
            resn[0] = 0;
        }
      }
    }
    kick_all(chan, who, resn[0] ? resn : IRC_BANNED,
             match_my_nick(nick) ? 0 : 1);
  }
  if (!nick[0] && (bounce_bans || bounce_modes) &&
      (!u_equals_mask(global_bans, who) || !u_equals_mask(chan->bans, who)))
    add_mode(chan, '-', 'b', who);
}

static void got_unban(struct chanset_t *chan, char *nick, char *from,
                      char *who, char *ch, struct userrec *u)
{
  masklist *b, *old;

  old = NULL;
  for (b = chan->channel.ban; b->mask[0] && rfc_casecmp(b->mask, who);
       old = b, b = b->next);
  if (b->mask[0]) {
    if (old)
      old->next = b->next;
    else
      chan->channel.ban = b->next;
    nfree(b->mask);
    nfree(b->who);
    nfree(b);
  }
  check_tcl_mode(nick, from, u, chan->dname, "-b", who);
  if (!(chan = modebind_refresh(ch, from, &user, NULL, NULL)))
    return;

  if (channel_pending(chan))
    return;

  if ((u_sticky_mask(chan->bans, who) || u_sticky_mask(global_bans, who)) ||
      ((u_equals_mask(global_bans, who) || u_equals_mask(chan->bans, who)) &&
      !channel_dynamicbans(chan) && ((!glob_bot(user) ||
      !(bot_flags(u) & BOT_SHARE)) && ((!glob_op(user) || chan_deop(user)) &&
      !chan_op(user)) && ((!glob_halfop(user) || chan_dehalfop(user)) &&
      !chan_halfop(user)))))
    add_mode(chan, '+', 'b', who);
}

static void got_exempt(struct chanset_t *chan, char *nick, char *from,
                       char *who, char *ch, struct userrec *u)
{
  char s[UHOSTLEN];

  simple_sprintf(s, "%s!%s", nick, from);
  newexempt(chan, who, s);
  check_tcl_mode(nick, from, u, chan->dname, "+e", who);
  if (!(chan = modebind_refresh(ch, from, &user, NULL, NULL)))
    return;

  if (channel_pending(chan) || !can_set_mode(chan, 'e'))
    return;

  if (!match_my_nick(nick)) {
    if (nick[0] && channel_nouserexempts(chan) && !glob_bot(user) &&
        !glob_master(user) && !chan_master(user)) {
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
                         char *who, char *ch, struct userrec *u)
{
  masklist *e = chan->channel.exempt, *old = NULL;
  masklist *b;
  int match = 0;

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
  check_tcl_mode(nick, from, u, chan->dname, "-e", who);
  if (!(chan = modebind_refresh(ch, from, &user, NULL, NULL)))
    return;

  if (channel_pending(chan))
    return;

  if (u_sticky_mask(chan->exempts, who) || u_sticky_mask(global_exempts, who))
    add_mode(chan, '+', 'e', who);

  /* If exempt was removed by master then leave it else check for bans */
  if (!nick[0] && glob_bot(user) && !glob_master(user) && !chan_master(user)) {
    b = chan->channel.ban;
    while (b->mask[0] && !match) {
      if (mask_match(b->mask, who)) {
        add_mode(chan, '+', 'e', who);
        match = 1;
      } else
        b = b->next;
    }
  }
  if ((u_equals_mask(global_exempts, who) ||
      u_equals_mask(chan->exempts, who)) && me_op(chan) &&
      !channel_dynamicexempts(chan) && (!glob_bot(user) ||
      !(bot_flags(u) & BOT_SHARE)))
    add_mode(chan, '+', 'e', who);
}

static void got_invite(struct chanset_t *chan, char *nick, char *from,
                       char *who, char *ch, struct userrec *u)
{
  char s[UHOSTLEN];

  simple_sprintf(s, "%s!%s", nick, from);
  newinvite(chan, who, s);
  check_tcl_mode(nick, from, u, chan->dname, "+I", who);
  if (!(chan = modebind_refresh(ch, from, &user, NULL, NULL)))
    return;

  if (channel_pending(chan) || !can_set_mode(chan, 'I'))
    return;

  if (!match_my_nick(nick)) {
    if (nick[0] && channel_nouserinvites(chan) && !glob_bot(user) &&
        !glob_master(user) && !chan_master(user)) {
      add_mode(chan, '-', 'I', who);
      return;
    }
    if ((!nick[0]) && (bounce_modes))
      reversing = 1;
  }
  if (reversing || (bounce_invites && (!nick[0]) &&
      (!u_equals_mask(global_invites, who) ||
      !u_equals_mask(chan->invites, who))))
    add_mode(chan, '-', 'I', who);
}

static void got_uninvite(struct chanset_t *chan, char *nick, char *from,
                         char *who, char *ch, struct userrec *u)
{
  masklist *inv = chan->channel.invite, *old = NULL;

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
  check_tcl_mode(nick, from, u, chan->dname, "-I", who);
  if (!(chan = modebind_refresh(ch, from, &user, NULL, NULL)))
    return;

  if (channel_pending(chan))
    return;

  if (u_sticky_mask(chan->invites, who) || u_sticky_mask(global_invites, who))
    add_mode(chan, '+', 'I', who);
  if (!nick[0] && glob_bot(user) && !glob_master(user) && !chan_master(user) &&
      (chan->channel.mode & CHANINV))
    add_mode(chan, '+', 'I', who);
  if ((u_equals_mask(global_invites, who) ||
      u_equals_mask(chan->invites, who)) && me_op(chan) &&
      !channel_dynamicinvites(chan) && (!glob_bot(user) ||
      !(bot_flags(u) & BOT_SHARE)))
    add_mode(chan, '+', 'I', who);
}

static struct chanset_t *got_voice_mode(struct chanset_t *chan, char *ch,
                                        char *nick, char *from, char *modechange,
                                        char *who, struct userrec *opu)
{
  memberlist *m;
  char s[UHOSTLEN];

  if (!who)
    return chan;
  m = ismember(chan, who);
  if (!m) {
    if (channel_pending(chan))
      return chan;
    putlog(LOG_MISC, chan->dname, CHAN_BADCHANMODE, chan->dname, who);
    chan->status |= CHAN_PEND;
    refresh_who_chan(chan->name);
    return chan;
  }

  simple_sprintf(s, "%s!%s", m->nick, m->userhost);
  get_user_flagrec(get_user_from_member(m), &victim, chan->dname);
  if (modechange[0] == '+') {
    member_set_prefix_sentplus(m, 'v', 0);
    member_set_prefixmode(m, 'v', 1);
    check_tcl_mode(nick, from, opu, chan->dname, modechange, who);
    if (!(chan = modebind_refresh(ch, from, &user, s, &victim)) ||
        !(m = ismember(chan, who)))
      return NULL;
    member_set_wasprefixmode(m, 'v', 1);
    if (channel_active(chan) && !glob_master(user) &&
        !chan_master(user) && !match_my_nick(nick)) {
      if (chan_quiet(victim) || (glob_quiet(victim) && !chan_voice(victim)))
        add_mode(chan, '-', 'v', who);
      else if (reversing)
        add_mode(chan, '-', 'v', who);
    }
  } else {
    member_set_prefix_sentminus(m, 'v', 0);
    member_set_prefixmode(m, 'v', 0);
    check_tcl_mode(nick, from, opu, chan->dname, modechange, who);
    if (!(chan = modebind_refresh(ch, from, &user, s, &victim)) ||
        !(m = ismember(chan, who)))
      return NULL;
    member_set_wasprefixmode(m, 'v', 0);
    if (channel_active(chan) && !glob_master(user) &&
        !chan_master(user) && !match_my_nick(nick)) {
      if ((channel_autovoice(chan) && !chan_quiet(victim) &&
          (chan_voice(victim) || glob_voice(victim))) ||
          (!chan_quiet(victim) && (glob_gvoice(victim) ||
          chan_gvoice(victim))))
        add_mode(chan, '+', 'v', who);
      else if (reversing)
        add_mode(chan, '+', 'v', who);
    }
  }
  return chan;
}

static struct chanset_t *got_generic_prefixmode(struct chanset_t *chan,
                                               char *ch, char *nick,
                                               char *from, char mode,
                                               char *modechange, char *who,
                                               struct userrec *opu)
{
  memberlist *m;
  char s[UHOSTLEN];

  if (!who)
    return chan;
  m = ismember(chan, who);
  if (!m) {
    if (channel_pending(chan))
      return chan;
    putlog(LOG_MISC, chan->dname, CHAN_BADCHANMODE, chan->dname, who);
    chan->status |= CHAN_PEND;
    refresh_who_chan(chan->name);
    return chan;
  }
  simple_sprintf(s, "%s!%s", m->nick, m->userhost);
  get_user_flagrec(get_user_from_member(m), &victim, chan->dname);
  if (modechange[0] == '+') {
    member_set_prefixmode(m, mode, 1);
    member_set_prefix_sentplus(m, mode, 0);
  } else {
    member_set_prefixmode(m, mode, 0);
    member_set_prefix_sentminus(m, mode, 0);
  }
  check_tcl_mode(nick, from, opu, chan->dname, modechange, who);
  if (!(chan = modebind_refresh(ch, from, &user, s, &victim)) ||
      !(m = ismember(chan, who)))
    return NULL;
  member_set_wasprefixmode(m, mode, modechange[0] == '+');
  return chan;
}

static struct chanset_t *got_generic_listmode(struct chanset_t *chan, char *ch,
                                              char *nick, char *from, char mode,
                                              char *modechange, char *mask,
                                              struct userrec *u)
{
  char setter[UHOSTLEN];

  simple_sprintf(setter, "%s!%s", nick, from);
  if (modechange[0] == '+')
    chanmode_list_add(chan, mode, mask, setter);
  else
    chanmode_list_remove(chan, mode, mask);
  check_tcl_mode(nick, from, u, chan->dname, modechange, mask ? mask : "");
  return modebind_refresh(ch, from, &user, NULL, NULL);
}

static struct chanset_t *got_flagmode(struct chanset_t *chan, char *ch,
                                      char *nick, char *from, char mode,
                                      char *modechange, struct userrec *u)
{
  if (!nick[0] && bounce_modes)
    reversing = 1;
  check_tcl_mode(nick, from, u, chan->dname, modechange, "");
  if (!(chan = modebind_refresh(ch, from, &user, NULL, NULL)))
    return NULL;
  if (modechange[0] == '+')
    chanmode_set(chan, mode, NULL);
  else
    chanmode_unset(chan, mode);
  if (channel_active(chan)) {
    int pls = chanmode_pls_protected(chan, mode),
        mns = chanmode_mns_protected(chan, mode);

    if ((((modechange[0] == '+') && mns) ||
        ((modechange[0] == '-') && pls)) &&
        !glob_master(user) && !chan_master(user) && !match_my_nick(nick))
      add_mode(chan, modechange[0] == '+' ? '-' : '+', mode, "");
    else if (reversing && ((modechange[0] == '+') || pls) &&
             ((modechange[0] == '-') || mns))
      add_mode(chan, modechange[0] == '+' ? '-' : '+', mode, "");
  }
  return chan;
}

static struct chanset_t *got_keymode(struct chanset_t *chan, char *ch,
                                     char *nick, char *from, char mode,
                                     char *modechange, char *arg,
                                     struct userrec *u)
{
  char oldkey[512];

  if (mode == 'k')
    strlcpy(oldkey, chan->channel.key, sizeof oldkey);
  else
    oldkey[0] = 0;
  if (modechange[0] == '+')
    chanmode_set(chan, mode, arg);
  else
    chanmode_unset(chan, mode);
  check_tcl_mode(nick, from, u, chan->dname, modechange, arg ? arg : "");
  if (!(chan = modebind_refresh(ch, from, &user, NULL, NULL)))
    return NULL;
  if (modechange[0] == '+') {
    chanmode_set(chan, mode, arg);
    if (mode == 'k' && channel_active(chan))
      got_key(chan, nick, from, arg ? arg : "");
  } else {
    if (mode == 'k' && channel_active(chan)) {
      if (reversing && oldkey[0])
        add_mode(chan, '+', mode, oldkey);
      else if (chan->key_prot[0] && !glob_master(user) &&
               !chan_master(user) && !match_my_nick(nick))
        add_mode(chan, '+', mode, chan->key_prot);
    }
    chanmode_unset(chan, mode);
  }
  return chan;
}

static struct chanset_t *got_limitmode(struct chanset_t *chan, char *ch,
                                       char *nick, char *from, char mode,
                                       char *modechange, char *arg,
                                       struct userrec *u)
{
  char s[UHOSTLEN], bindarg[512];

  if (!nick[0] && bounce_modes)
    reversing = 1;
  if (modechange[0] == '-') {
    check_tcl_mode(nick, from, u, chan->dname, modechange, "");
    if (!(chan = modebind_refresh(ch, from, &user, NULL, NULL)))
      return NULL;
    if (mode == 'l' && channel_active(chan)) {
      if (reversing && (chan->channel.maxmembers != 0)) {
        simple_sprintf(s, "%d", chan->channel.maxmembers);
        add_mode(chan, '+', mode, s);
      } else if ((chan->limit_prot != 0) && !glob_master(user) &&
                 !chan_master(user) && !match_my_nick(nick)) {
        simple_sprintf(s, "%d", chan->limit_prot);
        add_mode(chan, '+', mode, s);
      }
    }
    chanmode_unset(chan, mode);
  } else {
    chanmode_set(chan, mode, arg);
    if (mode == 'l')
      strlcpy(bindarg, int_to_base10(chan->channel.maxmembers), sizeof bindarg);
    else
      strlcpy(bindarg, arg ? arg : "", sizeof bindarg);
    check_tcl_mode(nick, from, u, chan->dname, modechange, bindarg);
    if (!(chan = modebind_refresh(ch, from, &user, NULL, NULL)))
      return NULL;
    if (channel_pending(chan))
      return chan;
    if (mode == 'l') {
      if ((reversing && !(chan->mode_pls_prot & CHANLIMIT)) ||
          ((chan->mode_mns_prot & CHANLIMIT) && !glob_master(user) &&
          !chan_master(user)))
        add_mode(chan, '-', mode, "");
      if ((chan->limit_prot != chan->channel.maxmembers) &&
          (chan->mode_pls_prot & CHANLIMIT) && (chan->limit_prot != 0) &&
          !glob_master(user) && !chan_master(user)) {
        simple_sprintf(s, "%d", chan->limit_prot);
        add_mode(chan, '+', mode, s);
      }
    }
  }
  return chan;
}

static int gotmode(char *from, char *origmsg)
{
  char *nick, *ch, *chg;
  char buf[511], joinbuf[512];
  char ms2[3], *arg;
  int nextarg;
  struct parsed_irc msg;
  struct userrec *u;
  memberlist *m;
  struct chanset_t *chan;

  strlcpy(buf, origmsg, sizeof buf);
  msg = parse_irc(buf);
  /* Usermode changes? */
  if (msg.argc > 1 && (strchr(CHANMETA, msg.argv[0][0]) != NULL)) {
    ch = msg.argv[0];
    chg = msg.argv[1];
    nextarg = 2;
    reversing = 0;
    chan = findchan(ch);
    if (!chan) {
      putlog(LOG_MISC, "*", CHAN_FORCEJOIN, ch);
      dprintf(DP_SERVER, "PART %s\n", ch);
    } else if (channel_active(chan) || channel_pending(chan)) {
      putlog(LOG_MODES, chan->dname, "%s: mode change '%s %s' by %s", ch, chg,
             join_str_array(msg.argv + 2, msg.argc - 2, " ", joinbuf, sizeof joinbuf), from);
      nick = splitnick(&from);
      m = ismember(chan, nick);
      if (m) {
        u = get_user_from_member(m);
        get_user_flagrec(u, &user, ch);
        m->last = now;
      } else {
        u = NULL;
      }
      if (m && channel_active(chan) && (me_op(chan) || (me_halfop(chan) &&
          !chan_hasop(m))) && !(glob_friend(user) || chan_friend(user) ||
          (channel_dontkickops(chan) && (chan_op(user) || (glob_op(user) &&
          !chan_deop(user))))) && !match_my_nick(nick)) {
        if (chan_fakeop(m) || chan_fakehalfop(m)) {
          putlog(LOG_MODES, ch, CHAN_FAKEMODE, ch);
          dprintf(DP_MODE, "KICK %s %s :%s\n", ch, nick, CHAN_FAKEMODE_KICK);
          m->flags |= SENTKICK;
          reversing = 1;
        } else if (!chan_hasop(m) && !chan_hashalfop(m) &&
                 !channel_nodesynch(chan)) {
          putlog(LOG_MODES, ch, CHAN_DESYNCMODE, ch);
          dprintf(DP_MODE, "KICK %s %s :%s\n", ch, nick, CHAN_DESYNCMODE_KICK);
          m->flags |= SENTKICK;
          reversing = 1;
        }
      }
      ms2[0] = '+';
      ms2[2] = 0;
      while ((ms2[1] = *chg)) {
        int mode_type;

        arg = NULL;

        switch (*chg) {
        case '+':
          ms2[0] = '+';
          chg++;
          continue;
        case '-':
          ms2[0] = '-';
          chg++;
          continue;
        }

        if ((ms2[0] == '+' && MODE_HAS_SET_ARG(*chg)) ||
            (ms2[0] == '-' && MODE_HAS_UNSET_ARG(*chg))) {
          if (nextarg < msg.argc)
            arg = msg.argv[nextarg++];
          else {
            putlog(LOG_MISC, "*",
                   "Error parsing modes in '%s', not enough arguments for %c%c",
                   origmsg, ms2[0], *chg);
            arg = "";
          }
        }
        debug5("%s: split mode change '%s%s%s' by %s", ch, ms2,
               arg ? " " : "", arg ? arg : "", from);

        mode_type = MODE_TYPE(*chg);
        switch (mode_type) {
        case MODETYPE_PREFIX:
          if (*chg == 'o' || *chg == 'h') {
            if (ms2[0] == '+')
              chan = got_prefixmode(chan, ch, nick, from, *chg, arg, u, &user);
            else
              chan = got_deprefixmode(chan, ch, nick, from, *chg, arg, u);
          } else if (*chg == 'v') {
            chan = got_voice_mode(chan, ch, nick, from, ms2, arg, u);
          } else {
            chan = got_generic_prefixmode(chan, ch, nick, from, *chg, ms2,
                                          arg, u);
          }
          if (!chan)
            return 0;
          break;
        case MODETYPE_LIST:
          if (*chg == 'b') {
            if (ms2[0] == '+')
              got_ban(chan, nick, from, arg, ch, u);
            else
              got_unban(chan, nick, from, arg, ch, u);
          } else if (*chg == 'e') {
            if (ms2[0] == '+')
              got_exempt(chan, nick, from, arg, ch, u);
            else
              got_unexempt(chan, nick, from, arg, ch, u);
          } else if (*chg == 'I') {
            if (ms2[0] == '+')
              got_invite(chan, nick, from, arg, ch, u);
            else
              got_uninvite(chan, nick, from, arg, ch, u);
          } else {
            chan = got_generic_listmode(chan, ch, nick, from, *chg, ms2, arg,
                                        u);
            if (!chan)
              return 0;
          }
          break;
        case MODETYPE_FLAG:
          chan = got_flagmode(chan, ch, nick, from, *chg, ms2, u);
          if (!chan)
            return 0;
          break;
        case MODETYPE_KEY:
          chan = got_keymode(chan, ch, nick, from, *chg, ms2, arg, u);
          if (!chan)
            return 0;
          break;
        case MODETYPE_LIMIT:
          chan = got_limitmode(chan, ch, nick, from, *chg, ms2, arg, u);
          if (!chan)
            return 0;
          break;
        default:
          check_tcl_mode(nick, from, u, chan->dname, ms2, "");
          if (!(chan = modebind_refresh(ch, from, &user, NULL, NULL)))
            return 0;
          break;
        }
        chg++;
      }
      if (!me_op(chan) && !nick[0])
        chan->status |= CHAN_ASKEDMODES;
    }
  }
  return 0;
}
