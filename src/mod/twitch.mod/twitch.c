/*
 * twitch.c -- part of twitch.mod
 *   A module that allows Eggdrop to connect to the Twitch game streaming
 *   service. Mostly.
 *   
 *   Twitch has an IRC interface, but it is only provided as a gateway and does
 *   not follow RFC in 97% of what it does. This module is intended to add some
 *   basic logging features, add some binds for twitch events, and track
 *   rudimentary userstate and roomstate values for channels. Most of your
 *   traditional Eggdrop functions are gone, and would not work with Twitch
 *   anyway.
 *
 *   Twitch has threatened to remove IRC support for some time now; that is
 *   obviously outside our control and if they do, again obviously this module
 *   will cease to work. Buyer beware.
 *
 * Originally written by Geo              April 2020
 */

/*
 * Copyright (C) 2020 - 2023 Eggheads Development Team
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

#define MODULE_NAME "twitch"
#define MAKING_TWITCH

#include "src/mod/module.h"
#include <stdlib.h>
#include "twitch.mod/twitch.h"
#include "server.mod/server.h"


#undef global
static Function *global = NULL, *server_funcs = NULL;

static p_tcl_bind_list H_ccht, H_cmsg, H_htgt, H_wspr, H_wspm, H_rmst, H_usst, H_usrntc;

twitchchan_t *twitchchan = NULL;
static char cap_request[55];

/* valuevar must be used immediately without calling back into Tcl, refcount is not increased */
#define GET_MSGTAG_VALUE_STR(tags, key, valuevar, errctx) do {                                              \
  Tcl_Obj *msgtagtmpvalue;                                                                                  \
  if (TCL_OK != Tcl_DictObjGet(interp, (tags), Tcl_NewStringObj((key), -1), &msgtagtmpvalue)) {             \
    putlog(LOG_MISC, "*", "* TWITCH: Error: Could not decode msgtag-dict message %s", (errctx));            \
    return 0;                                                                                               \
  }                                                                                                         \
  if (!msgtagtmpvalue) {                                                                                    \
    putlog(LOG_MISC, "*", "* TWITCH: Error: required msg-tag %s not found in message %s", (key), (errctx)); \
    return 0;                                                                                               \
  }                                                                                                         \
  (valuevar) = Tcl_GetString(msgtagtmpvalue);                                                               \
} while (0)

/* Calculate the memory we keep allocated.
 */
static int twitch_expmem()
{
  int size = 0;

  return size;
}

/* Find a twitch channel by it's display name */
twitchchan_t *findtchan_by_dname(char *name)
{
  twitchchan_t *chan;

  for (chan = twitchchan; chan; chan = chan->next)
    if (!rfc_casecmp(chan->dname, name))
      return chan;
  return NULL;
}

/* Remove given characters from a string */
void remove_chars(char* str, char c) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}

char *traced_keepnick(ClientData cd, Tcl_Interp *irp, EGG_CONST char *name1,
                   EGG_CONST char *name2, int flags)
{
  const char *value;

  if (flags & TCL_TRACE_DESTROYED) {
    Tcl_TraceVar(interp, "keep-nick", TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS, traced_keepnick, NULL);
  }
  value = Tcl_GetVar2(irp, name1, name2, TCL_GLOBAL_ONLY);
  if (value && strcmp(value, "0")) {
    putlog(LOG_MISC, "*", "Twitch: keep-nick is forced to be 0 when twitch.mod is loaded");
    Tcl_SetVar2(irp, name1, name2, "0", TCL_GLOBAL_ONLY);
  }
  return NULL;
}

static void cmd_twcmd(struct userrec *u, int idx, char *par) {
  char *chname;

  if (!par[0]) {
    dprintf(idx, "Usage: twcmd <channel> <cmd> [args]\n");
    return;
  }
  chname = newsplit(&par);
  if (!findtchan_by_dname(chname)) { /* Search for channel */
    dprintf(idx, "No such channel.\n");
    return;
  }
  dprintf(DP_SERVER, "PRIVMSG %s :/%s", chname, par);
  return;
}

static void cmd_roomstate(struct userrec *u, int idx, char *par) {
  twitchchan_t *tchan;

  if (!par[0]) {
    dprintf(idx, "Usage: roomstate <channel>\n");
    return;
  }
  if (!(tchan = findtchan_by_dname(par))) { /* Search for channel */
    dprintf(idx, "No such channel.\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# roomstate", dcc[idx].nick);
  dprintf(idx, "Roomstate for %s:\n", tchan->dname);
  dprintf(idx, "-------------------------------------\n");
  dprintf(idx, "Emote-only: %2d     Followers-only: %2d\n",
        tchan->emote_only, tchan->followers_only);
  dprintf(idx, "R9K:        %2d     Subs-only:      %2d\n",
        tchan->r9k, tchan->subs_only);
  dprintf(idx, "Slow:     %4d\n", tchan->slow);
  dprintf(idx, "End of roomstate info.\n");
  return;
}

static void cmd_userstate(struct userrec *u, int idx, char *par) {
  twitchchan_t *tchan;

  if (!par[0]) {
    dprintf(idx, "Usage: userstate <channel>\n");
    return;
  }
  if (!(tchan = findtchan_by_dname(par))) { /* Search for channel */
    dprintf(idx, "No such channel.\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# userstate", dcc[idx].nick);
  dprintf(idx, "Userstate for %s:\n", tchan->dname);
  dprintf(idx, "---------------------------------\n");
  dprintf(idx, "Display Name: %s\n", tchan->userstate.display_name);
  dprintf(idx, "Badges:       %s\n", tchan->userstate.badges);
  dprintf(idx, "Badge Info:   %d\n", tchan->userstate.badge_info);
  dprintf(idx, "Color:        %s\n", tchan->userstate.color);
  dprintf(idx, "Emote-Sets:   %s\n", tchan->userstate.emote_sets);
  dprintf(idx, "Moderator:    %s\n", tchan->userstate.mod ? "yes" : "no");
  dprintf(idx, "End of userstate info.\n");
  return;
}

static int check_tcl_clearchat(char *chan, char *nick) {
  int x;
  char mask[1024];
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  snprintf(mask, sizeof mask, "%s %s!%s@%s.tmi.twitch.tv",
                                chan, nick, nick, nick);
  Tcl_SetVar(interp, "_ccht1", nick ? (char *) nick : "", 0);
  Tcl_SetVar(interp, "_ccht2", chan, 0);
  x = check_tcl_bind(H_ccht, mask, &fr, " $_ccht1 $_ccht2",
        MATCH_MASK | BIND_STACKABLE);
  return (x == BIND_EXEC_LOG);
}

static int check_tcl_clearmsg(char *nick, char *chan, char *msgid, char *msg) {
  int x;
  char mask[1024];
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  snprintf(mask, sizeof mask, "%s %s!%s@%s.tmi.twitch.tv",
                                chan, nick, nick, nick);
  Tcl_SetVar(interp, "_cmsg1", nick, 0);
  Tcl_SetVar(interp, "_cmsg2", chan, 0);
  Tcl_SetVar(interp, "_cmsg3", msgid, 0);
  Tcl_SetVar(interp, "_cmsg4", msg, 0);
  x = check_tcl_bind(H_cmsg, mask, &fr, " $_cmsg1 $_cmsg2 $_cmsg3 $_cmsg4",
        MATCH_MASK | BIND_STACKABLE);
  return (x == BIND_EXEC_LOG);
}

static int check_tcl_hosttarget(char *chan, char *nick, char *viewers) {
  int x;
  char mask[1024];
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  snprintf(mask, sizeof mask, "%s %s", chan, nick);
  Tcl_SetVar(interp, "_htgt1", nick, 0);
  Tcl_SetVar(interp, "_htgt2", chan, 0);
  Tcl_SetVar(interp, "_htgt3", viewers, 0);
  x = check_tcl_bind(H_htgt, mask, &fr, " $_htgt1 $_htgt2 $_htgt3",
        MATCH_MASK | BIND_STACKABLE);

  return (x == BIND_EXEC_LOG);
}

static int check_tcl_whisper(char *from, char *cmd, char *msg) {
  char buf[UHOSTMAX], *uhost=buf, *nick, *hand;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };
  struct userrec *u = NULL;
  int x;

  strlcpy(uhost, from, sizeof buf);
  nick = splitnick(&uhost);
  get_user_flagrec(u, &fr, NULL);
  u = get_user_by_host(from);
  hand = (u ? u->handle : "*");
  Tcl_SetVar(interp, "_wspr1", nick, 0);
  Tcl_SetVar(interp, "_wspr2", uhost, 0);
  Tcl_SetVar(interp, "_wspr3", hand, 0);
  Tcl_SetVar(interp, "_wspr4", msg, 0);
  x = check_tcl_bind(H_wspr, cmd, &fr, " $_wspr1 $_wspr2 $_wspr3 $_wspr4",
        MATCH_MASK | BIND_STACKABLE);
  return (x == BIND_EXEC_LOG);
}

static int check_tcl_whisperm(char *from, char *cmd, char *msg) {
  char buf[UHOSTMAX], args[MSGMAX], *uhost=buf, *nick, *hand;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };
  struct userrec *u = NULL;
  int x;

  strlcpy(uhost, from, sizeof buf);
  nick = splitnick(&uhost);
  if (msg[0])                       /* Re-attach the cmd to the msg */
    snprintf(args, sizeof args, "%s %s", cmd, msg);
  else
    strlcpy(args, cmd, sizeof args);
  get_user_flagrec(u, &fr, NULL);
  u = get_user_by_host(from);
  hand = (u ? u->handle : "*");
  Tcl_SetVar(interp, "_wspm1", nick, 0);
  Tcl_SetVar(interp, "_wspm2", uhost, 0);
  Tcl_SetVar(interp, "_wspm3", hand, 0);
  Tcl_SetVar(interp, "_wspm4", args, 0);
  x = check_tcl_bind(H_wspm, args, &fr, " $_wspm1 $_wspm2 $_wspm3 $_wspm4",
        MATCH_MASK | BIND_STACKABLE);
  return (x == BIND_EXEC_LOG);
}

static void check_tcl_roomstate(char *chan, Tcl_Obj *tags) {
  char mask[TOTALTAGMAX + 200]; /* channel + key */

  snprintf(mask, sizeof mask, "%s %s", chan, encode_msgtags(tags));
  Tcl_SetVar(interp, "_rmst1", chan, 0);
  Tcl_SetVar(interp, "_rmst2", Tcl_GetString(tags), 0);
  check_tcl_bind(H_rmst, mask, NULL, " $_rmst1 $_rmst2",
        MATCH_MASK | BIND_STACKABLE);
}

static void check_tcl_userstate(char *chan, Tcl_Obj *tags) {
  char mask[TOTALTAGMAX + 200]; /* channel + key */

  snprintf(mask, sizeof mask, "%s %s", chan, encode_msgtags(tags));
  Tcl_SetVar(interp, "_usst1", chan, 0);
  Tcl_SetVar(interp, "_usst2", Tcl_GetString(tags), 0);
  check_tcl_bind(H_usst, mask, NULL, " $_usst1 $_usst2",
        MATCH_MASK | BIND_STACKABLE);
}

/* TODO: the actual message is missing */
static void check_tcl_usernotice(char *chan, Tcl_Obj *tags) {
  char mask[TOTALTAGMAX + 200]; /* channel + msgid */

  snprintf(mask, sizeof mask, "%s %s", chan, encode_msgtags(tags));
  Tcl_SetVar(interp, "_usrntc1", chan, 0);
  Tcl_SetVar(interp, "_usrntc2", Tcl_GetString(tags), 0);
  check_tcl_bind(H_usrntc, mask, NULL, " $_usrntc1 $_usrntc2",
        MATCH_MASK | BIND_STACKABLE);
}

/* Right now, we only use this to do some init stuff for a channel we join
 * since (are you tired of hearing it yet?) Twitch doesn't do normal IRC stuff
 */
static int gotjoin (char *from, char *msg) {
  char buf[UHOSTLEN], *uhost = buf, *chname, *nick;
  twitchchan_t *tchan;

  chname = newsplit(&msg);
  if (!(tchan = findtchan_by_dname(chname))) {    /* Find channel or, if it   */
    tchan = nmalloc(sizeof *tchan);             /* doesn't exist, create it */
    explicit_bzero(tchan, sizeof(twitchchan_t));
    strlcpy(tchan->dname, chname, sizeof tchan->dname);
    egg_list_append((struct list_type **) &twitchchan, (struct list_type *) tchan);
  }
  strlcpy(uhost, from, sizeof buf);
  nick = splitnick(&uhost);
  if (match_my_nick(nick)) {
    /* It was me joining! Let's get a list of mods and vips for the room */
    dprintf(DP_SERVER, "PRIVMSG %s :/mods", chname);
    dprintf(DP_SERVER, "PRIVMSG %s :/vips", chname);
  }
  return 0;
}


/* We use this to catch lists of mods and vips for a room */
static int gotnotice (char *from, char *msg, Tcl_Obj *tags) {
  twitchchan_t *tchan;
  char *chan, *modptr, *vipptr, *msgid;

  chan = newsplit(&msg);
  fixcolon(msg);
  tchan = findtchan_by_dname(chan);

  GET_MSGTAG_VALUE_STR(tags, "msg-id", msgid, "NOTICE");
  /* Check if this is a list of mods */
  if (!strcmp(msgid, "room_mods")) {
    modptr = msg + 36; /* Remove "The moderators of this channel are: " */
    remove_chars(modptr, ',');
    remove_chars(modptr, '.');
    strlcpy(tchan->mods, modptr, sizeof tchan->mods);
  } else if (!strcmp(msgid, "vips_success")) {
    vipptr = msg + 30; /* Remove "The VIPs of this channel are: " from str */
    remove_chars(vipptr, ',');
    remove_chars(vipptr, '.');
    strlcpy(tchan->vips, vipptr, sizeof tchan->vips);
  }
  return 0;
}


static int gotwhisper(char *from, char *msg, Tcl_Obj *tags) {
  int result = 0;
  char *code;

  newsplit(&msg);    /* Get rid of my own nick */
  fixcolon(msg);
  code = newsplit(&msg); /* In case whisperm bind */
  rmspace(msg);

  result = check_tcl_whisperm(from, code, msg);
  if (!result) {
    check_tcl_whisper(from, code, msg);
  }
  putlog(LOG_MSGS, "*", "[%s] %s %s", from, code, msg);
  return 0; 
}

static int gotclearmsg(char *from, char *msg, Tcl_Obj *tags) {
  char nick[NICKLEN], *chan, msgid[TOTALTAGMAX];
  char *value;
  
  chan = newsplit(&msg);
  fixcolon(msg);

  GET_MSGTAG_VALUE_STR(tags, "login", value, "CLEARMSG");
  strlcpy(nick, value, sizeof nick);

  GET_MSGTAG_VALUE_STR(tags, "target-msg-id", value, "CLEARMSG");
  strlcpy(msgid, value, sizeof msgid);

  check_tcl_clearmsg(nick, chan, msgid, msg);
  putlog(LOG_SERV, "*", "* TWITCH: Cleared message %s from %s", msgid, nick);
  return 0;
}

static int gotclearchat(char *from, char *msg) {
  char *nick=NULL, *chan=NULL;

  chan = newsplit(&msg);
  fixcolon(msg);
  nick = newsplit(&msg);
  check_tcl_clearchat(chan, nick);
  if (!strlen(nick)) {
    putlog(LOG_SERV, "*", "* TWITCH: Chat logs cleared on %s", chan);
  } else {
    putlog(LOG_SERV, "*", "* TWITCH: Chat logs cleared on %s for user %s", chan, nick);
  }
  return 0;
}

static int gothosttarget(char *from, char *msg) {
  char s[30], *nick, *chan, *viewers;

  chan = newsplit(&msg);
  fixcolon(msg);
  nick = newsplit(&msg);
  viewers = newsplit(&msg);
  if (viewers) {
    sprintf(s, " (Viewers: %s)", viewers);
  }
  check_tcl_hosttarget(chan, nick, viewers);
  if (nick[0] == '-') {             /* Check if it is an unhost */
    putlog(LOG_SERV, "*", "* TWITCH: %s has stopped host mode.", chan);
  } else {   
    putlog(LOG_SERV, "*", "* TWITCH: %s has started hosting %s%s",
            chan, nick, (viewers) ? s : "");
  }
  return 0;
}

static int gotuserstate(char *from, char *chan, Tcl_Obj *tags) {
  twitchchan_t *tchan;
  int done = 0, trigger_bind = 0;
  Tcl_DictSearch s;
  Tcl_Obj *value, *key;
  
  if (!(tchan = findtchan_by_dname(chan))) {    /* Find channel or, if it   */
    tchan = nmalloc(sizeof *tchan);             /* doesn't exist, create it */
    explicit_bzero(tchan, sizeof(twitchchan_t));
    strlcpy(tchan->dname, chan, sizeof tchan->dname);
    egg_list_append((struct list_type **) &twitchchan, (struct list_type *) tchan);
  }

  for (Tcl_DictObjFirst(interp, tags, &s, &key, &value, &done); !done; Tcl_DictObjNext(&s, &key, &value, &done)) {
    char *k = Tcl_GetString(key), *v = Tcl_GetString(value);
    long n = atol(v);
    int changed = 0;

    if (!strcmp(k, "badge-info") && tchan->userstate.badge_info != n) {
      changed = 1;
      tchan->userstate.badge_info = n;
    } else if (!strcmp(k, "badges") && strcmp(tchan->userstate.badges, v)) {
      changed = 1;
      strlcpy(tchan->userstate.badges, v, sizeof tchan->userstate.badges);
    } else if (!strcmp(k, "color") && strcmp(tchan->userstate.color, v)) {
      changed = 1;
      strlcpy(tchan->userstate.color, v, sizeof tchan->userstate.color);
    } else if (!strcmp(k, "display-name") && strcmp(tchan->userstate.display_name, v)) {
      changed = 1;
      strlcpy(tchan->userstate.display_name, v, sizeof tchan->userstate.display_name);
    } else if (!strcmp(k, "emote-sets") && strcmp(tchan->userstate.emote_sets, v)) {
      changed = 1;
      strlcpy(tchan->userstate.emote_sets, v, sizeof tchan->userstate.emote_sets);
    } else if (!strcmp(k, "mod") && tchan->userstate.mod != n) {
      changed = 1;
      tchan->userstate.mod = n;
    }
    if (changed) {
      putlog(LOG_SERV, "*", "* TWITCH: Userstate '%s' in room %s changed to %s", k, chan, v);
      trigger_bind = 1;
    }
  }
  if (trigger_bind) {
    check_tcl_userstate(chan, tags);
  }
  return 0;
}

static int gotroomstate(char *from, char *chan, Tcl_Obj *tags) {
  twitchchan_t *tchan;
  int done = 0;
  Tcl_DictSearch s;
  Tcl_Obj *value, *key;
  int trigger_bind = 0;

  if (!(tchan = findtchan_by_dname(chan))) {    /* Find channel or, if it   */
    tchan = nmalloc(sizeof *tchan);             /* doesn't exist, create it */
    explicit_bzero(tchan, sizeof(twitchchan_t));
    strlcpy(tchan->dname, chan, sizeof tchan->dname);
    egg_list_append((struct list_type **) &twitchchan, (struct list_type *) tchan);
  }

  for (Tcl_DictObjFirst(interp, tags, &s, &key, &value, &done); !done; Tcl_DictObjNext(&s, &key, &value, &done)) {
    char *k = Tcl_GetString(key), *v = Tcl_GetString(value);
    long n = atol(v);
    int changed = 0;

    if (!strcmp(k, "emote-only") && tchan->emote_only != n) {
      tchan->emote_only = n;
      changed = 1;
    } else if (!strcmp(k, "followers-only") && tchan->followers_only != n) {
      tchan->followers_only = n;
      changed = 1;
    } else if (!strcmp(k, "r9k") && tchan->r9k != n) {
      tchan->r9k = n;
      changed = 1;
    } else if (!strcmp(k, "subs-only") && tchan->subs_only != n) {
      tchan->subs_only = n;
      changed = 1;
    } else if (!strcmp(k, "slow") && tchan->slow != n) {
      tchan->slow = n;
      changed = 1;
    }
    if (changed) {
      putlog(LOG_SERV, "*", "* TWITCH: Roomstate '%s' in room %s changed to %s", k, chan, v);
      trigger_bind = 1;
    }
  }
  if (trigger_bind) {
    check_tcl_roomstate(chan, tags);
  }
  return 0;
}

static int gotusernotice(char *from, char *msg, Tcl_Obj *tags) {
  char *chan, login[NICKLEN], msgid[TOTALTAGMAX];
  char *value, *value2;

  chan = newsplit(&msg);
  fixcolon(msg);

  GET_MSGTAG_VALUE_STR(tags, "login", value, "USERNOTICE");
  strlcpy(login, value, sizeof login);

  GET_MSGTAG_VALUE_STR(tags, "msg-id", value, "USERNOTICE");
  strlcpy(msgid, value, sizeof msgid);

  if (!strcmp(msgid, "sub")) {
    GET_MSGTAG_VALUE_STR(tags, "msg-param-sub-plan", value, "USERNOTICE:SUB");
    putlog(LOG_SERV, "*", "* TWITCH: %s subscribed to the %s plan", login, value);
  } else if (!strcmp(msgid, "resub")) {
    GET_MSGTAG_VALUE_STR(tags, "msg-param-sub-plan", value, "USERNOTICE:RESUB");
    putlog(LOG_SERV, "*", "* TWITCH: %s re-subscribed to the %s plan", login, value);
  } else if (!strcmp(msgid, "subgift")) {
    GET_MSGTAG_VALUE_STR(tags, "msg-param-recipient-user-name", value, "USERNOTICE:SUBGIFT");
    GET_MSGTAG_VALUE_STR(tags, "msg-param-sub-plan", value2, "USERNOTICE:SUBGIFT");
    putlog(LOG_SERV, "*", "* TWITCH: %s gifted %s a subscription to the %s plan", login, value, value2);
  } else if (!strcmp(msgid, "anonsubgift")) {
    GET_MSGTAG_VALUE_STR(tags, "msg-param-recipient-user-name", value, "USERNOTICE:ANONSUBGIFT");
    GET_MSGTAG_VALUE_STR(tags, "msg-param-sub-plan", value2, "USERNOTICE:ANONSUBGIFT");
    putlog(LOG_SERV, "*", "* TWITCH: Someone gifted %s a subscription to the %s plan", value, value2);
  } else if (!strcmp(msgid, "submysterygift")) {
    putlog(LOG_SERV, "*", "* TWITCH: %s sent a mystery gift", login);
  } else if (!strcmp(msgid, "giftpaidupgrade")) {
    GET_MSGTAG_VALUE_STR(tags, "msg-param-recipient-user-name", value, "USERNOTICE:GIFTPAIDUPGRADE");
    putlog(LOG_SERV, "*", "* TWITCH: %s gifted a subsription upgrade to %s", login, value);
  } else if (!strcmp(msgid, "rewardgift")) {
    putlog(LOG_SERV, "*", "* TWITCH: %s sent a reward gift", login);
  } else if (!strcmp(msgid, "anongiftpaidupgrade")) {
    GET_MSGTAG_VALUE_STR(tags, "msg-param-recipient-user-name", value, "USERNOTICE:ANONGIFTPAIDUPGRADE");
    putlog(LOG_SERV, "*", "* TWITCH: Someone anonymously gifted a subscription upgrade to %s", value);
  } else if (!strcmp(msgid, "raid")) {
    GET_MSGTAG_VALUE_STR(tags, "msg-param-viewerCount", value, "USERNOTICE:RAID");
    putlog(LOG_SERV, "*", "* TWITCH: %s raided %s with %s users", login, chan, value);
  } else if (!strcmp(msgid, "unraid")) {
    putlog(LOG_SERV, "*", "* TWITCH: %s ended their raid on %s", login, chan);
  } else if (!strcmp(msgid, "ritual")) {
    GET_MSGTAG_VALUE_STR(tags, "msg-param-ritual-name", value, "USERNOTICE:RITUAL");
    putlog(LOG_SERV, "*", "* TWITCH: %s initiated a %s ritual", login, value);
  } else if (!strcmp(msgid, "bitsbadgetier")) {
    GET_MSGTAG_VALUE_STR(tags, "msg-param-threshold", value, "USERNOTICE:BITSBADGETIER");
    putlog(LOG_SERV, "*", "* TWITCH: %s earned a %s bits badge", login, value);
  }
  check_tcl_usernotice(chan, tags);
  return 0;
}

static int tcl_userstate STDVAR {
  twitchchan_t *tchan;
  char s [3];
  Tcl_DString usdict;

  BADARGS(2, 2, " chan");

  Tcl_DStringInit(&usdict);     /* Create a dict to capture userstate values */
  if (!(tchan = findtchan_by_dname(argv[1]))) {
    Tcl_AppendResult(irp, "No userstate found for channel", NULL);
    return TCL_ERROR;
  }
  Tcl_DStringAppendElement(&usdict, "badge-info");
  snprintf(s, sizeof s, "%d", tchan->userstate.badge_info);
  Tcl_DStringAppendElement(&usdict, s);
  Tcl_DStringAppendElement(&usdict, "badges");
  Tcl_DStringAppendElement(&usdict, tchan->userstate.badges);
  Tcl_DStringAppendElement(&usdict, "color");
  Tcl_DStringAppendElement(&usdict, tchan->userstate.color);
  Tcl_DStringAppendElement(&usdict, "display-name");
  Tcl_DStringAppendElement(&usdict, tchan->userstate.display_name);
  Tcl_DStringAppendElement(&usdict, "emote-sets");
  Tcl_DStringAppendElement(&usdict, tchan->userstate.emote_sets);
  Tcl_DStringAppendElement(&usdict, "mod");
  snprintf(s, sizeof s, "%d", tchan->userstate.mod);
  Tcl_DStringAppendElement(&usdict, s);
  
  Tcl_AppendResult(irp, Tcl_DStringValue(&usdict), NULL);
  Tcl_DStringFree(&usdict);
  return TCL_OK;
}

static int tcl_twitchmods STDVAR {
  twitchchan_t *tchan;

  BADARGS(2, 2, " chan");

  if (!(tchan = findtchan_by_dname(argv[1]))) {
    Tcl_AppendResult(irp, "No such channel", NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(irp, tchan->mods, NULL);
  return TCL_OK;
}

static int tcl_twitchvips STDVAR {
  twitchchan_t *tchan;

  BADARGS(2, 2, " chan");

  if (!(tchan = findtchan_by_dname(argv[1]))) {
    Tcl_AppendResult(irp, "No such channel", NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(irp, tchan->vips, NULL);
  return TCL_OK;
}

/* Checks if a user is a moderator or not. This differs from normal is* Tcl
 * cmds, as it does NOT check if the user is on the channel or not, as that
 * is unreliable on Twitch
 */
static int tcl_ismod STDVAR {
  twitchchan_t *tchan, *thechan=NULL;

  BADARGS(2, 3, " nick ?channel?");

  if (argc > 2) {
    tchan = findtchan_by_dname(argv[2]);
    thechan = tchan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else {
    tchan = twitchchan;
  }
  /* If there's no mods, no reason to even check, eh? */
  if (!strlen(tchan->mods)) {
    Tcl_AppendResult(irp, "0", NULL);
    return TCL_OK;
  }
  while (tchan && (thechan == NULL || thechan == tchan)) {
    if (strstr(tchan->mods, argv[1])) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
    tchan = tchan->next;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}


/* Checks if a user is a VIP or not. This differs from normal is* Tcl
 * cmds, as it does NOT check if the user is on the channel or not, as that
 * is unreliable on Twitch
 */
static int tcl_isvip STDVAR {
  twitchchan_t *tchan, *thechan = NULL;

  BADARGS(2, 3, " nick ?channel?");

  if (argc > 2) {
    tchan = findtchan_by_dname(argv[2]);
    thechan = tchan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else {
    tchan = twitchchan;
  }
  /* If there's no VIPs, no reason to even check, eh? */
  if (!strlen(tchan->vips)) {
    Tcl_AppendResult(irp, "0", NULL);
    return TCL_OK;
  }
  while (tchan && (thechan == NULL || thechan == tchan)) {
    if (strstr(tchan->vips, argv[1])) {
      Tcl_AppendResult(irp, "1", NULL);
      return TCL_OK;
    }
    tchan = tchan->next;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}


static int tcl_roomstate STDVAR {
  char s[5];
  twitchchan_t *tchan;
  Tcl_DString rsdict;

  BADARGS(2, 2, " chan");

  Tcl_DStringInit(&rsdict);     /* Create a dict to capture roomstate values */
  if (!(tchan = findtchan_by_dname(argv[1]))) {
    Tcl_AppendResult(irp, "No roomstate found for channel", NULL);
    return TCL_ERROR;
  }
  Tcl_DStringAppendElement(&rsdict, "emote-only");
  snprintf(s, sizeof s, "%d", tchan->emote_only);
  Tcl_DStringAppendElement(&rsdict, s);
  Tcl_DStringAppendElement(&rsdict, "followers-only");
  snprintf(s, sizeof s, "%d", tchan->followers_only);
  Tcl_DStringAppendElement(&rsdict, s);
  Tcl_DStringAppendElement(&rsdict, "r9k");
  snprintf(s, sizeof s, "%d", tchan->emote_only);
  Tcl_DStringAppendElement(&rsdict, s);
  Tcl_DStringAppendElement(&rsdict, "slow");
  snprintf(s, sizeof s, "%d", tchan->slow);
  Tcl_DStringAppendElement(&rsdict, s);
  Tcl_DStringAppendElement(&rsdict, "subs-only");
  snprintf(s, sizeof s, "%d", tchan->subs_only);
  Tcl_DStringAppendElement(&rsdict, s);

  Tcl_AppendResult(irp, Tcl_DStringValue(&rsdict), NULL);
  Tcl_DStringFree(&rsdict);
  return TCL_OK;
}


static int tcl_twcmd STDVAR {

  BADARGS(3, 4, " chan cmd ?arg?");

  if (argv[1][0] != '#') {
    Tcl_AppendResult(irp, "Invalid channel", NULL);
    return TCL_ERROR;
  }
  dprintf(DP_SERVER, "PRIVMSG %s :/%s %s", argv[1], argv[2],
      argc >= 4 && argv[3] ? argv[3] : "");
  return TCL_OK;
}


static int twitch_2char STDVAR
{
  Function F = (Function) cd;

  BADARGS(3, 3, " nick chan");

  CHECKVALIDITY(twitch_2char);
  F(argv[1], argv[2]);
  return TCL_OK;
}

static int twitch_3char STDVAR
{
  Function F = (Function) cd;

  BADARGS(4, 4, " from msg tags");

  CHECKVALIDITY(twitch_3char);
  F(argv[1], argv[2], argv[3]);
  return TCL_OK;
}

/* A report on the module status.
 *
 * details is either 0 or 1:
 *    0 - `.status'
 *    1 - `.status all'  or  `.module twitch'
 */
static void twitch_report(int idx, int details)
{
  if (details) {
    int size = twitch_expmem();

    dprintf(idx, "    Using %d byte%s of memory\n", size,
            (size != 1) ? "s" : "");
  }
}

static cmd_t mydcc[] = {
  /* command  flags  function     tcl-name */
  {"roomstate", "",     (IntFunc) cmd_roomstate,   NULL},
  {"userstate", "",     (IntFunc) cmd_userstate,   NULL},
  {"twcmd",     "o|o",  (IntFunc) cmd_twcmd,       NULL},
  {NULL,        NULL,   NULL,                      NULL}  /* Mark end. */
};

static tcl_cmds mytcl[] = {
  {"twcmd",           tcl_twcmd},
  {"roomstate",   tcl_roomstate},
  {"userstate",   tcl_userstate},
  {"twitchmods", tcl_twitchmods},
  {"twitchvips", tcl_twitchvips},
  {"ismod",           tcl_ismod},
  {"isvip",           tcl_isvip},
  {NULL,                   NULL}
};

/*
static tcl_ints my_tcl_ints[] = {
  {NULL,                NULL,                       0}
};
*/

static tcl_strings my_tcl_strings[] = {
  {"cap-request",   cap_request,    55,     STR_PROTECT},
  {NULL,            NULL,           0,                0}
};

static cmd_t twitch_raw[] = {
  {"CLEARCHAT",     "",     (IntFunc) gotclearchat,      "twitch:clearchat"},
  {"HOSTTARGET",    "",     (IntFunc) gothosttarget, "twitch:gothosttarget"},
  {"JOIN",          "",     (IntFunc) gotjoin,             "twitch:gotjoin"},
  {NULL,            NULL,   NULL,                                      NULL}
};

static cmd_t twitch_rawt[] = {
  {"CLEARMSG",   "",    (IntFunc) gotclearmsg,        "twitch:clearmsg"},
  {"ROOMSTATE",  "",    (IntFunc) gotroomstate,      "twitch:roomstate"},
  {"WHISPER",    "",    (IntFunc) gotwhisper,          "twitch:whisper"},
  {"USERSTATE",  "",    (IntFunc) gotuserstate,   "twitch:gotuserstate"},
  {"USERNOTICE", "",    (IntFunc) gotusernotice, "twitch:gotusernotice"},
  {"NOTICE",     "",    (IntFunc) gotnotice,         "twitch:gotnotice"},
  {NULL,         NULL,  NULL,                                      NULL}
};


static char *twitch_close()
{
  rem_builtins(H_dcc, mydcc);
  rem_builtins(H_raw, twitch_raw);
  rem_builtins(H_rawt, twitch_rawt);
  rem_tcl_commands(mytcl);
  // rem_tcl_ints(my_tcl_ints);
  rem_tcl_strings(my_tcl_strings);
  del_bind_table(H_ccht);
  del_bind_table(H_cmsg);
  del_bind_table(H_htgt);
  del_bind_table(H_wspr);
  del_bind_table(H_wspm);
  del_bind_table(H_rmst);
  del_bind_table(H_usst);
  del_bind_table(H_usrntc);
  Tcl_UntraceVar(interp, "keep-nick", TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS, traced_keepnick, NULL);
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *twitch_start();

static Function twitch_table[] = {
  (Function) twitch_start,
  (Function) twitch_close,
  (Function) twitch_expmem,
  (Function) twitch_report,
  (Function) & H_ccht,
  (Function) & H_cmsg,
  (Function) & H_htgt,
  (Function) & H_wspr,
  (Function) & H_wspm,
  (Function) & H_rmst,
  (Function) & H_usst,
  (Function) & H_usrntc
};

char *twitch_start(Function *global_funcs)
{
  const char *value;

  /* Assign the core function table. After this point you use all normal
   * functions defined in src/mod/modules.h
   */
  global = global_funcs;

  /* Register the module. */
  module_register(MODULE_NAME, twitch_table, 0, 1);

  if (!module_depend(MODULE_NAME, "eggdrop", 109, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.9.0 or later.";
  }
  if (!(server_funcs = module_depend(MODULE_NAME, "server", 1, 5))) {
    module_undepend(MODULE_NAME);
    return "This module requires server module 1.5 or later.";
  }
/*
  if (!(server_funcs = module_depend(MODULE_NAME, "irc", 1, 5))) {
    module_undepend(MODULE_NAME);
    return "This module requires irc module 1.5 or later.";
  }
  if (!(server_funcs = module_depend(MODULE_NAME, "channels", 1, 1))) {
    module_undepend(MODULE_NAME);
    return "This module requires channels module 1.1 or later.";
  }
*/

/* Yes, we could hard-code net-type as well, but let's make sure the user isn't
 * loading the Twitch module by accident- it will really dork up a normal IRC
 * server and would be difficult to troubleshoot with a user.
 */
  if (net_type_int != NETT_TWITCH) {
    fatal("ERROR: ATTEMPTED TO LOAD TWITCH MODULE WITH INCORRECT NET-TYPE SET\n"
          "  Please check that net-type is set to twitch in config before loadmodule twitch and try again", 0);
  }

  H_ccht = add_bind_table("ccht", HT_STACKABLE, twitch_2char);
  H_cmsg = add_bind_table("cmsg", HT_STACKABLE, twitch_3char);
  H_htgt = add_bind_table("htgt", HT_STACKABLE, twitch_2char);
  H_wspr = add_bind_table("wspr", HT_STACKABLE, twitch_3char);
  H_wspm = add_bind_table("wspm", HT_STACKABLE, twitch_3char);
  H_rmst = add_bind_table("rmst", HT_STACKABLE, twitch_3char);
  H_usst = add_bind_table("usst", HT_STACKABLE, twitch_3char);
  H_usrntc = add_bind_table("usrntc", HT_STACKABLE, twitch_3char);

  /* Override config setting with these values; they are required for Twitch */
  Tcl_SetVar(interp, "cap-request",
        "twitch.tv/commands twitch.tv/membership twitch.tv/tags", 0);
  /* keep-nick causes ISONs to be sent, which are not supported */
  if ((value = Tcl_GetVar2(interp, "keep-nick", NULL, TCL_GLOBAL_ONLY)) && strcmp(value, "0")) {
    putlog(LOG_MISC, "*", "Twitch: keep-nick is forced to be 0 when twitch.mod is loaded");
  }
  Tcl_SetVar2(interp, "keep-nick", NULL, "0", TCL_GLOBAL_ONLY);
  Tcl_TraceVar(interp, "keep-nick", TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS, traced_keepnick, NULL);

  /* Add command table to bind list */
  add_builtins(H_dcc, mydcc);
  add_builtins(H_raw, twitch_raw);
  add_builtins(H_rawt, twitch_rawt);
  add_tcl_commands(mytcl);
  // add_tcl_ints(my_tcl_ints);
  add_tcl_strings(my_tcl_strings);
  return NULL;
}
