/* 
 * tclhash.h
 * 
 * $Id: tclhash.h,v 1.4 1999/12/22 20:30:03 guppy Exp $
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

#ifndef _EGG_TCLHASH_H
#define _EGG_TCLHASH_H

#define HT_STACKABLE 1

typedef struct tct {
  struct flag_record flags;
  char *func_name;
  struct tct *next;
  int hits;
} tcl_cmd_t;

struct tcl_bind_mask {
  struct tcl_bind_mask *next;
  tcl_cmd_t *first;
  char *mask;
};

typedef struct tcl_bind_list {
  struct tcl_bind_list *next;
  struct tcl_bind_mask *first;
  char name[5];
  int flags;
  Function func;
} *p_tcl_bind_list;

#ifndef MAKING_MODS
void init_bind();
void kill_bind();
int expmem_tclhash();

p_tcl_bind_list add_bind_table(char *, int, Function);
void del_bind_table(p_tcl_bind_list);

p_tcl_bind_list find_bind_table(char *);

int check_tcl_bind(p_tcl_bind_list, char *, struct flag_record *, char *, int);
int check_tcl_dcc(char *, int, char *);
void check_tcl_chjn(char *, char *, int, char, int, char *);
void check_tcl_chpt(char *, char *, int, int);
void check_tcl_bot(char *, char *, char *);
void check_tcl_link(char *, char *);
void check_tcl_disc(char *);
char *check_tcl_filt(int, char *);
int check_tcl_note(char *, char *, char *);
void check_tcl_listen(char *, int);
void check_tcl_time(struct tm *);
void tell_binds(int, char *);
void check_tcl_nkch(char *, char *);
void check_tcl_away(char *, int, char *);
void check_tcl_chatactbcst(char *, int, char *, p_tcl_bind_list);
void check_tcl_event(char *);

#define check_tcl_chat(a,b,c) check_tcl_chatactbcst(a,b,c,H_chat)
#define check_tcl_act(a,b,c) check_tcl_chatactbcst(a,b,c,H_act)
#define check_tcl_bcst(a,b,c) check_tcl_chatactbcst(a,b,c,H_bcst)
void check_tcl_chonof(char *, int, p_tcl_bind_list);

#define check_tcl_chon(a,b) check_tcl_chonof(a,b,H_chon)
#define check_tcl_chof(a,b) check_tcl_chonof(a,b,H_chof)
void check_tcl_loadunld(char *, p_tcl_bind_list);

#define check_tcl_load(a) check_tcl_loadunld(a,H_load)
#define check_tcl_unld(a) check_tcl_loadunld(a,H_unld)

void rem_builtins(p_tcl_bind_list, cmd_t *);
void add_builtins(p_tcl_bind_list, cmd_t *);

int check_validity(char *, Function);
extern p_tcl_bind_list H_chat, H_act, H_bcst, H_chon, H_chof;
extern p_tcl_bind_list H_load, H_unld, H_dcc, H_bot, H_link;
extern p_tcl_bind_list H_away, H_nkch, H_filt, H_disc, H_event;

#endif

#define CHECKVALIDITY(a) if (!check_validity(argv[0],a)) { \
Tcl_AppendResult(irp, "bad builtin command call!", NULL); \
return TCL_ERROR; \
}

#endif				/* _EGG_TCLHASH_H */
