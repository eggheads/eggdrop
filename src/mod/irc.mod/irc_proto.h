/*
 * irc_proto.h -- part of irc.mod
 *   shared declarations between translation units of irc.mod
 */

#ifndef _EGG_MOD_IRC_IRC_PROTO_H
#define _EGG_MOD_IRC_IRC_PROTO_H

/* Module function tables from irc.c */
extern Function *global;
extern Function *channels_funcs;
extern Function *server_funcs;

/* Variables from irc.c */
extern int modesperline;
extern int mode_buf_len;
extern int max_bans;
extern int max_exempts;
extern int max_invites;
extern int max_modes;
extern int bounce_bans;
extern int bounce_exempts;
extern int bounce_invites;
extern int bounce_modes;
extern int prevent_mixing;
extern int include_lk;
extern int kick_method;
extern int use_354;
extern int learn_users;
extern int wait_info;
extern int invite_key;
extern int no_chanrec_info;
extern int wait_split;
extern int keepnick;
extern int twitch;
extern int ctcp_mode;
extern int rfc_compliant;
extern char opchars[8];
extern Tcl_Obj *tcl_account;
extern p_tcl_bind_list H_topc, H_splt, H_sign, H_rejn, H_part, H_pub, H_pubm;
extern p_tcl_bind_list H_nick, H_mode, H_kick, H_join, H_need, H_invt, H_ircaway;
extern p_tcl_bind_list H_account, H_chghost;

/* Functions from irc.c */
void check_tcl_mode(char *, char *, struct userrec *, char *, char *, char *);
void check_tcl_need(char *, char *);
void check_tcl_kick(char *, char *, struct userrec *, char *, char *, char *);
void check_tcl_invite(char *, char *, char *, char *);
int check_tcl_pub(char *, char *, char *, char *);
int check_tcl_pubm(char *, char *, char *, char *);
void check_tcl_joinspltrejn(char *, char *, struct userrec *, char *,
                            p_tcl_bind_list);
void check_tcl_part(char *, char *, struct userrec *, char *, char *);
void check_tcl_signtopcnick(char *, char *, struct userrec *, char *,
                            char *, p_tcl_bind_list);
int check_tcl_ircaway(char *, char *, char *, struct userrec *, char *, char *);
void check_tcl_account(char *, char *, struct userrec *, char *, char *);
int check_tcl_chghost(char *, char *, char *, struct userrec *, char *, char *,
                      char *);
void maybe_revenge(struct chanset_t *, char *, char *, int);
void set_key(struct chanset_t *, char *);
int me_op(struct chanset_t *);
int me_halfop(struct chanset_t *);
int me_voice(struct chanset_t *);
int any_ops(struct chanset_t *);
int hand_on_chan(struct chanset_t *, struct userrec *);
void refresh_who_chan(char *);
void newmask(masklist *, char *, char *);
void check_lonely_channel(struct chanset_t *);
int killmember(struct chanset_t *, char *);
void do_channel_part(struct chanset_t *);

/* Functions from chan.c */
char *getchanmode(struct chanset_t *);
void check_exemptlist(struct chanset_t *, char *);
void do_mask(struct chanset_t *, masklist *, char *, char);
int detect_chan_flood(char *, char *, char *, struct chanset_t *, int, char *);
char *quickban(struct chanset_t *, char *);
void kick_all(struct chanset_t *, char *, char *, int);
void refresh_exempt(struct chanset_t *, char *);
void recheck_channel(struct chanset_t *, int);
void recheck_channel_modes(struct chanset_t *);
void check_this_ban(struct chanset_t *, char *, int);
void check_this_user(char *, int, char *);
void set_delay(struct chanset_t *, char *);
void resetmasks(struct chanset_t *, masklist *, maskrec *, maskrec *, char);
extern cmd_t irc_raw[];
extern cmd_t irc_rawt[];
extern cmd_t irc_isupport_binds[];

/* Functions from mode.c */
void flush_mode(struct chanset_t *, int);
void real_add_mode(struct chanset_t *, char, char, char *);
int gotmode(char *, char *);

/* Functions/tables from cmdsirc.c */
extern cmd_t irc_dcc[];

/* Functions/tables from msgcmds.c */
extern cmd_t C_msg[];

/* Functions/tables from tclirc.c */
extern tcl_cmds tclchan_cmds[];

#endif /* _EGG_MOD_IRC_IRC_PROTO_H */
