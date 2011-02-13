/*
 * irc.h -- part of irc.mod
 *
 * $Id: irc.h,v 1.36 2011/02/13 14:19:34 simple Exp $
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

#ifndef _EGG_MOD_IRC_IRC_H
#define _EGG_MOD_IRC_IRC_H

#define check_tcl_join(a,b,c,d) check_tcl_joinspltrejn(a,b,c,d,H_join)
#define check_tcl_splt(a,b,c,d) check_tcl_joinspltrejn(a,b,c,d,H_splt)
#define check_tcl_rejn(a,b,c,d) check_tcl_joinspltrejn(a,b,c,d,H_rejn)
#define check_tcl_sign(a,b,c,d,e) check_tcl_signtopcnick(a,b,c,d,e,H_sign)
#define check_tcl_topc(a,b,c,d,e) check_tcl_signtopcnick(a,b,c,d,e,H_topc)
#define check_tcl_nick(a,b,c,d,e) check_tcl_signtopcnick(a,b,c,d,e,H_nick)

#define REVENGE_KICK 1          /* Kicked victim        */
#define REVENGE_DEOP 2          /* Took op              */

#ifdef MAKING_IRC
static void check_tcl_need(char *, char *);
static void check_tcl_kick(char *, char *, struct userrec *, char *, char *, char *);
static void check_tcl_mode(char *, char *, struct userrec *, char *, char *, char *);
static void check_tcl_joinspltrejn(char *, char *, struct userrec *, char *,
                                   p_tcl_bind_list);
static void check_tcl_part(char *, char *, struct userrec *, char *, char *);
static void check_tcl_signtopcnick(char *, char *, struct userrec *u, char *,
                                   char *, p_tcl_bind_list);
static int check_tcl_pubm(char *, char *, char *, char *);
static int check_tcl_pub(char *, char *, char *, char *);
static int me_op(struct chanset_t *);
static int me_halfop(struct chanset_t *);
static int me_voice(struct chanset_t *);
static int any_ops(struct chanset_t *);
static int hand_on_chan(struct chanset_t *, struct userrec *);
static char *getchanmode(struct chanset_t *);
static void flush_mode(struct chanset_t *, int);
static void set_delay(struct chanset_t *, char *);
static void refresh_who_chan(char *);

/* reset(bans|exempts|invites) are now just macros that call resetmasks
 * in order to reduce the code duplication. <cybah>
 */
#define resetbans(chan)     resetmasks((chan), (chan)->channel.ban,          \
                                       (chan)->bans, global_bans, 'b')
#define resetexempts(chan)  resetmasks((chan), (chan)->channel.exempt,       \
                                       (chan)->exempts, global_exempts, 'e')
#define resetinvites(chan)  resetmasks((chan), (chan)->channel.invite,       \
                                       (chan)->invites, global_invites, 'I')

static void reset_chan_info(struct chanset_t *, int);
static void recheck_channel(struct chanset_t *, int);
static void set_key(struct chanset_t *, char *);
static void maybe_revenge(struct chanset_t *, char *, char *, int);
static int detect_chan_flood(char *, char *, char *, struct chanset_t *, int,
                             char *);
static void newmask(masklist *, char *, char *);
static char *quickban(struct chanset_t *, char *);
static void got_op(struct chanset_t *chan, char *nick, char *from, char *who,
                   struct userrec *opu, struct flag_record *opper);
static void got_halfop(struct chanset_t *chan, char *nick, char *from,
                       char *who, struct userrec *opu,
                       struct flag_record *opper);
static int killmember(struct chanset_t *chan, char *nick);
static void check_lonely_channel(struct chanset_t *chan);
static int gotmode(char *, char *);

#define newban(chan, mask, who)         newmask((chan)->channel.ban, mask, who)
#define newexempt(chan, mask, who)      newmask((chan)->channel.exempt, mask, \
                                                who)
#define newinvite(chan, mask, who)      newmask((chan)->channel.invite, mask, \
                                                who)
#else
/* 4 - 7 */
#define H_splt (*(p_tcl_bind_list*)(irc_funcs[4]))
#define H_rejn (*(p_tcl_bind_list*)(irc_funcs[5]))
#define H_nick (*(p_tcl_bind_list*)(irc_funcs[6]))
#define H_sign (*(p_tcl_bind_list*)(irc_funcs[7]))
/* 8 - 11 */
#define H_join (*(p_tcl_bind_list*)(irc_funcs[8]))
#define H_part (*(p_tcl_bind_list*)(irc_funcs[9]))
#define H_mode (*(p_tcl_bind_list*)(irc_funcs[10]))
#define H_kick (*(p_tcl_bind_list*)(irc_funcs[11]))
/* 12 - 15 */
#define H_pubm (*(p_tcl_bind_list*)(irc_funcs[12]))
#define H_pub (*(p_tcl_bind_list*)(irc_funcs[13]))
#define H_topc (*(p_tcl_bind_list*)(irc_funcs[14]))
/* recheck_channel is here */
/* 16 - 19 */
#define me_op ((int(*)(struct chanset_t *))irc_funcs[16])
/* recheck_channel_modes is here */
#define H_need (*(p_tcl_bind_list*)(irc_funcs[18]))
/* do_channel_part is here. */
/* 20 - 23 */
/* check_this_ban is here. */
/* check_this_user is here. */
#define me_halfop ((int(*)(struct chanset_t *))irc_funcs[22])
#define me_voice ((int(*)(struct chanset_t *))irc_funcs[23])
/* 24 - 27 */
#define getchanmode ((char *(*)(struct chanset_t *))irc_funcs[24])

#endif /* MAKING_IRC */

#endif /* _EGG_MOD_IRC_IRC_H */
