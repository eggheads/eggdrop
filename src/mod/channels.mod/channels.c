/* 
 * channels.c - part of channels.mod
 * support for channels within the bot
 */
/* 
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

#define MODULE_NAME "channels"
#define MAKING_CHANNELS
#include "../module.h"
#include <sys/stat.h>

static int setstatic = 0;
static int use_info = 1;
static int ban_time = 60;
static int exempt_time = 0;	/* if exempt_time = 0, never remove them */
static int invite_time = 0;	/* if invite_time = 0, never remove them */
static char chanfile[121] = "chanfile";
static Function *global = NULL;
static int chan_hack = 0;
static int must_be_owner = 0;
static int quiet_save = 0;

/* global channel settings (drummer/dw) */
static char glob_chanset[512] = "\
-clearbans -enforcebans +dynamicbans +userbans -autoop -bitch +greet \
+protectops +statuslog +stopnethack -revenge -secret -autovoice +cycle \
+dontkickops -wasoptest -inactive -protectfriends +shared -seen \
+userexempts +dynamicexempts +userinvites +dynamicinvites -revengebot ";
/* DO NOT remove the extra space at the end of the string! */

/* default chanmode (drummer,990731) */
static char glob_chanmode[64] = "nt";

/* global flood settings */
static int gfld_chan_thr;
static int gfld_chan_time;
static int gfld_deop_thr;
static int gfld_deop_time;
static int gfld_kick_thr;
static int gfld_kick_time;
static int gfld_join_thr;
static int gfld_join_time;
static int gfld_ctcp_thr;
static int gfld_ctcp_time;

static void remove_channel(struct chanset_t *);

#include "channels.h"
#include "cmdschan.c"
#include "tclchan.c"
#include "userchan.c"

void *channel_malloc(int size, char *file, int line)
{
  char *p;

#ifdef DEBUG_MEM
  p = ((void *) (global[0] (size, MODULE_NAME, file, line)));
#else
  p = nmalloc(size);
#endif
  bzero(p, size);
  return p;
}

static void set_mode_protect(struct chanset_t *chan, char *set)
{
  int i, pos = 1;
  char *s, *s1;

  /* clear old modes */
  chan->mode_mns_prot = chan->mode_pls_prot = 0;
  chan->limit_prot = (-1);
  chan->key_prot[0] = 0;
  for (s = newsplit(&set); *s; s++) {
    i = 0;
    switch (*s) {
    case '+':
      pos = 1;
      break;
    case '-':
      pos = 0;
      break;
    case 'i':
      i = CHANINV;
      break;
    case 'p':
      i = CHANPRIV;
      break;
    case 's':
      i = CHANSEC;
      break;
    case 'm':
      i = CHANMODER;
      break;
    case 't':
      i = CHANTOPIC;
      break;
    case 'n':
      i = CHANNOMSG;
      break;
    case 'a':
      i = CHANANON;
      break;
    case 'q':
      i = CHANQUIET;
      break;
    case 'l':
      i = CHANLIMIT;
      chan->limit_prot = (-1);
      if (pos) {
	s1 = newsplit(&set);
	if (s1[0])
	  chan->limit_prot = atoi(s1);
      }
      break;
    case 'k':
      i = CHANKEY;
      chan->key_prot[0] = 0;
      if (pos) {
	s1 = newsplit(&set);
	if (s1[0])
	  strcpy(chan->key_prot, s1);
      }
      break;
    }
    if (i) {
      if (pos) {
	chan->mode_pls_prot |= i;
	chan->mode_mns_prot &= ~i;
      } else {
	chan->mode_pls_prot &= ~i;
	chan->mode_mns_prot |= i;
      }
    }
  }
  /* drummer: +s-p +p-s flood fixed. */
  if (chan->mode_pls_prot & CHANSEC)
    chan->mode_pls_prot &= ~CHANPRIV;
}

static void get_mode_protect(struct chanset_t *chan, char *s)
{
  char *p = s, s1[121];
  int ok = 0, i, tst;

  s1[0] = 0;
  for (i = 0; i < 2; i++) {
    ok = 0;
    if (i == 0) {
      tst = chan->mode_pls_prot;
      if ((tst) || (chan->limit_prot != (-1)) || (chan->key_prot[0]))
	*p++ = '+';
      if (chan->limit_prot != (-1)) {
	*p++ = 'l';
	sprintf(&s1[strlen(s1)], "%d ", chan->limit_prot);
      }
      if (chan->key_prot[0]) {
	*p++ = 'k';
	sprintf(&s1[strlen(s1)], "%s ", chan->key_prot);
      }
    } else {
      tst = chan->mode_mns_prot;
      if (tst)
	*p++ = '-';
      if (tst & CHANKEY)
	*p++ = 'k';
      if (tst & CHANLIMIT)
	*p++ = 'l';
    }
    if (tst & CHANINV)
      *p++ = 'i';
    if (tst & CHANPRIV)
      *p++ = 'p';
    if (tst & CHANSEC)
      *p++ = 's';
    if (tst & CHANMODER)
      *p++ = 'm';
    if (tst & CHANTOPIC)
      *p++ = 't';
    if (tst & CHANNOMSG)
      *p++ = 'n';
    if (tst & CHANANON)
      *p++ = 'a';
    if (tst & CHANQUIET)
      *p++ = 'q';
  }
  *p = 0;
  if (s1[0]) {
    s1[strlen(s1) - 1] = 0;
    strcat(s, " ");
    strcat(s, s1);
  }
}

/* returns true if this is one of the channel masks */
static int ismodeline(masklist *m, char *user)
{
  while (m && m->mask[0]) {
    if (!rfc_casecmp(m->mask, user))
      return 1;
    m = m->next;
  }

  return 0;
}

/* returns true if user matches one of the masklist -- drummer */
static int ismasked(masklist *m, char *user)
{
  while (m && m->mask[0]) {
    if (wild_match(m->mask, user))
      return 1;
    m = m->next;
  }

  return 0;
}

/* destroy a chanset in the list */
/* does NOT free up memory associated with channel data inside the chanset! */
static int killchanset(struct chanset_t *chan)
{
  struct chanset_t *c = chanset, *old = NULL;

  while (c) {
    if (c == chan) {
      if (old)
	old->next = c->next;
      else
	chanset = c->next;
      nfree(c);
      return 1;
    }
    old = c;
    c = c->next;
  }
  return 0;
}

/* Completely removes a channel.
 * This includes the removal of all channel-bans, -exempts and -invites, as
 * well as all user flags related to the channel.
 */
static void remove_channel(struct chanset_t *chan)
{
   clear_channel(chan, 0);
   noshare = 1;
   /* Remove channel-bans */
   while (chan->bans)
     u_delban(chan, chan->bans->mask, 1);
   /* Remove channel-exempts */
   while (chan->exempts)
     u_delexempt(chan, chan->exempts->mask, 1);
   /* Remove channel-invites */
   while (chan->invites)
     u_delinvite(chan, chan->invites->mask, 1);
   /* Remove channel specific user flags */
   user_del_chan(chan->name);
   noshare = 0;
   killchanset(chan);
}

/* bind this to chon and *if* the users console channel == ***
 * then set it to a specific channel */
static int channels_chon(char *handle, int idx)
{
  struct flag_record fr = {FR_CHAN | FR_ANYWH | FR_GLOBAL, 0, 0, 0, 0, 0};
  int find, found = 0;
  struct chanset_t *chan = chanset;

  if (dcc[idx].type == &DCC_CHAT) {
    if (!findchan(dcc[idx].u.chat->con_chan) &&
	((dcc[idx].u.chat->con_chan[0] != '*') ||
	 (dcc[idx].u.chat->con_chan[1] != 0))) {
      get_user_flagrec(dcc[idx].user, &fr, NULL);
      if (glob_op(fr))
	found = 1;
      if (chan_owner(fr))
	find = USER_OWNER;
      else if (chan_master(fr))
	find = USER_MASTER;
      else
	find = USER_OP;
      fr.match = FR_CHAN;
      while (chan && !found) {
	get_user_flagrec(dcc[idx].user, &fr, chan->name);
	if (fr.chan & find)
	  found = 1;
	else
	  chan = chan->next;
      }
      if (!chan)
	chan = chanset;
      if (chan)
	strcpy(dcc[idx].u.chat->con_chan, chan->name);
      else
	strcpy(dcc[idx].u.chat->con_chan, "*");
    }
  }
  return 0;
}

static char *convert_element(char *src, char *dst)
{
  int flags;

  Tcl_ScanElement(src, &flags);
  Tcl_ConvertElement(src, dst, flags);
  return dst;
}

#define PLSMNS(x) (x ? '+' : '-')

static void write_channels()
{
  FILE *f;
  char s[121], w[1024], w2[1024], name[163];
  char need1[242], need2[242], need3[242], need4[242], need5[242];
  struct chanset_t *chan;

  Context;
  if (!chanfile[0])
    return;
  sprintf(s, "%s~new", chanfile);
  f = fopen(s, "w");
  chmod(s, 0600);
  if (f == NULL) {
    putlog(LOG_MISC, "*", "ERROR writing channel file.");
    return;
  }
  if (!quiet_save)
    putlog(LOG_MISC, "*", "Writing channel file ...");
  fprintf(f, "#Dynamic Channel File for %s (%s) -- written %s\n",
	  origbotname, ver, ctime(&now));
  for (chan = chanset; chan; chan = chan->next) {
    convert_element(chan->name, name);
    get_mode_protect(chan, w);
    convert_element(w, w2);
    convert_element(chan->need_op, need1);
    convert_element(chan->need_invite, need2);
    convert_element(chan->need_key, need3);
    convert_element(chan->need_unban, need4);
    convert_element(chan->need_limit, need5);
    fprintf(f, "channel %s %s%schanmode %s idle-kick %d \
need-op %s need-invite %s need-key %s need-unban %s need-limit %s \
flood-chan %d:%d flood-ctcp %d:%d flood-join %d:%d \
flood-kick %d:%d flood-deop %d:%d \
%cclearbans %cenforcebans %cdynamicbans %cuserbans %cautoop %cbitch \
%cgreet %cprotectops %cprotectfriends %cdontkickops %cwasoptest \
%cstatuslog %cstopnethack %crevenge %crevengebot %cautovoice %csecret \
%cshared %ccycle %cseen %cinactive %cdynamicexempts %cuserexempts \
%cdynamicinvites %cuserinvites%s\n",
	channel_static(chan) ? "set" : "add",
	name,
	channel_static(chan) ? " " : " { ",
	w2,
/* now bot write chanmode "" too,
 * so bot wont use default-chanmode instead of "" -- bugfix
 */
	chan->idle_kick, /* idle-kick 0 is same as dont-idle-kick (less code)*/
	need1, need2, need3, need4, need5,
/* yes we will write empty need-xxxx too, why not? (less code + lazyness) */
	chan->flood_pub_thr, chan->flood_pub_time,
        chan->flood_ctcp_thr, chan->flood_ctcp_time,
        chan->flood_join_thr, chan->flood_join_time,
        chan->flood_kick_thr, chan->flood_kick_time,
        chan->flood_deop_thr, chan->flood_deop_time,
	PLSMNS(channel_clearbans(chan)),
	PLSMNS(channel_enforcebans(chan)),
	PLSMNS(channel_dynamicbans(chan)),
	PLSMNS(!channel_nouserbans(chan)),
	PLSMNS(channel_autoop(chan)),
	PLSMNS(channel_bitch(chan)),
	PLSMNS(channel_greet(chan)),
	PLSMNS(channel_protectops(chan)),
        PLSMNS(channel_protectfriends(chan)),
	PLSMNS(channel_dontkickops(chan)),
	PLSMNS(channel_wasoptest(chan)),
	PLSMNS(channel_logstatus(chan)),
	PLSMNS(channel_stopnethack(chan)),
	PLSMNS(channel_revenge(chan)),
	PLSMNS(channel_revengebot(chan)),
	PLSMNS(channel_autovoice(chan)),
	PLSMNS(channel_secret(chan)),
	PLSMNS(channel_shared(chan)),
	PLSMNS(channel_cycle(chan)),
	PLSMNS(channel_seen(chan)),
	PLSMNS(channel_inactive(chan)),
        PLSMNS(channel_dynamicexempts(chan)),
        PLSMNS(!channel_nouserexempts(chan)),
 	PLSMNS(channel_dynamicinvites(chan)),
        PLSMNS(!channel_nouserinvites(chan)),
	channel_static(chan) ? "" : " }");
    if (fflush(f)) {
      putlog(LOG_MISC, "*", "ERROR writing channel file.");
      fclose(f);
      return;
    }
  }
  fclose(f);
  unlink(chanfile);
  movefile(s, chanfile);
}

static void read_channels(int create)
{
  struct chanset_t *chan, *chan2;

  if (!chanfile[0])
    return;
  for (chan = chanset; chan; chan = chan->next)
    if (!channel_static(chan))
      chan->status |= CHAN_FLAGGED;
  chan_hack = 1;
  if (!readtclprog(chanfile) && create) {
    FILE *f;

    /* assume file isnt there & therfore make it */
    putlog(LOG_MISC, "*", "Creating channel file");
    f = fopen(chanfile, "w");
    if (!f)
      putlog(LOG_MISC, "*", "Couldn't create channel file: %s.  Dropping",
	     chanfile);
    else fclose(f);
  }
  chan_hack = 0;
  chan = chanset;
  while (chan != NULL) {
    if (chan->status & CHAN_FLAGGED) {
      putlog(LOG_MISC, "*", "No longer supporting channel %s", chan->name);
      if (!channel_inactive(chan))
	dprintf(DP_SERVER, "PART %s\n", chan->name);
      chan2 = chan->next;
      remove_channel(chan);
      chan = chan2;
    } else
      chan = chan->next;
  }
}

static void channels_prerehash()
{
  struct chanset_t *chan;

  for (chan = chanset; chan; chan = chan->next) {
    chan->status |= CHAN_FLAGGED;
    /* flag will be cleared as the channels are re-added by the config file */
    /* any still flagged afterwards will be removed */
    if (chan->status & CHAN_STATIC)
      chan->status &= ~CHAN_STATIC;
    /* flag is added to channels read from config file */
  }
  setstatic = 1;
}

static void channels_rehash()
{
  struct chanset_t *chan;

  setstatic = 0;
  read_channels(1);
  /* remove any extra channels */
  chan = chanset;
  while (chan) {
    if (chan->status & CHAN_FLAGGED) {
      putlog(LOG_MISC, "*", "No longer supporting channel %s", chan->name);
      if (!channel_inactive(chan))
	dprintf(DP_SERVER, "PART %s\n", chan->name);
      remove_channel(chan);
      chan = chanset;
    } else
      chan = chan->next;
  }
}

/* update the add/rem_builtins in channels.c if you add to this list!! */
static cmd_t my_chon[] =
{
  {"*", "", (Function) channels_chon, "channels:chon"},
  {0, 0, 0, 0}
};

static void channels_report(int idx, int details)
{
  struct chanset_t *chan;
  int i;
  char s[1024], s2[100];
  struct flag_record fr =
  {FR_CHAN | FR_GLOBAL, 0, 0, 0, 0, 0};

  chan = chanset;
  while (chan != NULL) {
    if (idx != DP_STDOUT)
      get_user_flagrec(dcc[idx].user, &fr, chan->name);
    if ((idx == DP_STDOUT) || glob_master(fr) || chan_master(fr)) {
      s[0] = 0;
      if (channel_greet(chan))
	strcat(s, "greet, ");
      if (channel_autoop(chan))
	strcat(s, "auto-op, ");
      if (channel_bitch(chan))
	strcat(s, "bitch, ");
      if (s[0])
	s[strlen(s) - 2] = 0;
      if (!s[0])
	strcpy(s, MISC_LURKING);
      get_mode_protect(chan, s2);
      if (!channel_inactive(chan)) {
	if (channel_active(chan)) {
	  dprintf(idx, "    %-10s: %2d member%s enforcing \"%s\" (%s)\n", chan->name,
		  chan->channel.members, chan->channel.members == 1 ? "," : "s,", s2, s);
	} else {
	  dprintf(idx, "    %-10s: (%s), enforcing \"%s\"  (%s)\n", chan->name,
		  channel_pending(chan) ? "pending" : "inactive", s2, s);
	}
      } else {
	dprintf(idx, "    %-10s: no IRC support for this channel\n", chan->name);
      }
      if (details) {
	s[0] = 0;
	i = 0;
	if (channel_clearbans(chan))
	  i += my_strcpy(s + i, "clear-bans ");
	if (channel_enforcebans(chan))
	  i += my_strcpy(s + i, "enforce-bans ");
	if (channel_dynamicbans(chan))
	  i += my_strcpy(s + i, "dynamic-bans ");
	if (channel_nouserbans(chan))
	  i += my_strcpy(s + i, "forbid-user-bans ");
	if (channel_autoop(chan))
	  i += my_strcpy(s + i, "op-on-join ");
	if (channel_bitch(chan))
	  i += my_strcpy(s + i, "bitch ");
	if (channel_greet(chan))
	  i += my_strcpy(s + i, "greet ");
	if (channel_protectops(chan))
	  i += my_strcpy(s + i, "protect-ops ");
        if (channel_protectfriends(chan))
          i += my_strcpy(s + i, "protect-friends ");
	if (channel_dontkickops(chan))
	  i += my_strcpy(s + i, "dont-kick-ops ");
	if (channel_wasoptest(chan))
	  i += my_strcpy(s + i, "was-op-test ");
	if (channel_logstatus(chan))
	  i += my_strcpy(s + i, "log-status ");
	if (channel_revenge(chan))
	  i += my_strcpy(s + i, "revenge ");
	if (channel_stopnethack(chan))
	  i += my_strcpy(s + i, "stopnethack ");
	if (channel_secret(chan))
	  i += my_strcpy(s + i, "secret ");
	if (channel_shared(chan))
	  i += my_strcpy(s + i, "shared ");
	if (!channel_static(chan))
	  i += my_strcpy(s + i, "dynamic ");
	if (channel_autovoice(chan))
	  i += my_strcpy(s + i, "autovoice ");
	if (channel_cycle(chan))
	  i += my_strcpy(s + i, "cycle ");
	if (channel_seen(chan))
	  i += my_strcpy(s + i, "seen ");
	if (channel_dynamicexempts(chan))
	  i += my_strcpy(s + i, "dynamic-exempts ");
	if (channel_nouserexempts(chan))
	  i += my_strcpy(s + i, "forbid-user-exempts ");
	if (channel_dynamicinvites(chan))
	  i += my_strcpy(s + i, "dynamic-invites ");
	if (channel_nouserinvites(chan))
	  i += my_strcpy(s + i, "forbid-user-invites ");
	if (channel_inactive(chan))
	  i += my_strcpy(s + i, "inactive ");
	dprintf(idx, "      Options: %s\n", s);
	if (chan->need_op[0])
	  dprintf(idx, "      To get ops I do: %s\n", chan->need_op);
	if (chan->need_invite[0])
	  dprintf(idx, "      To get invited I do: %s\n", chan->need_invite);
	if (chan->need_limit[0])
	  dprintf(idx, "      To get the channel limit up'd I do: %s\n", chan->need_limit);
	if (chan->need_unban[0])
	  dprintf(idx, "      To get unbanned I do: %s\n", chan->need_unban);
	if (chan->need_key[0])
	  dprintf(idx, "      To get the channel key I do: %s\n", chan->need_key);
	if (chan->idle_kick)
	  dprintf(idx, "      Kicking idle users after %d min\n", chan->idle_kick);
      }
    }
    chan = chan->next;
  }
  if (details) {
    dprintf(idx, "    Bans last %d mins.\n", ban_time);
    dprintf(idx, "    Exemptions last %d mins.\n", exempt_time);
    dprintf(idx, "    Invitations last %d mins.\n", invite_time);
  }
}

static int expmem_masklist(masklist *m)
{
  int result = 0;

  while (m) {
    result += sizeof(masklist);
    if (m->mask)
        result += strlen(m->mask) + 1;
    if (m->who)
        result += strlen(m->who) + 1;

    m = m->next;
  }

  return result;
}

static int channels_expmem()
{
  int tot = 0;
  struct chanset_t *chan = chanset;

  Context;
  while (chan != NULL) {
    tot += sizeof(struct chanset_t);

    tot += strlen(chan->channel.key) + 1;
    if (chan->channel.topic)
      tot += strlen(chan->channel.topic) + 1;
    tot += (sizeof(struct memstruct) * (chan->channel.members + 1));

    tot += expmem_masklist(chan->channel.ban);
    tot += expmem_masklist(chan->channel.exempt);
    tot += expmem_masklist(chan->channel.invite);

    chan = chan->next;
  }
  return tot;
}

static char *traced_globchanset(ClientData cdata, Tcl_Interp * irp,
				char *name1, char *name2, int flags)
{
  char *s;
  char *t;
  int i;
  int items;
  char **item;

  Context;
  if (flags & (TCL_TRACE_READS | TCL_TRACE_UNSETS)) {
    Tcl_SetVar2(interp, name1, name2, glob_chanset, TCL_GLOBAL_ONLY);
    if (flags & TCL_TRACE_UNSETS)
      Tcl_TraceVar(interp, "global-chanset",
	    TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
	    traced_globchanset, NULL);
  } else { /* write */
    s = Tcl_GetVar2(interp, name1, name2, TCL_GLOBAL_ONLY);
    Tcl_SplitList(interp, s, &items, &item);
    Context;
    for (i = 0; i<items; i++) {
      if (!(item[i]) || (strlen(item[i]) < 2)) continue;
      s = glob_chanset;
      while (s[0]) {
	t = strchr(s, ' '); /* cant be NULL coz of the extra space */
	Context;
	t[0] = 0;
	if (!strcmp(s + 1, item[i] + 1)) {
	  s[0] = item[i][0]; /* +- */
	  t[0] = ' ';
	  break;
	}
	t[0] = ' ';
	s = t + 1;
      }
    }
    if (item) /* hmm it cant be 0 */
      Tcl_Free((char *) item);
    Tcl_SetVar2(interp, name1, name2, glob_chanset, TCL_GLOBAL_ONLY);
  }
  return NULL;
}

static tcl_ints my_tcl_ints[] =
{
  {"share-greet", 0, 0},
  {"use-info", &use_info, 0},
  {"ban-time", &ban_time, 0},
  {"exempt-time", &exempt_time, 0},
  {"invite-time", &invite_time, 0},
  {"must-be-owner", &must_be_owner, 0},
  {"quiet-save", &quiet_save, 0},
  {0, 0, 0}
};

static tcl_coups mychan_tcl_coups[] =
{
  {"global-flood-chan", &gfld_chan_thr, &gfld_chan_time},
  {"global-flood-deop", &gfld_deop_thr, &gfld_deop_time},
  {"global-flood-kick", &gfld_kick_thr, &gfld_kick_time},
  {"global-flood-join", &gfld_join_thr, &gfld_join_time},
  {"global-flood-ctcp", &gfld_ctcp_thr, &gfld_ctcp_time},
  {0, 0, 0}
};

static tcl_strings my_tcl_strings[] =
{
  {"chanfile", chanfile, 120, STR_PROTECT},
  {"global-chanmode", glob_chanmode, 64, 0},
  {0, 0, 0, 0}
};

static char *channels_close()
{
  Context;
  write_channels();
  rem_builtins(H_chon, my_chon);
  rem_builtins(H_dcc, C_dcc_irc);
  rem_tcl_commands(channels_cmds);
  rem_tcl_strings(my_tcl_strings);
  rem_tcl_ints(my_tcl_ints);
  rem_tcl_coups(mychan_tcl_coups);
  del_hook(HOOK_USERFILE, channels_writeuserfile);
  del_hook(HOOK_REHASH, channels_rehash);
  del_hook(HOOK_PRE_REHASH, channels_prerehash);
  del_hook(HOOK_MINUTELY, check_expired_bans);
  del_hook(HOOK_MINUTELY,check_expired_exempts);
  del_hook(HOOK_MINUTELY,check_expired_invites);
  Tcl_UntraceVar(interp, "global-chanset",
		 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		 traced_globchanset, NULL);
  rem_help_reference("channels.help");
  rem_help_reference("chaninfo.help");
  module_undepend(MODULE_NAME);
  return NULL;
}

char *channels_start();

static Function channels_table[] =
{
  /* 0 - 3 */
  (Function) channels_start,
  (Function) channels_close,
  (Function) channels_expmem,
  (Function) channels_report,
  /* 4 - 7 */
  (Function) u_setsticky_mask,
  (Function) u_delban,
  (Function) u_addban,
  (Function) write_bans,
  /* 8 - 11 */
  (Function) get_chanrec,
  (Function) add_chanrec,
  (Function) del_chanrec,
  (Function) set_handle_chaninfo,
  /* 12 - 15 */
  (Function) channel_malloc,
  (Function) u_match_mask,
  (Function) u_equals_mask,
  (Function) clear_channel,
  /* 16 - 19 */
  (Function) set_handle_laston,
  (Function) & ban_time,
  (Function) & use_info,
  (Function) get_handle_chaninfo,
  /* 20 - 23 */
  (Function) u_sticky_mask,
  (Function) ismasked,
  (Function) add_chanrec_by_handle,
/* *HOLE* channels_funcs[23] used to be isexempted() <cybah> */
  (Function) NULL,
  /* 24 - 27 */
  (Function) & exempt_time,
/* *HOLE* channels_funcs[25] used to be isinvited() <cybah> */
  (Function) NULL,
  (Function) & invite_time,
  (Function) NULL,
  /* 28 - 31 */
/* *HOLE* channels_funcs[28] used to be u_setsticky_exempt() <cybah> */
  (Function) NULL,
  (Function) u_delexempt,
  (Function) u_addexempt,
  (Function) NULL,
  /* 32 - 35 */
/* *HOLE* channels_funcs[32] used to be u_sticky_exempt() <cybah> */
  (Function) NULL,
  (Function) NULL,
  (Function) killchanset,
  (Function) u_delinvite,
  /* 36 - 39 */
  (Function) u_addinvite,
  (Function) tcl_channel_add,
  (Function) tcl_channel_modify,
  (Function) write_exempts,
  /* 40 - 43 */
  (Function) write_invites,
  (Function) ismodeline,
};

char *channels_start(Function * global_funcs)
{
  global = global_funcs;

  gfld_chan_thr = 10;
  gfld_chan_time = 60;
  gfld_deop_thr = 3;
  gfld_deop_time = 10;
  gfld_kick_thr = 3;
  gfld_kick_time = 10;
  gfld_join_thr = 5;
  gfld_join_time = 60;
  gfld_ctcp_thr = 5;
  gfld_ctcp_time = 60;
  Context;
  module_register(MODULE_NAME, channels_table, 1, 0);
  if (!module_depend(MODULE_NAME, "eggdrop", 104, 0))
    return "This module needs eggdrop1.4.0 or later";
  add_hook(HOOK_MINUTELY, check_expired_bans);
  add_hook(HOOK_MINUTELY,check_expired_exempts);
  add_hook(HOOK_MINUTELY,check_expired_invites);
  add_hook(HOOK_USERFILE, channels_writeuserfile);
  add_hook(HOOK_REHASH, channels_rehash);
  add_hook(HOOK_PRE_REHASH, channels_prerehash);
  Tcl_TraceVar(interp, "global-chanset",
	       TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
	       traced_globchanset, NULL);
  add_builtins(H_chon, my_chon);
  add_builtins(H_dcc, C_dcc_irc);
  add_tcl_commands(channels_cmds);
  add_tcl_strings(my_tcl_strings);
  add_help_reference("channels.help");
  add_help_reference("chaninfo.help");
  my_tcl_ints[0].val = &share_greet;
  add_tcl_ints(my_tcl_ints);
  add_tcl_coups(mychan_tcl_coups);
  read_channels(0);
  setstatic = 1;
  return NULL;
}
