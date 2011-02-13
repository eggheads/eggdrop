/*
 * flags.c -- handles:
 *   all the flag matching/conversion functions in one neat package :)
 *
 * $Id: flags.c,v 1.37 2011/02/13 14:19:33 simple Exp $
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


extern int raw_log, require_p, noshare, allow_dk_cmds;
extern struct dcc_t *dcc;


int logmodes(char *s)
{
  int i;
  int res = 0;

  for (i = 0; i < strlen(s); i++)
    switch (s[i]) {
    case 'm':
    case 'M':
      res |= LOG_MSGS;
      break;
    case 'p':
    case 'P':
      res |= LOG_PUBLIC;
      break;
    case 'j':
    case 'J':
      res |= LOG_JOIN;
      break;
    case 'k':
    case 'K':
      res |= LOG_MODES;
      break;
    case 'c':
    case 'C':
      res |= LOG_CMDS;
      break;
    case 'o':
    case 'O':
      res |= LOG_MISC;
      break;
    case 'b':
    case 'B':
      res |= LOG_BOTS;
      break;
    case 'r':
    case 'R':
      res |= raw_log ? LOG_RAW : 0;
      break;
    case 'w':
    case 'W':
      res |= LOG_WALL;
      break;
    case 'x':
    case 'X':
      res |= LOG_FILES;
      break;
    case 's':
    case 'S':
      res |= LOG_SERV;
      break;
    case 'd':
    case 'D':
      res |= LOG_DEBUG;
      break;
    case 'v':
    case 'V':
      res |= raw_log ? LOG_SRVOUT : 0;
      break;
    case 't':
    case 'T':
      res |= raw_log ? LOG_BOTNET : 0;
      break;
    case 'h':
    case 'H':
      res |= raw_log ? LOG_BOTSHARE : 0;
      break;
    case '1':
      res |= LOG_LEV1;
      break;
    case '2':
      res |= LOG_LEV2;
      break;
    case '3':
      res |= LOG_LEV3;
      break;
    case '4':
      res |= LOG_LEV4;
      break;
    case '5':
      res |= LOG_LEV5;
      break;
    case '6':
      res |= LOG_LEV6;
      break;
    case '7':
      res |= LOG_LEV7;
      break;
    case '8':
      res |= LOG_LEV8;
      break;
    case '*':
      res |= LOG_ALL;
      break;
    }
  return res;
}

char *masktype(int x)
{
  static char s[24];            /* Change this if you change the levels */
  char *p = s;

  if (x & LOG_MSGS)
    *p++ = 'm';
  if (x & LOG_PUBLIC)
    *p++ = 'p';
  if (x & LOG_JOIN)
    *p++ = 'j';
  if (x & LOG_MODES)
    *p++ = 'k';
  if (x & LOG_CMDS)
    *p++ = 'c';
  if (x & LOG_MISC)
    *p++ = 'o';
  if (x & LOG_BOTS)
    *p++ = 'b';
  if ((x & LOG_RAW) && raw_log)
    *p++ = 'r';
  if (x & LOG_FILES)
    *p++ = 'x';
  if (x & LOG_SERV)
    *p++ = 's';
  if (x & LOG_DEBUG)
    *p++ = 'd';
  if (x & LOG_WALL)
    *p++ = 'w';
  if ((x & LOG_SRVOUT) && raw_log)
    *p++ = 'v';
  if ((x & LOG_BOTNET) && raw_log)
    *p++ = 't';
  if ((x & LOG_BOTSHARE) && raw_log)
    *p++ = 'h';
  if (x & LOG_LEV1)
    *p++ = '1';
  if (x & LOG_LEV2)
    *p++ = '2';
  if (x & LOG_LEV3)
    *p++ = '3';
  if (x & LOG_LEV4)
    *p++ = '4';
  if (x & LOG_LEV5)
    *p++ = '5';
  if (x & LOG_LEV6)
    *p++ = '6';
  if (x & LOG_LEV7)
    *p++ = '7';
  if (x & LOG_LEV8)
    *p++ = '8';
  if (p == s)
    *p++ = '-';
  *p = 0;
  return s;
}

char *maskname(int x)
{
  static char s[207];           /* Change this if you change the levels */
  int i = 0;

  s[0] = 0;
  if (x & LOG_MSGS)
    i += my_strcpy(s, "msgs, ");
  if (x & LOG_PUBLIC)
    i += my_strcpy(s + i, "public, ");
  if (x & LOG_JOIN)
    i += my_strcpy(s + i, "joins, ");
  if (x & LOG_MODES)
    i += my_strcpy(s + i, "kicks/modes, ");
  if (x & LOG_CMDS)
    i += my_strcpy(s + i, "cmds, ");
  if (x & LOG_MISC)
    i += my_strcpy(s + i, "misc, ");
  if (x & LOG_BOTS)
    i += my_strcpy(s + i, "bots, ");
  if ((x & LOG_RAW) && raw_log)
    i += my_strcpy(s + i, "raw, ");
  if (x & LOG_FILES)
    i += my_strcpy(s + i, "files, ");
  if (x & LOG_SERV)
    i += my_strcpy(s + i, "server, ");
  if (x & LOG_DEBUG)
    i += my_strcpy(s + i, "debug, ");
  if (x & LOG_WALL)
    i += my_strcpy(s + i, "wallops, ");
  if ((x & LOG_SRVOUT) && raw_log)
    i += my_strcpy(s + i, "server output, ");
  if ((x & LOG_BOTNET) && raw_log)
    i += my_strcpy(s + i, "botnet traffic, ");
  if ((x & LOG_BOTSHARE) && raw_log)
    i += my_strcpy(s + i, "share traffic, ");
  if (x & LOG_LEV1)
    i += my_strcpy(s + i, "level 1, ");
  if (x & LOG_LEV2)
    i += my_strcpy(s + i, "level 2, ");
  if (x & LOG_LEV3)
    i += my_strcpy(s + i, "level 3, ");
  if (x & LOG_LEV4)
    i += my_strcpy(s + i, "level 4, ");
  if (x & LOG_LEV5)
    i += my_strcpy(s + i, "level 5, ");
  if (x & LOG_LEV6)
    i += my_strcpy(s + i, "level 6, ");
  if (x & LOG_LEV7)
    i += my_strcpy(s + i, "level 7, ");
  if (x & LOG_LEV8)
    i += my_strcpy(s + i, "level 8, ");
  if (i)
    s[i - 2] = 0;
  else
    strcpy(s, "none");
  return s;
}

/* Some flags are mutually exclusive -- this roots them out
 */
int sanity_check(int atr)
{
  if ((atr & USER_BOT) &&
      (atr & (USER_PARTY | USER_MASTER | USER_COMMON | USER_OWNER)))
    atr &= ~(USER_PARTY | USER_MASTER | USER_COMMON | USER_OWNER);
  if ((atr & USER_OP) && (atr & USER_DEOP))
    atr &= ~(USER_OP | USER_DEOP);
  if ((atr & USER_HALFOP) && (atr & USER_DEHALFOP))
    atr &= ~(USER_HALFOP | USER_DEHALFOP);
  if ((atr & USER_AUTOOP) && (atr & USER_DEOP))
    atr &= ~(USER_AUTOOP | USER_DEOP);
  if ((atr & USER_AUTOHALFOP) && (atr & USER_DEHALFOP))
    atr &= ~(USER_AUTOHALFOP | USER_DEHALFOP);
  if ((atr & USER_VOICE) && (atr & USER_QUIET))
    atr &= ~(USER_VOICE | USER_QUIET);
  if ((atr & USER_GVOICE) && (atr & USER_QUIET))
    atr &= ~(USER_GVOICE | USER_QUIET);
  /* Can't be owner without also being master */
  if (atr & USER_OWNER)
    atr |= USER_MASTER;
  /* Master implies botmaster, op and janitor */
  if (atr & USER_MASTER)
    atr |= USER_BOTMAST | USER_OP | USER_JANITOR;
  /* Can't be botnet master without party-line access */
  if (atr & USER_BOTMAST)
    atr |= USER_PARTY;
  /* Janitors can use the file area */
  if (atr & USER_JANITOR)
    atr |= USER_XFER;
  /* Ops should be halfops */
  if (atr & USER_OP)
    atr |= USER_HALFOP;
  return atr;
}

/* Sanity check on channel attributes
 */
int chan_sanity_check(int chatr, int atr)
{
  if ((chatr & USER_OP) && (chatr & USER_DEOP))
    chatr &= ~(USER_OP | USER_DEOP);
  if ((chatr & USER_HALFOP) && (chatr & USER_DEHALFOP))
    chatr &= ~(USER_HALFOP | USER_DEHALFOP);
  if ((chatr & USER_AUTOOP) && (chatr & USER_DEOP))
    chatr &= ~(USER_AUTOOP | USER_DEOP);
  if ((chatr & USER_AUTOHALFOP) && (chatr & USER_DEHALFOP))
    chatr &= ~(USER_AUTOHALFOP | USER_DEHALFOP);
  if ((chatr & USER_VOICE) && (chatr & USER_QUIET))
    chatr &= ~(USER_VOICE | USER_QUIET);
  if ((chatr & USER_GVOICE) && (chatr & USER_QUIET))
    chatr &= ~(USER_GVOICE | USER_QUIET);
  /* Can't be channel owner without also being channel master */
  if (chatr & USER_OWNER)
    chatr |= USER_MASTER;
  /* Master implies op */
  if (chatr & USER_MASTER)
    chatr |= USER_OP;
  /* Op implies halfop */
  if (chatr & USER_OP)
    chatr |= USER_HALFOP;
  /* Can't be +s on chan unless you're a bot */
  if (!(atr & USER_BOT))
    chatr &= ~BOT_SHARE;
  return chatr;
}

/* Get icon symbol for a user (depending on access level)
 *
 * (*) owner on any channel
 * (+) master on any channel
 * (%) botnet master
 * (@) op on any channel
 * (^) halfop on any channel
 * (-) other
 */
char geticon(int idx)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };

  if (!dcc[idx].user)
    return '-';
  get_user_flagrec(dcc[idx].user, &fr, 0);
  if (glob_owner(fr) || chan_owner(fr))
    return '*';
  if (glob_master(fr) || chan_master(fr))
    return '+';
  if (glob_botmast(fr))
    return '%';
  if (glob_op(fr) || chan_op(fr))
    return '@';
  if (glob_halfop(fr) || chan_halfop(fr))
    return '^';
  return '-';
}

void break_down_flags(const char *string, struct flag_record *plus,
                      struct flag_record *minus)
{
  struct flag_record *which = plus;
  int mode = 0;                 /* 0 = glob, 1 = chan, 2 = bot */
  int flags = plus->match;

  if (!(flags & FR_GLOBAL)) {
    if (flags & FR_BOT)
      mode = 2;
    else if (flags & FR_CHAN)
      mode = 1;
    else
      return;                   /* We dont actually want any..huh? */
  }
  egg_bzero(plus, sizeof(struct flag_record));

  if (minus)
    egg_bzero(minus, sizeof(struct flag_record));

  plus->match = FR_OR;          /* Default binding type OR */
  while (*string) {
    switch (*string) {
    case '+':
      which = plus;
      break;
    case '-':
      which = minus ? minus : plus;
      break;
    case '|':
    case '&':
      if (!mode) {
        if (*string == '|')
          plus->match = FR_OR;
        else
          plus->match = FR_AND;
      }
      which = plus;
      mode++;
      if ((mode == 2) && !(flags & (FR_CHAN | FR_BOT)))
        string = "";
      else if (mode == 3)
        mode = 1;
      break;
    default:
      if ((*string >= 'a') && (*string <= 'z')) {
        switch (mode) {
        case 0:
          which->global |=1 << (*string - 'a');

          break;
        case 1:
          which->chan |= 1 << (*string - 'a');
          break;
        case 2:
          which->bot |= 1 << (*string - 'a');
        }
      } else if ((*string >= 'A') && (*string <= 'Z')) {
        switch (mode) {
        case 0:
          which->udef_global |= 1 << (*string - 'A');
          break;
        case 1:
          which->udef_chan |= 1 << (*string - 'A');
          break;
        }
      } else if ((*string >= '0') && (*string <= '9')) {
        switch (mode) {
          /* Map 0->9 to A->K for glob/chan so they are not lost */
        case 0:
          which->udef_global |= 1 << (*string - '0');
          break;
        case 1:
          which->udef_chan |= 1 << (*string - '0');
          break;
        case 2:
          which->bot |= BOT_FLAG0 << (*string - '0');
          break;
        }
      }
    }
    string++;
  }
  for (which = plus; which; which = (which == plus ? minus : 0)) {
    which->global &=USER_VALID;

    which->udef_global &= 0x03ffffff;
    which->chan &= CHAN_VALID;
    which->udef_chan &= 0x03ffffff;
    which->bot &= BOT_VALID;
  }
  plus->match |= flags;
  if (minus) {
    minus->match |= flags;
    if (!(plus->match & (FR_AND | FR_OR)))
      plus->match |= FR_OR;
  }
}

static int flag2str(char *string, int bot, int udef)
{
  char x = 'a', *old = string;

  while (bot && (x <= 'z')) {
    if (bot & 1)
      *string++ = x;
    x++;
    bot = bot >> 1;
  }
  x = 'A';
  while (udef && (x <= 'Z')) {
    if (udef & 1)
      *string++ = x;
    udef = udef >> 1;
    x++;
  }
  if (string == old)
    *string++ = '-';
  return string - old;
}

static int bot2str(char *string, int bot)
{
  char x = 'a', *old = string;

  while (x < 'v') {
    if (bot & 1)
      *string++ = x;
    x++;
    bot >>= 1;
  }
  x = '0';
  while (x <= '9') {
    if (bot & 1)
      *string++ = x;
    x++;
    bot >>= 1;
  }
  if (string == old)
    *string++ = '-';
  return string - old;
}

int build_flags(char *string, struct flag_record *plus,
                struct flag_record *minus)
{
  char *old = string;

  if (plus->match & FR_GLOBAL) {
    if (minus && (plus->global ||plus->udef_global))
      *string++ = '+';
    string += flag2str(string, plus->global, plus->udef_global);

    if (minus && (minus->global ||minus->udef_global)) {
      *string++ = '-';
      string += flag2str(string, minus->global, minus->udef_global);
    }
  } else if (plus->match & FR_BOT) {
    if (minus && plus->bot)
      *string++ = '+';
    string += bot2str(string, plus->bot);
    if (minus && minus->bot) {
      *string++ = '-';
      string += bot2str(string, minus->bot);
    }
  }
  if (plus->match & FR_CHAN) {
    if (plus->match & (FR_GLOBAL | FR_BOT))
      *string++ = (plus->match & FR_AND) ? '&' : '|';
    if (minus && (plus->chan || plus->udef_chan))
      *string++ = '+';
    string += flag2str(string, plus->chan, plus->udef_chan);
    if (minus && (minus->chan || minus->udef_chan)) {
      *string++ = '-';
      string += flag2str(string, minus->global, minus->udef_chan);
    }
  }
  if (string == old) {
    *string++ = '-';
    *string = 0;
    return 0;
  }
  *string = 0;
  return string - old;
}

int flagrec_ok(struct flag_record *req, struct flag_record *have)
{
  /* FIXME: flag masks with '&' in them won't be subject to
   *        further tests below. Example: 'o&j'
   */
  if (req->match & FR_AND)
    return flagrec_eq(req, have);
  else if (req->match & FR_OR) {
    int hav = have->global;

    /* Exception 1 - global +d/+k cant use -|-, unless they are +p */
    if (!req->chan && !req->global && !req->udef_global && !req->udef_chan) {
      if (!allow_dk_cmds) {
        if (glob_party(*have))
          return 1;
        if (glob_kick(*have) || chan_kick(*have))
          return 0;             /* +k cant use -|- commands */
        if (glob_deop(*have) || chan_deop(*have))
          return 0;             /* neither can +d's */
      }
      return 1;
    }
    /* The +n/+m checks aren't needed anymore because +n/+m
     * automatically adds lower flags
     */
    if (!require_p && ((hav & USER_OP) || (have->chan & USER_OWNER)))
      hav |= USER_PARTY;
    if (hav & req->global)
      return 1;
    if (have->chan & req->chan)
      return 1;
    if (have->udef_global & req->udef_global)
      return 1;
    if (have->udef_chan & req->udef_chan)
      return 1;
    return 0;
  }
  return 0;                     /* fr0k3 binding, dont pass it */
}

int flagrec_eq(struct flag_record *req, struct flag_record *have)
{
  if (req->match & FR_AND) {
    if (req->match & FR_GLOBAL) {
      if ((req->global & have->global) !=req->global)
        return 0;
      if ((req->udef_global & have->udef_global) != req->udef_global)
        return 0;
    }
    if (req->match & FR_BOT)
      if ((req->bot & have->bot) != req->bot)
        return 0;
    if (req->match & FR_CHAN) {
      if ((req->chan & have->chan) != req->chan)
        return 0;
      if ((req->udef_chan & have->udef_chan) != req->udef_chan)
        return 0;
    }
    return 1;
  } else if (req->match & FR_OR) {
    if (!req->chan && !req->global && !req->udef_chan &&
        !req->udef_global && !req->bot)
      return 1;
    if (req->match & FR_GLOBAL) {
      if (have->global & req->global)
        return 1;
      if (have->udef_global & req->udef_global)
        return 1;
    }
    if (req->match & FR_BOT)
      if (have->bot & req->bot)
        return 1;
    if (req->match & FR_CHAN) {
      if (have->chan & req->chan)
        return 1;
      if (have->udef_chan & req->udef_chan)
        return 1;
    }
    return 0;
  }
  return 0;                     /* fr0k3 binding, dont pass it */
}

void set_user_flagrec(struct userrec *u, struct flag_record *fr,
                      const char *chname)
{
  struct chanuserrec *cr = NULL;
  int oldflags = fr->match;
  char buffer[100];
  struct chanset_t *ch;

  if (!u)
    return;
  if (oldflags & FR_GLOBAL) {
    u->flags = fr->global;

    u->flags_udef = fr->udef_global;
    if (!noshare && !(u->flags & USER_UNSHARED)) {
      fr->match = FR_GLOBAL;
      build_flags(buffer, fr, NULL);
      shareout(NULL, "a %s %s\n", u->handle, buffer);
    }
  }
  if ((oldflags & FR_BOT) && (u->flags & USER_BOT))
    set_user(&USERENTRY_BOTFL, u, (void *) fr->bot);
  /* Don't share bot attrs */
  if ((oldflags & FR_CHAN) && chname) {
    for (cr = u->chanrec; cr; cr = cr->next)
      if (!rfc_casecmp(chname, cr->channel))
        break;
    ch = findchan_by_dname(chname);
    if (!cr && ch) {
      cr = user_malloc(sizeof(struct chanuserrec));
      egg_bzero(cr, sizeof(struct chanuserrec));

      cr->next = u->chanrec;
      u->chanrec = cr;
      strncpyz(cr->channel, chname, sizeof cr->channel);
    }
    if (cr && ch) {
      cr->flags = fr->chan;
      cr->flags_udef = fr->udef_chan;
      if (!noshare && !(u->flags & USER_UNSHARED) && channel_shared(ch)) {
        fr->match = FR_CHAN;
        build_flags(buffer, fr, NULL);
        shareout(ch, "a %s %s %s\n", u->handle, buffer, chname);
      }
    }
  }
  fr->match = oldflags;
}

/* Always pass the dname (display name) to this function for chname <cybah>
 */
void get_user_flagrec(struct userrec *u, struct flag_record *fr,
                      const char *chname)
{
  struct chanuserrec *cr = NULL;

  if (!u) {
    fr->global = fr->udef_global = fr->chan = fr->udef_chan = fr->bot = 0;

    return;
  }
  if (fr->match & FR_GLOBAL) {
    fr->global = u->flags;

    fr->udef_global = u->flags_udef;
  } else {
    fr->global = 0;

    fr->udef_global = 0;
  }
  if (fr->match & FR_BOT) {
    fr->bot = (long) get_user(&USERENTRY_BOTFL, u);
  } else
    fr->bot = 0;
  if (fr->match & FR_CHAN) {
    if (fr->match & FR_ANYWH) {
      fr->chan = u->flags;
      fr->udef_chan = u->flags_udef;
      for (cr = u->chanrec; cr; cr = cr->next)
        if (findchan_by_dname(cr->channel)) {
          fr->chan |= cr->flags;
          fr->udef_chan |= cr->flags_udef;
        }
    } else {
      if (chname)
        for (cr = u->chanrec; cr; cr = cr->next)
          if (!rfc_casecmp(chname, cr->channel))
            break;
      if (cr) {
        fr->chan = cr->flags;
        fr->udef_chan = cr->flags_udef;
      } else {
        fr->chan = 0;
        fr->udef_chan = 0;
      }
    }
  }
}

static int botfl_unpack(struct userrec *u, struct user_entry *e)
{
  struct flag_record fr = { FR_BOT, 0, 0, 0, 0, 0 };

  break_down_flags(e->u.list->extra, &fr, NULL);
  list_type_kill(e->u.list);
  e->u.ulong = fr.bot;
  return 1;
}

static int botfl_pack(struct userrec *u, struct user_entry *e)
{
  char x[100];
  struct flag_record fr = { FR_BOT, 0, 0, 0, 0, 0 };

  fr.bot = e->u.ulong;
  e->u.list = user_malloc(sizeof(struct list_type));
  e->u.list->next = NULL;
  e->u.list->extra = user_malloc(build_flags(x, &fr, NULL) + 1);
  strcpy(e->u.list->extra, x);
  return 1;
}

static int botfl_kill(struct user_entry *e)
{
  nfree(e);
  return 1;
}

static int botfl_write_userfile(FILE *f, struct userrec *u,
                                struct user_entry *e)
{
  char x[100];
  struct flag_record fr = { FR_BOT, 0, 0, 0, 0, 0 };

  fr.bot = e->u.ulong;
  build_flags(x, &fr, NULL);
  if (fprintf(f, "--%s %s\n", e->type->name, x) == EOF)
    return 0;
  return 1;
}

static int botfl_set(struct userrec *u, struct user_entry *e, void *buf)
{
  register long atr = ((long) buf & BOT_VALID);

  if (!(u->flags & USER_BOT))
    return 1;                   /* Don't even bother trying to set the
                                 * flags for a non-bot */

  if ((atr & BOT_HUB) && (atr & BOT_ALT))
    atr &= ~BOT_ALT;
  if (atr & BOT_REJECT) {
    if (atr & BOT_SHARE)
      atr &= ~(BOT_SHARE | BOT_REJECT);
    if (atr & BOT_HUB)
      atr &= ~(BOT_HUB | BOT_REJECT);
    if (atr & BOT_ALT)
      atr &= ~(BOT_ALT | BOT_REJECT);
  }
  if (!(atr & BOT_SHARE))
    atr &= ~BOT_GLOBAL;
  e->u.ulong = atr;
  return 1;
}

static int botfl_tcl_get(Tcl_Interp *interp, struct userrec *u,
                         struct user_entry *e, int argc, char **argv)
{
  char x[100];
  struct flag_record fr = { FR_BOT, 0, 0, 0, 0, 0 };

  fr.bot = e->u.ulong;
  build_flags(x, &fr, NULL);
  Tcl_AppendResult(interp, x, NULL);
  return TCL_OK;
}

static int botfl_tcl_set(Tcl_Interp *irp, struct userrec *u,
                         struct user_entry *e, int argc, char **argv)
{
  struct flag_record fr = { FR_BOT, 0, 0, 0, 0, 0 };

  BADARGS(4, 4, " handle BOTFL flags");

  if (u->flags & USER_BOT) {
    /* Silently ignore for users */
    break_down_flags(argv[3], &fr, NULL);
    botfl_set(u, e, (void *) fr.bot);
  }
  return TCL_OK;
}

static int botfl_expmem(struct user_entry *e)
{
  return 0;
}

static void botfl_display(int idx, struct user_entry *e)
{
  struct flag_record fr = { FR_BOT, 0, 0, 0, 0, 0 };
  char x[100];

  fr.bot = e->u.ulong;
  build_flags(x, &fr, NULL);
  dprintf(idx, "  BOT FLAGS: %s\n", x);
}

struct user_entry_type USERENTRY_BOTFL = {
  0,                            /* always 0 ;) */
  0,
  def_dupuser,
  botfl_unpack,
  botfl_pack,
  botfl_write_userfile,
  botfl_kill,
  def_get,
  botfl_set,
  botfl_tcl_get,
  botfl_tcl_set,
  botfl_expmem,
  botfl_display,
  "BOTFL"
};
