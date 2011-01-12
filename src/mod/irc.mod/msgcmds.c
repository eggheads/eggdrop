/*
 * msgcmds.c -- part of irc.mod
 *   all commands entered via /MSG
 *
 * $Id: msgcmds.c,v 1.3.2.3 2011/01/12 13:54:01 pseudo Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2010 Eggheads Development Team
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

static int msg_hello(char *nick, char *h, struct userrec *u, char *p)
{
  char host[UHOSTLEN], s[UHOSTLEN], s1[UHOSTLEN], handle[HANDLEN + 1];
  char *p1;
  int common = 0;
  int atr = 0;
  struct chanset_t *chan;

  if (!learn_users && !make_userfile)
    return 0;

  if (match_my_nick(nick))
    return 1;

  if (u) {
    atr = u->flags;
    if (!(atr & USER_COMMON)) {
      dprintf(DP_HELP, _("NOTICE %s :Hi, %s.\n"), nick, u->handle);
      return 1;
    }
  }
  strncpyz(handle, nick, sizeof(handle));
  if (get_user_by_handle(userlist, handle)) {
    dprintf(DP_HELP, "NOTICE %s :%s.\n", nick, _("I don't recognize you "
            "from that host."));
    dprintf(DP_HELP, _("NOTICE %s :Either you are using someone else's "
            "nickname or you need to type: /MSG %s IDENT (password)\n"),
            nick, botname);
    return 1;
  }
  egg_snprintf(s, sizeof s, "%s!%s", nick, h);
  if (u_match_mask(global_bans, s)) {
    dprintf(DP_HELP, "NOTICE %s :%s.\n", nick, _("You're banned, goober."));
    return 1;
  }
  if (atr & USER_COMMON) {
    maskhost(s, host);
    strcpy(s, host);
    egg_snprintf(host, sizeof host, "%s!%s", nick, s + 2);
    userlist = adduser(userlist, handle, host, "-", USER_DEFAULT);
    putlog(LOG_MISC, "*", _("Introduced to %s (%s) -- common site"),
           nick, host);
    common = 1;
  } else {
    maskhost(s, host);
    if (make_userfile) {
      userlist = adduser(userlist, handle, host, "-",
                 sanity_check(default_flags | USER_MASTER | USER_OWNER));
      set_user(&USERENTRY_HOSTS, get_user_by_handle(userlist, handle),
               "-telnet!*@*");
    } else
      userlist = adduser(userlist, handle, host, "-",
                         sanity_check(default_flags));
    putlog(LOG_MISC, "*", _("Introduced to %s (%s)"), nick, host);
  }
  for (chan = chanset; chan; chan = chan->next)
    if (ismember(chan, handle))
      add_chanrec_by_handle(userlist, handle, chan->dname);
  dprintf(DP_HELP, _("NOTICE %s :Hi %s!  I'm %s, an eggdrop bot.\n"),
          nick, nick, botname);
  dprintf(DP_HELP, _("NOTICE %s :I'll recognize you by hostmask '%s' "
          "from now on.\n"), nick, host);
  if (common)
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Since you come from a common "
            "irc site, this means you should\n"
            "always use this nickname when talking to me."));
  if (make_userfile) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("YOU ARE THE OWNER ON THIS "
            "BOT NOW"));
    dprintf(DP_HELP, _("NOTICE %s :As master you really need to "
            "set a password: with /MSG %s pass <your-chosen-password>."),
            nick, botname);
    dprintf(DP_HELP, "NOTICE %s :%s\n", _("All major commands are used from "
            "DCC chat. From now on, you don't need to use the -m option when "
            "starting the bot.  Enjoy !!!"));
    putlog(LOG_MISC, "*", _("Bot installation complete, first master is %s"),
           handle);
    make_userfile = 0;
    write_userfile(-1);
    add_note(handle, botnetnick, _("Welcome to Eggdrop! =]"), -1, 0);
  } else {
    dprintf(DP_HELP, _("NOTICE %s :All commands are done via /MSG. For the "
            "complete list, /MSG %s help  Cya!\n"), nick, botname);
  }
  if (strlen(nick) > HANDLEN)
    /* Notify the user that his/her handle was truncated. */
    dprintf(DP_HELP, _("NOTICE %s :Your nick was too long and therefore it "
            "was truncated to '%s'.\n"), nick, handle);
  if (notify_new[0]) {
    egg_snprintf(s, sizeof s, _("introduced to %s from %s"), nick, host);
    strcpy(s1, notify_new);
    while (s1[0]) {
      p1 = strchr(s1, ',');
      if (p1 != NULL) {
        *p1 = 0;
        p1++;
        rmspace(p1);
      }
      rmspace(s1);
      add_note(s1, botnetnick, s, -1, 0);
      if (p1 == NULL)
        s1[0] = 0;
      else
        strcpy(s1, p1);
    }
  }
  return 1;
}

static int msg_pass(char *nick, char *host, struct userrec *u, char *par)
{
  char *old, *new;

  if (!u || match_my_nick(nick) || (u->flags & (USER_BOT | USER_COMMON)))
    return 1;

  if (!par[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick,
            u_pass_match(u, "-") ? _("You don't have a password set.") :
            _("You have a password set."));
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! PASS?", nick, host, u->handle);
    return 1;
  }
  old = newsplit(&par);
  if (!u_pass_match(u, "-") && !par[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick,
            _("You already have a password set."));
    return 1;
  }
  if (par[0]) {
    if (!u_pass_match(u, old)) {
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Incorrect password."));
      return 1;
    }
    new = newsplit(&par);
  } else
    new = old;
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! PASS...", nick, host, u->handle);
  if (strlen(new) > 15)
    new[15] = 0;
  if (strlen(new) < 6) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Please use at least "
            "6 characters."));
    return 0;
  }
  set_user(&USERENTRY_PASS, u, new);
  dprintf(DP_HELP, "NOTICE %s :%s '%s'.\n", nick,
          new == old ? _("Password set to:") : _("Password changed to:"), new);
  return 1;
}

static int msg_ident(char *nick, char *host, struct userrec *u, char *par)
{
  char s[UHOSTLEN], s1[UHOSTLEN], *pass, who[NICKLEN];
  struct userrec *u2;

  if (match_my_nick(nick) || (u && (u->flags & USER_BOT)))
    return 1;

  if (u && (u->flags & USER_COMMON)) {
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick,
              _("You're at a common site; you can't IDENT."));
    return 1;
  }
  pass = newsplit(&par);
  if (!par[0])
    strcpy(who, nick);
  else {
    strncpy(who, par, NICKMAX);
    who[NICKMAX] = 0;
  }
  u2 = get_user_by_handle(userlist, who);
  if (!u2) {
    if (u && !quiet_reject)
      dprintf(DP_HELP, _("NOTICE %s :You're not %s, you're %s.\n"), nick,
              nick, u->handle);
  } else if (rfc_casecmp(who, origbotname) && !(u2->flags & USER_BOT)) {
    /* This could be used as detection... */
    if (u_pass_match(u2, "-")) {
      putlog(LOG_CMDS, "*", "(%s!%s) !*! IDENT %s", nick, host, who);
      if (!quiet_reject)
        dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You don't have "
                "a password set."));
    } else if (!u_pass_match(u2, pass)) {
      if (!quiet_reject)
        dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Access denied."));
    } else if (u == u2) {
      /*
       * NOTE: Checking quiet_reject *after* u_pass_match()
       * verifies the password makes NO sense!
       * (Broken since 1.3.0+bel17)  Bad Beldin! No Cookie!
       *   -Toth  [July 30, 2003]
       */
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("I recognize you there."));
      return 1;
    } else if (u) {
      dprintf(DP_HELP, _("NOTICE %s :You're not %s  you're %s.\n"), nick, who,
              u->handle);
      return 1;
    } else {
      putlog(LOG_CMDS, "*", "(%s!%s) !*! IDENT %s", nick, host, who);
      egg_snprintf(s, sizeof s, "%s!%s", nick, host);
      maskhost(s, s1);
      dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, _("Added hostmask"), s1);
      addhost_by_handle(who, s1);
      check_this_user(who, 0, NULL);
      return 1;
    }
  }
  putlog(LOG_CMDS, "*", _("(%s!%s) !*! failed IDENT %s"), nick, host, who);
  return 1;
}

static int msg_addhost(char *nick, char *host, struct userrec *u, char *par)
{
  char *pass;

  if (match_my_nick(nick) || !u || (u->flags & USER_BOT))
    return 1;

  if (u->flags & USER_COMMON) {
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick,
              _("You're at a common site; you can't IDENT."));
    return 1;
  }
  pass = newsplit(&par);
  if (!par[0]) {
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("NOTICE %s :You must supply "
              "a hostmask"));
  } else if (rfc_casecmp(u->handle, origbotname)) {
    /* This could be used as detection... */
    if (u_pass_match(u, "-")) {
      if (!quiet_reject)
        dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You don't have a password set."));
    } else if (!u_pass_match(u, pass)) {
      if (!quiet_reject)
        dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Access denied."));
    } else if (get_user_by_host(par)) {
      if (!quiet_reject)
        dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("That hostmask clashes with "
                "another already in use."));
    } else {
      putlog(LOG_CMDS, "*", "(%s!%s) !*! ADDHOST %s", nick, host, par);
      dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, _("Added hostmask"), par);
      addhost_by_handle(u->handle, par);
      check_this_user(u->handle, 0, NULL);
      return 1;
    }
  }
  putlog(LOG_CMDS, "*", _("(%s!%s) !*! failed ADDHOST %s"), nick, host, par);
  return 1;
}

static int msg_info(char *nick, char *host, struct userrec *u, char *par)
{
  char s[121], *pass, *chname, *p;
  int locked = 0;

  if (!use_info || match_my_nick(nick))
    return 1;

  if (!u)
    return 0;

  if (u->flags & (USER_COMMON | USER_BOT))
    return 1;

  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed INFO", nick, host, u->handle);
      return 1;
    }
  } else {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! failed INFO", nick, host, u->handle);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You don't have "
              "a password set."));
    return 1;
  }
  if (par[0] && (strchr(CHANMETA, par[0]) != NULL)) {
    if (!findchan_by_dname(chname = newsplit(&par))) {
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("I don't monitor "
              "that channel."));
      return 1;
    }
  } else
    chname = 0;
  if (par[0]) {
    p = get_user(&USERENTRY_INFO, u);
    if (p && (p[0] == '@'))
      locked = 1;
    if (chname) {
      get_handle_chaninfo(u->handle, chname, s);
      if (s[0] == '@')
        locked = 1;
    }
    if (locked) {
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Your info line is locked"));
      return 1;
    }
    if (!egg_strcasecmp(par, "none")) {
      par[0] = 0;
      if (chname) {
        set_handle_chaninfo(userlist, u->handle, chname, NULL);
        dprintf(DP_HELP, "NOTICE %s :%s %s.\n", nick,
                _("Removed your info line on"), chname);
        putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO %s NONE", nick, host,
               u->handle, chname);
      } else {
        set_user(&USERENTRY_INFO, u, NULL);
        dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Removed your info line."));
        putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO NONE", nick, host, u->handle);
      }
      return 1;
    }
    if (par[0] == '@')
      par++;
    dprintf(DP_HELP, "NOTICE %s :%s %s\n", nick, _("Now:"), par);
    if (chname) {
      set_handle_chaninfo(userlist, u->handle, chname, par);
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO %s ...", nick, host, u->handle,
             chname);
    } else {
      set_user(&USERENTRY_INFO, u, par);
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO ...", nick, host, u->handle);
    }
    return 1;
  }
  if (chname) {
    get_handle_chaninfo(u->handle, chname, s);
    p = s;
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO? %s", nick, host, u->handle,
           chname);
  } else {
    p = get_user(&USERENTRY_INFO, u);
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! INFO?", nick, host, u->handle);
  }
  if (p && p[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s %s\n", nick, _("Currently:"), p);
    dprintf(DP_HELP, "NOTICE %s :%s /msg %s info <pass>%s%s none\n",
            nick, _("To remove it:"), botname, chname ? " " : "", chname
            ? chname : "");
  } else {
    if (chname)
      dprintf(DP_HELP, "NOTICE %s :%s %s.\n", nick,
              _("You have no info set on"), chname);
    else
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You have no info set."));
  }
  return 1;
}

static int msg_who(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };
  memberlist *m;
  char s[UHOSTLEN], also[512], *info;
  int i;

  if (!use_info || match_my_nick(nick))
    return 1;

  if (!u)
    return 0;

  if (!par[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s: /msg %s who <channel>\n", nick,
            _("Usage"), botname);
    return 0;
  }
  chan = findchan_by_dname(par);
  if (!chan) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick,
            _("I don't monitor that channel."));
    return 0;
  }
  get_user_flagrec(u, &fr, par);
  if (channel_hidden(chan) && !hand_on_chan(chan, u) &&
      !glob_op(fr) && !glob_friend(fr) && !chan_op(fr) && !chan_friend(fr)) {
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Channel is currently "
            "hidden."));
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! WHO", nick, host, u->handle);
  also[0] = 0;
  i = 0;
  for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
    struct userrec *u;

    egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
    u = get_user_by_host(s);
    info = get_user(&USERENTRY_INFO, u);
    if (u && (u->flags & USER_BOT))
      info = 0;
    if (info && (info[0] == '@'))
      info++;
    else if (u) {
      get_handle_chaninfo(u->handle, chan->dname, s);
      if (s[0]) {
        info = s;
        if (info[0] == '@')
          info++;
      }
    }
    if (info && info[0])
      dprintf(DP_HELP, "NOTICE %s :[%9s] %s\n", nick, m->nick, info);
    else {
      if (match_my_nick(m->nick))
        dprintf(DP_HELP, "NOTICE %s :[%9s] %s\n",
                nick, m->nick, _("<-- I'm the bot, of course."));
      else if (u && (u->flags & USER_BOT)) {
        if (bot_flags(u) & BOT_SHARE)
          dprintf(DP_HELP, "NOTICE %s :[%9s] %s\n", nick, m->nick,
                  _("<-- a twin of me"));
        else
          dprintf(DP_HELP, "NOTICE %s :[%9s] %s\n", nick, m->nick,
                  _("<-- another bot"));
      } else {
        if (i) {
          also[i++] = ',';
          also[i++] = ' ';
        }
        i += my_strcpy(also + i, m->nick);
        if (i > 400) {
          dprintf(DP_HELP, _("NOTICE %s :No info: %s\n"), nick, also);
          i = 0;
          also[0] = 0;
        }
      }
    }
  }
  if (i) {
    dprintf(DP_HELP, _("NOTICE %s :No info: %s\n"), nick, also);
  }
  return 1;
}

static int msg_whois(char *nick, char *host, struct userrec *u, char *par)
{
  char s[UHOSTLEN], s1[81], *s2;
  int ok;
  struct chanset_t *chan;
  memberlist *m;
  struct chanuserrec *cr;
  struct userrec *u2;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };
  struct xtra_key *xk;
  time_t tt = 0;

  if (match_my_nick(nick))
    return 1;

  if (!u)
    return 0;

  if (!par[0]) {
    dprintf(DP_HELP, "NOTICE %s :%s: /msg %s whois <handle>\n", nick,
            _("Usage"), botname);
    return 0;
  }
  if (strlen(par) > NICKMAX)
    par[NICKMAX] = 0;
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! WHOIS %s", nick, host, u->handle, par);
  u2 = get_user_by_handle(userlist, par);
  if (!u2) {
    /* No such handle -- maybe it's a nickname of someone on a chan? */
    ok = 0;
    for (chan = chanset; chan && !ok; chan = chan->next) {
      m = ismember(chan, par);
      if (m) {
        egg_snprintf(s, sizeof s, "%s!%s", par, m->userhost);
        u2 = get_user_by_host(s);
        if (u2) {
          ok = 1;
          dprintf(DP_HELP, "NOTICE %s :[%s] AKA '%s':\n", nick,
                  par, u2->handle);
        }
      }
    }
    if (!ok) {
      dprintf(DP_HELP, "NOTICE %s :[%s] %s\n", nick, par,
              _("No such user record."));
      return 1;
    }
  }
  s2 = get_user(&USERENTRY_INFO, u2);
  if (s2 && (s2[0] == '@'))
    s2++;
  if (s2 && s2[0] && !(u2->flags & USER_BOT))
    dprintf(DP_HELP, "NOTICE %s :[%s] %s\n", nick, u2->handle, s2);
  for (xk = get_user(&USERENTRY_XTRA, u2); xk; xk = xk->next)
    if (!egg_strcasecmp(xk->key, "EMAIL"))
      dprintf(DP_HELP, "NOTICE %s :[%s] E-mail: %s\n", nick, u2->handle,
              xk->data);
  ok = 0;
  for (chan = chanset; chan; chan = chan->next) {
    if (hand_on_chan(chan, u2)) {
      egg_snprintf(s1, sizeof s1, "NOTICE %s :[%s] %s: %s.", nick, u2->handle,
                   _("Now on channel"), chan->dname);
      ok = 1;
    } else {
      get_user_flagrec(u, &fr, chan->dname);
      cr = get_chanrec(u2, chan->dname);
      if (cr && (cr->laston > tt) && (!channel_hidden(chan) ||
          hand_on_chan(chan, u) || (glob_op(fr) && !chan_deop(fr)) ||
          glob_friend(fr) || chan_op(fr) || chan_friend(fr))) {
        tt = cr->laston;
        strftime(s, 14, "%b %d %H:%M", localtime(&tt));
        ok = 1;
        egg_snprintf(s1, sizeof s1, "NOTICE %s :[%s] %s %s on %s", nick,
                     u2->handle, _("Last seen at"), s, chan->dname);
      }
    }
  }
  if (!ok)
    egg_snprintf(s1, sizeof s1, "NOTICE %s :[%s] %s", nick, u2->handle,
                 _("Never seen on channel."));
  dprintf(DP_HELP, "%s\n", s1);
  if (u2->flags & USER_BOT)
    dprintf(DP_HELP, "NOTICE %s :[%s] %s\n", nick, u2->handle,
            _("Status: bot"));
  else if (u2->flags & USER_OWNER)
    dprintf(DP_HELP, "NOTICE %s :[%s] %s\n", nick, u2->handle,
            _("Status: global owner"));
  else if (u2->flags & USER_MASTER)
    dprintf(DP_HELP, "NOTICE %s :[%s] %s\n", nick, u2->handle,
            _("Status: botnet master"));
  else if (u2->flags & USER_BOTMAST)
    dprintf(DP_HELP, "NOTICE %s :[%s] %s\n", nick, u2->handle,
            _("Status: botnet master"));
  else if (u2->flags & USER_OP)
    dprintf(DP_HELP, "NOTICE %s :[%s] %s\n", nick, u2->handle,
            _("Status: global op"));
  else if (u2->flags & USER_VOICE)
    dprintf(DP_HELP, "NOTICE %s :[%s] %s\n", nick, u2->handle,
            _("Status: global voice"));
  return 1;
}

static int msg_help(char *nick, char *host, struct userrec *u, char *par)
{
  char *p;

  if (match_my_nick(nick))
    return 1;

  if (!u) {
    if (!quiet_reject) {
      if (!learn_users)
        dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("No access"));
      else {
        dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("I don't know you; "
                "please introduce yourself first."));
        dprintf(DP_HELP, "NOTICE %s :/MSG %s hello\n", nick, botname);
      }
    }
    return 0;
  }

  if (helpdir[0]) {
    struct flag_record fr = { FR_ANYWH | FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

    get_user_flagrec(u, &fr, 0);
    if (!par[0])
      showhelp(nick, "help", &fr, 0);
    else {
      for (p = par; *p != 0; p++)
        if ((*p >= 'A') && (*p <= 'Z'))
          *p += ('a' - 'A');
      showhelp(nick, par, &fr, 0);
    }
  } else
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("No help."));

  return 1;
}

static int msg_op(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  if (match_my_nick(nick))
    return 1;

  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    if (!u_pass_match(u, "-")) {
      if (par[0]) {
        chan = findchan_by_dname(par);
        if (chan && channel_active(chan)) {
          get_user_flagrec(u, &fr, par);
          if (chan_op(fr) || (glob_op(fr) && !chan_deop(fr)))
            add_mode(chan, '+', 'o', nick);
          putlog(LOG_CMDS, "*", "(%s!%s) !%s! OP %s", nick, host, u->handle,
                 par);
          return 1;
        }
      } else {
        for (chan = chanset; chan; chan = chan->next) {
          get_user_flagrec(u, &fr, chan->dname);
          if (chan_op(fr) || (glob_op(fr) && !chan_deop(fr)))
            add_mode(chan, '+', 'o', nick);
        }
        putlog(LOG_CMDS, "*", "(%s!%s) !%s! OP", nick, host, u->handle);
        return 1;
      }
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !*! failed OP", nick, host);
  return 1;
}

static int msg_halfop(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  if (match_my_nick(nick))
    return 1;

  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    if (!u_pass_match(u, "-")) {
      if (par[0]) {
        chan = findchan_by_dname(par);
        if (chan && channel_active(chan)) {
          get_user_flagrec(u, &fr, par);
          if (chan_op(fr) || chan_halfop(fr) || (glob_op(fr) &&
              !chan_deop(fr)) || (glob_halfop(fr) && !chan_dehalfop(fr)))
            add_mode(chan, '+', 'h', nick);
          putlog(LOG_CMDS, "*", "(%s!%s) !%s! HALFOP %s",
                 nick, host, u->handle, par);
          return 1;
        }
      } else {
        for (chan = chanset; chan; chan = chan->next) {
          get_user_flagrec(u, &fr, chan->dname);
          if (chan_op(fr) || chan_halfop(fr) || (glob_op(fr) &&
              !chan_deop(fr)) || (glob_halfop(fr) && !chan_dehalfop(fr)))
            add_mode(chan, '+', 'h', nick);
        }
        putlog(LOG_CMDS, "*", "(%s!%s) !%s! HALFOP", nick, host, u->handle);
        return 1;
      }
    }
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !*! failed HALFOP", nick, host);
  return 1;
}

static int msg_key(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  if (match_my_nick(nick))
    return 1;

  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    /* Prevent people from getting key with no pass set */
    if (!u_pass_match(u, "-")) {
      if (!(chan = findchan_by_dname(par))) {
        dprintf(DP_HELP, "NOTICE %s :%s: /MSG %s key <pass> <channel>\n",
                nick, _("Usage"), botname);
        return 1;
      }
      if (!channel_active(chan)) {
        dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, par,
                _("Not on that channel right now."));
        return 1;
      }
      chan = findchan_by_dname(par);
      if (chan && channel_active(chan)) {
        get_user_flagrec(u, &fr, par);
        if (chan_op(fr) || chan_halfop(fr) || (glob_op(fr) && !chan_deop(fr)) ||
            (glob_halfop(fr) && !chan_dehalfop(fr))) {
          if (chan->channel.key[0]) {
            dprintf(DP_SERVER, "NOTICE %s :%s: key is %s\n", nick, par,
                    chan->channel.key);
            putlog(LOG_CMDS, "*", "(%s!%s) !%s! KEY %s", nick, host, u->handle,
                   par);
            if (invite_key && (chan->channel.mode & CHANINV)) {
              dprintf(DP_SERVER, "INVITE %s %s\n", nick, chan->name);
            }
          } else {
            dprintf(DP_HELP, "NOTICE %s :%s: %s\n",
                    nick, par, _("no key set for this channel"));
            putlog(LOG_CMDS, "*", "(%s!%s) !%s! KEY %s", nick, host, u->handle,
                   par);
          }
        }
        return 1;
      }
    }
  }
  putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed KEY %s"), nick, host,
         (u ? u->handle : "*"), par);
  return 1;
}

/* Don't have to specify a channel now and can use this command
 * regardless of +autovoice or being a chanop. (guppy 7Jan1999)
 */
static int msg_voice(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  if (match_my_nick(nick))
    return 1;

  pass = newsplit(&par);
  if (u_pass_match(u, pass)) {
    if (!u_pass_match(u, "-")) {
      if (par[0]) {
        chan = findchan_by_dname(par);
        if (chan && channel_active(chan)) {
          get_user_flagrec(u, &fr, par);
          if (chan_voice(fr) || glob_voice(fr) || chan_op(fr) || glob_op(fr)) {
            add_mode(chan, '+', 'v', nick);
            putlog(LOG_CMDS, "*", "(%s!%s) !%s! VOICE %s", nick, host,
                   u->handle, par);
          } else
            putlog(LOG_CMDS, "*", _("(%s!%s) !*! failed VOICE %s"),
                   nick, host, par);
          return 1;
        }
      } else {
        for (chan = chanset; chan; chan = chan->next) {
          get_user_flagrec(u, &fr, chan->dname);
          if (chan_voice(fr) || glob_voice(fr) ||
              chan_op(fr) || glob_op(fr) || chan_halfop(fr) || glob_halfop(fr))
            add_mode(chan, '+', 'v', nick);
        }
        putlog(LOG_CMDS, "*", "(%s!%s) !%s! VOICE", nick, host, u->handle);
        return 1;
      }
    }
  }
  putlog(LOG_CMDS, "*", _("(%s!%s) !*! failed VOICE"), nick, host);
  return 1;
}

static int msg_invite(char *nick, char *host, struct userrec *u, char *par)
{
  char *pass;
  struct chanset_t *chan;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  if (match_my_nick(nick))
    return 1;

  pass = newsplit(&par);
  if (u_pass_match(u, pass) && !u_pass_match(u, "-")) {
    if (par[0] == '*') {
      for (chan = chanset; chan; chan = chan->next) {
        get_user_flagrec(u, &fr, chan->dname);
        if ((chan_op(fr) || chan_halfop(fr) || (glob_op(fr) &&
            !chan_deop(fr)) || (glob_halfop(fr) && !chan_dehalfop(fr))) &&
            (chan->channel.mode & CHANINV))
          dprintf(DP_SERVER, "INVITE %s %s\n", nick, chan->name);
      }
      putlog(LOG_CMDS, "*", _("(%s!%s) !%s! INVITE ALL"), nick, host,
             u->handle);
      return 1;
    }
    if (!(chan = findchan_by_dname(par))) {
      dprintf(DP_HELP, "NOTICE %s :%s: /MSG %s invite <pass> <channel>\n",
              nick, _("Usage"), botname);
      return 1;
    }
    if (!channel_active(chan)) {
      dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, par,
              _("Not on that channel right now."));
      return 1;
    }
    /* We need to check access here also (dw 991002) */
    get_user_flagrec(u, &fr, par);
    if (chan_op(fr) || chan_halfop(fr) || (glob_op(fr) && !chan_deop(fr)) ||
        (glob_halfop(fr) && !chan_dehalfop(fr))) {
      dprintf(DP_SERVER, "INVITE %s %s\n", nick, chan->name);
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! INVITE %s", nick, host,
             u->handle, par);
      return 1;
    }
  }
  putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed INVITE %s"), nick, host,
         (u ? u->handle : "*"), par);
  return 1;
}

static int msg_status(char *nick, char *host, struct userrec *u, char *par)
{
  char s[256], *vers_t, *uni_t, *pass;
  int i;
  struct chanset_t *chan;
  time_t now2 = now - online_since, hr, min;

#ifdef HAVE_UNAME
  struct utsname un;

  if (uname(&un) < 0) {
#endif
    vers_t = " ";
    uni_t  = "*unknown*";
#ifdef HAVE_UNAME
  } else {
    vers_t = un.release;
    uni_t  = un.sysname;
  }
#endif

  if (match_my_nick(nick))
    return 1;

  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed STATUS"), nick, host,
             u->handle);
      return 1;
    }
  } else {
    putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed STATUS"), nick, host, u->handle);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You don't have "
              "a password set."));
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! STATUS", nick, host, u->handle);

  i = count_users(userlist);
  dprintf(DP_HELP, _("NOTICE %s :I am %s, running %s: %d %s  "
          "(mem: %uk).\n"), nick, botnetnick, ver, i, P_("user", "users", i),
         (int) (expected_memory() / 1024));

  s[0] = 0;
  if (now2 > 86400) {
    /* days */
    sprintf(s, "%d day", (int) (now2 / 86400));
    if ((int) (now2 / 86400) >= 2)
      strcat(s, "s");
    strcat(s, ", ");
    now2 -= (((int) (now2 / 86400)) * 86400);
  }
  hr = (time_t) ((int) now2 / 3600);
  now2 -= (hr * 3600);
  min = (time_t) ((int) now2 / 60);
  sprintf(&s[strlen(s)], "%02d:%02d", (int) hr, (int) min);
  dprintf(DP_HELP, "NOTICE %s :%s %s.\n", nick, _("Online for"), s);

  if (admin[0])
    dprintf(DP_HELP, _("NOTICE %s :Admin: %s.\n"), nick, admin);
  dprintf(DP_HELP, _("NOTICE %s :OS: %s %s.\n"), nick, uni_t, vers_t);
  dprintf(DP_HELP, _("NOTICE %s :Online as: %s!%s.\n"), nick, botname,
          botuserhost);

  /* This shouldn't overflow anymore -Wcc */
  s[0] = 0;
  strncpyz(s, _("Channels: "), sizeof s);
  for (chan = chanset; chan; chan = chan->next) {
    strncat(s, chan->dname, sizeof(s) - 1 - strlen(s));
    if (!channel_active(chan))
      strncat(s, _(" (trying)"), sizeof(s) - 1 - strlen(s));
    else if (channel_pending(chan))
      strncat(s, _(" (pending)"), sizeof(s) - 1 - strlen(s));
    else if (!me_op(chan))
      strncat(s, _(" (need ops)"), sizeof(s) - 1 - strlen(s));
    strncat(s, ", ", sizeof(s) - 1 - strlen(s));
    if (strlen(s) > 140) {
      s[strlen(s) - 2] = 0; /* remove ', ' */
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, s);
      s[0] = 0;
    }
  }
  if (strlen(s) > 10) {
    s[strlen(s) - 2] = 0; /* remove ', ' */
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, s);
  }
  return 1;
}

static int msg_memory(char *nick, char *host, struct userrec *u, char *par)
{
  char *pass;

  if (match_my_nick(nick))
    return 1;

  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed MEMORY"), nick, host,
             u->handle);
      return 1;
    }
  } else {
    putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed MEMORY"), nick, host,
           u->handle);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You don't have "
              "a password set."));
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! MEMORY", nick, host, u->handle);
  tell_mem_status(nick);
  return 1;
}

static int msg_die(char *nick, char *host, struct userrec *u, char *par)
{
  char s[1024];
  char *pass;

  if (match_my_nick(nick))
    return 1;

  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed DIE"), nick, host, u->handle);
      return 1;
    }
  } else {
    putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed DIE"), nick, host, u->handle);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You don't have "
              "a password set."));
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! DIE", nick, host, u->handle);
  dprintf(-serv, "NOTICE %s :%s\n", nick, _("Bot shut down beginning...."));
  if (!par[0])
    egg_snprintf(s, sizeof s, _("BOT SHUTDOWN (authorized by %s)"), u->handle);
  else
    egg_snprintf(s, sizeof s, _("BOT SHUTDOWN (%s: %s)"), u->handle, par);
  chatout("*** %s\n", s);
  botnet_send_chat(-1, botnetnick, s);
  botnet_send_bye();
  if (!par[0])
    nuke_server(nick);
  else
    nuke_server(par);
  write_userfile(-1);
  sleep(1);                     /* Give the server time to understand */
  egg_snprintf(s, sizeof s, _("DEAD BY REQUEST OF %s!%s"), nick, host);
  fatal(s, 0);
  return 1;
}

static int msg_rehash(char *nick, char *host, struct userrec *u, char *par)
{
  if (match_my_nick(nick))
    return 1;

  if (u_pass_match(u, par)) {
    putlog(LOG_CMDS, "*", _("(%s!%s) !%s! REHASH"), nick, host, u->handle);
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Rehashing..."));
    if (make_userfile)
      make_userfile = 0;
    write_userfile(-1);
    do_restart = -2;
    return 1;
  }
  putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed REHASH"), nick, host, u->handle);
  return 1;
}

static int msg_save(char *nick, char *host, struct userrec *u, char *par)
{
  if (match_my_nick(nick))
    return 1;

  if (u_pass_match(u, par)) {
    putlog(LOG_CMDS, "*", _("(%s!%s) !%s! SAVE"), nick, host, u->handle);
    dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Saving user file..."));
    write_userfile(-1);
    return 1;
  }
  putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed SAVE"), nick, host, u->handle);
  return 1;
}

static int msg_reset(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  char *pass;

  if (match_my_nick(nick))
    return 1;

  if (!u_pass_match(u, "-")) {
    pass = newsplit(&par);
    if (!u_pass_match(u, pass)) {
      putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed RESET"), nick, host,
             u->handle);
      return 1;
    }
  } else {
    putlog(LOG_CMDS, "*", _("(%s!%s) !*! failed RESET"), nick, host);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You don't have "
              "a password set."));
    return 1;
  }
  if (par[0]) {
    chan = findchan_by_dname(par);
    if (!chan) {
      dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, par, _("I don't monitor "
              "that channel."));
      return 0;
    }
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! RESET %s", nick, host, u->handle, par);
    dprintf(DP_HELP, "NOTICE %s :%s: %s\n", nick, par,
            _("Resetting channel info."));
    reset_chan_info(chan, CHAN_RESETALL);
    return 1;
  }
  putlog(LOG_CMDS, "*", "(%s!%s) !%s! RESET ALL", nick, host, u->handle);
  dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("Resetting channel info."));
  for (chan = chanset; chan; chan = chan->next)
    reset_chan_info(chan, CHAN_RESETALL);
  return 1;
}

static int msg_go(char *nick, char *host, struct userrec *u, char *par)
{
  struct chanset_t *chan;
  int ok = 0, ok2 = 0;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  if (match_my_nick(nick))
    return 1;

  if (!u)
    return 0;

  if (par[0]) {
    chan = findchan_by_dname(par);
    if (!chan)
      return 0;
    if (!(chan->status & CHAN_ACTIVE)) {
      putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed GO %s (i'm blind)"), nick,
             host, u->handle, par);
      return 1;
    }
    get_user_flagrec(u, &fr, par);
    if (!chan_op(fr) && !(glob_op(fr) && !chan_deop(fr))) {
      putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed GO %s (not op)"), nick, host,
             u->handle, par);
      return 1;
    }
    if (!me_op(chan)) {
      dprintf(DP_SERVER, "PART %s\n", chan->name);
      putlog(LOG_CMDS, chan->dname, "(%s!%s) !%s! GO %s", nick, host,
             u->handle, par);
      return 1;
    }
    putlog(LOG_CMDS, chan->dname, _("(%s!%s) !%s! failed GO %s (i'm chop)"),
           nick, host, u->handle, par);
    return 1;
  }
  for (chan = chanset; chan; chan = chan->next) {
    if (ismember(chan, nick)) {
      get_user_flagrec(u, &fr, par);
      if (chan_op(fr) || (glob_op(fr) && !chan_deop(fr))) {
        ok2 = 1;
        if (!me_op(chan)) {
          dprintf(DP_SERVER, "PART %s\n", chan->name);
          ok = 1;
        }
      }
    }
  }
  if (ok) {
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! GO %s", nick, host, u->handle, par);
  } else if (ok2) {
    putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed GO %s (i'm chop)"), nick, host,
           u->handle, par);
  } else {
    putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed GO %s (not op)"), nick, host,
           u->handle, par);
  }
  return 1;
}

static int msg_jump(char *nick, char *host, struct userrec *u, char *par)
{
  char *s;
#ifdef TLS
  char *sport;
#endif
  int port;

  if (match_my_nick(nick))
    return 1;

  if (u_pass_match(u, "-")) {
    putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed JUMP"), nick, host, u->handle);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, _("You don't have "
              "a password set."));
    return 1;
  }
  s = newsplit(&par);           /* Password */
  if (u_pass_match(u, s)) {
    if (par[0]) {
      s = newsplit(&par);
#ifdef TLS
      sport = newsplit(&par);
      if (*sport == '+')
        use_ssl = 1;
      else
        use_ssl = 0;
      port = atoi(sport);
      if (!port) {
        port = default_port;
        use_ssl = 0;
      }
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! JUMP %s %s%d %s", nick, host,
             u->handle, s, use_ssl ? "+" : "", port, par);
#else
      port = atoi(newsplit(&par));
      if (!port)
        port = default_port;
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! JUMP %s %d %s", nick, host,
             u->handle, s, port, par);
#endif
      strcpy(newserver, s);
      newserverport = port;
      strcpy(newserverpass, par);
    } else
      putlog(LOG_CMDS, "*", "(%s!%s) !%s! JUMP", nick, host, u->handle);
    dprintf(-serv, "NOTICE %s :%s\n", nick, _("Jumping servers..."));
    cycle_time = 0;
    nuke_server(_("changing servers"));
  } else
    putlog(LOG_CMDS, "*", _("(%s!%s) !%s! failed JUMP"), nick, host, u->handle);
  return 1;
}

/* MSG COMMANDS
 *
 * Function call should be:
 *    int msg_cmd("handle","nick","user@host","params");
 *
 * The function is responsible for any logging. Return 1 if successful,
 * 0 if not.
 */
static cmd_t C_msg[] = {
  {"addhost", "",    (IntFunc) msg_addhost, NULL},
  {"die",     "n",   (IntFunc) msg_die,     NULL},
  {"go",      "",    (IntFunc) msg_go,      NULL},
  {"hello",   "",    (IntFunc) msg_hello,   NULL},
  {"help",    "",    (IntFunc) msg_help,    NULL},
  {"ident",   "",    (IntFunc) msg_ident,   NULL},
  {"info",    "",    (IntFunc) msg_info,    NULL},
  {"invite",  "o|o", (IntFunc) msg_invite,  NULL},
  {"jump",    "m",   (IntFunc) msg_jump,    NULL},
  {"key",     "o|o", (IntFunc) msg_key,     NULL},
  {"memory",  "m",   (IntFunc) msg_memory,  NULL},
  {"op",      "",    (IntFunc) msg_op,      NULL},
  {"halfop",  "",    (IntFunc) msg_halfop,  NULL},
  {"pass",    "",    (IntFunc) msg_pass,    NULL},
  {"rehash",  "m",   (IntFunc) msg_rehash,  NULL},
  {"reset",   "m",   (IntFunc) msg_reset,   NULL},
  {"save",    "m",   (IntFunc) msg_save,    NULL},
  {"status",  "m|m", (IntFunc) msg_status,  NULL},
  {"voice",   "",    (IntFunc) msg_voice,   NULL},
  {"who",     "",    (IntFunc) msg_who,     NULL},
  {"whois",   "",    (IntFunc) msg_whois,   NULL},
  {NULL,      NULL,  NULL,                   NULL}
};
