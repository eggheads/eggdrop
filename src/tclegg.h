/* 
 * tclegg.h
 *   stuff used by tcl.c and tclhash.c
 * 
 * $Id: tclegg.h,v 1.3 1999/12/15 02:32:58 guppy Exp $
 */
/* 
 * Copyright (C) 1997  Robey Pointer
 * Copyright (C) 1999  Eggheads
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

#include "../lush.h"		/* include this here, since it's needed
				 * in this file */
#ifndef MAKING_MODS
#  include "proto.h"		/* this file needs this */
#endif

/* types of commands */
#define CMD_MSG   0
#define CMD_DCC   1
#define CMD_FIL   2
#define CMD_PUB   3
#define CMD_MSGM  4
#define CMD_PUBM  5
#define CMD_JOIN  6
#define CMD_PART  7
#define CMD_SIGN  8
#define CMD_KICK  9
#define CMD_TOPC  10
#define CMD_MODE  11
#define CMD_CTCP  12
#define CMD_CTCR  13
#define CMD_NICK  14
#define CMD_RAW   15
#define CMD_BOT   16
#define CMD_CHON  17
#define CMD_CHOF  18
#define CMD_SENT  19
#define CMD_RCVD  20
#define CMD_CHAT  21
#define CMD_LINK  22
#define CMD_DISC  23
#define CMD_SPLT  24
#define CMD_REJN  25
#define CMD_FILT  26
#define CMD_FLUD  27
#define CMD_NOTE  28
#define CMD_ACT   29
#define CMD_NOTC  30
#define CMD_WALL  31
#define CMD_BCST  32
#define CMD_CHJN  33
#define CMD_CHPT  34
#define CMD_TIME  35
#define BINDS 36

/* match types for check_tcl_bind */
#define MATCH_PARTIAL       0
#define MATCH_EXACT         1
#define MATCH_MASK          2
#define MATCH_CASE          3

/* bitwise 'or' these: */
#define BIND_USE_ATTR       4
#define BIND_STACKABLE      8
#define BIND_HAS_BUILTINS   16
#define BIND_WANTRET        32
#define BIND_ALTER_ARGS     64

/* return values */
#define BIND_NOMATCH    0
#define BIND_AMBIGUOUS  1
#define BIND_MATCHED    2	/* but the proc couldn't be found */
#define BIND_EXECUTED   3
#define BIND_EXEC_LOG   4	/* proc returned 1 -> wants to be logged */
#define BIND_EXEC_BRK   5	/* proc returned BREAK (quit) */

/* extra commands are stored in Tcl hash tables (one hash table for each type
 * of command: msg, dcc, etc) */
typedef struct timer_str {
  unsigned int mins;		/* time to elapse */
  char *cmd;			/* command linked to */
  unsigned long id;		/* used to remove timers */
  struct timer_str *next;
} tcl_timer_t;

/* used for stub functions : */
#define STDVAR (cd,irp,argc,argv) \
  ClientData cd; Tcl_Interp *irp; int argc; char *argv[];
#define BADARGS(nl,nh,example) \
  if ((argc<(nl)) || (argc>(nh))) { \
    Tcl_AppendResult(irp,"wrong # args: should be \"",argv[0], \
		     (example),"\"",NULL); \
    return TCL_ERROR; \
  }

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
  void *func;
} tcl_cmds;

void add_tcl_commands(tcl_cmds *);
void rem_tcl_commands(tcl_cmds *);
void add_tcl_strings(tcl_strings *);
void rem_tcl_strings(tcl_strings *);
void add_tcl_coups(tcl_coups *);
void rem_tcl_coups(tcl_coups *);
void add_tcl_ints(tcl_ints *);
void rem_tcl_ints(tcl_ints *);

#endif				/* _EGG_TCLEGG_H */
