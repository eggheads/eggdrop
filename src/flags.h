/*
 * flags.h
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2021 Eggheads Development Team
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
 *   abcde?ghij?l?n?p?rs?u0123456789
 *   unused letters: fkmoqt
 *   unusable letters: vwxyz
 *
 * chanflags:
 *   a??defg???klmno?qrs??vw?yz + user defined A-Z
 *   unused letters: bchijptux (s from bot flags)
 */
#define USER_VALID 0x003fbfeff   /* Sum of all valid USER_ flags */
#define CHAN_VALID 0x003677c79   /* Sum of all valid CHAN_ flags */
#define BOT_VALID  0x07ff6abdf   /* Sum of all valid BOT_  flags */


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
#define BOT_SHBAN      0x00000002 /* b  can share bans                  */
#define BOT_SHCHAN     0x00000004 /* c  can share channel changes       */
#define BOT_SHLIMITED  0x00000008 /* d  shares userfiles but can't share anything */
#define BOT_SHEXEMPT   0x00000010 /* e  can share exempts               */
#define BOT_F          0x00000020 /* f  unused                          */
#define BOT_GLOBAL     0x00000040 /* g  all channel are shared          */
#define BOT_HUB        0x00000080 /* h  auto-link to ONE of these bots  */
#define BOT_ISOLATE    0x00000100 /* i  isolate party line from botnet  */
#define BOT_SHINV      0x00000200 /* j  can share invites               */
#define BOT_K          0x00000400 /* k  unused                          */
#define BOT_LEAF       0x00000800 /* l  may not link other bots         */
#define BOT_M          0x00001000 /* m  unused                          */
#define BOT_SHIGN      0x00002000 /* n  can share ignores               */
#define BOT_O          0x00004000 /* o  unused                          */
#define BOT_PASSIVE    0x00008000 /* p  share passively with this bot   */
#define BOT_Q          0x00010000 /* q  unused                          */
#define BOT_REJECT     0x00020000 /* r  automatically reject anywhere   */
#define BOT_AGGRESSIVE 0x00040000 /* s  shares userfiles and can share all */
#define BOT_T          0x00080000 /* t  unused                          */
#define BOT_SHUSER     0x00100000 /* u  can share user changes          */
/* BOT_V to BOT_Z not usable as they're bitflags 32-36 */
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
/* BOT_S still shares everything for backward compat, but if removed to share more precisely,
   we need those detailed flags to still indicate the bot shares */
#define BOT_SHPERMS    (BOT_SHBAN|BOT_SHCHAN|BOT_SHEXEMPT|BOT_SHIGN|BOT_SHINV|BOT_SHUSER|BOT_SHLIMITED)
#define BOT_SHARE      (BOT_AGGRESSIVE|BOT_PASSIVE|BOT_SHPERMS)

/* When adding more here, also add messages in cmds.c attr_inform */
/* Bot flag checking message ID's */
#define BOT_SANE_ALTOWNSHUB		0x00000001
#define BOT_SANE_HUBOWNSALT		0x00000002
#define BOT_SANE_OWNSALTHUB		0x00000004
#define BOT_SANE_SHPOWNSAGGR		0x00000008
#define BOT_SANE_AGGROWNSSHP		0x00000010
#define BOT_SANE_OWNSSHPAGGR		0x00000020
#define BOT_SANE_SHPOWNSPASS		0x00000040
#define BOT_SANE_PASSOWNSSHP		0x00000080
#define BOT_SANE_OWNSSHPPASS		0x00000100
#define BOT_SANE_SHAREOWNSREJ		0x00000200
#define BOT_SANE_REJOWNSSHARE		0x00000400
#define BOT_SANE_OWNSSHAREREJ		0x00000800
#define BOT_SANE_HUBOWNSREJ		0x00001000
#define BOT_SANE_REJOWNSHUB		0x00002000
#define BOT_SANE_OWNSHUBREJ		0x00004000
#define BOT_SANE_ALTOWNSREJ		0x00008000
#define BOT_SANE_REJOWNSALT		0x00010000
#define BOT_SANE_OWNSALTREJ		0x00020000
#define BOT_SANE_AGGROWNSPASS		0x00040000
#define BOT_SANE_PASSOWNSAGGR		0x00080000
#define BOT_SANE_OWNSAGGRPASS		0x00100000
#define BOT_SANE_NOSHAREOWNSGLOB	0x00200000
#define BOT_SANE_OWNSGLOB		0x00400000
/* User/Chan flag checking message ID's */
#define UC_SANE_DEOPOWNSOP		0x00000001
#define UC_SANE_OPOWNSDEOP		0x00000002
#define UC_SANE_OWNSDEOPOP		0x00000004
#define UC_SANE_DEHALFOPOWNSHALFOP	0x00000008
#define UC_SANE_HALFOPOWNSDEHALFOP	0x00000010
#define UC_SANE_OWNSDEHALFOPHALFOP	0x00000020
#define UC_SANE_DEOPOWNSAUTOOP		0x00000040
#define UC_SANE_AUTOOPOWNSDEOP		0x00000080
#define UC_SANE_OWNSDEOPAUTOOP		0x00000100
#define UC_SANE_DEHALFOPOWNSAHALFOP	0x00000200
#define UC_SANE_AHALFOPOWNSDEHALFOP	0x00000400
#define UC_SANE_OWNSDEHALFOPAHALFOP	0x00000800
#define UC_SANE_QUIETOWNSVOICE		0x00001000
#define UC_SANE_VOICEOWNSQUIET		0x00002000
#define UC_SANE_OWNSQUIETVOICE		0x00004000
#define UC_SANE_QUIETOWNSGVOICE		0x00008000
#define UC_SANE_GVOICEOWNSQUIET		0x00010000
#define UC_SANE_OWNSQUIETGVOICE		0x00020000
#define UC_SANE_OWNERADDSMASTER		0x00040000
#define UC_SANE_MASTERADDSOP		0x00080000
#define UC_SANE_OPADDSHALFOP		0x00100000
#define UC_SANE_NOBOTOWNSAGGR		0x00200000
#define UC_SANE_BOTOWNSPARTY		0x00400000
#define UC_SANE_BOTOWNSMASTER		0x00800000
#define UC_SANE_BOTOWNSCOMMON		0x01000000
#define UC_SANE_BOTOWNSOWNER		0x02000000
#define UC_SANE_MASTERADDSBOTMOPJAN	0x04000000
#define UC_SANE_BOTMASTADDSPARTY	0x08000000
#define UC_SANE_JANADDSXFER		0x10000000
#define UC_SANE_AUTOOPADDSOP		0x20000000
#define UC_SANE_AUTOHALFOPADDSHALFOP	0x40000000
#define UC_SANE_GVOICEADDSVOICE		0x80000000

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
int user_sanity_check(int * const, int const, int const);
int bot_sanity_check(intptr_t * const, intptr_t const, intptr_t const);
int chan_sanity_check(int * const, int const, int const, int const);
char geticon(int);

#endif /* MAKING_MODS */

#endif /* _EGG_FLAGS_H */
