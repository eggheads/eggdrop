/*
 * chancmds.c -- part of irc.mod
 *   handles commands directly relating to channel interaction
 *
 * $Id: cmdsirc.c,v 1.67 2011/02/13 14:19:33 simple Exp $
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

static struct chanset_t *get_channel(int idx, char *chname)
{
  struct chanset_t *chan;

  if (chname && chname[0]) {
    chan = findchan_by_dname(chname);
    if (chan)
      return chan;
    else
      dprintf(idx, "No such channel.\n");
  } else {
    chname = dcc[idx].u.chat->con_chan;
    chan = findchan_by_dname(chname);
    if (chan)
      return chan;
    else
      dprintf(idx, "Invalid console channel.\n");
  }
  return 0;
}

/* Do we have any flags that will allow us ops on a channel?
 */
static int has_op(int idx, struct chanset_t *chan)
{
  get_user_flagrec(dcc[idx].user, &user, chan->dname);
  if (chan_op(user) || (glob_op(user) && !chan_deop(user)))
    return 1;
  dprintf(idx, "You are not a channel op on %s.\n", chan->dname);
  return 0;
}

static int has_oporhalfop(int idx, struct chanset_t *chan)
{
  get_user_flagrec(dcc[idx].user, &user, chan->dname);
  if (chan_op(user) || chan_halfop(user) || (glob_op(user) &&
      !chan_deop(user)) || (glob_halfop(user) && !chan_dehalfop(user)))
    return 1;
  dprintf(idx, "You are not a channel op or halfop on %s.\n", chan->dname);
  return 0;
}

/* Finds a nick of the handle. Returns m->nick if
 * the nick was found, otherwise NULL (Sup 1Nov2000)
 */
static char *getnick(char *handle, struct chanset_t *chan)
{
  char s[UHOSTLEN];
  struct userrec *u;
  register memberlist *m;

  for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
    egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
    if ((u = get_user_by_host(s)) && !egg_strcasecmp(u->handle, handle))
      return m->nick;
  }
  return NULL;
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
  chan = get_channel(idx, chname);
  if (!chan || !has_op(idx, chan))
    return;
  m = ismember(chan, botname);
  if (!m) {
    dprintf(idx, "Cannot say to %s: I'm not on that channel.\n", chan->dname);
    return;
  }
  if ((chan->channel.mode & CHANMODER) && !me_op(chan) && !me_halfop(chan) &&
      !me_voice(chan)) {
    dprintf(idx, "Cannot say to %s: It is moderated.\n", chan->dname);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# (%s) act %s", dcc[idx].nick, chan->dname, par);
  dprintf(DP_HELP, "PRIVMSG %s :\001ACTION %s\001\n", chan->name, par);
  dprintf(idx, "Action to %s: %s\n", chan->dname, par);
}

static void cmd_msg(struct userrec *u, int idx, char *par)
{
  char *nick;

  nick = newsplit(&par);
  if (!par[0])
    dprintf(idx, "Usage: msg <nick> <message>\n");
  else {
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
  chan = get_channel(idx, chname);
  if (!chan || !has_op(idx, chan))
    return;
  m = ismember(chan, botname);
  if (!m) {
    dprintf(idx, "Cannot say to %s: I'm not on that channel.\n", chan->dname);
    return;
  }
  if ((chan->channel.mode & CHANMODER) && !me_op(chan) && !me_halfop(chan) &&
      !me_voice(chan)) {
    dprintf(idx, "Cannot say to %s: It is moderated.\n", chan->dname);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# (%s) say %s", dcc[idx].nick, chan->dname, par);
  dprintf(DP_HELP, "PRIVMSG %s :%s\n", chan->name, par);
  dprintf(idx, "Said to %s: %s\n", chan->dname, par);
}

static void cmd_kickban(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *chname, *nick, *s1;
  memberlist *m;
  char s[UHOSTLEN];
  char bantype = 0;

  if (!par[0]) {
    dprintf(idx, "Usage: kickban [channel] [-|@]<nick> [reason]\n");
    return;
  }

  if (strchr(CHANMETA, par[0]) != NULL)
    chname = newsplit(&par);
  else
    chname = 0;
  chan = get_channel(idx, chname);
  if (!chan || !has_oporhalfop(idx, chan))
    return;
  if (!channel_active(chan)) {
    dprintf(idx, "I'm not on %s right now!\n", chan->dname);
    return;
  }
  if (HALFOP_CANTDOMODE('b')) {
    dprintf(idx, "I can't help you now because I'm not a channel op or halfop "
            "on %s, or halfops cannot set bans.\n", chan->dname);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# (%s) kickban %s", dcc[idx].nick,
         chan->dname, par);
  nick = newsplit(&par);
  if ((nick[0] == '@') || (nick[0] == '-')) {
    bantype = nick[0];
    nick++;
  }
  if (match_my_nick(nick)) {
    dprintf(idx, "I'm not going to kickban myself.\n");
    return;
  }
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s\n", nick, chan->dname);
    return;
  }
  if (!me_op(chan) && chan_hasop(m)) {
    dprintf(idx, "I can't help you now because halfops cannot kick ops.\n");
    return;
  }
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->dname);
  if ((chan_op(victim) || (glob_op(victim) && !chan_deop(victim))) &&
      !(chan_master(user) || glob_master(user))) {
    dprintf(idx, "%s is a legal op.\n", nick);
    return;
  }
  if ((chan_master(victim) || glob_master(victim)) &&
      !(glob_owner(user) || chan_owner(user))) {
    dprintf(idx, "%s is a %s master.\n", nick, chan->dname);
    return;
  }
  if (glob_bot(victim) && !(glob_owner(user) || chan_owner(user))) {
    dprintf(idx, "%s is another channel bot!\n", nick);
    return;
  }
  if (use_exempts && (u_match_mask(global_exempts, s) ||
      u_match_mask(chan->exempts, s))) {
    dprintf(idx, "%s is permanently exempted!\n", nick);
    return;
  }
  if (m->flags & CHANOP)
    add_mode(chan, '-', 'o', m->nick);
  check_exemptlist(chan, s);
  switch (bantype) {
  case '@':
    s1 = strchr(s, '@');
    s1 -= 3;
    s1[0] = '*';
    s1[1] = '!';
    s1[2] = '*';
    break;
  case '-':
    s1 = strchr(s, '!');
    s1[1] = '*';
    s1--;
    s1[0] = '*';
    break;
  default:
    s1 = quickban(chan, m->userhost);
    break;
  }
  if (bantype == '@' || bantype == '-')
    do_mask(chan, chan->channel.ban, s1, 'b');
  if (!par[0])
    par = "requested";
  dprintf(DP_SERVER, "KICK %s %s :%s\n", chan->name, m->nick, par);
  m->flags |= SENTKICK;
  u_addban(chan, s1, dcc[idx].nick, par, now + (60 * chan->ban_time), 0);
  dprintf(idx, "Okay, done.\n");
}

static void cmd_op(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *nick;
  memberlist *m;
  char s[UHOSTLEN];

  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan || !has_op(idx, chan))
    return;

  if (!nick[0] && !(nick = getnick(u->handle, chan))) {
    dprintf(idx, "Usage: op <nick> [channel]\n");
    return;
  }

  if (!channel_active(chan)) {
    dprintf(idx, "I'm not on %s right now!\n", chan->dname);
    return;
  }

  if (HALFOP_CANTDOMODE('o')) {
    dprintf(idx, "I can't help you now because I'm not a chan op or halfop on "
            "%s, or halfops cannot set +o modes.\n", chan->dname);
    return;
  }

  putlog(LOG_CMDS, "*", "#%s# (%s) op %s", dcc[idx].nick, chan->dname, nick);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s.\n", nick, chan->dname);
    return;
  }
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->dname);
  if (chan_deop(victim) || (glob_deop(victim) && !glob_op(victim))) {
    dprintf(idx, "%s is currently being auto-deopped.\n", m->nick);
    return;
  }
  if (channel_bitch(chan) && !(chan_op(victim) || (glob_op(victim) &&
      !chan_deop(victim)))) {
    dprintf(idx, "%s is not a registered op.\n", m->nick);
    return;
  }
  add_mode(chan, '+', 'o', nick);
  dprintf(idx, "Gave op to %s on %s.\n", nick, chan->dname);
}

static void cmd_deop(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *nick;
  memberlist *m;
  char s[UHOSTLEN];

  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan || !has_op(idx, chan))
    return;

  if (!nick[0] && !(nick = getnick(u->handle, chan))) {
    dprintf(idx, "Usage: deop <nick> [channel]\n");
    return;
  }

  if (!channel_active(chan)) {
    dprintf(idx, "I'm not on %s right now!\n", chan->dname);
    return;
  }

  if (HALFOP_CANTDOMODE('o')) {
    dprintf(idx, "I can't help you now because I'm not a chan op or halfop on "
            "%s, or halfops cannot set -o modes.\n", chan->dname);
    return;
  }

  putlog(LOG_CMDS, "*", "#%s# (%s) deop %s", dcc[idx].nick, chan->dname, nick);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s.\n", nick, chan->dname);
    return;
  }
  if (match_my_nick(nick)) {
    dprintf(idx, "I'm not going to deop myself.\n");
    return;
  }
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->dname);
  if ((chan_master(victim) || glob_master(victim)) &&
      !(chan_owner(user) || glob_owner(user))) {
    dprintf(idx, "%s is a master for %s.\n", m->nick, chan->dname);
    return;
  }
  if ((chan_op(victim) || (glob_op(victim) && !chan_deop(victim))) &&
      !(chan_master(user) || glob_master(user))) {
    dprintf(idx, "%s has the op flag for %s.\n", m->nick, chan->dname);
    return;
  }
  add_mode(chan, '-', 'o', nick);
  dprintf(idx, "Took op from %s on %s.\n", nick, chan->dname);
}

static void cmd_halfop(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  struct userrec *u2;
  char *nick;
  memberlist *m;
  char s[UHOSTLEN];

  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan)
    return;

  if (!nick[0] && !(nick = getnick(u->handle, chan))) {
    dprintf(idx, "Usage: halfop <nick> [channel]\n");
    return;
  }

  get_user_flagrec(dcc[idx].user, &user, chan->dname);
  m = ismember(chan, nick);
  if (m && !chan_op(user) && (!glob_op(user) || chan_deop(user))) {
    egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
    u2 = m->user ? m->user : get_user_by_host(s);

    if (!u2 || strcmp(u2->handle, dcc[idx].nick) || (!chan_halfop(user) &&
        (!glob_halfop(user) || chan_dehalfop(user)))) {
      dprintf(idx, "You are not a channel op on %s.\n", chan->dname);
      return;
    }
  }

  if (!channel_active(chan)) {
    dprintf(idx, "I'm not on %s right now!\n", chan->dname);
    return;
  }

  if (HALFOP_CANTDOMODE('h')) {
    dprintf(idx, "I can't help you now because I'm not a chan op or halfop on "
            "%s, or halfops cannot set +h modes.\n", chan->dname);
    return;
  }

  putlog(LOG_CMDS, "*", "#%s# (%s) halfop %s", dcc[idx].nick,
         chan->dname, nick);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s.\n", nick, chan->dname);
    return;
  }
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->dname);
  if (chan_dehalfop(victim) || (glob_dehalfop(victim) && !glob_halfop(victim))) {
    dprintf(idx, "%s is currently being auto-dehalfopped.\n", m->nick);
    return;
  }
  if (channel_bitch(chan) && !(chan_op(victim) || chan_op(victim) ||
      (glob_op(victim) && !chan_deop(victim)) || (glob_halfop(victim) &&
      !chan_dehalfop(victim)))) {
    dprintf(idx, "%s is not a registered halfop.\n", m->nick);
    return;
  }
  add_mode(chan, '+', 'h', nick);
  dprintf(idx, "Gave halfop to %s on %s.\n", nick, chan->dname);
}

static void cmd_dehalfop(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  struct userrec *u2;
  char *nick;
  memberlist *m;
  char s[UHOSTLEN];

  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan)
    return;

  if (!nick[0] && !(nick = getnick(u->handle, chan))) {
    dprintf(idx, "Usage: dehalfop <nick> [channel]\n");
    return;
  }

  get_user_flagrec(dcc[idx].user, &user, chan->dname);
  m = ismember(chan, nick);
  if (m && !chan_op(user) && (!glob_op(user) || chan_deop(user))) {
    egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
    u2 = m->user ? m->user : get_user_by_host(s);

    if (!u2 || strcmp(u2->handle, dcc[idx].nick) || (!chan_halfop(user) &&
        (!glob_halfop(user) || chan_dehalfop(user)))) {
      dprintf(idx, "You are not a channel op on %s.\n", chan->dname);
      return;
    }
  }

  if (!channel_active(chan)) {
    dprintf(idx, "I'm not on %s right now!\n", chan->dname);
    return;
  }

  if (HALFOP_CANTDOMODE('h')) {
    dprintf(idx, "I can't help you now because I'm not a chan op or halfop on "
            "%s, or halfops cannot set -h modes.\n", chan->dname);
    return;
  }

  putlog(LOG_CMDS, "*", "#%s# (%s) dehalfop %s", dcc[idx].nick,
         chan->dname, nick);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s.\n", nick, chan->dname);
    return;
  }
  if (match_my_nick(nick)) {
    dprintf(idx, "I'm not going to dehalfop myself.\n");
    return;
  }
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->dname);
  if ((chan_master(victim) || glob_master(victim)) &&
      !(chan_owner(user) || glob_owner(user))) {
    dprintf(idx, "%s is a master for %s.\n", m->nick, chan->dname);
    return;
  }
  if ((chan_op(victim) || (glob_op(victim) && !chan_deop(victim))) &&
      !(chan_master(user) || glob_master(user))) {
    dprintf(idx, "%s has the op flag for %s.\n", m->nick, chan->dname);
    return;
  }
  if ((chan_halfop(victim) || (glob_halfop(victim) &&
      !chan_dehalfop(victim))) && !(chan_master(user) || glob_master(user))) {
    dprintf(idx, "%s has the halfop flag for %s.\n", m->nick, chan->dname);
    return;
  }
  add_mode(chan, '-', 'h', nick);
  dprintf(idx, "Took halfop from %s on %s.\n", nick, chan->dname);
}

static void cmd_voice(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  struct userrec *u2;
  char *nick;
  memberlist *m;
  char s[UHOSTLEN];

  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan)
    return;

  if (!nick[0] && !(nick = getnick(u->handle, chan))) {
    dprintf(idx, "Usage: voice <nick> [channel]\n");
    return;
  }

  get_user_flagrec(dcc[idx].user, &user, chan->dname);
  m = ismember(chan, nick);

  /* By factoring out a !, this code becomes a lot clearer.
   * If you are... not a (channel op, or a channel half op, or a global op
   * without channel deop, or a global halfop without channel dehalfop)...
   * - stdarg */
  if (m && !(chan_op(user) || chan_halfop(user) || (glob_op(user) &&
      !chan_deop(user)) || (glob_halfop(user) && !chan_dehalfop(user)))) {
    egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
    u2 = m->user ? m->user : get_user_by_host(s);

    if (!u2 || strcmp(u2->handle, dcc[idx].nick) || (!chan_voice(user) &&
        (!glob_voice(user) || chan_quiet(user)))) {
      dprintf(idx, "You are not a channel op or halfop on %s.\n", chan->dname);
      return;
    }
  }

  if (!channel_active(chan)) {
    dprintf(idx, "I'm not on %s right now!\n", chan->dname);
    return;
  }

  if (HALFOP_CANTDOMODE('v')) {
    dprintf(idx, "I can't help you now because I'm not a chan op or halfop on "
            "%s, or halfops cannot set +v modes.\n", chan->dname);
    return;
  }

  putlog(LOG_CMDS, "*", "#%s# (%s) voice %s", dcc[idx].nick, chan->dname, nick);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s.\n", nick, chan->dname);
    return;
  }
  add_mode(chan, '+', 'v', nick);
  dprintf(idx, "Gave voice to %s on %s\n", nick, chan->dname);
}

static void cmd_devoice(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  struct userrec *u2;
  char *nick;
  memberlist *m;
  char s[UHOSTLEN];

  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan)
    return;

  if (!nick[0] && !(nick = getnick(u->handle, chan))) {
    dprintf(idx, "Usage: devoice <nick> [channel]\n");
    return;
  }

  get_user_flagrec(dcc[idx].user, &user, chan->dname);
  m = ismember(chan, nick);
  if (m && !(chan_op(user) || chan_halfop(user) || (glob_op(user) &&
      !chan_deop(user)) || (glob_halfop(user) && !chan_dehalfop(user)))) {
    egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
    u2 = m->user ? m->user : get_user_by_host(s);

    if (!u2 || strcmp(u2->handle, dcc[idx].nick) || (!chan_voice(user) &&
        (!glob_voice(user) || chan_quiet(user)))) {
      dprintf(idx, "You are not a channel op or halfop on %s.\n", chan->dname);
      return;
    }
  }

  if (!channel_active(chan)) {
    dprintf(idx, "I'm not on %s right now!\n", chan->dname);
    return;
  }

  if (HALFOP_CANTDOMODE('v')) {
    dprintf(idx, "I can't help you now because I'm not a chan op or halfop on "
            "%s, or halfops cannot set -v modes.\n", chan->dname);
    return;
  }

  putlog(LOG_CMDS, "*", "#%s# (%s) devoice %s", dcc[idx].nick,
         chan->dname, nick);
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s.\n", nick, chan->dname);
    return;
  }
  add_mode(chan, '-', 'v', nick);
  dprintf(idx, "Devoiced %s on %s\n", nick, chan->dname);
}

static void cmd_kick(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *chname, *nick;
  memberlist *m;
  char s[UHOSTLEN];

  if (!par[0]) {
    dprintf(idx, "Usage: kick [channel] <nick> [reason]\n");
    return;
  }
  if (strchr(CHANMETA, par[0]) != NULL)
    chname = newsplit(&par);
  else
    chname = 0;
  chan = get_channel(idx, chname);
  if (!chan || !has_oporhalfop(idx, chan))
    return;
  if (!channel_active(chan)) {
    dprintf(idx, "I'm not on %s right now!\n", chan->dname);
    return;
  }
  if (!me_op(chan) && !me_halfop(chan)) {
    dprintf(idx, "I can't help you now because I'm not a channel op or halfop "
            "on %s.\n", chan->dname);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# (%s) kick %s", dcc[idx].nick, chan->dname, par);
  nick = newsplit(&par);
  if (!par[0])
    par = "request";
  if (match_my_nick(nick)) {
    dprintf(idx, "I'm not going to kick myself.\n");
    return;
  }
  m = ismember(chan, nick);
  if (!m) {
    dprintf(idx, "%s is not on %s\n", nick, chan->dname);
    return;
  }
  if (!me_op(chan) && chan_hasop(m)) {
    dprintf(idx, "I can't help you now because halfops cannot kick ops.\n",
            chan->dname);
    return;
  }
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  get_user_flagrec(u, &victim, chan->dname);
  if ((chan_op(victim) || (glob_op(victim) && !chan_deop(victim))) &&
      !(chan_master(user) || glob_master(user))) {
    dprintf(idx, "%s is a legal op.\n", nick);
    return;
  }
  if ((chan_master(victim) || glob_master(victim)) &&
      !(glob_owner(user) || chan_owner(user))) {
    dprintf(idx, "%s is a %s master.\n", nick, chan->dname);
    return;
  }
  if (glob_bot(victim) && !(glob_owner(user) || chan_owner(user))) {
    dprintf(idx, "%s is another channel bot!\n", nick);
    return;
  }
  dprintf(DP_SERVER, "KICK %s %s :%s\n", chan->name, m->nick, par);
  m->flags |= SENTKICK;
  dprintf(idx, "Okay, done.\n");
}

static void cmd_invite(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  memberlist *m;
  char *nick;

  if (!par[0])
    par = dcc[idx].nick;
  nick = newsplit(&par);
  chan = get_channel(idx, par);
  if (!chan || !has_oporhalfop(idx, chan))
    return;

  putlog(LOG_CMDS, "*", "#%s# (%s) invite %s", dcc[idx].nick, chan->dname,
         nick);
  if (!me_op(chan) && !me_halfop(chan)) {
    if (chan->channel.mode & CHANINV) {
      dprintf(idx, "I can't help you now because I'm not a channel op or "
              "halfop on %s.\n", chan->dname);
      return;
    }
    if (!channel_active(chan)) {
      dprintf(idx, "I'm not on %s right now!\n", chan->dname);
      return;
    }
  }
  m = ismember(chan, nick);
  if (m && !chan_issplit(m)) {
    dprintf(idx, "%s is already on %s!\n", nick, chan->dname);
    return;
  }
  dprintf(DP_SERVER, "INVITE %s %s\n", nick, chan->name);
  dprintf(idx, "Inviting %s to %s.\n", nick, chan->dname);
}

static void cmd_channel(struct userrec *u, int idx, char *par)
{
  char handle[HANDLEN + 1], s[UHOSTLEN], s1[UHOSTLEN], atrflag, chanflag;
  struct chanset_t *chan;
  memberlist *m;
  int maxnicklen, maxhandlen;
  char format[81];

  chan = get_channel(idx, par);
  if (!chan || !has_oporhalfop(idx, chan))
    return;
  putlog(LOG_CMDS, "*", "#%s# (%s) channel", dcc[idx].nick, chan->dname);
  strncpyz(s, getchanmode(chan), sizeof s);
  if (channel_pending(chan))
    egg_snprintf(s1, sizeof s1, "%s %s", IRC_PROCESSINGCHAN, chan->dname);
  else if (channel_active(chan))
    egg_snprintf(s1, sizeof s1, "%s %s", IRC_CHANNEL, chan->dname);
  else
    egg_snprintf(s1, sizeof s1, "%s %s", IRC_DESIRINGCHAN, chan->dname);
  dprintf(idx, "%s, %d member%s, mode %s:\n", s1, chan->channel.members,
          chan->channel.members == 1 ? "" : "s", s);
  if (chan->channel.topic)
    dprintf(idx, "%s: %s\n", IRC_CHANNELTOPIC, chan->channel.topic);
  if (channel_active(chan)) {
    /* find max nicklen and handlen */
    maxnicklen = maxhandlen = 0;
    for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
      if (strlen(m->nick) > maxnicklen)
        maxnicklen = strlen(m->nick);
      if ((m->user) && (strlen(m->user->handle) > maxhandlen))
        maxhandlen = strlen(m->user->handle);
    }
    if (maxnicklen < 9)
      maxnicklen = 9;
    if (maxhandlen < 9)
      maxhandlen = 9;

    dprintf(idx, "(n = owner, m = master, o = op, d = deop, b = bot)\n");
    egg_snprintf(format, sizeof format, " %%-%us %%-%us %%-6s %%-5s %%s\n",
                 maxnicklen, maxhandlen);
    dprintf(idx, format, "NICKNAME", "HANDLE", " JOIN", "IDLE", "USER@HOST");
    for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
      if (m->joined > 0) {
        if ((now - (m->joined)) > 86400)
          egg_strftime(s, 6, "%d%b", localtime(&(m->joined)));
        else
          egg_strftime(s, 6, "%H:%M", localtime(&(m->joined)));
      } else
        strncpyz(s, " --- ", sizeof s);
      if (m->user == NULL) {
        egg_snprintf(s1, sizeof s1, "%s!%s", m->nick, m->userhost);
        m->user = get_user_by_host(s1);
      }
      if (m->user == NULL)
        strncpyz(handle, "*", sizeof handle);
      else
        strncpyz(handle, m->user->handle, sizeof handle);
      get_user_flagrec(m->user, &user, chan->dname);
      /* Determine status char to use */
      if (glob_bot(user) && (glob_op(user) || chan_op(user)))
        atrflag = 'B';
      else if (glob_bot(user))
        atrflag = 'b';
      else if (glob_owner(user))
        atrflag = 'N';
      else if (chan_owner(user))
        atrflag = 'n';
      else if (glob_master(user))
        atrflag = 'M';
      else if (chan_master(user))
        atrflag = 'm';
      else if (glob_deop(user))
        atrflag = 'D';
      else if (chan_deop(user))
        atrflag = 'd';
      else if (glob_dehalfop(user))
        atrflag = 'R';
      else if (chan_dehalfop(user))
        atrflag = 'r';
      else if (glob_autoop(user))
        atrflag = 'A';
      else if (chan_autohalfop(user))
        atrflag = 'y';
      else if (glob_autohalfop(user))
        atrflag = 'Y';
      else if (chan_autoop(user))
        atrflag = 'a';
      else if (glob_op(user))
        atrflag = 'O';
      else if (chan_op(user))
        atrflag = 'o';
      else if (glob_halfop(user))
        atrflag = 'L';
      else if (chan_halfop(user))
        atrflag = 'l';
      else if (glob_quiet(user))
        atrflag = 'Q';
      else if (chan_quiet(user))
        atrflag = 'q';
      else if (glob_gvoice(user))
        atrflag = 'G';
      else if (chan_gvoice(user))
        atrflag = 'g';
      else if (glob_voice(user))
        atrflag = 'V';
      else if (chan_voice(user))
        atrflag = 'v';
      else if (glob_friend(user))
        atrflag = 'F';
      else if (chan_friend(user))
        atrflag = 'f';
      else if (glob_kick(user))
        atrflag = 'K';
      else if (chan_kick(user))
        atrflag = 'k';
      else if (glob_wasoptest(user))
        atrflag = 'W';
      else if (chan_wasoptest(user))
        atrflag = 'w';
      else if (glob_exempt(user))
        atrflag = 'E';
      else if (chan_exempt(user))
        atrflag = 'e';
      else
        atrflag = ' ';
      if (chan_hasop(m))
        chanflag = '@';
      else if (chan_hashalfop(m))
        chanflag = '%';
      else if (chan_hasvoice(m))
        chanflag = '+';
      else
        chanflag = ' ';
      if (chan_issplit(m)) {
        egg_snprintf(format, sizeof format,
                     "%%c%%-%us %%-%us %%s %%c     <- netsplit, %%lus\n",
                     maxnicklen, maxhandlen);
        dprintf(idx, format, chanflag, m->nick, handle, s, atrflag,
                now - (m->split));
      } else if (!rfc_casecmp(m->nick, botname)) {
        egg_snprintf(format, sizeof format,
                     "%%c%%-%us %%-%us %%s %%c     <- it's me!\n",
                     maxnicklen, maxhandlen);
        dprintf(idx, format, chanflag, m->nick, handle, s, atrflag);
      } else {
        /* Determine idle time */
        if (now - (m->last) > 86400)
          egg_snprintf(s1, sizeof s1, "%2lud", ((now - (m->last)) / 86400));
        else if (now - (m->last) > 3600)
          egg_snprintf(s1, sizeof s1, "%2luh", ((now - (m->last)) / 3600));
        else if (now - (m->last) > 180)
          egg_snprintf(s1, sizeof s1, "%2lum", ((now - (m->last)) / 60));
        else
          strncpyz(s1, "   ", sizeof s1);
        egg_snprintf(format, sizeof format,
                     "%%c%%-%us %%-%us %%s %%c %%s  %%s\n", maxnicklen,
                     maxhandlen);
        dprintf(idx, format, chanflag, m->nick, handle, s, atrflag, s1,
                m->userhost);
      }
      if (chan_fakeop(m))
        dprintf(idx, "    (%s)\n", IRC_FAKECHANOP);
      if (chan_sentop(m))
        dprintf(idx, "    (%s)\n", IRC_PENDINGOP);
      if (chan_sentdeop(m))
        dprintf(idx, "    (%s)\n", IRC_PENDINGDEOP);
      if (chan_sentkick(m))
        dprintf(idx, "    (%s)\n", IRC_PENDINGKICK);
    }
  }
  dprintf(idx, "%s\n", IRC_ENDCHANINFO);
}

static void cmd_topic(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;

  if (par[0] && (strchr(CHANMETA, par[0]) != NULL)) {
    char *chname = newsplit(&par);

    chan = get_channel(idx, chname);
  } else
    chan = get_channel(idx, "");

  if (!chan || !has_oporhalfop(idx, chan))
    return;

  if (!channel_active(chan)) {
    dprintf(idx, "I'm not on %s right now!\n", chan->dname);
    return;
  }
  if (!par[0]) {
    if (chan->channel.topic)
      dprintf(idx, "The topic for %s is: %s\n", chan->dname,
              chan->channel.topic);
    else
      dprintf(idx, "No topic is set for %s\n", chan->dname);
  } else if (channel_optopic(chan) && !me_op(chan) && !me_halfop(chan))
    dprintf(idx, "I'm not a channel op or halfop on %s and the channel is "
            "+t.\n", chan->dname);
  else {
    dprintf(DP_SERVER, "TOPIC %s :%s\n", chan->name, par);
    dprintf(idx, "Changing topic...\n");
    putlog(LOG_CMDS, "*", "#%s# (%s) topic %s", dcc[idx].nick,
           chan->dname, par);
  }
}

static void cmd_resetbans(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *chname = newsplit(&par);

  chan = get_channel(idx, chname);
  if (!chan || !has_oporhalfop(idx, chan))
    return;

  putlog(LOG_CMDS, "*", "#%s# (%s) resetbans", dcc[idx].nick, chan->dname);
  dprintf(idx, "Resetting bans on %s...\n", chan->dname);
  resetbans(chan);
}

static void cmd_resetexempts(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *chname = newsplit(&par);

  chan = get_channel(idx, chname);
  if (!chan || !has_oporhalfop(idx, chan))
    return;

  putlog(LOG_CMDS, "*", "#%s# (%s) resetexempts", dcc[idx].nick, chan->dname);
  dprintf(idx, "Resetting exempts on %s...\n", chan->dname);
  resetexempts(chan);
}

static void cmd_resetinvites(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;
  char *chname = newsplit(&par);

  chan = get_channel(idx, chname);
  if (!chan || !has_oporhalfop(idx, chan))
    return;

  putlog(LOG_CMDS, "*", "#%s# (%s) resetinvites", dcc[idx].nick, chan->dname);
  dprintf(idx, "Resetting resetinvites on %s...\n", chan->dname);
  resetinvites(chan);
}

static void cmd_adduser(struct userrec *u, int idx, char *par)
{
  char *nick, *hand;
  struct chanset_t *chan;
  memberlist *m = NULL;
  char s[UHOSTLEN], s1[UHOSTLEN];
  int atr = u ? u->flags : 0;
  int statichost = 0;
  char *p1 = s1;

  if ((!par[0]) || ((par[0] == '!') && (!par[1]))) {
    dprintf(idx, "Usage: adduser <nick> [handle]\n");
    return;
  }
  nick = newsplit(&par);

  /* This flag allows users to have static host (added by drummer, 20Apr99) */
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
      if ((unsigned char) *p <= 32)
        ok = 0;
    if (!ok) {
      dprintf(idx, "You can't have strange characters in a nick.\n");
      return;
    } else if (strchr(BADHANDCHARS, par[0]) != NULL) {
      dprintf(idx, "You can't start a nick with '%c'.\n", par[0]);
      return;
    }
    hand = par;
  }

  for (chan = chanset; chan; chan = chan->next) {
    m = ismember(chan, nick);
    if (m)
      break;
  }
  if (!m) {
    dprintf(idx, "%s is not on any channels I monitor\n", nick);
    return;
  }
  if (strlen(hand) > HANDLEN)
    hand[HANDLEN] = 0;
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  if (u) {
    dprintf(idx, "%s is already known as %s.\n", nick, u->handle);
    return;
  }
  u = get_user_by_handle(userlist, hand);
  if (u && (u->flags & (USER_OWNER | USER_MASTER)) &&
      !(atr & USER_OWNER) && egg_strcasecmp(dcc[idx].nick, hand)) {
    dprintf(idx, "You can't add hostmasks to the bot owner/master.\n");
    return;
  }
  if (!statichost)
    maskhost(s, s1);
  else {
    strncpyz(s1, s, sizeof s1);
    p1 = strchr(s1, '!');
    if (strchr("~^+=-", p1[1])) {
      if (strict_host)
        p1[1] = '?';
      else {
        p1[1] = '!';
        p1++;
      }
    }
    p1--;
    p1[0] = '*';
  }
  if (!u) {
    dprintf(idx, "Added [%s]%s with no password.\n", hand, p1);
    userlist = adduser(userlist, hand, p1, "-", USER_DEFAULT);
  } else {
    dprintf(idx, "Added hostmask %s to %s.\n", p1, u->handle);
    addhost_by_handle(hand, p1);
    get_user_flagrec(u, &user, chan->dname);
    check_this_user(hand, 0, NULL);
  }
  putlog(LOG_CMDS, "*", "#%s# adduser %s %s", dcc[idx].nick, nick,
         hand == nick ? "" : hand);
}

static void cmd_deluser(struct userrec *u, int idx, char *par)
{
  char *nick, s[UHOSTLEN];
  struct chanset_t *chan;
  memberlist *m = NULL;
  struct flag_record victim = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };

  if (!par[0]) {
    dprintf(idx, "Usage: deluser <nick>\n");
    return;
  }
  nick = newsplit(&par);

  for (chan = chanset; chan; chan = chan->next) {
    m = ismember(chan, nick);
    if (m)
      break;
  }
  if (!m) {
    dprintf(idx, "%s is not on any channels I monitor\n", nick);
    return;
  }
  get_user_flagrec(u, &user, chan->dname);
  egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
  u = get_user_by_host(s);
  if (!u) {
    dprintf(idx, "%s is not a valid user.\n", nick);
    return;
  }
  get_user_flagrec(u, &victim, NULL);
  /* This maybe should allow glob +n's to deluser glob +n's but I don't
   * like that - beldin
   */
  /* Checks vs channel owner/master ANYWHERE now -
   * so deluser on a channel they're not on should work
   */
  /* Shouldn't allow people to remove permanent owners (guppy 9Jan1999) */
  if ((glob_owner(victim) && egg_strcasecmp(dcc[idx].nick, nick)) ||
      isowner(u->handle)) {
    dprintf(idx, "You can't remove a bot owner!\n");
  } else if (glob_botmast(victim) && !glob_owner(user)) {
    dprintf(idx, "You can't remove a bot master!\n");
  } else if (chan_owner(victim) && !glob_owner(user)) {
    dprintf(idx, "You can't remove a channel owner!\n");
  } else if (chan_master(victim) && !(glob_owner(user) || chan_owner(user))) {
    dprintf(idx, "You can't remove a channel master!\n");
  } else if (glob_bot(victim) && !glob_owner(user)) {
    dprintf(idx, "You can't remove a bot!\n");
  } else {
    char buf[HANDLEN + 1];

    strncpyz(buf, u->handle, sizeof buf);
    buf[HANDLEN] = 0;
    if (deluser(u->handle)) {
      dprintf(idx, "Deleted %s.\n", buf);       /* ?!?! :) */
      putlog(LOG_CMDS, "*", "#%s# deluser %s [%s]", dcc[idx].nick, nick, buf);
    } else
      dprintf(idx, "Failed.\n");
  }
}

static void cmd_reset(struct userrec *u, int idx, char *par)
{
  struct chanset_t *chan;

  if (par[0]) {
    chan = findchan_by_dname(par);
    if (!chan)
      dprintf(idx, "%s\n", IRC_NOMONITOR);
    else {
      get_user_flagrec(u, &user, par);
      if (!glob_master(user) && !chan_master(user))
        dprintf(idx, "You are not a master on %s.\n", chan->dname);
      else if (!channel_active(chan))
        dprintf(idx, "I'm not on %s at the moment!\n", chan->dname);
      else {
        putlog(LOG_CMDS, "*", "#%s# reset %s", dcc[idx].nick, par);
        dprintf(idx, "Resetting channel info for %s...\n", chan->dname);
        reset_chan_info(chan, CHAN_RESETALL);
      }
    }
  } else if (!(u->flags & USER_MASTER))
    dprintf(idx, "You are not a Bot Master.\n");
  else {
    putlog(LOG_CMDS, "*", "#%s# reset all", dcc[idx].nick);
    dprintf(idx, "Resetting channel info for all channels...\n");
    for (chan = chanset; chan; chan = chan->next) {
      if (channel_active(chan))
        reset_chan_info(chan, CHAN_RESETALL);
    }
  }
}

static cmd_t irc_dcc[] = {
  {"adduser",      "m|m",   (IntFunc) cmd_adduser,      NULL},
  {"deluser",      "m|m",   (IntFunc) cmd_deluser,      NULL},
  {"reset",        "m|m",   (IntFunc) cmd_reset,        NULL},
  {"resetbans",    "o|o",   (IntFunc) cmd_resetbans,    NULL},
  {"resetexempts", "o|o",   (IntFunc) cmd_resetexempts, NULL},
  {"resetinvites", "o|o",   (IntFunc) cmd_resetinvites, NULL},
  {"act",          "o|o",   (IntFunc) cmd_act,          NULL},
  {"channel",      "o|o",   (IntFunc) cmd_channel,      NULL},
  {"op",           "o|o",   (IntFunc) cmd_op,           NULL},
  {"deop",         "o|o",   (IntFunc) cmd_deop,         NULL},
  {"halfop",       "ol|ol", (IntFunc) cmd_halfop,       NULL},
  {"dehalfop",     "ol|ol", (IntFunc) cmd_dehalfop,     NULL},
  {"voice",        "ov|ov", (IntFunc) cmd_voice,        NULL},
  {"devoice",      "ov|ov", (IntFunc) cmd_devoice,      NULL},
  {"invite",       "o|o",   (IntFunc) cmd_invite,       NULL},
  {"kick",         "lo|lo", (IntFunc) cmd_kick,         NULL},
  {"kickban",      "lo|lo", (IntFunc) cmd_kickban,      NULL},
  {"msg",          "o",     (IntFunc) cmd_msg,          NULL},
  {"say",          "o|o",   (IntFunc) cmd_say,          NULL},
  {"topic",        "lo|lo", (IntFunc) cmd_topic,        NULL},
  {NULL,           NULL,    NULL,                        NULL}
};
