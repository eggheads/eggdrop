/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

#ifndef _H_TANDEM
#define _H_TANDEM

/* keep track of tandem-bots in the chain */
typedef struct tand_t_struct {
  char bot[HANDLEN + 1];
  struct tand_t_struct *via;
  struct tand_t_struct *uplink;
  struct tand_t_struct *next;
  int ver;
  char share;
} tand_t;

/* keep track of party-line members */
typedef struct {
  char nick[HANDLEN + 1];
  char bot[HANDLEN + 1];
  int sock;
  int chan;
  char *from;
  char flag;
  char status;
  time_t timer;			/* track idle time */
  char *away;
} party_t;

/* status: */
#define PLSTAT_AWAY   0x01
#define IS_PARTY      0x02

/* minimum version that uses tokens & base64 ints */
/* for channel msg's */
#define NEAT_BOTNET 1029900
#define GLOBAL_CHANS 100000

#ifndef MAKING_MODS
void send_tand_but(int, char *, int);
void botnet_send_chan(int, char *, char *, int, char *);
void botnet_send_chat(int, char *, char *);
void botnet_send_act(int, char *, char *, int, char *);
void botnet_send_ping(int);
void botnet_send_pong(int);
void botnet_send_priv VARARGS(int, arg1);
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

#endif

#define b_status(a) (dcc[a].status)
#define b_version(a) (dcc[a].u.bot->version)
#define b_linker(a) (dcc[a].u.bot->linker)
#define b_numver(a) (dcc[a].u.bot->numver)

#define safe_str(a) (a ? a : "")
#endif
