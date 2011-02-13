/*
 * tandem.h
 *
 * $Id: tandem.h,v 1.19 2011/02/13 14:19:33 simple Exp $
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

#ifndef _EGG_TANDEM_H
#define _EGG_TANDEM_H

/* Keep track of tandem-bots in the chain */
typedef struct tand_t_struct {
  char bot[HANDLEN + 1];
  struct tand_t_struct *via;
  struct tand_t_struct *uplink;
  struct tand_t_struct *next;
  int ver;
  char share;
} tand_t;

/* Keep track of party-line members */
typedef struct {
  char nick[HANDLEN + 1];
  char bot[HANDLEN + 1];
  int sock;
  int chan;
  char *from;
  char flag;
  char status;
  time_t timer;                 /* Track idle time */
  char *away;
} party_t;

/* Status: */
#define PLSTAT_AWAY   0x001
#define IS_PARTY      0x002

/* Minimum version that uses tokens & base64 ints
 * for channel msg's
 */
#define NEAT_BOTNET 1029900
#define GLOBAL_CHANS 100000


#ifndef MAKING_MODS

void send_tand_but(int, char *, int);
void botnet_send_chan(int, char *, char *, int, char *);
void botnet_send_chat(int, char *, char *);
void botnet_send_act(int, char *, char *, int, char *);
void botnet_send_ping(int);
void botnet_send_pong(int);
void botnet_send_priv EGG_VARARGS(int, arg1);
void botnet_send_who(int, char *, char *, int);
void botnet_send_infoq(int, char *);
void botnet_send_unlinked(int, char *, char *);
void botnet_send_traced(int, char *, char *);
void botnet_send_trace(int, char *, char *, char *);
void botnet_send_unlink(int, char *, char *, char *, char *);
void botnet_send_link(int, char *, char *, char *);
void botnet_send_update(int, tand_t *);
void botnet_send_nlinked(int, char *, char *, char, int);
void botnet_send_reject(int, char *, char *, char *, char *, char *);
void botnet_send_zapf(int, char *, char *, char *);
void botnet_send_zapf_broad(int, char *, char *, char *);
void botnet_send_motd(int, char *, char *);
void botnet_send_filereq(int, char *, char *, char *);
void botnet_send_filereject(int, char *, char *, char *);
void botnet_send_filesend(int, char *, char *, char *);
void botnet_send_away(int, char *, int, char *, int);
void botnet_send_idle(int, char *, int, int, char *);
void botnet_send_join_idx(int, int);
void botnet_send_join_party(int, int, int, int);
void botnet_send_part_idx(int, char *);
void botnet_send_part_party(int, int, char *, int);
void botnet_send_bye();
void botnet_send_nkch_part(int, int, char *);
void botnet_send_nkch(int, char *);
int bots_in_subtree(tand_t *);
int users_in_subtree(tand_t *);

#endif /* MAKING_MODS */


#define b_status(a)  (dcc[a].status)
#define b_version(a) (dcc[a].u.bot->version)
#define b_linker(a)  (dcc[a].u.bot->linker)
#define b_numver(a)  (dcc[a].u.bot->numver)

#endif /* _EGG_TANDEM_H */
