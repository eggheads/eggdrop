/*
 * prototypes!  for every function used outside its own module
 * (i guess i'm not very modular, cuz there are a LOT of these.)
 * with full prototyping, some have been moved to other .h files
 * because they use structures in those (saves including those
 * .h files EVERY time) - Beldin
 */
/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

#ifndef _H_PROTO
#define _H_PROTO

#include "../lush.h"

#ifdef HAVE_DPRINTF
#define dprintf dprintf_eggdrop
#endif

#ifndef HAVE_BZERO
void bzero(char *, int);

#endif

struct chanset_t;		/* keeps the compiler warnings down :) */
struct userrec;
struct banrec;
struct igrec;
struct exemptrec;
struct inviterec;
struct flag_record;
struct list_type;
struct tand_t_struct;

#if !defined(MAKING_MODS)
extern void (*encrypt_pass) (char *, char *);
extern char *(*encrypt_string) (char *, char *);
extern char *(*decrypt_string) (char *, char *);

#endif
/* botcmd.c */
void bot_share(int, char *);
int base64_to_int(char *);

/* botnet.c */
void answer_local_whom(int, int);
char *lastbot(char *);
int nextbot(char *);
int in_chain(char *);
void tell_bots(int);
void tell_bottree(int, int);
int botlink(char *, int, char *);
int botunlink(int, char *, char *);
void dump_links(int);
void addbot(char *, char *, char *, char, int);
void updatebot(int, char *, char, int);
void rembot(char *);
struct tand_t_struct *findbot(char *);
void unvia(int, struct tand_t_struct *);
void check_botnet_pings();
int partysock(char *, char *);
int addparty(char *, char *, int, char, int, char *, int *);
void remparty(char *, int);
void partystat(char *, int, int, int);
int partynick(char *, int, char *);
int partyidle(char *, char *);
void partysetidle(char *, int, int);
void partyaway(char *, int, char *);
void zapfbot(int);
void tandem_relay(int, char *, int);
int getparty(char *, int);

/* botmsg.c */
int add_note(char *, char *, char *, int, int);
int simple_sprintf VARARGS(char *, arg1);
void tandout_but VARARGS(int, arg1);
char *int_to_base10(unsigned int);
char *int_to_base64(unsigned int);
extern int (*match_noterej) (struct userrec*, char *);

/* chanprog.c */
void tell_verbose_uptime(int);
void tell_verbose_status(int);
void tell_settings(int);
int logmodes(char *);
int isowner(char *);
char *masktype(int);
char *maskname(int);
void reaffirm_owners();
void rehash();
void reload();
void chanprog();
void check_timers();
void check_utimers();
void rmspace(char *);
void check_timers();
void set_chanlist(char *host, struct userrec *rec);
void clear_chanlist();

/* cmds.c */
int check_dcc_attrs(struct userrec *, int);
int check_dcc_chanattrs(struct userrec *, char *, int, int);
int stripmodes(char *);
char *stripmasktype(int);

/* dcc.c */
void failed_link(int);

/* dccutil.c */
void dprintf VARARGS(int, arg1);
void chatout VARARGS(char *, arg1);
extern void (*shareout) ();
extern void (*sharein) (int, char *);
void chanout_but VARARGS(int, arg1);
void dcc_chatter(int);
void lostdcc(int);
void makepass(char *);
void tell_dcc(int);
void not_away(int);
void set_away(int, char *);
void *_get_data_ptr(int, char *, int);

#define get_data_ptr(x) _get_data_ptr(x,__FILE__,__LINE__)
void flush_lines(int, struct chat_info *);
struct dcc_t *find_idx(int);
int new_dcc(struct dcc_table *, int);
void del_dcc(int);
char *add_cr(char *);

/* gotdcc.c */
void gotdcc(char *, char *, struct userrec *, char *);
void do_boot(int, char *, char *);
int detect_dcc_flood(time_t *, struct chat_info *, int);

/* language.c */
char *get_language(int);
int cmd_loadlanguage(struct userrec *, int, char *);
void add_lang_section(char *);

/* main.c */
void fatal(char *, int);
int expected_memory();
void backup_userfile();

/* match.c */
int _wild_match(register unsigned char *, register unsigned char *);

#define wild_match(a,b) _wild_match((unsigned char *)(a),(unsigned char *)(b))

/* mem.c */
void *n_malloc(int, char *, int);
void *n_realloc(void *, int, char *, int);
void n_free(void *, char *, int);
void tell_mem_status(char *);
void tell_mem_status_dcc(int);
void debug_mem_to_dcc(int);

/* misc.c */
int my_strcpy(char *, char *);
void putlog VARARGS(int, arg1);
void flushlogs();
void check_logsize();
void maskhost(char *, char *);
char *stristr(char *, char *);
void splitc(char *, char *, char);
char *newsplit(char **);
char *splitnick(char **);
void stridx(char *, char *, int);
void dumplots(int, char *, char *);
void daysago(time_t, time_t, char *);
void days(time_t, time_t, char *);
void daysdur(time_t, time_t, char *);
void help_subst(char *, char *, struct flag_record *, int, char *);
void sub_lang(int, char *);
void show_motd(int);
void tellhelp(int, char *, struct flag_record *, int);
void tellwildhelp(int, char *, struct flag_record *);
void tellallhelp(int, char *, struct flag_record *);
void showhelp(char *, char *, struct flag_record *, int);
int copyfile(char *, char *);
int movefile(char *, char *);
void rem_help_reference(char *file);
void add_help_reference(char *file);
void debug_help(int);
void reload_help_data(void);
void remove_gunk(char *);
char *extracthostname(char *);
void show_banner(int i);

/* net.c */
void my_memcpy(char *, char *, int);
IP my_atoul(char *);
unsigned long iptolong(IP);
IP getmyip();
void neterror(char *);
void setsock(int, int);
int getsock(int);
void killsock(int);
int answer(int, char *, unsigned long *, unsigned short *, int);
int open_listen(int *);
int open_telnet(char *, int);
int open_telnet_dcc(int, char *, char *);
int open_telnet_raw(int, char *, int);
void tputs(int, char *, unsigned int);
void dequeue_sockets();
int sockgets(char *, int *);
void tell_netdebug(int);
int sanitycheck_dcc(char *, char *, char *, char *);

/* tcl.c */
void protect_tcl();
void unprotect_tcl();
void do_tcl(char *, char *);
void set_tcl_vars();
int readtclprog(char *);
int findidx(int);
int findanyidx(int);

/* userent.c */
void list_type_kill(struct list_type *);
int list_type_expmem(struct list_type *);
int xtra_set();

/* userrec.c */
struct userrec *adduser(struct userrec *, char *, char *, char *, int);
void addhost_by_handle(char *, char *);
void clear_userlist(struct userrec *);
int u_pass_match(struct userrec *, char *);
int delhost_by_handle(char *, char *);
int ishost_for_handle(char *, char *);
int count_users(struct userrec *);
int deluser(char *);
void freeuser(struct userrec *);
int change_handle(struct userrec *, char *);
void correct_handle(char *);
int write_user(struct userrec *u, FILE * f, int shr);
void write_userfile(int);
struct userrec *check_dcclist_hand(char *);
void touch_laston(struct userrec *, char *, time_t);

/* users.c */
void addignore(char *, char *, char *, time_t);
int delignore(char *);
void tell_ignores(int, char *);
int match_ignore(char *);
void check_expired_ignores();
void autolink_cycle(char *);
void tell_file_stats(int, char *);
void tell_user_ident(int, char *, int);
void tell_users_match(int, char *, int, int, int, char *);
int readuserfile(char *, struct userrec **);

/* rfc1459.c */
int rfc_casecmp(char *, char *);
int rfc_ncasecmp(char *, char *, int);

#endif
