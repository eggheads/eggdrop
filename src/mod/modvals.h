/* 
 * modvals.h
 * 
 * $Id: modvals.h,v 1.8 1999/12/15 02:32:58 guppy Exp $
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
#ifndef _EGG_MOD_MODVALS_H
#define _EGG_MOD_MODVALS_H

/* these *were* something, but changed */
#define HOOK_GET_FLAGREC	  0
#define HOOK_BUILD_FLAGREC	  1
#define HOOK_SET_FLAGREC	  2
#define HOOK_READ_USERFILE	  3
#define HOOK_REHASH		  4
#define HOOK_MINUTELY		  5
#define HOOK_DAILY		  6
#define HOOK_HOURLY		  7
#define HOOK_USERFILE		  8
#define HOOK_SECONDLY		  9
#define HOOK_PRE_REHASH		 10
#define HOOK_IDLE		 11
#define HOOK_5MINUTELY		 12
#define REAL_HOOKS		 13
#define HOOK_SHAREOUT		105
#define HOOK_SHAREIN		106
#define HOOK_ENCRYPT_PASS	107
#define HOOK_QSERV		108
#define HOOK_ADD_MODE		109
#define HOOK_MATCH_NOTEREJ	110
#define HOOK_RFC_CASECMP	111

/* these are FIXED once they are in a release they STAY
 * well, unless im feeling grumpy ;) */
#define MODCALL_START  0
#define MODCALL_CLOSE  1
#define MODCALL_EXPMEM 2
#define MODCALL_REPORT 3
/* filesys */
#define FILESYS_REMOTE_REQ 4
#define FILESYS_ADDFILE    5
#define FILESYS_INCRGOTS   6
#define FILESYS_ISVALID	   7
/* share */
#define SHARE_FINISH       4
#define SHARE_DUMP_RESYNC  5
/* channels */
#define CHANNEL_CLEAR     15
/* server */
#define SERVER_BOTNAME       4
#define SERVER_BOTUSERHOST   5
/* irc */
#define IRC_RECHECK_CHANNEL 15
/* notes */
#define NOTES_CMD_NOTE       4
/* console */
#define CONSOLE_DOSTORE      4
 
#ifdef HPUX_HACKS
#include <dl.h>
#endif

typedef struct _module_entry {
  char *name;			/* name of the module (without .so) */
  int major;			/* major version number MUST match */
  int minor;			/* minor version number MUST be >= */
#ifndef STATIC
#ifdef HPUX_HACKS
  shl_t hand;
#else
  void *hand;			/* module handle */
#endif
#endif
  struct _module_entry *next;
  Function *funcs;
#ifdef DEBUG_MEM
  int mem_work;
#endif
} module_entry;

#endif				/* _EGG_MOD_MODVALS_H */
