/* 
 * cmds.c -- handles:
 * commands from a user via dcc
 * (split in 2, this portion contains no-irc commands)
 * * 
 * dprintf'ized, 3nov1995
 */
/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

#include "main.h"
#include "tandem.h"
#include "modules.h"
#include <ctype.h>

extern struct chanset_t *chanset;
extern struct dcc_t *dcc;
extern struct userrec *userlist;
extern tcl_timer_t *timer, *utimer;
extern int dcc_total, remote_boots, backgrd, make_userfile, do_restart;
extern int conmask, require_p;
extern int must_be_owner;
extern Tcl_Interp *interp;
extern char botnetnick[], origbotname[], ver[];
extern char network[], owner[], spaces[];
extern time_t now, online_since;

#define BADNICKCHARS "-,+*=:!.@#;$%&"

/* add hostmask to a bot's record if possible */
static int add_bot_hostmask(int idx, char *nick)
{
  struct chanset_t *chan;
  memberlist *m;
  char s[UHOSTLEN];

  for (chan = chanset; chan; chan = chan->next) {
    if (channel_active(chan)) {
      m = ismember(chan, nick);
      if (m) {
	struct userrec *u;

	simple_sprintf(s, "%s!%s", m->nick, m->userhost);
	u = get_user_by_host(s);
	if (u) {
	  dprintf(idx, "(Can't add userhost for %s because it matches %s)\n",
		  nick, u->handle);
	  return 0;
	}
	simple_sprintf(s, "*!%s", m->userhost);
	dprintf(idx, "(Added hostmask for %s from %s)\n", nick, chan->name);
	addhost_by_handle(nick, s);
	return 1;
      }
    }
  }
  return 0;
}

static void tell_who(struct userrec *u, int idx, int chan)
{
  int i, k, ok = 0, atr = u ? u->flags : 0, len;
  char s[1024];			/* temp fix - 1.4 has a better one */

  if (chan == 0)
    dprintf(idx, "Party line members:  (* = owner, + = master, @ = op)\n");
  else {
    simple_sprintf(s, "assoc %d", chan);
    if ((Tcl_Eval(interp, s) != TCL_OK) || !interp->result[0])
      dprintf(idx,
	    "People on channel %s%d:  (* = owner, + = master, @ = op)\n",
	      (chan < 100000) ? "" : "*", chan % 100000);
    else
      dprintf(idx,
      "People on channel '%s' (%s%d):  (* = owner, + = master, @ = op)\n",
	      interp->result, (chan < 100000) ? "" : "*", chan % 100000);
  }
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_CHAT)
      if (dcc[i].u.chat->channel == chan) {
	spaces[len = HANDLEN - strlen(dcc[i].nick)] = 0;
	if (atr & USER_OWNER) {
	  sprintf(s, "  [%.2lu]  %c%s%s %s",
		  dcc[i].sock, (geticon(i) == '-' ? ' ' : geticon(i)),
		  dcc[i].nick, spaces, dcc[i].host);
	} else {
	  sprintf(s, "  %c%s%s %s",
		  (geticon(i) == '-' ? ' ' : geticon(i)),
		  dcc[i].nick, spaces, dcc[i].host);
	}
	spaces[len] = ' ';
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
      strcpy(s, ctime(&dcc[i].timeval));
      strcpy(s, &s[1]);
      s[9] = 0;
      strcpy(s, &s[7]);
      s[2] = ' ';
      strcpy(&s[7], &s[10]);
      s[12] = 0;
      spaces[len = HANDLEN - strlen(dcc[i].nick)] = 0;
      if (atr & USER_OWNER) {
	dprintf(idx, "  [%.2lu]  %s%c%s%s (%s) %s\n",
		dcc[i].sock, dcc[i].status & STAT_CALLED ? "<-" : "->",
		dcc[i].status & STAT_SHARE ? '+' : ' ',
		dcc[i].nick, spaces, s, dcc[i].u.bot->version);
      } else {
	dprintf(idx, "  %s%c%s%s (%s) %s\n",
		dcc[i].status & STAT_CALLED ? "<-" : "->",
		dcc[i].status & STAT_SHARE ? '+' : ' ',
		dcc[i].nick, spaces, s, dcc[i].u.bot->version);
      }
      spaces[len] = ' ';
    }
  ok = 0;
  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type == &DCC_CHAT) && (dcc[i].u.chat->channel != chan)) {
      if (!ok) {
	ok = 1;
	dprintf(idx, "Other people on the bot:\n");
      }
      spaces[len = HANDLEN - strlen(dcc[i].nick)] = 0;
      if (atr & USER_OWNER) {
	sprintf(s, "  [%.2lu]  %c%s%s ",
		dcc[i].sock,
		(geticon(i) == '-' ? ' ' : geticon(i)), dcc[i].nick,
		spaces);
      } else {
	sprintf(s, "  %c%s%s ",
		(geticon(i) == '-' ? ' ' : geticon(i)), dcc[i].nick,
		spaces);
      }
      spaces[len] = ' ';
      if (atr & USER_MASTER) {
	if (dcc[i].u.chat->channel < 0)
	  strcat(s, "(-OFF-) ");
	else if (dcc[i].u.chat->channel == 0)
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
      spaces[len = HANDLEN - strlen(dcc[i].nick)] = 0;
      if (atr & USER_OWNER) {
	sprintf(s, "  [%.2lu]  %c%s%s (files) %s",
		dcc[i].sock, dcc[i].status & STAT_CHAT ? '+' : ' ',
		dcc[i].nick, spaces, dcc[i].host);
      } else {
	sprintf(s, "  %c%s%s (files) %s",
		dcc[i].status & STAT_CHAT ? '+' : ' ',
		dcc[i].nick, spaces, dcc[i].host);
      }
      spaces[len] = ' ';
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

  context;
  chan = chanset;
  now2 = now - online_since;
  s2[0] = 0;
  if (now2 > 86400) {
    int days = now2 / 86400;

    /* days */
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
    while (chan != NULL) {
      if (!channel_secret(chan)) {
	if ((strlen(s) + strlen(chan->name) + strlen(network) 
                   + strlen(botnetnick) + strlen(ver) + 1) >= 490) {
          strcat(s,"++  ");
          break; /* yeesh! */
	}
	strcat(s, chan->name);
	strcat(s, ", ");
      }
      chan = chan->next;
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
      Tcl_SetVar(interp, "chan", par, 0);
      if ((Tcl_VarEval(interp, "assoc ", "$chan", NULL) == TCL_OK) &&
	  interp->result[0]) {
	chan = atoi(interp->result);
      }
      if (chan <= 0) {
	dprintf(idx, "No such channel.\n");
	return;
      }
    } else
      chan = atoi(par);
    if ((chan < 0) || (chan > 99999)) {
      dprintf(idx, "Channel # out of range: must be 0-99999\n");
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
    if (!strcasecmp(par, botnetnick))
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
  dprintf(idx, "Changed password to '%s'\n", new);
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
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  get_user_flagrec(u, &fr, dcc[idx].u.chat->con_chan);
  rmspace(par);			/* maintain some sanity */
  if (par[0]) {
    putlog(LOG_CMDS, "*", "#%s# help %s", dcc[idx].nick, par);
    if (!strcmp(par, "all"))
      tellallhelp(idx, "all", &fr);
    else if (strchr(par, '*') || strchr(par, '?'))
      tellwildhelp(idx, par, &fr);
    else
      tellhelp(idx, par, &fr, 0);
  } else {
    putlog(LOG_CMDS, "*", "#%s# help", dcc[idx].nick);
    if (glob_op(fr) || glob_botmast(fr) || chan_op(fr))
      tellhelp(idx, "help", &fr, 0);
    else
      tellhelp(idx, "helpparty", &fr, 0);
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
    if (!strcasecmp(par, botnetnick))
      tell_who(u, idx, dcc[idx].u.chat->channel);
    else {
      i = nextbot(par);
      if (i < 0) {
	dprintf(idx, "That bot isn't connected.\n");
      } else if (dcc[idx].u.chat->channel > 99999)
	dprintf(idx, "You are on a local channel\n");
      else {
	char s[40];

	simple_sprintf(s, "%d:%s@%s", dcc[idx].sock,
		       dcc[idx].nick, botnetnick);
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

  if (!strcasecmp(par, "all")) {
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
    char whonick[512];

    splitc(whonick, who, '@');
    whonick[20] = 0;
    if (!strcasecmp(who, botnetnick)) {
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
      putlog(LOG_MISC, "*", "#%s# boot %s@%s (%s)", dcc[idx].nick, whonick,
	     who, par[0] ? par : dcc[idx].nick);
    } else
      dprintf(idx, "Remote boots are disabled here.\n");
    return;
  }
  for (i = 0; i < dcc_total; i++)
    if (!strcasecmp(dcc[i].nick, who) && !ok &&
	(dcc[i].type->flags & DCT_CANBOOT)) {
      u2 = get_user_by_handle(userlist, dcc[i].nick);
      if (u2 && (u2->flags & USER_OWNER) &&
	  strcasecmp(dcc[idx].nick, who)) {
	dprintf(idx, "Can't boot the bot owner.\n");
	return;
      }
      if (u2 && (u2->flags & USER_MASTER) &&
	  !(u && (u->flags & USER_MASTER))) {
	dprintf(idx, "Can't boot a bot master.\n");
	return;
      }
      files = (dcc[i].type->flags & DCT_FILES);
      if (files)
	dprintf(idx, "Booted %s from the file section.\n", dcc[i].nick);
      else
	dprintf(idx, "Booted %s from the bot.\n", dcc[i].nick);
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
  int dest = 0, i, ok = 0, pls, md;
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  module_entry *me;

  if (!par[0]) {
    dprintf(idx, "Your console is %s: %s (%s)\n",
	    dcc[idx].u.chat->con_chan,
	    masktype(dcc[idx].u.chat->con_flags),
	    maskname(dcc[idx].u.chat->con_flags));
    return;
  }
  get_user_flagrec(u, &fr, dcc[idx].u.chat->con_chan);
  strcpy(s1, par);
  nick = newsplit(&par);
  if (nick[0] && !strchr("#&+-*", nick[0]) && glob_master(fr)) {
    for (i = 0; i < dcc_total; i++)
      if (!strcasecmp(nick, dcc[i].nick) &&
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
  if (strchr("#&*", nick[0])) {
    if ((nick[0] != '*') && !findchan(nick)) {
      dprintf(idx, "Invalid console channel: %s\n", nick);
      return;
    }
    get_user_flagrec(u, &fr, nick);
    if (!chan_op(fr) && !(glob_op(fr) && !chan_deop(fr))) {
      dprintf(idx, "You don't have op or master access to channel %s\n",
	      nick);
      return;
    }
    strncpy(dcc[dest].u.chat->con_chan, nick, 80);
    dcc[dest].u.chat->con_chan[80] = 0;
    nick[0] = 0;
    if ((dest == idx) && !glob_master(fr) && !chan_master(fr))
      /* consoling to another channel for self */
      dcc[dest].u.chat->con_flags &= ~(LOG_MISC | LOG_CMDS | LOG_WALL);
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
	md = logmodes(s);
	if ((dest == idx) && !glob_master(fr) && pls) {
	  if (chan_master(fr))
	    md &= ~(LOG_FILES | LOG_LEV1 | LOG_LEV2 | LOG_LEV3 |
		    LOG_LEV4 | LOG_LEV5 | LOG_LEV6 | LOG_LEV7 |
		    LOG_LEV8 | LOG_DEBUG);
	  else
	    md &= ~(LOG_MISC | LOG_CMDS | LOG_FILES | LOG_LEV1 |
		    LOG_LEV2 | LOG_LEV3 | LOG_LEV4 | LOG_LEV5 |
		    LOG_LEV6 | LOG_LEV7 | LOG_LEV8 | LOG_WALL |
		    LOG_DEBUG);
	}
	if (!glob_owner(fr) && pls)
	  md &= ~(LOG_RAW | LOG_SRVOUT | LOG_BOTNET | LOG_BOTSHARE);
	if (!glob_botmast(fr) && pls)
	  md &= ~LOG_BOTS;
	if (pls)
	  dcc[dest].u.chat->con_flags |= md;
	else
	  dcc[dest].u.chat->con_flags &= ~md;
      }
    }
  }
  putlog(LOG_CMDS, "*", "#%s# console %s", dcc[idx].nick, s1);
  if (dest == idx) {
    dprintf(idx, "Set your console to %s: %s (%s)\n",
	    dcc[idx].u.chat->con_chan,
	    masktype(dcc[idx].u.chat->con_flags),
	    maskname(dcc[idx].u.chat->con_flags));
  } else {
    dprintf(idx, "Set console of %s to %s: %s (%s)\n", dcc[dest].nick,
	    dcc[dest].u.chat->con_chan,
	    masktype(dcc[dest].u.chat->con_flags),
	    maskname(dcc[dest].u.chat->con_flags));
    dprintf(dest, "%s set your console to %s: %s (%s)\n", dcc[idx].nick,
	    dcc[dest].u.chat->con_chan,
	    masktype(dcc[dest].u.chat->con_flags),
	    maskname(dcc[dest].u.chat->con_flags));
  }
  /* new style autosave -- drummer,07/25/1999*/
  if ((me = module_find("console", 0, 0))) {
    Function *func = me->funcs;
    (func[4]) (idx);
  }
}

static void cmd_pls_bot(struct userrec *u, int idx, char *par)
{
  char *handle, *addr, *p, *q, *host;
  struct userrec *u1;
  struct bot_addr *bi;

  if (!par[0])
    dprintf(idx, "Usage: +bot <handle> <address[:telnet-port[/relay-port]]> [host]\n");
  else {
    handle = newsplit(&par);
    addr = newsplit(&par);
    if (strlen(handle) > HANDLEN)
      handle[HANDLEN] = 0;	/* max len = XX .. for the moment :) */
    if (get_user_by_handle(userlist, handle))
      dprintf(idx, "Someone already exists by that name.\n");
    else if (strchr(BADNICKCHARS, handle[0]) != NULL)
      dprintf(idx, "You can't start a botnick with '%c'.\n", handle[0]);
    else {
      if (strlen(addr) > 60)
	addr[60] = 0;
      putlog(LOG_CMDS, "*", "#%s# +bot %s %s", dcc[idx].nick, handle, addr);
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
	} else {
	  bi->relay_port = atoi(q + 1);
	}
      }
      set_user(&USERENTRY_BOTADDR, u1, bi);
      dprintf(idx, "Added bot '%s' with address '%s' and no password.\n",
	      handle, addr);
      host = newsplit(&par);
      if (host[0]) {
	addhost_by_handle(handle, host);
      } else if (!add_bot_hostmask(idx, handle))
	dprintf(idx, "You'll want to add a hostmask if this bot will ever %s",
		"be on any channels that I'm on.\n");
    }
  }
}

static void cmd_chnick(struct userrec *u, int idx, char *par)
{
  char hand[HANDLEN + 1], newhand[HANDLEN + 1];
  int i, atr = u ? u->flags : 0, atr2;
  struct userrec *u2;

  strncpy(hand, newsplit(&par), sizeof(hand));
  strncpy(newhand, newsplit(&par), sizeof(newhand));

  if (!hand[0] || !newhand[0]) {
    dprintf(idx, "Usage: chnick <oldnick> <newnick>\n");
    return;
  }
  for (i = 0; i < strlen(newhand); i++)
    if ((newhand[i] <= 32) || (newhand[i] >= 127) || (newhand[i] == '@'))
      newhand[i] = '?';
  if (strchr(BADNICKCHARS, newhand[0]) != NULL)
    dprintf(idx, "Bizarre quantum forces prevent nicknames from starting with %c\n",
           newhand[0]);
  else if (get_user_by_handle(userlist, newhand) &&
          strcasecmp(hand, newhand))
    dprintf(idx, "Somebody is already using %s.\n", newhand);
  else if (!strcasecmp(newhand, botnetnick))
    dprintf(idx, "Hey! That's MY name!\n");
  else {
    u2 = get_user_by_handle(userlist, hand);
    atr2 = u2 ? u2->flags : 0;
    if ((atr & USER_BOTMAST) && !(atr & USER_MASTER) &&
       !(atr2 & USER_BOT))
      dprintf(idx, "You can't change nick for non-bots.\n");
    else if ((bot_flags(u2) & BOT_SHARE) && !(atr & USER_OWNER))
      dprintf(idx, "You can't change shared bot's nick.\n");
    else if ((atr2 & USER_OWNER) && !(atr & USER_OWNER) &&
            strcasecmp(dcc[idx].nick, hand))
      dprintf(idx, "Can't change the bot owner's handle.\n");
    else if (isowner(hand) && strcasecmp(dcc[idx].nick, hand))
      dprintf(idx, "Can't change the permanent bot owner's handle.\n");
    else if (change_handle(u2, newhand)) {
      putlog(LOG_CMDS, "*", "#%s# chnick %s %s", dcc[idx].nick,
            hand, newhand);
      dprintf(idx, "Changed.\n");
    } else
      dprintf(idx, "Failed.\n");
  }
}

static void cmd_nick(struct userrec *u, int idx, char *par)
{
  char oldnick[HANDLEN + 1], newnick[HANDLEN + 1];
  int i;

  strncpy(newnick, newsplit(&par), sizeof(newnick));

  if (!newnick[0]) {
    dprintf(idx, "Usage: nick <new-handle>\n");
    return;
  }
  for (i = 0; i < strlen(newnick); i++)
    if ((newnick[i] <= 32) || (newnick[i] >= 127) || (newnick[i] == '@'))
      newnick[i] = '?';
  if (strchr(BADNICKCHARS, newnick[0]) != NULL) {
    dprintf(idx, "Bizarre quantum forces prevent nicknames from starting with '%c'\n",
	    newnick[0]);
  } else if (get_user_by_handle(userlist, newnick) &&
	     strcasecmp(dcc[idx].nick, newnick)) {
    dprintf(idx, "Somebody is already using %s.\n", newnick);
  } else if (!strcasecmp(newnick, botnetnick)) {
    dprintf(idx, "Hey!  That's MY name!\n");
  } else {
    strncpy(oldnick, dcc[idx].nick, sizeof(oldnick));
    if (change_handle(u, newnick)) {
      putlog(LOG_CMDS, "*", "#%s# nick %s", oldnick, newnick);
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
      dprintf(idx, "You can't change password for non-bots.\n");
    else if ((bot_flags(u) & BOT_SHARE) && !(atr & USER_OWNER))
      dprintf(idx, "You can't change shared bot's password.\n");
    else if ((u->flags & USER_OWNER) && !(atr & USER_OWNER) &&
	     strcasecmp(handle, dcc[idx].nick))
      dprintf(idx, "Can't change the bot owner's password.\n");
    else if (isowner(handle) && strcasecmp(dcc[idx].nick, handle))
      dprintf(idx, "Can't change the permanent bot owner's handle.\n");
    else if (!par[0]) {
      putlog(LOG_CMDS, "*", "#%s# chpass %s [nothing]", dcc[idx].nick,
	     handle);
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
  char *handle, *addr, *p, *q;
  struct bot_addr *bi;
  struct userrec *u1;

  if (!par[0]) {
    dprintf(idx, "Usage: chaddr <botname> <address:botport#/userport#>\n");
    return;
  }
  handle = newsplit(&par);
  addr = newsplit(&par);
  if (strlen(addr) > UHOSTLEN)
    addr[UHOSTLEN] = 0;
  u1 = get_user_by_handle(userlist, handle);
  if (!u1 || !(u1->flags & USER_BOT)) {
    dprintf(idx, "Useful only for tandem bots.\n");
    return;
  }
  if ((bot_flags(u1) & BOT_SHARE) && (!u || !u->flags & USER_OWNER)) {
    dprintf(idx, "You can't change shared bot's address.\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# chaddr %s %s", dcc[idx].nick, handle, addr);
  dprintf(idx, "Changed bot's address.\n");
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

  if (!par[0]) {
    dprintf(idx, "Usage: comment <handle> <newcomment>\n");
    return;
  }
  handle = newsplit(&par);
  u1 = get_user_by_handle(userlist, handle);
  if (!u1) {
    dprintf(idx, "No such user!\n");
    return;
  }
  if ((u1->flags & USER_OWNER) && !(u && (u->flags & USER_OWNER)) &&
      strcasecmp(handle, dcc[idx].nick)) {
    dprintf(idx, "Can't change comment on the bot owner.\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# comment %s %s", dcc[idx].nick, handle, par);
  if (!strcasecmp(par, "none")) {
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
    dprintf(idx, "You can not .restart a bot when running -n (due to tcl)\n");
    return;
  }
  dprintf(idx, "Restarting.\n");
  if (make_userfile) {
    putlog(LOG_MISC, "*",
	   "Uh, guess you don't need to create a new userfile.");
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
    putlog(LOG_MISC, "*",
	   "Uh, guess you don't need to create a new userfile.");
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

/* this get replaced in server.so with a version that handles the server */
void cmd_die(struct userrec *u, int idx, char *par)
{
  char s[1024];

  putlog(LOG_CMDS, "*", "#%s# die %s", dcc[idx].nick, par);
  if (par[0]) {
    simple_sprintf(s, "BOT SHUTDOWN (%s: %s)", dcc[idx].nick, par);
  } else {
    simple_sprintf(s, "BOT SHUTDOWN (authorized by %s)", dcc[idx].nick);
  }
  chatout("*** %s\n", s);
  botnet_send_chat(-1, botnetnick, s);
  botnet_send_bye();
  write_userfile(-1);
  simple_sprintf(s, "DIE BY %s!%s (%s)", dcc[idx].nick,
		 dcc[idx].host, par[0] ? par : "request");
  fatal(s, 0);
}

static void cmd_debug(struct userrec *u, int idx, char *par)
{
  if (!strcasecmp(par, "help")) {
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
    dprintf(idx, "Usage: simul <nick> <text>\n");
    return;
  }
  if (isowner(nick)) {
    dprintf(idx, "Unable to '.simul' permanent owners.\n");
    return;
  }
  for (i = 0; i < dcc_total; i++)
    if (!strcasecmp(nick, dcc[i].nick) && !ok &&
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
  if (!par[0] || !strcasecmp(par, botnetnick))
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
    botunlink(idx, bot, par);
    return;
  }
  /* if we're directly connected to that bot, just do it 
   * (is nike gunna sue?) */
  if (!strcasecmp(dcc[i].nick, bot))
    botunlink(idx, bot, par);
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
  dprintf(idx, "Backing up the user file...\n");
  backup_userfile();
}

static void cmd_trace(struct userrec *u, int idx, char *par)
{
  int i;
  char x[NOTENAMELEN + 11], y[11];

  if (!par[0]) {
    dprintf(idx, "Usage: trace <botname>\n");
    return;
  }
  if (!strcasecmp(par, botnetnick)) {
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
  simple_sprintf(s, "\007\007### Botwide:[%s] %s\n", dcc[idx].nick, par);
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type->flags & DCT_MASTER)
      dprintf(i, "%s", s);
}

/* after messing with someone's user flags, make sure the dcc-chat flags
 * are set correctly */
int check_dcc_attrs(struct userrec *u, int oatr)
{
  int i, stat;
  char *p, *q, s[121];

  /* if it matches someone in the owner list, make sure he/she has +n */
  if (!u)
    return 0;
  /* make sure default owners are +n */
  if (owner[0]) {
    q = owner;
    p = strchr(q, ',');
    while (p) {
      strncpy(s, q, p - q);
      s[p - q] = 0;
      rmspace(s);
      if (!strcasecmp(u->handle, s))
	u->flags = sanity_check(u->flags | USER_OWNER);
      q = p + 1;
      p = strchr(q, ',');
    }
    strcpy(s, q);
    rmspace(s);
    if (!strcasecmp(u->handle, s))
      u->flags = sanity_check(u->flags | USER_OWNER);
  }
  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type->flags & DCT_MASTER) &&
	(!strcasecmp(u->handle, dcc[i].nick))) {
      stat = dcc[i].status;
      if ((dcc[i].type == &DCC_CHAT) &&
	  ((u->flags & (USER_OP | USER_MASTER | USER_OWNER |
			USER_BOTMAST))
	   != (oatr & (USER_OP | USER_MASTER | USER_OWNER |
		       USER_BOTMAST)))) {
	botnet_send_join_idx(i, -1);
      }
      if ((oatr & USER_MASTER) && !(u->flags & USER_MASTER)) {
	struct flag_record fr =
	{FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

	dcc[i].u.chat->con_flags &= ~(LOG_MISC | LOG_CMDS | LOG_RAW |
				      LOG_FILES | LOG_LEV1 | LOG_LEV2 |
				      LOG_LEV3 | LOG_LEV4 | LOG_LEV5 |
				      LOG_LEV6 | LOG_LEV7 | LOG_LEV8 |
				      LOG_WALL | LOG_DEBUG);
	get_user_flagrec(u, &fr, NULL);
	if (!chan_master(fr))
	  dcc[i].u.chat->con_flags |= (LOG_MISC | LOG_CMDS);
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
      if ((stat & STAT_PARTY) && (u->flags & USER_OP))
	stat &= ~STAT_PARTY;
      if (!(stat & STAT_PARTY) && !(u->flags & USER_OP) &&
	  !(u->flags & USER_MASTER))
	stat |= STAT_PARTY;
      if ((stat & STAT_CHAT) && !(u->flags & USER_PARTY) &&
	  !(u->flags & USER_MASTER) &&
	  (!(u->flags & USER_OP) || require_p))
	stat &= ~STAT_CHAT;
      if ((dcc[i].type->flags & DCT_FILES) && !(stat & STAT_CHAT) &&
	  ((u->flags & USER_MASTER) || (u->flags & USER_PARTY) ||
	   ((u->flags & USER_OP) && !require_p)))
	stat |= STAT_CHAT;
      dcc[i].status = stat;
      /* check if they no longer have access to wherever they are */
      /* DON'T kick someone off the party line just cuz they lost +p
       * (pinvite script removes +p after 5 mins automatically) */
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
	    context;
	    if (dcc[i].u.chat->channel < 100000)
	      botnet_send_join_idx(i, -1);
	  }
	} else {
	  killsock(dcc[i].sock);
	  dcc[i].sock = (long) dcc[i].type;
	  dcc[i].type = &DCC_LOST;
	}
      }
    }
    if ((dcc[i].type == &DCC_BOT) && !strcasecmp(u->handle, dcc[i].nick)) {
      if ((dcc[i].status & STAT_LEAF) && !(u->flags & BOT_LEAF))
	dcc[i].status &= ~(STAT_LEAF | STAT_WARNED);
      if (!(dcc[i].status & STAT_LEAF) && (u->flags & BOT_LEAF))
	dcc[i].status |= STAT_LEAF;
    }
  }
  return u->flags;
}

int check_dcc_chanattrs(struct userrec *u, char *chname, int chflags,
			int ochatr)
{
  int i, found = 0, atr = u ? u->flags : 0;
  struct chanset_t *chan;

  if (!u)
    return 0;
  chan = chanset;
  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type->flags & DCT_MASTER) &&
	!strcasecmp(u->handle, dcc[i].nick)) {
      if ((dcc[i].type == &DCC_CHAT) &&
	  ((chflags & (USER_OP | USER_MASTER | USER_OWNER))
	   != (ochatr & (USER_OP | USER_MASTER | USER_OWNER))))
	botnet_send_join_idx(i, -1);
      if ((ochatr & USER_MASTER) && !(chflags & USER_MASTER)) {
	if (!(atr & USER_MASTER))
	  dcc[i].u.chat->con_flags &= ~(LOG_MISC | LOG_CMDS);
	dprintf(i, "*** POOF! ***\n");
	dprintf(i, "You are no longer a master on %s.\n", chname);
      }
      if (!(ochatr & USER_MASTER) && (chflags & USER_MASTER)) {
	dcc[i].u.chat->con_flags |= conmask;
	if (!(atr & USER_MASTER))
	  dcc[i].u.chat->con_flags &=
	    ~(LOG_LEV1 | LOG_LEV2 | LOG_LEV3 | LOG_LEV4 |
	      LOG_LEV5 | LOG_LEV6 | LOG_LEV7 | LOG_LEV8 |
	      LOG_RAW | LOG_DEBUG | LOG_WALL | LOG_FILES | LOG_SRVOUT);
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
	struct flag_record fr =
	{FR_CHAN, 0, 0, 0, 0, 0};

	while (chan && !found) {
	  get_user_flagrec(u, &fr, chan->name);
	  if (fr.chan & (USER_OP | USER_MASTER | USER_OWNER))
	    found = 1;
	  else
	    chan = chan->next;
	}
	if (!chan)
	  chan = chanset;
	if (chan)
	  strcpy(dcc[i].u.chat->con_chan, chan->name);
	else
	  strcpy(dcc[i].u.chat->con_chan, "*");
      }
    }
  }
  return chflags;
}

static void cmd_chattr(struct userrec *u, int idx, char *par)
{
  char *hand, *tmpchg = NULL, *chg = NULL, work[1024];
  struct chanset_t *chan = NULL;
  struct userrec *u2;
  struct flag_record pls =
  {0, 0, 0, 0, 0, 0}, mns =
  {0, 0, 0, 0, 0, 0}, user =
  {0, 0, 0, 0, 0, 0};
  module_entry *me;
  int fl = -1, of = 0, ocf = 0;

  if (!par[0]) {
    dprintf(idx, "Usage: chattr <handle> [changes] [channel]\n");
    return;
  }
  hand = newsplit(&par);
  u2 = get_user_by_handle(userlist, hand);
  if ((hand[0] == '*') || !u2) {
    dprintf(idx, "No such user!\n");
    return;
  }
  if (par[0] && ((strchr(CHANMETA, par[0]) == NULL) || par[0]=='+')) {
    chg = newsplit(&par);
    if (!par[0] && strpbrk(chg, "&|"))
      par = dcc[idx].u.chat->con_chan;
     else if (par[0] && !strpbrk(chg, "&|")) {
       context;
       tmpchg = nmalloc(sizeof(chg)+1);
       sprintf(tmpchg,"|%s",chg);
       chg = nmalloc(sizeof(tmpchg));
       strcpy(chg,tmpchg);
     }
  }
  chan = findchan(par);
  if (!chan && par[0] && (par[0] != '*')) {
    dprintf(idx, "No channel record for %s.\n", par);
    if (tmpchg) {
      nfree(chg); nfree(tmpchg);
    }
    return;
  }
  user.match = FR_GLOBAL;
  if (chan)
    user.match |= FR_CHAN;
  get_user_flagrec(u, &user, chan ? chan->name : 0);
  if (!chan && !glob_botmast(user)) {
    dprintf(idx, "You do not have Bot Master privileges.\n");
    if (tmpchg) {
      nfree(chg); nfree(tmpchg);
    }
    return;
  }
  if (chan && !glob_master(user) && !chan_master(user)) {
    dprintf(idx, "You do not have channel master privileges for channel %s\n",
	    par);
    if (tmpchg) {
      nfree(chg); nfree(tmpchg);
    }
    return;
  }
  user.match &= fl;
  if (chg) {
    pls.match = user.match;
    break_down_flags(chg, &pls, &mns);
    /* no-one can change these flags on-the-fly */
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
	   dcc[idx].nick, chan ? chan->name : "*", hand, chg ? chg : "");
  else
    putlog(LOG_CMDS, "*", "#%s# chattr %s %s", dcc[idx].nick, hand,
	   chg ? chg : "");
  /* get current flags and display them */
  if (user.match & FR_GLOBAL) {
    user.match = FR_GLOBAL;
    if (chg)
      check_dcc_attrs(u2, of);
    get_user_flagrec(u2, &user, NULL);
    build_flags(work, &user, NULL);
    if (work[0] != '-')
      dprintf(idx, "Global flags for %s are now +%s\n", hand, work);
    else
      dprintf(idx, "No global flags for %s.\n", hand);
  }
  if (chan) {
    user.match = FR_CHAN;
    get_user_flagrec(u2, &user, par);
    user.chan &= ~BOT_SHARE;
    if (chg)
      check_dcc_chanattrs(u2, chan->name, user.chan, ocf);
    build_flags(work, &user, NULL);
    if (work[0] != '-')
      dprintf(idx, "Channel flags for %s on %s are now +%s\n", hand,
	      chan->name, work);
    else
      dprintf(idx, "No flags for %s on %s.\n", hand, chan->name);
    if ((me = module_find("irc", 0, 0))) {
      Function *func = me->funcs;

      if (chan)
	(func[15]) (chan, 0);
    }
  }
  if (tmpchg) {
      nfree(chg); nfree(tmpchg);
    }
}

static void cmd_botattr(struct userrec *u, int idx, char *par)
{
  char *hand, *chg = NULL, work[1024];
  struct chanset_t *chan = NULL;
  struct userrec *u2;
  struct flag_record pls =
  {0, 0, 0, 0, 0, 0}, mns =
  {0, 0, 0, 0, 0, 0}, user =
  {0, 0, 0, 0, 0, 0};
  int idx2;

  if (!par[0]) {
    dprintf(idx, "Usage: botattr <handle> [changes] [channel]\n");
    return;
  }
  hand = newsplit(&par);
  u2 = get_user_by_handle(userlist, hand);
  if ((hand[0] == '*') || !u2 || !(u2->flags & USER_BOT)) {
    dprintf(idx, "No such bot!\n");
    return;
  }
  for (idx2 = 0; idx2 < dcc_total; idx2++)
    if (!strcasecmp(dcc[idx2].nick, hand))
      break;
  if (idx2 != dcc_total) {
    dprintf(idx, "You may not change the attributes of a linked bot.\n");
    return;
  }
  if (par[0] && ((strchr(CHANMETA, par[0]) == NULL) || par[0]=='+')) {
    chg = newsplit(&par);
    if (!par[0] && strpbrk(chg, "|&"))
      par = dcc[idx].u.chat->con_chan;
  }
  if (!(chan = findchan(par)) && par[0]) {
    dprintf(idx, "No channel record for %s.\n", par);
    return;
  }
  user.match = FR_GLOBAL;
  get_user_flagrec(u, &user, chan ? chan->name : 0);
  if (!glob_botmast(user)) {
    dprintf(idx, "You do not have Bot Master privileges.\n");
    return;
  }
  if (chg) {
    user.match = FR_BOT | (chan ? FR_CHAN : 0);
    pls.match = user.match;
    break_down_flags(chg, &pls, &mns);
    /* no-one can change these flags on-the-fly */
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
	   dcc[idx].nick, chan->name, hand, chg ? chg : "");
  else
    putlog(LOG_CMDS, "*", "#%s# botattr %s %s", dcc[idx].nick, hand,
	   chg ? chg : "");
  /* get current flags and display them */
  if (!chan || pls.bot || mns.bot) {
    user.match = FR_BOT;
    get_user_flagrec(u2, &user, NULL);
    build_flags(work, &user, NULL);
    if (work[0] != '-')
      dprintf(idx, "Bot flags for %s are now +%s\n", hand, work);
    else
      dprintf(idx, "No bot flags for %s.\n", hand);
  }
  if (chan) {
    user.match = FR_CHAN;
    get_user_flagrec(u2, &user, par);
    user.chan &= BOT_SHARE;
    build_flags(work, &user, NULL);
    if (work[0] != '-')
      dprintf(idx, "Bot flags for %s on %s are now +%s\n", hand,
	      chan->name, work);
    else
      dprintf(idx, "No bot flags for %s on %s.\n", hand, chan->name);
  }
}

static void cmd_chat(struct userrec *u, int idx, char *par)
{
  int newchan, oldchan;

  if (!strcasecmp(par, "off")) {
    /* turn chat off */
    if (dcc[idx].u.chat->channel < 0)
      dprintf(idx, "You weren't in chat anyway!\n");
    else {
      dprintf(idx, "Leaving chat mode...\n");
      check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock,
		     dcc[idx].u.chat->channel);
      chanout_but(-1, dcc[idx].u.chat->channel,
		  "*** %s left the party line.\n",
		  dcc[idx].nick);
      context;
      if (dcc[idx].u.chat->channel < 100000)
	botnet_send_part_idx(idx, "");
    }
    dcc[idx].u.chat->channel = (-1);
  } else {
    if (par[0] == '*') {
      if (((par[1] < '0') || (par[1] > '9'))) {
	if (par[1] == 0)
	  newchan = 0;
	else {
	  Tcl_SetVar(interp, "chan", par, 0);
	  if ((Tcl_VarEval(interp, "assoc ", "$chan", NULL) == TCL_OK) &&
	      interp->result[0])
	    newchan = atoi(interp->result);
	  else
	    newchan = -1;
	}
	if (newchan < 0) {
	  dprintf(idx, "No channel by that name.\n");
	  return;
	}
      } else
	newchan = 100000 + atoi(par + 1);
      if (newchan < 100000 || newchan > 199999) {
	dprintf(idx, "Channel # out of range: local channels must be *0-*99999\n");
	return;
      }
    } else {
      if (((par[0] < '0') || (par[0] > '9')) && (par[0])) {
	if (!strcasecmp(par, "on"))
	  newchan = 0;
	else {
	  Tcl_SetVar(interp, "chan", par, 0);
	  if ((Tcl_VarEval(interp, "assoc ", "$chan", NULL) == TCL_OK) &&
	      interp->result[0])
	    newchan = atoi(interp->result);
	  else
	    newchan = -1;
	}
	if (newchan < 0) {
	  dprintf(idx, "No channel by that name.\n");
	  return;
	}
      } else
	newchan = atoi(par);
      if ((newchan < 0) || (newchan > 199999)) {
	dprintf(idx, "Channel # out of range: must be between 0 and 99999.\n");
	return;
      }
    }
    /* if coming back from being off the party line, make sure they're
     * not away */
    if ((dcc[idx].u.chat->channel < 0) && (dcc[idx].u.chat->away != NULL))
      not_away(idx);
    if (dcc[idx].u.chat->channel == newchan) {
      if (newchan == 0)
	dprintf(idx, "You're already on the party line!\n");
      else
	dprintf(idx, "You're already on channel %s%d!\n",
		(newchan < 100000) ? "" : "*", newchan % 100000);
    } else {
      oldchan = dcc[idx].u.chat->channel;
      if (oldchan >= 0)
	check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock, oldchan);
      if (oldchan == 0) {
	chanout_but(-1, 0, "*** %s left the party line.\n", dcc[idx].nick);
	context;
      } else if (oldchan > 0) {
	chanout_but(-1, oldchan, "*** %s left the channel.\n", dcc[idx].nick);
	context;
      }
      dcc[idx].u.chat->channel = newchan;
      if (newchan == 0) {
	dprintf(idx, "Entering the party line...\n");
	chanout_but(-1, 0, "*** %s joined the party line.\n", dcc[idx].nick);
	context;
      } else {
	dprintf(idx, "Joining channel '%s'...\n", par);
	chanout_but(-1, newchan, "*** %s joined the channel.\n", dcc[idx].nick);
	context;
      }
      check_tcl_chjn(botnetnick, dcc[idx].nick, newchan, geticon(idx),
		     dcc[idx].sock, dcc[idx].host);
      if (newchan < 100000)
	botnet_send_join_idx(idx, oldchan);
      else if (oldchan < 100000)
	botnet_send_part_idx(idx, "");
    }
  }
}

static void cmd_echo(struct userrec *u, int idx, char *par)
{
  if (!par[0]) {
    dprintf(idx, "Echo is currently %s.\n", dcc[idx].status & STAT_ECHO ?
	    "on" : "off");
    return;
  }
  if (!strcasecmp(par, "on")) {
    dprintf(idx, "Echo turned on.\n");
    dcc[idx].status |= STAT_ECHO;
    return;
  }
  if (!strcasecmp(par, "off")) {
    dprintf(idx, "Echo turned off.\n");
    dcc[idx].status &= ~STAT_ECHO;
    return;
  }
  dprintf(idx, "Usage: echo <on/off>\n");
}

int stripmodes(char *s)
{
  int res = 0;

  for (; *s; s++)
    switch (tolower(*s)) {
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
  char *nick, *changes, s[2];
  int dest = 0, i, pls, md, ok = 0;

  if (!par[0]) {
    dprintf(idx, "Your current strip settings are: %s (%s)\n",
	    stripmasktype(dcc[idx].u.chat->strip_flags),
	    stripmaskname(dcc[idx].u.chat->strip_flags));
    return;
  }
  nick = newsplit(&par);
  if ((nick[0] != '+') && (nick[0] != '-') && u &&
      (u->flags & USER_MASTER)) {
    for (i = 0; i < dcc_total; i++)
      if (!strcasecmp(nick, dcc[i].nick) && (dcc[i].type == &DCC_CHAT) &&
	  !ok) {
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
  if ((changes[0] != '+') && (changes[0] != '-'))
    dcc[dest].u.chat->strip_flags = 0;
  s[1] = 0;
  for (pls = 1; *changes; changes++) {
    switch (*changes) {
    case '+':
      pls = 1;
      break;
    case '-':
      pls = 0;
      break;
    default:
      s[0] = *changes;
      md = stripmodes(s);
      if (pls == 1)
	dcc[dest].u.chat->strip_flags |= md;
      else
	dcc[dest].u.chat->strip_flags &= ~md;
    }
  }
  putlog(LOG_CMDS, "*", "#%s# strip %s %s", dcc[idx].nick, nick, changes);
  if (dest == idx) {
    dprintf(idx, "Your strip settings are: %s (%s)\n",
	    stripmasktype(dcc[idx].u.chat->strip_flags),
	    stripmaskname(dcc[idx].u.chat->strip_flags));
  } else {
    dprintf(idx, "Strip setting for %s: %s (%s)\n", dcc[dest].nick,
	    stripmasktype(dcc[dest].u.chat->strip_flags),
	    stripmaskname(dcc[dest].u.chat->strip_flags));
    dprintf(dest, "%s set your strip settings to: %s (%s)\n", dcc[idx].nick,
	    stripmasktype(dcc[dest].u.chat->strip_flags),
	    stripmaskname(dcc[dest].u.chat->strip_flags));
  }
}

static void cmd_su(struct userrec *u, int idx, char *par)
{
  int atr = u ? u->flags : 0;
  struct flag_record fr =
  {FR_ANYWH | FR_CHAN | FR_GLOBAL, 0, 0, 0, 0, 0};

  context;
  u = get_user_by_handle(userlist, par);

  if (!par[0])
    dprintf(idx, "Usage: su <user>\n");
  else if (!u)
    dprintf(idx, "No such user.\n");
  else if (u->flags & USER_BOT)
    dprintf(idx, "Can't su to a bot... then again, why would you wanna?\n");
  else if (u_pass_match(u, "-"))
    dprintf(idx, "No password set for user. You may not .su to them\n");
  else if (dcc[idx].u.chat->su_nick)
    dprintf(idx, "You cannot currently double .su, try .su'ing directly\n");
  else {
    get_user_flagrec(u, &fr, NULL);
    if ((!glob_party(fr) && (require_p || !(glob_op(fr) || chan_op(fr)))) &&
	!(atr & USER_BOTMAST))
      dprintf(idx, "No party line access permitted for %s.\n", par);
    else {
      correct_handle(par);
      putlog(LOG_CMDS, "*", "#%s# su %s", dcc[idx].nick, par);
      if (!(atr & USER_OWNER) ||
	  ((atr & USER_OWNER) && !(isowner(dcc[idx].nick))) ||
	  ((u->flags & USER_OWNER) && (isowner(par)))) {
	if (dcc[idx].u.chat->channel < 100000)
	  botnet_send_part_idx(idx, "");
	chanout_but(-1, dcc[idx].u.chat->channel,
		    "*** %s left the party line.\n", dcc[idx].nick);
	context;
	/* store the old nick in the away section, for weenies who can't get
	 * their password right ;) */
	if (dcc[idx].u.chat->away != NULL)
	  nfree(dcc[idx].u.chat->away);
	dcc[idx].u.chat->away = n_malloc(strlen(dcc[idx].nick) + 1,
					 "dccutil.c", 9999);	/* evil hack :) */
	strcpy(dcc[idx].u.chat->away, dcc[idx].nick);
	dcc[idx].u.chat->su_nick = n_malloc(strlen(dcc[idx].nick) + 1,
					    "dccutil.c", 9999);
	strcpy(dcc[idx].u.chat->su_nick, dcc[idx].nick);
	dcc[idx].user = u;
	strcpy(dcc[idx].nick, par);
	dprintf(idx, "Enter password for %s%s\n", par,
		(dcc[idx].status & STAT_TELNET) ? "\377\373\001" : "");
	dcc[idx].type = &DCC_CHAT_PASS;
      } else if (atr & USER_OWNER) {
	if (dcc[idx].u.chat->channel < 100000)
	  botnet_send_part_idx(idx, "");
	chanout_but(-1, dcc[idx].u.chat->channel,
		    "*** %s left the party line.\n", dcc[idx].nick);
	dprintf(idx, "Setting your username to %s.\n", par);
	if (atr & USER_MASTER)
	  dcc[idx].u.chat->con_flags = conmask;
	dcc[idx].u.chat->su_nick = n_malloc(strlen(dcc[idx].nick) + 1, "dccutil.c", 9999);
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
  if (dcc[idx].status & STAT_ECHO) {
    dcc[idx].status |= STAT_TELNET;
    dcc[idx].status &= ~STAT_ECHO;
    dprintf(idx, "Turned on telnet codes\n");
    putlog(LOG_CMDS, "*", "#%s# fixcodes (telnet on)", dcc[idx].nick);
    return;
  }
  if (dcc[idx].status & STAT_TELNET) {
    dcc[idx].status |= STAT_ECHO;
    dcc[idx].status &= ~STAT_TELNET;
    dprintf(idx, "Turned off telnet codes\n");
    putlog(LOG_CMDS, "*", "#%s# fixcodes (telnet off)", dcc[idx].nick);
    return;
  }
}

static void cmd_page(struct userrec *u, int idx, char *par)
{
  int a;

  if (!par[0]) {
    if (dcc[idx].status & STAT_PAGE) {
      dprintf(idx, "Currently paging outputs to %d lines.\n",
	      dcc[idx].u.chat->max_line);
    } else
      dprintf(idx, "You dont have paging on.\n");
    return;
  }
  a = atoi(par);
  if (!strcasecmp(par, "off") || (a == 0 && par[0] == 0)) {
    dcc[idx].status &= ~STAT_PAGE;
    dcc[idx].u.chat->max_line = 0x7ffffff;	/* flush_lines needs this */
    while (dcc[idx].u.chat->buffer)
      flush_lines(idx, dcc[idx].u.chat);
    dprintf(idx, "Paging turned off.\n");
    putlog(LOG_CMDS, "*", "#%s# page off", dcc[idx].nick);
    return;
  }
  if (a > 0) {
    dprintf(idx, "Paging turned on, stopping every %d lines.\n", a);
    dcc[idx].status |= STAT_PAGE;
    dcc[idx].u.chat->max_line = a;
    dcc[idx].u.chat->line_count = 0;
    dcc[idx].u.chat->current_lines = 0;
    putlog(LOG_CMDS, "*", "#%s# page %d", dcc[idx].nick, a);
    return;
  }
  dprintf(idx, "Usage: page <off or #>\n");
}

/* evaluate a Tcl command, send output to a dcc user */
static void cmd_tcl(struct userrec *u, int idx, char *msg)
{
  int code;

  if (!(isowner(dcc[idx].nick)) && (must_be_owner)) {
    dprintf(idx, MISC_NOSUCHCMD);
    return;
  }
  debug1("tcl: evaluate (.tcl): %s", msg);
  set_tcl_vars();
  code = Tcl_GlobalEval(interp, msg);
  if (code == TCL_OK)
    dumplots(idx, "Tcl: ", interp->result);
  else
    dumplots(idx, "TCL error: ", interp->result);
}

/* perform a 'set' command */
static void cmd_set(struct userrec *u, int idx, char *msg)
{
  int code;
  char s[512];

  if (!(isowner(dcc[idx].nick)) && (must_be_owner)) {
    dprintf(idx, MISC_NOSUCHCMD);
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# set %s", dcc[idx].nick, msg);
  set_tcl_vars();
  strcpy(s, "set ");
  if (!msg[0]) {
    strcpy(s, "info globals");
    Tcl_Eval(interp, s);
    dumplots(idx, "global vars: ", interp->result);
    return;
  }
  strcpy(s + 4, msg);
  code = Tcl_Eval(interp, s);
  if (code == TCL_OK) {
    if (!strchr(msg, ' '))
      dumplots(idx, "currently: ", interp->result);
    else
      dprintf(idx, "Ok, set.\n");
  } else
    dprintf(idx, "Error: %s\n", interp->result);
}

static void cmd_module(struct userrec *u, int idx, char *par)
{
  context;
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
  context;
  if (!par[0]) {
    dprintf(idx, "%s: loadmod <module>\n", USAGE);
  } else {
    p = module_load(par);
    if (p)
      dprintf(idx, "%s: %s %s\n", par, MOD_LOADERROR, p);
    else {
      putlog(LOG_CMDS, "*", "#%s# loadmod %s", dcc[idx].nick, par);
      dprintf(idx, "%s %s\n", MOD_LOADED, par);
    }
  }
  context;
}

static void cmd_unloadmod(struct userrec *u, int idx, char *par)
{
  char *p;

     if (!(isowner(dcc[idx].nick)) && (must_be_owner)) {
         dprintf(idx, MISC_NOSUCHCMD);
         return;
     }
  context;
  if (!par[0]) {
    dprintf(idx, "%s: unloadmod <module>\n", USAGE);
  } else {
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
  char *who;
  char s[UHOSTLEN + 1];

  if (!par[0]) {
    dprintf(idx, "Usage: +ignore <hostmask> [comment]\n");
    return;
  }
  who = newsplit(&par);
  remove_gunk(who);
  if (!par[0])
    par = "requested";
  else if (strlen(par) > 65)
    par[65] = 0;
  if (strlen(who) > UHOSTLEN - 4)
    who[UHOSTLEN - 4] = 0;
  /* fix missing ! or @ BEFORE continuing - sounds familiar */
  if (!strchr(who, '!')) {
    if (!strchr(who, '@'))
      simple_sprintf(s, "%s!*@*", who);
    else
      simple_sprintf(s, "*!%s", who);
  } else if (!strchr(who, '@'))
    simple_sprintf(s, "%s@*", who);
  else
    strcpy(s, who);
  if (match_ignore(s)) {
    dprintf(idx, "That already matches an existing ignore.\n");
    return;
  }
  dprintf(idx, "Now ignoring: %s (%s)\n", s, par);
  addignore(s, dcc[idx].nick, par, 0L);
  putlog(LOG_CMDS, "*", "#%s# +ignore %s %s", dcc[idx].nick, s, par);
}

static void cmd_mns_ignore(struct userrec *u, int idx, char *par)
{
  char buf[UHOSTLEN + 1];

  if (!par[0]) {
    dprintf(idx, "Usage: -ignore <hostmask | ignore #>\n");
    return;
  }
  strncpy(buf, par, UHOSTLEN);
  buf[UHOSTLEN] = 0;
  if (delignore(buf)) {
    putlog(LOG_CMDS, "*", "#%s# -ignore %s", dcc[idx].nick, buf);
    dprintf(idx, "No longer ignoring: %s\n", buf);
  } else
    dprintf(idx, "Can't find that ignore.\n");
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
    dprintf(idx, "Usage: +user <handle> <hostmask>\n");
    return;
  }
  handle = newsplit(&par);
  host = newsplit(&par);
  if (strlen(handle) > HANDLEN)
    handle[HANDLEN] = 0;	/* max len = XX */
  if (get_user_by_handle(userlist, handle)) {
    dprintf(idx, "Someone already exists by that name.\n");
  } else if (strchr(BADNICKCHARS, handle[0]) != NULL) {
    dprintf(idx, "You can't start a nick with '%c'.\n", handle[0]);
  } else {
    putlog(LOG_CMDS, "*", "#%s# +user %s %s", dcc[idx].nick, handle, host);
    userlist = adduser(userlist, handle, host, "-", 0);
    dprintf(idx, "Added %s (%s) with no password or flags.\n", handle, host);
  }
}

static void cmd_mns_user(struct userrec *u, int idx, char *par)
{
  char *handle;
  struct userrec *u2;

  if (!par[0]) {
    dprintf(idx, "Usage: -user <nick>\n");
    return;
  }
  handle = newsplit(&par);
  u2 = get_user_by_handle(userlist, handle);
  if (!u2 || !u) {
    dprintf(idx, "No such user!\n");
    return;
  }
  if (isowner(u2->handle)) {
    dprintf(idx, "Can't remove the permanent bot owner!\n");
    return;
  }
  if ((u2->flags & USER_OWNER) && !(u->flags & USER_OWNER)) {
    dprintf(idx, "Can't remove the bot owner!\n");
    return;
  }
  if ((u2->flags & USER_BOT) && (bot_flags(u2) & BOT_SHARE) &&
      !(u->flags & USER_OWNER)) {
    dprintf(idx, "You can't remove shared bots.\n");
    return;
  }
  if ((u->flags & USER_BOTMAST) && !(u->flags & USER_MASTER) &&
      !(u2->flags & USER_BOT)) {
    dprintf(idx, "Can't remove users who aren't bots!\n");
    return;
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
  struct flag_record fr =
  {FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

  context;
  handle = newsplit(&par);
  if (!par[0]) {
    dprintf(idx, "Usage: +host <handle> <newhostmask>\n");
    return;
  }
  host = newsplit(&par);
  u2 = get_user_by_handle(userlist, handle);
  if (!u2) {
    dprintf(idx, "No such user.\n");
    return;
  }
  for (q = get_user(&USERENTRY_HOSTS, u); q; q = q->next)
    if (!strcasecmp(q->extra, host)) {
      dprintf(idx, "That hostmask is already there.\n");
      return;
    }
  get_user_flagrec(u, &fr, NULL);
  if ((u->flags & USER_BOTMAST) && !(u->flags & USER_MASTER) &&
      !(u2->flags & USER_BOT) && !chan_master(fr)) {
    dprintf(idx, "You can't add hostmasks to non-bots.\n");
    return;
  }
  if (!(u->flags & USER_OWNER) && (u2->flags & USER_BOT) &&
      (bot_flags(u2) & BOT_SHARE)) {
    dprintf(idx, "You can't add hostmasks to share-bots.\n");
    return;
  }
  if ((u2->flags & USER_OWNER) &&
      !(u->flags & USER_OWNER) && strcasecmp(handle, dcc[idx].nick)) {
    dprintf(idx, "Can't add hostmasks to the bot owner.\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# +host %s %s", dcc[idx].nick, handle, host);
  addhost_by_handle(handle, host);
  dprintf(idx, "Added '%s' to %s\n", host, handle);
}

static void cmd_mns_host(struct userrec *u, int idx, char *par)
{
  char *handle, *host;
  struct userrec *u2;
  struct flag_record fr =
  {FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

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
  if (strcasecmp(handle, dcc[idx].nick)) {
    get_user_flagrec(u, &fr, NULL);
    if (!(u2->flags & USER_BOT) && !(u->flags & USER_MASTER) &&
	!chan_master(fr)) {
      dprintf(idx, "You can't remove hostmasks from non-bots.\n");
      return;
    } else if ((u2->flags & USER_BOT) && (bot_flags(u2) & BOT_SHARE) &&
	       !(u->flags & USER_OWNER)) {
      dprintf(idx, "You can't remove hostmask from a shared bot.\n");
      return;
    } else if ((u2->flags & USER_OWNER) && !(u->flags & USER_OWNER) &&
	       (u2 != u)) {
      dprintf(idx, "Can't remove hostmasks from the bot owner.\n");
      return;
    } else if (!(u->flags & USER_BOTMAST) && !chan_master(fr)) {
      dprintf(idx, "Permission denied.\n");
      return;
    }
  }
  if (delhost_by_handle(handle, host)) {
    putlog(LOG_CMDS, "*", "#%s# -host %s %s", dcc[idx].nick, handle, host);
    dprintf(idx, "Removed '%s' from %s\n", host, handle);
  } else
    dprintf(idx, "Failed.\n");
}

static void cmd_modules(struct userrec *u, int idx, char *par)
{
  int ptr;

  context;
  if (!par[0])
    dprintf(idx, "Usage: modules <bot>\n");
  else {
    putlog(LOG_CMDS, "*", "#%s# modules %s", dcc[idx].nick, par);
    if ((ptr = nextbot(par)) >= 0)
      dprintf(ptr, "v %s %s %d:%s\n", botnetnick, par, dcc[idx].sock,
	      dcc[idx].nick);
    else
      dprintf(idx, "No such bot online.\n");
  }
}

/* DCC CHAT COMMANDS */
/* function call should be:
 * int cmd_whatever(idx,"parameters");
 * as with msg commands, function is responsible for any logging
 */
cmd_t C_dcc[64] =
{
  {"+bot", "t", (Function) cmd_pls_bot, NULL},
  {"+host", "tm|m", (Function) cmd_pls_host, NULL},
  {"+ignore", "m", (Function) cmd_pls_ignore, NULL},
  {"+user", "m", (Function) cmd_pls_user, NULL},
  {"-bot", "t", (Function) cmd_mns_user, NULL},
  {"-host", "", (Function) cmd_mns_host, NULL},
  {"-ignore", "m", (Function) cmd_mns_ignore, NULL},
  {"-user", "m", (Function) cmd_mns_user, NULL},
  {"addlog", "to|o", (Function) cmd_addlog, NULL},
  {"away", "", (Function) cmd_away, NULL},
  {"back", "", (Function) cmd_back, NULL},
  {"backup", "m|m", (Function) cmd_backup, NULL},
  {"banner", "t", (Function) cmd_banner, NULL},
  {"binds", "m", (Function) cmd_binds, NULL},
  {"boot", "t", (Function) cmd_boot, NULL},
  {"botattr", "t", (Function) cmd_botattr, NULL},
  {"botinfo", "t", (Function) cmd_botinfo, NULL},
  {"bots", "", (Function) cmd_bots, NULL},
  {"bottree", "t", (Function) cmd_bottree, NULL},
  {"chaddr", "t", (Function) cmd_chaddr, NULL},
  {"chat", "", (Function) cmd_chat, NULL},
  {"chattr", "m|m", (Function) cmd_chattr, NULL},
  {"chnick", "t", (Function) cmd_chnick, NULL},
  {"chpass", "t", (Function) cmd_chpass, NULL},
  {"comment", "m", (Function) cmd_comment, NULL},
  {"console", "to|o", (Function) cmd_console, NULL},
  {"dccstat", "t", (Function) cmd_dccstat, NULL},
  {"debug", "m", (Function) cmd_debug, NULL},
  {"die", "n", (Function) cmd_die, NULL},
  {"echo", "", (Function) cmd_echo, NULL},
  {"fixcodes", "", (Function) cmd_fixcodes, NULL},
  {"help", "", (Function) cmd_help, NULL},
  {"ignores", "m", (Function) cmd_ignores, NULL},
  {"link", "t", (Function) cmd_link, NULL},
  {"loadmod", "n", (Function) cmd_loadmod, NULL},
  {"match", "to|o", (Function) cmd_match, NULL},
  {"me", "", (Function) cmd_me, NULL},
  {"module", "m", (Function) cmd_module, NULL},
  {"modules", "n", (Function) cmd_modules, NULL},
  {"motd", "", (Function) cmd_motd, NULL},
  {"newpass", "", (Function) cmd_newpass, NULL},
  {"nick", "", (Function) cmd_nick, NULL},
  {"page", "", (Function) cmd_page, NULL},
  {"quit", "", (Function) 0, NULL},
  {"rehash", "m", (Function) cmd_rehash, NULL},
  {"rehelp", "n", (Function) cmd_rehelp, NULL},
  {"relay", "t", (Function) cmd_relay, NULL},
  {"reload", "m|m", (Function) cmd_reload, NULL},
  {"restart", "m", (Function) cmd_restart, NULL},
  {"save", "m|m", (Function) cmd_save, NULL},
  {"set", "n", (Function) cmd_set, NULL},
  {"simul", "n", (Function) cmd_simul, NULL},
  {"status", "m|m", (Function) cmd_status, NULL},
  {"strip", "", (Function) cmd_strip, NULL},
  {"su", "", (Function) cmd_su, NULL},
  {"tcl", "n", (Function) cmd_tcl, NULL},
  {"trace", "", (Function) cmd_trace, NULL},
  {"unlink", "t", (Function) cmd_unlink, NULL},
  {"unloadmod", "n", (Function) cmd_unloadmod, NULL},
  {"uptime", "m|m", (Function) cmd_uptime, NULL},
  {"vbottree", "t", (Function) cmd_vbottree, NULL},
  {"who", "", (Function) cmd_who, NULL},
  {"whois", "to|o", (Function) cmd_whois, NULL},
  {"whom", "", (Function) cmd_whom, NULL},
};
