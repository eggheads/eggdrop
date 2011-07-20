/*
 * channels.c -- part of channels.mod
 *   support for channels within the bot
 *
 * $Id: channels.c,v 1.109 2011/07/20 10:50:35 thommey Exp $
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

#define MODULE_NAME "channels"
#define MAKING_CHANNELS

#include <sys/stat.h>
#include "src/mod/module.h"

static Function *global = NULL;

static char chanfile[121], glob_chanmode[64];
static char *lastdeletedmask;

static struct udef_struct *udef;

static int use_info, chan_hack, quiet_save, global_revenge_mode,
           global_stopnethack_mode, global_idle_kick, global_aop_min,
           global_aop_max, global_ban_time, global_exempt_time,
           global_invite_time, global_ban_type, allow_ps;

/* Global channel settings (drummer/dw) */
static char glob_chanset[512];

/* Global flood settings */
static int gfld_chan_thr, gfld_chan_time, gfld_deop_thr, gfld_deop_time,
           gfld_kick_thr, gfld_kick_time, gfld_join_thr, gfld_join_time,
           gfld_ctcp_thr, gfld_ctcp_time, gfld_nick_thr, gfld_nick_time;

#include "channels.h"
#include "cmdschan.c"
#include "tclchan.c"
#include "userchan.c"
#include "udefchan.c"


static void *channel_malloc(int size, char *file, int line)
{
  char *p;

#ifdef DEBUG_MEM
  p = ((void *) (global[0] (size, MODULE_NAME, file, line)));
#else
  p = nmalloc(size);
#endif
  egg_bzero(p, size);
  return p;
}

static void set_mode_protect(struct chanset_t *chan, char *set)
{
  int i, pos = 1;
  char *s, *s1;

  /* Clear old modes */
  chan->mode_mns_prot = chan->mode_pls_prot = 0;
  chan->limit_prot = 0;
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
    case 'c':
      i = CHANNOCLR;
      break;
    case 'C':
      i = CHANNOCTCP;
      break;
    case 'R':
      i = CHANREGON;
      break;
    case 'M':
      i = CHANMODREG;
      break;
    case 'r':
      i = CHANLONLY;
      break;
    case 'D':
      i = CHANDELJN;
      break;
    case 'u':
      i = CHANSTRIP;
      break;
    case 'N':
      i = CHANNONOTC;
      break;
    case 'T':
      i = CHANNOAMSG;
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
      chan->limit_prot = 0;
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
  /* Prevents a +s-p +p-s flood  (fixed by drummer) */
  if (chan->mode_pls_prot & CHANSEC && !allow_ps)
    chan->mode_pls_prot &= ~CHANPRIV;
}

static void get_mode_protect(struct chanset_t *chan, char *s)
{
  char *p = s, s1[121];
  int i, tst;

  s1[0] = 0;
  for (i = 0; i < 2; i++) {
    if (i == 0) {
      tst = chan->mode_pls_prot;
      if ((tst) || (chan->limit_prot != 0) || (chan->key_prot[0]))
        *p++ = '+';
      if (chan->limit_prot != 0) {
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
    if (tst & CHANNOCLR)
      *p++ = 'c';
    if (tst & CHANNOCTCP)
      *p++ = 'C';
    if (tst & CHANREGON)
      *p++ = 'R';
    if (tst & CHANMODREG)
      *p++ = 'M';
    if (tst & CHANLONLY)
      *p++ = 'r';
    if (tst & CHANDELJN)
      *p++ = 'D';
    if (tst & CHANSTRIP)
      *p++ = 'u';
    if (tst & CHANNONOTC)
      *p++ = 'N';
    if (tst & CHANNOAMSG)
      *p++ = 'T';
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

/* Returns true if this is one of the channel masks
 */
static int ismodeline(masklist *m, char *user)
{
  for (; m && m->mask[0]; m = m->next)
    if (!rfc_casecmp(m->mask, user))
      return 1;
  return 0;
}

/* Returns true if user matches one of the masklist -- drummer
 */
static int ismasked(masklist *m, char *user)
{
  for (; m && m->mask[0]; m = m->next)
    if (match_addr(m->mask, user))
      return 1;
  return 0;
}

/* Unlink chanset element from chanset list.
 */
inline static int chanset_unlink(struct chanset_t *chan)
{
  struct chanset_t *c, *c_old = NULL;

  for (c = chanset; c; c_old = c, c = c->next) {
    if (c == chan) {
      if (c_old)
        c_old->next = c->next;
      else
        chanset = c->next;
      return 1;
    }
  }
  return 0;
}

/* Completely removes a channel.
 *
 * This includes the removal of all channel-bans, -exempts and -invites, as
 * well as all user flags related to the channel.
 */
static void remove_channel(struct chanset_t *chan)
{
  int i;
  module_entry *me;

  /* Remove the channel from the list, so that noone can pull it
   * away from under our feet during the check_tcl_part() call. */
  (void) chanset_unlink(chan);

  if ((me = module_find("irc", 1, 3)) != NULL)
    (me->funcs[IRC_DO_CHANNEL_PART]) (chan);

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
  user_del_chan(chan->dname);
  noshare = 0;
  nfree(chan->channel.key);
  for (i = 0; i < MODES_PER_LINE_MAX && chan->cmode[i].op; i++)
    nfree(chan->cmode[i].op);
  if (chan->key)
    nfree(chan->key);
  if (chan->rmkey)
    nfree(chan->rmkey);
  nfree(chan);
}

/* Bind this to chon and *if* the users console channel == ***
 * then set it to a specific channel
 */
static int channels_chon(char *handle, int idx)
{
  struct flag_record fr = { FR_CHAN | FR_ANYWH | FR_GLOBAL, 0, 0, 0, 0, 0 };
  int find, found = 0;
  struct chanset_t *chan = chanset;

  if (dcc[idx].type == &DCC_CHAT) {
    if (!findchan_by_dname(dcc[idx].u.chat->con_chan) &&
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
        get_user_flagrec(dcc[idx].user, &fr, chan->dname);
        if (fr.chan & find)
          found = 1;
        else
          chan = chan->next;
      }
      if (!chan)
        chan = chanset;
      if (chan)
        strcpy(dcc[idx].u.chat->con_chan, chan->dname);
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
/* Work around Tcl bug 3371644 (only present in 8.5.10) */
#ifdef TCL_DONT_QUOTE_HASH
  flags |= TCL_DONT_QUOTE_HASH;
#endif
  Tcl_ConvertElement(src, dst, flags);
  return dst;
}

#define PLSMNS(x) (x ? '+' : '-')

/*
 * Note:
 *  - We write chanmode "" too, so that the bot won't use default-chanmode
 *    instead of ""
 *  - We will write empty need-xxxx too, why not? (less code + lazyness)
 */
static void write_channels()
{
  FILE *f;
  char s[121], w[1024], w2[1024], name[163];
  char need1[242], need2[242], need3[242], need4[242], need5[242];
  struct chanset_t *chan;
  struct udef_struct *ul;

  if (!chanfile[0])
    return;
  sprintf(s, "%s~new", chanfile);
  f = fopen(s, "w");
  chmod(s, userfile_perm);
  if (f == NULL) {
    putlog(LOG_MISC, "*", "ERROR writing channel file.");
    return;
  }
  if (!quiet_save)
    putlog(LOG_MISC, "*", "Writing channel file...");
  fprintf(f, "#Dynamic Channel File for %s (%s) -- written %s\n",
          botnetnick, ver, ctime(&now));
  for (chan = chanset; chan; chan = chan->next) {
    convert_element(chan->dname, name);
    get_mode_protect(chan, w);
    convert_element(w, w2);
    convert_element(chan->need_op, need1);
    convert_element(chan->need_invite, need2);
    convert_element(chan->need_key, need3);
    convert_element(chan->need_unban, need4);
    convert_element(chan->need_limit, need5);
    fprintf(f,
            "channel add %s { chanmode %s idle-kick %d stopnethack-mode %d "
            "revenge-mode %d need-op %s need-invite %s need-key %s "
            "need-unban %s need-limit %s flood-chan %d:%d flood-ctcp %d:%d "
            "flood-join %d:%d flood-kick %d:%d flood-deop %d:%d "
            "flood-nick %d:%d aop-delay %d:%d ban-type %d ban-time %d "
            "exempt-time %d invite-time %d %cenforcebans %cdynamicbans "
            "%cuserbans %cautoop %cautohalfop %cbitch %cgreet %cprotectops "
            "%cprotecthalfops %cprotectfriends %cdontkickops %cstatuslog "
            "%crevenge %crevengebot %cautovoice %csecret %cshared %ccycle "
            "%cseen %cinactive %cdynamicexempts %cuserexempts %cdynamicinvites "
            "%cuserinvites %cnodesynch %cstatic }" "\n",
            name, w2, chan->idle_kick, chan->stopnethack_mode,
            chan->revenge_mode, need1, need2, need3, need4, need5,
            chan->flood_pub_thr, chan->flood_pub_time,
            chan->flood_ctcp_thr, chan->flood_ctcp_time,
            chan->flood_join_thr, chan->flood_join_time,
            chan->flood_kick_thr, chan->flood_kick_time,
            chan->flood_deop_thr, chan->flood_deop_time,
            chan->flood_nick_thr, chan->flood_nick_time,
            chan->aop_min, chan->aop_max, chan->ban_type, chan->ban_time,
            chan->exempt_time, chan->invite_time,
            PLSMNS(channel_enforcebans(chan)),
            PLSMNS(channel_dynamicbans(chan)),
            PLSMNS(!channel_nouserbans(chan)),
            PLSMNS(channel_autoop(chan)),
            PLSMNS(channel_autohalfop(chan)),
            PLSMNS(channel_bitch(chan)),
            PLSMNS(channel_greet(chan)),
            PLSMNS(channel_protectops(chan)),
            PLSMNS(channel_protecthalfops(chan)),
            PLSMNS(channel_protectfriends(chan)),
            PLSMNS(channel_dontkickops(chan)),
            PLSMNS(channel_logstatus(chan)),
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
            PLSMNS(channel_nodesynch(chan)),
            PLSMNS(channel_static(chan)));
    for (ul = udef; ul; ul = ul->next) {
      if (ul->defined && ul->name) {
        if (ul->type == UDEF_FLAG)
          fprintf(f, "channel set %s %c%s%s\n", name, getudef(ul->values,
                  chan->dname) ? '+' : '-', "udef-flag-", ul->name);
        else if (ul->type == UDEF_INT)
          fprintf(f, "channel set %s %s%s %d\n", name, "udef-int-", ul->name,
                  (int) getudef(ul->values, chan->dname));
        else if (ul->type == UDEF_STR) {
          char *p = (char *) getudef(ul->values, chan->dname);

          if (!p)
            p = "{}";

          fprintf(f, "channel set %s udef-str-%s %s\n", name, ul->name, p);
        } else
          debug1("UDEF-ERROR: unknown type %d", ul->type);
      }
    }
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

static void read_channels(int create, int reload)
{
  struct chanset_t *chan, *chan_next;

  if (!chanfile[0])
    return;

  if (reload)
    for (chan = chanset; chan; chan = chan->next)
      chan->status |= CHAN_FLAGGED;

  chan_hack = 1;
  if (!readtclprog(chanfile) && create) {
    FILE *f;

    /* Assume file isnt there & therfore make it */
    putlog(LOG_MISC, "*", "Creating channel file");
    f = fopen(chanfile, "w");
    if (!f)
      putlog(LOG_MISC, "*", "Couldn't create channel file: %s.  Dropping",
             chanfile);
    else
      fclose(f);
  }
  chan_hack = 0;
  if (!reload)
    return;
  for (chan = chanset; chan; chan = chan_next) {
    chan_next = chan->next;
    if (chan->status & CHAN_FLAGGED) {
      putlog(LOG_MISC, "*", "No longer supporting channel %s", chan->dname);
      remove_channel(chan);
    }
  }
}

static void backup_chanfile()
{
  char s[125];

  if (quiet_save < 2)
    putlog(LOG_MISC, "*", "Backing up channel file...");
  egg_snprintf(s, sizeof s, "%s~bak", chanfile);
  copyfile(chanfile, s);
}

static void channels_prerehash()
{
  write_channels();
}

static void channels_rehash()
{
  /* add channels from the chanfile but don't remove missing ones */
  read_channels(1, 0);
  write_channels();
}

static cmd_t my_chon[] = {
  {"*",  "",   (IntFunc) channels_chon, "channels:chon"},
  {NULL, NULL, NULL,                                NULL}
};

static void channels_report(int idx, int details)
{
  int i;
  char s[1024], s1[100], s2[100];
  struct chanset_t *chan;
  struct flag_record fr = { FR_CHAN | FR_GLOBAL, 0, 0, 0, 0, 0 };

  for (chan = chanset; chan; chan = chan->next) {

    /* Get user's flags if output isn't going to stdout */
    if (idx != DP_STDOUT)
      get_user_flagrec(dcc[idx].user, &fr, chan->dname);

    /* Don't show channel information to someone who isn't a master */
    if ((idx != DP_STDOUT) && !glob_master(fr) && !chan_master(fr))
      continue;

    s[0] = 0;

    sprintf(s, "    %-20s: ", chan->dname);

    if (channel_inactive(chan))
      strcat(s, "(inactive)");
    else if (channel_pending(chan))
      strcat(s, "(pending)");
    else if (!channel_active(chan))
      strcat(s, "(not on channel)");
    else {

      s1[0] = 0;
      sprintf(s1, "%3d member%s", chan->channel.members,
              (chan->channel.members == 1) ? "" : "s");
      strcat(s, s1);

      s2[0] = 0;
      get_mode_protect(chan, s2);

      if (s2[0]) {
        s1[0] = 0;
        sprintf(s1, ", enforcing \"%s\"", s2);
        strcat(s, s1);
      }

      s2[0] = 0;

      if (channel_greet(chan))
        strcat(s2, "greet, ");
      if (channel_autoop(chan))
        strcat(s2, "auto-op, ");
      if (channel_bitch(chan))
        strcat(s2, "bitch, ");

      if (s2[0]) {
        s2[strlen(s2) - 2] = 0;

        s1[0] = 0;
        sprintf(s1, " (%s)", s2);
        strcat(s, s1);
      }

      /* If it's a !chan, we want to display it's unique name too <cybah> */
      if (chan->dname[0] == '!') {
        s1[0] = 0;
        sprintf(s1, ", unique name %s", chan->name);
        strcat(s, s1);
      }
    }

    dprintf(idx, "%s\n", s);

    if (details) {
      s[0] = 0;
      i = 0;

      if (channel_enforcebans(chan))
        i += my_strcpy(s + i, "enforcebans ");
      if (channel_dynamicbans(chan))
        i += my_strcpy(s + i, "dynamicbans ");
      if (!channel_nouserbans(chan))
        i += my_strcpy(s + i, "userbans ");
      if (channel_autoop(chan))
        i += my_strcpy(s + i, "autoop ");
      if (channel_bitch(chan))
        i += my_strcpy(s + i, "bitch ");
      if (channel_greet(chan))
        i += my_strcpy(s + i, "greet ");
      if (channel_protectops(chan))
        i += my_strcpy(s + i, "protectops ");
      if (channel_protecthalfops(chan))
        i += my_strcpy(s + i, "protecthalfops ");
      if (channel_protectfriends(chan))
        i += my_strcpy(s + i, "protectfriends ");
      if (channel_dontkickops(chan))
        i += my_strcpy(s + i, "dontkickops ");
      if (channel_logstatus(chan))
        i += my_strcpy(s + i, "statuslog ");
      if (channel_revenge(chan))
        i += my_strcpy(s + i, "revenge ");
      if (channel_revenge(chan))
        i += my_strcpy(s + i, "revengebot ");
      if (channel_secret(chan))
        i += my_strcpy(s + i, "secret ");
      if (channel_shared(chan))
        i += my_strcpy(s + i, "shared ");
      if (!channel_static(chan))
        i += my_strcpy(s + i, "dynamic ");
      if (channel_autovoice(chan))
        i += my_strcpy(s + i, "autovoice ");
      if (channel_autohalfop(chan))
        i += my_strcpy(s + i, "autohalfop ");
      if (channel_cycle(chan))
        i += my_strcpy(s + i, "cycle ");
      if (channel_seen(chan))
        i += my_strcpy(s + i, "seen ");
      if (channel_dynamicexempts(chan))
        i += my_strcpy(s + i, "dynamicexempts ");
      if (!channel_nouserexempts(chan))
        i += my_strcpy(s + i, "userexempts ");
      if (channel_dynamicinvites(chan))
        i += my_strcpy(s + i, "dynamicinvites ");
      if (!channel_nouserinvites(chan))
        i += my_strcpy(s + i, "userinvites ");
      if (channel_inactive(chan))
        i += my_strcpy(s + i, "inactive ");
      if (channel_nodesynch(chan))
        i += my_strcpy(s + i, "nodesynch ");

      dprintf(idx, "      Options: %s\n", s);

      if (chan->need_op[0])
        dprintf(idx, "      To get ops, I do: %s\n", chan->need_op);

      if (chan->need_invite[0])
        dprintf(idx, "      To get invited, I do: %s\n", chan->need_invite);

      if (chan->need_limit[0])
        dprintf(idx, "      To get the channel limit raised, I do: %s\n",
                chan->need_limit);

      if (chan->need_unban[0])
        dprintf(idx, "      To get unbanned, I do: %s\n", chan->need_unban);

      if (chan->need_key[0])
        dprintf(idx, "      To get the channel key, I do: %s\n",
                chan->need_key);

      if (chan->idle_kick)
        dprintf(idx, "      Kicking idle users after %d minute%s\n",
                chan->idle_kick, (chan->idle_kick != 1) ? "s" : "");

      if (chan->stopnethack_mode)
        dprintf(idx, "      stopnethack-mode: %d\n", chan->stopnethack_mode);

      if (chan->revenge_mode)
        dprintf(idx, "      revenge-mode: %d\n", chan->revenge_mode);

      dprintf(idx, "      ban-type: %d\n", chan->ban_type);
      dprintf(idx, "      Bans last %d minute%s.\n", chan->ban_time,
               (chan->ban_time == 1) ? "" : "s");
      dprintf(idx, "      Exemptions last %d minute%s.\n", chan->exempt_time,
               (chan->exempt_time == 1) ? "" : "s");
      dprintf(idx, "      Invitations last %d minute%s.\n", chan->invite_time,
               (chan->invite_time == 1) ? "" : "s");
    }
  }
}

static int expmem_masklist(masklist *m)
{
  int result = 0;

  for (; m; m = m->next) {
    result += sizeof(masklist);
    if (m->mask)
      result += strlen(m->mask) + 1;
    if (m->who)
      result += strlen(m->who) + 1;
  }
  return result;
}

static int channels_expmem()
{
  int tot = 0, i;
  struct chanset_t *chan;

  for (chan = chanset; chan; chan = chan->next) {
    tot += sizeof(struct chanset_t);

    tot += strlen(chan->channel.key) + 1;
    if (chan->channel.topic)
      tot += strlen(chan->channel.topic) + 1;
    tot += (sizeof(struct memstruct) * (chan->channel.members + 1));

    tot += expmem_masklist(chan->channel.ban);
    tot += expmem_masklist(chan->channel.exempt);
    tot += expmem_masklist(chan->channel.invite);

    for (i = 0; i < MODES_PER_LINE_MAX && chan->cmode[i].op; i++)
      tot += strlen(chan->cmode[i].op) + 1;
    if (chan->key)
      tot += strlen(chan->key) + 1;
    if (chan->rmkey)
      tot += strlen(chan->rmkey) + 1;
  }
  tot += expmem_udef(udef);
  if (lastdeletedmask)
    tot += strlen(lastdeletedmask) + 1;
  return tot;
}

static char *traced_globchanset(ClientData cdata, Tcl_Interp *irp,
                                EGG_CONST char *name1,
                                EGG_CONST char *name2, int flags)
{
  int i, items;
  char *t, *s;
  EGG_CONST char **item, *s2;

  if (flags & (TCL_TRACE_READS | TCL_TRACE_UNSETS)) {
    Tcl_SetVar2(interp, name1, name2, glob_chanset, TCL_GLOBAL_ONLY);
    if (flags & TCL_TRACE_UNSETS)
      Tcl_TraceVar(interp, "global-chanset",
                   TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                   traced_globchanset, NULL);
  } else {                        /* Write */
    s2 = Tcl_GetVar2(interp, name1, name2, TCL_GLOBAL_ONLY);
    Tcl_SplitList(interp, s2, &items, &item);
    for (i = 0; i < items; i++) {
      if (!(item[i]) || (strlen(item[i]) < 2))
        continue;
      s = glob_chanset;
      while (s[0]) {
        t = strchr(s, ' ');     /* Can't be NULL coz of the extra space */
        t[0] = 0;
        if (!strcmp(s + 1, item[i] + 1)) {
          s[0] = item[i][0];    /* +- */
          t[0] = ' ';
          break;
        }
        t[0] = ' ';
        s = t + 1;
      }
    }
    if (item)                   /* hmm it cant be 0 */
      Tcl_Free((char *) item);
    Tcl_SetVar2(interp, name1, name2, glob_chanset, TCL_GLOBAL_ONLY);
  }
  return NULL;
}

static tcl_ints my_tcl_ints[] = {
  {"share-greet",             NULL,                     0},
  {"use-info",                &use_info,                0},
  {"quiet-save",              &quiet_save,              0},
  {"allow-ps",                &allow_ps,                0},
  {"global-stopnethack-mode", &global_stopnethack_mode, 0},
  {"global-revenge-mode",     &global_revenge_mode,     0},
  {"global-idle-kick",        &global_idle_kick,        0},
  {"global-ban-time",         &global_ban_time,         0},
  {"global-exempt-time",      &global_exempt_time,      0},
  {"global-invite-time",      &global_invite_time,      0},
  {"global-ban-type",         &global_ban_type,         0},
  /* keeping [ban|exempt|invite]-time for compatability <Wcc[07/20/02]> */
  {"ban-time",                &global_ban_time,         0},
  {"exempt-time",             &global_exempt_time,      0},
  {"invite-time",             &global_invite_time,      0},
  {NULL,                      NULL,                     0}
};

static tcl_coups mychan_tcl_coups[] = {
  {"global-flood-chan", &gfld_chan_thr,  &gfld_chan_time},
  {"global-flood-deop", &gfld_deop_thr,  &gfld_deop_time},
  {"global-flood-kick", &gfld_kick_thr,  &gfld_kick_time},
  {"global-flood-join", &gfld_join_thr,  &gfld_join_time},
  {"global-flood-ctcp", &gfld_ctcp_thr,  &gfld_ctcp_time},
  {"global-flood-nick", &gfld_nick_thr,  &gfld_nick_time},
  {"global-aop-delay",  &global_aop_min, &global_aop_max},
  {NULL,                NULL,                       NULL}
};

static tcl_strings my_tcl_strings[] = {
  {"chanfile",        chanfile,      120, STR_PROTECT},
  {"global-chanmode", glob_chanmode, 64,            0},
  {NULL,              NULL,          0,             0}
};

static char *channels_close()
{
  write_channels();
  free_udef(udef);
  if (lastdeletedmask)
    nfree(lastdeletedmask);
  rem_builtins(H_chon, my_chon);
  rem_builtins(H_dcc, C_dcc_irc);
  rem_tcl_commands(channels_cmds);
  rem_tcl_strings(my_tcl_strings);
  rem_tcl_ints(my_tcl_ints);
  rem_tcl_coups(mychan_tcl_coups);
  del_hook(HOOK_USERFILE, (Function) channels_writeuserfile);
  del_hook(HOOK_BACKUP, (Function) backup_chanfile);
  del_hook(HOOK_REHASH, (Function) channels_rehash);
  del_hook(HOOK_PRE_REHASH, (Function) channels_prerehash);
  del_hook(HOOK_MINUTELY, (Function) check_expired_bans);
  del_hook(HOOK_MINUTELY, (Function) check_expired_exempts);
  del_hook(HOOK_MINUTELY, (Function) check_expired_invites);
  Tcl_UntraceVar(interp, "global-chanset",
                 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                 traced_globchanset, NULL);
  rem_help_reference("channels.help");
  rem_help_reference("chaninfo.help");
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *channels_start();

static Function channels_table[] = {
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
  (Function) NULL,           /* [17] used to be ban_time <Wcc[07/19/02]>    */
  (Function) & use_info,
  (Function) get_handle_chaninfo,
  /* 20 - 23 */
  (Function) u_sticky_mask,
  (Function) ismasked,
  (Function) add_chanrec_by_handle,
  (Function) NULL,           /* [23] used to be isexempted() <cybah>         */
  /* 24 - 27 */
  (Function) NULL,           /* [24] used to be exempt_time <Wcc[07/19/02]>  */
  (Function) NULL,           /* [25] used to be isinvited() <cybah>          */
  (Function) NULL,           /* [26] used to be ban_time <Wcc[07/19/02]>     */
  (Function) NULL,
  /* 28 - 31 */
  (Function) NULL,           /* [28] used to be u_setsticky_exempt() <cybah> */
  (Function) u_delexempt,
  (Function) u_addexempt,
  (Function) NULL,
  /* 32 - 35 */
  (Function) NULL,           /* [32] used to be u_sticky_exempt() <cybah>    */
  (Function) NULL,
  (Function) NULL,           /* [34] used to be killchanset().               */
  (Function) u_delinvite,
  /* 36 - 39 */
  (Function) u_addinvite,
  (Function) tcl_channel_add,
  (Function) tcl_channel_modify,
  (Function) write_exempts,
  /* 40 - 43 */
  (Function) write_invites,
  (Function) ismodeline,
  (Function) initudef,
  (Function) ngetudef,
  /* 44 - 47 */
  (Function) expired_mask,
  (Function) remove_channel,
  (Function) & global_ban_time,
  (Function) & global_exempt_time,
  /* 48 - 51 */
  (Function) & global_invite_time,
};

char *channels_start(Function *global_funcs)
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
  global_idle_kick = 0;
  global_aop_min = 5;
  global_aop_max = 30;
  allow_ps = 0;
  lastdeletedmask = 0;
  use_info = 1;
  strcpy(chanfile, "chanfile");
  chan_hack = 0;
  quiet_save = 0;
  strcpy(glob_chanmode, "nt");
  udef = NULL;
  global_stopnethack_mode = 0;
  global_revenge_mode = 0;
  global_ban_type = 3;
  global_ban_time = 120;
  global_exempt_time = 60;
  global_invite_time = 60;
  strcpy(glob_chanset,
         "-enforcebans "
         "+dynamicbans "
         "+userbans "
         "-autoop "
         "-bitch "
         "+greet "
         "+protectops "
         "+statuslog "
         "-revenge "
         "-secret "
         "-autovoice "
         "+cycle "
         "+dontkickops "
         "-inactive "
         "-protectfriends "
         "+shared "
         "-seen "
         "+userexempts "
         "+dynamicexempts "
         "+userinvites "
         "+dynamicinvites "
         "-revengebot "
         "-protecthalfops "
         "-autohalfop "
         "-nodesynch "
         "-static ");
  module_register(MODULE_NAME, channels_table, 1, 1);
  if (!module_depend(MODULE_NAME, "eggdrop", 106, 20)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.6.20 or later.";
  }
  add_hook(HOOK_MINUTELY, (Function) check_expired_bans);
  add_hook(HOOK_MINUTELY, (Function) check_expired_exempts);
  add_hook(HOOK_MINUTELY, (Function) check_expired_invites);
  add_hook(HOOK_USERFILE, (Function) channels_writeuserfile);
  add_hook(HOOK_BACKUP, (Function) backup_chanfile);
  add_hook(HOOK_REHASH, (Function) channels_rehash);
  add_hook(HOOK_PRE_REHASH, (Function) channels_prerehash);
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
  read_channels(0, 1);
  return NULL;
}
