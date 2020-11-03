/*
 * servmsg.c -- part of server.mod
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2020 Eggheads Development Team
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

#undef answer /* before resolv.h because it could collide with src/mod/module.h
		 (dietlibc) */
#include <resolv.h> /* base64 encode b64_ntop() and base64 decode b64_pton() */
#ifdef TLS
  #include <openssl/err.h>
#endif
#include "../irc.mod/irc.h"
#include "../channels.mod/channels.h"
#include "server.h"

static time_t last_ctcp = (time_t) 0L;
static int count_ctcp = 0;
static char altnick_char = 0;
struct cap_list cap = {"", "", ""};
int ncapesc, account_notify = 0, extended_join = 0;
Tcl_Obj **ncapesv, *ncapeslist;

/* We try to change to a preferred unique nick here. We always first try the
 * specified alternate nick. If that fails, we repeatedly modify the nick
 * until it gets accepted.
 *
 * sent nick:
 *     "<altnick><c>"
 *                ^--- additional count character: 1-9^-_\\[]`a-z
 *          ^--------- given, alternate nick
 *
 * The last added character is always saved in altnick_char. At the very first
 * attempt (where altnick_char is 0), we try the alternate nick without any
 * additions.
 *
 * fixed by guppy (1999/02/24) and Fabian (1999/11/26)
 */
static int gotfake433(char *from)
{
  int l = strlen(botname) - 1;

  /* First run? */
  if (altnick_char == 0) {
    char *alt = get_altbotnick();

    if (alt[0] && (rfc_casecmp(alt, botname)))
      /* Alternate nickname defined. Let's try that first. */
      strcpy(botname, alt);
    else {
      /* Fall back to appending count char. */
      altnick_char = '0';
      if ((l + 1) == nick_len) {
        botname[l] = altnick_char;
      } else {
        botname[++l] = altnick_char;
        botname[l + 1] = 0;
      }
    }
    /* No, we already tried the default stuff. Now we'll go through variations
     * of the original alternate nick.
     */
  } else {
    char *oknicks = "^-_\\[]`";
    char *p = strchr(oknicks, altnick_char);

    if (p == NULL) {
      if (altnick_char == '9')
        altnick_char = oknicks[0];
      else
        altnick_char = altnick_char + 1;
    } else {
      p++;
      if (!*p)
        altnick_char = 'a' + randint(26);
      else
        altnick_char = (*p);
    }
    botname[l] = altnick_char;
  }
  putlog(LOG_MISC, "*", IRC_BOTNICKINUSE, botname);
  dprintf(DP_MODE, "NICK %s\n", botname);
  return 0;
}

/* Check for tcl-bound msg command, return 1 if found
 *
 * msg: proc-name <nick> <user@host> <handle> <args...>
 */
static int check_tcl_msg(char *cmd, char *nick, char *uhost,
                         struct userrec *u, char *args)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };
  char *hand = u ? u->handle : "*";
  int x;

  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_msg1", nick, 0);
  Tcl_SetVar(interp, "_msg2", uhost, 0);
  Tcl_SetVar(interp, "_msg3", hand, 0);
  Tcl_SetVar(interp, "_msg4", args, 0);
  x = check_tcl_bind(H_msg, cmd, &fr, " $_msg1 $_msg2 $_msg3 $_msg4",
                     MATCH_EXACT | BIND_HAS_BUILTINS | BIND_USE_ATTR);
  if (x == BIND_EXEC_LOG)
    putlog(LOG_CMDS, "*", "(%s!%s) !%s! %s %s", nick, uhost, hand, cmd, args);
  return ((x == BIND_MATCHED) || (x == BIND_EXECUTED) || (x == BIND_EXEC_LOG));
}

static int check_tcl_msgm(char *cmd, char *nick, char *uhost,
                          struct userrec *u, char *arg)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };
  int x;
  char args[1024];

  if (arg[0])
    simple_sprintf(args, "%s %s", cmd, arg);
  else
    strlcpy(args, cmd, sizeof args);
  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_msgm1", nick, 0);
  Tcl_SetVar(interp, "_msgm2", uhost, 0);
  Tcl_SetVar(interp, "_msgm3", u ? u->handle : "*", 0);
  Tcl_SetVar(interp, "_msgm4", args, 0);
  x = check_tcl_bind(H_msgm, args, &fr, " $_msgm1 $_msgm2 $_msgm3 $_msgm4",
                     MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE | BIND_STACKRET);

  /*
   * 0 - no match
   * 1 - match, log
   * 2 - match, don't log
   */
  if (x == BIND_NOMATCH)
    return 0;
  if (x == BIND_EXEC_LOG)
    return 2;

  return 1;
}

static int check_tcl_notc(char *nick, char *uhost, struct userrec *u,
                          char *dest, char *arg)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };
  int x;

  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_notc1", nick, 0);
  Tcl_SetVar(interp, "_notc2", uhost, 0);
  Tcl_SetVar(interp, "_notc3", u ? u->handle : "*", 0);
  Tcl_SetVar(interp, "_notc4", arg, 0);
  Tcl_SetVar(interp, "_notc5", dest, 0);
  x = check_tcl_bind(H_notc, arg, &fr, " $_notc1 $_notc2 $_notc3 $_notc4 $_notc5",
                     MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE | BIND_STACKRET);

  /*
   * 0 - no match
   * 1 - match, log
   * 2 - match, don't log
   */
  if (x == BIND_NOMATCH)
    return 0;
  if (x == BIND_EXEC_LOG)
    return 2;

  return 1;
}

static int check_tcl_raw(char *from, char *code, char *msg)
{
  int x;

  Tcl_SetVar(interp, "_raw1", from, 0);
  Tcl_SetVar(interp, "_raw2", code, 0);
  Tcl_SetVar(interp, "_raw3", msg, 0);
  x = check_tcl_bind(H_raw, code, 0, " $_raw1 $_raw2 $_raw3",
                     MATCH_EXACT | BIND_STACKABLE | BIND_WANTRET);

  /* Return 1 if processed */
  return (x == BIND_EXEC_LOG);
}

/* tagstr is a space-separated list of key/value pairs */
static int check_tcl_rawt(char *from, char *code, char *msg, char *tagstr)
{
  int x;
  char * ptr;
  Tcl_DString tagdict;

  Tcl_DStringInit(&tagdict);
  Tcl_SetVar(interp, "_rawt1", from, 0);
  Tcl_SetVar(interp, "_rawt2", code, 0);
  Tcl_SetVar(interp, "_rawt3", msg, 0);
  ptr = strtok(tagstr, " ");
  if (!msgtag) {
    Tcl_SetVar(interp, "_rawt4", NULL, 0);
  } else {
    while (ptr != NULL) {
      Tcl_DStringAppendElement(&tagdict, ptr);
      ptr = strtok(NULL, " ");
    }
  }
  Tcl_SetVar(interp, "_rawt4", Tcl_DStringValue(&tagdict), 0);
  x = check_tcl_bind(H_rawt, code, 0, " $_rawt1 $_rawt2 $_rawt3 $_rawt4",
                    MATCH_EXACT | BIND_STACKABLE | BIND_WANTRET);
  Tcl_DStringFree(&tagdict);
  return (x == BIND_EXEC_LOG);
}

static int check_tcl_ctcpr(char *nick, char *uhost, struct userrec *u,
                           char *dest, char *keyword, char *args,
                           p_tcl_bind_list table)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };
  int x;

  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_ctcpr1", nick, 0);
  Tcl_SetVar(interp, "_ctcpr2", uhost, 0);
  Tcl_SetVar(interp, "_ctcpr3", u ? u->handle : "*", 0);
  Tcl_SetVar(interp, "_ctcpr4", dest, 0);
  Tcl_SetVar(interp, "_ctcpr5", keyword, 0);
  Tcl_SetVar(interp, "_ctcpr6", args, 0);
  x = check_tcl_bind(table, keyword, &fr,
                     " $_ctcpr1 $_ctcpr2 $_ctcpr3 $_ctcpr4 $_ctcpr5 $_ctcpr6",
                     MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE |
                     ((table == H_ctcp) ? BIND_WANTRET : 0));
  return (x == BIND_EXEC_LOG) || (table == H_ctcr);
}

static int check_tcl_wall(char *from, char *msg)
{
  int x;

  Tcl_SetVar(interp, "_wall1", from, 0);
  Tcl_SetVar(interp, "_wall2", msg, 0);
  x = check_tcl_bind(H_wall, msg, 0, " $_wall1 $_wall2",
                     MATCH_MASK | BIND_STACKABLE | BIND_STACKRET);

  /*
   * 0 - no match
   * 1 - match, log
   * 2 - match, don't log
   */
  if (x == BIND_NOMATCH)
    return 0;
  if (x == BIND_EXEC_LOG)
    return 2;

  return 1;
}

static int check_tcl_account(char *nick, char *uhost, char *mask,
                            struct userrec *u, char *chan,  char *account)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };
  int x;

  Tcl_SetVar(interp, "_acnt1", nick, 0);
  Tcl_SetVar(interp, "_acnt2", uhost, 0);
  Tcl_SetVar(interp, "_acnt3", u ? u->handle : "*", 0);
  Tcl_SetVar(interp, "_acnt4", chan, 0);
  Tcl_SetVar(interp, "_acnt5", account, 0);
  x = check_tcl_bind(H_account, mask, &fr,
       " $_acnt1 $_acnt2 $_acnt3 $_acnt4 $_acnt5", MATCH_MASK | BIND_STACKABLE);
  return (x == BIND_EXEC_LOG);
}

static int check_tcl_flud(char *nick, char *uhost, struct userrec *u,
                          char *ftype, char *chname)
{
  int x;

  Tcl_SetVar(interp, "_flud1", nick, 0);
  Tcl_SetVar(interp, "_flud2", uhost, 0);
  Tcl_SetVar(interp, "_flud3", u ? u->handle : "*", 0);
  Tcl_SetVar(interp, "_flud4", ftype, 0);
  Tcl_SetVar(interp, "_flud5", chname, 0);
  x = check_tcl_bind(H_flud, ftype, 0,
                     " $_flud1 $_flud2 $_flud3 $_flud4 $_flud5",
                     MATCH_MASK | BIND_STACKABLE | BIND_WANTRET);
  return (x == BIND_EXEC_LOG);
}

static int check_tcl_out(int which, char *msg, int sent)
{
  int x;
  char args[32], *queue;

  switch (which) {
  case DP_MODE:
  case DP_MODE_NEXT:
    queue = "mode";
    break;
  case DP_SERVER:
  case DP_SERVER_NEXT:
    queue = "server";
    break;
  case DP_HELP:
  case DP_HELP_NEXT:
    queue = "help";
    break;
  default:
    queue = "noqueue";
  }
  egg_snprintf(args, sizeof args, "%s %s", queue, sent ? "sent" : "queued");
  Tcl_SetVar(interp, "_out1", queue, 0);
  Tcl_SetVar(interp, "_out2", msg, 0);
  Tcl_SetVar(interp, "_out3", sent ? "sent" : "queued", 0);
  x = check_tcl_bind(H_out, args, 0, " $_out1 $_out2 $_out3",
                     MATCH_MASK | BIND_STACKABLE | BIND_WANTRET);

  return (x == BIND_EXEC_LOG);
}

static int match_my_nick(char *nick)
{
  return (!rfc_casecmp(nick, botname));
}

/* 001: welcome to IRC (use it to fix the server name) */
static int got001(char *from, char *msg)
{
  int i;
  char *key;
  struct chanset_t *chan;
  struct server_list *x = serverlist;

  /* FIXME - x should never be NULL anywhere in this function, but
   * apparently it sometimes is. */
  if (x) {
    for (i = curserv; i > 0 && x; i--)
      x = x->next;
    if (!x) {
      putlog(LOG_MISC, "*", "Invalid server list!");
    } else {
      if (x->realname)
        nfree(x->realname);
      x->realname = nmalloc(strlen(from) + 1);
      strcpy(x->realname, from);
    }
    if (realservername)
      nfree(realservername);
    realservername = nmalloc(strlen(from) + 1);
    strcpy(realservername, from);
  } else
    putlog(LOG_MISC, "*", "No server list!");

  server_online = now;
  fixcolon(msg);
  strlcpy(botname, msg, NICKLEN);
  altnick_char = 0;
  dprintf(DP_SERVER, "WHOIS %s\n", botname); /* get user@host */
  if (initserver[0])
    do_tcl("init-server", initserver); /* Call Tcl init-server */
  check_tcl_event("init-server");

  if (!x)
    return 0;

  if (module_find("irc", 0, 0)) {  /* Only join if the IRC module is loaded. */
    for (chan = chanset; chan; chan = chan->next) {
      chan->status &= ~(CHAN_ACTIVE | CHAN_PEND);
      if (!channel_inactive(chan)) {

        key = chan->channel.key[0] ? chan->channel.key : chan->key_prot;
        if (key[0])
          dprintf(DP_SERVER, "JOIN %s %s\n",
                  chan->name[0] ? chan->name : chan->dname, key);
        else
          dprintf(DP_SERVER, "JOIN %s\n",
                  chan->name[0] ? chan->name : chan->dname);
      }
    }
  }

  return 0;
}

/* Got 005: ISUPPORT network information
 */
static int got005(char *from, char *msg)
{
  newsplit(&msg); /* skip botnick */
  isupport_parse(msg, isupport_set);
  return 0;
}

/* Got 442: not on channel
 */
static int got442(char *from, char *msg)
{
  char *chname, *key;
  struct chanset_t *chan;

  if (!realservername || strcasecmp(from, realservername))
    return 0;
  newsplit(&msg);
  chname = newsplit(&msg);
  chan = findchan(chname);
  if (chan && !channel_inactive(chan)) {
    module_entry *me = module_find("channels", 0, 0);

    putlog(LOG_MISC, chname, IRC_SERVNOTONCHAN, chname);
    if (me && me->funcs)
      (me->funcs[CHANNEL_CLEAR]) (chan, CHAN_RESETALL);
    chan->status &= ~CHAN_ACTIVE;

    key = chan->channel.key[0] ? chan->channel.key : chan->key_prot;
    if (key[0])
      dprintf(DP_SERVER, "JOIN %s %s\n", chan->name, key);
    else
      dprintf(DP_SERVER, "JOIN %s\n", chan->name);
  }
  return 0;
}

/* Close the current server connection.
 */
static void nuke_server(char *reason)
{
  struct chanset_t *chan;
  module_entry *me;

  if (serv >= 0) {
    int servidx = findanyidx(serv);

    if (reason && (servidx > 0))
      dprintf(servidx, "QUIT :%s\n", reason);
    for (chan = chanset; chan; chan = chan->next) {
      if (channel_active(chan))
        if ((me = module_find("irc", 1, 3)) != NULL)
          (me->funcs[CHANNEL_CLEAR]) (chan, CHAN_RESETALL);
    }

    disconnect_server(servidx);
    lostdcc(servidx);
  }
}

static char ctcp_reply[1024] = "";

static int lastmsgs[FLOOD_GLOBAL_MAX];
static char lastmsghost[FLOOD_GLOBAL_MAX][81];
static time_t lastmsgtime[FLOOD_GLOBAL_MAX];

/* Do on NICK, PRIVMSG, NOTICE and JOIN.
 */
static int detect_flood(char *floodnick, char *floodhost, char *from, int which)
{
  char *p, ftype[10], h[1024];
  struct userrec *u;
  int thr = 0, lapse = 0, atr;

  /* Okay, make sure i'm not flood-checking myself */
  if (match_my_nick(floodnick))
    return 0;

  /* My user@host (?) */
  if (!strcasecmp(floodhost, botuserhost))
    return 0;

  u = get_user_by_host(from);
  atr = u ? u->flags : 0;
  if (atr & (USER_BOT | USER_FRIEND))
    return 0;

  /* Determine how many are necessary to make a flood */
  switch (which) {
  case FLOOD_PRIVMSG:
  case FLOOD_NOTICE:
    thr = flud_thr;
    lapse = flud_time;
    strcpy(ftype, "msg");
    break;
  case FLOOD_CTCP:
    thr = flud_ctcp_thr;
    lapse = flud_ctcp_time;
    strcpy(ftype, "ctcp");
    break;
  }
  if ((thr == 0) || (lapse == 0))
    return 0;                   /* No flood protection */

  p = strchr(floodhost, '@');
  if (p) {
    p++;
    if (strcasecmp(lastmsghost[which], p)) {        /* New */
      strlcpy(lastmsghost[which], p, sizeof lastmsghost[which]);
      lastmsgtime[which] = now;
      lastmsgs[which] = 0;
      return 0;
    }
  } else
    return 0;                   /* Uh... whatever. */

  if (lastmsgtime[which] < now - lapse) {
    /* Flood timer expired, reset it */
    lastmsgtime[which] = now;
    lastmsgs[which] = 0;
    return 0;
  }
  lastmsgs[which]++;
  if (lastmsgs[which] >= thr) { /* FLOOD */
    /* Reset counters */
    lastmsgs[which] = 0;
    lastmsgtime[which] = 0;
    lastmsghost[which][0] = 0;
    u = get_user_by_host(from);
    if (check_tcl_flud(floodnick, floodhost, u, ftype, "*"))
      return 0;
    /* Private msg */
    simple_sprintf(h, "*!*@%s", p);
    putlog(LOG_MISC, "*", IRC_FLOODIGNORE1, p);
    addignore(h, botnetnick, (which == FLOOD_CTCP) ? "CTCP flood" :
              "MSG/NOTICE flood", now + (60 * ignore_time));
  }
  return 0;
}

/* Got a private message.
 */
static int gotmsg(char *from, char *msg, char *tag)
{
  char *to, buf[UHOSTLEN], *nick, ctcpbuf[512], *uhost = buf, *ctcp,
       *p, *p1, *code;
  struct userrec *u;
  int ctcp_count = 0;
  int ignoring;

  /* Notice to a channel, not handled here */
  if (msg[0] && ((strchr(CHANMETA, *msg) != NULL) || (*msg == '@')))
    return 0;

  ignoring = match_ignore(from);
  to = newsplit(&msg);
  fixcolon(msg);
  strlcpy(uhost, from, sizeof buf);
  nick = splitnick(&uhost);
  /* Apparently servers can send CTCPs now too, not just nicks */
  if (nick[0] == '\0')
    nick = uhost;

  /* Check for CTCP: */
  ctcp_reply[0] = 0;
  p = strchr(msg, 1);
  while ((p != NULL) && (*p)) {
    p++;
    p1 = p;
    while ((*p != 1) && (*p != 0))
      p++;
    if (*p == 1) {
      *p = 0;
      strlcpy(ctcpbuf, p1, sizeof(ctcpbuf));
      ctcp = ctcpbuf;

      /* remove the ctcp in msg */
      memmove(p1 - 1, p + 1, strlen(p + 1) + 1);

      if (!ignoring)
        detect_flood(nick, uhost, from,
                     strncmp(ctcp, "ACTION ", 7) ? FLOOD_CTCP : FLOOD_PRIVMSG);
      /* Respond to the first answer_ctcp */
      p = strchr(msg, 1);
      if (ctcp_count < answer_ctcp) {
        ctcp_count++;
        if (ctcp[0] != ' ') {
          code = newsplit(&ctcp);

          /* CTCP from oper, don't interpret */
          if ((to[0] == '$') || strchr(to, '.')) {
            if (!ignoring)
              putlog(LOG_PUBLIC, to, "CTCP %s: %s from %s (%s) to %s",
                     code, ctcp, nick, uhost, to);
          } else {
            u = get_user_by_host(from);
            if (!ignoring || trigger_on_ignore) {
              if (!check_tcl_ctcp(nick, uhost, u, to, code, ctcp) && !ignoring) {
                if ((lowercase_ctcp && !strcasecmp(code, "DCC")) ||
                    (!lowercase_ctcp && !strcmp(code, "DCC"))) {
                  /* If it gets this far unhandled, it means that
                   * the user is totally unknown.
                   */
                  code = newsplit(&ctcp);
                  if (!strcmp(code, "CHAT")) {
                    if (!quiet_reject) {
                      if (u)
                        dprintf(DP_HELP, "NOTICE %s :I'm not accepting calls "
                                "at the moment.\n", nick);
                      else
                        dprintf(DP_HELP, "NOTICE %s :%s\n", nick,
                                DCC_NOSTRANGERS);
                    }
                    putlog(LOG_MISC, "*", "%s: %s", DCC_REFUSED, from);
                  } else
                    putlog(LOG_MISC, "*", "Refused DCC %s: %s", code, from);
                }
              }
              if (!strcmp(code, "ACTION")) {
                putlog(LOG_MSGS, "*", "Action to %s: %s %s", to, nick, ctcp);
              } else {
                putlog(LOG_MSGS, "*", "CTCP %s: %s from %s (%s)", code, ctcp,
                       nick, uhost);
              }                 /* I love a good close cascade ;) */
            }
          }
        }
      }
    }
  }
  /* Send out possible ctcp responses */
  if (ctcp_reply[0]) {
    if (ctcp_mode != 2) {
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, ctcp_reply);
    } else {
      if (now - last_ctcp > flud_ctcp_time) {
        dprintf(DP_HELP, "NOTICE %s :%s\n", nick, ctcp_reply);
        count_ctcp = 1;
      } else if (count_ctcp < flud_ctcp_thr) {
        dprintf(DP_HELP, "NOTICE %s :%s\n", nick, ctcp_reply);
        count_ctcp++;
      }
      last_ctcp = now;
    }
  }
  if (msg[0]) {
    int result = 0;

    /* Msg from oper, don't interpret */
    if ((to[0] == '$') || (strchr(to, '.') != NULL)) {
      if (!ignoring) {
        detect_flood(nick, uhost, from, FLOOD_PRIVMSG);
        putlog(LOG_MSGS | LOG_SERV, "*", "[%s!%s to %s] %s",
               nick, uhost, to, msg);
      }
      return 0;
    }

    detect_flood(nick, uhost, from, FLOOD_PRIVMSG);
    u = get_user_by_host(from);
    code = newsplit(&msg);
    rmspace(msg);

    if (!ignoring || trigger_on_ignore) {
      result = check_tcl_msgm(code, nick, uhost, u, msg);

      if (!result || !exclusive_binds)
        if (check_tcl_msg(code, nick, uhost, u, msg))
          return 0;
    }

    if (!ignoring && result != 2)
      putlog(LOG_MSGS, "*", "[%s] %s %s", from, code, msg);
  }
  return 0;
}

/* Got a private notice.
 */
static int gotnotice(char *from, char *msg)
{
  char *to, *nick, ctcpbuf[512], *p, *p1, buf[512], *uhost = buf, *ctcp;
  struct userrec *u;
  int ignoring;

  /* Notice to a channel, not handled here */
  if (msg[0] && ((strchr(CHANMETA, *msg) != NULL) || (*msg == '@')))
    return 0;

  ignoring = match_ignore(from);
  to = newsplit(&msg);
  fixcolon(msg);
  strlcpy(uhost, from, sizeof buf);
  nick = splitnick(&uhost);

  /* Check for CTCP: */
  p = strchr(msg, 1);
  while ((p != NULL) && (*p)) {
    p++;
    p1 = p;
    while ((*p != 1) && (*p != 0))
      p++;
    if (*p == 1) {
      *p = 0;
      ctcp = strcpy(ctcpbuf, p1);
      strcpy(p1 - 1, p + 1);
      if (!ignoring)
        detect_flood(nick, uhost, from, FLOOD_CTCP);
      p = strchr(msg, 1);
      if (ctcp[0] != ' ') {
        char *code = newsplit(&ctcp);

        /* CTCP reply from oper, don't interpret */
        if ((to[0] == '$') || strchr(to, '.')) {
          if (!ignoring)
            putlog(LOG_PUBLIC, "*",
                   "CTCP reply %s: %s from %s (%s) to %s", code, ctcp,
                   nick, uhost, to);
        } else {
          u = get_user_by_host(from);
          if (!ignoring || trigger_on_ignore) {
            check_tcl_ctcr(nick, uhost, u, to, code, ctcp);
            if (!ignoring)
              /* Who cares? */
              putlog(LOG_MSGS, "*",
                     "CTCP reply %s: %s from %s (%s) to %s",
                     code, ctcp, nick, uhost, to);
          }
        }
      }
    }
  }
  if (msg[0]) {

    /* Notice from oper, don't interpret */
    if ((to[0] == '$') || (strchr(to, '.') != NULL)) {
      if (!ignoring) {
        detect_flood(nick, uhost, from, FLOOD_NOTICE);
        putlog(LOG_MSGS | LOG_SERV, "*", "-%s (%s) to %s- %s",
               nick, uhost, to, msg);
      }
      return 0;
    }

    /* Server notice? */
    if ((nick[0] == 0) || (uhost[0] == 0)) {

      /* Hidden `250' connection count message from server */
      if (strncmp(msg, "Highest connection count:", 25))
        putlog(LOG_SERV, "*", "-NOTICE- %s", msg);

      return 0;
    }

    detect_flood(nick, uhost, from, FLOOD_NOTICE);
    u = get_user_by_host(from);

    if (!ignoring || trigger_on_ignore)
      if (check_tcl_notc(nick, uhost, u, botname, msg) == 2)
        return 0;

    if (!ignoring)
      putlog(LOG_MSGS, "*", "-%s (%s)- %s", nick, uhost, msg);
  }
  return 0;
}

static int gottagmsg(char *from, char *msg) {
  char *nick;
  fixcolon(msg);
  if (strchr(from, '!')) {
    nick = splitnick(&from);
    putlog(LOG_SERV, "*", "[#]%s(%s)[#] %s", nick, from, msg);
  } else {
    putlog(LOG_SERV, "*", "[#]%s[#] %s");
  }
  return 0;
}

/* WALLOPS: oper's nuisance
 */
static int gotwall(char *from, char *msg)
{
  char *nick;

  fixcolon(msg);

  if (check_tcl_wall(from, msg) != 2) {
    if (strchr(from, '!')) {
      nick = splitnick(&from);
      putlog(LOG_WALL, "*", "!%s(%s)! %s", nick, from, msg);
    } else
      putlog(LOG_WALL, "*", "!%s! %s", from, msg);
  }
  return 0;
}

/* Called once a minute...
 */
static void minutely_checks()
{
  char *alt;

  /* Only check if we have already successfully logged in.  */
  if (!server_online)
    return;
  if (keepnick) {
    /* NOTE: now that botname can but up to NICKLEN bytes long,
     * check that it's not just a truncation of the full nick.
     */
    if (strncmp(botname, origbotname, strlen(botname))) {
      /* See if my nickname is in use and if if my nick is right. */
      alt = get_altbotnick();
      if (alt[0] && strcasecmp(botname, alt))
        dprintf(DP_SERVER, "ISON :%s %s %s\n", botname, origbotname, alt);
      else
        dprintf(DP_SERVER, "ISON :%s %s\n", botname, origbotname);
    }
  }
}

/* Pong from server.
 */
static int gotpong(char *from, char *msg)
{
  newsplit(&msg);
  fixcolon(msg);                /* Scrap server name */
  server_lag = now - my_atoul(msg);
  if (server_lag > 99999) {
    /* IRCnet lagmeter support by drummer */
    server_lag = now - lastpingtime;
  }
  return 0;
}

/* This is a reply on ISON :<current> <orig> [<alt>]
 */
static void got303(char *from, char *msg)
{
  char *tmp, *alt;
  int ison_orig = 0, ison_alt = 0;

  if (!keepnick || !strncmp(botname, origbotname, strlen(botname))) {
    return;
  }
  newsplit(&msg);
  fixcolon(msg);
  alt = get_altbotnick();
  tmp = newsplit(&msg);
  if (tmp[0] && !rfc_casecmp(botname, tmp)) {
    while ((tmp = newsplit(&msg))[0]) { /* no, it's NOT == */
      if (!rfc_casecmp(tmp, origbotname))
        ison_orig = 1;
      else if (alt[0] && !rfc_casecmp(tmp, alt))
        ison_alt = 1;
    }
    if (!ison_orig) {
      if (!nick_juped)
        putlog(LOG_MISC, "*", IRC_GETORIGNICK, origbotname);
      dprintf(DP_SERVER, "NICK %s\n", origbotname);
    } else if (alt[0] && !ison_alt && rfc_casecmp(botname, alt)) {
      putlog(LOG_MISC, "*", IRC_GETALTNICK, alt);
      dprintf(DP_SERVER, "NICK %s\n", alt);
    }
  }
}

/* 432 : Bad nickname
 */
static int got432(char *from, char *msg)
{
  char *erroneous, nick[nick_len + 1];

  newsplit(&msg);
  erroneous = newsplit(&msg);
  if (server_online)
    putlog(LOG_MISC, "*", "NICK IS INVALID: '%s' (keeping '%s').", erroneous,
           botname);
  else {
    putlog(LOG_MISC, "*", IRC_BADBOTNICK);
    if (!keepnick) {
      make_rand_str_from_chars(nick, sizeof nick - 1, CHARSET_ALPHA);
      putlog(LOG_MISC, "*", "NICK IS INVALID: '%s' (using '%s' instead)",
              erroneous, nick);
      dprintf(DP_MODE, "NICK %s\n", nick);
    }
    return 0;
  }
  return 0;
}

/* 433 : Nickname in use
 * Change nicks till we're acceptable or we give up
 */
static int got433(char *from, char *msg)
{
  char *tmp;

  if (server_online) {
    /* We are online and have a nickname, we'll keep it */
    newsplit(&msg);
    tmp = newsplit(&msg);
    putlog(LOG_MISC, "*", "NICK IN USE: %s (keeping '%s').", tmp, botname);
    nick_juped = 0;
    return 0;
  }
  gotfake433(from);
  return 0;
}

/* 437 : Nickname juped (IRCnet)
 */
static int got437(char *from, char *msg)
{
  char *s;
  struct chanset_t *chan;

  newsplit(&msg);
  s = newsplit(&msg);
  if (s[0] && (strchr(CHANMETA, s[0]) != NULL)) {
    chan = findchan(s);
    if (chan) {
      if (chan->status & CHAN_ACTIVE) {
        putlog(LOG_MISC, "*", IRC_CANTCHANGENICK, s);
      } else {
        if (!channel_juped(chan)) {
          putlog(LOG_MISC, "*", IRC_CHANNELJUPED, s);
          chan->status |= CHAN_JUPED;
        }
      }
    }
  } else if (server_online) {
    if (!nick_juped)
      putlog(LOG_MISC, "*", "NICK IS JUPED: %s (keeping '%s').", s, botname);
    if (!rfc_casecmp(s, origbotname))
      nick_juped = 1;
  } else {
    putlog(LOG_MISC, "*", "%s: %s", IRC_BOTNICKJUPED, s);
    gotfake433(from);
  }
  return 0;
}

/* 438 : Nick change too fast
 */
static int got438(char *from, char *msg)
{
  newsplit(&msg);
  newsplit(&msg);
  fixcolon(msg);
  putlog(LOG_MISC, "*", "%s", msg);
  return 0;
}

static int got451(char *from, char *msg)
{
  /* Usually if we get this then we really messed up somewhere
   * or this is a non-standard server, so we log it and kill the socket
   * hoping the next server will work :) -poptix
   */
  /* Um, this does occur on a lagged anti-spoof server connection if the
   * (minutely) sending of joins occurs before the bot does its ping reply.
   * Probably should do something about it some time - beldin
   */
  putlog(LOG_MISC, "*", IRC_NOTREGISTERED1, from);
  nuke_server(IRC_NOTREGISTERED2);
  return 0;
}

/* Got error notice
 */
static int goterror(char *from, char *msg)
{
  /* FIXME: fixcolon doesn't do what we need here, this is a temp fix
   * fixcolon(msg);
   */
  if (msg[0] == ':')
    msg++;
  putlog(LOG_SERV | LOG_MSGS, "*", "-ERROR from server- %s", msg);
  if (serverror_quit) {
    putlog(LOG_SERV, "*", "Disconnecting from server.");
    nuke_server("Bah, stupid error messages.");
  }
  return 1;
}

/* Got nick change.
 */
static int gotnick(char *from, char *msg)
{
  char *nick, *alt = get_altbotnick();

  nick = splitnick(&from);
  fixcolon(msg);
  check_queues(nick, msg);
  if (match_my_nick(nick)) {
    /* Regained nick! */
    strlcpy(botname, msg, NICKLEN);
    altnick_char = 0;
    if (!strcmp(msg, origbotname)) {
      putlog(LOG_SERV | LOG_MISC, "*", "Regained nickname '%s'.", msg);
      nick_juped = 0;
    } else if (alt[0] && !strcmp(msg, alt))
      putlog(LOG_SERV | LOG_MISC, "*", "Regained alternate nickname '%s'.",
             msg);
    else if (keepnick && strcmp(nick, msg)) {
      putlog(LOG_SERV | LOG_MISC, "*", "Nickname changed to '%s'???", msg);
      if (!rfc_casecmp(nick, origbotname)) {
        putlog(LOG_MISC, "*", IRC_GETORIGNICK, origbotname);
        dprintf(DP_SERVER, "NICK %s\n", origbotname);
      } else if (alt[0] && !rfc_casecmp(nick, alt) &&
               strcasecmp(botname, origbotname)) {
        putlog(LOG_MISC, "*", IRC_GETALTNICK, alt);
        dprintf(DP_SERVER, "NICK %s\n", alt);
      }
    } else
      putlog(LOG_SERV | LOG_MISC, "*", "Nickname changed to '%s'???", msg);
  } else if ((keepnick) && (rfc_casecmp(nick, msg))) {
    /* Only do the below if there was actual nick change, case doesn't count */
    if (!rfc_casecmp(nick, origbotname)) {
      putlog(LOG_MISC, "*", IRC_GETORIGNICK, origbotname);
      dprintf(DP_SERVER, "NICK %s\n", origbotname);
    } else if (alt[0] && !rfc_casecmp(nick, alt) &&
             strcasecmp(botname, origbotname)) {
      putlog(LOG_MISC, "*", IRC_GETALTNICK, altnick);
      dprintf(DP_SERVER, "NICK %s\n", altnick);
    }
  }
  return 0;
}

static int gotmode(char *from, char *msg)
{
  char *ch;

  ch = newsplit(&msg);
  /* Usermode changes? */
  if (strchr(CHANMETA, ch[0]) == NULL) {
    if (match_my_nick(ch)) {
      fixcolon(msg);
      if ((msg[0] == '+') || (msg[0] == '-')) {
        /* send a WHOIS in case our host was cloaked */
        dprintf(DP_SERVER, "WHOIS %s\n", botname);
      }
      if (check_mode_r) {
        /* umode +r? - D0H dalnet uses it to mean something different */
        if ((msg[0] == '+') && strchr(msg, 'r')) {
          int servidx = findanyidx(serv);

          putlog(LOG_MISC | LOG_JOIN, "*",
                 "%s has me i-lined (jumping)", dcc[servidx].host);
          nuke_server("i-lines suck");
        }
      }
    }
  }
  return 0;
}

static void disconnect_server(int idx)
{
  if (server_online > 0)
    check_tcl_event("disconnect-server");
  *cap.supported = 0;
  *cap.negotiated = 0;
  *cap.desired = 0;
  server_online = 0;
  if (realservername)
    nfree(realservername);
  realservername = 0;
  /* $::server should be empty for this, so isupport binds can ignore it */
  isupport_clear_values(0);
  if (dcc[idx].sock >= 0)
    killsock(dcc[idx].sock);
  dcc[idx].sock = -1;
  serv = -1;
  botuserhost[0] = 0;
}

static void eof_server(int idx)
{
  putlog(LOG_SERV, "*", "%s %s", IRC_DISCONNECTED, dcc[idx].host);
  disconnect_server(idx);
  lostdcc(idx);
}

static void display_server(int idx, char *buf)
{
  sprintf(buf, "%s  (lag: %d)", trying_server ? "conn" : "serv", server_lag);
}

static void connect_server(void);

static void kill_server(int idx, void *x)
{
  module_entry *me;

  disconnect_server(idx);
  if ((me = module_find("channels", 0, 0)) && me->funcs) {
    struct chanset_t *chan;

    for (chan = chanset; chan; chan = chan->next)
      (me->funcs[CHANNEL_CLEAR]) (chan, CHAN_RESETALL);
  }
  /* A new server connection will be automatically initiated in
   * about 2 seconds. */
}

static void timeout_server(int idx)
{
  putlog(LOG_SERV, "*", "Timeout: connect to %s", dcc[idx].host);
  disconnect_server(idx);
  check_tcl_event("fail-server");
  lostdcc(idx);
}

static void server_activity(int idx, char *msg, int len);

static struct dcc_table SERVER_SOCKET = {
  "SERVER",
  0,
  eof_server,
  server_activity,
  NULL,
  timeout_server,
  display_server,
  NULL,
  kill_server,
  NULL
};

static void server_activity(int idx, char *tagmsg, int len)
{
  char *from, *code, *s1, *s2, *saveptr1=NULL, *saveptr2=NULL, *tagstrptr=NULL;
  char *token, *subtoken, tagstr[TOTALTAGMAX+1], tagdict[TOTALTAGMAX+1] = "";
  char *msgptr, rawmsg[RECVLINEMAX+7];
  int taglen, i, found;

  if (trying_server) {
    strcpy(dcc[idx].nick, "(server)");
    putlog(LOG_SERV, "*", "Connected to %s", dcc[idx].host);
    trying_server = 0;
    SERVER_SOCKET.timeout_val = 0;
  }
  lastpingcheck = 0;
/* Check if message-tags are enabled and, if so, check/grab the tag */
  msgptr = tagmsg;
  strlcpy(rawmsg, tagmsg, TOTALTAGMAX+1);
  if (msgtag) {
    if (*tagmsg == '@') {
      taglen = 0;
      tagstrptr = newsplit(&msgptr);
      strlcpy(tagstr, tagstrptr, TOTALTAGMAX+1);
      tagstrptr++;     /* Remove @ */
      /* Split each key/value pair apart, then split the key from the value */
      for (i = 0, s1 = tagstrptr; ; i++, s1 = NULL){
        token = strtok_r(s1, ";", &saveptr1);
        if (token == NULL) {
          break;
        }
        if (*token == '+') {
          token++;
        }
        if (strchr(token, '=')) {
          found = 0;
          for (s2 = token; ; s2 = NULL) {
            subtoken = strtok_r(s2, "=", &saveptr2);
            if (subtoken == NULL) {
              break;
            }
            taglen += egg_snprintf(tagdict + taglen, TOTALTAGMAX - taglen,
                  "%s ", subtoken);
            found++;
          }
          /* Account for tags (not key/value pairs), prep empty value for Tcl */
          if (found < 2) {
            taglen += egg_snprintf(tagdict + taglen, TOTALTAGMAX - taglen,
                "{} ");
          }
        }
      }
      if (taglen > 0) {
        tagdict[taglen-1] = '\0';     /* Remove trailing space */
      }
    }
  }
  from = "";
  if (*msgptr == ':') {
    msgptr++;
    from = newsplit(&msgptr);
  }
  code = newsplit(&msgptr);
  if (raw_log && ((strcmp(code, "PRIVMSG") && strcmp(code, "NOTICE")) ||
      !match_ignore(from))) {
    putlog(LOG_RAW, "*", "[@] %s", rawmsg);
  }
  /* Check both raw and rawt, to allow backwards compatibility with older
   * scripts */
  check_tcl_rawt(from, code, msgptr, tagdict);
  check_tcl_raw(from, code, msgptr);
}

static int gotping(char *from, char *msg)
{
  fixcolon(msg);
  dprintf(DP_MODE, "PONG :%s\n", msg);
  return 0;
}

static int gotkick(char *from, char *msg)
{
  char *nick;

  nick = from;
  if (rfc_casecmp(nick, botname))
    /* Not my kick, I don't need to bother about it. */
    return 0;
  if (use_penalties) {
    last_time += 2;
    if (raw_log)
      putlog(LOG_SRVOUT, "*", "adding 2secs penalty (successful kick)");
  }
  return 0;
}

/* Another sec penalty if bot did a whois on another server.
 */
static int whoispenalty(char *from, char *msg)
{
  if (realservername && use_penalties &&
      strcasecmp(from, realservername)) {

    last_time += 1;

    if (raw_log)
      putlog(LOG_SRVOUT, "*", "adding 1sec penalty (remote whois)");
  }

  return 0;
}

static int got311(char *from, char *msg)
{
  char *n1, *n2, *u, *h;

  n1 = newsplit(&msg);
  n2 = newsplit(&msg);
  u = newsplit(&msg);
  h = newsplit(&msg);

  if (!n1 || !n2 || !u || !h)
    return 0;

  if (match_my_nick(n2))
    egg_snprintf(botuserhost, sizeof botuserhost, "%s@%s", u, h);

  return 0;
}

static int got396orchghost(char *nick, char *user, char *uhost)
{
  struct chanset_t *chan;
  memberlist *m;

  for (chan = chanset; chan; chan = chan->next) {
    m = ismember(chan, nick);
    if (m) {
      snprintf(m->userhost, sizeof m->userhost, "%s@%s", user, uhost);
      strcpy(botuserhost, m->userhost);
    }
  }
  return 0;
}


/* React to IRCv3 CHGHOST command. CHGHOST changes the hostname and/or
 * ident of the user. Format:
 * :geo!awesome@eggdrop.com CHGHOST tehgeo foo.io
 * changes user hostmask to tehgeo@foo.io
 */
static int gotchghost(char *from, char *msg){
  char *nick, *user;

  nick = splitnick(&from); /* Get the nick */
  user = newsplit(&msg);  /* Get the user */
  got396orchghost(nick, user, msg);
  return 0;
}

/* React to 396 numeric (HOSTHIDDEN), sent when user mode +x (hostmasking) was
 * successfully set. Format:
 * :barjavel.freenode.net 396 BeerBot unaffiliated/geo/bot/beerbot :is now your hidden host (set by services.)
 */
static int got396(char *from, char *msg)
{
  char *nick, *uhost, *user, userbuf[UHOSTLEN];

  nick = newsplit(&msg);
  if (match_my_nick(nick)) {  /* Double check this really is for me */
    uhost = newsplit(&msg);
    strlcpy(userbuf, botuserhost, sizeof userbuf);
    user = strtok(userbuf, "@");
    got396orchghost(nick, user, uhost);
  }
  return 0;
}

static int gotsetname(char *from, char *msg)
{
  fixcolon(msg);
  strlcpy(botrealname, msg, sizeof botrealname);
  return 0;
}

static int tryauthenticate(char *from, char *msg) 
{
  char src[(sizeof sasl_username) + (sizeof sasl_username) +
           (sizeof sasl_password)] = "";
  char *s;
  /* 400-byte chunk, see: https://ircv3.net/specs/extensions/sasl-3.1.html
   * base64 padding */
  #ifndef MAX
    #define MAX(a,b) (((a)>(b))?(a):(b))
  #endif
  unsigned char dst[((MAX((sizeof src), 400) + 2) / 3) << 2] = "";
#ifdef HAVE_EVP_PKEY_GET1_EC_KEY
  EC_KEY *eckey;
  int ret;
  int olen;
  unsigned int olen2;
  unsigned char *dst2;
  FILE *fp;
  EVP_PKEY *privateKey;
#endif /* HAVE_EVP_PKEY_GET1_EC_KEY */
  putlog(LOG_SERV, "*", "SASL: got AUTHENTICATE %s", msg);
  if (msg[0] == '+') {
    s = src;
    /* Don't use snprintf due to \0 inside */
    if (sasl_mechanism == SASL_MECHANISM_PLAIN) {
      strcpy(s, sasl_username);
      s += strlen(sasl_username) + 1;
      strcpy(s, sasl_username);
      s += strlen(sasl_username) + 1;
      strcpy(s, sasl_password);
      s += strlen(sasl_password);
      dst[0] = 0;
      if (b64_ntop((unsigned char *) src, s - src, (char *) dst, sizeof dst) == -1) {
        putlog(LOG_SERV, "*", "SASL: AUTHENTICATE error: could not base64 "
                    "encode");
        /* TODO: send cap end for all error cases in this function ? */
        return 1;
      }
      /* TODO: what about olen we used for mbedtls_base64_encode() ? */
    }
    else if (sasl_mechanism == SASL_MECHANISM_ECDSA_NIST256P_CHALLENGE) {
#ifdef HAVE_EVP_PKEY_GET1_EC_KEY
      strcpy(s, sasl_username);
      s += strlen(sasl_username) + 1;
      strcpy(s, sasl_username);
      s += strlen(sasl_username);
      if (b64_ntop((unsigned char *) src, s - src, (char *) dst, sizeof dst) == -1) {
        putlog(LOG_SERV, "*", "SASL: AUTHENTICATE error: could not base64 "
                "encode");
        return 1;
      }
    }
#else
      putlog(LOG_DEBUG, "*", "SASL: TLS libs missing EC support, try PLAIN or "
                "EXTERNAL method");
      return 1;
    }
#endif
    else {          /* sasl_mechanism == SASL_MECHANISM_EXTERNAL */
#ifdef TLS          /* TLS required for EXTERNAL sasl */ 
      dst[0] = '+';
      dst[1] = 0;
    }
    putlog(LOG_SERV, "*", "SASL: put AUTHENTICATE %s", dst);
    dprintf(DP_MODE, "AUTHENTICATE %s\n", dst);
#else
    putlog(LOG_DEBUG, "*", "SASL: TLS libs required for EXTERNAL but are not "
            "installed, try PLAIN method");
    }
#endif /* TLS */
  } else {      /* Only EC-challenges get extra auth messages w/o a + */
#ifdef TLS
#ifdef HAVE_EVP_PKEY_GET1_EC_KEY
    putlog(LOG_SERV, "*", "SASL: got AUTHENTICATE Challenge");
    olen = b64_pton(msg, dst, sizeof dst);
    if (olen == -1) {
      putlog(LOG_SERV, "*", "SASL: AUTHENTICATE error: could not base64 decode "
                "line from server");
      return 1;
    }
    fp = fopen(sasl_ecdsa_key, "r");
    if (!fp) {
      putlog(LOG_SERV, "*", "SASL: AUTHENTICATE error: could not open file "
                "sasl_ecdsa_key %s: %s\n", sasl_ecdsa_key, strerror(errno));
      return 1;
    }
    privateKey = PEM_read_PrivateKey(fp, NULL, 0, NULL);
    if (!privateKey) {
      putlog(LOG_SERV, "*", "SASL: AUTHENTICATE: PEM_read_PrivateKey(): SSL "
                "error = %s\n", ERR_error_string(ERR_get_error(), 0));
      fclose(fp);
      return 1;
    }
    fclose(fp);
    eckey = EVP_PKEY_get1_EC_KEY(privateKey);
    EVP_PKEY_free(privateKey);
    if (!eckey) {
      putlog(LOG_SERV, "*", "SASL: AUTHENTICATE: EVP_PKEY_get1_EC_KEY(): SSL error = %s\n",
             ERR_error_string(ERR_get_error(), 0));
      return 1;
    }
    dst2 = nmalloc(ECDSA_size(eckey));
    ret = ECDSA_sign(0, dst, olen, dst2, &olen2, eckey);
    EC_KEY_free(eckey);
    if (!ret) {
      printf("SASL: AUTHENTICATE: ECDSA_sign() SSL error = %s\n",
             ERR_error_string(ERR_get_error(), 0));
      nfree(dst2);
      return 1;
    } 
    if (b64_ntop(dst2, olen2, (char *) dst, sizeof dst) == -1) {
      putlog(LOG_SERV, "*", "SASL: AUTHENTICATE error: could not base64 encode");
      return 1;
    }
    nfree(dst2);
    putlog(LOG_SERV, "*", "SASL: put AUTHENTICATE Response %s", dst);
    dprintf(DP_MODE, "AUTHENTICATE %s\n", dst);
#endif /* HAVE_EVP_PKEY_GET1_EC_KEY */
#else /* TLS */
    dprintf(LOG_DEBUG, "*", "SASL: Received EC message, but no TLS EC libs "
                "present. Try PLAIN method");
    return 1;
#endif /* TLS */
  }
  return 0;
}

static int gotauthenticate(char *from, char *msg)
{
  fixcolon(msg); /* Because Inspircd does its own thing */
  if (tryauthenticate(from, msg) && !sasl_continue) {
    putlog(LOG_DEBUG, "*", "SASL: Aborting connection and retrying");
    nuke_server("Quitting...");
    return 1;
  }
  return 0;
}

/* Got 900: RPL_SASLLOGGEDIN, user account name is set */
static int got900(char *from, char *msg)
{
  newsplit(&msg); /* nick */
  newsplit(&msg); /* nick!ident@host */
  newsplit(&msg); /* account */
  fixcolon(msg);
  putlog(LOG_SERV, "*", "SASL: %s", msg);
  return 0;
}

static int sasl_error(char *msg)
{
  putlog(LOG_SERV, "*", "SASL: %s", msg);
  dprintf(DP_MODE, "CAP END\n");
  sasl_timeout_time = 0;
  if (!sasl_continue) {
    putlog(LOG_DEBUG, "*", "SASL: Aborting connection and retrying");
    nuke_server("Quitting...");
  }
  return 1;
}

/* Got 904: ERR_SASLFAIL, invalid credentials (or something not covered)
   Got 905: ERR_SASLTOOLONG, AUTHENTICATE command was too long (>400 bytes)
   Got 906: ERR_SASL_ABORTED, sent AUTHENTICATE command with * as parameter
 */
static int got904905and906(char *from, char *msg)
{
  newsplit(&msg); /* nick */
  fixcolon(msg);
  return sasl_error(msg);
}

/* Got 903: RPL_SASLSUCCESS, authentication successful */
static int got903(char *from, char *msg)
{
  newsplit(&msg); /* nick */
  fixcolon(msg);
  putlog(LOG_SERV, "*", "SASL: %s", msg);
  dprintf(DP_MODE, "CAP END\n");
  sasl_timeout_time = 0;
  return 0;
}

/* Got 908: RPL_SASLMECHS, mechanisms supported by network */
static int got908(char *from, char *msg)
{
  newsplit(&msg); /* nick */
  fixcolon(msg);
  putlog(LOG_SERV, "*", "SASL: %s", msg);
  return 0;
}

static int handle_sasl_timeout()
{
  return sasl_error("timeout");
}

/* Got ACCOUNT message; only valid for account-notify capability */
static int gotaccount(char *from, char *msg) {
  struct chanset_t *chan;
  struct userrec *u;
  memberlist *m;
  char *nick, *chname, mask[CHANNELLEN+UHOSTLEN+NICKMAX+2];

  u = get_user_by_host(from);
  nick = splitnick(&from);
  for (chan = chanset; chan; chan = chan->next) {
    chname = chan->dname;
    if ((m = ismember(chan, nick))) {
      strlcpy (m->account, msg, sizeof m->account);
      snprintf(mask, sizeof mask, "%s %s", chname, from);
      if (!strcasecmp(msg, "*")) {
        msg[0] = '\0';
        putlog(LOG_JOIN | LOG_MISC, chname, "%s!%s has logged out of their "
                "account", nick, from);
      } else {
        putlog(LOG_JOIN | LOG_MISC, chname, "%s!%s has logged into account %s",
                nick, from, msg);
      }
      check_tcl_account(nick, from, mask, u, chname, msg);
    }
  }
  return 0;
}

/*
 * 465     ERR_YOUREBANNEDCREEP :You are banned from this server
 */
static int got465(char *from, char *msg)
{
  newsplit(&msg); /* nick */

  fixcolon(msg);

  putlog(LOG_SERV, "*", "Server (%s) says I'm banned: %s", from, msg);
  putlog(LOG_SERV, "*", "Disconnecting from server.");
  nuke_server("Banned from server.");
  return 1;
}

/*
 * Invalid CAP command
 */
static int got410(char *from, char *msg) {
  char *cmd;

  newsplit(&msg);
 
  putlog(LOG_SERV, "*", "%s", msg);
  cmd = newsplit(&msg);
  putlog(LOG_MISC, "*", "CAP sub-command %s not supported", cmd);

  return 1;
}

/* got417: ERR_INPUTTOOLONG. Client sent a message longer than allowed limit */
static int got417(char *from, char *msg) {
  newsplit(&msg);
  putlog (LOG_SERV, "*", "MESSAGE-TAG: %s reported error: %s", from, msg);

  return 1;
}

static int got421(char *from, char *msg) {
  newsplit(&msg);
  putlog(LOG_SERV, "*", "%s reported an error: %s", from, msg);

  return 1;
}

/* Add negotiated capability to Tcl List for easy addition/deletion */
void add_cape(char *cape) {
  if (!strstr(cap.negotiated, cape)) {
    putlog(LOG_DEBUG, "*", "CAP: Adding cape %s to negotiated list", cape);
    /* Update Tcl List object with new capability */
    Tcl_ListObjAppendElement(interp, ncapeslist, Tcl_NewStringObj(cape, -1));
    /* Update C variable with new capability */
    if (*cap.negotiated) {
      strncat(cap.negotiated, " ", CAPMAX - strlen(cap.negotiated) - 1);
    }
    strncat(cap.negotiated, cape, CAPMAX - strlen(cap.negotiated) - 1);
    /* This section adds status variables used by other modules, if they need to
     * know if a specific capability is active or not
     */
    if (!strcasecmp(cape, "message-tags") || !strcasecmp(cape, "twitch.tv/tags")) {
      msgtag = 1;
    } else if (!strcasecmp(cape, "extended-join")) {
      extended_join = 1;
    } else if (!strcasecmp(cape, "account-notify")) {
      account_notify = 1;
    }
  } else {
    putlog(LOG_DEBUG, "*", "CAP: %s is already added to negotiated list", cape);
  }
}

/* Remove capability from internal CAP request list */
void del_cape(char *cape) {
  int i, j, len = 0;
  if (strstr(cap.negotiated, cape)) {
    putlog(LOG_DEBUG, "*", "CAP: Removing %s from negotiated list", cape);
    Tcl_ListObjGetElements(interp, ncapeslist, &ncapesc, &ncapesv);
    for (i = 0; i < ncapesc; i++) {
      if (!strcmp(cape, Tcl_GetString(ncapesv[i]))) {
        /* Remove deleted capability from Tcl List object */
        Tcl_ListObjReplace(interp, ncapeslist, i, 1, 0, NULL);
        /* Match C variable to Tcl List object (ie, delete the capability) */
        Tcl_ListObjGetElements(interp, ncapeslist, &ncapesc, &ncapesv);
        if (!ncapesc) {
          *cap.negotiated = 0;
        } else {
          for (j = 0; j < ncapesc; j++) {
            if (j)
              cap.negotiated[len++] = ' ';
            len += strlcpy(cap.negotiated + len, Tcl_GetString(ncapesv[j]), sizeof cap.negotiated - len);
          }
        }
        if (!strcasecmp(cape, "message-tags") || !strcasecmp(cape, "twitch.tv/tags")) {
          msgtag = 0;
        } else if (!strcasecmp(cape, "extended-join")) {
          extended_join = 0;
        } else if (!strcasecmp(cape, "account-notify")) {
          account_notify = 0;
        }
      }
      if (!strcasecmp(cape, "message-tags") || !strcasecmp(cape, "twitch.tv/tags")) {
        msgtag = 0;
      } else if (!strcasecmp(cape, "extended-join")) {
        extended_join = 0;
      }
    }
  } else {
    putlog(LOG_DEBUG, "*", "CAP: %s is not on negotiated list", cape);
  }
}

/* Smash desired CAP capabilities into a single string */
void add_req(char *cape) {
  int len = strlen(cap.desired);
  if (len)
    cap.desired[len++] = ' ';
  strlcpy(cap.desired + len, cape, sizeof cap.desired - len);
}

static int gotcap(char *from, char *msg) {
  char *cmd, *splitstr;
  char cape[CAPMAX+1], *p;
  int listlen = 0;

  newsplit(&msg);
  putlog(LOG_DEBUG, "*", "CAP: %s", msg);
  cmd = newsplit(&msg);
  fixcolon(msg);
  if (!strcmp(cmd, "LS")) {
    putlog(LOG_DEBUG, "*", "CAP: %s supports CAP sub-commands: %s", from, msg);
    strlcpy(cap.supported, msg, sizeof cap.supported);
/* CAP is supported, yay! Lets load what we want to request */
    if (sasl) {
      add_req("sasl");
    }
/* Add any custom capes the user listed */
    strlcpy(cape, cap_request, sizeof cape);
    if ( (p = strtok(cape, " ")) ) {
      while (p != NULL) {
        add_req(p);
        p = strtok(NULL, " ");
      }
    }
    if (strlen(cap.desired) > 0) {
      putlog(LOG_DEBUG, "*", "CAP: Requesting %s capabilities from server", cap.desired);
      dprintf(DP_MODE, "CAP REQ :%s\n", cap.desired);
    } else {
      dprintf(DP_MODE, "CAP END\n");
    }
  } else if (!strcmp(cmd, "LIST")) {
    putlog(LOG_SERV, "*", "CAP: Negotiated CAP capabilities: %s", msg);
    /* You're getting the current list, may as well the clear old stuff */
    *cap.negotiated = 0;
    Tcl_ListObjLength(interp, ncapeslist, &listlen);
    Tcl_ListObjReplace(interp, ncapeslist, 0, listlen, 0, NULL);
    splitstr = strtok(msg, " ");
    while (splitstr != NULL) {
      add_cape(splitstr);
      splitstr = strtok(NULL, " ");
    }
  } else if (!strcmp(cmd, "ACK")) {
    Tcl_ListObjGetElements(interp, ncapeslist, &ncapesc, &ncapesv);
    splitstr = strtok(msg, " ");
    while (splitstr != NULL) {
      if (splitstr[0] == '-') { /* Remove this cape from negotiated list */
        splitstr++;
        del_cape(splitstr);
      } else {
        add_cape(splitstr);
      }
      splitstr = strtok(NULL, " ");
    }
    putlog(LOG_SERV, "*", "CAP: Current negotiations with %s: %s", from, cap.negotiated);
    /* If a negotiated capability requires immediate action by Eggdrop, add it
     * here. However, that capability must take responsibility for sending an
     * END. Future eggheads: add support for more than 1 of these async
     * capabilities, right now SASL is the only one so we're OK.
     */
    if (strstr(cap.negotiated, "sasl") && strstr(cap.desired, "sasl")) {
      *cap.desired = 0;
#ifndef HAVE_EVP_PKEY_GET1_EC_KEY
      if (sasl_mechanism != SASL_MECHANISM_ECDSA_NIST256P_CHALLENGE) {
#endif
        putlog(LOG_SERV, "*", "SASL: put AUTHENTICATE %s",
            SASL_MECHANISMS[sasl_mechanism]);
        dprintf(DP_MODE, "AUTHENTICATE %s\n", SASL_MECHANISMS[sasl_mechanism]);
        sasl_timeout_time = sasl_timeout;
#ifndef HAVE_EVP_PKEY_GET1_EC_KEY
      } else {
#ifdef TLS
        return sasl_error("TLS libs missing EC support, try PLAIN or EXTERNAL method, aborting authentication");
      }
#else /* TLS */
        if (sasl_mechanism != SASL_MECHANISM_PLAIN) {
	      return sasl_error("TLS libs not present, try PLAIN method, aborting authentication");
        }
      }
#endif /* TLS */
#endif /* HAVE_EVP_PKEY */
    } else {
      dprintf(DP_MODE, "CAP END\n");
      return 0;
    }
  } else if (!strcmp(cmd, "NAK")) {
    putlog(LOG_SERV, "*", "CAP: Requested capability change %s rejected by %s",
        msg, from);
    dprintf(DP_MODE, "CAP END\n");
  } else if (!strcmp(cmd, "NEW")) { /* TODO: CAP 302 stuff? */
    /* Do things */
  } else if (!strcmp(cmd, "DEL")) { /* TODO: CAP 302 stuff? */
    /* Do things */
  }
  return 1;
}

static int server_isupport(char *key, char *isset_str, char *value)
{
  int isset = !strcmp(isset_str, "1");

  if (!strcmp(key, "NICKLEN") || !strcmp(key, "MAXNICKLEN")) {
    isupport_parseint(key, isset ? value : NULL, 9, NICKMAX, 1, 9, &nick_len);
  }
  return 0;
}

static cmd_t my_raw_binds[] = {
  {"PRIVMSG",      "",   (IntFunc) gotmsg,          NULL},
  {"NOTICE",       "",   (IntFunc) gotnotice,       NULL},
  {"MODE",         "",   (IntFunc) gotmode,         NULL},
  {"PING",         "",   (IntFunc) gotping,         NULL},
  {"PONG",         "",   (IntFunc) gotpong,         NULL},
  {"WALLOPS",      "",   (IntFunc) gotwall,         NULL},
  {"001",          "",   (IntFunc) got001,          NULL},
  {"005",          "",   (IntFunc) got005,          NULL},
  {"303",          "",   (IntFunc) got303,          NULL},
  {"311",          "",   (IntFunc) got311,          NULL},
  {"318",          "",   (IntFunc) whoispenalty,    NULL},
  {"396",          "",   (IntFunc) got396,          NULL},
  {"410",          "",   (IntFunc) got410,          NULL},
  {"417",          "",   (IntFunc) got417,          NULL},
  {"421",          "",   (IntFunc) got421,          NULL},
  {"432",          "",   (IntFunc) got432,          NULL},
  {"433",          "",   (IntFunc) got433,          NULL},
  {"437",          "",   (IntFunc) got437,          NULL},
  {"438",          "",   (IntFunc) got438,          NULL},
  {"451",          "",   (IntFunc) got451,          NULL},
  {"442",          "",   (IntFunc) got442,          NULL},
  {"465",          "",   (IntFunc) got465,          NULL},
  {"900",          "",   (IntFunc) got900,          NULL},
  {"903",          "",   (IntFunc) got903,          NULL},
  {"904",          "",   (IntFunc) got904905and906, NULL},
  {"905",          "",   (IntFunc) got904905and906, NULL},
  {"906",          "",   (IntFunc) got904905and906, NULL},
  {"908",          "",   (IntFunc) got908,          NULL},
  {"NICK",         "",   (IntFunc) gotnick,         NULL},
  {"ERROR",        "",   (IntFunc) goterror,        NULL},
/* ircu2.10.10 has a bug when a client is throttled ERROR is sent wrong */
  {"ERROR:",       "",   (IntFunc) goterror,        NULL},
  {"KICK",         "",   (IntFunc) gotkick,         NULL},
  {"CAP",          "",   (IntFunc) gotcap,          NULL},
  {"AUTHENTICATE", "",   (IntFunc) gotauthenticate, NULL},
  {"ACCOUNT",      "",   (IntFunc) gotaccount,      NULL},
  {"CHGHOST",      "",   (IntFunc) gotchghost,      NULL},
  {"SETNAME",      "",   (IntFunc) gotsetname,      NULL},
  {NULL,           NULL, NULL,                      NULL}
};

static cmd_t my_rawt_binds[] = {
  {"TAGMSG",       "",   (IntFunc) gottagmsg,       NULL},
  {NULL,           NULL, NULL,                      NULL}
};

static cmd_t my_isupport_binds[] = {
  {"*",      "",   (IntFunc) server_isupport, "server:isupport"},
  {NULL,   NULL,   NULL,                                   NULL}
};

static void server_resolve_success(int);
static void server_resolve_failure(int);

/* Hook up to a server
 */
static void connect_server(void)
{
  char pass[121], botserver[UHOSTLEN], s[1024];
#ifdef IPV6
  char buf[sizeof(struct in6_addr)];
#endif
  int servidx, len = 0;
  unsigned int botserverport = 0;

  lastpingcheck = 0;
  trying_server = now;
  empty_msgq();
  if (newserverport) {          /* Jump to specified server */
    curserv = -1;             /* Reset server list */
    strcpy(botserver, newserver);
    botserverport = newserverport;
    strcpy(pass, newserverpass);
    newserver[0] = 0;
    newserverport = 0;
    newserverpass[0] = 0;
  } else {
    if (curserv == -1)
      curserv = 999;
    pass[0] = 0;
  }
  if (!cycle_time) {
    struct chanset_t *chan;
    struct server_list *x = serverlist;

    if (!x && !botserverport) {
      putlog(LOG_SERV, "*", "No servers in server list");
      cycle_time = 300;
      return;
    }

    servidx = new_dcc(&DCC_DNSWAIT, sizeof(struct dns_info));
    if (servidx < 0) {
      putlog(LOG_SERV, "*",
             "NO MORE DCC CONNECTIONS -- Can't create server connection.");
      return;
    }

    isupport_preconnect();
    if (connectserver[0])       /* drummer */
      do_tcl("connect-server", connectserver);
    check_tcl_event("connect-server");
    next_server(&curserv, botserver, &botserverport, pass);

#ifdef IPV6
    if (inet_pton(AF_INET6, botserver, buf)) {
      len += egg_snprintf(s, sizeof s, "%s [%s]", IRC_SERVERTRY, botserver);
    } else {
#endif
     len += egg_snprintf(s, sizeof s, "%s %s", IRC_SERVERTRY, botserver);
#ifdef IPV6
    }
#endif

#ifdef TLS
    len += egg_snprintf(s + len, sizeof s - len, ":%s%d",
            use_ssl ? "+" : "", botserverport);
    dcc[servidx].ssl = use_ssl;
#else
    len += egg_snprintf(s + len, sizeof s - len, ":%d", botserverport);
#endif
    putlog(LOG_SERV, "*", "%s", s);
    dcc[servidx].port = botserverport;
    strcpy(dcc[servidx].nick, "(server)");
    strlcpy(dcc[servidx].host, botserver, UHOSTLEN);

    botuserhost[0] = 0;

    nick_juped = 0;
    for (chan = chanset; chan; chan = chan->next)
      chan->status &= ~CHAN_JUPED;

    dcc[servidx].timeval = now;
    dcc[servidx].sock = -1;
    dcc[servidx].u.dns->host = get_data_ptr(strlen(dcc[servidx].host) + 1);
    strcpy(dcc[servidx].u.dns->host, dcc[servidx].host);
    dcc[servidx].u.dns->cbuf = get_data_ptr(strlen(pass) + 1);
    strcpy(dcc[servidx].u.dns->cbuf, pass);
    dcc[servidx].u.dns->dns_success = server_resolve_success;
    dcc[servidx].u.dns->dns_failure = server_resolve_failure;
    dcc[servidx].u.dns->dns_type = RES_IPBYHOST;
    dcc[servidx].u.dns->type = &SERVER_SOCKET;

    if (server_cycle_wait)
      /* Back to 1st server & set wait time.
       * Note: Put it here, just in case the server quits on us quickly
       */
      cycle_time = server_cycle_wait;
    else
      cycle_time = 0;

    /* I'm resolving... don't start another server connect request */
    resolvserv = 1;
    /* Resolve the hostname. */
    dcc_dnsipbyhost(dcc[servidx].host);
  }
}

static void server_resolve_failure(int servidx)
{
  serv = -1;
  resolvserv = 0;
  putlog(LOG_SERV, "*", "%s %s (%s)", IRC_FAILEDCONNECT, dcc[servidx].host,
         IRC_DNSFAILED);
  check_tcl_event("fail-server");
  lostdcc(servidx);
}

static void server_resolve_success(int servidx)
{
  char pass[121];

  resolvserv = 0;
  strlcpy(pass, dcc[servidx].u.dns->cbuf, sizeof pass);
  changeover_dcc(servidx, &SERVER_SOCKET, 0);
  dcc[servidx].sock = getsock(dcc[servidx].sockname.family, 0);
  setsnport(dcc[servidx].sockname, dcc[servidx].port);
  serv = open_telnet_raw(dcc[servidx].sock, &dcc[servidx].sockname);
  if (serv < 0) {
    char *errstr = NULL;
    if (errno == EINVAL) {
      errstr = IRC_VHOSTWRONGNET;
    } else if (errno == EADDRNOTAVAIL) {
      errstr = IRC_VHOSTBADADDR;
    } else {
      errstr = strerror(errno);
    }
    putlog(LOG_SERV, "*", "%s %s (%s)", IRC_FAILEDCONNECT, dcc[servidx].host,
           errstr);
    check_tcl_event("fail-server");
    lostdcc(servidx);
    return;
  }
  /* Setup ident with server values populated */
  check_tcl_event("ident");
#ifdef TLS
  if (dcc[servidx].ssl && ssl_handshake(serv, TLS_CONNECT, tls_vfyserver,
                                        LOG_SERV, dcc[servidx].host, NULL)) {
    putlog(LOG_SERV, "*", "%s %s (%s)", IRC_FAILEDCONNECT, dcc[servidx].host,
           "TLS negotiation failure");
    check_tcl_event("fail-server");
    lostdcc(servidx);
    return;
  }
#endif
  /* Queue standard login */
  dcc[servidx].timeval = now;
  SERVER_SOCKET.timeout_val = &server_timeout;
  /* Another server may have truncated it, so use the original */
  strcpy(botname, origbotname);
  /* Start alternate nicks from the beginning */
  altnick_char = 0;
  check_tcl_event("preinit-server");
  ncapeslist = Tcl_NewListObj(0, NULL);
  /* See if server supports CAP command */
  dprintf(DP_MODE, "CAP LS\n");
  if (pass[0])
    dprintf(DP_MODE, "PASS %s\n", pass);
  dprintf(DP_MODE, "NICK %s\n", botname);

  rmspace(botrealname);
  if (botrealname[0] == 0)
    strcpy(botrealname, "/msg LamestBot hello");
  dprintf(DP_MODE, "USER %s . . :%s\n", botuser, botrealname);

  /* Wait for async result now. */
}
