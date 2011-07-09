/*
 * proto.h
 *   prototypes for every function used outside its own module
 *
 * (i guess i'm not very modular, cuz there are a LOT of these.)
 * with full prototyping, some have been moved to other .h files
 * because they use structures in those
 * (saves including those .h files EVERY time) - Beldin
 *
 * $Id: proto.h,v 1.84 2011/07/09 15:07:48 thommey Exp $
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

#ifndef _EGG_PROTO_H
#define _EGG_PROTO_H

#include "lush.h"
#include "misc_file.h"

#define dprintf dprintf_eggdrop

struct chanset_t;               /* keeps the compiler warnings down :) */
struct userrec;
struct maskrec;
struct igrec;
struct flag_record;
struct list_type;
struct tand_t_struct;

#ifndef MAKING_MODS
extern void (*encrypt_pass) (char *, char *);
extern char *(*encrypt_string) (char *, char *);
extern char *(*decrypt_string) (char *, char *);
extern int (*rfc_casecmp) (const char *, const char *);
extern int (*rfc_ncasecmp) (const char *, const char *, int);
extern int (*rfc_toupper) (int);
extern int (*rfc_tolower) (int);
extern int (*match_noterej) (struct userrec *, char *);
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
int botunlink(int, char *, char *, char *);
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
int simple_sprintf EGG_VARARGS(char *, arg1);
void tandout_but EGG_VARARGS(int, arg1);
char *int_to_base10(int);
char *unsigned_int_to_base10(unsigned int);
char *int_to_base64(unsigned int);

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
void rmspace(char *s);
void check_timers();
void set_chanlist(const char *host, struct userrec *rec);
void clear_chanlist(void);
void clear_chanlist_member(const char *nick);

/* cmds.c */
int check_dcc_attrs(struct userrec *, int);
int check_dcc_chanattrs(struct userrec *, char *, int, int);
int stripmodes(char *);
char *stripmasktype(int);

/* dcc.c */
void failed_link(int);
void strip_mirc_codes(int, char *);
int check_ansi(char *);
void dupwait_notify(char *);

/* dccutil.c */
int increase_socks_max();
int findidx(int);
int findanyidx(int);
char *add_cr(char *);
void dprintf EGG_VARARGS(int, arg1);
void chatout EGG_VARARGS(char *, arg1);
extern void (*shareout) ();
extern void (*sharein) (int, char *);
void chanout_but EGG_VARARGS(int, arg1);
void dcc_chatter(int);
void lostdcc(int);
void killtransfer(int);
void removedcc(int);
void makepass(char *);
void tell_dcc(int);
void not_away(int);
void set_away(int, char *);
void *_get_data_ptr(int, char *, int);
void dcc_remove_lost(void);
void do_boot(int, char *, char *);
int detect_dcc_flood(time_t *, struct chat_info *, int);

#define get_data_ptr(x) _get_data_ptr(x,__FILE__,__LINE__)
void flush_lines(int, struct chat_info *);
struct dcc_t *find_idx(int);
int new_dcc(struct dcc_table *, int);
void del_dcc(int);
void changeover_dcc(int, struct dcc_table *, int);

/* dns.c */
extern void (*dns_hostbyip) (IP);
extern void (*dns_ipbyhost) (char *);
void block_dns_hostbyip(IP);
void block_dns_ipbyhost(char *);
void call_hostbyip(IP, char *, int);
void call_ipbyhost(char *, IP, int);
void dcc_dnshostbyip(IP);
void dcc_dnsipbyhost(char *);

/* language.c */
char *get_language(int);
int cmd_loadlanguage(struct userrec *, int, char *);
void add_lang_section(char *);
int del_lang_section(char *);
int exist_lang_section(char *);

/* main.c */
void fatal(const char *, int);
int expected_memory(void);
void eggContext(const char *, int, const char *);
void eggContextNote(const char *, int, const char *, const char *);
void eggAssert(const char *, int, const char *);
void backup_userfile(void);
int mainloop(int);

/* match.c */
int casecharcmp(unsigned char, unsigned char);
int charcmp(unsigned char, unsigned char);
int _wild_match(register unsigned char *, register unsigned char *);
int _wild_match_per(register unsigned char *, register unsigned char *,
                    int (*)(unsigned char, unsigned char),
                    int (*)(unsigned char, unsigned char),
                    unsigned char *);
int addr_match(char *, char *, int, int);
int mask_match(char *, char *);
int cidr_match(char *, char *, int);
int cron_match(const char *, const char *);

#define wild_match(a,b) _wild_match((unsigned char *)(a),(unsigned char *)(b))
#define wild_match_per(a,b) _wild_match_per((unsigned char *)(a),              \
                            (unsigned char *)(b),casecharcmp,NULL,NULL)
#define wild_match_partial_case(a,b) _wild_match_per((unsigned char *)(a),     \
                            (unsigned char *)(b),casecharcmp,charcmp,          \
                            (unsigned char *)strchr((b),' '))
#define match_addr(a,b) addr_match((char *)(a),(char *)(b),0,0)
#define match_useraddr(a,b) addr_match((char *)(a),(char *)(b),1,0)
#define cmp_masks(a,b) addr_match((char *)(a),(char *)(b),0,1)
#define cmp_usermasks(a,b) addr_match((char *)(a),(char *)(b),1,1)

/* mem.c */
char *n_strdup(const char *, const char *, int);
void *n_malloc(int, const char *, int);
void *n_realloc(void *, int, const char *, int);
void n_free(void *, const char *, int);
void tell_mem_status(char *);
void tell_mem_status_dcc(int);
void debug_mem_to_dcc(int);

/* misc.c */
int egg_strcatn(char *, const char *, size_t);
int my_strcpy(char *, char *);
void putlog EGG_VARARGS(int, arg1);
void flushlogs();
void check_logsize();
void splitc(char *, char *, char);
void splitcn(char *, char *, char, size_t);
void remove_crlf(char **);
char *newsplit(char **);
char *splitnick(char **);
void stridx(char *, char *, int);
void dumplots(int, const char *, const char *);
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
void rem_help_reference(char *);
void add_help_reference(char *);
void debug_help(int);
void reload_help_data(void);
char *extracthostname(char *);
void show_banner(int i);
void make_rand_str(char *, int);
int oatoi(const char *);
int is_file(const char *);
void logsuffix_change(char *);
char *str_escape(const char *, const char, const char);
char *strchr_unescape(char *, const char, register const char);
void str_unescape(char *, register const char);
int str_isdigit(const char *);
void kill_bot(char *, char *);

void maskaddr(const char *, char *, int);
#define maskhost(a,b) maskaddr((a),(b),3)
#define maskban(a,b)  maskaddr((a),(b),3)

/* net.c */
IP my_atoul(char *);
unsigned long iptolong(IP);
IP getmyip();
void neterror(char *);
void setsock(int, int);
int allocsock(int, int);
int alloctclsock(int, int, Tcl_FileProc *, ClientData);
int getsock(int);
void killsock(int);
void killtclsock(int);
int answer(int, char *, unsigned long *, unsigned short *, int);
inline int open_listen(int *);
int open_address_listen(IP addr, int *);
int open_telnet(char *, int);
int open_telnet_dcc(int, char *, char *);
int open_telnet_raw(int, char *, int);
void tputs(int, char *, unsigned int);
void dequeue_sockets();
int preparefdset(fd_set *, sock_list *, int, int, int);
int sockread(char *, int *, sock_list *, int, int);
int sockgets(char *, int *);
void tell_netdebug(int);
int sanitycheck_dcc(char *, char *, char *, char *);
int hostsanitycheck_dcc(char *, char *, IP, char *, char *);
char *iptostr(IP);
int sock_has_data(int, int);
int sockoptions(int sock, int operation, int sock_options);
int flush_inbuf(int idx);

/* tcl.c */
struct threaddata *threaddata();
int init_threaddata(int);
void protect_tcl();
void unprotect_tcl();
void do_tcl(char *, char *);
int readtclprog(char *fname);

/* userent.c */
void list_type_kill(struct list_type *);
int list_type_expmem(struct list_type *);
int xtra_set();

/* userrec.c */
struct userrec *adduser(struct userrec *, char *, char *, char *, int);
void addhost_by_handle(char *, char *);
void clear_masks(struct maskrec *);
void clear_userlist(struct userrec *);
int u_pass_match(struct userrec *, char *);
int delhost_by_handle(char *, char *);
int ishost_for_handle(char *, char *);
int count_users(struct userrec *);
int deluser(char *);
void freeuser(struct userrec *);
int change_handle(struct userrec *, char *);
void correct_handle(char *);
int write_user(struct userrec *, FILE *, int);
int write_ignores(FILE *f, int);
void write_userfile(int);
struct userrec *check_dcclist_hand(char *);
void touch_laston(struct userrec *, char *, time_t);
void user_del_chan(char *);
char *fixfrom(char *);
int check_conflags(struct flag_record *fr, int md);

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
int _rfc_casecmp(const char *, const char *);
int _rfc_ncasecmp(const char *, const char *, int);
int _rfc_toupper(int);
int _rfc_tolower(int);

#endif /* _EGG_PROTO_H */
