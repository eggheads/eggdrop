/*
 * tcldcc.c -- handles:
 *   Tcl stubs for the dcc commands
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2021 Eggheads Development Team
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
#include <errno.h>
#include <signal.h>

extern Tcl_Interp *interp;
extern tcl_timer_t *timer, *utimer;
extern struct dcc_t *dcc;
extern char botnetnick[], listen_ip[];
extern int dcc_total, backgrd, parties, make_userfile, remote_boots, max_dcc,
           conmask;
#ifdef IPV6
extern int pref_af;
#endif
extern volatile sig_atomic_t do_restart;
#ifdef TLS
extern int tls_vfydcc;
extern sock_list *socklist;
#endif
extern party_t *party;
extern tand_t *tandbot;
extern time_t now;
extern unsigned long otraffic_irc, otraffic_irc_today, itraffic_irc,
                     itraffic_irc_today, otraffic_bn, otraffic_bn_today,
                     itraffic_bn, itraffic_bn_today, otraffic_dcc,
                     otraffic_dcc_today, itraffic_dcc, itraffic_dcc_today,
                     otraffic_trans, otraffic_trans_today, itraffic_trans,
                     itraffic_trans_today, otraffic_unknown, itraffic_unknown,
                     otraffic_unknown_today, itraffic_unknown_today;
static struct portmap *root = NULL;


int expmem_tcldcc(void)
{
  int tot = 0;
  struct portmap *pmap;

  for (pmap = root; pmap; pmap = pmap->next)
    tot += sizeof(struct portmap);

  return tot;
}

static int tcl_putdcc STDVAR
{
  int j;

  BADARGS(3, 4, " idx text ?options?");

  if ((argc == 4) && strcasecmp(argv[3], "-raw")) {
    Tcl_AppendResult(irp, "unknown putdcc option: should be ",
                     "-raw", NULL);
    return TCL_ERROR;
  }

  j = findidx(atoi(argv[1]));
  if (j < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (argc == 4)
    tputs(dcc[j].sock, argv[2], strlen(argv[2]));
  else
    dumplots(j, "", argv[2]);

  return TCL_OK;
}

/* Allows tcl scripts to send out raw data. Can be used for fast server write.
 * The server idx is 0.
 *
 * Usage: putdccraw <idx> <size> <rawdata>
 *
 * Example: putdccraw 6 13 "eggdrop rulz\n"
 *
 * Added by <drummer@sophia.jpte.hu>.
 */
static int tcl_putdccraw STDVAR
{
#if 0
  int i, j = 0, z;

  BADARGS(4, 4, " idx size text");

  z = atoi(argv[1]);
  for (i = 0; i < dcc_total; i++) {
    if (!z && !strcmp(dcc[i].nick, "(server)")) {
      j = dcc[i].sock;
      break;
    } else if (dcc[i].sock == z) {
      j = dcc[i].sock;
      break;
    }
  }
  if (i == dcc_total) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  tputs(j, argv[3], atoi(argv[2]));
  return TCL_OK;
#endif

  Tcl_AppendResult(irp, "putdccraw is deprecated. "
                   "Please use putdcc/putnow instead.", NULL);
  return TCL_ERROR;
}

static int tcl_dccsimul STDVAR
{
  int idx;

  BADARGS(3, 3, " idx command");

  idx = findidx(atoi(argv[1]));
  if (idx >= 0 && (dcc[idx].type->flags & DCT_SIMUL)) {
    int l = strlen(argv[2]);

    if (l > 510) {
      l = 510;
      argv[2][510] = 0;        /* Restrict length of cmd */
    }
    if (dcc[idx].type && dcc[idx].type->activity) {
      dcc[idx].type->activity(idx, argv[2], l);
      return TCL_OK;
    }
  } else {
    Tcl_AppendResult(irp, "invalid idx", NULL);
  }
  return TCL_ERROR;
}


static int tcl_dccbroadcast STDVAR
{
  char msg[401];

  BADARGS(2, 2, " message");

  strlcpy(msg, argv[1], sizeof msg);
  chatout("*** %s\n", msg);
  botnet_send_chat(-1, botnetnick, msg);
  check_tcl_bcst(botnetnick, -1, msg);
  return TCL_OK;
}

static int tcl_hand2idx STDVAR
{
  int i;
  char s[11];

  BADARGS(2, 2, " handle");

  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type->flags & (DCT_SIMUL | DCT_BOT)) &&
        !strcasecmp(argv[1], dcc[i].nick)) {
      egg_snprintf(s, sizeof s, "%ld", dcc[i].sock);
      Tcl_AppendResult(irp, s, NULL);
      return TCL_OK;
    }
  Tcl_AppendResult(irp, "-1", NULL);
  return TCL_OK;
}

static int tcl_getchan STDVAR
{
  char s[7];
  int idx;

  BADARGS(2, 2, " idx");

  idx = findidx(atoi(argv[1]));
  if (idx < 0 || (dcc[idx].type != &DCC_CHAT &&
      dcc[idx].type != &DCC_SCRIPT)) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }

  if (dcc[idx].type == &DCC_SCRIPT)
    egg_snprintf(s, sizeof s, "%d", dcc[idx].u.script->u.chat->channel);
  else
    egg_snprintf(s, sizeof s, "%d", dcc[idx].u.chat->channel);

  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_setchan STDVAR
{
  int idx, chan;
  module_entry *me;

  BADARGS(3, 3, " idx channel");

  idx = findidx(atoi(argv[1]));
  if (idx < 0 || (dcc[idx].type != &DCC_CHAT &&
      dcc[idx].type != &DCC_SCRIPT)) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (argv[2][0] < '0' || argv[2][0] > '9') {
    if (!strcmp(argv[2], "-1") || !strcasecmp(argv[2], "off"))
      chan = -1;
    else {
      Tcl_SetVar(irp, "_chan", argv[2], 0);
      if (Tcl_VarEval(irp, "assoc ", "$_chan", NULL) != TCL_OK ||
          tcl_resultempty()) {
        Tcl_AppendResult(irp, "channel name is invalid", NULL);
        return TCL_ERROR;
      }
      chan = tcl_resultint();
    }
  } else
    chan = atoi(argv[2]);
  if ((chan < -1) || (chan > 199999)) {
    Tcl_AppendResult(irp, "channel out of range; must be -1 through 199999",
                     NULL);
    return TCL_ERROR;
  }
  if (dcc[idx].type == &DCC_SCRIPT)
    dcc[idx].u.script->u.chat->channel = chan;
  else {
    int oldchan = dcc[idx].u.chat->channel;

    if (dcc[idx].u.chat->channel >= 0) {
      if ((chan >= GLOBAL_CHANS) && (oldchan < GLOBAL_CHANS))
        botnet_send_part_idx(idx, "*script*");
      check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock,
                     dcc[idx].u.chat->channel);
    }
    dcc[idx].u.chat->channel = chan;
    if (chan < GLOBAL_CHANS)
      botnet_send_join_idx(idx, oldchan);
    check_tcl_chjn(botnetnick, dcc[idx].nick, chan, geticon(idx),
                   dcc[idx].sock, dcc[idx].host);
  }
  /* Console autosave. */
  if ((me = module_find("console", 1, 1))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (idx);
  }
  return TCL_OK;
}

static int tcl_dccputchan STDVAR
{
  int chan;
  char msg[401];

  BADARGS(3, 3, " channel message");

  chan = atoi(argv[1]);
  if ((chan < 0) || (chan > 199999)) {
    Tcl_AppendResult(irp, "channel out of range; must be 0 through 199999", NULL);
    return TCL_ERROR;
  }
  strlcpy(msg, argv[2], sizeof msg);

  chanout_but(-1, chan, "*** %s\n", argv[2]);
  botnet_send_chan(-1, botnetnick, NULL, chan, argv[2]);
  check_tcl_bcst(botnetnick, chan, argv[2]);
  return TCL_OK;
}

static int tcl_do_console(Tcl_Interp *irp, ClientData cd, int argc,
    char **argv, int reset)
{
  int i, j, pls, arg;
  module_entry *me;

  i = findidx(atoi(argv[1]));
  if (i < 0 || dcc[i].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  pls = 1;

  for (arg = 2; arg < argc; arg++) {
    if (argv[arg][0] && !reset && ((strchr(CHANMETA, argv[arg][0]) 
        != NULL) || (argv[arg][0] == '*'))) {
      if ((argv[arg][0] != '*') && (!findchan_by_dname(argv[arg]))) {
        /* If we don't find the channel, and it starts with a +, assume it
         * should be the console flags to set. */
        if (argv[arg][0] == '+')
          goto do_console_flags;
        Tcl_AppendResult(irp, "invalid channel", NULL);
        return TCL_ERROR;
      }
      strlcpy(dcc[i].u.chat->con_chan, argv[arg], sizeof dcc[i].u.chat->con_chan);
    } else {
      if (!reset && (argv[arg][0] != '+') && (argv[arg][0] != '-'))
        dcc[i].u.chat->con_flags = 0;
    do_console_flags:
      if (reset) {
        dcc[i].u.chat->con_flags =
            (dcc[i].user && (dcc[i].user->flags & USER_MASTER) ? conmask : 0);
      } else {
        for (j = 0; j < strlen(argv[arg]); j++) {
          if (argv[arg][j] == '+')
            pls = 1;
          else if (argv[arg][j] == '-')
            pls = -1;
          else {
            char s[2];

            s[0] = argv[arg][j];
            s[1] = 0;
            if (pls == 1)
              dcc[i].u.chat->con_flags |= logmodes(s);
            else
              dcc[i].u.chat->con_flags &= ~logmodes(s);
          }
        }
      }
    }
  }
  Tcl_AppendElement(irp, dcc[i].u.chat->con_chan);
  Tcl_AppendElement(irp, masktype(dcc[i].u.chat->con_flags));
  /* Console autosave. */
  if (argc > 2 && (me = module_find("console", 1, 1))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (i);
  }
  return TCL_OK;
}

static int tcl_console STDVAR
{
  BADARGS(2, 4, " idx ?channel? ?console-modes?");
  return tcl_do_console(irp, cd, argc, argv, 0);
}

static int tcl_resetconsole STDVAR
{
  BADARGS(2, 2, " idx");
  return tcl_do_console(irp, cd, argc, argv, 1);
}


static int tcl_strip STDVAR
{
  int i, j, pls, arg;
  module_entry *me;

  BADARGS(2, 4, " idx ?strip-flags?");

  i = findidx(atoi(argv[1]));
  if (i < 0 || dcc[i].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  pls = 1;

  for (arg = 2; arg < argc; arg++) {
    if ((argv[arg][0] != '+') && (argv[arg][0] != '-'))
      dcc[i].u.chat->strip_flags = 0;
    for (j = 0; j < strlen(argv[arg]); j++) {
      if (argv[arg][j] == '+')
        pls = 1;
      else if (argv[arg][j] == '-')
        pls = -1;
      else {
        char s[2];

        s[0] = argv[arg][j];
        s[1] = 0;
        if (pls == 1)
          dcc[i].u.chat->strip_flags |= stripmodes(s);
        else
          dcc[i].u.chat->strip_flags &= ~stripmodes(s);
      }
    }
  }
  Tcl_AppendElement(irp, stripmasktype(dcc[i].u.chat->strip_flags));
  /* Console autosave. */
  if (argc > 2 && (me = module_find("console", 1, 1))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (i);
  }
  return TCL_OK;
}

static int tcl_echo STDVAR
{
  int i;
  module_entry *me;

  BADARGS(2, 3, " idx ?status?");

  i = findidx(atoi(argv[1]));
  if (i < 0 || dcc[i].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (argc == 3) {
    if (atoi(argv[2]))
      dcc[i].status |= STAT_ECHO;
    else
      dcc[i].status &= ~STAT_ECHO;
  }
  if (dcc[i].status & STAT_ECHO)
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  /* Console autosave. */
  if (argc > 2 && (me = module_find("console", 1, 1))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (i);
  }
  return TCL_OK;
}

static int tcl_page STDVAR
{
  int i;
  char x[20];
  module_entry *me;

  BADARGS(2, 3, " idx ?status?");

  i = findidx(atoi(argv[1]));
  if (i < 0 || dcc[i].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (argc == 3) {
    int l = atoi(argv[2]);

    if (!l)
      dcc[i].status &= ~STAT_PAGE;
    else {
      dcc[i].status |= STAT_PAGE;
      dcc[i].u.chat->max_line = l;
    }
  }
  if (dcc[i].status & STAT_PAGE) {
    egg_snprintf(x, sizeof x, "%d", dcc[i].u.chat->max_line);
    Tcl_AppendResult(irp, x, NULL);
  } else
    Tcl_AppendResult(irp, "0", NULL);
  /* Console autosave. */
  if ((argc > 2) && (me = module_find("console", 1, 1))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (i);
  }
  return TCL_OK;
}

static int tcl_control STDVAR
{
  int idx;
  void *hold;

  BADARGS(3, 3, " idx command");

  idx = findidx(atoi(argv[1]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[idx].type->flags & DCT_CHAT) {
    if (dcc[idx].u.chat->channel >= 0) {
      chanout_but(idx, dcc[idx].u.chat->channel, "*** %s has gone.\n",
                  dcc[idx].nick);
      check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock,
                     dcc[idx].u.chat->channel);
      botnet_send_part_idx(idx, "gone");
    }
    check_tcl_chof(dcc[idx].nick, dcc[idx].sock);
  }
  hold = dcc[idx].u.other;
  dcc[idx].u.script = get_data_ptr(sizeof(struct script_info));
  dcc[idx].u.script->u.other = hold;
  dcc[idx].u.script->type = dcc[idx].type;
  dcc[idx].type = &DCC_SCRIPT;
  /* Do not buffer data anymore. All received and stored data is passed
   * over to the dcc functions from now on.  */
  sockoptions(dcc[idx].sock, EGG_OPTION_UNSET, SOCK_BUFFER);
  strlcpy(dcc[idx].u.script->command, argv[2], 120);
  return TCL_OK;
}

static int tcl_valididx STDVAR
{
  int idx;

  BADARGS(2, 2, " idx");

  idx = findidx(atoi(argv[1]));
  if (idx < 0 || !(dcc[idx].type->flags & DCT_VALIDIDX))
    Tcl_AppendResult(irp, "0", NULL);
  else
    Tcl_AppendResult(irp, "1", NULL);
   return TCL_OK;
}

static int tcl_killdcc STDVAR
{
  int idx;

  BADARGS(2, 3, " idx ?reason?");

  idx = findidx(atoi(argv[1]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }

  if ((dcc[idx].sock == STDOUT) && !backgrd) /* Don't kill terminal socket */
    return TCL_OK;


  if (dcc[idx].type->flags & DCT_CHAT) { /* Make sure 'whom' info is updated */
    chanout_but(idx, dcc[idx].u.chat->channel, "*** %s has left the %s%s%s\n",
                dcc[idx].nick, dcc[idx].u.chat ? "channel" : "partyline",
                argc == 3 ? ": " : "", argc == 3 ? argv[2] : "");
    botnet_send_part_idx(idx, argc == 3 ? argv[2] : "");
    if ((dcc[idx].u.chat->channel >= 0) &&
        (dcc[idx].u.chat->channel < GLOBAL_CHANS))
      check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock,
                     dcc[idx].u.chat->channel);
    check_tcl_chof(dcc[idx].nick, dcc[idx].sock);
  }
  killsock(dcc[idx].sock);
  killtransfer(idx);
  lostdcc(idx);
  return TCL_OK;
}

static int tcl_putbot STDVAR
{
  int i;
  char msg[401];

  BADARGS(3, 3, " botnick message");

  i = nextbot(argv[1]);
  if (i < 0) {
    Tcl_AppendResult(irp, "bot is not on the botnet", NULL);
    return TCL_ERROR;
  }
  strlcpy(msg, argv[2], sizeof msg);

  botnet_send_zapf(i, botnetnick, argv[1], msg);
  return TCL_OK;
}

static int tcl_putallbots STDVAR
{
  char msg[401];

  BADARGS(2, 2, " message");

  strlcpy(msg, argv[1], sizeof msg);
  botnet_send_zapf_broad(-1, botnetnick, NULL, msg);
  return TCL_OK;
}

static int tcl_idx2hand STDVAR
{
  int idx;

  BADARGS(2, 2, " idx");

  idx = findidx(atoi(argv[1]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }

  Tcl_AppendResult(irp, dcc[idx].nick, NULL);
  return TCL_OK;
}

static int tcl_islinked STDVAR
{
  int i;

  BADARGS(2, 2, " bot");

  i = nextbot(argv[1]);
  if (i < 0)
    Tcl_AppendResult(irp, "0", NULL);
  else
    Tcl_AppendResult(irp, "1", NULL);
  return TCL_OK;
}

static int tcl_bots STDVAR
{
  tand_t *bot;

  BADARGS(1, 1, "");

  for (bot = tandbot; bot; bot = bot->next)
    Tcl_AppendElement(irp, bot->bot);
  return TCL_OK;
}

static int tcl_botlist STDVAR
{
  char *p, sh[2], string[20];
  EGG_CONST char *list[4];
  tand_t *bot;

  BADARGS(1, 1, "");

  sh[1] = 0;
  list[3] = sh;
  list[2] = string;
  for (bot = tandbot; bot; bot = bot->next) {
    list[0] = bot->bot;
    list[1] = (bot->uplink == (tand_t *) 1) ? botnetnick : bot->uplink->bot;
    strlcpy(string, int_to_base10(bot->ver), sizeof string);
    sh[0] = bot->share;
    p = Tcl_Merge(4, list);
    Tcl_AppendElement(irp, p);
    Tcl_Free((char *) p);
  }
  return TCL_OK;
}

static void build_dcc_list(Tcl_Interp *irp, char *idxstr, char *nick, char *host,
            char *portstring, char *type, char *other, char *timestamp) {
  char *p;
  EGG_CONST char *list[7];

  list[0] = idxstr;
  list[1] = nick;
  list[2] = host;
  list[3] = portstring;
  list[4] = type;
  list[5] = other;
  list[6] = timestamp;
  p = Tcl_Merge(7, list);
  Tcl_AppendElement(irp, p);
  Tcl_Free((char *) p);
}

/* Build and return a list of lists of all sockets, in dict-readable format */
static void build_sock_list(Tcl_Interp *irp, Tcl_Obj *masterlist, char *idxstr,
            char *nick, char *host, char *ip, int port, int secure,
            char *type, char *other, char *timestamp) {
  EGG_CONST char *val[] = {"idx", "handle", "host", "ip", "port", "secure",
                           "type", "info", "time"};
  Tcl_Obj *thelist;
  char securestr[2], portstr[6];

  egg_snprintf(securestr, sizeof securestr, "%d", secure);
  egg_snprintf(portstr, sizeof portstr, "%d", port);
  thelist = Tcl_NewListObj(0, NULL);
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(val[0], -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(idxstr, -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(val[1], -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(nick, -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(val[2], -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(host, -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(val[3], -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(ip, -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(val[4], -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(portstr, -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(val[5], -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(securestr, -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(val[6], -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(type, -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(val[7], -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(other, -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(val[8], -1));
  Tcl_ListObjAppendElement(irp, thelist, Tcl_NewStringObj(timestamp, -1));
  Tcl_ListObjAppendElement(irp, masterlist, thelist);
  Tcl_SetObjResult(irp, masterlist);
}

/* Gather information for dcclist or socklist */
static void dccsocklist(Tcl_Interp *irp, int argc, char *type, int src) {
  int i;
  char idxstr[10], timestamp[11], other[160];
  char portstring[7]; /* ssl + portmax + NULL */
  long tv;
#ifdef IPV6
  char s[INET6_ADDRSTRLEN];
#else
  char s[INET_ADDRSTRLEN];
#endif
  socklen_t namelen;
  struct sockaddr_storage ss;
  Tcl_Obj *masterlist = NULL; /* initialize to NULL to make old gcc versions
                               * happy */
 
  if (src) {
    masterlist = Tcl_NewListObj(0, NULL);
  }
  for (i = 0; i < dcc_total; i++) {
    if (argc == 1 || ((argc == 2) && (dcc[i].type &&
        !strcasecmp(dcc[i].type->name, type)))) {
      egg_snprintf(idxstr, sizeof idxstr, "%ld", dcc[i].sock);
      tv = dcc[i].timeval;
      egg_snprintf(timestamp, sizeof timestamp, "%ld", tv);
      if (dcc[i].type && dcc[i].type->display)
        dcc[i].type->display(i, other);
      else {
        egg_snprintf(other, sizeof other, "?:%lX  !! ERROR !!",
                     (long) dcc[i].type);
        break;
      }
#ifdef TLS
      egg_snprintf(portstring, sizeof portstring, "%s%d", dcc[i].ssl ? "+" : "", dcc[i].port);
#else
      egg_snprintf(portstring, sizeof portstring, "%d", dcc[i].port);
#endif
      /* If this came from dcclist... */
      if (!src) {
        build_dcc_list(irp, idxstr, dcc[i].nick,
            (dcc[i].host[0] == '\0') ? iptostr(&dcc[i].sockname.addr.sa) : dcc[i].host,
            portstring, dcc[i].type ? dcc[i].type->name : "*UNKNOWN*", other,
            timestamp);
      /* If this came from socklist... */
      } else {
        /* Update dcc table socket information, needed for getting local IP */
        namelen = sizeof ss;
        getsockname(dcc[i].sock, (struct sockaddr *) &ss, &namelen);
        if (ss.ss_family == AF_INET) {
          struct sockaddr_in *saddr = (struct sockaddr_in *)&ss;
          inet_ntop(AF_INET, &(saddr->sin_addr), s, INET_ADDRSTRLEN);
#ifdef IPV6
        } else if (ss.ss_family == AF_INET6) {
          struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)&ss;
            inet_ntop(AF_INET6, &(saddr->sin6_addr), s, INET6_ADDRSTRLEN);
#endif
        }
        build_sock_list(irp, masterlist, idxstr, dcc[i].nick,
            (dcc[i].host[0] == '\0') ? iptostr(&dcc[i].sockname.addr.sa) : dcc[i].host,
            s, dcc[i].port,
#ifdef TLS
            dcc[i].ssl,
#else
            0,
#endif
            dcc[i].type ? dcc[i].type->name : "*UNKNOWN*", other,
            timestamp);
      }
    }
  }
}

static int tcl_socklist STDVAR
{

  BADARGS(1, 2, " ?type?");
  dccsocklist(irp, argc, (argc == 2) ? argv[1] : NULL, 1);
  return TCL_OK;
}

static int tcl_dcclist STDVAR
{
  BADARGS(1, 2, " ?type?");
  dccsocklist(irp, argc, (argc == 2) ? argv[1] : NULL, 0);
  return TCL_OK;
}

static int tcl_whom STDVAR
{
  int chan, i;
  char c[2], idle[32], work[20], *p;
  long tv = 0;
  EGG_CONST char *list[7];

  BADARGS(2, 2, " chan");

  if (argv[1][0] == '*')
    chan = -1;
  else {
    if ((argv[1][0] < '0') || (argv[1][0] > '9')) {
      Tcl_SetVar(interp, "_chan", argv[1], 0);
      if ((Tcl_VarEval(interp, "assoc ", "$_chan", NULL) != TCL_OK) ||
          tcl_resultempty()) {
        Tcl_AppendResult(irp, "channel name is invalid", NULL);
        return TCL_ERROR;
      }
      chan = tcl_resultint();
    } else
      chan = atoi(argv[1]);
    if ((chan < 0) || (chan > 199999)) {
      Tcl_AppendResult(irp, "channel out of range; must be 0 through 199999",
                       NULL);
      return TCL_ERROR;
    }
  }
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_CHAT) {
      if (dcc[i].u.chat->channel == chan || chan == -1) {
        c[0] = geticon(i);
        c[1] = 0;
        tv = (now - dcc[i].timeval) / 60;
        egg_snprintf(idle, sizeof idle, "%li", tv);
        list[0] = dcc[i].nick;
        list[1] = botnetnick;
        list[2] = dcc[i].host;
        list[3] = c;
        list[4] = idle;
        list[5] = dcc[i].u.chat->away ? dcc[i].u.chat->away : "";
        if (chan == -1) {
          egg_snprintf(work, sizeof work, "%d", dcc[i].u.chat->channel);
          list[6] = work;
        }
        p = Tcl_Merge((chan == -1) ? 7 : 6, list);
        Tcl_AppendElement(irp, p);
        Tcl_Free((char *) p);
      }
    }
  for (i = 0; i < parties; i++) {
    if (party[i].chan == chan || chan == -1) {
      c[0] = party[i].flag;
      c[1] = 0;
      if (party[i].timer == 0L)
        strcpy(idle, "0");
      else {
        tv = (now - party[i].timer) / 60;
        egg_snprintf(idle, sizeof idle, "%li", tv);
      }
      list[0] = party[i].nick;
      list[1] = party[i].bot;
      list[2] = party[i].from ? party[i].from : "";
      list[3] = c;
      list[4] = idle;
      list[5] = party[i].status & PLSTAT_AWAY ? party[i].away : "";
      if (chan == -1) {
        egg_snprintf(work, sizeof work, "%d", party[i].chan);
        list[6] = work;
      }
      p = Tcl_Merge((chan == -1) ? 7 : 6, list);
      Tcl_AppendElement(irp, p);
      Tcl_Free((char *) p);
    }
  }
  return TCL_OK;
}

static int tcl_dccused STDVAR
{
  char s[20];

  BADARGS(1, 1, "");

  egg_snprintf(s, sizeof s, "%d", dcc_total);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_getdccidle STDVAR
{
  int x, idx;
  char s[21];

  BADARGS(2, 2, " idx");

  idx = findidx(atoi(argv[1]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  x = (now - dcc[idx].timeval);

  egg_snprintf(s, sizeof s, "%d", x);
  Tcl_AppendElement(irp, s);
  return TCL_OK;
}

static int tcl_getdccaway STDVAR
{
  int idx;

  BADARGS(2, 2, " idx");

  idx = findidx(atol(argv[1]));
  if (idx < 0 || dcc[idx].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[idx].u.chat->away == NULL)
    return TCL_OK;

  Tcl_AppendResult(irp, dcc[idx].u.chat->away, NULL);
  return TCL_OK;
}

static int tcl_setdccaway STDVAR
{
  int idx;

  BADARGS(3, 3, " idx message");

  idx = findidx(atol(argv[1]));
  if (idx < 0 || dcc[idx].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (!argv[2][0]) {
    if (dcc[idx].u.chat->away != NULL)
      not_away(idx);
    return TCL_OK;
  }
  set_away(idx, argv[2]);
  return TCL_OK;
}

static int tcl_link STDVAR
{
  int x, i;
  char bot[HANDLEN + 1], bot2[HANDLEN + 1];

  BADARGS(2, 3, " ?via-bot? bot");

  strlcpy(bot, argv[1], sizeof bot);
  if (argc == 3) {
    x = 1;
    strlcpy(bot2, argv[2], sizeof bot2);
    i = nextbot(bot);
    if (i < 0)
      x = 0;
    else
      botnet_send_link(i, botnetnick, bot, bot2);
  } else
    x = botlink("", -2, bot);

  egg_snprintf(bot, sizeof bot, "%d", x);
  Tcl_AppendResult(irp, bot, NULL);
  return TCL_OK;
}

static int tcl_unlink STDVAR
{
  int i, x;
  char bot[HANDLEN + 1];

  BADARGS(2, 3, " bot ?comment?");

  strlcpy(bot, argv[1], sizeof bot);
  i = nextbot(bot);
  if (i < 0)
    x = 0;
  else {
    x = 1;
    if (!strcasecmp(bot, dcc[i].nick))
      x = botunlink(-2, bot, argv[2], botnetnick);
    else
      botnet_send_unlink(i, botnetnick, lastbot(bot), bot, argv[2]);
  }
  egg_snprintf(bot, sizeof bot, "%d", x);

  Tcl_AppendResult(irp, bot, NULL);
  return TCL_OK;
}

static int tcl_connect STDVAR
{
  int i, sock;
  char s[81];

  BADARGS(3, 3, " hostname port");

  if (dcc_total == max_dcc && increase_socks_max()) {
    Tcl_AppendResult(irp, "out of dcc table space", NULL);
    return TCL_ERROR;
  }

  i = new_dcc(&DCC_SOCKET, 0);
  if (i < 0) {
    Tcl_AppendResult(irp, "Could not allocate socket.", NULL);
    return TCL_ERROR;
  }
  sock = open_telnet(i, argv[1], atoi(argv[2]));
  if (sock < 0) {
    switch (sock) {
      case -3:
        Tcl_AppendResult(irp, MISC_NOFREESOCK, NULL);
        break;
      case -2:
        Tcl_AppendResult(irp, "DNS lookup failed", NULL);
        break;
      default:
        Tcl_AppendResult(irp, strerror(errno), NULL);
    }
    lostdcc(i);
    return TCL_ERROR;
  }
#ifdef TLS
  if (*argv[2] == '+') {
    if (ssl_handshake(sock, TLS_CONNECT, 0, LOG_MISC, NULL, NULL)) {
      killsock(sock);
      lostdcc(i);
      strlcpy(s, "Failed to establish a TLS session", sizeof s);
      Tcl_AppendResult(irp, s, NULL);
      return TCL_ERROR;
    } else
      dcc[i].ssl = 1;
  }
#endif
  strcpy(dcc[i].nick, "*");
  strlcpy(dcc[i].host, argv[1], UHOSTMAX);
  egg_snprintf(s, sizeof s, "%d", sock);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int setlisten(Tcl_Interp *irp, char *ip, char *portp, char *type, char *maskproc, char *flag) {
  int i, idx = -1, port, realport, found=0, ipv4=1;
  char s[11], msg[256], newip[INET6_ADDRSTRLEN];
  struct portmap *pmap = NULL, *pold = NULL;
  sockname_t name;
  struct in_addr ipaddr4;
  struct addrinfo hint, *ipaddr = NULL;
  int ret;
#ifdef IPV6
  struct in6_addr ipaddr6;
#endif

  memset(&hint, '\0', sizeof hint);
  hint.ai_family = PF_UNSPEC;
  hint.ai_flags = AI_NUMERICHOST;
  if (!ip[0]) {
#ifdef IPV6
    if (pref_af) {
      strlcpy(newip, "::", sizeof newip);
    } else {
#endif
      strlcpy(newip, "0.0.0.0", sizeof newip);
#ifdef IPV6
    }
#endif
  } else {
    strlcpy(newip, ip, sizeof newip);
  }
  /* Return addrinfo struct ipaddr containing family... */
  ret = getaddrinfo(newip, NULL, &hint, &ipaddr);
  if (!ret) {
  /* Load network address to in(6)_addr struct for later byte comparisons */
    if (ipaddr->ai_family == AF_INET) {
      inet_pton(AF_INET, newip, &ipaddr4);
    }
#ifdef IPV6
    else if (ipaddr->ai_family == AF_INET6) {
      inet_pton(AF_INET6, newip, &ipaddr6);
      ipv4 = 0;
    }
#endif
  }
  freeaddrinfo(ipaddr);
  port = realport = atoi(portp);
  for (pmap = root; pmap; pold = pmap, pmap = pmap->next) {
    if (pmap->realport == port) {
      port = pmap->mappedto;
      break;
    }
  }
  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type == &DCC_TELNET) && (dcc[i].port == port)) {
      idx = i;
      found = 1;

      /* Check if this is an exact match and skip these checks (ie, rehash) */
      if (ipv4) {
        if (ipaddr4.s_addr == dcc[idx].sockname.addr.s4.sin_addr.s_addr) {
          break;
        }
      }
#ifdef IPV6
      else if (ipaddr6.s6_addr == dcc[idx].sockname.addr.s6.sin6_addr.s6_addr) {
        break;
      }

      /* Check if the bound IP is IPvX, and the new IP is IPvY */
      if (((ipv4) && (dcc[idx].sockname.addr.sa.sa_family != AF_INET)) ||
         ((!ipv4) && (dcc[idx].sockname.addr.sa.sa_family != AF_INET6))) {
        found = 0;
        break;
      }
#endif

      /* Check if IP is specific, but the already-bound IP is all-interfaces */
      if (ipv4) {
        if ((ipaddr4.s_addr != 0) && (dcc[idx].sockname.addr.s4.sin_addr.s_addr == 0)) {
          Tcl_AppendResult(irp, "this port is already bound to 0.0.0.0 on this "
                "machine, remove it before trying to bind to this IP", NULL);
          return TCL_ERROR;
        }
      }
#ifdef IPV6
      else if ((!IN6_IS_ADDR_UNSPECIFIED(&ipaddr6)) &&
                (IN6_IS_ADDR_UNSPECIFIED(&dcc[idx].sockname.addr.s6.sin6_addr))) {
          Tcl_AppendResult(irp, "this port is already bound to :: on this "
                "machine, remove it before trying to bind to this IP", NULL);
          return TCL_ERROR;
      }
#endif

      /* Check if IP is all-interfaces, but the already-bound IP is specific */
      if (ipv4) {
        if ((ipaddr4.s_addr == 0) && (dcc[idx].sockname.addr.s4.sin_addr.s_addr != 0)) {
          Tcl_AppendResult(irp, "this port is already bound to a specific IP "
                "on this machine, remove it before trying to bind to all "
                "interfaces", NULL);
          return TCL_ERROR;
        }
      }
#ifdef IPV6
      else if (IN6_IS_ADDR_UNSPECIFIED(&ipaddr6) &&
                (!IN6_IS_ADDR_UNSPECIFIED(&dcc[idx].sockname.addr.s6.sin6_addr))) {
          Tcl_AppendResult(irp, "this port is already bound to a specific IP "
                "on this machine, remove it before trying to bind to this all "
                "interfaces", NULL);
          return TCL_ERROR;
      }
#endif
    }
  }
  if (!strcasecmp(type, "off")) {
    if (pmap) {
      if (pold)
        pold->next = pmap->next;
      else
        root = pmap->next;
      nfree(pmap);
    }
    /* Remove */
    if (idx < 0) {
      Tcl_AppendResult(irp, "no such listen port is open", NULL);
      return TCL_ERROR;
    }
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return TCL_OK;
  }
  /* If there isn't already something listening on that port, or there is but
   * it is something that may allow us to try a different IP for that port
   */
  if ((idx < 0) || (!found)) {
    /* Make new one */
    if (dcc_total >= max_dcc && increase_socks_max()) {
      Tcl_AppendResult(irp, "No more DCC slots available.", NULL);
      return TCL_ERROR;
    }
    /* We used to try up to 20 ports here, but have scientifically concluded
     * that is just silly.
     */
    /* If we didn't find a listening ip/port, or we did but it isn't all
     * interfaces
     */
    if (strlen(newip)) {
      setsockname(&name, newip, port, 1);
      i = open_address_listen(&name);
    } else {
      i = open_listen(&port);
    }
    if (i < 0) {
      egg_snprintf(msg, sizeof msg, "Couldn't listen on port '%d' on the given "
                 "address: %s. Please check that the port is not already in use",
                  realport, strerror(errno));
      Tcl_AppendResult(irp, msg, NULL);
      return TCL_ERROR;
    }
    idx = new_dcc(&DCC_TELNET, 0);
    dcc[idx].sockname.addrlen = sizeof(dcc[idx].sockname.addr);
    getsockname(i, &dcc[idx].sockname.addr.sa, &dcc[idx].sockname.addrlen);
    dcc[idx].sockname.family = dcc[idx].sockname.addr.sa.sa_family;
    dcc[idx].port = port;
    dcc[idx].sock = i;
    dcc[idx].timeval = now;
  }
#ifdef TLS
  if (portp[0] == '+')
    dcc[idx].ssl = 1;
  else
    dcc[idx].ssl = 0;
#endif
  /* script? */
  if (!strcmp(type, "script")) {
    strcpy(dcc[idx].nick, "(script)");
    if (flag) {
      dcc[idx].status = LSTN_PUBLIC;
    }
    strlcpy(dcc[idx].host, maskproc, UHOSTMAX);
    egg_snprintf(s, sizeof s, "%d", port);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  }
  /* bots/users/all */
  if (!strcmp(type, "bots"))
    strcpy(dcc[idx].nick, "(bots)");
  else if (!strcmp(type, "users"))
    strcpy(dcc[idx].nick, "(users)");
  else if (!strcmp(type, "all"))
    strcpy(dcc[idx].nick, "(telnet)");
  if (maskproc[0])
    strlcpy(dcc[idx].host, maskproc, UHOSTMAX);
  else
    strcpy(dcc[idx].host, "*");
  egg_snprintf(s, sizeof s, "%d", port);
  Tcl_AppendResult(irp, s, NULL);
  if (!pmap) {
    pmap = nmalloc(sizeof(struct portmap));
    pmap->next = root;
    root = pmap;
  }
  pmap->realport = realport;
  pmap->mappedto = port;

#ifdef TLS
  putlog(LOG_MISC, "*", "Listening for telnet connections on %s port %s%d (%s).",
        iptostr(&dcc[idx].sockname.addr.sa), dcc[idx].ssl ? "+" : "", port, type);
#else
  putlog(LOG_MISC, "*", "Listening for telnet connections on %s port %d (%s).",
        iptostr(&dcc[idx].sockname.addr.sa), port, type);
#endif

  return TCL_OK;
}

/* Create a new listening port (or destroy one)
 *
 * listen [ip] <port> bots/all/users [mask]
 * listen [ip] <port> script <proc> <flag>
 * listen [ip] <port> off
 */
static int tcl_listen STDVAR
{
  char ip[121], maskproc[UHOSTMAX] = "";
  char port[7], type[7], flag[4], *endptr;
  unsigned char buf[sizeof(struct in6_addr)];
  int i = 1;

/* People like to add comments to this command for some reason, and it can cause
 * errors that are difficult to figure out. Let's instead throw a more helpful
 * error for this case to get around BADARGS, and handle other cases further
 * down in the code
 *
 * Check if extra args are config comments 
 */
  if (argc > 6) {
    if (argv[6][0] == '#') {
      fatal(DCC_BADLISTEN, 0);
    }
  }

  BADARGS(3, 6, " ?ip? port type ?mask?/?proc flag?");

/* default listen-addr if not specified */
  strlcpy(ip, listen_ip, sizeof(ip));

/* Check if IP exists, set to NULL if not */
  strtol(argv[1], &endptr, 10);
  if (*endptr != '\0') {
    if (inet_pton(AF_INET, argv[1], buf)
#ifdef IPV6
        || inet_pton(AF_INET6, argv[1], buf)
#endif
      ) {
      strlcpy(ip, argv[1], sizeof(ip));
      i++;
    } else {
      Tcl_AppendResult(irp, "invalid ip address", NULL);
      return TCL_ERROR;
    }
  }
/* Check for port */
  if ((atoi(argv[i]) > 65535) || (atoi(argv[i]) < 1)) {
    Tcl_AppendResult(irp, "invalid listen port", NULL);
    return TCL_ERROR;
  }
  strlcpy(port, argv[i], sizeof(port));
  i++;
/* Check for listen type */
  if (!argv[i]) {
    Tcl_AppendResult(irp, "missing listen type", NULL);
    return TCL_ERROR;
  }
  if ((strcmp(argv[i], "bots")) && (strcmp(argv[i], "users"))
        && (strcmp(argv[i], "all")) && (strcmp(argv[i], "off"))
        && (strcmp(argv[i], "script"))) {
    Tcl_AppendResult(irp, "invalid listen type: must be one of ",
          "bots, users, all, off, script", NULL);
    return TCL_ERROR;
  }
  strlcpy(type, argv[i], sizeof(type));
/* Check if mask or proc exists */
  if ((((argc>3) && !ip[0]) || ((argc >4) && ip[0])) &&
        (argv[i+1][0] != '#')) { /* Ignore config comments! */
    i++;
    strlcpy(maskproc, argv[i], sizeof(maskproc));
  }
/* If script, check for proc and flag */
  if (!strcmp(type, "script")) {
    if (!maskproc[0]) {
      Tcl_AppendResult(irp, "a proc name must be specified for a script listen", NULL);
      return TCL_ERROR;
    }
    if ((!ip[0] && (argc==4)) || (ip[0] && argc==5)) {
      Tcl_AppendResult(irp, "missing flag. allowed flags: pub", NULL);
      return TCL_ERROR;
    }
    if ((!ip[0] && (argc==5)) || (argc == 6)) {
      i++;
      if (strcmp(argv[i], "pub")) {
        Tcl_AppendResult(irp, "unknown flag: ", flag, ". allowed flags: pub",
              NULL);
        return TCL_ERROR;
      }
      strlcpy(flag, argv[i], sizeof flag);
    }
  }
  return setlisten(irp, ip, port, type, maskproc, flag);
}

static int tcl_boot STDVAR
{
  char who[NOTENAMELEN + 1];
  int i, ok = 0;

  BADARGS(2, 3, " user@bot ?reason?");

  strlcpy(who, argv[1], sizeof who);

  if (strchr(who, '@') != NULL) {
    char whonick[HANDLEN + 1];

    splitc(whonick, who, '@');
    whonick[HANDLEN] = 0;
    if (!strcasecmp(who, botnetnick))
      strlcpy(who, whonick, sizeof who);
    else if (remote_boots > 0) {
      i = nextbot(who);
      if (i < 0)
        return TCL_OK;
      botnet_send_reject(i, botnetnick, NULL, whonick, who,
                         argc >= 3 && argv[2] ? argv[2] : "");
    } else
      return TCL_OK;
  }
  for (i = 0; i < dcc_total; i++)
    if (!ok && (dcc[i].type->flags & DCT_CANBOOT) &&
        !strcasecmp(dcc[i].nick, who)) {
      do_boot(i, botnetnick, argc >= 3 && argv[2] ? argv[2] : "");
      ok = 1;
    }
  return TCL_OK;
}

static int tcl_rehash STDVAR
{
  BADARGS(1, 1, "");

  if (make_userfile) {
    putlog(LOG_MISC, "*", USERF_NONEEDNEW);
    make_userfile = 0;
  }
  write_userfile(-1);

  putlog(LOG_MISC, "*", USERF_REHASHING);
  do_restart = -2;
  return TCL_OK;
}

static int tcl_restart STDVAR
{
  BADARGS(1, 1, "");

  if (!backgrd) {
    Tcl_AppendResult(interp, "You can't restart a -n bot", NULL);
    return TCL_ERROR;
  }
  if (make_userfile) {
    putlog(LOG_MISC, "*", USERF_NONEEDNEW);
    make_userfile = 0;
  }
  write_userfile(-1);
  putlog(LOG_MISC, "*", MISC_RESTARTING);
  wipe_timers(interp, &utimer);
  wipe_timers(interp, &timer);
  do_restart = -1;
  return TCL_OK;
}

static int tcl_traffic STDVAR
{
  char buf[1024];
  unsigned long out_total_today, out_total;
  unsigned long in_total_today, in_total;

  /* IRC traffic */
  sprintf(buf, "irc %lu %lu %lu %lu", itraffic_irc_today, itraffic_irc +
          itraffic_irc_today, otraffic_irc_today,
          otraffic_irc + otraffic_irc_today);
  Tcl_AppendElement(irp, buf);

  /* Botnet traffic */
  sprintf(buf, "botnet %lu %lu %lu %lu", itraffic_bn_today, itraffic_bn +
          itraffic_bn_today, otraffic_bn_today,
          otraffic_bn + otraffic_bn_today);
  Tcl_AppendElement(irp, buf);

  /* Partyline */
  sprintf(buf, "partyline %lu %lu %lu %lu", itraffic_dcc_today, itraffic_dcc +
          itraffic_dcc_today, otraffic_dcc_today,
          otraffic_dcc + otraffic_dcc_today);
  Tcl_AppendElement(irp, buf);

  /* Transfer */
  sprintf(buf, "transfer %lu %lu %lu %lu", itraffic_trans_today,
          itraffic_trans + itraffic_trans_today, otraffic_trans_today,
          otraffic_trans + otraffic_trans_today);
  Tcl_AppendElement(irp, buf);

  /* Misc traffic */
  sprintf(buf, "misc %lu %lu %lu %lu", itraffic_unknown_today,
          itraffic_unknown + itraffic_unknown_today, otraffic_unknown_today,
          otraffic_unknown + otraffic_unknown_today);
  Tcl_AppendElement(irp, buf);

  /* Totals */
  in_total_today = itraffic_irc_today + itraffic_bn_today +
                   itraffic_dcc_today + itraffic_trans_today +
                   itraffic_unknown_today;
  in_total = in_total_today + itraffic_irc + itraffic_bn + itraffic_dcc +
             itraffic_trans + itraffic_unknown;
  out_total_today = otraffic_irc_today + otraffic_bn_today +
                    otraffic_dcc_today + itraffic_trans_today +
                    otraffic_unknown_today;
  out_total = out_total_today + otraffic_irc + otraffic_bn + otraffic_dcc +
              otraffic_trans + otraffic_unknown;
  sprintf(buf, "total %lu %lu %lu %lu", in_total_today, in_total,
          out_total_today, out_total);
  Tcl_AppendElement(irp, buf);
  return TCL_OK;
}

tcl_cmds tcldcc_cmds[] = {
  {"putdcc",             tcl_putdcc},
  {"putdccraw",       tcl_putdccraw},
  {"putidx",             tcl_putdcc},
  {"dccsimul",         tcl_dccsimul},
  {"dccbroadcast", tcl_dccbroadcast},
  {"hand2idx",         tcl_hand2idx},
  {"getchan",           tcl_getchan},
  {"setchan",           tcl_setchan},
  {"dccputchan",     tcl_dccputchan},
  {"console",           tcl_console},
  {"resetconsole", tcl_resetconsole},
  {"strip",               tcl_strip},
  {"echo",                 tcl_echo},
  {"page",                 tcl_page},
  {"control",           tcl_control},
  {"valididx",         tcl_valididx},
  {"killdcc",           tcl_killdcc},
  {"putbot",             tcl_putbot},
  {"putallbots",     tcl_putallbots},
  {"idx2hand",         tcl_idx2hand},
  {"bots",                 tcl_bots},
  {"botlist",           tcl_botlist},
  {"dcclist",           tcl_dcclist},
  {"socklist",         tcl_socklist},
  {"whom",                 tcl_whom},
  {"dccused",           tcl_dccused},
  {"getdccidle",     tcl_getdccidle},
  {"getdccaway",     tcl_getdccaway},
  {"setdccaway",     tcl_setdccaway},
  {"islinked",         tcl_islinked},
  {"link",                 tcl_link},
  {"unlink",             tcl_unlink},
  {"connect",           tcl_connect},
  {"listen",             tcl_listen},
  {"boot",                 tcl_boot},
  {"rehash",             tcl_rehash},
  {"restart",           tcl_restart},
  {"traffic",           tcl_traffic},
  {NULL,                       NULL}
};
