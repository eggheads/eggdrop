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
static int u_sticky_mask(maskrec *u, char *uhost);
static int u_setsticky_mask(struct chanset_t *chan, maskrec *m, char *uhost, int sticky, char *botcmd);

static int u_equals_mask(maskrec *u, char *uhost);
static int u_match_mask(struct maskrec *rec, char *mask);
static int u_delexempt (struct chanset_t * c, char * who, int doit);
static int u_addexempt (struct chanset_t * chan, char * exempt, char * from,
 			char * note,  time_t expire_time, int flags);
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
static int ismasked(masklist *m, char *user);
static int ismodeline(masklist *m, char *user);
static int tcl_channel_modify(Tcl_Interp * irp, struct chanset_t *chan,
			      int items, char **item);
static int tcl_channel_add(Tcl_Interp * irp, char *, char *);
static char *convert_element(char *src, char *dst);
#else

/* 4 - 7 */
#define u_setsticky_mask ((int (*)(struct chanset_t *, maskrec *, char *, int, char *))channels_funcs[4])
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
#define u_match_mask ((int(*)(maskrec *, char *))channels_funcs[13])
#define u_equals_mask ((int(*)(maskrec *, char *))channels_funcs[14])
#define clear_channel ((void(*)(struct chanset_t *,int))channels_funcs[15])
/* 16 - 19 */
#define set_handle_laston ((void (*)(char *,struct userrec *,time_t))channels_funcs[16])
#define ban_time (*(int *)(channels_funcs[17]))
#define use_info (*(int *)(channels_funcs[18]))
#define get_handle_chaninfo ((void (*)(char *, char *, char *))channels_funcs[19])
/* 20 - 23 */
#define u_sticky_mask ((int(*)(maskrec *, char *))channels_funcs[20])
#define ismasked ((int(*)(masklist *, char *))channels_funcs[21])
#define add_chanrec_by_handle ((void (*)(struct userrec *, char *, char *))channels_funcs[22])
/* *HOLE* channels_funcs[23] used to be isexempted() <cybah> */
/* 24 - 27 */
#define exempt_time (*(int *)(channels_funcs[24]))
/* *HOLE* channels_funcs[25] used to be isinvited() by arthur2 <cybah> */
#define invite_time (*(int *)(channels_funcs[26]))
/* *HOLE* channels_funcs[27] used to be u_match_exempt() by arthur2 <cybah> */
/* 28 - 31 */
/* *HOLE* channels_funcs[28] used to be u_setsticky_exempt() <cybah> */
#define u_delexempt ((int (*)(struct chanset_t *, char *, int))channels_funcs[29])
#define u_addexempt ((int (*)(struct chanset_t *, char *, char *, char *, time_t, int))channels_funcs[30])
/* *HOLE* channels_funcs[31] used to be u_equals_exempt() <cybah> */
/* 32 - 35 */
/* *HOLE* channels_funcs[32] used to be u_sticky_exempt()
 * *HOLE* channels_funcs[33] used to be u_match_invite()
 * *HOLE* channels_funcs[34] used to be u_setsticky_invite() <cybah>
 */
#define u_delinvite ((int (*)(struct chanset_t *, char *, int))channels_funcs[35])
/* 36 - 39 */
#define u_addinvite ((int (*)(struct chanset_t *, char *, char *, char *, time_t, int))channels_funcs[36])
/* *HOLE* channels_funcs[37] used to be u_equals_invite()
 * *HOLE* channels_funcs[38] used to be u_sticky_invite() <cybah>
 */
#define write_exempts ((int (*)(FILE *, int))channels_funcs[39])
/* 40 - 43 */
#define write_invites ((int (*)(FILE *, int))channels_funcs[40])
#define ismodeline ((int(*)(masklist *, char *))channels_funcs[41])

#endif

/*        Macro's here because their functions were replaced by somthing more
 *      generic. <cybah>
 */
#define isbanned(chan, user)    ismasked((chan)->channel.ban, user)
#define isexempted(chan, user)  ismasked((chan)->channel.exempt, user)
#define isinvited(chan, user)   ismasked((chan)->channel.invite, user)

#define ischanban(chan, user)    ismodeline((chan)->channel.ban, user)
#define ischanexempt(chan, user) ismodeline((chan)->channel.exempt, user)
#define ischaninvite(chan, user) ismodeline((chan)->channel.invite, user)

#define u_setsticky_ban(chan, host, sticky)     u_setsticky_mask(chan, ((struct chanset_t *)chan) ? ((struct chanset_t *)chan)->bans : global_bans, host, sticky, "s")
#define u_setsticky_exempt(chan, host, sticky)  u_setsticky_mask(chan, ((struct chanset_t *)chan) ? ((struct chanset_t *)chan)->exempts : global_exempts, host, sticky, "se")
#define u_setsticky_invite(chan, host, sticky)  u_setsticky_mask(chan, ((struct chanset_t *)chan) ? ((struct chanset_t *)chan)->invites : global_invites, host, sticky, "sInv")
