/* 
 * channels.h - part of channels.mod
 * 
 */
/* 
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

#ifdef MAKING_CHANNELS
static void del_chanrec(struct userrec *u, char *);
static struct chanuserrec *get_chanrec(struct userrec *u, char *chname);
static struct chanuserrec *add_chanrec(struct userrec *u, char *chname);
static void add_chanrec_by_handle(struct userrec *bu, char *hand, char *chname);
static void get_handle_chaninfo(char *handle, char *chname, char *s);
static void set_handle_chaninfo(struct userrec *bu, char *handle,
				char *chname, char *info);
static void set_handle_laston(char *chan, struct userrec *u, time_t n);
static int u_sticky_ban(struct banrec *u, char *uhost);
static int u_setsticky_ban(struct chanset_t *chan, char *uhost, int sticky);
static int u_equals_ban(struct banrec *u, char *uhost);
static int u_match_ban(struct banrec *u, char *uhost);
static int u_match_exempt (struct exemptrec * e, char * uhost);
static int u_sticky_exempt (struct exemptrec * u, char * uhost);
static int u_setsticky_exempt (struct chanset_t * chan, char * uhost, int sticky);
static int u_equals_exempt (struct exemptrec * u, char * uhost);
static int u_delexempt (struct chanset_t * c, char * who, int doit);
static int u_addexempt (struct chanset_t * chan, char * exempt, char * from,
 			char * note,  time_t expire_time, int flags);
static int u_match_invite (struct inviterec * e, char * uhost);
static int u_sticky_invite (struct inviterec * u, char * uhost);
static int u_setsticky_invite (struct chanset_t * chan, char * uhost, int sticky);
static int u_equals_invite (struct inviterec * u, char * uhost);
static int u_delinvite (struct chanset_t * c, char * who, int doit);
static int u_addinvite (struct chanset_t * chan, char * invite, char * from,
 			char * note,  time_t expire_time, int flags);
static int u_delban(struct chanset_t *c, char *who, int doit);
static int u_addban(struct chanset_t *chan, char *ban, char *from, char *note,
		    time_t expire_time, int flags);
static void tell_bans(int idx, int show_inact, char *match);
static int write_bans(FILE * f, int idx);
static void check_expired_bans(void);
static void tell_exempts (int idx, int show_inact, char * match);
static int write_exempts (FILE * f, int idx);
static void check_expired_exempts(void);
static void tell_invites (int idx, int show_inact, char * match);
static int write_invites (FILE * f, int idx);
static void check_expired_invites(void);
static void write_channels(void);
static void read_channels(int);
static int killchanset(struct chanset_t *);
static void clear_channel(struct chanset_t *, int);
static void get_mode_protect(struct chanset_t *chan, char *s);
static void set_mode_protect(struct chanset_t *chan, char *set);
static int isbanned(struct chanset_t *chan, char *);
static int isexempted(struct chanset_t *chan, char *);	/* Crotale */
static int isinvited(struct chanset_t *chan, char *);	/* arthur2 */
static int tcl_channel_modify(Tcl_Interp * irp, struct chanset_t *chan,
			      int items, char **item);
static int tcl_channel_add(Tcl_Interp * irp, char *, char *);
#else

/* 4 - 7 */
#define u_setsticky_ban ((int (*)(struct chanset_t *, char *,int))channels_funcs[4])
#define u_delban ((int (*)(struct chanset_t *, char *, int))channels_funcs[5])
#define u_addban ((int (*)(struct chanset_t *, char *, char *, char *, time_t, int))channels_funcs[6])
#define write_bans ((int (*)(FILE *, int))channels_funcs[7])
/* 8 - 11 */
#define get_chanrec ((struct chanuserrec *(*)(struct userrec *, char *))channels_funcs[8])
#define add_chanrec ((struct chanuserrec *(*)(struct userrec *, char *))channels_funcs[9])
#define del_chanrec ((void (*)(struct userrec *, char *))channels_funcs[10])
#define set_handle_chaninfo ((void (*)(struct userrec *, char *, char *, char *))channels_funcs[11])
/* 12 - 15 */
#define channel_malloc(x) ((void *(*)(int,char*,int))channels_funcs[12])(x,__FILE__,__LINE__)
#define u_match_ban ((int(*)(struct banrec *, char *))channels_funcs[13])
#define u_equals_ban ((int(*)(struct banrec *, char *))channels_funcs[14])
#define clear_channel ((void(*)(struct chanset_t *,int))channels_funcs[15])
/* 16 - 19 */
#define set_handle_laston ((void (*)(char *,struct userrec *,time_t))channels_funcs[16])
#define u_match_exempt ((int(*)(struct exemptrec *, char *))channels_funcs[27])
/* 28 - 31 */
#define u_setsticky_exempt ((int (*)(struct chanset_t *, char *,int))channels_funcs[28])
#define u_delexempt ((int (*)(struct chanset_t *, char *, int))channels_funcs[29])
#define u_addexempt ((int (*)(struct chanset_t *, char *, char *, char *, time_t, int))channels_funcs[30])
#define u_equals_exempt ((int(*)(struct exemptrec *, char *))channels_funcs[31])
/* 32 - 35 */
#define u_sticky_exempt ((int(*)(struct exemptrec *, char *))channels_funcs[32])
#define u_match_invite ((int(*)(struct inviterec *, char *))channels_funcs[33])
#define u_setsticky_invite ((int (*)(struct chanset_t *, char *,int))channels_funcs[34])
#define u_delinvite ((int (*)(struct chanset_t *, char *, int))channels_funcs[35])
/* 36 - 39 */
#define u_addinvite ((int (*)(struct chanset_t *, char *, char *, char *, time_t, int))channels_funcs[36])
#define u_equals_invite ((int(*)(struct inviterec *, char *))channels_funcs[37])
#define u_sticky_invite ((int(*)(struct inviterec *, char *))channels_funcs[38])
#define write_exempts ((int (*)(FILE *, int))channels_funcs[39])
/* 40 - 43 */
#define write_invites ((int (*)(FILE *, int))channels_funcs[40])
#define ban_time (*(int *)(channels_funcs[17]))
#define use_info (*(int *)(channels_funcs[18]))
#define get_handle_chaninfo ((void (*)(char *, char *, char *))channels_funcs[19])
/* 20 - 23 */
#define u_sticky_ban ((int(*)(struct banrec *, char *))channels_funcs[20])
#define isbanned ((int(*)(struct chanset_t *,char *))channels_funcs[21])
#define add_chanrec_by_handle ((void (*)(struct userrec *, char *, char *))channels_funcs[22])
#define isexempted ((int(*)(struct chanset_t *,char *))channels_funcs[23])	/* Crotale */
/* 24 - 27 */
#define exempt_time (*(int *)(channels_funcs[24]))
#define isinvited ((int(*)(struct chanset_t *,char *))channels_funcs[25])	/* arthur2 */
#define invite_time (*(int *)(channels_funcs[26]))
#endif
