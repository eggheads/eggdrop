/*
 * cmds.c -- handles:
 *   commands from a user via dcc
 *   (split in 2, this portion contains no-irc commands)
 *
 * $Id: cmds.c,v 1.129 2011/02/13 14:19:33 simple Exp $
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

#include "main.h"
#include "tandem.h"
#include "modules.h"
#include <ctype.h>

extern struct chanset_t *chanset;
extern struct dcc_t *dcc;
extern struct userrec *userlist;
extern tcl_timer_t *timer, *utimer;
extern int dcc_total, remote_boots, backgrd, make_userfile, do_restart,
           conmask, require_p, must_be_owner, strict_host;
extern unsigned long otraffic_irc, otraffic_irc_today, itraffic_irc,
                     itraffic_irc_today, otraffic_bn, otraffic_bn_today,
                     itraffic_bn, itraffic_bn_today, otraffic_dcc,
                     otraffic_dcc_today, itraffic_dcc, itraffic_dcc_today,
                     otraffic_trans, otraffic_trans_today, itraffic_trans,
                     itraffic_trans_today, otraffic_unknown,
                     otraffic_unknown_today, itraffic_unknown,
                     itraffic_unknown_today;
extern Tcl_Interp *interp;
extern char botnetnick[], origbotname[], ver[], network[], owner[], quit_msg[];
extern time_t now, online_since;
extern module_entry *module_list;

static char *btos(unsigned long);


/* Add hostmask to a bot's record if possible.
 */
static int add_bot_hostmask(int idx, char *nick)
{
  struct chanset_t *chan;

  for (chan = chanset; chan; chan = chan->next)
    if (channel_active(chan)) {
      memberlist *m = ismember(chan, nick);

      if (m) {
        char s[UHOSTLEN];
        struct userrec *u;

        egg_snprintf(s, sizeof s, "%s!%s", m->nick, m->userhost);
        u = get_user_by_host(s);
        if (u) {
          dprintf(idx, "(Can't add hostmask for %s because it matches %s)\n",
                  nick, u->handle);
          return 0;
        }
        if (strchr("~^+=-", m->userhost[0]))
          egg_snprintf(s, sizeof s, "*!%s%s", strict_host ? "?" : "",
                       m->userhost + 1);
        else
          egg_snprintf(s, sizeof s, "*!%s", m->userhost);
        dprintf(idx, "(Added hostmask for %s from %s)\n", nick, chan->dname);
        addhost_by_handle(nick, s);
        return 1;
      }
    }
  return 0;
}

static void tell_who(struct userrec *u, int idx, int chan)
{
  int i, k, ok = 0, atr = u ? u->flags : 0;
  int nicklen;
  char format[81];
  char s[1024]; /* temp fix - 1.4 has a better one */

  if (!chan)
    dprintf(idx, "%s (* = owner, + = master, %% = botmaster, @ = op, "
            "^ = halfop)\n", BOT_PARTYMEMBS);
  else {
    simple_sprintf(s, "assoc %d", chan);
    if ((Tcl_Eval(interp, s) != TCL_OK) || tcl_resultempty())
      dprintf(idx, "%s %s%d: (* = owner, + = master, %% = botmaster, @ = op, "
              "^ = halfop)\n", BOT_PEOPLEONCHAN, (chan < GLOBAL_CHANS) ? "" :
              "*", chan % GLOBAL_CHANS);
    else
      dprintf(idx, "%s '%s' (%s%d): (* = owner, + = master, %% = botmaster, @ = op, "
              "^ = halfop)\n", BOT_PEOPLEONCHAN, tcl_resultstring(),
              (chan < GLOBAL_CHANS) ? "" : "*", chan % GLOBAL_CHANS);
  }

  /* calculate max nicklen */
  nicklen = 0;
  for (i = 0; i < dcc_total; i++) {
    if (strlen(dcc[i].nick) > nicklen)
      nicklen = strlen(dcc[i].nick);
  }
  if (nicklen < 9)
    nicklen = 9;

  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_CHAT)
      if (dcc[i].u.chat->channel == chan) {
        if (atr & USER_OWNER) {
          egg_snprintf(format, sizeof format, "  [%%.2lu]  %%c%%-%us %%s",
                       nicklen);
          sprintf(s, format, dcc[i].sock,
                  (geticon(i) == '-' ? ' ' : geticon(i)), dcc[i].nick,
                  dcc[i].host);
        } else {
          egg_snprintf(format, sizeof format, "  %%c%%-%us %%s", nicklen);
          sprintf(s, format,
                  (geticon(i) == '-' ? ' ' : geticon(i)),
                  dcc[i].nick, dcc[i].host);
        }
        if (atr & USER_MASTER) {
          if (dcc[i].u.chat->con_flags)
            sprintf(&s[strlen(s)], " (con:%s)",
                    masktype(dcc[i].u.chat->con_flags));
        }
        if (now - dcc[i].timeval > 300) {
          unsigned long days, hrs, mins;

          days = (now - dcc[i].timeval) / 86400;
          hrs = ((now - dcc[i].timeval) - (days * 86400)) / 3600;
          mins = ((now - dcc[i].timeval) - (hrs * 3600)) / 60;
          if (days > 0)
            sprintf(&s[strlen(s)], " (idle %lud%luh)", days, hrs);
          else if (hrs > 0)
            sprintf(&s[strlen(s)], " (idle %luh%lum)", hrs, mins);
          else
            sprintf(&s[strlen(s)], " (idle %lum)", mins);
        }
        dprintf(idx, "%s\n", s);
        if (dcc[i].u.chat->away != NULL)
          dprintf(idx, "      AWAY: %s\n", dcc[i].u.chat->away);
      }
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_BOT) {
      if (!ok) {
        ok = 1;
        dprintf(idx, "Bots connected:\n");
      }
      egg_strftime(s, 14, "%d %b %H:%M", localtime(&dcc[i].timeval));
      if (atr & USER_OWNER) {
        egg_snprintf(format, sizeof format,
                     "  [%%.2lu]  %%s%%c%%-%us (%%s) %%s\n", nicklen);
        dprintf(idx, format, dcc[i].sock,
                dcc[i].status & STAT_CALLED ? "<-" : "->",
                dcc[i].status & STAT_SHARE ? '+' : ' ', dcc[i].nick, s,
                dcc[i].u.bot->version);
      } else {
        egg_snprintf(format, sizeof format, "  %%s%%c%%-%us (%%s) %%s\n",
                     nicklen);
        dprintf(idx, format, dcc[i].status & STAT_CALLED ? "<-" : "->",
                dcc[i].status & STAT_SHARE ? '+' : ' ', dcc[i].nick, s,
                dcc[i].u.bot->version);
      }
    }
  ok = 0;
  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type == &DCC_CHAT) && (dcc[i].u.chat->channel != chan)) {
      if (!ok) {
        ok = 1;
        dprintf(idx, "Other people on the bot:\n");
      }
      if (atr & USER_OWNER) {
        egg_snprintf(format, sizeof format, "  [%%.2lu]  %%c%%-%us ", nicklen);
        sprintf(s, format, dcc[i].sock,
                (geticon(i) == '-' ? ' ' : geticon(i)), dcc[i].nick);
      } else {
        egg_snprintf(format, sizeof format, "  %%c%%-%us ", nicklen);
        sprintf(s, format, (geticon(i) == '-' ? ' ' : geticon(i)), dcc[i].nick);
      }
      if (atr & USER_MASTER) {
        if (dcc[i].u.chat->channel < 0)
          strcat(s, "(-OFF-) ");
        else if (!dcc[i].u.chat->channel)
          strcat(s, "(party) ");
        else
          sprintf(&s[strlen(s)], "(%5d) ", dcc[i].u.chat->channel);
      }
      strcat(s, dcc[i].host);
      if (atr & USER_MASTER) {
        if (dcc[i].u.chat->con_flags)
          sprintf(&s[strlen(s)], " (con:%s)",
                  masktype(dcc[i].u.chat->con_flags));
      }
      if (now - dcc[i].timeval > 300) {
        k = (now - dcc[i].timeval) / 60;
        if (k < 60)
          sprintf(&s[strlen(s)], " (idle %dm)", k);
        else
          sprintf(&s[strlen(s)], " (idle %dh%dm)", k / 60, k % 60);
      }
      dprintf(idx, "%s\n", s);
      if (dcc[i].u.chat->away != NULL)
        dprintf(idx, "      AWAY: %s\n", dcc[i].u.chat->away);
    }
    if ((atr & USER_MASTER) && (dcc[i].type->flags & DCT_SHOWWHO) &&
        (dcc[i].type != &DCC_CHAT)) {
      if (!ok) {
        ok = 1;
        dprintf(idx, "Other people on the bot:\n");
      }
      if (atr & USER_OWNER) {
        egg_snprintf(format, sizeof format, "  [%%.2lu]  %%c%%-%us (files) %%s",
                     nicklen);
        sprintf(s, format,
                dcc[i].sock, dcc[i].status & STAT_CHAT ? '+' : ' ',
                dcc[i].nick, dcc[i].host);
      } else {
        egg_snprintf(format, sizeof format, "  %%c%%-%us (files) %%s", nicklen);
        sprintf(s, format,
                dcc[i].status & STAT_CHAT ? '+' : ' ',
                dcc[i].nick, dcc[i].host);
      }
      dprintf(idx, "%s\n", s);
    }
  }
}

static void cmd_botinfo(struct userrec *u, int idx, char *par)
{
  char s[512], s2[32];
  struct chanset_t *chan;
  time_t now2;
  int hr, min;

  now2 = now - online_since;
  s2[0] = 0;
  if (now2 > 86400) {
    int days = now2 / 86400;

    /* Days */
    sprintf(s2, "%d day", days);
    if (days >= 2)
      strcat(s2, "s");
    strcat(s2, ", ");
    now2 -= days * 86400;
  }
  hr = (time_t) ((int) now2 / 3600);
  now2 -= (hr * 3600);
  min = (time_t) ((int) now2 / 60);
  sprintf(&s2[strlen(s2)], "%02d:%02d", (int) hr, (int) min);
  putlog(LOG_CMDS, "*", "#%s# botinfo", dcc[idx].nick);
  simple_sprintf(s, "%d:%s@%s", dcc[idx].sock, dcc[idx].nick, botnetnick);
  botnet_send_infoq(-1, s);
  s[0] = 0;
  if (module_find("server", 0, 0)) {
    for (chan = chanset; chan; chan = chan->next) {
      if (!channel_secret(chan)) {
        if ((strlen(s) + strlen(chan->dname) + strlen(network)
             + strlen(botnetnick) + strlen(ver) + 1) >= 490) {
          strcat(s, "++  ");
          break;                /* yeesh! */
        }
        strcat(s, chan->dname);
        strcat(s, ", ");
      }
    }

    if (s[0]) {
      s[strlen(s) - 2] = 0;
      dprintf(idx, "*** [%s] %s <%s> (%s) [UP %s]\n", botnetnick,
              ver, network, s, s2);
    } else
      dprintf(idx, "*** [%s] %s <%s> (%s) [UP %s]\n", botnetnick,
              ver, network, BOT_NOCHANNELS, s2);
  } else
    dprintf(idx, "*** [%s] %s <NO_IRC> [UP %s]\n", botnetnick, ver, s2);
}

static void cmd_whom(struct userrec *u, int idx, char *par)
{
  if (par[0] == '*') {
    putlog(LOG_CMDS, "*", "#%s# whom %s", dcc[idx].nick, par);
    answer_local_whom(idx, -1);
    return;
  } else if (dcc[idx].u.chat->channel < 0) {
    dprintf(idx, "You have chat turned off.\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# whom %s", dcc[idx].nick, par);
  if (!par[0]) {
    answer_local_whom(idx, dcc[idx].u.chat->channel);
  } else {
    int chan = -1;

    if ((par[0] < '0') || (par[0] > '9')) {
      Tcl_SetVar(interp, "_chan", par, 0);
      if ((Tcl_VarEval(interp, "assoc ", "$_chan", NULL) == TCL_OK) &&
          !tcl_resultempty()) {
        chan = tcl_resultint();
      }
      if (chan <= 0) {
        dprintf(idx, "No such channel exists.\n");
        return;
      }
    } else
      chan = atoi(par);
    if ((chan < 0) || (chan >= GLOBAL_CHANS)) {
      dprintf(idx, "Channel number out of range: must be between 0 and %d."
              "\n", GLOBAL_CHANS);
      return;
    }
    answer_local_whom(idx, chan);
  }
}

static void cmd_me(struct userrec *u, int idx, char *par)
{
  int i;

  if (dcc[idx].u.chat->channel < 0) {
    dprintf(idx, "You have chat turned off.\n");
    return;
  }
  if (!par[0]) {
    dprintf(idx, "Usage: me <action>\n");
    return;
  }
  if (dcc[idx].u.chat->away != NULL)
    not_away(idx);
  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type->flags & DCT_CHAT) &&
        (dcc[i].u.chat->channel == dcc[idx].u.chat->channel) &&
        ((i != idx) || (dcc[i].status & STAT_ECHO)))
      dprintf(i, "* %s %s\n", dcc[idx].nick, par);
  botnet_send_act(idx, botnetnick, dcc[idx].nick,
                  dcc[idx].u.chat->channel, par);
  check_tcl_act(dcc[idx].nick, dcc[idx].u.chat->channel, par);
}

static void cmd_motd(struct userrec *u, int idx, char *par)
{
  int i;

  if (par[0]) {
    putlog(LOG_CMDS, "*", "#%s# motd %s", dcc[idx].nick, par);
    if (!egg_strcasecmp(par, botnetnick))
      show_motd(idx);
    else {
      i = nextbot(par);
      if (i < 0)
        dprintf(idx, "That bot isn't connected.\n");
      else {
        char x[40];

        simple_sprintf(x, "%s%d:%s@%s",
                       (u->flags & USER_HIGHLITE) ?
                       ((dcc[idx].status & STAT_TELNET) ? "#" : "!") : "",
                       dcc[idx].sock, dcc[idx].nick, botnetnick);
        botnet_send_motd(i, x, par);
      }
    }
  } else {
    putlog(LOG_CMDS, "*", "#%s# motd", dcc[idx].nick);
    show_motd(idx);
  }
}

static void cmd_away(struct userrec *u, int idx, char *par)
{
  if (strlen(par) > 60)
    par[60] = 0;
  set_away(idx, par);
}

static void cmd_back(struct userrec *u, int idx, char *par)
{
  not_away(idx);
}

static void cmd_newpass(struct userrec *u, int idx, char *par)
{
  char *new;

  if (!par[0]) {
    dprintf(idx, "Usage: newpass <newpassword>\n");
    return;
  }
  new = newsplit(&par);
  if (strlen(new) > 16)
    new[16] = 0;
  if (strlen(new) < 6) {
    dprintf(idx, "Please use at least 6 characters.\n");
    return;
  }
  set_user(&USERENTRY_PASS, u, new);
  putlog(LOG_CMDS, "*", "#%s# newpass...", dcc[idx].nick);
  dprintf(idx, "Changed password to '%s'.\n", new);
}

static void cmd_bots(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# bots", dcc[idx].nick);
  tell_bots(idx);
}

static void cmd_bottree(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# bottree", dcc[idx].nick);
  tell_bottree(idx, 0);
}

static void cmd_vbottree(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# vbottree", dcc[idx].nick);
  tell_bottree(idx, 1);
}

static void cmd_rehelp(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# rehelp", dcc[idx].nick);
  dprintf(idx, "Reload help cache...\n");
  reload_help_data();
}

static void cmd_help(struct userrec *u, int idx, char *par)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  get_user_flagrec(u, &fr, dcc[idx].u.chat->con_chan);
  if (par[0]) {
    putlog(LOG_CMDS, "*", "#%s# help %s", dcc[idx].nick, par);
    if (!strcmp(par, "all"))
      tellallhelp(idx, "all", &fr);
    else if (strchr(par, '*') || strchr(par, '?')) {
      char *p = par;

      /* Check if the search pattern only consists of '*' and/or '?'
       * If it does, show help for "all" instead of listing all help
       * entries.
       */
      for (p = par; *p && ((*p == '*') || (*p == '?')); p++);
      if (*p)
        tellwildhelp(idx, par, &fr);
      else
        tellallhelp(idx, "all", &fr);
    } else
      tellhelp(idx, par, &fr, 0);
  } else {
    putlog(LOG_CMDS, "*", "#%s# help", dcc[idx].nick);
    if (glob_op(fr) || glob_botmast(fr) || chan_op(fr))
      tellhelp(idx, "help", &fr, 0);
    else
      tellhelp(idx, "partyline", &fr, 0);
  }
}

static void cmd_addlog(struct userrec *u, int idx, char *par)
{
  if (!par[0]) {
    dprintf(idx, "Usage: addlog <message>\n");
    return;
  }
  dprintf(idx, "Placed entry in the log file.\n");
  putlog(LOG_MISC, "*", "%s: %s", dcc[idx].nick, par);
}

static void cmd_who(struct userrec *u, int idx, char *par)
{
  int i;

  if (par[0]) {
    if (dcc[idx].u.chat->channel < 0) {
      dprintf(idx, "You have chat turned off.\n");
      return;
    }
    putlog(LOG_CMDS, "*", "#%s# who %s", dcc[idx].nick, par);
    if (!egg_strcasecmp(par, botnetnick))
      tell_who(u, idx, dcc[idx].u.chat->channel);
    else {
      i = nextbot(par);
      if (i < 0) {
        dprintf(idx, "That bot isn't connected.\n");
      } else if (dcc[idx].u.chat->channel >= GLOBAL_CHANS)
        dprintf(idx, "You are on a local channel.\n");
      else {
        char s[40];

        simple_sprintf(s, "%d:%s@%s", dcc[idx].sock, dcc[idx].nick, botnetnick);
        botnet_send_who(i, s, par, dcc[idx].u.chat->channel);
      }
    }
  } else {
    putlog(LOG_CMDS, "*", "#%s# who", dcc[idx].nick);
    if (dcc[idx].u.chat->channel < 0)
      tell_who(u, idx, 0);
    else
      tell_who(u, idx, dcc[idx].u.chat->channel);
  }
}

static void cmd_whois(struct userrec *u, int idx, char *par)
{
  if (!par[0]) {
    dprintf(idx, "Usage: whois <handle>\n");
    return;
  }

  putlog(LOG_CMDS, "*", "#%s# whois %s", dcc[idx].nick, par);
  tell_user_ident(idx, par, u ? (u->flags & USER_MASTER) : 0);
}

static void cmd_match(struct userrec *u, int idx, char *par)
{
  int start = 1, limit = 20;
  char *s, *s1, *chname;

  if (!par[0]) {
    dprintf(idx, "Usage: match <nick/host> [[skip] count]\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# match %s", dcc[idx].nick, par);
  s = newsplit(&par);
  if (strchr(CHANMETA, par[0]) != NULL)
    chname = newsplit(&par);
  else
    chname = "";
  if (atoi(par) > 0) {
    s1 = newsplit(&par);
    if (atoi(par) > 0) {
      start = atoi(s1);
      limit = atoi(par);
    } else
      limit = atoi(s1);
  }
  tell_users_match(idx, s, start, limit, u ? (u->flags & USER_MASTER) : 0,
                   chname);
}

static void cmd_uptime(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# uptime", dcc[idx].nick);
  tell_verbose_uptime(idx);
}

static void cmd_status(struct userrec *u, int idx, char *par)
{
  int atr = u ? u->flags : 0;

  if (!egg_strcasecmp(par, "all")) {
    if (!(atr & USER_MASTER)) {
      dprintf(idx, "You do not have Bot Master privileges.\n");
      return;
    }
    putlog(LOG_CMDS, "*", "#%s# status all", dcc[idx].nick);
    tell_verbose_status(idx);
    tell_mem_status_dcc(idx);
    dprintf(idx, "\n");
    tell_settings(idx);
    do_module_report(idx, 1, NULL);
  } else {
    putlog(LOG_CMDS, "*", "#%s# status", dcc[idx].nick);
    tell_verbose_status(idx);
    tell_mem_status_dcc(idx);
    do_module_report(idx, 0, NULL);
  }
}

static void cmd_dccstat(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# dccstat", dcc[idx].nick);
  tell_dcc(idx);
}

static void cmd_boot(struct userrec *u, int idx, char *par)
{
  int i, files = 0, ok = 0;
  char *who;
  struct userrec *u2;

  if (!par[0]) {
    dprintf(idx, "Usage: boot nick[@bot]\n");
    return;
  }
  who = newsplit(&par);
  if (strchr(who, '@') != NULL) {
    char whonick[HANDLEN + 1];

    splitcn(whonick, who, '@', HANDLEN + 1);
    if (!egg_strcasecmp(who, botnetnick)) {
      cmd_boot(u, idx, whonick);
      return;
    }
    if (remote_boots > 0) {
      i = nextbot(who);
      if (i < 0) {
        dprintf(idx, "No such bot connected.\n");
        return;
      }
      botnet_send_reject(i, dcc[idx].nick, botnetnick, whonick,
                         who, par[0] ? par : dcc[idx].nick);
      putlog(LOG_BOTS, "*", "#%s# boot %s@%s (%s)", dcc[idx].nick, whonick,
             who, par[0] ? par : dcc[idx].nick);
    } else
      dprintf(idx, "Remote boots are disabled here.\n");
    return;
  }
  for (i = 0; i < dcc_total; i++)
    if (!egg_strcasecmp(dcc[i].nick, who) && !ok &&
        (dcc[i].type->flags & DCT_CANBOOT)) {
      u2 = get_user_by_handle(userlist, dcc[i].nick);
      if (u2 && (u2->flags & USER_OWNER) &&
          egg_strcasecmp(dcc[idx].nick, who)) {
        dprintf(idx, "You can't boot a bot owner.\n");
        return;
      }
      if (u2 && (u2->flags & USER_MASTER) && !(u && (u->flags & USER_MASTER))) {
        dprintf(idx, "You can't boot a bot master.\n");
        return;
      }
      files = (dcc[i].type->flags & DCT_FILES);
      if (files)
        dprintf(idx, "Booted %s from the file area.\n", dcc[i].nick);
      else
        dprintf(idx, "Booted %s from the party line.\n", dcc[i].nick);
      putlog(LOG_CMDS, "*", "#%s# boot %s %s", dcc[idx].nick, who, par);
      do_boot(i, dcc[idx].nick, par);
      ok = 1;
    }
  if (!ok)
    dprintf(idx, "Who?  No such person on the party line.\n");
}

static void cmd_console(struct userrec *u, int idx, char *par)
{
  char *nick, s[2], s1[512];
  int dest = 0, i, ok = 0, pls;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };
  module_entry *me;

  if (!par[0]) {
    dprintf(idx, "Your console is %s: %s (%s).\n",
            dcc[idx].u.chat->con_chan,
            masktype(dcc[idx].u.chat->con_flags),
            maskname(dcc[idx].u.chat->con_flags));
    return;
  }
  get_user_flagrec(u, &fr, dcc[idx].u.chat->con_chan);
  strcpy(s1, par);
  nick = newsplit(&par);
  /* Don't remove '+' as someone couldn't have '+' in CHANMETA cause
   * he doesn't use IRCnet ++rtc.
   */
  if (nick[0] && !strchr(CHANMETA "+-*", nick[0]) && glob_master(fr)) {
    for (i = 0; i < dcc_total; i++)
      if (!egg_strcasecmp(nick, dcc[i].nick) &&
          (dcc[i].type == &DCC_CHAT) && (!ok)) {
        ok = 1;
        dest = i;
      }
    if (!ok) {
      dprintf(idx, "No such user on the party line!\n");
      return;
    }
    nick[0] = 0;
  } else
    dest = idx;
  if (!nick[0])
    nick = newsplit(&par);
  /* Consider modeless channels, starting with '+' */
  if ((nick[0] == '+' && findchan_by_dname(nick)) ||
      (nick[0] != '+' && strchr(CHANMETA "*", nick[0]))) {
    if (strcmp(nick, "*") && !findchan_by_dname(nick)) {
      dprintf(idx, "Invalid console channel: %s.\n", nick);
      return;
    }
    get_user_flagrec(u, &fr, nick);
    if (!chan_op(fr) && !(glob_op(fr) && !chan_deop(fr))) {
      dprintf(idx, "You don't have op or master access to channel %s.\n",
              nick);
      return;
    }
    strncpyz(dcc[dest].u.chat->con_chan, nick, 81);
    nick[0] = 0;
    if (dest != idx)
      get_user_flagrec(dcc[dest].user, &fr, dcc[dest].u.chat->con_chan);
  }
  if (!nick[0])
    nick = newsplit(&par);
  pls = 1;
  if (nick[0]) {
    if ((nick[0] != '+') && (nick[0] != '-'))
      dcc[dest].u.chat->con_flags = 0;
    for (; *nick; nick++) {
      if (*nick == '+')
        pls = 1;
      else if (*nick == '-')
        pls = 0;
      else {
        s[0] = *nick;
        s[1] = 0;
        if (pls)
          dcc[dest].u.chat->con_flags |= logmodes(s);
        else
          dcc[dest].u.chat->con_flags &= ~logmodes(s);
      }
    }
  }
  dcc[dest].u.chat->con_flags = check_conflags(&fr,
                                               dcc[dest].u.chat->con_flags);
  putlog(LOG_CMDS, "*", "#%s# console %s", dcc[idx].nick, s1);
  if (dest == idx) {
    dprintf(idx, "Set your console to %s: %s (%s).\n",
            dcc[idx].u.chat->con_chan,
            masktype(dcc[idx].u.chat->con_flags),
            maskname(dcc[idx].u.chat->con_flags));
  } else {
    dprintf(idx, "Set console of %s to %s: %s (%s).\n", dcc[dest].nick,
            dcc[dest].u.chat->con_chan,
            masktype(dcc[dest].u.chat->con_flags),
            maskname(dcc[dest].u.chat->con_flags));
    dprintf(dest, "%s set your console to %s: %s (%s).\n", dcc[idx].nick,
            dcc[dest].u.chat->con_chan,
            masktype(dcc[dest].u.chat->con_flags),
            maskname(dcc[dest].u.chat->con_flags));
  }
  /* New style autosave -- drummer,07/25/1999 */
  if ((me = module_find("console", 1, 1))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (dest);
  }
}

static void cmd_pls_bot(struct userrec *u, int idx, char *par)
{
  char *handle, *addr, *p, *q, *host;
  struct userrec *u1;
  struct bot_addr *bi;

  if (!par[0]) {
    dprintf(idx, "Usage: +bot <handle> [address[:telnet-port[/relay-port]]] "
            "[host]\n");
    return;
  }

  handle = newsplit(&par);
  addr = newsplit(&par);
  host = newsplit(&par);

  if (strlen(handle) > HANDLEN)
    handle[HANDLEN] = 0;

  if (get_user_by_handle(userlist, handle)) {
    dprintf(idx, "Someone already exists by that name.\n");
    return;
  }

  if (strchr(BADHANDCHARS, handle[0]) != NULL) {
    dprintf(idx, "You can't start a botnick with '%c'.\n", handle[0]);
    return;
  }

  if (strlen(addr) > 60)
    addr[60] = 0;

  putlog(LOG_CMDS, "*", "#%s# +bot %s%s%s%s%s", dcc[idx].nick, handle,
         addr[0] ? " " : "", addr, host[0] ? " " : "", host);
  userlist = adduser(userlist, handle, "none", "-", USER_BOT);
  u1 = get_user_by_handle(userlist, handle);
  bi = user_malloc(sizeof(struct bot_addr));

  q = strchr(addr, ':');
  if (!q) {
    bi->address = user_malloc(strlen(addr) + 1);
    strcpy(bi->address, addr);
    bi->telnet_port = 3333;
    bi->relay_port = 3333;
  } else {
    bi->address = user_malloc(q - addr + 1);
    strncpy(bi->address, addr, q - addr);
    bi->address[q - addr] = 0;
    p = q + 1;
    bi->telnet_port = atoi(p);
    q = strchr(p, '/');
    if (!q) {
      bi->relay_port = bi->telnet_port;
    } else  {
      bi->relay_port = atoi(q + 1);
    }
  }

  set_user(&USERENTRY_BOTADDR, u1, bi);
  if (addr[0]) {
    dprintf(idx, "Added bot '%s' with address '%s' and %s%s%s.\n", handle,
            addr, host[0] ? "hostmask '" : "no hostmask", host[0] ? host : "",
            host[0] ? "'" : "");
  } else{
    dprintf(idx, "Added bot '%s' with no address and %s%s%s.\n", handle,
            host[0] ? "hostmask '" : "no hostmask", host[0] ? host : "",
            host[0] ? "'" : "");
  }
  if (host[0]) {
    addhost_by_handle(handle, host);
  } else if (!add_bot_hostmask(idx, handle)) {
    dprintf(idx, "You'll want to add a hostmask if this bot will ever be on "
            "any channels that I'm on.\n");
  }
}

static void cmd_chhandle(struct userrec *u, int idx, char *par)
{
  char hand[HANDLEN + 1], newhand[HANDLEN + 1];
  int i, atr = u ? u->flags : 0, atr2;
  struct userrec *u2;

  strncpyz(hand, newsplit(&par), sizeof hand);
  strncpyz(newhand, newsplit(&par), sizeof newhand);

  if (!hand[0] || !newhand[0]) {
    dprintf(idx, "Usage: chhandle <oldhandle> <newhandle>\n");
    return;
  }
  for (i = 0; i < strlen(newhand); i++)
    if (((unsigned char) newhand[i] <= 32) || (newhand[i] == '@'))
      newhand[i] = '?';
  if (strchr(BADHANDCHARS, newhand[0]) != NULL)
    dprintf(idx, "Bizarre quantum forces prevent nicknames from starting with "
            "'%c'.\n", newhand[0]);
  else if (get_user_by_handle(userlist, newhand) &&
           egg_strcasecmp(hand, newhand))
    dprintf(idx, "Somebody is already using %s.\n", newhand);
  else {
    u2 = get_user_by_handle(userlist, hand);
    atr2 = u2 ? u2->flags : 0;
    if ((atr & USER_BOTMAST) && !(atr & USER_MASTER) && !(atr2 & USER_BOT))
      dprintf(idx, "You can't change handles for non-bots.\n");
    else if ((bot_flags(u2) & BOT_SHARE) && !(atr & USER_OWNER))
      dprintf(idx, "You can't change share bot's nick.\n");
    else if ((atr2 & USER_OWNER) && !(atr & USER_OWNER) &&
             egg_strcasecmp(dcc[idx].nick, hand))
      dprintf(idx, "You can't change a bot owner's handle.\n");
    else if (isowner(hand) && egg_strcasecmp(dcc[idx].nick, hand))
      dprintf(idx, "You can't change a permanent bot owner's handle.\n");
    else if (!egg_strcasecmp(newhand, botnetnick) && (!(atr2 & USER_BOT) ||
             nextbot(hand) != -1))
      dprintf(idx, "Hey! That's MY name!\n");
    else if (change_handle(u2, newhand)) {
      putlog(LOG_CMDS, "*", "#%s# chhandle %s %s", dcc[idx].nick,
             hand, newhand);
      dprintf(idx, "Changed.\n");
    } else
      dprintf(idx, "Failed.\n");
  }
}

static void cmd_handle(struct userrec *u, int idx, char *par)
{
  char oldhandle[HANDLEN + 1], newhandle[HANDLEN + 1];
  int i;

  strncpyz(newhandle, newsplit(&par), sizeof newhandle);

  if (!newhandle[0]) {
    dprintf(idx, "Usage: handle <new-handle>\n");
    return;
  }
  for (i = 0; i < strlen(newhandle); i++)
    if (((unsigned char) newhandle[i] <= 32) || (newhandle[i] == '@'))
      newhandle[i] = '?';
  if (strchr(BADHANDCHARS, newhandle[0]) != NULL)
    dprintf(idx,
            "Bizarre quantum forces prevent handle from starting with '%c'.\n",
            newhandle[0]);
  else if (get_user_by_handle(userlist, newhandle) &&
           egg_strcasecmp(dcc[idx].nick, newhandle))
    dprintf(idx, "Somebody is already using %s.\n", newhandle);
  else if (!egg_strcasecmp(newhandle, botnetnick))
    dprintf(idx, "Hey!  That's MY name!\n");
  else {
    strncpyz(oldhandle, dcc[idx].nick, sizeof oldhandle);
    if (change_handle(u, newhandle)) {
      putlog(LOG_CMDS, "*", "#%s# handle %s", oldhandle, newhandle);
      dprintf(idx, "Okay, changed.\n");
    } else
      dprintf(idx, "Failed.\n");
  }
}

static void cmd_chpass(struct userrec *u, int idx, char *par)
{
  char *handle, *new;
  int atr = u ? u->flags : 0, l;

  if (!par[0])
    dprintf(idx, "Usage: chpass <handle> [password]\n");
  else {
    handle = newsplit(&par);
    u = get_user_by_handle(userlist, handle);
    if (!u)
      dprintf(idx, "No such user.\n");
    else if ((atr & USER_BOTMAST) && !(atr & USER_MASTER) &&
             !(u->flags & USER_BOT))
      dprintf(idx, "You can't change passwords for non-bots.\n");
    else if ((bot_flags(u) & BOT_SHARE) && !(atr & USER_OWNER))
      dprintf(idx, "You can't change a share bot's password.\n");
    else if ((u->flags & USER_OWNER) && !(atr & USER_OWNER) &&
             egg_strcasecmp(handle, dcc[idx].nick))
      dprintf(idx, "You can't change a bot owner's password.\n");
    else if (isowner(handle) && egg_strcasecmp(dcc[idx].nick, handle))
      dprintf(idx, "You can't change a permanent bot owner's password.\n");
    else if (!par[0]) {
      putlog(LOG_CMDS, "*", "#%s# chpass %s [nothing]", dcc[idx].nick, handle);
      set_user(&USERENTRY_PASS, u, NULL);
      dprintf(idx, "Removed password.\n");
    } else {
      l = strlen(new = newsplit(&par));
      if (l > 16)
        new[16] = 0;
      if (l < 6)
        dprintf(idx, "Please use at least 6 characters.\n");
      else {
        set_user(&USERENTRY_PASS, u, new);
        putlog(LOG_CMDS, "*", "#%s# chpass %s [something]", dcc[idx].nick,
               handle);
        dprintf(idx, "Changed password.\n");
      }
    }
  }
}

static void cmd_chaddr(struct userrec *u, int idx, char *par)
{
  int telnet_port = 3333, relay_port = 3333;
  char *handle, *addr, *p, *q;
  struct bot_addr *bi;
  struct userrec *u1;

  handle = newsplit(&par);
  if (!par[0]) {
    dprintf(idx, "Usage: chaddr <botname> "
            "<address[:telnet-port[/relay-port]]>\n");
    return;
  }
  addr = newsplit(&par);
  if (strlen(addr) > UHOSTMAX)
    addr[UHOSTMAX] = 0;
  u1 = get_user_by_handle(userlist, handle);
  if (!u1 || !(u1->flags & USER_BOT)) {
    dprintf(idx, "This command is only useful for tandem bots.\n");
    return;
  }
  if ((bot_flags(u1) & BOT_SHARE) && (!u || !(u->flags & USER_OWNER))) {
    dprintf(idx, "You can't change a share bot's address.\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# chaddr %s %s", dcc[idx].nick, handle, addr);
  dprintf(idx, "Changed bot's address.\n");

  bi = (struct bot_addr *) get_user(&USERENTRY_BOTADDR, u1);
  if (bi) {
    telnet_port = bi->telnet_port;
    relay_port = bi->relay_port;
  }

  bi = user_malloc(sizeof(struct bot_addr));

  q = strchr(addr, ':');
  if (!q) {
    bi->address = user_malloc(strlen(addr) + 1);
    strcpy(bi->address, addr);
    bi->telnet_port = telnet_port;
    bi->relay_port = relay_port;
  } else {
    bi->address = user_malloc(q - addr + 1);
    strncpyz(bi->address, addr, q - addr + 1);
    p = q + 1;
    bi->telnet_port = atoi(p);
    q = strchr(p, '/');
    if (!q) {
      bi->relay_port = bi->telnet_port;
    } else {
      bi->relay_port = atoi(q + 1);
    }
  }
  set_user(&USERENTRY_BOTADDR, u1, bi);
}

static void cmd_comment(struct userrec *u, int idx, char *par)
{
  char *handle;
  struct userrec *u1;

  handle = newsplit(&par);
  if (!par[0]) {
    dprintf(idx, "Usage: comment <handle> <newcomment>\n");
    return;
  }
  u1 = get_user_by_handle(userlist, handle);
  if (!u1) {
    dprintf(idx, "No such user!\n");
    return;
  }
  if ((u1->flags & USER_OWNER) && !(u && (u->flags & USER_OWNER)) &&
      egg_strcasecmp(handle, dcc[idx].nick)) {
    dprintf(idx, "You can't change comment on a bot owner.\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# comment %s %s", dcc[idx].nick, handle, par);
  if (!egg_strcasecmp(par, "none")) {
    dprintf(idx, "Okay, comment blanked.\n");
    set_user(&USERENTRY_COMMENT, u1, NULL);
    return;
  }
  dprintf(idx, "Changed comment.\n");
  set_user(&USERENTRY_COMMENT, u1, par);
}

static void cmd_restart(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# restart", dcc[idx].nick);
  if (!backgrd) {
    dprintf(idx, "You cannot .restart a bot when running -n (due to Tcl).\n");
    return;
  }
  dprintf(idx, "Restarting.\n");
  if (make_userfile) {
    putlog(LOG_MISC, "*", "Uh, guess you don't need to create a new userfile.");
    make_userfile = 0;
  }
  write_userfile(-1);
  putlog(LOG_MISC, "*", "Restarting ...");
  wipe_timers(interp, &utimer);
  wipe_timers(interp, &timer);
  do_restart = idx;
}

static void cmd_rehash(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# rehash", dcc[idx].nick);
  dprintf(idx, "Rehashing.\n");
  if (make_userfile) {
    putlog(LOG_MISC, "*", "Uh, guess you don't need to create a new userfile.");
    make_userfile = 0;
  }
  write_userfile(-1);
  putlog(LOG_MISC, "*", "Rehashing ...");
  do_restart = -2;
}

static void cmd_reload(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# reload", dcc[idx].nick);
  dprintf(idx, "Reloading user file...\n");
  reload();
}

void cmd_die(struct userrec *u, int idx, char *par)
{
  char s1[1024], s2[1024];

  putlog(LOG_CMDS, "*", "#%s# die %s", dcc[idx].nick, par);
  if (par[0]) {
    egg_snprintf(s1, sizeof s1, "BOT SHUTDOWN (%s: %s)", dcc[idx].nick, par);
    egg_snprintf(s2, sizeof s2, "DIE BY %s!%s (%s)", dcc[idx].nick,
                 dcc[idx].host, par);
    strncpyz(quit_msg, par, 1024);
  } else {
    egg_snprintf(s1, sizeof s1, "BOT SHUTDOWN (Authorized by %s)",
                 dcc[idx].nick);
    egg_snprintf(s2, sizeof s2, "DIE BY %s!%s (request)", dcc[idx].nick,
                 dcc[idx].host);
    strncpyz(quit_msg, dcc[idx].nick, 1024);
  }
  kill_bot(s1, s2);
}

static void cmd_debug(struct userrec *u, int idx, char *par)
{
  if (!egg_strcasecmp(par, "help")) {
    putlog(LOG_CMDS, "*", "#%s# debug help", dcc[idx].nick);
    debug_help(idx);
  } else {
    putlog(LOG_CMDS, "*", "#%s# debug", dcc[idx].nick);
    debug_mem_to_dcc(idx);
  }
}

static void cmd_simul(struct userrec *u, int idx, char *par)
{
  char *nick;
  int i, ok = 0;

  nick = newsplit(&par);
  if (!par[0]) {
    dprintf(idx, "Usage: simul <hand> <text>\n");
    return;
  }
  if (isowner(nick)) {
    dprintf(idx, "Unable to '.simul' permanent owners.\n");
    return;
  }
  for (i = 0; i < dcc_total; i++)
    if (!egg_strcasecmp(nick, dcc[i].nick) && !ok &&
        (dcc[i].type->flags & DCT_SIMUL)) {
      putlog(LOG_CMDS, "*", "#%s# simul %s %s", dcc[idx].nick, nick, par);
      if (dcc[i].type && dcc[i].type->activity) {
        dcc[i].type->activity(i, par, strlen(par));
        ok = 1;
      }
    }
  if (!ok)
    dprintf(idx, "No such user on the party line.\n");
}

static void cmd_link(struct userrec *u, int idx, char *par)
{
  char *s;
  int i;

  if (!par[0]) {
    dprintf(idx, "Usage: link [some-bot] <new-bot>\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# link %s", dcc[idx].nick, par);
  s = newsplit(&par);
  if (!par[0] || !egg_strcasecmp(par, botnetnick))
    botlink(dcc[idx].nick, idx, s);
  else {
    char x[40];

    i = nextbot(s);
    if (i < 0) {
      dprintf(idx, "No such bot online.\n");
      return;
    }
    simple_sprintf(x, "%d:%s@%s", dcc[idx].sock, dcc[idx].nick, botnetnick);
    botnet_send_link(i, x, s, par);
  }
}

static void cmd_unlink(struct userrec *u, int idx, char *par)
{
  int i;
  char *bot;

  if (!par[0]) {
    dprintf(idx, "Usage: unlink <bot> [reason]\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# unlink %s", dcc[idx].nick, par);
  bot = newsplit(&par);
  i = nextbot(bot);
  if (i < 0) {
    botunlink(idx, bot, par, dcc[idx].nick);
    return;
  }
  /* If we're directly connected to that bot, just do it
   * (is nike gunna sue?)
   */
  if (!egg_strcasecmp(dcc[i].nick, bot))
    botunlink(idx, bot, par, dcc[i].nick);
  else {
    char x[40];

    simple_sprintf(x, "%d:%s@%s", dcc[idx].sock, dcc[idx].nick, botnetnick);
    botnet_send_unlink(i, x, lastbot(bot), bot, par);
  }
}

static void cmd_relay(struct userrec *u, int idx, char *par)
{
  if (!par[0]) {
    dprintf(idx, "Usage: relay <bot>\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# relay %s", dcc[idx].nick, par);
  tandem_relay(idx, par, 0);
}

static void cmd_save(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# save", dcc[idx].nick);
  dprintf(idx, "Saving user file...\n");
  write_userfile(-1);
}

static void cmd_backup(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# backup", dcc[idx].nick);
  dprintf(idx, "Backing up the channel & user files...\n");
  call_hook(HOOK_BACKUP);
}

static void cmd_trace(struct userrec *u, int idx, char *par)
{
  int i;
  char x[NOTENAMELEN + 11], y[11];

  if (!par[0]) {
    dprintf(idx, "Usage: trace <botname>\n");
    return;
  }
  if (!egg_strcasecmp(par, botnetnick)) {
    dprintf(idx, "That's me!  Hiya! :)\n");
    return;
  }
  i = nextbot(par);
  if (i < 0) {
    dprintf(idx, "Unreachable bot.\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# trace %s", dcc[idx].nick, par);
  simple_sprintf(x, "%d:%s@%s", dcc[idx].sock, dcc[idx].nick, botnetnick);
  simple_sprintf(y, ":%d", now);
  botnet_send_trace(i, x, par, y);
}

static void cmd_binds(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# binds %s", dcc[idx].nick, par);
  tell_binds(idx, par);
}

static void cmd_banner(struct userrec *u, int idx, char *par)
{
  char s[1024];
  int i;

  if (!par[0]) {
    dprintf(idx, "Usage: banner <message>\n");
    return;
  }
  simple_sprintf(s, "\007### Botwide: [%s] %s\n", dcc[idx].nick, par);
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type->flags & DCT_MASTER)
      dprintf(i, "%s", s);
}

/* After messing with someone's user flags, make sure the dcc-chat flags
 * are set correctly.
 */
int check_dcc_attrs(struct userrec *u, int oatr)
{
  int i, stat;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  if (!u)
    return 0;
  /* Make sure default owners are +n */
  if (isowner(u->handle)) {
    u->flags = sanity_check(u->flags | USER_OWNER);
  }
  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type->flags & DCT_MASTER) &&
        (!egg_strcasecmp(u->handle, dcc[i].nick))) {
      stat = dcc[i].status;
      if ((dcc[i].type == &DCC_CHAT) &&
          ((u->flags & (USER_OP | USER_MASTER | USER_OWNER | USER_BOTMAST)) !=
          (oatr & (USER_OP | USER_MASTER | USER_OWNER | USER_BOTMAST)))) {
        botnet_send_join_idx(i, -1);
      }
      if ((oatr & USER_MASTER) && !(u->flags & USER_MASTER)) {
        dprintf(i, "*** POOF! ***\n");
        dprintf(i, "You are no longer a master on this bot.\n");
      }
      if (!(oatr & USER_MASTER) && (u->flags & USER_MASTER)) {
        dcc[i].u.chat->con_flags |= conmask;
        dprintf(i, "*** POOF! ***\n");
        dprintf(i, "You are now a master on this bot.\n");
      }
      if (!(oatr & USER_BOTMAST) && (u->flags & USER_BOTMAST)) {
        dprintf(i, "### POOF! ###\n");
        dprintf(i, "You are now a botnet master on this bot.\n");
      }
      if ((oatr & USER_BOTMAST) && !(u->flags & USER_BOTMAST)) {
        dprintf(i, "### POOF! ###\n");
        dprintf(i, "You are no longer a botnet master on this bot.\n");
      }
      if (!(oatr & USER_OWNER) && (u->flags & USER_OWNER)) {
        dprintf(i, "@@@ POOF! @@@\n");
        dprintf(i, "You are now an OWNER of this bot.\n");
      }
      if ((oatr & USER_OWNER) && !(u->flags & USER_OWNER)) {
        dprintf(i, "@@@ POOF! @@@\n");
        dprintf(i, "You are no longer an owner of this bot.\n");
      }
      get_user_flagrec(u, &fr, dcc[i].u.chat->con_chan);
      dcc[i].u.chat->con_flags = check_conflags(&fr,
                                                dcc[i].u.chat->con_flags);
      if ((stat & STAT_PARTY) && (u->flags & USER_OP))
        stat &= ~STAT_PARTY;
      if (!(stat & STAT_PARTY) && !(u->flags & USER_OP) &&
          !(u->flags & USER_MASTER))
        stat |= STAT_PARTY;
      if ((stat & STAT_CHAT) && !(u->flags & USER_PARTY) &&
          !(u->flags & USER_MASTER) && (!(u->flags & USER_OP) || require_p))
        stat &= ~STAT_CHAT;
      if ((dcc[i].type->flags & DCT_FILES) && !(stat & STAT_CHAT) &&
          ((u->flags & USER_MASTER) || (u->flags & USER_PARTY) ||
          ((u->flags & USER_OP) && !require_p)))
        stat |= STAT_CHAT;
      dcc[i].status = stat;
      /* Check if they no longer have access to wherever they are.
       *
       * NOTE: DON'T kick someone off the party line just cuz they lost +p
       *       (pinvite script removes +p after 5 mins automatically)
       */
      if ((dcc[i].type->flags & DCT_FILES) && !(u->flags & USER_XFER) &&
          !(u->flags & USER_MASTER)) {
        dprintf(i, "-+- POOF! -+-\n");
        dprintf(i, "You no longer have file area access.\n\n");
        putlog(LOG_MISC, "*", "DCC user [%s]%s removed from file system",
               dcc[i].nick, dcc[i].host);
        if (dcc[i].status & STAT_CHAT) {
          struct chat_info *ci;

          ci = dcc[i].u.file->chat;
          nfree(dcc[i].u.file);
          dcc[i].u.chat = ci;
          dcc[i].status &= (~STAT_CHAT);
          dcc[i].type = &DCC_CHAT;
          if (dcc[i].u.chat->channel >= 0) {
            chanout_but(-1, dcc[i].u.chat->channel,
                        "*** %s has returned.\n", dcc[i].nick);
            if (dcc[i].u.chat->channel < GLOBAL_CHANS)
              botnet_send_join_idx(i, -1);
          }
        } else {
          killsock(dcc[i].sock);
          lostdcc(i);
        }
      }
    }

    if (dcc[i].type == &DCC_BOT && !egg_strcasecmp(u->handle, dcc[i].nick)) {
      if ((dcc[i].status & STAT_LEAF) && !(bot_flags(u) & BOT_LEAF))
        dcc[i].status &= ~(STAT_LEAF | STAT_WARNED);
      if (!(dcc[i].status & STAT_LEAF) && (bot_flags(u) & BOT_LEAF))
        dcc[i].status |= STAT_LEAF;
    }
  }

  return u->flags;
}

int check_dcc_chanattrs(struct userrec *u, char *chname, int chflags,
                        int ochatr)
{
  int i, found = 0;
  struct flag_record fr = { FR_CHAN, 0, 0, 0, 0, 0 };
  struct chanset_t *chan;

  if (!u)
    return 0;
  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type->flags & DCT_MASTER) &&
        !egg_strcasecmp(u->handle, dcc[i].nick)) {
      if ((dcc[i].type == &DCC_CHAT) &&
          ((chflags & (USER_OP | USER_MASTER | USER_OWNER)) !=
          (ochatr & (USER_OP | USER_MASTER | USER_OWNER))))
        botnet_send_join_idx(i, -1);
      if ((ochatr & USER_MASTER) && !(chflags & USER_MASTER)) {
        dprintf(i, "*** POOF! ***\n");
        dprintf(i, "You are no longer a master on %s.\n", chname);
      }
      if (!(ochatr & USER_MASTER) && (chflags & USER_MASTER)) {
        dcc[i].u.chat->con_flags |= conmask;
        dprintf(i, "*** POOF! ***\n");
        dprintf(i, "You are now a master on %s.\n", chname);
      }
      if (!(ochatr & USER_OWNER) && (chflags & USER_OWNER)) {
        dprintf(i, "@@@ POOF! @@@\n");
        dprintf(i, "You are now an OWNER of %s.\n", chname);
      }
      if ((ochatr & USER_OWNER) && !(chflags & USER_OWNER)) {
        dprintf(i, "@@@ POOF! @@@\n");
        dprintf(i, "You are no longer an owner of %s.\n", chname);
      }
      if (((ochatr & (USER_OP | USER_MASTER | USER_OWNER)) &&
          (!(chflags & (USER_OP | USER_MASTER | USER_OWNER)))) ||
          ((chflags & (USER_OP | USER_MASTER | USER_OWNER)) &&
          (!(ochatr & (USER_OP | USER_MASTER | USER_OWNER))))) {

        for (chan = chanset; chan && !found; chan = chan->next) {
          get_user_flagrec(u, &fr, chan->dname);
          if (fr.chan & (USER_OP | USER_MASTER | USER_OWNER))
            found = 1;
        }
        if (!chan)
          chan = chanset;
        if (chan)
          strcpy(dcc[i].u.chat->con_chan, chan->dname);
        else
          strcpy(dcc[i].u.chat->con_chan, "*");
      }
      fr.match = (FR_CHAN | FR_GLOBAL);
      get_user_flagrec(u, &fr, dcc[i].u.chat->con_chan);
      dcc[i].u.chat->con_flags = check_conflags(&fr,
                                                dcc[i].u.chat->con_flags);
    }
  }
  return chflags;
}

static void cmd_chattr(struct userrec *u, int idx, char *par)
{
  char *hand, *arg = NULL, *tmpchg = NULL, *chg = NULL, work[1024];
  struct chanset_t *chan = NULL;
  struct userrec *u2;
  struct flag_record pls = { 0, 0, 0, 0, 0, 0 },
                     mns = { 0, 0, 0, 0, 0, 0 },
                     user = { 0, 0, 0, 0, 0, 0 };
  module_entry *me;
  int fl = -1, of = 0, ocf = 0;

  if (!par[0]) {
    dprintf(idx, "Usage: chattr <handle> [changes] [channel]\n");
    return;
  }
  hand = newsplit(&par);
  u2 = get_user_by_handle(userlist, hand);
  if (!u2) {
    dprintf(idx, "No such user!\n");
    return;
  }

  /* Parse args */
  if (par[0]) {
    arg = newsplit(&par);
    if (par[0]) {
      /* .chattr <handle> <changes> <channel> */
      chg = arg;
      arg = newsplit(&par);
      chan = findchan_by_dname(arg);
    } else {
      chan = findchan_by_dname(arg);
      /* Consider modeless channels, starting with '+' */
      if (!(arg[0] == '+' && chan) &&
          !(arg[0] != '+' && strchr(CHANMETA, arg[0]))) {
        /* .chattr <handle> <changes> */
        chg = arg;
        chan = NULL; /* uh, !strchr (CHANMETA, channel[0]) && channel found?? */
        arg = NULL;
      }
      /* .chattr <handle> <channel>: nothing to do... */
    }
  }
  /* arg:  pointer to channel name, NULL if none specified
   * chan: pointer to channel structure, NULL if none found or none specified
   * chg:  pointer to changes, NULL if none specified
   */
  Assert(!(!arg && chan));
  if (arg && !chan) {
    dprintf(idx, "No channel record for %s.\n", arg);
    return;
  }
  if (chg) {
    if (!arg && strpbrk(chg, "&|")) {
      /* .chattr <handle> *[&|]*: use console channel if found... */
      if (!strcmp((arg = dcc[idx].u.chat->con_chan), "*"))
        arg = NULL;
      else
        chan = findchan_by_dname(arg);
      if (arg && !chan) {
        dprintf(idx, "Invalid console channel %s.\n", arg);
        return;
      }
    } else if (arg && !strpbrk(chg, "&|")) {
      tmpchg = nmalloc(strlen(chg) + 2);
      strcpy(tmpchg, "|");
      strcat(tmpchg, chg);
      chg = tmpchg;
    }
  }
  par = arg;
  user.match = FR_GLOBAL;
  if (chan)
    user.match |= FR_CHAN;
  get_user_flagrec(u, &user, chan ? chan->dname : 0);
  if (!chan && !glob_botmast(user)) {
    dprintf(idx, "You do not have Bot Master privileges.\n");
    if (tmpchg)
      nfree(tmpchg);
    return;
  }
  if (chan && !glob_master(user) && !chan_master(user)) {
    dprintf(idx, "You do not have channel master privileges for channel %s.\n",
            par);
    if (tmpchg)
      nfree(tmpchg);
    return;
  }
  user.match &= fl;
  if (chg) {
    pls.match = user.match;
    break_down_flags(chg, &pls, &mns);
    /* No-one can change these flags on-the-fly */
    pls.global &=~(USER_BOT);
    mns.global &=~(USER_BOT);

    if (chan) {
      pls.chan &= ~(BOT_SHARE);
      mns.chan &= ~(BOT_SHARE);
    }
    if (!glob_owner(user)) {
      pls.global &=~(USER_OWNER | USER_MASTER | USER_BOTMAST | USER_UNSHARED);
      mns.global &=~(USER_OWNER | USER_MASTER | USER_BOTMAST | USER_UNSHARED);

      if (chan) {
        pls.chan &= ~USER_OWNER;
        mns.chan &= ~USER_OWNER;
      }
      if (!glob_master(user)) {
        pls.global &=USER_PARTY | USER_XFER;
        mns.global &=USER_PARTY | USER_XFER;

        if (!glob_botmast(user)) {
          pls.global = 0;
          mns.global = 0;
        }
      }
    }
    if (chan && !chan_owner(user) && !glob_owner(user)) {
      pls.chan &= ~USER_MASTER;
      mns.chan &= ~USER_MASTER;
      if (!chan_master(user) && !glob_master(user)) {
        pls.chan = 0;
        mns.chan = 0;
      }
    }
    get_user_flagrec(u2, &user, par);
    if (user.match & FR_GLOBAL) {
      of = user.global;
      user.global = sanity_check((user.global |pls.global) &~mns.global);

      user.udef_global = (user.udef_global | pls.udef_global)
                         & ~mns.udef_global;
    }
    if (chan) {
      ocf = user.chan;
      user.chan = chan_sanity_check((user.chan | pls.chan) & ~mns.chan,
                                    user.global);

      user.udef_chan = (user.udef_chan | pls.udef_chan) & ~mns.udef_chan;
    }
    set_user_flagrec(u2, &user, par);
  }
  if (chan)
    putlog(LOG_CMDS, "*", "#%s# (%s) chattr %s %s",
           dcc[idx].nick, chan ? chan->dname : "*", hand, chg ? chg : "");
  else
    putlog(LOG_CMDS, "*", "#%s# chattr %s %s", dcc[idx].nick, hand,
           chg ? chg : "");
  /* Get current flags and display them */
  if (user.match & FR_GLOBAL) {
    user.match = FR_GLOBAL;
    if (chg)
      check_dcc_attrs(u2, of);
    get_user_flagrec(u2, &user, NULL);
    build_flags(work, &user, NULL);
    if (work[0] != '-')
      dprintf(idx, "Global flags for %s are now +%s.\n", hand, work);
    else
      dprintf(idx, "No global flags for %s.\n", hand);
  }
  if (chan) {
    user.match = FR_CHAN;
    get_user_flagrec(u2, &user, par);
    user.chan &= ~BOT_SHARE;
    if (chg)
      check_dcc_chanattrs(u2, chan->dname, user.chan, ocf);
    build_flags(work, &user, NULL);
    if (work[0] != '-')
      dprintf(idx, "Channel flags for %s on %s are now +%s.\n", hand,
              chan->dname, work);
    else
      dprintf(idx, "No flags for %s on %s.\n", hand, chan->dname);
  }
  if (chg && (me = module_find("irc", 0, 0))) {
    Function *func = me->funcs;
    (func[IRC_CHECK_THIS_USER]) (hand, 0, NULL);
  }
  if (tmpchg)
    nfree(tmpchg);
}

static void cmd_botattr(struct userrec *u, int idx, char *par)
{
  char *hand, *chg = NULL, *arg = NULL, *tmpchg = NULL, work[1024];
  struct chanset_t *chan = NULL;
  struct userrec *u2;
  struct flag_record  pls = { 0, 0, 0, 0, 0, 0 },
                      mns = { 0, 0, 0, 0, 0, 0 },
                      user = { 0, 0, 0, 0, 0, 0 };
  int idx2;

  if (!par[0]) {
    dprintf(idx, "Usage: botattr <handle> [changes] [channel]\n");
    return;
  }
  hand = newsplit(&par);
  u2 = get_user_by_handle(userlist, hand);
  if (!u2 || !(u2->flags & USER_BOT)) {
    dprintf(idx, "No such bot!\n");
    return;
  }
  for (idx2 = 0; idx2 < dcc_total; idx2++)
    if (dcc[idx2].type != &DCC_RELAY && dcc[idx2].type != &DCC_FORK_BOT &&
        !egg_strcasecmp(dcc[idx2].nick, hand))
      break;
  if (idx2 != dcc_total) {
    dprintf(idx,
            "You may not change the attributes of a directly linked bot.\n");
    return;
  }
  /* Parse args */
  if (par[0]) {
    arg = newsplit(&par);
    if (par[0]) {
      /* .botattr <handle> <changes> <channel> */
      chg = arg;
      arg = newsplit(&par);
      chan = findchan_by_dname(arg);
    } else {
      chan = findchan_by_dname(arg);
      /* Consider modeless channels, starting with '+' */
      if (!(arg[0] == '+' && chan) &&
          !(arg[0] != '+' && strchr(CHANMETA, arg[0]))) {
        /* .botattr <handle> <changes> */
        chg = arg;
        chan = NULL; /* uh, !strchr (CHANMETA, channel[0]) && channel found?? */
        arg = NULL;
      }
      /* .botattr <handle> <channel>: nothing to do... */
    }
  }
  /* arg:  pointer to channel name, NULL if none specified
   * chan: pointer to channel structure, NULL if none found or none specified
   * chg:  pointer to changes, NULL if none specified
   */
  Assert(!(!arg && chan));
  if (arg && !chan) {
    dprintf(idx, "No channel record for %s.\n", arg);
    return;
  }
  if (chg) {
    if (!arg && strpbrk(chg, "&|")) {
      /* botattr <handle> *[&|]*: use console channel if found... */
      if (!strcmp((arg = dcc[idx].u.chat->con_chan), "*"))
        arg = NULL;
      else
        chan = findchan_by_dname(arg);
      if (arg && !chan) {
        dprintf(idx, "Invalid console channel %s.\n", arg);
        return;
      }
    } else if (arg && !strpbrk(chg, "&|")) {
      tmpchg = nmalloc(strlen(chg) + 2);
      strcpy(tmpchg, "|");
      strcat(tmpchg, chg);
      chg = tmpchg;
    }
  }
  par = arg;

  user.match = FR_GLOBAL;
  get_user_flagrec(u, &user, chan ? chan->dname : 0);
  if (!glob_botmast(user)) {
    dprintf(idx, "You do not have Bot Master privileges.\n");
    if (tmpchg)
      nfree(tmpchg);
    return;
  }
  if (chg) {
    user.match = FR_BOT | (chan ? FR_CHAN : 0);
    pls.match = user.match;
    break_down_flags(chg, &pls, &mns);
    /* No-one can change these flags on-the-fly */
    pls.global &=~BOT_BOT;
    mns.global &=~BOT_BOT;

    if (chan && glob_owner(user)) {
      pls.chan &= BOT_SHARE;
      mns.chan &= BOT_SHARE;
    } else {
      pls.chan = 0;
      mns.chan = 0;
    }
    if (!glob_owner(user)) {
      pls.bot &= ~(BOT_SHARE | BOT_GLOBAL);
      mns.bot &= ~(BOT_SHARE | BOT_GLOBAL);
    }
    user.match = FR_BOT | (chan ? FR_CHAN : 0);
    get_user_flagrec(u2, &user, par);
    user.bot = (user.bot | pls.bot) & ~mns.bot;
    if ((user.bot & BOT_SHARE) == BOT_SHARE)
      user.bot &= ~BOT_SHARE;
    if (chan)
      user.chan = (user.chan | pls.chan) & ~mns.chan;
    set_user_flagrec(u2, &user, par);
  }
  if (chan)
    putlog(LOG_CMDS, "*", "#%s# (%s) botattr %s %s",
           dcc[idx].nick, chan->dname, hand, chg ? chg : "");
  else
    putlog(LOG_CMDS, "*", "#%s# botattr %s %s", dcc[idx].nick, hand,
           chg ? chg : "");
  /* get current flags and display them */
  if (!chan || pls.bot || mns.bot) {
    user.match = FR_BOT;
    get_user_flagrec(u2, &user, NULL);
    build_flags(work, &user, NULL);
    if (work[0] != '-')
      dprintf(idx, "Bot flags for %s are now +%s.\n", hand, work);
    else
      dprintf(idx, "There are no bot flags for %s.\n", hand);
  }
  if (chan) {
    user.match = FR_CHAN;
    get_user_flagrec(u2, &user, par);
    user.chan &= BOT_SHARE;
    user.udef_chan = 0; /* udef chan flags are user only */
    build_flags(work, &user, NULL);
    if (work[0] != '-')
      dprintf(idx, "Bot flags for %s on %s are now +%s.\n", hand,
              chan->dname, work);
    else
      dprintf(idx, "There are no bot flags for %s on %s.\n", hand, chan->dname);
  }
  if (tmpchg)
    nfree(tmpchg);
}

static void cmd_chat(struct userrec *u, int idx, char *par)
{
  char *arg;
  int newchan, oldchan;
  module_entry *me;

  arg = newsplit(&par);
  if (!egg_strcasecmp(arg, "off")) {
    /* Turn chat off */
    if (dcc[idx].u.chat->channel < 0) {
      dprintf(idx, "You weren't in chat anyway!\n");
      return;
    } else {
      dprintf(idx, "Leaving chat mode...\n");
      check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock,
                     dcc[idx].u.chat->channel);
      chanout_but(-1, dcc[idx].u.chat->channel,
                  "*** %s left the party line.\n", dcc[idx].nick);
      if (dcc[idx].u.chat->channel < GLOBAL_CHANS)
        botnet_send_part_idx(idx, "");
    }
    dcc[idx].u.chat->channel = -1;
  } else {
    if (arg[0] == '*') {
      if (((arg[1] < '0') || (arg[1] > '9'))) {
        if (!arg[1])
          newchan = 0;
        else {
          Tcl_SetVar(interp, "_chan", arg, 0);
          if ((Tcl_VarEval(interp, "assoc ", "$_chan", NULL) == TCL_OK) &&
              !tcl_resultempty())
            newchan = tcl_resultint();
          else
            newchan = -1;
        }
        if (newchan < 0) {
          dprintf(idx, "No channel exists by that name.\n");
          return;
        }
      } else
        newchan = GLOBAL_CHANS + atoi(arg + 1);
      if (newchan < GLOBAL_CHANS || newchan > 199999) {
        dprintf(idx, "Channel number out of range: local channels must be "
                "*0-*99999.\n");
        return;
      }
    } else {
      if (((arg[0] < '0') || (arg[0] > '9')) && (arg[0])) {
        if (!egg_strcasecmp(arg, "on"))
          newchan = 0;
        else {
          Tcl_SetVar(interp, "_chan", arg, 0);
          if ((Tcl_VarEval(interp, "assoc ", "$_chan", NULL) == TCL_OK) &&
              !tcl_resultempty())
            newchan = tcl_resultint();
          else
            newchan = -1;
        }
        if (newchan < 0) {
          dprintf(idx, "No channel exists by that name.\n");
          return;
        }
      } else
        newchan = atoi(arg);
      if ((newchan < 0) || (newchan >= GLOBAL_CHANS)) {
        dprintf(idx, "Channel number out of range: must be between 0 and %d."
                "\n", GLOBAL_CHANS);
        return;
      }
    }
    /* If coming back from being off the party line, make sure they're
     * not away.
     */
    if ((dcc[idx].u.chat->channel < 0) && (dcc[idx].u.chat->away != NULL))
      not_away(idx);
    if (dcc[idx].u.chat->channel == newchan) {
      if (!newchan) {
        dprintf(idx, "You're already on the party line!\n");
        return;
      } else {
        dprintf(idx, "You're already on channel %s%d!\n",
                (newchan < GLOBAL_CHANS) ? "" : "*", newchan % GLOBAL_CHANS);
        return;
      }
    } else {
      oldchan = dcc[idx].u.chat->channel;
      if (oldchan >= 0)
        check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock, oldchan);
      if (!oldchan)
        chanout_but(-1, 0, "*** %s left the party line.\n", dcc[idx].nick);
      else if (oldchan > 0)
        chanout_but(-1, oldchan, "*** %s left the channel.\n", dcc[idx].nick);
      dcc[idx].u.chat->channel = newchan;
      if (!newchan) {
        dprintf(idx, "Entering the party line...\n");
        chanout_but(-1, 0, "*** %s joined the party line.\n", dcc[idx].nick);
      } else {
        dprintf(idx, "Joining channel '%s'...\n", arg);
        chanout_but(-1, newchan, "*** %s joined the channel.\n", dcc[idx].nick);
      }
      check_tcl_chjn(botnetnick, dcc[idx].nick, newchan, geticon(idx),
                     dcc[idx].sock, dcc[idx].host);
      if (newchan < GLOBAL_CHANS)
        botnet_send_join_idx(idx, oldchan);
      else if (oldchan < GLOBAL_CHANS)
        botnet_send_part_idx(idx, "");
    }
  }
  /* New style autosave here too -- rtc, 09/28/1999 */
  if ((me = module_find("console", 1, 1))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (idx);
  }
}

static void cmd_echo(struct userrec *u, int idx, char *par)
{
  module_entry *me;

  if (!par[0]) {
    dprintf(idx, "Echo is currently %s.\n", dcc[idx].status & STAT_ECHO ?
            "on" : "off");
    return;
  }
  if (!egg_strcasecmp(par, "on")) {
    dprintf(idx, "Echo turned on.\n");
    dcc[idx].status |= STAT_ECHO;
  } else if (!egg_strcasecmp(par, "off")) {
    dprintf(idx, "Echo turned off.\n");
    dcc[idx].status &= ~STAT_ECHO;
  } else {
    dprintf(idx, "Usage: echo <on/off>\n");
    return;
  }
  /* New style autosave here too -- rtc, 09/28/1999 */
  if ((me = module_find("console", 1, 1))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (idx);
  }
}

int stripmodes(char *s)
{
  int res = 0;

  for (; *s; s++)
    switch (tolower((unsigned) *s)) {
    case 'b':
      res |= STRIP_BOLD;
      break;
    case 'c':
      res |= STRIP_COLOR;
      break;
    case 'r':
      res |= STRIP_REV;
      break;
    case 'u':
      res |= STRIP_UNDER;
      break;
    case 'a':
      res |= STRIP_ANSI;
      break;
    case 'g':
      res |= STRIP_BELLS;
      break;
    case '*':
      res |= STRIP_ALL;
      break;
    }
  return res;
}

char *stripmasktype(int x)
{
  static char s[20];
  char *p = s;

  if (x & STRIP_BOLD)
    *p++ = 'b';
  if (x & STRIP_COLOR)
    *p++ = 'c';
  if (x & STRIP_REV)
    *p++ = 'r';
  if (x & STRIP_UNDER)
    *p++ = 'u';
  if (x & STRIP_ANSI)
    *p++ = 'a';
  if (x & STRIP_BELLS)
    *p++ = 'g';
  if (p == s)
    *p++ = '-';
  *p = 0;
  return s;
}

static char *stripmaskname(int x)
{
  static char s[161];
  int i = 0;

  s[i] = 0;
  if (x & STRIP_BOLD)
    i += my_strcpy(s + i, "bold, ");
  if (x & STRIP_COLOR)
    i += my_strcpy(s + i, "color, ");
  if (x & STRIP_REV)
    i += my_strcpy(s + i, "reverse, ");
  if (x & STRIP_UNDER)
    i += my_strcpy(s + i, "underline, ");
  if (x & STRIP_ANSI)
    i += my_strcpy(s + i, "ansi, ");
  if (x & STRIP_BELLS)
    i += my_strcpy(s + i, "bells, ");
  if (!i)
    strcpy(s, "none");
  else
    s[i - 2] = 0;
  return s;
}

static void cmd_strip(struct userrec *u, int idx, char *par)
{
  char *nick, *changes, *c, s[2];
  int dest = 0, i, pls, md, ok = 0;
  module_entry *me;

  if (!par[0]) {
    dprintf(idx, "Your current strip settings are: %s (%s).\n",
            stripmasktype(dcc[idx].u.chat->strip_flags),
            stripmaskname(dcc[idx].u.chat->strip_flags));
    return;
  }
  nick = newsplit(&par);
  if ((nick[0] != '+') && (nick[0] != '-') && u && (u->flags & USER_MASTER)) {
    for (i = 0; i < dcc_total; i++)
      if (!egg_strcasecmp(nick, dcc[i].nick) && dcc[i].type == &DCC_CHAT && !ok) {
        ok = 1;
        dest = i;
      }
    if (!ok) {
      dprintf(idx, "No such user on the party line!\n");
      return;
    }
    changes = par;
  } else {
    changes = nick;
    nick = "";
    dest = idx;
  }
  c = changes;
  if ((c[0] != '+') && (c[0] != '-'))
    dcc[dest].u.chat->strip_flags = 0;
  s[1] = 0;
  for (pls = 1; *c; c++) {
    switch (*c) {
    case '+':
      pls = 1;
      break;
    case '-':
      pls = 0;
      break;
    default:
      s[0] = *c;
      md = stripmodes(s);
      if (pls == 1)
        dcc[dest].u.chat->strip_flags |= md;
      else
        dcc[dest].u.chat->strip_flags &= ~md;
    }
  }
  if (nick[0])
    putlog(LOG_CMDS, "*", "#%s# strip %s %s", dcc[idx].nick, nick, changes);
  else
    putlog(LOG_CMDS, "*", "#%s# strip %s", dcc[idx].nick, changes);
  if (dest == idx) {
    dprintf(idx, "Your strip settings are: %s (%s).\n",
            stripmasktype(dcc[idx].u.chat->strip_flags),
            stripmaskname(dcc[idx].u.chat->strip_flags));
  } else {
    dprintf(idx, "Strip setting for %s: %s (%s).\n", dcc[dest].nick,
            stripmasktype(dcc[dest].u.chat->strip_flags),
            stripmaskname(dcc[dest].u.chat->strip_flags));
    dprintf(dest, "%s set your strip settings to: %s (%s).\n", dcc[idx].nick,
            stripmasktype(dcc[dest].u.chat->strip_flags),
            stripmaskname(dcc[dest].u.chat->strip_flags));
  }
  /* Set highlight flag here so user is able to control stripping of
   * bold also as intended -- dw 27/12/1999
   */
  if (dcc[dest].u.chat->strip_flags & STRIP_BOLD && u->flags & USER_HIGHLITE) {
    u->flags &= ~USER_HIGHLITE;
  } else if (!(dcc[dest].u.chat->strip_flags & STRIP_BOLD) &&
           !(u->flags & USER_HIGHLITE)) {
    u->flags |= USER_HIGHLITE;
  }
  /* New style autosave here too -- rtc, 09/28/1999 */
  if ((me = module_find("console", 1, 1))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (dest);
  }
}

static void cmd_su(struct userrec *u, int idx, char *par)
{
  int atr = u ? u->flags : 0;
  struct flag_record fr = { FR_ANYWH | FR_CHAN | FR_GLOBAL, 0, 0, 0, 0, 0 };

  u = get_user_by_handle(userlist, par);

  if (!par[0])
    dprintf(idx, "Usage: su <user>\n");
  else if (!u)
    dprintf(idx, "No such user.\n");
  else if (u->flags & USER_BOT)
    dprintf(idx, "You can't su to a bot... then again, why would you wanna?\n");
  else if (dcc[idx].u.chat->su_nick)
    dprintf(idx, "You cannot currently double .su; try .su'ing directly.\n");
  else {
    get_user_flagrec(u, &fr, NULL);
    if ((!glob_party(fr) && (require_p || !(glob_op(fr) || chan_op(fr)))) &&
        !(atr & USER_BOTMAST))
      dprintf(idx, "No party line access permitted for %s.\n", par);
    else {
      correct_handle(par);
      putlog(LOG_CMDS, "*", "#%s# su %s", dcc[idx].nick, par);
      if (!(atr & USER_OWNER) || ((u->flags & USER_OWNER) && (isowner(par)) &&
          !(isowner(dcc[idx].nick)))) {
        /* This check is only important for non-owners */
        if (u_pass_match(u, "-")) {
          dprintf(idx, "No password set for user. You may not .su to them.\n");
          return;
        }
        if (dcc[idx].u.chat->channel < GLOBAL_CHANS)
          botnet_send_part_idx(idx, "");
        chanout_but(-1, dcc[idx].u.chat->channel,
                    "*** %s left the party line.\n", dcc[idx].nick);
        /* Store the old nick in the away section, for weenies who can't get
         * their password right ;)
         */
        if (dcc[idx].u.chat->away != NULL)
          nfree(dcc[idx].u.chat->away);
        dcc[idx].u.chat->away = get_data_ptr(strlen(dcc[idx].nick) + 1);
        strcpy(dcc[idx].u.chat->away, dcc[idx].nick);
        dcc[idx].u.chat->su_nick = get_data_ptr(strlen(dcc[idx].nick) + 1);
        strcpy(dcc[idx].u.chat->su_nick, dcc[idx].nick);
        dcc[idx].user = u;
        strcpy(dcc[idx].nick, par);
        /* Display password prompt and turn off echo (send IAC WILL ECHO). */
        dprintf(idx, "Enter password for %s%s\n", par,
                (dcc[idx].status & STAT_TELNET) ? TLN_IAC_C TLN_WILL_C
                TLN_ECHO_C : "");
        dcc[idx].type = &DCC_CHAT_PASS;
      } else if (atr & USER_OWNER) {
        if (dcc[idx].u.chat->channel < GLOBAL_CHANS)
          botnet_send_part_idx(idx, "");
        chanout_but(-1, dcc[idx].u.chat->channel,
                    "*** %s left the party line.\n", dcc[idx].nick);
        dprintf(idx, "Setting your username to %s.\n", par);
        if (atr & USER_MASTER)
          dcc[idx].u.chat->con_flags = conmask;
        dcc[idx].u.chat->su_nick = get_data_ptr(strlen(dcc[idx].nick) + 1);
        strcpy(dcc[idx].u.chat->su_nick, dcc[idx].nick);
        dcc[idx].user = u;
        strcpy(dcc[idx].nick, par);
        dcc_chatter(idx);
      }
    }
  }
}

static void cmd_fixcodes(struct userrec *u, int idx, char *par)
{
  if (dcc[idx].status & STAT_TELNET) {
    dcc[idx].status |= STAT_ECHO;
    dcc[idx].status &= ~STAT_TELNET;
    dprintf(idx, "Turned off telnet codes.\n");
    putlog(LOG_CMDS, "*", "#%s# fixcodes (telnet off)", dcc[idx].nick);
  } else {
    dcc[idx].status |= STAT_TELNET;
    dcc[idx].status &= ~STAT_ECHO;
    dprintf(idx, "Turned on telnet codes.\n");
    putlog(LOG_CMDS, "*", "#%s# fixcodes (telnet on)", dcc[idx].nick);
  }
}

static void cmd_page(struct userrec *u, int idx, char *par)
{
  int a;
  module_entry *me;

  if (!par[0]) {
    if (dcc[idx].status & STAT_PAGE) {
      dprintf(idx, "Currently paging outputs to %d lines.\n",
              dcc[idx].u.chat->max_line);
    } else
      dprintf(idx, "You don't have paging on.\n");
    return;
  }
  a = atoi(par);
  if ((!a && !par[0]) || !egg_strcasecmp(par, "off")) {
    dcc[idx].status &= ~STAT_PAGE;
    dcc[idx].u.chat->max_line = 0x7ffffff;      /* flush_lines needs this */
    while (dcc[idx].u.chat->buffer)
      flush_lines(idx, dcc[idx].u.chat);
    dprintf(idx, "Paging turned off.\n");
    putlog(LOG_CMDS, "*", "#%s# page off", dcc[idx].nick);
  } else if (a > 0) {
    dprintf(idx, "Paging turned on, stopping every %d line%s.\n", a,
            (a != 1) ? "s" : "");
    dcc[idx].status |= STAT_PAGE;
    dcc[idx].u.chat->max_line = a;
    dcc[idx].u.chat->line_count = 0;
    dcc[idx].u.chat->current_lines = 0;
    putlog(LOG_CMDS, "*", "#%s# page %d", dcc[idx].nick, a);
  } else {
    dprintf(idx, "Usage: page <off or #>\n");
    return;
  }
  /* New style autosave here too -- rtc, 09/28/1999 */
  if ((me = module_find("console", 1, 1))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (idx);
  }
}

/* Evaluate a Tcl command, send output to a dcc user.
 */
static void cmd_tcl(struct userrec *u, int idx, char *msg)
{
  int code;
  char *result;
#ifdef USE_TCL_ENCODING
  Tcl_DString dstr;
#endif

  if (!(isowner(dcc[idx].nick)) && (must_be_owner)) {
    dprintf(idx, MISC_NOSUCHCMD);
    return;
  }
  debug1("tcl: evaluate (.tcl): %s", msg);
  code = Tcl_GlobalEval(interp, msg);

#ifdef USE_TCL_ENCODING
  /* properly convert string to system encoding. */
  Tcl_DStringInit(&dstr);
  Tcl_UtfToExternalDString(NULL, tcl_resultstring(), -1, &dstr);
  result = Tcl_DStringValue(&dstr);
#else
  /* use old pre-Tcl 8.1 way. */
  result = tcl_resultstring();
#endif

  if (code == TCL_OK)
    dumplots(idx, "Tcl: ", result);
  else
    dumplots(idx, "Tcl error: ", result);

#ifdef USE_TCL_ENCODING
  Tcl_DStringFree(&dstr);
#endif
}

/* Perform a 'set' command
 */
static void cmd_set(struct userrec *u, int idx, char *msg)
{
  int code;
  char s[512], *result;
#ifdef USE_TCL_ENCODING
  Tcl_DString dstr;
#endif

  if (!(isowner(dcc[idx].nick)) && (must_be_owner)) {
    dprintf(idx, MISC_NOSUCHCMD);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# set %s", dcc[idx].nick, msg);
  strcpy(s, "set ");
  if (!msg[0]) {
    strcpy(s, "info globals");
    Tcl_Eval(interp, s);
    dumplots(idx, "Global vars: ", tcl_resultstring());
    return;
  }
  strcpy(s + 4, msg);
  code = Tcl_Eval(interp, s);

#ifdef USE_TCL_ENCODING
  /* properly convert string to system encoding. */
  Tcl_DStringInit(&dstr);
  Tcl_UtfToExternalDString(NULL, tcl_resultstring(), -1, &dstr);
  result = Tcl_DStringValue(&dstr);
#else
  /* use old pre-Tcl 8.1 way. */
  result = tcl_resultstring();
#endif

  if (code == TCL_OK) {
    if (!strchr(msg, ' '))
      dumplots(idx, "Currently: ", result);
    else
      dprintf(idx, "Ok, set.\n");
  } else
    dprintf(idx, "Error: %s\n", result);

#ifdef USE_TCL_ENCODING
  Tcl_DStringFree(&dstr);
#endif
}

static void cmd_module(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# module %s", dcc[idx].nick, par);
  do_module_report(idx, 2, par[0] ? par : NULL);
}

static void cmd_loadmod(struct userrec *u, int idx, char *par)
{
  const char *p;

  if (!(isowner(dcc[idx].nick)) && (must_be_owner)) {
    dprintf(idx, MISC_NOSUCHCMD);
    return;
  }
  if (!par[0]) {
    dprintf(idx, "%s: loadmod <module>\n", MISC_USAGE);
  } else {
    p = module_load(par);
    if (p)
      dprintf(idx, "%s: %s %s\n", par, MOD_LOADERROR, p);
    else {
      putlog(LOG_CMDS, "*", "#%s# loadmod %s", dcc[idx].nick, par);
      dprintf(idx, MOD_LOADED, par);
      dprintf(idx, "\n");
    }
  }
}

static void cmd_unloadmod(struct userrec *u, int idx, char *par)
{
  char *p;

  if (!(isowner(dcc[idx].nick)) && (must_be_owner)) {
    dprintf(idx, MISC_NOSUCHCMD);
    return;
  }
  if (!par[0])
    dprintf(idx, "%s: unloadmod <module>\n", MISC_USAGE);
  else {
    p = module_unload(par, dcc[idx].nick);
    if (p)
      dprintf(idx, "%s %s: %s\n", MOD_UNLOADERROR, par, p);
    else {
      putlog(LOG_CMDS, "*", "#%s# unloadmod %s", dcc[idx].nick, par);
      dprintf(idx, "%s %s\n", MOD_UNLOADED, par);
    }
  }
}

static void cmd_pls_ignore(struct userrec *u, int idx, char *par)
{
  char *who, s[UHOSTLEN];
  unsigned long int expire_time = 0;

  if (!par[0]) {
    dprintf(idx, "Usage: +ignore <hostmask> [%%<XdXhXm>] [comment]\n");
    return;
  }

  who = newsplit(&par);
  if (par[0] == '%') {
    char *p, *p_expire;
    unsigned long int expire_foo;

    p = newsplit(&par);
    p_expire = p + 1;
    while (*(++p) != 0) {
      switch (tolower((unsigned) *p)) {
      case 'd':
        *p = 0;
        expire_foo = strtol(p_expire, NULL, 10);
        if (expire_foo > 365)
          expire_foo = 365;
        expire_time += 86400 * expire_foo;
        p_expire = p + 1;
        break;
      case 'h':
        *p = 0;
        expire_foo = strtol(p_expire, NULL, 10);
        if (expire_foo > 8760)
          expire_foo = 8760;
        expire_time += 3600 * expire_foo;
        p_expire = p + 1;
        break;
      case 'm':
        *p = 0;
        expire_foo = strtol(p_expire, NULL, 10);
        if (expire_foo > 525600)
          expire_foo = 525600;
        expire_time += 60 * expire_foo;
        p_expire = p + 1;
      }
    }
  }
  if (!par[0])
    par = "requested";
  else if (strlen(par) > 65)
    par[65] = 0;
  if (strlen(who) > UHOSTMAX - 4)
    who[UHOSTMAX - 4] = 0;

  /* Fix missing ! or @ BEFORE continuing */
  if (!strchr(who, '!')) {
    if (!strchr(who, '@'))
      simple_sprintf(s, "%s!*@*", who);
    else
      simple_sprintf(s, "*!%s", who);
  } else if (!strchr(who, '@'))
    simple_sprintf(s, "%s@*", who);
  else
    strcpy(s, who);

  if (match_ignore(s))
    dprintf(idx, "That already matches an existing ignore.\n");
  else {
    dprintf(idx, "Now ignoring: %s (%s)\n", s, par);
    addignore(s, dcc[idx].nick, par, expire_time ? now + expire_time : 0L);
    putlog(LOG_CMDS, "*", "#%s# +ignore %s %s", dcc[idx].nick, s, par);
  }
}

static void cmd_mns_ignore(struct userrec *u, int idx, char *par)
{
  char buf[UHOSTLEN];

  if (!par[0]) {
    dprintf(idx, "Usage: -ignore <hostmask | ignore #>\n");
    return;
  }
  strncpyz(buf, par, sizeof buf);
  if (delignore(buf)) {
    putlog(LOG_CMDS, "*", "#%s# -ignore %s", dcc[idx].nick, buf);
    dprintf(idx, "No longer ignoring: %s\n", buf);
  } else
    dprintf(idx, "That ignore cannot be found.\n");
}

static void cmd_ignores(struct userrec *u, int idx, char *par)
{
  putlog(LOG_CMDS, "*", "#%s# ignores %s", dcc[idx].nick, par);
  tell_ignores(idx, par);
}

static void cmd_pls_user(struct userrec *u, int idx, char *par)
{
  char *handle, *host;

  if (!par[0]) {
    dprintf(idx, "Usage: +user <handle> [hostmask]\n");
    return;
  }
  handle = newsplit(&par);
  host = newsplit(&par);
  if (strlen(handle) > HANDLEN)
    handle[HANDLEN] = 0;
  if (get_user_by_handle(userlist, handle))
    dprintf(idx, "Someone already exists by that name.\n");
  else if (strchr(BADNICKCHARS, handle[0]) != NULL)
    dprintf(idx, "You can't start a nick with '%c'.\n", handle[0]);
  else if (!egg_strcasecmp(handle, botnetnick))
    dprintf(idx, "Hey! That's MY name!\n");
  else {
    putlog(LOG_CMDS, "*", "#%s# +user %s %s", dcc[idx].nick, handle, host);
    userlist = adduser(userlist, handle, host, "-", 0);
    dprintf(idx, "Added %s (%s) with no password and no flags.\n", handle,
            host[0] ? host : "no host");
  }
}

static void cmd_mns_user(struct userrec *u, int idx, char *par)
{
  int idx2;
  char *handle;
  struct userrec *u2;
  module_entry *me;

  if (!par[0]) {
    dprintf(idx, "Usage: -user <hand>\n");
    return;
  }
  handle = newsplit(&par);
  u2 = get_user_by_handle(userlist, handle);
  if (!u2 || !u) {
    dprintf(idx, "No such user!\n");
    return;
  }
  if (isowner(u2->handle)) {
    dprintf(idx, "You can't remove a permanent bot owner!\n");
    return;
  }
  if ((u2->flags & USER_OWNER) && !(u->flags & USER_OWNER)) {
    dprintf(idx, "You can't remove a bot owner!\n");
    return;
  }
  if (u2->flags & USER_BOT) {
    if ((bot_flags(u2) & BOT_SHARE) && !(u->flags & USER_OWNER)) {
      dprintf(idx, "You can't remove share bots.\n");
      return;
    }
    for (idx2 = 0; idx2 < dcc_total; idx2++)
      if (dcc[idx2].type != &DCC_RELAY && dcc[idx2].type != &DCC_FORK_BOT &&
          !egg_strcasecmp(dcc[idx2].nick, handle))
        break;
    if (idx2 != dcc_total) {
      dprintf(idx, "You can't remove a directly linked bot.\n");
      return;
    }
  }
  if ((u->flags & USER_BOTMAST) && !(u->flags & USER_MASTER) &&
      !(u2->flags & USER_BOT)) {
    dprintf(idx, "You can't remove users who aren't bots!\n");
    return;
  }
  if ((me = module_find("irc", 0, 0))) {
    Function *func = me->funcs;

    (func[IRC_CHECK_THIS_USER]) (handle, 1, NULL);
  }
  if (deluser(handle)) {
    putlog(LOG_CMDS, "*", "#%s# -user %s", dcc[idx].nick, handle);
    dprintf(idx, "Deleted %s.\n", handle);
  } else
    dprintf(idx, "Failed.\n");
}

static void cmd_pls_host(struct userrec *u, int idx, char *par)
{
  char *handle, *host;
  struct userrec *u2;
  struct list_type *q;
  struct flag_record fr2 = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 },
                      fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };
  module_entry *me;

  if (!par[0]) {
    dprintf(idx, "Usage: +host [handle] <newhostmask>\n");
    return;
  }

  handle = newsplit(&par);

  if (par[0]) {
    host = newsplit(&par);
    u2 = get_user_by_handle(userlist, handle);
  } else {
    host = handle;
    handle = dcc[idx].nick;
    u2 = u;
  }
  if (!u2 || !u) {
    dprintf(idx, "No such user.\n");
    return;
  }
  get_user_flagrec(u, &fr, NULL);
  if (egg_strcasecmp(handle, dcc[idx].nick)) {
    get_user_flagrec(u2, &fr2, NULL);
    if (!glob_master(fr) && !glob_bot(fr2) && !chan_master(fr)) {
      dprintf(idx, "You can't add hostmasks to non-bots.\n");
      return;
    }
    if (!glob_owner(fr) && glob_bot(fr2) && (bot_flags(u2) & BOT_SHARE)) {
      dprintf(idx, "You can't add hostmasks to share bots.\n");
      return;
    }
    if ((glob_owner(fr2) || glob_master(fr2)) && !glob_owner(fr)) {
      dprintf(idx, "You can't add hostmasks to a bot owner/master.\n");
      return;
    }
    if ((chan_owner(fr2) || chan_master(fr2)) && !glob_master(fr) &&
        !glob_owner(fr) && !chan_owner(fr)) {
      dprintf(idx, "You can't add hostmasks to a channel owner/master.\n");
      return;
    }
    if (!glob_botmast(fr) && !glob_master(fr) && !chan_master(fr)) {
      dprintf(idx, "Permission denied.\n");
      return;
    }
  }
  if (!glob_botmast(fr) && !chan_master(fr) && get_user_by_host(host)) {
    dprintf(idx, "You cannot add a host matching another user!\n");
    return;
  }
  for (q = get_user(&USERENTRY_HOSTS, u); q; q = q->next)
    if (!egg_strcasecmp(q->extra, host)) {
      dprintf(idx, "That hostmask is already there.\n");
      return;
    }
  putlog(LOG_CMDS, "*", "#%s# +host %s %s", dcc[idx].nick, handle, host);
  addhost_by_handle(handle, host);
  dprintf(idx, "Added '%s' to %s.\n", host, handle);
  if ((me = module_find("irc", 0, 0))) {
    Function *func = me->funcs;

    (func[IRC_CHECK_THIS_USER]) (handle, 0, NULL);
  }
}

static void cmd_mns_host(struct userrec *u, int idx, char *par)
{
  char *handle, *host;
  struct userrec *u2;
  struct flag_record fr2 = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 },
                      fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };
  module_entry *me;

  if (!par[0]) {
    dprintf(idx, "Usage: -host [handle] <hostmask>\n");
    return;
  }
  handle = newsplit(&par);
  if (par[0]) {
    host = newsplit(&par);
    u2 = get_user_by_handle(userlist, handle);
  } else {
    host = handle;
    handle = dcc[idx].nick;
    u2 = u;
  }
  if (!u2 || !u) {
    dprintf(idx, "No such user.\n");
    return;
  }

  get_user_flagrec(u, &fr, NULL);
  get_user_flagrec(u2, &fr2, NULL);
  /* check to see if user is +d or +k and don't let them remove hosts */
  if (((glob_deop(fr) || glob_kick(fr)) && !glob_master(fr)) ||
      ((chan_deop(fr) || chan_kick(fr)) && !chan_master(fr))) {
    dprintf(idx, "You can't remove hostmasks while having the +d or +k "
            "flag.\n");
    return;
  }

  if (egg_strcasecmp(handle, dcc[idx].nick)) {
    if (!glob_master(fr) && !glob_bot(fr2) && !chan_master(fr)) {
      dprintf(idx, "You can't remove hostmasks from non-bots.\n");
      return;
    }
    if (glob_bot(fr2) && (bot_flags(u2) & BOT_SHARE) && !glob_owner(fr)) {
      dprintf(idx, "You can't remove hostmasks from a share bot.\n");
      return;
    }
    if ((glob_owner(fr2) || glob_master(fr2)) && !glob_owner(fr)) {
      dprintf(idx, "You can't remove hostmasks from a bot owner/master.\n");
      return;
    }
    if ((chan_owner(fr2) || chan_master(fr2)) && !glob_master(fr) &&
        !glob_owner(fr) && !chan_owner(fr)) {
      dprintf(idx, "You can't remove hostmasks from a channel owner/master.\n");
      return;
    }
    if (!glob_botmast(fr) && !glob_master(fr) && !chan_master(fr)) {
      dprintf(idx, "Permission denied.\n");
      return;
    }
  }
  if (delhost_by_handle(handle, host)) {
    putlog(LOG_CMDS, "*", "#%s# -host %s %s", dcc[idx].nick, handle, host);
    dprintf(idx, "Removed '%s' from %s.\n", host, handle);
    if ((me = module_find("irc", 0, 0))) {
      Function *func = me->funcs;

      (func[IRC_CHECK_THIS_USER]) (handle, 2, host);
    }
  } else
    dprintf(idx, "Failed.\n");
}

static void cmd_modules(struct userrec *u, int idx, char *par)
{
  int ptr;
  char *bot;
  module_entry *me;

  putlog(LOG_CMDS, "*", "#%s# modules %s", dcc[idx].nick, par);

  if (!par[0]) {
    dprintf(idx, "Modules loaded:\n");
    for (me = module_list; me; me = me->next)
      dprintf(idx, "  Module: %s (v%d.%d)\n", me->name, me->major, me->minor);
    dprintf(idx, "End of modules list.\n");
  } else {
    bot = newsplit(&par);
    if ((ptr = nextbot(bot)) >= 0)
      dprintf(ptr, "v %s %s %d:%s\n", botnetnick, bot, dcc[idx].sock,
              dcc[idx].nick);
    else
      dprintf(idx, "No such bot online.\n");
  }
}

static void cmd_traffic(struct userrec *u, int idx, char *par)
{
  unsigned long itmp, itmp2;

  dprintf(idx, "Traffic since last restart\n");
  dprintf(idx, "==========================\n");
  if (otraffic_irc > 0 || itraffic_irc > 0 || otraffic_irc_today > 0 ||
      itraffic_irc_today > 0) {
    dprintf(idx, "IRC:\n");
    dprintf(idx, "  out: %s", btos(otraffic_irc + otraffic_irc_today));
    dprintf(idx, " (%s today)\n", btos(otraffic_irc_today));
    dprintf(idx, "   in: %s", btos(itraffic_irc + itraffic_irc_today));
    dprintf(idx, " (%s today)\n", btos(itraffic_irc_today));
  }
  if (otraffic_bn > 0 || itraffic_bn > 0 || otraffic_bn_today > 0 ||
      itraffic_bn_today > 0) {
    dprintf(idx, "Botnet:\n");
    dprintf(idx, "  out: %s", btos(otraffic_bn + otraffic_bn_today));
    dprintf(idx, " (%s today)\n", btos(otraffic_bn_today));
    dprintf(idx, "   in: %s", btos(itraffic_bn + itraffic_bn_today));
    dprintf(idx, " (%s today)\n", btos(itraffic_bn_today));
  }
  if (otraffic_dcc > 0 || itraffic_dcc > 0 || otraffic_dcc_today > 0 ||
      itraffic_dcc_today > 0) {
    dprintf(idx, "Partyline:\n");
    itmp = otraffic_dcc + otraffic_dcc_today;
    itmp2 = otraffic_dcc_today;
    dprintf(idx, "  out: %s", btos(itmp));
    dprintf(idx, " (%s today)\n", btos(itmp2));
    dprintf(idx, "   in: %s", btos(itraffic_dcc + itraffic_dcc_today));
    dprintf(idx, " (%s today)\n", btos(itraffic_dcc_today));
  }
  if (otraffic_trans > 0 || itraffic_trans > 0 || otraffic_trans_today > 0 ||
      itraffic_trans_today > 0) {
    dprintf(idx, "Transfer.mod:\n");
    dprintf(idx, "  out: %s", btos(otraffic_trans + otraffic_trans_today));
    dprintf(idx, " (%s today)\n", btos(otraffic_trans_today));
    dprintf(idx, "   in: %s", btos(itraffic_trans + itraffic_trans_today));
    dprintf(idx, " (%s today)\n", btos(itraffic_trans_today));
  }
  if (otraffic_unknown > 0 || otraffic_unknown_today > 0) {
    dprintf(idx, "Misc:\n");
    dprintf(idx, "  out: %s", btos(otraffic_unknown + otraffic_unknown_today));
    dprintf(idx, " (%s today)\n", btos(otraffic_unknown_today));
    dprintf(idx, "   in: %s", btos(itraffic_unknown + itraffic_unknown_today));
    dprintf(idx, " (%s today)\n", btos(itraffic_unknown_today));
  }
  dprintf(idx, "---\n");
  dprintf(idx, "Total:\n");
  itmp = otraffic_irc + otraffic_bn + otraffic_dcc + otraffic_trans +
         otraffic_unknown + otraffic_irc_today + otraffic_bn_today +
         otraffic_dcc_today + otraffic_trans_today + otraffic_unknown_today;
  itmp2 = otraffic_irc_today + otraffic_bn_today + otraffic_dcc_today +
          otraffic_trans_today + otraffic_unknown_today;
  dprintf(idx, "  out: %s", btos(itmp));
  dprintf(idx, " (%s today)\n", btos(itmp2));
  dprintf(idx, "   in: %s", btos(itraffic_irc + itraffic_bn + itraffic_dcc +
          itraffic_trans + itraffic_unknown + itraffic_irc_today +
          itraffic_bn_today + itraffic_dcc_today + itraffic_trans_today +
          itraffic_unknown_today));
  dprintf(idx, " (%s today)\n", btos(itraffic_irc_today + itraffic_bn_today +
          itraffic_dcc_today + itraffic_trans_today + itraffic_unknown_today));
  putlog(LOG_CMDS, "*", "#%s# traffic", dcc[idx].nick);
}

static char traffictxt[20];
static char *btos(unsigned long bytes)
{
  char unit[10];
  float xbytes;

  sprintf(unit, "Bytes");
  xbytes = bytes;
  if (xbytes > 1024.0) {
    sprintf(unit, "KBytes");
    xbytes = xbytes / 1024.0;
  }
  if (xbytes > 1024.0) {
    sprintf(unit, "MBytes");
    xbytes = xbytes / 1024.0;
  }
  if (xbytes > 1024.0) {
    sprintf(unit, "GBytes");
    xbytes = xbytes / 1024.0;
  }
  if (xbytes > 1024.0) {
    sprintf(unit, "TBytes");
    xbytes = xbytes / 1024.0;
  }
  if (bytes > 1024)
    sprintf(traffictxt, "%.2f %s", xbytes, unit);
  else
    sprintf(traffictxt, "%lu Bytes", bytes);
  return traffictxt;
}

static void cmd_whoami(struct userrec *u, int idx, char *par)
{
  dprintf(idx, "You are %s@%s.\n", dcc[idx].nick, botnetnick);
  putlog(LOG_CMDS, "*", "#%s# whoami", dcc[idx].nick);
}

/* DCC CHAT COMMANDS
 */
/* Function call should be:
 *   static void cmd_whatever(struct userrec *u, int idx, char *par);
 * As with msg commands, function is responsible for any logging.
 */
cmd_t C_dcc[] = {
  {"+bot",      "t",    (IntFunc) cmd_pls_bot,    NULL},
  {"+host",     "t|m",  (IntFunc) cmd_pls_host,   NULL},
  {"+ignore",   "m",    (IntFunc) cmd_pls_ignore, NULL},
  {"+user",     "m",    (IntFunc) cmd_pls_user,   NULL},
  {"-bot",      "t",    (IntFunc) cmd_mns_user,   NULL},
  {"-host",     "",     (IntFunc) cmd_mns_host,   NULL},
  {"-ignore",   "m",    (IntFunc) cmd_mns_ignore, NULL},
  {"-user",     "m",    (IntFunc) cmd_mns_user,   NULL},
  {"addlog",    "to|o", (IntFunc) cmd_addlog,     NULL},
  {"away",      "",     (IntFunc) cmd_away,       NULL},
  {"back",      "",     (IntFunc) cmd_back,       NULL},
  {"backup",    "m|m",  (IntFunc) cmd_backup,     NULL},
  {"banner",    "t",    (IntFunc) cmd_banner,     NULL},
  {"binds",     "m",    (IntFunc) cmd_binds,      NULL},
  {"boot",      "t",    (IntFunc) cmd_boot,       NULL},
  {"botattr",   "t",    (IntFunc) cmd_botattr,    NULL},
  {"botinfo",   "",     (IntFunc) cmd_botinfo,    NULL},
  {"bots",      "",     (IntFunc) cmd_bots,       NULL},
  {"bottree",   "",     (IntFunc) cmd_bottree,    NULL},
  {"chaddr",    "t",    (IntFunc) cmd_chaddr,     NULL},
  {"chat",      "",     (IntFunc) cmd_chat,       NULL},
  {"chattr",    "m|m",  (IntFunc) cmd_chattr,     NULL},
  {"chhandle",  "t",    (IntFunc) cmd_chhandle,   NULL},
  {"chnick",    "t",    (IntFunc) cmd_chhandle,   NULL},
  {"chpass",    "t",    (IntFunc) cmd_chpass,     NULL},
  {"comment",   "m",    (IntFunc) cmd_comment,    NULL},
  {"console",   "to|o", (IntFunc) cmd_console,    NULL},
  {"dccstat",   "t",    (IntFunc) cmd_dccstat,    NULL},
  {"debug",     "m",    (IntFunc) cmd_debug,      NULL},
  {"die",       "n",    (IntFunc) cmd_die,        NULL},
  {"echo",      "",     (IntFunc) cmd_echo,       NULL},
  {"fixcodes",  "",     (IntFunc) cmd_fixcodes,   NULL},
  {"help",      "",     (IntFunc) cmd_help,       NULL},
  {"ignores",   "m",    (IntFunc) cmd_ignores,    NULL},
  {"link",      "t",    (IntFunc) cmd_link,       NULL},
  {"loadmod",   "n",    (IntFunc) cmd_loadmod,    NULL},
  {"match",     "to|o", (IntFunc) cmd_match,      NULL},
  {"me",        "",     (IntFunc) cmd_me,         NULL},
  {"module",    "m",    (IntFunc) cmd_module,     NULL},
  {"modules",   "n",    (IntFunc) cmd_modules,    NULL},
  {"motd",      "",     (IntFunc) cmd_motd,       NULL},
  {"newpass",   "",     (IntFunc) cmd_newpass,    NULL},
  {"handle",    "",     (IntFunc) cmd_handle,     NULL},
  {"nick",      "",     (IntFunc) cmd_handle,     NULL},
  {"page",      "",     (IntFunc) cmd_page,       NULL},
  {"quit",      "",     (IntFunc) CMD_LEAVE,      NULL},
  {"rehash",    "m",    (IntFunc) cmd_rehash,     NULL},
  {"rehelp",    "n",    (IntFunc) cmd_rehelp,     NULL},
  {"relay",     "o",    (IntFunc) cmd_relay,      NULL},
  {"reload",    "m|m",  (IntFunc) cmd_reload,     NULL},
  {"restart",   "m",    (IntFunc) cmd_restart,    NULL},
  {"save",      "m|m",  (IntFunc) cmd_save,       NULL},
  {"set",       "n",    (IntFunc) cmd_set,        NULL},
  {"simul",     "n",    (IntFunc) cmd_simul,      NULL},
  {"status",    "m|m",  (IntFunc) cmd_status,     NULL},
  {"strip",     "",     (IntFunc) cmd_strip,      NULL},
  {"su",        "",     (IntFunc) cmd_su,         NULL},
  {"tcl",       "n",    (IntFunc) cmd_tcl,        NULL},
  {"trace",     "t",    (IntFunc) cmd_trace,      NULL},
  {"unlink",    "t",    (IntFunc) cmd_unlink,     NULL},
  {"unloadmod", "n",    (IntFunc) cmd_unloadmod,  NULL},
  {"uptime",    "m|m",  (IntFunc) cmd_uptime,     NULL},
  {"vbottree",  "",     (IntFunc) cmd_vbottree,   NULL},
  {"who",       "",     (IntFunc) cmd_who,        NULL},
  {"whois",     "to|o", (IntFunc) cmd_whois,      NULL},
  {"whom",      "",     (IntFunc) cmd_whom,       NULL},
  {"traffic",   "m|m",  (IntFunc) cmd_traffic,    NULL},
  {"whoami",    "",     (IntFunc) cmd_whoami,     NULL},
  {NULL,        NULL,   NULL,                      NULL}
};
