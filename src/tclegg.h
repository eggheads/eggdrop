/*
 * tclegg.h
 *   stuff used by tcl.c and tclhash.c
 *
 * $Id: tclegg.h,v 1.18 2003/01/28 06:37:24 wcc Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Eggheads Development Team
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

#ifndef _EGG_TCLEGG_H
#define _EGG_TCLEGG_H

#include "lush.h"               /* Include this here, since it's needed
                                 * in this file */
#ifndef MAKING_MODS
#  include "proto.h"            /* This file needs this */
#endif

/* Match types for check_tcl_bind
 */
#define MATCH_PARTIAL       0
#define MATCH_EXACT         1
#define MATCH_MASK          2
#define MATCH_CASE          3

/* Bitwise 'or' these:
 */
#define BIND_USE_ATTR       0x04
#define BIND_STACKABLE      0x08
#define BIND_HAS_BUILTINS   0x10
#define BIND_WANTRET        0x20
#define BIND_ALTER_ARGS     0x40

/* Return values
 */
#define BIND_NOMATCH    0
#define BIND_AMBIGUOUS  1
#define BIND_MATCHED    2       /* But the proc couldn't be found */
#define BIND_EXECUTED   3
#define BIND_EXEC_LOG   4       /* Proc returned 1 -> wants to be logged */
#define BIND_EXEC_BRK   5       /* Proc returned BREAK (quit) */

/* Extra commands are stored in Tcl hash tables (one hash table for each type
 * of command: msg, dcc, etc)
 */
typedef struct timer_str {
  struct timer_str *next;
  unsigned int mins;            /* Time to elapse                       */
  char *cmd;                    /* Command linked to                    */
  unsigned long id;             /* Used to remove timers                */
} tcl_timer_t;


/* Used for stub functions:
 */

#define STDVAR		(cd, irp, argc, argv)				\
	ClientData cd;							\
	Tcl_Interp *irp;						\
	int argc;							\
	char *argv[];
#define BADARGS(nl, nh, example)	do {				\
	if ((argc < (nl)) || (argc > (nh))) {				\
		Tcl_AppendResult(irp, "wrong # args: should be \"",	\
				 argv[0], (example), "\"", NULL);	\
		return TCL_ERROR;					\
	}								\
} while (0)


unsigned long add_timer(tcl_timer_t **, int, char *, unsigned long);
int remove_timer(tcl_timer_t **, unsigned long);
void list_timers(Tcl_Interp *, tcl_timer_t *);
void wipe_timers(Tcl_Interp *, tcl_timer_t **);
void do_check_timers(tcl_timer_t **);

typedef struct _tcl_strings {
  char *name;
  char *buf;
  int length;
  int flags;
} tcl_strings;

typedef struct _tcl_int {
  char *name;
  int *val;
  int readonly;
} tcl_ints;

typedef struct _tcl_coups {
  char *name;
  int *lptr;
  int *rptr;
} tcl_coups;

typedef struct _tcl_cmds {
  char *name;
  Function func;
} tcl_cmds;

typedef struct _cd_tcl_cmd {
  char *name;
  Function callback;
  void *cdata;
} cd_tcl_cmd;

void add_tcl_commands(tcl_cmds *);
void add_cd_tcl_cmds(cd_tcl_cmd *);
void rem_tcl_commands(tcl_cmds *);
void rem_cd_tcl_cmds(cd_tcl_cmd *);
void add_tcl_strings(tcl_strings *);
void rem_tcl_strings(tcl_strings *);
void add_tcl_coups(tcl_coups *);
void rem_tcl_coups(tcl_coups *);
void add_tcl_ints(tcl_ints *);
void rem_tcl_ints(tcl_ints *);

/* From Tcl's tclUnixInit.c */
/* The following table is used to map from Unix locale strings to
 * encoding files.
 */
typedef struct LocaleTable {
  const char *lang;
  const char *encoding;
} LocaleTable;

static const LocaleTable localeTable[] = {
  {"ja_JP.SJIS",    "shiftjis"},
  {"ja_JP.EUC",       "euc-jp"},
  {"ja_JP.JIS",   "iso2022-jp"},
  {"ja_JP.mscode",  "shiftjis"},
  {"ja_JP.ujis",      "euc-jp"},
  {"ja_JP",           "euc-jp"},
  {"Ja_JP",         "shiftjis"},
  {"Jp_JP",         "shiftjis"},
  {"japan",           "euc-jp"},
#ifdef hpux
  {"japanese",      "shiftjis"},
  {"ja",            "shiftjis"},
#else
  {"japanese",        "euc-jp"},
  {"ja",              "euc-jp"},
#endif
  {"japanese.sjis", "shiftjis"},
  {"japanese.euc",    "euc-jp"},
  {"japanese-sjis", "shiftjis"},
  {"japanese-ujis",   "euc-jp"},

  {"ko",              "euc-kr"},
  {"ko_KR",           "euc-kr"},
  {"ko_KR.EUC",       "euc-kr"},
  {"ko_KR.euc",       "euc-kr"},
  {"ko_KR.eucKR",     "euc-kr"},
  {"korean",          "euc-kr"},

  {"ru",           "iso8859-5"},
  {"ru_RU",        "iso8859-5"},
  {"ru_SU",        "iso8859-5"},

  {"zh",               "cp936"},

  {NULL,                  NULL}
};

#endif /* _EGG_TCLEGG_H */
