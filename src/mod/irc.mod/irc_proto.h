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

/* Functions from irc.c */
void check_tcl_mode(char *, char *, struct userrec *, char *, char *, char *);
void check_tcl_need(char *, char *);
void maybe_revenge(struct chanset_t *, char *, char *, int);
void set_key(struct chanset_t *, char *);
int me_op(struct chanset_t *);
int me_halfop(struct chanset_t *);
void refresh_who_chan(char *);
void newmask(masklist *, char *, char *);

/* Functions from chan.c (compiled as part of irc.c) */
int detect_chan_flood(char *, char *, char *, struct chanset_t *, int, char *);
void kick_all(struct chanset_t *, char *, char *, int);
void refresh_exempt(struct chanset_t *, char *);
void recheck_channel(struct chanset_t *, int);

/* Functions from mode.c */
void flush_mode(struct chanset_t *, int);
void real_add_mode(struct chanset_t *, char, char, char *);
int gotmode(char *, char *);

#endif /* _EGG_MOD_IRC_IRC_PROTO_H */
