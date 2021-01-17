/*
 * flags.c -- handles:
 *   all the flag matching/conversion functions in one neat package :)
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
    case 'l':
    case 'L':
      res |= LOG_BOTMSG;
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
      res |= raw_log ? LOG_BOTNETIN : 0;
      break;
    case 'u':
    case 'U':
      res |= raw_log ? LOG_BOTNETOUT : 0;
      break;
    case 'h':
    case 'H':
      res |= raw_log ? LOG_BOTSHRIN : 0;
      break;
    case 'g':
    case 'G':
      res |= raw_log ? LOG_BOTSHROUT : 0;
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
  static char s[27];            /* Change this if you change the levels */
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
  if (x & LOG_BOTMSG)
    *p++ = 'l';
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
  if ((x & LOG_BOTNETIN) && raw_log)
    *p++ = 't';
  if ((x & LOG_BOTNETOUT) && raw_log)
    *p++ = 'u';
  if ((x & LOG_BOTSHRIN) && raw_log)
    *p++ = 'h';
  if ((x & LOG_BOTSHROUT) && raw_log)
    *p++ = 'g';
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
  static char s[275]; /* Change this if you change the levels */
  int i = 0;          /* 6+8+7+13+6+6+6+17+5+7+8+7+9+15+17+17+24+24+(8*9)+1 */

  s[0] = 0;
  if (x & LOG_MSGS)
    i += my_strcpy(s, "msgs, "); /* 6 */
  if (x & LOG_PUBLIC)
    i += my_strcpy(s + i, "public, "); /* 8 */
  if (x & LOG_JOIN)
    i += my_strcpy(s + i, "joins, "); /* 7 */
  if (x & LOG_MODES)
    i += my_strcpy(s + i, "kicks/modes, "); /* 13 */
  if (x & LOG_CMDS)
    i += my_strcpy(s + i, "cmds, "); /* 6 */
  if (x & LOG_MISC)
    i += my_strcpy(s + i, "misc, "); /* 6 */
  if (x & LOG_BOTS)
    i += my_strcpy(s + i, "bots, "); /* 6 */
  if (x & LOG_BOTMSG)
    i += my_strcpy(s + i, "linked bot msgs, "); /* 17 */
  if ((x & LOG_RAW) && raw_log)
    i += my_strcpy(s + i, "raw, "); /* 5 */
  if (x & LOG_FILES)
    i += my_strcpy(s + i, "files, "); /* 7 */
  if (x & LOG_SERV)
    i += my_strcpy(s + i, "server input, "); /* 8 */
  if (x & LOG_DEBUG)
    i += my_strcpy(s + i, "debug, "); /* 7 */
  if (x & LOG_WALL)
    i += my_strcpy(s + i, "wallops, "); /* 9 */
  if ((x & LOG_SRVOUT) && raw_log)
    i += my_strcpy(s + i, "server output, "); /* 15 */
  if ((x & LOG_BOTNETIN) && raw_log)
    i += my_strcpy(s + i, "botnet incoming, "); /* 17 */
  if ((x & LOG_BOTNETOUT) && raw_log)
    i += my_strcpy(s + i, "botnet outgoing, "); /* 17 */
  if ((x & LOG_BOTSHRIN) && raw_log)
    i += my_strcpy(s + i, "incoming share traffic, "); /* 24 */
  if ((x & LOG_BOTSHROUT) && raw_log)
    i += my_strcpy(s + i, "outgoing share traffic, "); /* 24 */
  if (x & LOG_LEV1)
    i += my_strcpy(s + i, "level 1, "); /* 9 */
  if (x & LOG_LEV2)
    i += my_strcpy(s + i, "level 2, "); /* 9 */
  if (x & LOG_LEV3)
    i += my_strcpy(s + i, "level 3, "); /* 9 */
  if (x & LOG_LEV4)
    i += my_strcpy(s + i, "level 4, "); /* 9 */
  if (x & LOG_LEV5)
    i += my_strcpy(s + i, "level 5, "); /* 9 */
  if (x & LOG_LEV6)
    i += my_strcpy(s + i, "level 6, "); /* 9 */
  if (x & LOG_LEV7)
    i += my_strcpy(s + i, "level 7, "); /* 9 */
  if (x & LOG_LEV8)
    i += my_strcpy(s + i, "level 8, "); /* 9 */
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

/* Sanity check on user attributes
 * Wanted attributes (pls_atr) take precedence over existing (atr) attributes
 * which can cause conflict.
 * Arguments are: pointer to old attr (to modify attr to save),
 *		  attr to add, attr to remove
 * Returns: int with number corresponding to zero or more messages
 */
int user_sanity_check(int * const atr, int const pls_atr, int const min_atr)
{
  /* bitwise sum of msg id's defined in flags.h */
  int msgids = 0;
  /* save prior state to have a history */
  int old_atr = *atr;
  int chg = (pls_atr & ~min_atr);
  /* apply changes */
  *atr = (*atr | pls_atr) & ~min_atr;

  /* +b (bot) and ... */
  if ((*atr & USER_BOT)) {
    /* ... +p (partyline) */
    if (*atr & USER_PARTY) {
      *atr &= ~USER_PARTY;
      msgids |= UC_SANE_BOTOWNSPARTY;
    }
    /* ... +m (master) */
    if (*atr & USER_MASTER) {
      *atr &= ~USER_MASTER;
      msgids |= UC_SANE_BOTOWNSMASTER;
    }
    /* ... +c (common) */
    if (*atr & USER_COMMON) {
      *atr &= ~USER_COMMON;
      msgids |= UC_SANE_BOTOWNSCOMMON;
    }
    /* ... +n (owner) */
    if (*atr & USER_OWNER) {
      *atr &= ~USER_OWNER;
      msgids |= UC_SANE_BOTOWNSOWNER;
    }
  }

  /* +o (op) and +d (deop) */
  if ((*atr & USER_OP) && (*atr & USER_DEOP)) {
    if ((chg & USER_OP) && (chg & USER_DEOP)) {
      /* +do wanted */
      *atr &= ~(USER_DEOP | USER_OP);
      if (old_atr & USER_DEOP)
        *atr |= USER_DEOP;
      else if (old_atr & USER_OP)
        *atr |= USER_OP;
      msgids |= UC_SANE_OWNSDEOPOP;
    } else if (old_atr & USER_OP) {
      /* +d wanted */
      *atr &= ~USER_OP;
      msgids |= UC_SANE_DEOPOWNSOP;
    } else if (old_atr & USER_DEOP) {
      /* +o wanted */
      *atr &= ~USER_DEOP;
      msgids |= UC_SANE_OPOWNSDEOP;
    } else {
      /* +do wanted */
      *atr &= ~(USER_DEOP | USER_OP);
      msgids |= UC_SANE_OWNSDEOPOP;
    }
  }

  /* +l (halfop) and +r (dehalfop) */
  if ((*atr & USER_HALFOP) && (*atr & USER_DEHALFOP)) {
    if ((chg & USER_HALFOP) && (chg & USER_DEHALFOP)) {
      /* +rl wanted */
      *atr &= ~(USER_DEHALFOP | USER_HALFOP);
      if (old_atr & USER_DEHALFOP)
        *atr |= USER_DEHALFOP;
      else if (old_atr & USER_HALFOP)
        *atr |= USER_HALFOP;
      msgids |= UC_SANE_OWNSDEHALFOPHALFOP;
    } else if (old_atr & USER_HALFOP) {
      /* +r wanted */
      *atr &= ~USER_HALFOP;
      msgids |= UC_SANE_DEHALFOPOWNSHALFOP;
    } else if (old_atr & USER_DEHALFOP) {
      /* +l wanted */
      *atr &= ~USER_DEHALFOP;
      msgids |= UC_SANE_HALFOPOWNSDEHALFOP;
    } else {
      /* +rl wanted */
      *atr &= ~(USER_DEHALFOP | USER_HALFOP);
      msgids |= UC_SANE_OWNSDEHALFOPHALFOP;
    }
  }

  /* +a (autop) and +d (deop) */
  if ((*atr & USER_AUTOOP) && (*atr & USER_DEOP)) {
    if ((chg & USER_AUTOOP) && (chg & USER_DEOP)) {
      /* +da wanted */
      *atr &= ~(USER_DEOP | USER_AUTOOP);
      if (old_atr & USER_DEOP)
        *atr |= USER_DEOP;
      else if (old_atr & USER_AUTOOP)
        *atr |= USER_AUTOOP;
      msgids |= UC_SANE_OWNSDEOPAUTOOP;
    } else if (old_atr & USER_AUTOOP) {
      /* +d wanted */
      *atr &= ~USER_AUTOOP;
      msgids |= UC_SANE_DEOPOWNSAUTOOP;
    } else if (old_atr & USER_DEOP) {
      /* +a wanted */
      *atr &= ~USER_DEOP;
      msgids |= UC_SANE_AUTOOPOWNSDEOP;
    } else {
      /* +da wanted */
      *atr &= ~(USER_DEOP | USER_AUTOOP);
      msgids |= UC_SANE_OWNSDEOPAUTOOP;
    }
  }

  /* +y (autohalfop) and +r (dehalfop) */
  if ((*atr & USER_AUTOHALFOP) && (*atr & USER_DEHALFOP)) {
    if ((chg & USER_AUTOHALFOP) && (chg & USER_DEHALFOP)) {
      /* +ry wanted */
      *atr &= ~(USER_DEHALFOP | USER_AUTOHALFOP);
      if (old_atr & USER_DEHALFOP)
        *atr |= USER_DEHALFOP;
      else if (old_atr & USER_AUTOHALFOP)
        *atr |= USER_AUTOHALFOP;
      msgids |= UC_SANE_OWNSDEHALFOPAHALFOP;
    } else if (old_atr & USER_AUTOHALFOP) {
      /* +r wanted */
      *atr &= ~USER_AUTOHALFOP;
      msgids |= UC_SANE_DEHALFOPOWNSAHALFOP;
    } else if (old_atr & USER_DEHALFOP) {
      /* +y wanted */
      *atr &= ~USER_DEHALFOP;
      msgids |= UC_SANE_AHALFOPOWNSDEHALFOP;
    } else {
      /* +ry wanted */
      *atr &= ~(USER_DEHALFOP | USER_AUTOHALFOP);
      msgids |= UC_SANE_OWNSDEHALFOPAHALFOP;
    }
  }

  /* +v (voice) and +q (quiet) */
  if ((*atr & USER_VOICE) && (*atr & USER_QUIET)) {
    if ((chg & USER_VOICE) && (chg & USER_QUIET)) {
      /* +qv wanted */
      *atr &= ~(USER_QUIET | USER_VOICE);
      if (old_atr & USER_QUIET)
        *atr |= USER_QUIET;
      else if (old_atr & USER_VOICE)
        *atr |= USER_VOICE;
      msgids |= UC_SANE_OWNSQUIETVOICE;
    } else if (old_atr & USER_VOICE) {
      /* +q wanted */
      *atr &= ~USER_VOICE;
      msgids |= UC_SANE_QUIETOWNSVOICE;
    } else if (old_atr & USER_QUIET) {
      /* +v wanted */
      *atr &= ~USER_QUIET;
      msgids |= UC_SANE_VOICEOWNSQUIET;
    } else {
      /* +qv wanted */
      *atr &= ~(USER_QUIET | USER_VOICE);
      msgids |= UC_SANE_OWNSQUIETVOICE;
    }
  }

  /* +g (autovoice) and +q (quiet) */
  if ((*atr & USER_GVOICE) && (*atr & USER_QUIET)) {
    if ((chg & USER_GVOICE) && (chg & USER_QUIET)) {
      /* +qg wanted */
      *atr &= ~(USER_QUIET | USER_GVOICE);
      if (old_atr & USER_QUIET)
        *atr |= USER_QUIET;
      else if (old_atr & USER_GVOICE)
        *atr |= USER_GVOICE;
      msgids |= UC_SANE_OWNSQUIETGVOICE;
    } else if (old_atr & USER_GVOICE) {
      /* +q wanted */
      *atr &= ~USER_GVOICE;
      msgids |= UC_SANE_QUIETOWNSGVOICE;
    } else if (old_atr & USER_QUIET) {
      /* +g wanted */
      *atr &= ~USER_QUIET;
      msgids |= UC_SANE_GVOICEOWNSQUIET;
    } else {
      /* +qg wanted */
      *atr &= ~(USER_QUIET | USER_GVOICE);
      msgids |= UC_SANE_OWNSQUIETGVOICE;
    }
  }

  /* +a (autoop) adds +o (op), but +o can be removed */
  if ((*atr & USER_AUTOOP) && (chg & USER_AUTOOP) && !(old_atr & USER_OP)) {
    *atr |= USER_OP;
    msgids |= UC_SANE_AUTOOPADDSOP;
  }

  /* +y (autohalfop) adds +l (halfop), but +l can be removed */
  if ((*atr & USER_AUTOHALFOP) && (chg & USER_AUTOHALFOP) && !(old_atr & USER_HALFOP)) {
    *atr |= USER_HALFOP;
    msgids |= UC_SANE_AUTOHALFOPADDSHALFOP;
  }

  /* +g (autovoice) adds +v (voice), but +v can be removed */
  if ((*atr & USER_GVOICE) && (chg & USER_GVOICE) && !(old_atr & USER_VOICE)) {
    *atr |= USER_VOICE;
    msgids |= UC_SANE_GVOICEADDSVOICE;
  }

  /* Don't change the order, first owner, then master, then botmaster,
   * then janitor, then op */

  /* +n (owner) and +m (master) */
  /* Can't be channel owner without also being channel master */
  if ((*atr & USER_OWNER) && !(*atr & USER_MASTER)) {
    *atr |= USER_MASTER;
    msgids |= UC_SANE_OWNERADDSMASTER;
  }

  /* +m (master) and +t (botmaster), +o (op), +j (janitor) */
  /* Master implies botmaster, op and janitor */
  if ((*atr & USER_MASTER) && !(*atr & (USER_BOTMAST | USER_OP | USER_JANITOR))) {
    *atr |= USER_BOTMAST | USER_OP | USER_JANITOR;
    msgids |= UC_SANE_MASTERADDSBOTMOPJAN;
  }

  /* +t (botmaster) and +p (partyline) */
  /* Can't be botnet master without party-line access */
  if ((*atr & USER_BOTMAST) && !(*atr & USER_PARTY)) {
    *atr |= USER_PARTY;
    msgids |= UC_SANE_BOTMASTADDSPARTY;
  }

  /* +j (janitor) and +x (xfer) */
  /* Janitors can use the file area */
  if ((*atr & USER_JANITOR) && !(*atr & USER_XFER)) {
    *atr |= USER_XFER;
    msgids |= UC_SANE_JANADDSXFER;
  }

  /* +o (op) and +l (halfop) */
  /* Op implies halfop */
  if ((*atr & USER_OP) && !(*atr & USER_HALFOP)) {
    *atr |= USER_HALFOP;
    msgids |= UC_SANE_OPADDSHALFOP;
  }

  return msgids;
}

/* Sanity check on channel attributes
 * Wanted attributes (pls_atr) take precedence over existing (atr) attributes
 * which can cause conflict.
 * Arguments are: pointer to old attr (to modify attr to save),
 *		  attr to add, attr to remove, user attr
 * Returns: int with number corresponding to zero or more messages
 */
int chan_sanity_check(int * const atr, int const pls_atr, int const min_atr, int const usr_atr)
{
  /* bitwise sum of msg id's defined in flags.h */
  int msgids = 0;
  /* save prior state to have a history */
  int old_atr = *atr;
  int chg = (pls_atr & ~min_atr);
  /* apply changes */
  *atr = (*atr | pls_atr) & ~min_atr;

  /* +o (op) and +d (deop) */
  if ((*atr & USER_OP) && (*atr & USER_DEOP)) {
    if ((chg & USER_OP) && (chg & USER_DEOP)) {
      /* +do wanted */
      *atr &= ~(USER_DEOP | USER_OP);
      if (old_atr & USER_DEOP)
        *atr |= USER_DEOP;
      else if (old_atr & USER_OP)
        *atr |= USER_OP;
      msgids |= UC_SANE_OWNSDEOPOP;
    } else if (old_atr & USER_OP) {
      /* +d wanted */
      *atr &= ~USER_OP;
      msgids |= UC_SANE_DEOPOWNSOP;
    } else if (old_atr & USER_DEOP) {
      /* +o wanted */
      *atr &= ~USER_DEOP;
      msgids |= UC_SANE_OPOWNSDEOP;
    } else {
      /* +do wanted */
      *atr &= ~(USER_DEOP | USER_OP);
      msgids |= UC_SANE_OWNSDEOPOP;
    }
  }

  /* +l (halfop) and +r (dehalfop) */
  if ((*atr & USER_HALFOP) && (*atr & USER_DEHALFOP)) {
    if ((chg & USER_HALFOP) && (chg & USER_DEHALFOP)) {
      /* +rl wanted */
      *atr &= ~(USER_DEHALFOP | USER_HALFOP);
      if (old_atr & USER_DEHALFOP)
        *atr |= USER_DEHALFOP;
      else if (old_atr & USER_HALFOP)
        *atr |= USER_HALFOP;
      msgids |= UC_SANE_OWNSDEHALFOPHALFOP;
    } else if (old_atr & USER_HALFOP) {
      /* +r wanted */
      *atr &= ~USER_HALFOP;
      msgids |= UC_SANE_DEHALFOPOWNSHALFOP;
    } else if (old_atr & USER_DEHALFOP) {
      /* +l wanted */
      *atr &= ~USER_DEHALFOP;
      msgids |= UC_SANE_HALFOPOWNSDEHALFOP;
    } else {
      /* +rl wanted */
      *atr &= ~(USER_DEHALFOP | USER_HALFOP);
      msgids |= UC_SANE_OWNSDEHALFOPHALFOP;
    }
  }

  /* +a (autop) and +d (deop) */
  if ((*atr & USER_AUTOOP) && (*atr & USER_DEOP)) {
    if ((chg & USER_AUTOOP) && (chg & USER_DEOP)) {
      /* +da wanted */
      *atr &= ~(USER_DEOP | USER_AUTOOP);
      if (old_atr & USER_DEOP)
        *atr |= USER_DEOP;
      else if (old_atr & USER_AUTOOP)
        *atr |= USER_AUTOOP;
      msgids |= UC_SANE_OWNSDEOPAUTOOP;
    } else if (old_atr & USER_AUTOOP) {
      /* +d wanted */
      *atr &= ~USER_AUTOOP;
      msgids |= UC_SANE_DEOPOWNSAUTOOP;
    } else if (old_atr & USER_DEOP) {
      /* +a wanted */
      *atr &= ~USER_DEOP;
      msgids |= UC_SANE_AUTOOPOWNSDEOP;
    } else {
      /* +da wanted */
      *atr &= ~(USER_DEOP | USER_AUTOOP);
      msgids |= UC_SANE_OWNSDEOPAUTOOP;
    }
  }

  /* +y (autohalfop) and +r (dehalfop) */
  if ((*atr & USER_AUTOHALFOP) && (*atr & USER_DEHALFOP)) {
    if ((chg & USER_AUTOHALFOP) && (chg & USER_DEHALFOP)) {
      /* +ry wanted */
      *atr &= ~(USER_DEHALFOP | USER_AUTOHALFOP);
      if (old_atr & USER_DEHALFOP)
        *atr |= USER_DEHALFOP;
      else if (old_atr & USER_AUTOHALFOP)
        *atr |= USER_AUTOHALFOP;
      msgids |= UC_SANE_OWNSDEHALFOPAHALFOP;
    } else if (old_atr & USER_AUTOHALFOP) {
      /* +r wanted */
      *atr &= ~USER_AUTOHALFOP;
      msgids |= UC_SANE_DEHALFOPOWNSAHALFOP;
    } else if (old_atr & USER_DEHALFOP) {
      /* +y wanted */
      *atr &= ~USER_DEHALFOP;
      msgids |= UC_SANE_AHALFOPOWNSDEHALFOP;
    } else {
      /* +ry wanted */
      *atr &= ~(USER_DEHALFOP | USER_AUTOHALFOP);
      msgids |= UC_SANE_OWNSDEHALFOPAHALFOP;
    }
  }

  /* +v (voice) and +q (quiet) */
  if ((*atr & USER_VOICE) && (*atr & USER_QUIET)) {
    if ((chg & USER_VOICE) && (chg & USER_QUIET)) {
      /* +qv wanted */
      *atr &= ~(USER_QUIET | USER_VOICE);
      if (old_atr & USER_QUIET)
        *atr |= USER_QUIET;
      else if (old_atr & USER_VOICE)
        *atr |= USER_VOICE;
      msgids |= UC_SANE_OWNSQUIETVOICE;
    } else if (old_atr & USER_VOICE) {
      /* +q wanted */
      *atr &= ~USER_VOICE;
      msgids |= UC_SANE_QUIETOWNSVOICE;
    } else if (old_atr & USER_QUIET) {
      /* +v wanted */
      *atr &= ~USER_QUIET;
      msgids |= UC_SANE_VOICEOWNSQUIET;
    } else {
      /* +qv wanted */
      *atr &= ~(USER_QUIET | USER_VOICE);
      msgids |= UC_SANE_OWNSQUIETVOICE;
    }
  }

  /* +g (autovoice) and +q (quiet) */
  if ((*atr & USER_GVOICE) && (*atr & USER_QUIET)) {
    if ((chg & USER_GVOICE) && (chg & USER_QUIET)) {
      /* +qg wanted */
      *atr &= ~(USER_QUIET | USER_GVOICE);
      if (old_atr & USER_QUIET)
        *atr |= USER_QUIET;
      else if (old_atr & USER_GVOICE)
        *atr |= USER_GVOICE;
      msgids |= UC_SANE_OWNSQUIETGVOICE;
    } else if (old_atr & USER_GVOICE) {
      /* +q wanted */
      *atr &= ~USER_GVOICE;
      msgids |= UC_SANE_QUIETOWNSGVOICE;
    } else if (old_atr & USER_QUIET) {
      /* +g wanted */
      *atr &= ~USER_QUIET;
      msgids |= UC_SANE_GVOICEOWNSQUIET;
    } else {
      /* +qg wanted */
      *atr &= ~(USER_QUIET | USER_GVOICE);
      msgids |= UC_SANE_OWNSQUIETGVOICE;
    }
  }

  /* +a (autoop) adds +o (op), but +o can be removed */
  if ((*atr & USER_AUTOOP) && (chg & USER_AUTOOP) && !(old_atr & USER_OP)) {
    *atr |= USER_OP;
    msgids |= UC_SANE_AUTOOPADDSOP;
  }

  /* +y (autohalfop) adds +l (halfop), but +l can be removed */
  if ((*atr & USER_AUTOHALFOP) && (chg & USER_AUTOHALFOP) && !(old_atr & USER_HALFOP)) {
    *atr |= USER_HALFOP;
    msgids |= UC_SANE_AUTOHALFOPADDSHALFOP;
  }

  /* +g (autovoice) adds +v (voice), but +v can be removed */
  if ((*atr & USER_GVOICE) && (chg & USER_GVOICE) && !(old_atr & USER_VOICE)) {
    *atr |= USER_VOICE;
    msgids |= UC_SANE_GVOICEADDSVOICE;
  }

  /* Don't change the order, first owner, then master, then op */

  /* +n (owner) and +m (master) */
  /* Can't be channel owner without also being channel master */
  if ((*atr & USER_OWNER) && !(*atr & USER_MASTER)) {
    *atr |= USER_MASTER;
    msgids |= UC_SANE_OWNERADDSMASTER;
  }

  /* +m (master) and +o (op) */
  /* Master implies op */
  if ((*atr & USER_MASTER) && !(*atr & USER_OP)) {
    *atr |= USER_OP;
    msgids |= UC_SANE_MASTERADDSOP;
  }

  /* +o (op) and +l (halfop) */
  /* Op implies halfop */
  if ((*atr & USER_OP) && !(*atr & USER_HALFOP)) {
    *atr |= USER_HALFOP;
    msgids |= UC_SANE_OPADDSHALFOP;
  }

  /* +b user (bot) and +s (bot aggressive share) */
  /* Can't be +s on chan unless you're a bot */
  if (!(usr_atr & USER_BOT) && (*atr & BOT_AGGRESSIVE)) {
    *atr &= ~BOT_AGGRESSIVE;
    msgids |= UC_SANE_NOBOTOWNSAGGR;
  }

  return msgids;
}

/* Sanity check on bot attributes
 * Wanted attributes (pls_atr) take precedence over existing (atr) attributes
 * which can cause conflict.
 * Arguments are: pointer to old attr (to modify attr to save),
 *		  attr to add, attr to remove
 * Returns: int with number corresponding to zero or more messages
 */
int bot_sanity_check(intptr_t * const atr, intptr_t const pls_atr, intptr_t const min_atr)
{
  /* bitwise sum of msg id's defined in flags.h */
  int msgids = 0;
  /* save prior state to have a history */
  intptr_t old_atr = *atr;
  intptr_t chg = (pls_atr & ~min_atr);
  /* apply changes */
  *atr = (*atr | pls_atr) & ~min_atr;

  /* +h (hub) and +a (alt hub) */
  if ((*atr & BOT_HUB) && (*atr & BOT_ALT)) {
    if ((chg & BOT_HUB) && (chg & BOT_ALT)) {
      /* +ah wanted */
      *atr &= ~(BOT_ALT | BOT_HUB);
      if (old_atr & BOT_ALT)
        *atr |= BOT_ALT;
      else if (old_atr & BOT_HUB)
        *atr |= BOT_HUB;
      msgids |= BOT_SANE_OWNSALTHUB;
    } else if (old_atr & BOT_HUB) {
      /* +a wanted */
      *atr &= ~BOT_HUB;
      msgids |= BOT_SANE_ALTOWNSHUB;
    } else if (old_atr & BOT_ALT) {
      /* +h wanted */
      *atr &= ~BOT_ALT;
      msgids |= BOT_SANE_HUBOWNSALT;
    } else {
      /* +ah wanted */
      *atr &= ~(BOT_ALT | BOT_HUB);
      msgids |= BOT_SANE_OWNSALTHUB;
    }
  }

  /* +b|c|e|j|n|u|d (granular sharing) and ... */
  if (*atr & BOT_SHPERMS) {
    /* ... +s (aggressive sharing) */
    if (*atr & BOT_AGGRESSIVE) {
      if ((chg & BOT_AGGRESSIVE) && (chg & BOT_SHPERMS)) {
        /* +(b|c|e|j|n|u)s wanted */
        *atr &= ~(BOT_SHPERMS | BOT_AGGRESSIVE);
        if (old_atr & BOT_AGGRESSIVE)
          *atr |= BOT_AGGRESSIVE;
        else if (old_atr & BOT_SHPERMS)
          *atr |= BOT_SHPERMS;
        msgids |= BOT_SANE_OWNSSHPAGGR;
      } else if (old_atr & BOT_AGGRESSIVE) {
        /* +b|c|e|j|n|u|d wanted */
        *atr &= ~BOT_AGGRESSIVE;
        msgids |= BOT_SANE_SHPOWNSAGGR;
      } else if (old_atr & BOT_SHPERMS) {
        /* +s wanted */
        *atr &= ~BOT_SHPERMS;
        msgids |= BOT_SANE_AGGROWNSSHP;
      } else {
        /* +(b|c|e|j|n|u)s wanted */
        *atr &= ~(BOT_SHPERMS | BOT_AGGRESSIVE);
        msgids |= BOT_SANE_OWNSSHPAGGR;
      }
    }
    /* ... +p (passive sharing) */
    if (*atr & BOT_PASSIVE) {
      if ((chg & BOT_PASSIVE) && (chg & BOT_SHPERMS)) {
        /* +(b|c|e|j|n|u)p wanted */
        *atr &= ~(BOT_SHPERMS | BOT_PASSIVE);
        if (old_atr & BOT_PASSIVE)
          *atr |= BOT_PASSIVE;
        else if (old_atr & BOT_SHPERMS)
          *atr |= BOT_SHPERMS;
        msgids |= BOT_SANE_OWNSSHPPASS;
      } else if (old_atr & BOT_PASSIVE) {
        /* +b|c|e|j|n|u|d wanted */
        *atr &= ~BOT_PASSIVE;
        msgids |= BOT_SANE_SHPOWNSPASS;
      } else if (old_atr & BOT_SHPERMS) {
        /* +s wanted */
        *atr &= ~BOT_SHPERMS;
        msgids |= BOT_SANE_PASSOWNSSHP;
      } else {
        /* +(b|c|e|j|n|u)p wanted */
        *atr &= ~(BOT_SHPERMS | BOT_PASSIVE);
        msgids |= BOT_SANE_OWNSSHPPASS;
      }
    }
  }

  /* +r (reject) and ... */
  if (*atr & BOT_REJECT) {
    /* ... +p|s|b|c|e|j|n|u|d (sharing) */
    if (*atr & BOT_SHARE) {
      if ((chg & BOT_SHARE) && (chg & BOT_REJECT)) {
        /* +(p|s|b|c|e|j|n|u|d)r wanted */
        *atr &= ~(BOT_SHARE | BOT_REJECT);
        if (old_atr & BOT_SHARE)
          *atr |= BOT_SHARE;
        else if (old_atr & BOT_REJECT)
          *atr |= BOT_REJECT;
        msgids |= BOT_SANE_OWNSSHAREREJ;
      } else if (old_atr & BOT_REJECT) {
        /* +p|s|b|c|e|j|n|u|d wanted */
        *atr &= ~BOT_REJECT;
        msgids |= BOT_SANE_SHAREOWNSREJ;
      } else if (old_atr & BOT_SHARE) {
        /* +r wanted */
        *atr &= ~BOT_SHARE;
        msgids |= BOT_SANE_REJOWNSSHARE;
      } else {
        /* +(p|s|b|c|e|j|n|u|d)r wanted */
        *atr &= ~(BOT_SHARE | BOT_REJECT);
        msgids |= BOT_SANE_OWNSSHAREREJ;
      }
    }
    /* ... +h (hub) */
    if (*atr & BOT_HUB) {
      if ((chg & BOT_HUB) && (chg & BOT_REJECT)) {
        /* +hr wanted */
        *atr &= ~(BOT_HUB | BOT_REJECT);
        if (old_atr & BOT_HUB)
          *atr |= BOT_HUB;
        else if (old_atr & BOT_REJECT)
          *atr |= BOT_REJECT;
        msgids |= BOT_SANE_OWNSHUBREJ;
      } else if (old_atr & BOT_REJECT) {
        /* +h wanted */
        *atr &= ~BOT_REJECT;
        msgids |= BOT_SANE_HUBOWNSREJ;
      } else if (old_atr & BOT_HUB) {
        /* +r wanted */
        *atr &= ~BOT_HUB;
        msgids |= BOT_SANE_REJOWNSHUB;
      } else {
        /* +hr wanted */
        *atr &= ~(BOT_HUB | BOT_REJECT);
        msgids |= BOT_SANE_OWNSHUBREJ;
      }
    }
    /* ... +a (alt hub) */
    if (*atr & BOT_ALT) {
      if ((chg & BOT_ALT) && (chg & BOT_REJECT)) {
        /* +ar wanted */
        *atr &= ~(BOT_ALT | BOT_REJECT);
        if (old_atr & BOT_ALT)
          *atr |= BOT_ALT;
        else if (old_atr & BOT_REJECT)
          *atr |= BOT_REJECT;
        msgids |= BOT_SANE_OWNSALTREJ;
      } else if (old_atr & BOT_REJECT) {
        /* +a wanted */
        *atr &= ~BOT_REJECT;
        msgids |= BOT_SANE_ALTOWNSREJ;
      } else if (old_atr & BOT_ALT) {
        /* +r wanted */
        *atr &= ~BOT_ALT;
        msgids |= BOT_SANE_REJOWNSALT;
      } else {
        /* +ar wanted */
        *atr &= ~(BOT_ALT | BOT_REJECT);
        msgids |= BOT_SANE_OWNSALTREJ;
      }
    }
  }

  /* +p (passive) and +s (aggressive) */
  if ((*atr & BOT_PASSIVE) && (*atr & BOT_AGGRESSIVE)) {
    if ((chg & BOT_PASSIVE) && (chg & BOT_AGGRESSIVE)) {
      /* +ps wanted */
      *atr &= ~(BOT_AGGRESSIVE | BOT_PASSIVE);
      if (old_atr & BOT_AGGRESSIVE)
        *atr |= BOT_AGGRESSIVE;
      else if (old_atr & BOT_PASSIVE)
        *atr |= BOT_PASSIVE;
      msgids |= BOT_SANE_OWNSAGGRPASS;
    } else if (old_atr & BOT_PASSIVE) {
      /* +s wanted */
      *atr &= ~BOT_PASSIVE;
      msgids |= BOT_SANE_AGGROWNSPASS;
    } else if (old_atr & BOT_AGGRESSIVE) {
      /* +p wanted */
      *atr &= ~BOT_AGGRESSIVE;
      msgids |= BOT_SANE_PASSOWNSAGGR;
    } else {
      /* +ps wanted */
      *atr &= ~(BOT_AGGRESSIVE | BOT_PASSIVE);
      msgids |= BOT_SANE_OWNSAGGRPASS;
    }
  }

  /* no +p|s|b|c|e|j|n|u|d (sharing) and +g (allchanshared) */
  if (!(*atr & BOT_SHARE) && (*atr & BOT_GLOBAL)) {
    if (old_atr & BOT_GLOBAL) {
      /* no +p|s|b|c|e|j|n|u|d anymore */
      *atr &= ~BOT_GLOBAL;
      msgids |= BOT_SANE_NOSHAREOWNSGLOB;
    } else if (!(old_atr & BOT_SHARE)) {
      /* +g wanted */
      *atr &= ~BOT_GLOBAL;
      msgids |= BOT_SANE_OWNSGLOB;
    }
  }

  return msgids;
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
      return;                   /* We don't actually want any..huh? */
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
        goto breakout; /* string = ""; does not work here because we need to
                          break out of while() / nested switch(), see
                          "string++;" below and string = "\0"; is worse than
                          goto */
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
          if (*string <= 'u')
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
breakout:
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

  while (x <= 'u') {
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
      string += flag2str(string, minus->chan, minus->udef_chan);
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
  return 0;                     /* fr0k3 binding, don't pass it */
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
  return 0;                     /* fr0k3 binding, don't pass it */
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
      strlcpy(cr->channel, chname, sizeof cr->channel);
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
  long atr = ((long) buf & BOT_VALID);

  if (!(u->flags & USER_BOT))
    return 1;                   /* Don't even bother trying to set the
                                 * flags for a non-bot */

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
