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
 *   You should also look at loading scripts/twitch.tcl, which adds the ability
 *   to track users (and their statuses, like moderator) on a channel.
 *
 * Originally written by Geo              April 2020
 */

/*
 * Copyright (C) 2020 Eggheads Development Team
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

static p_tcl_bind_list H_ccht, H_cmsg, H_htgt, H_wspr, H_rmst, H_usst;

struct twitchchan_t *twitchchan = NULL;

/* Calculate the memory we keep allocated.
 */
static int twitch_expmem()
{
  int size = 0;

  Context;
  return size;
}

/* Find a twitch channel by it's display name */
struct twitchchan_t *findtchan_by_dname(char *name)
{
  struct twitchchan_t *chan;

  for (chan = twitchchan; chan; chan = chan->next)
    if (!rfc_casecmp(chan->dname, name))
      return chan;
  return NULL;
}

static int cmd_roomstate(struct userrec *u, int idx, char *par) {
  struct twitchchan_t *tchan;

  if (!(tchan = findtchan_by_dname(par))) { /* Search for channel */
    dprintf(idx, "No such channel.\n");
    return 1;
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
  return 0;
}

static int cmd_userstate(struct userrec *u, int idx, char *par) {
  struct twitchchan_t *tchan;

  if (!(tchan = findtchan_by_dname(par))) { /* Search for channel */
    dprintf(idx, "No such channel.\n");
    return 1;
  }
  putlog(LOG_CMDS, "*", "#%s# userstate", dcc[idx].nick);
  dprintf(idx, "Userstate for %s:\n", tchan->dname);
  dprintf(idx, "---------------------------------\n");
  dprintf(idx, "Display Name: %s\n", tchan->userstate.display_name);
  dprintf(idx, "Badges:       %s\n", tchan->userstate.badges);
  dprintf(idx, "Badge Info:   %d\n", tchan->userstate.badge_info);
  dprintf(idx, "Color:        %s\n", tchan->userstate.color);
  dprintf(idx, "Emote-Sets:   %s\n", tchan->userstate.emote_sets);
  dprintf(idx, "Moderator:    %d\n", tchan->userstate.mod);
  dprintf(idx, "End of userstate info.\n");
  return 0;
}


/* Takes a space-seperated string (key/value pair), makes a copy of the it
 * (since we're going to muck with it) and returns a pointer to the value
 * associated with the key provided.
 */
char *get_value(char *dict, char *key) {
  char *ptr, s[8092];
  strcpy(s, dict);
  ptr = strstr(s, key);                  /* Get ptr to key */
    if (!ptr) {
      return NULL;
    }
  strtok(ptr, " ");                      /* Move to value */
    if (!ptr) {
      return NULL;
    }
  return strtok(NULL, " ");              /* Return null-term'd value for key */
}

static int check_tcl_clearchat(char *chan, char *nick) {
  int x;
  char mask[1024];
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  snprintf(mask, sizeof mask, "%s %s", chan, nick);
  Tcl_SetVar(interp, "_ccht1", chan, 0);
  Tcl_SetVar(interp, "_ccht2", nick ? (char *) nick : "", 0);
  x = check_tcl_bind(H_ccht, mask, &fr, " $_ccht1, $_ccht2",
        MATCH_MASK | BIND_STACKABLE);
  return (x == BIND_EXEC_LOG);
}

static int check_tcl_clearmsg(char *chan, char *nick, char *msgid, char *msg) {
  int x;
  char mask[1024];
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  snprintf(mask, sizeof mask, "%s %s", chan, nick);
  Tcl_SetVar(interp, "_cmsg1", chan, 0);
  Tcl_SetVar(interp, "_cmsg2", nick, 0);
  Tcl_SetVar(interp, "_cmsg3", msgid, 0);
  Tcl_SetVar(interp, "_cmsg4", msg, 0);
  x = check_tcl_bind(H_cmsg, mask, &fr, " $_cmsg1, $_cmsg2, $_cmsg3, $_cmsg4",
        MATCH_MASK | BIND_STACKABLE);
  return (x == BIND_EXEC_LOG);
}

static int check_tcl_hosttarget(char *chan, char *nick, char *viewers) {
  int x;
  char mask[1024];
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  snprintf(mask, sizeof mask, "%s %s", chan, nick);
  Tcl_SetVar(interp, "_htgt1", chan, 0);
  Tcl_SetVar(interp, "_htgt2", nick, 0);
  Tcl_SetVar(interp, "_htgt3", viewers, 0);
  x = check_tcl_bind(H_htgt, mask, &fr, " $_htgt1, $_htgt2, $_htgt3",
        MATCH_MASK | BIND_STACKABLE);

  return (x == BIND_EXEC_LOG);
}

static int check_tcl_whisper(char *from, char *msg) {
  char buf[UHOSTMAX], *uhost=buf, *nick, mask[1024];
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };
  struct userrec *u;
  int x;

  strlcpy(uhost, from, sizeof uhost);
  nick = splitnick(&uhost);
  u = get_user_by_host(from);
  char *hand = u ? u->handle : "*";
  Tcl_SetVar(interp, "_wspr1", nick, 0);
  Tcl_SetVar(interp, "_wspr2", uhost, 0);
  Tcl_SetVar(interp, "_wspr3", hand, 0);
  Tcl_SetVar(interp, "_wspr4", msg, 0);
  x = check_tcl_bind(H_wspr, mask, &fr, " $_wspr1, $_wspr2, $_wspr3, $_wspr4",
        MATCH_MASK | BIND_STACKABLE);
  return (x == BIND_EXEC_LOG);
}

static int check_tcl_roomstate(char *chan, char *tags) {
  int x;

  Tcl_SetVar(interp, "_rmst1", chan, 0);
  Tcl_SetVar(interp, "_rmst2", tags, 0);
  x = check_tcl_bind(H_rmst, 0, 0, " $_rmst1, $_rmst2",
        MATCH_MASK | BIND_STACKABLE);
  return (x == BIND_EXEC_LOG);
}

static int check_tcl_userstate(char *chan, char *tags) {
  int x;

  Tcl_SetVar(interp, "_usst1", chan, 0);
  Tcl_SetVar(interp, "_usst2", tags, 0);
  x = check_tcl_bind(H_usst, 0, 0, " $_usst1, $_usst2",
        MATCH_MASK | BIND_STACKABLE);
  return (x == BIND_EXEC_LOG);
}

static int gotwhisper(char *from, char *msg, char *tags) {
  newsplit(&msg);    /* Get rid of my own nick */
  fixcolon(msg);
  putlog(LOG_MSGS, "*", "[%s] %s", from, msg);
  check_tcl_whisper(from, msg);
  return 0; 
}

static int gotclearmsg(char *from, char *msg, char *tags) {
  char *nick, *chan, *msgid;
  
  chan = newsplit(&msg);
  fixcolon(msg);
  nick = get_value(tags, "login");
  msgid = get_value(tags, "target-msg-id");
  putlog(LOG_SERV, "*", "* TWITCH: Cleared message %s from %s", msgid, nick);
  check_tcl_clearmsg(chan, nick, msgid, msg);
  return 0;
}

static int gotclearchat(char *from, char *msg) {
  char *nick=NULL, *chan=NULL;

putlog(LOG_DEBUG, "*", "TWITCH: from is %s msg is %s", from, msg);
  chan = newsplit(&msg);
  fixcolon(msg);
  nick = newsplit(&msg);
  if (!strlen(nick)) {
    putlog(LOG_SERV, "*", "* TWITCH: Chat logs cleared on %s", chan);
  } else {
    putlog(LOG_SERV, "*", "* TWITCH: Chat logs cleared on %s for user %s", chan, nick);
  }
  check_tcl_clearchat(chan, nick);
  return 0;
}

static int gothosttarget(char *from, char *msg) {
  char s[30], *nick, *chan, *viewers;

putlog(LOG_DEBUG, "*", "TWITCH: hosttarget from is %s msg is %s", msg);
  chan = newsplit(&msg);
  fixcolon(msg);
  nick = newsplit(&msg);
  viewers = newsplit(&msg);
  if (viewers) {
    sprintf(s, " (Viewers: %s)", viewers);
  }
  if (nick[0] == '-') {             /* Check if it is an unhost */
    putlog(LOG_SERV, "*", "* TWITCH: %s has stopped host mode.", chan);
  } else {   
    putlog(LOG_SERV, "*", "* TWITCH: %s has started hosting %s%s",
            chan, nick, (viewers) ? s : "");
  }
  check_tcl_hosttarget(chan, nick, viewers);
  return 0;
}

static int gotuserstate(char *from, char *chan, char *tags) {
  struct twitchchan_t *tchan;
  char *ptr, s[TOTALTAGMAX];
  
  if (!(tchan = findtchan_by_dname(chan))) {    /* Find channel or, if it   */
    tchan = nmalloc(sizeof *tchan);             /* doesn't exist, create it */
    explicit_bzero(tchan, sizeof(struct twitchchan_t));
    strlcpy(tchan->dname, chan, sizeof tchan->dname);
    egg_list_append((struct list_type **) &twitchchan, (struct list_type *) tchan);
  }
  strcpy(s, tags);
  ptr = strtok(s, " ");
  while (ptr != NULL) {
    if (!strcmp(ptr, "badge-info")) {
      ptr = strtok(NULL, " ");
      tchan->userstate.badge_info = atol(ptr);
    }
    if (!strcmp(ptr, "badges")) {
      ptr = strtok(NULL, " ");
      strlcpy(tchan->userstate.badges, ptr,
            sizeof tchan->userstate.badges);
    }
    if (!strcmp(ptr, "color")) {
      ptr = strtok(NULL, " ");
      strlcpy(tchan->userstate.color, ptr,
            sizeof tchan->userstate.display_name);
    }
    if (!strcmp(ptr, "display-name")) {
      ptr = strtok(NULL, " ");
      strlcpy(tchan->userstate.display_name, ptr,
            sizeof tchan->userstate.display_name);
    }
    if (!strcmp(ptr, "emote-sets")) {
      ptr = strtok(NULL, " ");
      strlcpy(tchan->userstate.emote_sets, ptr,
            sizeof tchan->userstate.emote_sets);
    }
    if (!strcmp(ptr, "mod")) {
      ptr = strtok(NULL, " ");
      tchan->userstate.mod = atol(ptr);
    }
    ptr = strtok(NULL, " ");
  }
  check_tcl_userstate(chan, tags);
  return 0;
}

static int gotroomstate(char *from, char *msg, char *tags) {
  char *channame, *ptr;
  char s[TOTALTAGMAX];
  struct twitchchan_t *chan;

  channame = newsplit(&msg);
  if (!(chan = findtchan_by_dname(channame))) {  /* Find channel or, if it   */
    chan = nmalloc(sizeof *chan);                /* doesn't exist, create it */
    explicit_bzero(chan, sizeof(struct twitchchan_t));
    strlcpy(chan->dname, channame, sizeof chan->dname);
    egg_list_append((struct list_type **) &twitchchan, (struct list_type *) chan);
  }
  strcpy(s, tags);
  ptr = strtok(s, " ");
  while (ptr != NULL) {                   /* Go through the tag-msg and upate */
    if (!strcmp(ptr, "emote-only")) {     /* roomstate values present         */
      ptr = strtok(NULL, " ");
      if (chan->emote_only != atol(ptr)) {
        putlog(LOG_SERV, "*", "* TWITCH: Roomstate 'emote-only' changed to %s",
            ptr);
        chan->emote_only = atol(ptr);
      }
    }
    if (!strcmp(ptr, "followers-only")) {
      ptr = strtok(NULL, " ");
      if (chan->followers_only != atol(ptr)) {
        putlog(LOG_SERV, "*", "* TWITCH: Roomstate 'followers-only' changed to %s",
            ptr);
        chan->followers_only = atol(ptr);
      }
    }
    if (!strcmp(ptr, "r9k")) {
      ptr = strtok(NULL, " ");
      if (chan->r9k != atol(ptr)) {
        putlog(LOG_SERV, "*", "* TWITCH: Roomstate 'r9k' changed to %s",
            ptr);
        chan->r9k = atol(ptr);
      }
    }
    if (!strcmp(ptr, "subs-only")) {
      ptr = strtok(NULL, " ");
      if (chan->subs_only != atol(ptr)) {
        putlog(LOG_SERV, "*", "* TWITCH: Roomstate 'subs-only' changed to %s",
            ptr);
        chan->subs_only = atol(ptr);
      }
    }
    if (!strcmp(ptr, "slow")) {
      ptr = strtok(NULL, " ");
      if (chan->slow != atol(ptr)) {
        putlog(LOG_SERV, "*", "* TWITCH: Roomstate 'slow' changed to %s",
            ptr);
        chan->slow = atol(ptr);
      }
    }
    check_tcl_roomstate(channame, tags);
    ptr = strtok(NULL, " ");
  }
  return 0;
}

static int gotusernotice(char *from, char *msg, char *tags) {
  char *chan, *login, *msgid;

  chan = newsplit(&msg);
  fixcolon(msg);
  login = get_value(tags, "login");
  msgid = get_value(tags, "msg-id");
  if (!strcmp(msgid, "sub")) {
    putlog(LOG_SERV, "*", "* TWITCH: %s subscribed to the %s plan", login,
        get_value(tags, "msg-param-sub-plan"));
  } else if (!strcmp(msgid, "resub")) {
    putlog(LOG_SERV, "*", "* TWITCH: %s re-subscribed to the %s plan", login,
        get_value(tags, "msg-param-sub-plan")); 
  } else if (!strcmp(msgid, "subgift")) {
    putlog(LOG_SERV, "*", "* TWITCH: %s gifted %s a subscription to the %s "
        "plan", login, get_value(tags, "msg-param-recipient-user-name"),
        get_value(tags, "msg-param-sub-plan"));
  } else if (!strcmp(msgid, "anonsubgift")) {
    putlog(LOG_SERV, "*", "* TWITCH: Someone gifted %s a subscription to the "
        "%s plan", get_value(tags, "msg-param-recipient-user-name"),
        get_value(tags, "msg-param-sub-plan"));
  } else if (!strcmp(msgid, "submysterygift")) {
    putlog(LOG_SERV, "*", "* TWITCH: %s sent a mystery gift %s", login,
        get_value(tags, "msg-param-recipient-user-name"));
  } else if (!strcmp(msgid, "giftpaidupgrade")) {
    putlog(LOG_SERV, "*", "* TWITCH: %s gifted a subsription upgrade to %s",
        login, get_value(tags, "msg-param-recipient-user-name"));
  } else if (!strcmp(msgid, "rewardgift")) {
    putlog(LOG_SERV, "*", "* TWITCH: %s sent a reward gift", login);
  } else if (!strcmp(msgid, "anongiftpaidupgrade")) {
    putlog(LOG_SERV, "*", "* TWITCH: Someone anonomously gifted a subscription "
        "upgrade to %s", get_value(tags, "msg-param-recipient-user-name"));
  } else if (!strcmp(msgid, "raid")) {
    putlog(LOG_SERV, "*", "* TWITCH: %s raided %s with %s users", login, chan,
        get_value(tags, "msg-param-viewerCount"));
  } else if (!strcmp(msgid, "unraid")) {
    putlog(LOG_SERV, "*", "* TWITCH: %s ended their raid on %s", login, chan);
  } else if (!strcmp(msgid, "ritual")) {
    putlog(LOG_SERV, "*", "* TWITCH: %s initiated a %s ritual", login,
        get_value(tags, "msg-param-ritual-name"));
  } else if (!strcmp(msgid, "bitsbadgetier")) {
    putlog(LOG_SERV, "*", "* TWITCH: %s earned a %s bits badge", login,
        get_value(tags, "msg-param-threshold"));
  }
  return 0;
}

static int tcl_userstate STDVAR {
  struct twitchchan_t *tchan;
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


static int tcl_roomstate STDVAR {
  char s[5];
  struct twitchchan_t *tchan;
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

  BADARGS(3, 4, " cmd arg");

  dprintf(DP_SERVER, "PRIVMSG %s :/%s %s", argv[2], argv[1]);
  return 0;
}


static int twitch_2char STDVAR
{
  Function F = (Function) cd;

  BADARGS(3, 3, " nick chan");

  CHECKVALIDITY(twitch_2char);
  F(argv[1], argv[2]);
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
  {"roomstate", "",     cmd_roomstate,   NULL},
  {"userstate", "",     cmd_userstate,   NULL},
  {NULL,        NULL,   NULL,            NULL}  /* Mark end. */
};

static tcl_cmds mytcl[] = {
  {"twcmd", tcl_twcmd},
  {"roomstate", tcl_roomstate},
  {"userstate", tcl_userstate},
  {NULL,        NULL}
};

static cmd_t twitch_raw[] = {
  {"CLEARCHAT",     "",     (IntFunc) gotclearchat, "twitch:clearchat"},
  {"HOSTTARGET",    "",     (IntFunc) gothosttarget,"twitch:gothosttarget"},
  {NULL,            NULL,   NULL,                   NULL}
};

static cmd_t twitch_rawt[] = {
  {"CLEARMSG",  "",     (IntFunc) gotclearmsg,  "twitch:clearmsg"},
  {"ROOMSTATE", "",     (IntFunc) gotroomstate, "twitch:roomstate"},
  {"WHISPER",   "",     (IntFunc) gotwhisper,   "twitch:whisper"},
  {"USERSTATE", "",     (IntFunc) gotuserstate, "twitch:gotuserstate"},
  {"USERNOTICE","",     (IntFunc) gotusernotice,"twitch:gotusernotice"},
  {NULL,        NULL,   NULL,                   NULL}
};


static char *twitch_close()
{
  Context;
  rem_builtins(H_dcc, mydcc);
  rem_builtins(H_raw, twitch_raw);
  rem_builtins(H_rawt, twitch_rawt);
  rem_tcl_commands(mytcl);
  del_bind_table(H_ccht);
  del_bind_table(H_cmsg);
  del_bind_table(H_htgt);
  del_bind_table(H_wspr);
  del_bind_table(H_rmst);
  del_bind_table(H_usst);
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
  (Function) & H_rmst,
  (Function) & H_usst
};

char *twitch_start(Function *global_funcs)
{
  /* Assign the core function table. After this point you use all normal
   * functions defined in src/mod/modules.h
   */
  global = global_funcs;

  Context;
  /* Register the module. */
  module_register(MODULE_NAME, twitch_table, 0, 1);

  if (!module_depend(MODULE_NAME, "eggdrop", 108, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.8.0 or later.";
  }
  if (!(server_funcs = module_depend(MODULE_NAME, "server", 1, 5))) {
    module_undepend(MODULE_NAME);
    return "This module requires server module 1.5 or later.";
  }

  H_ccht = add_bind_table("ccht", HT_STACKABLE, twitch_2char);
  H_cmsg = add_bind_table("cmsg", HT_STACKABLE, twitch_2char);
  H_htgt = add_bind_table("htgt", HT_STACKABLE, twitch_2char);
  H_wspr = add_bind_table("wspr", HT_STACKABLE, twitch_2char);
  H_rmst = add_bind_table("rmst", HT_STACKABLE, twitch_2char);
  H_usst = add_bind_table("usst", HT_STACKABLE, twitch_2char);

  /* Add command table to bind list */
  add_builtins(H_dcc, mydcc);
  add_builtins(H_raw, twitch_raw);
  add_builtins(H_rawt, twitch_rawt);
  add_tcl_commands(mytcl);
  return NULL;
}
