/*
 * tcldcc.c -- handles:
 *   Tcl stubs for the dcc commands
 *
 * $Id: tcldcc.c,v 1.70 2011/02/13 14:19:33 simple Exp $
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

extern Tcl_Interp *interp;
extern tcl_timer_t *timer, *utimer;
extern struct dcc_t *dcc;
extern char botnetnick[];
extern int dcc_total, backgrd, parties, make_userfile, do_restart, remote_boots, max_dcc;
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

  if ((argc == 4) && egg_strcasecmp(argv[3], "-raw")) {
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

  strncpyz(msg, argv[1], sizeof msg);
  chatout("*** %s\n", msg);
  botnet_send_chat(-1, botnetnick, msg);
  check_tcl_bcst(botnetnick, -1, msg);
  return TCL_OK;
}

static int tcl_hand2idx STDVAR
{
  int i;
  char s[11];

  BADARGS(2, 2, " nickname");

  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type->flags & (DCT_SIMUL | DCT_BOT)) &&
        !egg_strcasecmp(argv[1], dcc[i].nick)) {
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
    if (!strcmp(argv[2], "-1") || !egg_strcasecmp(argv[2], "off"))
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
  strncpyz(msg, argv[2], sizeof msg);

  chanout_but(-1, chan, "*** %s\n", argv[2]);
  botnet_send_chan(-1, botnetnick, NULL, chan, argv[2]);
  check_tcl_bcst(botnetnick, chan, argv[2]);
  return TCL_OK;
}

static int tcl_console STDVAR
{
  int i, j, pls, arg;
  module_entry *me;

  BADARGS(2, 4, " idx ?channel? ?console-modes?");

  i = findidx(atoi(argv[1]));
  if (i < 0 || dcc[i].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  pls = 1;

  for (arg = 2; arg < argc; arg++) {
    if (argv[arg][0] && ((strchr(CHANMETA, argv[arg][0]) != NULL) ||
        (argv[arg][0] == '*'))) {
      if ((argv[arg][0] != '*') && (!findchan_by_dname(argv[arg]))) {
        /* If we dont find the channel, and it starts with a +, assume it
         * should be the console flags to set. */
        if (argv[arg][0] == '+')
          goto do_console_flags;
        Tcl_AppendResult(irp, "invalid channel", NULL);
        return TCL_ERROR;
      }
      strncpyz(dcc[i].u.chat->con_chan, argv[arg], 81);
    } else {
      if ((argv[arg][0] != '+') && (argv[arg][0] != '-'))
        dcc[i].u.chat->con_flags = 0;
    do_console_flags:
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
  Tcl_AppendElement(irp, dcc[i].u.chat->con_chan);
  Tcl_AppendElement(irp, masktype(dcc[i].u.chat->con_flags));
  /* Console autosave. */
  if (argc > 2 && (me = module_find("console", 1, 1))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (i);
  }
  return TCL_OK;
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
  strncpyz(dcc[idx].u.script->command, argv[2], 120);
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
  strncpyz(msg, argv[2], sizeof msg);

  botnet_send_zapf(i, botnetnick, argv[1], msg);
  return TCL_OK;
}

static int tcl_putallbots STDVAR
{
  char msg[401];

  BADARGS(2, 2, " message");

  strncpyz(msg, argv[1], sizeof msg);
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
    strncpyz(string, int_to_base10(bot->ver), sizeof string);
    sh[0] = bot->share;
    p = Tcl_Merge(4, list);
    Tcl_AppendElement(irp, p);
    Tcl_Free((char *) p);
  }
  return TCL_OK;
}

static int tcl_dcclist STDVAR
{
  int i;
  char *p, idxstr[10], timestamp[11], other[160];
  long tv;
  EGG_CONST char *list[6];

  BADARGS(1, 2, " ?type?");

  for (i = 0; i < dcc_total; i++) {
    if (argc == 1 || ((argc == 2) && (dcc[i].type &&
        !egg_strcasecmp(dcc[i].type->name, argv[1])))) {
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
      list[0] = idxstr;
      list[1] = dcc[i].nick;
      list[2] = dcc[i].host;
      list[3] = dcc[i].type ? dcc[i].type->name : "*UNKNOWN*";
      list[4] = other;
      list[5] = timestamp;
      p = Tcl_Merge(6, list);
      Tcl_AppendElement(irp, p);
      Tcl_Free((char *) p);
    }
  }
  return TCL_OK;
}

static int tcl_whom STDVAR
{
  int chan, i;
  char c[2], idle[11], work[20], *p;
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

  strncpyz(bot, argv[1], sizeof bot);
  if (argc == 3) {
    x = 1;
    strncpyz(bot2, argv[2], sizeof bot2);
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

  strncpyz(bot, argv[1], sizeof bot);
  i = nextbot(bot);
  if (i < 0)
    x = 0;
  else {
    x = 1;
    if (!egg_strcasecmp(bot, dcc[i].nick))
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
  int i, z, sock;
  char s[81];

  BADARGS(3, 3, " hostname port");

  if (dcc_total == max_dcc && increase_socks_max()) {
    Tcl_AppendResult(irp, "out of dcc table space", NULL);
    return TCL_ERROR;
  }
  sock = getsock(0);

  if (sock < 0) {
    Tcl_AppendResult(irp, MISC_NOFREESOCK, NULL);
    return TCL_ERROR;
  }
  z = open_telnet_raw(sock, argv[1], atoi(argv[2]));
  if (z < 0) {
    killsock(sock);
    if (z == -2)
      strncpyz(s, "DNS lookup failed", sizeof s);
    else
      neterror(s);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_ERROR;
  }
  i = new_dcc(&DCC_SOCKET, 0);
  dcc[i].sock = sock;
  dcc[i].port = atoi(argv[2]);
  strcpy(dcc[i].nick, "*");
  strncpyz(dcc[i].host, argv[1], UHOSTMAX);
  egg_snprintf(s, sizeof s, "%d", sock);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

/* Create a new listening port (or destroy one)
 *
 * listen <port> bots/all/users [mask]
 * listen <port> script <proc> [flag]
 * listen <port> off
 */
static int tcl_listen STDVAR
{
  int i, j, idx = -1, port, realport;
  char s[11], msg[256];
  struct portmap *pmap = NULL, *pold = NULL;

  BADARGS(3, 5, " port type ?mask?/?proc ?flag??");

  port = realport = atoi(argv[1]);
  for (pmap = root; pmap; pold = pmap, pmap = pmap->next)
    if (pmap->realport == port) {
      port = pmap->mappedto;
      break;
  }
  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_TELNET) && (dcc[i].port == port))
      idx = i;
  if (!egg_strcasecmp(argv[2], "off")) {
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
  if (idx < 0) {
    /* Make new one */
    if (dcc_total >= max_dcc && increase_socks_max()) {
      Tcl_AppendResult(irp, "No more DCC slots available.", NULL);
      return TCL_ERROR;
    }
    /* Try to grab port */
    j = port + 20;
    i = -1;
    while (port < j && i < 0) {
      i = open_listen(&port);
      if (i == -1)
        port++;
      else if (i == -2)
        break;
    }
    if (i == -1) {
      egg_snprintf(msg, sizeof msg, "Couldn't listen on port '%d' on the "
                   "given address. Please make sure 'my-ip' is set correctly, "
                   "or try a different port.", realport);
      Tcl_AppendResult(irp, msg, NULL);
      return TCL_ERROR;
    } else if (i == -2) {
      Tcl_AppendResult(irp, "Couldn't assign the requested IP. Please make "
                       "sure 'my-ip' is set properly.", NULL);
      return TCL_ERROR;
    }
    idx = new_dcc(&DCC_TELNET, 0);
    dcc[idx].addr = iptolong(getmyip());
    dcc[idx].port = port;
    dcc[idx].sock = i;
    dcc[idx].timeval = now;
  }
  /* script? */
  if (!strcmp(argv[2], "script")) {
    strcpy(dcc[idx].nick, "(script)");
    if (argc < 4) {
      Tcl_AppendResult(irp, "a proc name must be specified for a script listen", NULL);
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return TCL_ERROR;
    }
    if (argc == 5) {
      if (strcmp(argv[4], "pub")) {
        Tcl_AppendResult(irp, "unknown flag: ", argv[4], ". allowed flags: pub",
                         NULL);
        killsock(dcc[idx].sock);
        lostdcc(idx);
        return TCL_ERROR;
      }
      dcc[idx].status = LSTN_PUBLIC;
    }
    strncpyz(dcc[idx].host, argv[3], UHOSTMAX);
    egg_snprintf(s, sizeof s, "%d", port);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  }
  /* bots/users/all */
  if (!strcmp(argv[2], "bots"))
    strcpy(dcc[idx].nick, "(bots)");
  else if (!strcmp(argv[2], "users"))
    strcpy(dcc[idx].nick, "(users)");
  else if (!strcmp(argv[2], "all"))
    strcpy(dcc[idx].nick, "(telnet)");
  if (!dcc[idx].nick[0]) {
    Tcl_AppendResult(irp, "invalid listen type: must be one of ",
                     "bots, users, all, off, script", NULL);
    killsock(dcc[idx].sock);
    dcc_total--;
    return TCL_ERROR;
  }
  if (argc == 4)
    strncpyz(dcc[idx].host, argv[3], UHOSTMAX);
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
  putlog(LOG_MISC, "*", "Listening at telnet port %d (%s).", port, argv[2]);
  return TCL_OK;
}

static int tcl_boot STDVAR
{
  char who[NOTENAMELEN + 1];
  int i, ok = 0;

  BADARGS(2, 3, " user@bot ?reason?");

  strncpyz(who, argv[1], sizeof who);

  if (strchr(who, '@') != NULL) {
    char whonick[HANDLEN + 1];

    splitc(whonick, who, '@');
    whonick[HANDLEN] = 0;
    if (!egg_strcasecmp(who, botnetnick))
      strncpyz(who, whonick, sizeof who);
    else if (remote_boots > 0) {
      i = nextbot(who);
      if (i < 0)
        return TCL_OK;
      botnet_send_reject(i, botnetnick, NULL, whonick, who,
                         argv[2] ? argv[2] : "");
    } else
      return TCL_OK;
  }
  for (i = 0; i < dcc_total; i++)
    if (!ok && (dcc[i].type->flags & DCT_CANBOOT) &&
        !egg_strcasecmp(dcc[i].nick, who)) {
      do_boot(i, botnetnick, argv[2] ? argv[2] : "");
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
  sprintf(buf, "irc %ld %ld %ld %ld", itraffic_irc_today, itraffic_irc +
          itraffic_irc_today, otraffic_irc_today,
          otraffic_irc + otraffic_irc_today);
  Tcl_AppendElement(irp, buf);

  /* Botnet traffic */
  sprintf(buf, "botnet %ld %ld %ld %ld", itraffic_bn_today, itraffic_bn +
          itraffic_bn_today, otraffic_bn_today,
          otraffic_bn + otraffic_bn_today);
  Tcl_AppendElement(irp, buf);

  /* Partyline */
  sprintf(buf, "partyline %ld %ld %ld %ld", itraffic_dcc_today, itraffic_dcc +
          itraffic_dcc_today, otraffic_dcc_today,
          otraffic_dcc + otraffic_dcc_today);
  Tcl_AppendElement(irp, buf);

  /* Transfer */
  sprintf(buf, "transfer %ld %ld %ld %ld", itraffic_trans_today,
          itraffic_trans + itraffic_trans_today, otraffic_trans_today,
          otraffic_trans + otraffic_trans_today);
  Tcl_AppendElement(irp, buf);

  /* Misc traffic */
  sprintf(buf, "misc %ld %ld %ld %ld", itraffic_unknown_today,
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
  sprintf(buf, "total %ld %ld %ld %ld", in_total_today, in_total,
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
