/*
 * tcldcc.c -- handles:
 * Tcl stubs for the dcc commands
 * 
 * dprintf'ized, 1aug1996
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

extern Tcl_Interp *interp;
extern tcl_timer_t *timer, *utimer;
extern struct dcc_t *dcc;
extern int dcc_total;
extern char botnetnick[];
extern int backgrd;
extern party_t *party;
extern int parties;
extern int make_userfile;
extern int do_restart;
extern int remote_boots;
extern tand_t *tandbot;
extern int max_dcc;
int enable_simul = 0;
extern time_t now;
static struct portmap *root = NULL;

int expmem_tcldcc(void)
{
  int tot = 0;
  struct portmap *pmap;

  for (pmap = root; pmap; pmap = pmap->next)
    tot += sizeof(struct portmap);

  return tot;
}

/***********************************************************************/

static int tcl_putdcc STDVAR
{
  int i, j;

  context;
  BADARGS(3, 3, " idx text");
  i = atoi(argv[1]);
  j = findidx(i);
  if (j < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  dumplots(-i, "", argv[2]);

  return TCL_OK;
}

/* function added by drummer@sophia.jpte.hu
 * allows tcl scripts to send out raw data
 * can be used for fast server write (idx=0)
 * usage: putdccraw <idx> <size> <rawdata> 
 * example: putdccraw 6 13 "eggdrop rulz\n" */
static int tcl_putdccraw STDVAR
{
  int i, j, z;

  context;
  BADARGS(4, 4, " idx size text");
  z = atoi(argv[1]);
  j = 0;
  for (i = 0; i < dcc_total; i++) {
    if (!(strcmp(dcc[i].nick, "(server)")) && (z == 0)) {
      j = dcc[i].sock;
    }
    if (dcc[i].sock == z) {
      j = dcc[i].sock;
    }
  }
  if (j == 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  tputs(j, argv[3], atoi(argv[2]));
  return TCL_OK;
}

static int tcl_dccsimul STDVAR
{
  int i, idx;

  context;
  BADARGS(3, 3, " idx command");
  if (enable_simul) {
    i = atoi(argv[1]);
    idx = findidx(i);
    if ((idx >= 0) && (dcc[idx].type->flags & DCT_SIMUL)) {
      int l = strlen(argv[2]);

      if (l > 510) {
	l = 510;
	argv[2][510] = 0;	/* restrict length of cmd */
      }
      if (dcc[idx].type && dcc[idx].type->activity) {
	dcc[idx].type->activity(idx, argv[2], l);
	return TCL_OK;
      }
    }
  }
  Tcl_AppendResult(irp, "invalid idx", NULL);
  return TCL_ERROR;
}

static int tcl_dccbroadcast STDVAR
{
  char msg[401];

  context;
  BADARGS(2, 2, " message");
  strncpy(msg, argv[1], 400);
  msg[400] = 0;
  chatout("*** %s\n", msg);
  botnet_send_chat(-1, botnetnick, msg);
  return TCL_OK;
}

static int tcl_hand2idx STDVAR
{
  int i;
  char s[10];

  context;
  BADARGS(2, 2, " nickname");
  for (i = 0; i < dcc_total; i++)
    if ((!strcasecmp(argv[1], dcc[i].nick)) &&
	 (dcc[i].type->flags & DCT_SIMUL)) {
      simple_sprintf(s, "%d", dcc[i].sock);
      Tcl_AppendResult(irp, s, NULL);
      return TCL_OK;
    }
  Tcl_AppendResult(irp, "-1", NULL);

  return TCL_OK;
}

static int tcl_getchan STDVAR
{
  char s[10];
  int idx, i;

  context;
  BADARGS(2, 2, " idx");
  i = atoi(argv[1]);
  idx = findidx(i);
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if ((dcc[idx].type != &DCC_CHAT) && (dcc[idx].type != &DCC_SCRIPT)) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[idx].type == &DCC_SCRIPT)
    sprintf(s, "%d", dcc[idx].u.script->u.chat->channel);
  else
    sprintf(s, "%d", dcc[idx].u.chat->channel);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_setchan STDVAR
{
  int idx, i, chan;

  context;
  BADARGS(3, 3, " idx channel");
  i = atoi(argv[1]);
  idx = findidx(i);
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if ((dcc[idx].type != &DCC_CHAT) && (dcc[idx].type != &DCC_SCRIPT)) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if ((argv[2][0] < '0') || (argv[2][0] > '9')) {
    if ((!strcmp(argv[2], "-1")) || (!strcasecmp(argv[2], "off")))
      chan = (-1);
    else {
      Tcl_SetVar(irp, "chan", argv[2], 0);
      if ((Tcl_VarEval(irp, "assoc ", "$chan", NULL) != TCL_OK) ||
	  !interp->result[0]) {
	Tcl_AppendResult(irp, "channel name is invalid", NULL);
	return TCL_ERROR;
      }
      chan = atoi(interp->result);
    }
  } else
    chan = atoi(argv[2]);
  if ((chan < -1) || (chan > 199999)) {
    Tcl_AppendResult(irp, "channel out of range; must be -1 thru 199999",
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
  return TCL_OK;
}

static int tcl_dccputchan STDVAR
{
  int chan;
  char msg[401];

  context;
  BADARGS(3, 3, " channel message");
  chan = atoi(argv[1]);
  if ((chan < 0) || (chan > 199999)) {
    Tcl_AppendResult(irp, "channel out of range; must be 0 thru 199999",
		     NULL);
    return TCL_ERROR;
  }
  strncpy(msg, argv[2], 400);

  msg[400] = 0;
  chanout_but(-1, chan, "*** %s\n", argv[2]);
  botnet_send_chan(-1, botnetnick, NULL, chan, argv[2]);
  check_tcl_bcst(botnetnick, chan, argv[2]);
  return TCL_OK;
}

static int tcl_console STDVAR
{
  int i, j, pls, arg;
  module_entry *me;

  context;
  BADARGS(2, 4, " idx ?channel? ?console-modes?");
  j = atoi(argv[1]);
  i = findidx(j);
  if (i < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[i].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  pls = 1;
  for (arg = 2; arg < argc; arg++) {
    if (argv[arg][0] && ((strchr(CHANMETA, argv[arg][0]) != NULL) ||
	(argv[arg][0] == '*'))) {
      if ((argv[arg][0] != '*') && (!findchan(argv[arg]))) {
	Tcl_AppendResult(irp, "invalid channel", NULL);
	return TCL_ERROR;
      }
      strncpy(dcc[i].u.chat->con_chan, argv[arg], 80);
      dcc[i].u.chat->con_chan[80] = 0;
    } else {
      if ((argv[arg][0] != '+') && (argv[arg][0] != '-'))
	dcc[i].u.chat->con_flags = 0;
      for (j = 0; j < strlen(argv[arg]); j++) {
	if (argv[arg][j] == '+')
	  pls = 1;
	else if (argv[arg][j] == '-')
	  pls = (-1);
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
  /* new style autosave -- drummer,07/25/1999*/
  if ((me = module_find("console", 0, 0))) {
    Function *func = me->funcs;
    (func[4]) (i);
  }
  return TCL_OK;
}

static int tcl_strip STDVAR
{
  int i, j, pls, arg;

  context;
  BADARGS(2, 4, " idx ?strip-flags?");
  j = atoi(argv[1]);
  i = findidx(j);
  if (i < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[i].type != &DCC_CHAT) {
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
	pls = (-1);
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
  return TCL_OK;
}

static int tcl_echo STDVAR
{
  int i, j;

  context;
  BADARGS(2, 3, " idx ?status?");
  j = atoi(argv[1]);
  i = findidx(j);
  if (i < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[i].type != &DCC_CHAT) {
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
  return TCL_OK;
}

static int tcl_page STDVAR
{
  int i, j;
  char x[20];

  context;
  BADARGS(2, 3, " idx ?status?");
  j = atoi(argv[1]);
  i = findidx(j);
  if (i < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[i].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (argc == 3) {
    int l = atoi(argv[2]);

    if (l == 0)
      dcc[i].status &= ~STAT_PAGE;
    else {
      dcc[i].status |= STAT_PAGE;
      dcc[i].u.chat->max_line = l;
    }
  }
  if (dcc[i].status & STAT_PAGE) {
    sprintf(x, "%d", dcc[i].u.chat->max_line);
    Tcl_AppendResult(irp, x, NULL);
  } else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_control STDVAR
{
  int idx, i;
  void *hold;

  context;
  BADARGS(3, 3, " idx command");
  i = atoi(argv[1]);
  idx = findidx(i);
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
  strncpy(dcc[idx].u.script->command, argv[2], 120);
  dcc[idx].u.script->command[119] = 0;
  return TCL_OK;
}

static int tcl_valididx STDVAR
{
  int idx;

  context;
  BADARGS(2, 2, " idx");
  idx = findidx(atoi(argv[1]));
  if ((idx < 0) || !(dcc[idx].type->flags & DCT_VALIDIDX))
     Tcl_AppendResult(irp, "0", NULL);
  else
     Tcl_AppendResult(irp, "1", NULL);
   return TCL_OK;
}

static int tcl_killdcc STDVAR
{
  int idx, i;

  contextnote(argv[1] ? argv[1] : "argv[1] == NULL!");
  BADARGS(2, 3, " idx ?reason?");
  i = atoi(argv[1]);
  idx = findidx(i);
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  context;

  /* don't kill terminal socket */
  if ((dcc[idx].sock == STDOUT) && !backgrd)
    return TCL_OK;
  /* make sure 'whom' info is updated for other bots */
  if (dcc[idx].type->flags & DCT_CHAT) {
    chanout_but(idx, dcc[idx].u.chat->channel, "*** %s has left the %s%s%s\n",
		dcc[idx].nick, dcc[idx].u.chat ? "channel" : "partyline",
		argc == 3 ? ": " : "", argc == 3 ? argv[2] : "");
    botnet_send_part_idx(idx, argc == 3 ? argv[2] : "");
    if ((dcc[idx].u.chat->channel >= 0) &&
	(dcc[idx].u.chat->channel < GLOBAL_CHANS))
      check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock,
		     dcc[idx].u.chat->channel);
    check_tcl_chof(dcc[idx].nick, dcc[idx].sock);
    /* no notice is sent to the party line -- that's the scripts' job */
    /* well now it does go through, but the script can add a reason */
  }
  killsock(dcc[idx].sock);
  lostdcc(idx);
  return TCL_OK;
}

static int tcl_putbot STDVAR
{
  int i;
  char msg[401];

  context;
  BADARGS(3, 3, " botnick message");
  i = nextbot(argv[1]);
  if (i < 0) {
    Tcl_AppendResult(irp, "bot is not in the botnet", NULL);
    return TCL_ERROR;
  }
  strncpy(msg, argv[2], 400);

  msg[400] = 0;
  botnet_send_zapf(i, botnetnick, argv[1], msg);
  return TCL_OK;
}

static int tcl_putallbots STDVAR
{
  char msg[401];

  context;
  BADARGS(2, 2, " message");
  strncpy(msg, argv[1], 400);
  msg[400] = 0;
  botnet_send_zapf_broad(-1, botnetnick, NULL, msg);
  return TCL_OK;
}

static int tcl_idx2hand STDVAR
{
  int i, idx;

  context;
  BADARGS(2, 2, " idx");
  i = atoi(argv[1]);
  idx = findidx(i);
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

  context;
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

  context;
  BADARGS(1, 1, "");
  for (bot = tandbot; bot; bot = bot->next)
     Tcl_AppendElement(irp, bot->bot);
   return TCL_OK;
}

static int tcl_botlist STDVAR
{
  tand_t *bot;
  char *list[4], *p;
  char sh[2], string[20];

  context;
  BADARGS(1, 1, "");
  sh[1] = 0;
  list[3] = sh;
  list[2] = string;
  for (bot = tandbot; bot; bot = bot->next) {
    list[0] = bot->bot;
    list[1] = (bot->uplink == (tand_t *) 1) ? botnetnick : bot->uplink->bot;
    strcpy(string, int_to_base10(bot->ver));
    sh[0] = bot->share;
    p = Tcl_Merge(4, list);
    Tcl_AppendElement(irp, p);
    n_free(p, "", 0);
  }
  return TCL_OK;
}

/* list of { idx nick host type {other}  timestamp} */

static int tcl_dcclist STDVAR
{
  int i;
  char idxstr[10];
  char timestamp[15]; /* when will unixtime ever be 14 numbers long */
  char *list[6], *p;
  char other[160];

  context;
  BADARGS(1, 2, " ?type?");
  for (i = 0; i < dcc_total; i++) {
    if ((argc == 1) ||
	(dcc[i].type && !strcasecmp(dcc[i].type->name, argv[1]))) {
      sprintf(idxstr, "%ld", dcc[i].sock);
      sprintf(timestamp, "%ld", dcc[i].timeval);
      if (dcc[i].type && dcc[i].type->display)
	dcc[i].type->display(i, other);
      else {
	sprintf(other, "?:%lX  !! ERROR !!", (long) dcc[i].type);
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
      n_free(p, "", 0);
    }
  }
  return TCL_OK;
}

/* list of { nick bot host flag idletime awaymsg [channel]} */
static int tcl_whom STDVAR
{
  char c[2], idle[10], work[20], *list[7], *p;
  int chan, i;

  context;
  BADARGS(2, 2, " chan");
  if (argv[1][0] == '*')
     chan = -1;
  else {
    if ((argv[1][0] < '0') || (argv[1][0] > '9')) {
      Tcl_SetVar(interp, "chan", argv[1], 0);
      if ((Tcl_VarEval(interp, "assoc ", "$chan", NULL) != TCL_OK) ||
	  !interp->result[0]) {
	Tcl_AppendResult(irp, "channel name is invalid", NULL);
	return TCL_ERROR;
      }
      chan = atoi(interp->result);
    } else
      chan = atoi(argv[1]);
    if ((chan < 0) || (chan > 199999)) {
      Tcl_AppendResult(irp, "channel out of range; must be 0 thru 199999",
		       NULL);
      return TCL_ERROR;
    }
  }
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_CHAT) {
      if ((dcc[i].u.chat->channel == chan) || (chan == -1)) {
	c[0] = geticon(i);
	c[1] = 0;
	sprintf(idle, "%lu", (now - dcc[i].timeval) / 60);
	list[0] = dcc[i].nick;
	list[1] = botnetnick;
	list[2] = dcc[i].host;
	list[3] = c;
	list[4] = idle;
	list[5] = dcc[i].u.chat->away ? dcc[i].u.chat->away : "";
	if (chan == -1) {
	  sprintf(work, "%d", dcc[i].u.chat->channel);
	  list[6] = work;
	}
	p = Tcl_Merge((chan == -1) ? 7 : 6, list);
	Tcl_AppendElement(irp, p);
	n_free(p, "", 0);
      }
    }
  for (i = 0; i < parties; i++) {
    if ((party[i].chan == chan) || (chan == -1)) {
      c[0] = party[i].flag;
      c[1] = 0;
      if (party[i].timer == 0L)
	strcpy(idle, "0");
      else
	sprintf(idle, "%lu", (now - party[i].timer) / 60);
      list[0] = party[i].nick;
      list[1] = party[i].bot;
      list[2] = party[i].from ? party[i].from : "";
      list[3] = c;
      list[4] = idle;
      list[5] = party[i].status & PLSTAT_AWAY ? party[i].away : "";
      if (chan == -1) {
	sprintf(work, "%d", party[i].chan);
	list[6] = work;
      }
      p = Tcl_Merge((chan == -1) ? 7 : 6, list);
      Tcl_AppendElement(irp, p);
      n_free(p, "", 0);
    }
  }
  return TCL_OK;
}

static int tcl_dccused STDVAR
{
  char s[20];

  context;
  BADARGS(1, 1, "");
  sprintf(s, "%d", dcc_total);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_getdccidle STDVAR
{
  int i, x, idx;
  char s[21];

  context;
  BADARGS(2, 2, " idx");
  i = atoi(argv[1]);
  idx = findidx(i);
  if ((idx < 0) || (dcc[idx].type == &DCC_SCRIPT)) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  x = (now - (dcc[idx].timeval));
  sprintf(s, "%d", x);
  Tcl_AppendElement(irp, s);
  return TCL_OK;
}

static int tcl_getdccaway STDVAR
{
  int i, idx;

  context;
  BADARGS(2, 2, " idx");
  i = atol(argv[1]);
  idx = findidx(i);
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[idx].type != &DCC_CHAT) {
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
  int i, idx;

  context;
  BADARGS(3, 3, " idx message");
  i = atol(argv[1]);
  idx = findidx(i);
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[idx].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (!argv[2][0]) {
    /* un-away */
    if (dcc[idx].u.chat->away != NULL)
      not_away(idx);
    return TCL_OK;
  }
  /* away */
  set_away(idx, argv[2]);
  return TCL_OK;
}

static int tcl_link STDVAR
{
  int x, i;
  char bot[HANDLEN + 1], bot2[HANDLEN + 1];

  context;
  BADARGS(2, 3, " ?via-bot? bot");
  strncpy(bot, argv[1], HANDLEN);
  bot[HANDLEN] = 0;
  if (argc == 2)
     x = botlink("", -2, bot);
  else {
    x = 1;
    strncpy(bot2, argv[2], HANDLEN);
    bot2[HANDLEN] = 0;
    i = nextbot(bot);
    if (i < 0)
      x = 0;
    else
      botnet_send_link(i, botnetnick, bot, bot2);
  }
  sprintf(bot, "%d", x);
  Tcl_AppendResult(irp, bot, NULL);
  return TCL_OK;
}

static int tcl_unlink STDVAR
{
  int i, x;
  char bot[HANDLEN + 1];

  context;
  BADARGS(2, 3, " bot ?comment?");
  strncpy(bot, argv[1], HANDLEN);
  bot[HANDLEN] = 0;
  i = nextbot(bot);
  if (i < 0)
     x = 0;
  else {
    x = 1;
    if (!strcasecmp(bot, dcc[i].nick))
      x = botunlink(-2, bot, argv[2]);
    else
      botnet_send_unlink(i, botnetnick, lastbot(bot), bot, argv[2]);
  }
  sprintf(bot, "%d", x);
  Tcl_AppendResult(irp, bot, NULL);
  return TCL_OK;
}

static int tcl_connect STDVAR
{
  int i, z, sock;
  char s[81];

  context;
  BADARGS(3, 3, " hostname port");
  if (dcc_total == max_dcc) {
    Tcl_AppendResult(irp, "out of dcc table space", NULL);
    return TCL_ERROR;
  }
  i = dcc_total;
  sock = getsock(0);
  z = open_telnet_raw(sock, argv[1], atoi(argv[2]));
  if (z < 0) {
    if (z == (-2))
      strcpy(s, "DNS lookup failed");
    else
      neterror(s);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_ERROR;
  }
  /* well well well... it worked! */
  i = new_dcc(&DCC_SOCKET, 0);
  dcc[i].sock = sock;
  dcc[i].port = atoi(argv[2]);
  strcpy(dcc[i].nick, "*");
  strncpy(dcc[i].host, argv[1], UHOSTLEN);
  dcc[i].host[UHOSTLEN] = 0;
  sprintf(s, "%d", sock);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

/* create a new listening port (or destroy one) */
/* listen <port> bots/all/users [mask]
 * listen <port> script <proc>
 * listen <port> off */
static int tcl_listen STDVAR
{
  int i, j, idx = (-1), port, realport;
  char s[10];
  struct portmap *pmap = NULL, *pold = NULL;

  context;
  BADARGS(3, 4, " port type ?mask/proc?");
  port = realport = atoi(argv[1]);
  for (pmap = root; pmap; pold = pmap, pmap = pmap->next)
    if (pmap->realport == port) {
      port = pmap->mappedto;
      break;
    }
  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_TELNET) && (dcc[i].port == port))
      idx = i;
  if (!strcasecmp(argv[2], "off")) {
    if (pmap) {
      if (pold)
	pold->next = pmap->next;
      else
	root = pmap->next;
      nfree(pmap);
    }
    /* remove */
    if (idx < 0) {
      Tcl_AppendResult(irp, "no such listen port is open", NULL);
      return TCL_ERROR;
    }
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return TCL_OK;
  }
  if (idx < 0) {
    /* make new one */
    if (dcc_total >= max_dcc) {
      Tcl_AppendResult(irp, "no more DCC slots available", NULL);
      return TCL_ERROR;
    }
    /* try to grab port */
    j = port + 20;
    i = (-1);
    while ((port < j) && (i < 0)) {
      i = open_listen(&port);
      if (i < 0)
	port++;
    }
    if (i < 0) {
      Tcl_AppendResult(irp, "couldn't grab nearby port", NULL);
      return TCL_ERROR;
    }
    idx = new_dcc(&DCC_TELNET, 0);
    dcc[idx].addr = iptolong(getmyip());
    dcc[idx].port = port;
    dcc[idx].sock = i;
  }
  /* script? */
  if (!strcasecmp(argv[2], "script")) {
    strcpy(dcc[idx].nick, "(script)");
    if (argc < 4) {
      Tcl_AppendResult(irp, "must give proc name for script listen", NULL);
      killsock(dcc[idx].sock);
      dcc_total--;
      return TCL_ERROR;
    }
    strncpy(dcc[idx].host, argv[3], UHOSTLEN - 1);
    dcc[idx].host[UHOSTLEN - 1] = 0;
    sprintf(s, "%d", port);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  }
  /* bots/users/all */
  if (!strcasecmp(argv[2], "bots"))
    strcpy(dcc[idx].nick, "(bots)");
  else if (!strcasecmp(argv[2], "users"))
    strcpy(dcc[idx].nick, "(users)");
  else if (!strcasecmp(argv[2], "all"))
    strcpy(dcc[idx].nick, "(telnet)");
  if (!dcc[idx].nick[0]) {
    Tcl_AppendResult(irp, "illegal listen type: must be one of ",
		     "bots, users, all, off, script", NULL);
    killsock(dcc[idx].sock);
    dcc_total--;
    return TCL_ERROR;
  }
  if (argc == 4) {
    strncpy(dcc[idx].host, argv[3], UHOSTLEN - 1);
    dcc[idx].host[UHOSTLEN - 1] = 0;
  } else
    strcpy(dcc[idx].host, "*");
  sprintf(s, "%d", port);
  Tcl_AppendResult(irp, s, NULL);
  if (!pmap) {
    pmap = nmalloc(sizeof(struct portmap));

    pmap->next = root;
    root = pmap;
  }
  pmap->realport = realport;
  pmap->mappedto = port;
  putlog(LOG_MISC, "*", "Listening at telnet port %d (%s)", port, argv[2]);
  return TCL_OK;
}

static int tcl_boot STDVAR
{
  char who[512];
  int i, ok = 0;

  context;
  BADARGS(2, 3, " user@bot ?reason?");
  strcpy(who, argv[1]);
  if (strchr(who, '@') != NULL) {
    char whonick[161];
     splitc(whonick, who, '@');
     whonick[161] = 0;
    if (!strcasecmp(who, botnetnick))
       strcpy(who, whonick);
    else if (remote_boots > 1) {
      i = nextbot(who);
      if (i < 0)
	return TCL_OK;
      botnet_send_reject(i, botnetnick, NULL, whonick, who, argv[2]);
    } else {
      return TCL_OK;
    }
  }
  for (i = 0; i < dcc_total; i++)
    if ((!strcasecmp(dcc[i].nick, who)) && !ok &&
	(dcc[i].type->flags & DCT_CANBOOT)) {
      do_boot(i, botnetnick, argv[2] ? argv[2] : "");
      ok = 1;
    }
  return TCL_OK;
}

static int tcl_rehash STDVAR
{
  context;
  BADARGS(1, 1, " ");
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
  context;
  BADARGS(1, 1, " ");
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

tcl_cmds tcldcc_cmds[] =
{
  {"putdcc", tcl_putdcc},
  {"putdccraw", tcl_putdccraw},
  {"putidx", tcl_putdcc},
  {"dccsimul", tcl_dccsimul},
  {"dccbroadcast", tcl_dccbroadcast},
  {"hand2idx", tcl_hand2idx},
  {"getchan", tcl_getchan},
  {"setchan", tcl_setchan},
  {"dccputchan", tcl_dccputchan},
  {"console", tcl_console},
  {"strip", tcl_strip},
  {"echo", tcl_echo},
  {"page", tcl_page},
  {"control", tcl_control},
  {"valididx", tcl_valididx},
  {"killdcc", tcl_killdcc},
  {"putbot", tcl_putbot},
  {"putallbots", tcl_putallbots},
  {"idx2hand", tcl_idx2hand},
  {"bots", tcl_bots},
  {"botlist", tcl_botlist},
  {"dcclist", tcl_dcclist},
  {"whom", tcl_whom},
  {"dccused", tcl_dccused},
  {"getdccidle", tcl_getdccidle},
  {"getdccaway", tcl_getdccaway},
  {"setdccaway", tcl_setdccaway},
  {"islinked", tcl_islinked},
  {"link", tcl_link},
  {"unlink", tcl_unlink},
  {"connect", tcl_connect},
  {"listen", tcl_listen},
  {"boot", tcl_boot},
  {"rehash", tcl_rehash},
  {"restart", tcl_restart},
  {0, 0}
};
