/*
 * flags.h
 *
 * $Id: flags.h,v 1.21 2011/02/13 14:19:33 simple Exp $
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

#ifndef _EGG_FLAGS_H
#define _EGG_FLAGS_H

struct flag_record {
  int match;
  int global;
  int udef_global;
  intptr_t bot;
  int chan;
  int udef_chan;
};

#define FR_GLOBAL 0x00000001
#define FR_BOT    0x00000002
#define FR_CHAN   0x00000004
#define FR_OR     0x40000000
#define FR_AND    0x20000000
#define FR_ANYWH  0x10000000
#define FR_ALL    0x0fffffff

/*
 * userflags:
 *   abcdefgh?jklmnopqr?tuvwxyz + user defined A-Z
 *   unused letters: is
 *
 * botflags:
 *   0123456789ab????ghi??l???p?rs???????
 *   unused letters: cdefjkmnoqtuvwxyz
 *
 * chanflags:
 *   a??defg???klmno?qr??uv??yz + user defined A-Z
 *   unused letters: bchijpstwx
 */
#define USER_VALID 0x03fbfeff   /* Sum of all USER_ flags */
#define CHAN_VALID 0x03777c79   /* Sum of all CHAN_ flags */
#define BOT_VALID  0x7fe689C1   /* Sum of all BOT_  flags */


#define USER_AUTOOP        0x00000001 /* a  auto-op                               */
#define USER_BOT           0x00000002 /* b  user is a bot                         */
#define USER_COMMON        0x00000004 /* c  user is actually a public site        */
#define USER_DEOP          0x00000008 /* d  user is global de-op                  */
#define USER_EXEMPT        0x00000010 /* e  exempted from stopnethack             */
#define USER_FRIEND        0x00000020 /* f  user is global friend                 */
#define USER_GVOICE        0x00000040 /* g  auto-voice                            */
#define USER_HIGHLITE      0x00000080 /* h  highlighting (bold)                   */
#define USER_I             0x00000100 /* i  unused                                */
#define USER_JANITOR       0x00000200 /* j  user is file area master              */
#define USER_KICK          0x00000400 /* k  user is global auto-kick              */
#define USER_HALFOP        0x00000800 /* l  user is +h on all channels            */
#define USER_MASTER        0x00001000 /* m  user has full bot access              */
#define USER_OWNER         0x00002000 /* n  user is the bot owner                 */
#define USER_OP            0x00004000 /* o  user is +o on all channels            */
#define USER_PARTY         0x00008000 /* p  user has party line access            */
#define USER_QUIET         0x00010000 /* q  user is global de-voice               */
#define USER_DEHALFOP      0x00020000 /* r  user is global de-halfop              */
#define USER_S             0x00040000 /* s  unused                                */
#define USER_BOTMAST       0x00080000 /* t  user is botnet master                 */
#define USER_UNSHARED      0x00100000 /* u  not shared with sharebots             */
#define USER_VOICE         0x00200000 /* v  user is +v on all channels            */
#define USER_WASOPTEST     0x00400000 /* w  wasop test needed for stopnethack     */
#define USER_XFER          0x00800000 /* x  user has file area access             */
#define USER_AUTOHALFOP    0x01000000 /* y  auto-halfop                           */
#define USER_WASHALFOPTEST 0x02000000 /* z  washalfop test needed for stopnethack */
#define USER_DEFAULT       0x40000000 /*    use default-flags                     */

/* Flags specifically for bots */
#define BOT_ALT        0x00000001 /* a  auto-link here if all hubs fail */
#define BOT_BOT        0x00000002 /* b  sanity bot flag                 */
#define BOT_C          0x00000004 /* c  unused                          */
#define BOT_D          0x00000008 /* d  unused                          */
#define BOT_E          0x00000010 /* e  unused                          */
#define BOT_F          0x00000020 /* f  unused                          */
#define BOT_GLOBAL     0x00000040 /* g  all channel are shared          */
#define BOT_HUB        0x00000080 /* h  auto-link to ONE of these bots  */
#define BOT_ISOLATE    0x00000100 /* i  isolate party line from botnet  */
#define BOT_J          0x00000200 /* j  unused                          */
#define BOT_K          0x00000400 /* k  unused                          */
#define BOT_LEAF       0x00000800 /* l  may not link other bots         */
#define BOT_M          0x00001000 /* m  unused                          */
#define BOT_N          0x00002000 /* n  unused                          */
#define BOT_O          0x00004000 /* o  unused                          */
#define BOT_PASSIVE    0x00008000 /* p  share passively with this bot   */
#define BOT_Q          0x00010000 /* q  unused                          */
#define BOT_REJECT     0x00020000 /* r  automatically reject anywhere   */
#define BOT_AGGRESSIVE 0x00040000 /* s  bot shares user files           */
#define BOT_T          0x00080000 /* t  unused                          */
#define BOT_U          0x00100000 /* u  unused                          */
#define BOT_V          0x00200000 /* v  unused                          */
#define BOT_W          0x00400000 /* w  unused                          */
#define BOT_X          0x00800000 /* x  unused                          */
#define BOT_Y          0x01000000 /* y  unused                          */
#define BOT_Z          0x02000000 /* z  unused                          */
#define BOT_FLAG0      0x00200000 /* 0  user-defined flag #0            */
#define BOT_FLAG1      0x00400000 /* 1  user-defined flag #1            */
#define BOT_FLAG2      0x00800000 /* 2  user-defined flag #2            */
#define BOT_FLAG3      0x01000000 /* 3  user-defined flag #3            */
#define BOT_FLAG4      0x02000000 /* 4  user-defined flag #4            */
#define BOT_FLAG5      0x04000000 /* 5  user-defined flag #5            */
#define BOT_FLAG6      0x08000000 /* 6  user-defined flag #6            */
#define BOT_FLAG7      0x10000000 /* 7  user-defined flag #7            */
#define BOT_FLAG8      0x20000000 /* 8  user-defined flag #8            */
#define BOT_FLAG9      0x40000000 /* 9  user-defined flag #9            */
#define BOT_SHARE      (BOT_AGGRESSIVE|BOT_PASSIVE)


/* Flag checking macros */
#define chan_op(x)              ((x).chan & USER_OP)
#define glob_op(x)              ((x).global & USER_OP)
#define chan_halfop(x)          ((x).chan & USER_HALFOP)
#define glob_halfop(x)          ((x).global & USER_HALFOP)
#define chan_deop(x)            ((x).chan & USER_DEOP)
#define glob_deop(x)            ((x).global & USER_DEOP)
#define chan_dehalfop(x)        ((x).chan & USER_DEHALFOP)
#define glob_dehalfop(x)        ((x).global & USER_DEHALFOP)
#define glob_master(x)          ((x).global & USER_MASTER)
#define glob_bot(x)             ((x).global & USER_BOT)
#define glob_owner(x)           ((x).global & USER_OWNER)
#define chan_master(x)          ((x).chan & USER_MASTER)
#define chan_owner(x)           ((x).chan & USER_OWNER)
#define chan_autoop(x)          ((x).chan & USER_AUTOOP)
#define glob_autoop(x)          ((x).global & USER_AUTOOP)
#define chan_autohalfop(x)      ((x).chan & USER_AUTOHALFOP)
#define glob_autohalfop(x)      ((x).global & USER_AUTOHALFOP)
#define chan_gvoice(x)          ((x).chan & USER_GVOICE)
#define glob_gvoice(x)          ((x).global & USER_GVOICE)
#define chan_kick(x)            ((x).chan & USER_KICK)
#define glob_kick(x)            ((x).global & USER_KICK)
#define chan_voice(x)           ((x).chan & USER_VOICE)
#define glob_voice(x)           ((x).global & USER_VOICE)
#define chan_wasoptest(x)       ((x).chan & USER_WASOPTEST)
#define glob_wasoptest(x)       ((x).global & USER_WASOPTEST)
#define chan_washalfoptest(x)   ((x).chan & USER_WASHALFOPTEST)
#define glob_washalfoptest(x)   ((x).global & USER_WASHALFOPTEST)
#define chan_quiet(x)           ((x).chan & USER_QUIET)
#define glob_quiet(x)           ((x).global & USER_QUIET)
#define chan_friend(x)          ((x).chan & USER_FRIEND)
#define glob_friend(x)          ((x).global & USER_FRIEND)
#define glob_botmast(x)         ((x).global & USER_BOTMAST)
#define glob_party(x)           ((x).global & USER_PARTY)
#define glob_xfer(x)            ((x).global & USER_XFER)
#define glob_hilite(x)          ((x).global & USER_HIGHLITE)
#define chan_exempt(x)          ((x).chan & USER_EXEMPT)
#define glob_exempt(x)          ((x).global & USER_EXEMPT)

#define bot_global(x)           ((x).bot & BOT_GLOBAL)
#define bot_chan(x)             ((x).chan & BOT_AGGRESSIVE)
#define bot_shared(x)           ((x).bot & BOT_SHARE)


#ifndef MAKING_MODS

void get_user_flagrec(struct userrec *, struct flag_record *, const char *);
void set_user_flagrec(struct userrec *, struct flag_record *, const char *);
void break_down_flags(const char *, struct flag_record *, struct flag_record *);
int build_flags(char *, struct flag_record *, struct flag_record *);
int flagrec_eq(struct flag_record *, struct flag_record *);
int flagrec_ok(struct flag_record *, struct flag_record *);
int sanity_check(int);
int chan_sanity_check(int, int);
char geticon(int);

#endif /* MAKING_MODS */

#endif /* _EGG_FLAGS_H */
