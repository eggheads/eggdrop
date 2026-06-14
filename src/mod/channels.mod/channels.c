/*
 * channels.c -- part of channels.mod
 *   support for channels within the bot
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2025 Eggheads Development Team
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
static void get_extban_prefix(char *prefix);
static int is_extban_mask(const char *mask);
static int extban_is_matchable(const char *mask);
static char chanfile[121], glob_chanmode[65];
static char *lastdeletedmask;

/* Look up an ISUPPORT (raw 005) value via server.mod, if loaded.
 * Returns NULL if server.mod is not loaded or the key isn't set.
 */
static const char *servermod_isupport_get(const char *name)
{
  module_entry *me = module_find("server", 0, 0);
  if (me && me->funcs && me->funcs[SERVER_GET_ISUPPORT])
    return ((const char *(*)(const char *, size_t)) me->funcs[SERVER_GET_ISUPPORT])(name, strlen(name));
  return NULL;
}

static p_tcl_bind_list H_chanset;

static struct udef_struct *udef;

static int channels_should_store_generic_mode(char mode)
{
  return chanmode_prot_index(mode) >= 0 && mode != 'b' && mode != 'e' &&
         mode != 'I' && mode != 'o' && mode != 'h' && mode != 'v';
}

static char *append_generic_prot_modes(char *p, uint64_t modes)
{
  const char *c;

  for (c = CHANMODE_INDEX_CHARS; *c; c++)
    if (modes & chanmode_prot_bit(*c))
      *p++ = *c;
  return p;
}

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

/* Parse extban mask into type and arg pointers.
 * Supports both prefixed (<prefix><type>:<arg>) and non-prefixed (<type>:<arg>) forms.
 * Returns a 1 if mask is an extban mask, 0 otherwise
 * If 1 is returned:
 *   type is filled with the extban character
 *   arg points to the extban value
 */
int extban_parse(const char *mask, char *type, const char **arg) {
  const char *value, *comma;
  char prefix = 0;

  /* now we know mask is set, don't check it again later*/
  if (!mask || !mask[0] || strlen(mask) < 3)
    return 0;

  value = servermod_isupport_get("EXTBAN");
  if (value && value[0]) {
    comma = strchr(value, ',');
    if (comma && comma - value == 1)
      prefix = value[0];
  }

  /* Break out prefixed masks first when the server advertised a prefix. */
  if (prefix && mask[0] == prefix && isalnum((unsigned char) mask[1]) &&
      mask[2] == ':') {
    if (type) {
      *type = mask[1];
    }
    if (arg) {
      *arg = mask + 3;
    }
    return 1;
  }

  /* Break out no-prefix mask. If EXTBAN is known and did not advertise a
   * prefix, this is the only accepted form.
   */
  if (isalnum((unsigned char) mask[0]) && mask[1] == ':') {
    if (type) {
      *type = mask[0];
    }
    if (arg) {
      *arg = mask + 2;
    }
    return 1;
  }

/* If EXTBAN is unknown (e.g. before connect), retain the historical fallback
 * so stored prefixed extbans can still be parsediwth any non-alnum prefixchar.
 */
  if ((!value || !value[0]) && !isalnum((unsigned char) mask[0]) &&
      isalnum((unsigned char) mask[1]) && mask[2] == ':') {
    if (type)
      *type = mask[1];
    if (arg)
      *arg = mask + 3; /* ~a:x!y@z -- mask+3 = x */
    return 1;
  }
  return 0;
}

/* Return 1 if the server currently advertises support for the given extban
 * flag, either through ACCOUNTEXTBAN or the EXTBAN type list; otherwise return
 * 0 so callers can avoid trying to set or recheck it on this server.
 */
int extban_flag_supported(char flag)
{
  const char *accountflag, *value, *comma, *types;

  accountflag = servermod_isupport_get("ACCOUNTEXTBAN");
  if (accountflag && accountflag[0] && flag == accountflag[0])
    return 1;

  value = servermod_isupport_get("EXTBAN");
  if (!value || !value[0])
    return 0;

  comma = strchr(value, ',');
  types = comma ? comma + 1 : value;
  return strchr(types, flag) ? 1 : 0;
}

/* Return 1 if mask uses extban syntax. */
static int is_extban_mask(const char *mask)
{
  return extban_parse(mask, NULL, NULL);
}

/* Extban prefix from ISUPPORT EXTBAN, if present.
 * EXTBAN grammar is [prefix],<types>
 */
static void get_extban_prefix(char *prefix)
{
  const char *value, *comma;

  /* Clear out old value */
  if (prefix) {
    *prefix = '\0';
  }
  value = servermod_isupport_get("EXTBAN");
  if (!value || !value[0]) {
    //TO DO: log issue to partyline
    return;
  }
  comma = strchr(value, ',');
  if (comma) {
    if (comma == value) {
      // No prefix
      return;
    }
    if ((comma - value) == 1) {
      if (prefix)
        *prefix = value[0];
      // Prefix
      return;
    }
  }
  // What do we do if no , present?
  return;
}

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

static void clear_mode_protect_store(struct chanset_t *chan)
{
  int i;

  chan->mode_mns_prot = chan->mode_pls_prot = 0;
  chan->mode_pls_prot_generic = chan->mode_mns_prot_generic = 0;
  chan->limit_prot = 0;
  chan->key_prot[0] = 0;
  for (i = 0; i < (int) (sizeof chan->mode_prot_args /
      sizeof chan->mode_prot_args[0]); i++) {
    if (chan->mode_prot_args[i])
      nfree(chan->mode_prot_args[i]);
    chan->mode_prot_args[i] = NULL;
  }
}

static void free_mode_protect(struct chanset_t *chan)
{
  clear_mode_protect_store(chan);
  if (chan->chanmode_verbatim) {
    nfree(chan->chanmode_verbatim);
    chan->chanmode_verbatim = NULL;
  }
}

static void set_mode_protect_arg(struct chanset_t *chan, char mode,
                                 const char *arg)
{
  int idx = chanmode_prot_index(mode);

  if (idx < 0)
    return;
  if (chan->mode_prot_args[idx])
    nfree(chan->mode_prot_args[idx]);
  chan->mode_prot_args[idx] = NULL;
  if (arg) {
    chan->mode_prot_args[idx] = nmalloc(strlen(arg) + 1);
    strcpy(chan->mode_prot_args[idx], arg);
  }
}

static int legacy_mode_protect_bit(char mode)
{
  switch (mode) {
  case 'i':
    return CHANINV;
  case 'p':
    return CHANPRIV;
  case 's':
    return CHANSEC;
  case 'm':
    return CHANMODER;
  case 'c':
    return CHANNOCLR;
  case 'C':
    return CHANNOCTCP;
  case 'R':
    return CHANREGON;
  case 'M':
    return CHANMODREG;
  case 'r':
    return CHANLONLY;
  case 'D':
    return CHANDELJN;
  case 'u':
    return CHANSTRIP;
  case 'N':
    return CHANNONOTC;
  case 'T':
    return CHANNOAMSG;
  case 't':
    return CHANTOPIC;
  case 'n':
    return CHANNOMSG;
  case 'a':
    return CHANANON;
  case 'q':
    return CHANQUIET;
  case 'l':
    return CHANLIMIT;
  case 'k':
    return CHANKEY;
  default:
    return 0;
  }
}

static void set_mode_protect_bit(struct chanset_t *chan, int pos, char mode,
                                 const char *arg)
{
  uint64_t bit = chanmode_prot_bit(mode);
  int legacy = legacy_mode_protect_bit(mode);

  if (!bit)
    return;
  if (pos) {
    chan->mode_pls_prot_generic |= bit;
    chan->mode_mns_prot_generic &= ~bit;
    set_mode_protect_arg(chan, mode, arg);
    if (legacy) {
      chan->mode_pls_prot |= legacy;
      chan->mode_mns_prot &= ~legacy;
    }
    if (mode == 'l')
      chan->limit_prot = arg && arg[0] ? atoi(arg) : 0;
    else if (mode == 'k') {
      chan->key_prot[0] = 0;
      if (arg && arg[0])
        strlcpy(chan->key_prot, arg, sizeof chan->key_prot);
    }
  } else {
    chan->mode_pls_prot_generic &= ~bit;
    chan->mode_mns_prot_generic |= bit;
    set_mode_protect_arg(chan, mode, NULL);
    if (legacy) {
      chan->mode_pls_prot &= ~legacy;
      chan->mode_mns_prot |= legacy;
    }
    if (mode == 'l')
      chan->limit_prot = 0;
    else if (mode == 'k')
      chan->key_prot[0] = 0;
  }
}

static void apply_mode_protect_policy(struct chanset_t *chan)
{
  uint64_t p_bit = chanmode_prot_bit('p');

  /* Prevents a +s-p +p-s flood (fixed by drummer). */
  if (chanmode_pls_prot_isset(chan, 's') && !allow_ps) {
    chan->mode_pls_prot &= ~CHANPRIV;
    chan->mode_pls_prot_generic &= ~p_bit;
    set_mode_protect_arg(chan, 'p', NULL);
  }
}

static int chanmode_shape_valid(const char *set, Tcl_Interp *irp)
{
  char *copy, *cursor, *token;
  int saw_modes = 0;

  copy = nmalloc(strlen(set) + 1);
  strcpy(copy, set);
  cursor = copy;
  for (token = newsplit(&cursor); token[0]; token = newsplit(&cursor)) {
    char *p;
    int saw_letter = 0;

    if (token[0] == '+' || token[0] == '-' || !saw_modes) {
      for (p = token; *p; p++) {
        if (*p == '+' || *p == '-')
          continue;
        if (!isalnum((unsigned char) *p))
          break;
        saw_letter = 1;
      }
      if (*p || !saw_letter) {
        if (!saw_modes) {
          if (irp)
            Tcl_AppendResult(irp, "invalid chanmode shape: ", token, NULL);
          nfree(copy);
          return 0;
        }
      } else {
        saw_modes = 1;
      }
    }
  }
  nfree(copy);
  return 1;
}

static void set_mode_protect_verbatim(struct chanset_t *chan, const char *set)
{
  if (chan->chanmode_verbatim)
    nfree(chan->chanmode_verbatim);
  chan->chanmode_verbatim = nmalloc(strlen(set) + 1);
  strcpy(chan->chanmode_verbatim, set);
}

static void parse_mode_protect_compat(struct chanset_t *chan, const char *set)
{
  int pos = 1;
  char *copy, *cursor, *s, *s1;

  copy = nmalloc(strlen(set) + 1);
  strcpy(copy, set);
  cursor = copy;
  for (s = newsplit(&cursor); *s; s++) {
    char mode = *s;

    switch (*s) {
    case '+':
      pos = 1;
      break;
    case '-':
      pos = 0;
      break;
    case 'l':
      s1 = "";
      if (pos) {
        s1 = newsplit(&cursor);
      }
      set_mode_protect_bit(chan, pos, mode, s1);
      break;
    case 'k':
      s1 = "";
      if (pos) {
        s1 = newsplit(&cursor);
      }
      set_mode_protect_bit(chan, pos, mode, s1);
      break;
    default:
      if (legacy_mode_protect_bit(mode) || channels_should_store_generic_mode(mode))
        set_mode_protect_bit(chan, pos, mode, NULL);
      break;
    }
  }
  nfree(copy);
  apply_mode_protect_policy(chan);
}

static int set_mode_protect(struct chanset_t *chan, const char *set,
                            Tcl_Interp *irp)
{
  module_entry *me;

  if (!set)
    set = "";
  if (!chanmode_shape_valid(set, irp))
    return TCL_ERROR;
  set_mode_protect_verbatim(chan, set);
  clear_mode_protect_store(chan);
  parse_mode_protect_compat(chan, set);
  me = module_find("irc", 0, 0);
  if (me && me->funcs && me->funcs[IRC_REPARSE_CHANNEL_MODES]) {
    int ret = ((int (*)(struct chanset_t *, Tcl_Interp *, int))
               me->funcs[IRC_REPARSE_CHANNEL_MODES]) (chan, irp, 1);

    apply_mode_protect_policy(chan);
    return ret;
  }
  return TCL_OK;
}

static void get_mode_protect(struct chanset_t *chan, char *s, size_t slen)
{
  char *p = s, s1[122];
  int i, tst;

  if (!slen)
    return;
  if (chan->chanmode_verbatim) {
    strlcpy(s, chan->chanmode_verbatim, slen);
    return;
  }
  s1[0] = 0;
  for (i = 0; i < 2; i++) {
    if (i == 0) {
      const char *limit_arg = chanmode_prot_arg(chan, 'l');
      const char *key_arg = chanmode_prot_arg(chan, 'k');

      tst = chan->mode_pls_prot;
      if ((tst) || limit_arg[0] || key_arg[0] ||
          chan->mode_pls_prot_generic)
        *p++ = '+';
      if (limit_arg[0]) {
        *p++ = 'l';
        snprintf(s1 + strlen(s1), (sizeof s1) - strlen(s1), "%s ", limit_arg);
      }
      if (key_arg[0]) {
        *p++ = 'k';
        snprintf(s1 + strlen(s1), (sizeof s1) - strlen(s1), "%s ", key_arg);
      }
    } else {
      tst = chan->mode_mns_prot;
      if (tst || chan->mode_mns_prot_generic)
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
    p = append_generic_prot_modes(p, i == 0 ? chan->mode_pls_prot_generic :
                                  chan->mode_mns_prot_generic);
  }
  *p = 0;
  if (s1[0]) {
    s1[strlen(s1) - 1] = 0;
    strlcat(s, " ", slen);
    strlcat(s, s1, slen);
  }
}

static int builtin_chanset STDVAR
{
  Function F = (Function) cd;

  BADARGS(3, 3, " chan setting value");

  CHECKVALIDITY(builtin_chanset);
  F(argv[1], argv[2], argv[3]);
  return TCL_OK;
}

int check_tcl_chanset(const char *chan, const char *setting, const char *value)
{
  Tcl_SetVar(interp, "_chanset1", (char *) chan, 0);
  Tcl_SetVar(interp, "_chanset2", (char *) setting, 0);
  Tcl_SetVar(interp, "_chanset3", (char *) value, 0);

  return BIND_EXEC_LOG == check_tcl_bind(H_chanset, setting, 0, " $_chanset1 $_chanset2 $_chanset3",
                     MATCH_MASK | BIND_STACKABLE | BIND_STACKRET | BIND_WANTRET);
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
static int chanset_unlink(struct chanset_t *chan)
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
  module_entry *me;

  /* Remove the channel from the list, so that no one can pull it
   * away from under our feet during the check_tcl_part() call. */
  (void) chanset_unlink(chan);

  if ((me = module_find("irc", 1, 3)) != NULL)
    (me->funcs[IRC_DO_CHANNEL_PART]) (chan);

  clear_channel(chan, 0);
  free_mode_protect(chan);
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
 *  - We will write empty need-xxxx too, why not? (less code + laziness)
 */
static void write_channels()
{
  FILE *f;
  char s[sizeof chanfile + 4], s1[26], w[1024], w2[1024], name[163];
  char need1[242], need2[242], need3[242], need4[242], need5[242];
  struct chanset_t *chan;
  struct udef_struct *ul;

  if (!chanfile[0])
    return;
  egg_snprintf(s, sizeof s, "%s~new", chanfile);
  f = fopen(s, "w");
  chmod(s, userfile_perm);
  if (f == NULL) {
    putlog(LOG_MISC, "*", "ERROR writing channel file.");
    return;
  }
  if (!quiet_save)
    putlog(LOG_MISC, "*", "%s", CHAN_FILE_WRITING);
  ctime_r(&now, s1);
  fprintf(f, "#Dynamic Channel File for %s (%s) -- written %s",
          botnetnick, ver, s1);
  for (chan = chanset; chan; chan = chan->next) {
    convert_element(chan->dname, name);
    get_mode_protect(chan, w, sizeof w);
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

    /* Assume file isnt there & therefore make it */
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
  char s[sizeof chanfile + 4];

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
      get_mode_protect(chan, s2, sizeof s2);

      if (s2[0]) {
        int len = strlen(s);
        egg_snprintf(s + len, (sizeof s) - len, ", enforcing \"%s\"", s2); /* Concatenation */
      }

      s2[0] = 0;

      if (channel_greet(chan))
        strcat(s2, "greet, ");
      if (channel_autoop(chan))
        strcat(s2, "auto-op, ");
      if (channel_bitch(chan))
        strcat(s2, "bitch, ");

      if (s2[0]) {
        int len = strlen(s);
        s2[strlen(s2) - 2] = 0;
        egg_snprintf(s + len, (sizeof s) - len, " (%s)", s2); /* Concatenation */
      }

      /* If it's a !chan, we want to display it's unique name too <cybah> */
      if (chan->dname[0] == '!') {
        int len = strlen(s);
        egg_snprintf(s + len, (sizeof s) - len, ", unique name %s", chan->name); /* Concatenation */
      }
    }

    dprintf(idx, "%s\n", s);

    if (details) {
      if ((i = snprintf(s, sizeof s, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
                        channel_enforcebans(chan) ? "enforcebans " : "",
                        channel_dynamicbans(chan) ? "dynamicbans " : "",
                        !channel_nouserbans(chan) ? "userbans " : "",
                        channel_autoop(chan) ? "autoop " : "",
                        channel_bitch(chan) ? "bitch " : "",
                        channel_greet(chan) ? "greet " : "",
                        channel_protectops(chan) ? "protectops " : "",
                        channel_protecthalfops(chan) ? "protecthalfops " : "",
                        channel_protectfriends(chan) ? "protectfriends " : "",
                        channel_dontkickops(chan) ? "dontkickops " : "",
                        channel_logstatus(chan) ? "statuslog " : "",
                        channel_revenge(chan) ? "revenge " : "",
                        channel_revengebot(chan) ? "revengebot " : "",
                        channel_secret(chan) ? "secret " : "",
                        channel_shared(chan) ? "shared " : "",
                        !channel_static(chan) ? "dynamic " : "",
                        channel_autovoice(chan) ? "autovoice " : "",
                        channel_autohalfop(chan) ? "autohalfop " : "",
                        channel_cycle(chan) ? "cycle " : "",
                        channel_seen(chan) ? "seen " : "",
                        channel_dynamicexempts(chan) ? "dynamicexempts " : "",
                        !channel_nouserexempts(chan) ? "userexempts " : "",
                        channel_dynamicinvites(chan) ? "dynamicinvites " : "",
                        !channel_nouserinvites(chan) ? "userinvites " : "",
                        channel_inactive(chan) ? "inactive " : "",
                        channel_nodesynch(chan) ? "nodesynch " : "")))
        s[i - 2] = 0;

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

static int expmem_chanmode_lists(chanmode_list *list)
{
  int result = 0;

  for (; list; list = list->next) {
    chanmode_masklist *mask;

    result += sizeof(chanmode_list);
    for (mask = list->masks; mask; mask = mask->next) {
      result += sizeof(chanmode_masklist);
      if (mask->mask)
        result += strlen(mask->mask) + 1;
      if (mask->who)
        result += strlen(mask->who) + 1;
    }
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

    for (i = 0; i < (int) (sizeof chan->channel.modeargs /
        sizeof chan->channel.modeargs[0]); i++)
      if (chan->channel.modeargs[i])
        tot += strlen(chan->channel.modeargs[i]) + 1;
    tot += expmem_chanmode_lists(chan->channel.modelists);
    for (i = 0; i < MODEQUEUE_MAX; i++)
      if (chan->modequeue[i].arg)
        tot += strlen(chan->modequeue[i].arg) + 1;
    for (i = 0; i < (int) (sizeof chan->mode_prot_args /
        sizeof chan->mode_prot_args[0]); i++)
      if (chan->mode_prot_args[i])
        tot += strlen(chan->mode_prot_args[i]) + 1;
    if (chan->chanmode_verbatim)
      tot += strlen(chan->chanmode_verbatim) + 1;
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
  Tcl_Size i, items;
  char *t, *s;
  EGG_CONST char **item, *s2;

  if (flags & (TCL_TRACE_READS | TCL_TRACE_UNSETS)) {
    Tcl_SetVar2(interp, name1, name2, glob_chanset, TCL_GLOBAL_ONLY);
    if (flags & TCL_TRACE_UNSETS) {
      Tcl_TraceVar(interp, "global-chanset",
                   TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                   traced_globchanset, NULL); /* keep for backward compatibility */
      Tcl_TraceVar(interp, "default-chanset",
                   TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                   traced_globchanset, NULL);
    }
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

static char *traced_account_extban(ClientData cdata, Tcl_Interp *irp,
                                   EGG_CONST char *name1,
                                   EGG_CONST char *name2, int flags)
{
  const char *account_extban = servermod_isupport_get("ACCOUNTEXTBAN");

  Tcl_SetVar2(interp, name1, name2,
              (account_extban && account_extban[0]) ? account_extban : "",
              TCL_GLOBAL_ONLY);
  if (flags & TCL_TRACE_UNSETS) {
    Tcl_TraceVar(interp, "account-extban",
                 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                 traced_account_extban, NULL);
  }
  return NULL;
}

static char *traced_extban_flags(ClientData cdata, Tcl_Interp *irp,
                                  EGG_CONST char *name1,
                                  EGG_CONST char *name2, int flags)
{
  const char *extban = servermod_isupport_get("EXTBAN");
  const char *comma, *extban_flags = "";

  if (extban && extban[0]) {
    comma = strchr(extban, ',');
    extban_flags = comma ? comma + 1 : extban;
  }

  Tcl_SetVar2(interp, name1, name2, extban_flags, TCL_GLOBAL_ONLY);
  if (flags & TCL_TRACE_UNSETS) {
    Tcl_TraceVar(interp, "extban-flags",
                 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                 traced_extban_flags, NULL);
  }
  return NULL;
}

static tcl_ints my_tcl_ints[] = {
  {"use-info",                 &use_info,                0},
  {"quiet-save",               &quiet_save,              0},
  {"allow-ps",                 &allow_ps,                0},
  {"default-stopnethack-mode", &global_stopnethack_mode, 0},
  {"default-revenge-mode",     &global_revenge_mode,     0},
  {"default-idle-kick",        &global_idle_kick,        0},
  {"default-ban-time",         &global_ban_time,         0},
  {"default-exempt-time",      &global_exempt_time,      0},
  {"default-invite-time",      &global_invite_time,      0},
  {"default-ban-type",         &global_ban_type,         0},
  /* keep global-* for backward compatibility */
  {"global-stopnethack-mode", &global_stopnethack_mode, 0},
  {"global-revenge-mode",     &global_revenge_mode,     0},
  {"global-idle-kick",        &global_idle_kick,        0},
  {"global-ban-time",         &global_ban_time,         0},
  {"global-exempt-time",      &global_exempt_time,      0},
  {"global-invite-time",      &global_invite_time,      0},
  {"global-ban-type",         &global_ban_type,         0},
  /* keeping [ban|exempt|invite]-time for compatibility <Wcc[07/20/02]> */
  {"ban-time",                &global_ban_time,         0},
  {"exempt-time",             &global_exempt_time,      0},
  {"invite-time",             &global_invite_time,      0},
  {NULL,                      NULL,                     0}
};

static tcl_coups mychan_tcl_coups[] = {
  {"default-flood-chan", &gfld_chan_thr,  &gfld_chan_time},
  {"default-flood-deop", &gfld_deop_thr,  &gfld_deop_time},
  {"default-flood-kick", &gfld_kick_thr,  &gfld_kick_time},
  {"default-flood-join", &gfld_join_thr,  &gfld_join_time},
  {"default-flood-ctcp", &gfld_ctcp_thr,  &gfld_ctcp_time},
  {"default-flood-nick", &gfld_nick_thr,  &gfld_nick_time},
  {"default-aop-delay",  &global_aop_min, &global_aop_max},
  /* keep global-* for backward compatibility */
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
  {"chanfile",         chanfile,      120, STR_PROTECT},
  {"default-chanmode", glob_chanmode, 64,            0},
  /* keep global-chanmode for backward compatibility */
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
                 traced_globchanset, NULL); /* keep for backward compatibility */
  Tcl_UntraceVar(interp, "default-chanset",
                 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                 traced_globchanset, NULL);
  Tcl_UntraceVar(interp, "account-extban",
                 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                 traced_account_extban, NULL);
  Tcl_UntraceVar(interp, "extban-flags",
                 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                 traced_extban_flags, NULL);
  traced_account_extban(NULL, interp, "account-extban", NULL, TCL_TRACE_READS);
  traced_extban_flags(NULL, interp, "extban-flags", NULL, TCL_TRACE_READS);
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
  (Function) extban_parse,
  (Function) extban_flag_supported
};

char *channels_start(Function *global_funcs)
{
  global = global_funcs;

  gfld_chan_thr = 15;
  gfld_chan_time = 60;
  gfld_deop_thr = 3;
  gfld_deop_time = 10;
  gfld_kick_thr = 3;
  gfld_kick_time = 10;
  gfld_join_thr = 5;
  gfld_join_time = 60;
  gfld_ctcp_thr = 3;
  gfld_ctcp_time = 60;
  gfld_nick_thr = 5;
  gfld_nick_time = 60;
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
         "-statuslog "
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
  module_register(MODULE_NAME, channels_table, 1, 2);
  if (!module_depend(MODULE_NAME, "eggdrop", 108, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.8.0 or later.";
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
               traced_globchanset, NULL); /* keep for backward compatibility */
  Tcl_TraceVar(interp, "default-chanset",
               TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
               traced_globchanset, NULL);
  Tcl_TraceVar(interp, "account-extban",
               TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
               traced_account_extban, NULL);
  Tcl_TraceVar(interp, "extban-flags",
               TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
               traced_extban_flags, NULL);
  H_chanset = add_bind_table("chanset", HT_STACKABLE, builtin_chanset);
  H_chanset = add_bind_table("chanset", HT_STACKABLE, builtin_chanset);
  add_builtins(H_chon, my_chon);
  add_builtins(H_dcc, C_dcc_irc);
  add_tcl_commands(channels_cmds);
  add_tcl_strings(my_tcl_strings);
  add_help_reference("channels.help");
  add_help_reference("chaninfo.help");
  add_tcl_ints(my_tcl_ints);
  add_tcl_coups(mychan_tcl_coups);
  read_channels(0, 0);
  return NULL;
}
