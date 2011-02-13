/*
 * lang.h
 *   Conversion definitions for language support
 *
 * $Id: lang.h,v 1.40 2011/02/13 14:19:33 simple Exp $
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

#ifndef _EGG_LANG_H
#define _EGG_LANG_H

#define MISC_USAGE              get_language(0x001)
#define MISC_FAILED             get_language(0x002)

/* Userfile messages */
#define USERF_XFERDONE          get_language(0x400)
/* was: USERF_BADREREAD         0x401            */
#define USERF_CANTREAD          get_language(0x402)
#define USERF_CANTSEND          get_language(0x403)
#define USERF_NOMATCH           get_language(0x404)
#define USERF_OLDFMT            get_language(0x405)
#define USERF_INVALID           get_language(0x406)
#define USERF_CORRUPT           get_language(0x407)
#define USERF_DUPE              get_language(0x408)
#define USERF_BROKEPASS         get_language(0x409)
#define USERF_IGNBANS           get_language(0x40a)
#define USERF_WRITING           get_language(0x40b)
#define USERF_ERRWRITE          get_language(0x40c)
#define USERF_ERRWRITE2         get_language(0x40d)
#define USERF_NONEEDNEW         get_language(0x40e)
#define USERF_REHASHING         get_language(0x40f)
#define USERF_UNKNOWN           get_language(0x410)
#define USERF_NOUSERREC         get_language(0x411)
#define USERF_BACKUP            get_language(0x412)
#define USERF_FAILEDXFER        get_language(0x413)
#define USERF_OLDSHARE          get_language(0x414)
#define USERF_ANTIQUESHARE      get_language(0x415)
#define USERF_REJECTED          get_language(0x416)

/* Misc messages */
#define MISC_EXPIRED            get_language(0x500)
#define MISC_TOTAL              get_language(0x501)
#define MISC_ERASED             get_language(0x502)
/* was: MISC_LEFT               0x503            */
#define MISC_ONLOCALE           get_language(0x504)
#define MISC_MATCHING           get_language(0x505)
#define MISC_SKIPPING           get_language(0x506)
#define MISC_TRUNCATED          get_language(0x507)
#define MISC_FOUNDMATCH         get_language(0x508)
#define MISC_AMBIGUOUS          get_language(0x509)
#define MISC_NOSUCHCMD          get_language(0x50a)
#define MISC_CMDBINDS           get_language(0x50b)
#define MISC_RESTARTING         get_language(0x50c)
#define MISC_MATCH_PLURAL       get_language(0x50d)
#define MISC_LOGSWITCH          get_language(0x50e)
/* was: MISC_OWNER              0x50f            */
/* was: MISC_MASTER             0x510            */
/* was: MISC_OP                 0x511            */
#define MISC_IDLE               get_language(0x512)
#define MISC_AWAY               get_language(0x513)
/* was: MISC_IGNORING           0x514            */
/* was: MISC_UNLINKED           0x515            */
#define MISC_DISCONNECTED       get_language(0x516)
#define MISC_INVALIDBOT         get_language(0x517)
#define MISC_LOOP               get_language(0x518)
/* was: MISC_MUTUAL             0x519            */
#define MISC_FROM               get_language(0x51a)
#define MISC_OUTDATED           get_language(0x51b)
#define MISC_REJECTED           get_language(0x51c)
#define MISC_IMPOSTER           get_language(0x51d)
#define MISC_TRYING             get_language(0x51e)
#define MISC_MOTDFILE           get_language(0x51f)
#define MISC_NOMOTDFILE         get_language(0x520)
#define MISC_USEFORMAT          get_language(0x521)
#define MISC_CHADDRFORMAT       get_language(0x522)
/* was: MISC_UNKNOWN            0x523            */
/* was: MISC_CHANNELS           0x524            */
/* was: MISC_TRYINGMISTAKE      0x525            */
#define MISC_PENDING            get_language(0x526)
#define MISC_WANTOPS            get_language(0x527)
/* was: MISC_LURKING            0x528            */
#define MISC_BACKGROUND         get_language(0x529)
#define MISC_TERMMODE           get_language(0x52a)
#define MISC_STATMODE           get_language(0x52b)
#define MISC_LOGMODE            get_language(0x52c)
#define MISC_ONLINEFOR          get_language(0x52d)
#define MISC_CACHEHIT           get_language(0x52e)
#define MISC_TCLLIBRARY         get_language(0x52f)
#define MISC_NEWUSERFLAGS       get_language(0x530)
#define MISC_NOTIFY             get_language(0x531)
#define MISC_PERMOWNER          get_language(0x532)
/* was: MISC_ROOTWARN           0x533            */
#define MISC_NOCONFIGFILE       get_language(0x534)
#define MISC_NOUSERFILE         get_language(0x535)
#define MISC_NOUSERFILE2        get_language(0x536)
#define MISC_USERFCREATE1       get_language(0x537)
#define MISC_USERFCREATE2       get_language(0x538)
#define MISC_USERFEXISTS        get_language(0x539)
#define MISC_CANTWRITETEMP      get_language(0x53a)
#define MISC_CANTRELOADUSER     get_language(0x53b)
#define MISC_MISSINGUSERF       get_language(0x53c)
/* was: MISC_BOTSCONNECTED      0x53d            */
#define MISC_BANNER             get_language(0x53e)
#define MISC_CLOGS              get_language(0x53f)
#define MISC_BANNER_STEALTH     get_language(0x540)
#define MISC_LOGREPEAT          get_language(0x541)
#define MISC_JUPED              get_language(0x542)
#define MISC_NOFREESOCK         get_language(0x543)
#define MISC_TCLVERSION         get_language(0x544)
#define MISC_TCLHVERSION        get_language(0x545)

/* IRC */
#define IRC_BANNED              get_language(0x600)
#define IRC_YOUREBANNED         get_language(0x601)
#define IRC_IBANNEDME           get_language(0x602)
#define IRC_FUNKICK             get_language(0x603)
#define IRC_HI                  get_language(0x604)
#define IRC_GOODBYE             get_language(0x605)
#define IRC_BANNED2             get_language(0x606)
#define IRC_NICKTOOLONG         get_language(0x607)
#define IRC_INTRODUCED          get_language(0x608)
#define IRC_COMMONSITE          get_language(0x609)
#define IRC_SALUT1              get_language(0x60a)
#define IRC_SALUT2              get_language(0x60b)
#define IRC_SALUT2A             get_language(0x60c)
#define IRC_SALUT2B             get_language(0x60d)
#define IRC_INITOWNER1          get_language(0x60e)
#define IRC_INIT1               get_language(0x60f)
#define IRC_INITNOTE            get_language(0x610)
#define IRC_INITINTRO           get_language(0x611)
#define IRC_PASS                get_language(0x612)
#define IRC_NOPASS              get_language(0x613)
/* was: IRC_NOPASS2             0x614            */
#define IRC_EXISTPASS           get_language(0x615)
#define IRC_PASSFORMAT          get_language(0x616)
#define IRC_SETPASS             get_language(0x617)
#define IRC_FAILPASS            get_language(0x618)
#define IRC_CHANGEPASS          get_language(0x619)
#define IRC_FAILCOMMON          get_language(0x61a)
#define IRC_MISIDENT            get_language(0x61b)
#define IRC_DENYACCESS          get_language(0x61c)
#define IRC_RECOGNIZED          get_language(0x61d)
#define IRC_ADDHOSTMASK         get_language(0x61e)
/* was: IRC_DELMAILADDR         0x61f            */
#define IRC_FIELDCURRENT        get_language(0x620)
#define IRC_FIELDCHANGED        get_language(0x621)
#define IRC_FIELDTOREMOVE       get_language(0x622)
/* was: IRC_NOEMAIL             0x623            */
#define IRC_INFOLOCKED          get_language(0x624)
#define IRC_REMINFOON           get_language(0x625)
#define IRC_REMINFO             get_language(0x626)
#define IRC_NOINFOON            get_language(0x627)
#define IRC_NOINFO              get_language(0x628)
#define IRC_NOMONITOR           get_language(0x629)
#define IRC_RESETCHAN           get_language(0x62a)
#define IRC_JUMP                get_language(0x62b)
#define IRC_CHANHIDDEN          get_language(0x62c)
#define IRC_ONCHANNOW           get_language(0x62d)
#define IRC_NEVERJOINED         get_language(0x62e)
#define IRC_LASTSEENAT          get_language(0x62f)
#define IRC_DONTKNOWYOU         get_language(0x630)
#define IRC_NOHELP              get_language(0x631)
#define IRC_NOHELP2             get_language(0x632)
#define IRC_NOTONCHAN           get_language(0x634)
#define IRC_GETORIGNICK         get_language(0x635)
#define IRC_BADBOTNICK          get_language(0x636)
#define IRC_BOTNICKINUSE        get_language(0x637)
#define IRC_CANTCHANGENICK      get_language(0x638)
#define IRC_BOTNICKJUPED        get_language(0x639)
#define IRC_CHANNELJUPED        get_language(0x63a)
#define IRC_NOTREGISTERED1      get_language(0x63b)
#define IRC_NOTREGISTERED2      get_language(0x63c)
#define IRC_FLOODIGNORE1        get_language(0x63d)
/* was: IRC_FLOODIGNORE2        0x63e            */
#define IRC_FLOODIGNORE3        get_language(0x63f)
#define IRC_FLOODKICK           get_language(0x640)
#define IRC_SERVERTRY           get_language(0x641)
#define IRC_DNSFAILED           get_language(0x642)
#define IRC_FAILEDCONNECT       get_language(0x643)
#define IRC_SERVERSTONED        get_language(0x644)
#define IRC_DISCONNECTED        get_language(0x645)
#define IRC_NOSERVER            get_language(0x646)
#define IRC_MODEQUEUE           get_language(0x647)
#define IRC_SERVERQUEUE         get_language(0x648)
#define IRC_HELPQUEUE           get_language(0x649)
/* was: IRC_BOTNOTONIRC         0x64a            */
/* was: IRC_NOTACTIVECHAN       0x64b            */
#define IRC_PROCESSINGCHAN      get_language(0x64c)
#define IRC_CHANNEL             get_language(0x64d)
#define IRC_DESIRINGCHAN        get_language(0x64e)
#define IRC_CHANNELTOPIC        get_language(0x64f)
#define IRC_PENDINGOP           get_language(0x650)
#define IRC_PENDINGDEOP         get_language(0x651)
#define IRC_PENDINGKICK         get_language(0x652)
#define IRC_FAKECHANOP          get_language(0x653)
#define IRC_ENDCHANINFO         get_language(0x654)
#define IRC_MASSKICK            get_language(0x655)
#define IRC_REMOVEDBAN          get_language(0x656)
#define IRC_UNEXPECTEDMODE      get_language(0x657)
#define IRC_POLITEKICK          get_language(0x658)
#define IRC_AUTOJUMP            get_language(0x659)
#define IRC_CHANGINGSERV        get_language(0x65a)
#define IRC_TOOMANYCHANS        get_language(0x65b)
#define IRC_CHANFULL            get_language(0x65c)
#define IRC_CHANINVITEONLY      get_language(0x65d)
#define IRC_BANNEDFROMCHAN      get_language(0x65e)
#define IRC_SERVNOTONCHAN       get_language(0x65f)
#define IRC_BADCHANKEY          get_language(0x660)
#define IRC_INTRO1              get_language(0x661)
#define IRC_BADHOST1            get_language(0x662)
#define IRC_BADHOST2            get_language(0x663)
#define IRC_NEWBOT1             get_language(0x664)
#define IRC_NEWBOT2             get_language(0x665)
#define IRC_TELNET              get_language(0x666)
#define IRC_TELNET1             get_language(0x667)
#define IRC_LIMBO               get_language(0x668)
#define IRC_TELNETFLOOD         get_language(0x669)
#define IRC_PREBANNED           get_language(0x66a)
#define IRC_JOIN_FLOOD          get_language(0x66b)
#define IRC_KICK_PROTECT        get_language(0x66c)
#define IRC_DEOP_PROTECT        get_language(0x66f)
#define IRC_COMMENTKICK         get_language(0x66d)
#define IRC_GETALTNICK          get_language(0x66e)
#define IRC_REMOVEDEXEMPT       get_language(0x670)
#define IRC_REMOVEDINVITE       get_language(0x671)
#define IRC_FLOODIGNORE4        get_language(0x672)
#define IRC_NICK_FLOOD          get_language(0x673)

/* Eggdrop command line usage */
#define EGG_USAGE               get_language(0x700)
#define EGG_RUNNING1            get_language(0x701)
#define EGG_RUNNING2            get_language(0x702)
#define EGG_NOWRITE             get_language(0x703)

#define USER_ISGLOBALOP         get_language(0x800)
#define USER_ISBOT              get_language(0x801)
#define USER_ISMASTER           get_language(0x802)

/* '.bans/.invites/.exempts' common messages */
#define MODES_CREATED           get_language(0x130)
#define MODES_LASTUSED          get_language(0x131)
#define MODES_INACTIVE          get_language(0x132)
#define MODES_PLACEDBY          get_language(0x133)
#define MODES_NOTACTIVE         get_language(0x135)
#define MODES_NOTACTIVE2        get_language(0x137)
#define MODES_NOTBYBOT          get_language(0x138)

/* Messages used when listing with `.bans' */
#define BANS_GLOBAL             get_language(0x104)
#define BANS_BYCHANNEL          get_language(0x106)
#define BANS_USEBANSALL         get_language(0x109)
#define BANS_NOLONGER           get_language(0x10a)

/* Messages used when listing with '.exempts' */
#define EXEMPTS_GLOBAL          get_language(0x114)
#define EXEMPTS_BYCHANNEL       get_language(0x116)
#define EXEMPTS_USEEXEMPTSALL   get_language(0x119)
#define EXEMPTS_NOLONGER        get_language(0x11a)

/* Messages used when listing with '.invites' */
#define INVITES_GLOBAL          get_language(0x124)
#define INVITES_BYCHANNEL       get_language(0x126)
#define INVITES_USEINVITESALL   get_language(0x129)
#define INVITES_NOLONGER        get_language(0x12a)


/* Messages referring to channels */
#define CHAN_NOSUCH             get_language(0x900)
#define CHAN_BADCHANMODE        get_language(0x902)
#define CHAN_MASSDEOP           get_language(0x903)
#define CHAN_MASSDEOP_KICK      get_language(0x904)
#define CHAN_FORCEJOIN          get_language(0x907)
#define CHAN_FAKEMODE           get_language(0x908)
#define CHAN_FAKEMODE_KICK      get_language(0x909)
#define CHAN_DESYNCMODE         get_language(0x90a)
#define CHAN_DESYNCMODE_KICK    get_language(0x90b)
#define CHAN_FLOOD              get_language(0x90c)

/* Messages referring to ignores */
#define IGN_NONE                get_language(0xa00)
#define IGN_CURRENT             get_language(0xa01)
#define IGN_NOLONGER            get_language(0xa02)

/* Messages referring to bots */
#define BOT_NOTHERE             get_language(0xb00)
#define BOT_NONOTES             get_language(0xb01)
#define BOT_USERAWAY            get_language(0xb02)
#define BOT_NOTEARRIVED         get_language(0xb07)
#define BOT_MSGDIE              get_language(0xb18)
#define BOT_NOSUCHUSER          get_language(0xb19)
#define BOT_NOCHANNELS          get_language(0xb1a)
#define BOT_PARTYMEMBS          get_language(0xb1b)
#define BOT_BOTSCONNECTED       get_language(0xb1c)
#define BOT_OTHERPEOPLE         get_language(0xb1d)
/* was: BOT_OUTDATEDWHOM - 0xb1e                 */
#define BOT_LINKATTEMPT         get_language(0xb1f)
#define BOT_NOTESTORED2         get_language(0xb20)
#define BOT_NOTEBOXFULL         get_language(0xb21)
#define BOT_NOTEISAWAY          get_language(0xb22)
#define BOT_NOTESENTTO          get_language(0xb23)
#define BOT_DISCONNECTED        get_language(0xb24)
#define BOT_PEOPLEONCHAN        get_language(0xb25)
#define BOT_CANTLINKTHERE       get_language(0xb26)
#define BOT_CANTUNLINK          get_language(0xb27)
#define BOT_LOOPDETECT          get_language(0xb28)
#define BOT_BOGUSLINK           get_language(0xb29)
/* was: BOT_BOGUSLINK2 - 0xb2a                   */
#define BOT_DISCONNLEAF         get_language(0xb2b)
#define BOT_LINKEDTO            get_language(0xb2c)
#define BOT_ILLEGALLINK         get_language(0xb2d)
#define BOT_YOUREALEAF          get_language(0xb2e)
#define BOT_REJECTING           get_language(0xb2f)
#define BOT_OLDBOT              get_language(0xb30)
#define BOT_TRACERESULT         get_language(0xb31)
#define BOT_DOESNTEXIST         get_language(0xb32)
#define BOT_NOREMOTEBOOT        get_language(0xb33)
#define BOT_NOOWNERBOOT         get_language(0xb34)
#define BOT_XFERREJECTED        get_language(0xb35)
/* was: BOT_NOFILESYS - 0xb36                    */
#define BOT_BOTNETUSERS         get_language(0xb37)
#define BOT_PARTYLINE           get_language(0xb38)
#define BOT_LOCALCHAN           get_language(0xb39)
#define BOT_USERSONCHAN         get_language(0xb3a)
#define BOT_NOBOTSLINKED        get_language(0xb3b)
#define BOT_NOTRACEINFO         get_language(0xb3c)
#define BOT_COMPLEXTREE         get_language(0xb3d)
#define BOT_UNLINKALL           get_language(0xb3e)
#define BOT_KILLLINKATTEMPT     get_language(0xb3f)
#define BOT_ENDLINKATTEMPT      get_language(0xb40)
#define BOT_BREAKLINK           get_language(0xb41)
#define BOT_UNLINKEDFROM        get_language(0xb42)
#define BOT_NOTCONNECTED        get_language(0xb43)
#define BOT_WIPEBOTTABLE        get_language(0xb44)
#define BOT_BOTUNKNOWN          get_language(0xb45)
#define BOT_CANTLINKMYSELF      get_language(0xb46)
#define BOT_ALREADYLINKED       get_language(0xb47)
#define BOT_NOTELNETADDY        get_language(0xb48)
#define BOT_LINKING             get_language(0xb49)
#define BOT_CANTFINDRELAYUSER   get_language(0xb4a)
#define BOT_CANTLINKTO          get_language(0xb4b)
#define BOT_CANTRELAYMYSELF     get_language(0xb4c)
#define BOT_CONNECTINGTO        get_language(0xb4d)
#define BOT_BYEINFO1            get_language(0xb4e)
#define BOT_ABORTRELAY1         get_language(0xb4f)
#define BOT_ABORTRELAY2         get_language(0xb50)
#define BOT_ABORTRELAY3         get_language(0xb51)
/* was: BOT_PARTYJOINED - 0xb52                  */
#define BOT_LOSTDCCUSER         get_language(0xb53)
#define BOT_DROPPINGRELAY       get_language(0xb54)
#define BOT_RELAYSUCCESS        get_language(0xb55)
#define BOT_BYEINFO2            get_language(0xb56)
#define BOT_RELAYLINK           get_language(0xb57)
#define BOT_PARTYLEFT           get_language(0xb58)
#define BOT_ENDRELAY1           get_language(0xb59)
#define BOT_ENDRELAY2           get_language(0xb5a)
#define BOT_PARTYREJOINED       get_language(0xb5b)
#define BOT_DROPPEDRELAY        get_language(0xb5c)
#define BOT_BREAKRELAY          get_language(0xb5d)
#define BOT_RELAYBROKEN         get_language(0xb5e)
#define BOT_PINGTIMEOUT         get_language(0xb5f)
#define BOT_BOTNOTLEAFLIKE      get_language(0xb60)
#define BOT_BOTDROPPED          get_language(0xb61)
#define BOT_ALREADYLINKING      get_language(0xb62)

/* Messages pertaining to MODULES */
#define MOD_ALREADYLOAD         get_language(0x200)
#define MOD_BADCWD              get_language(0x201)
#define MOD_NOSTARTDEF          get_language(0x202)
#define MOD_NEEDED              get_language(0x204)
#define MOD_NOCLOSEDEF          get_language(0x205)
#define MOD_UNLOADED            get_language(0x206)
#define MOD_NOSUCH              get_language(0x207)
/* was: MOD_NOINFO - 0x208                       */
#define MOD_LOADERROR           get_language(0x209)
#define MOD_UNLOADERROR         get_language(0x20a)
#define MOD_CANTLOADMOD         get_language(0x20b)
#define MOD_STAGNANT            get_language(0x20c)
#define MOD_NOCRYPT             get_language(0x20d)
#define MOD_NOFILESYSMOD        get_language(0x20e)
#define MOD_LOADED_WITH_LANG    get_language(0x20f)
#define MOD_LOADED              get_language(0x210)

#define DCC_NOSTRANGERS         get_language(0xc00)
#define DCC_REFUSED             get_language(0xc01)
#define DCC_REFUSED2            get_language(0xc02)
#define DCC_REFUSED3            get_language(0xc03)
#define DCC_REFUSED4            get_language(0xc04)
#define DCC_REFUSED5            get_language(0xc05)
/* was: DCC_REFUSED6            0xc06            */
#define DCC_REFUSED7            get_language(0xc21)
#define DCC_TOOMANY             get_language(0xc07)
/* was: DCC_TRYLATER            0xc08            */
/* was: DCC_REFUSEDTAND         0xc09            */
/* was: DCC_NOSTRANGERFILES1    0xc0a            */
/* was: DCC_NOSTRANGERFILES2    0xc0b            */
#define DCC_TOOMANYDCCS1        get_language(0xc0c)
#define DCC_TOOMANYDCCS2        get_language(0xc0d)
/* was: DCC_DCCNOTSUPPORTED     0xc0e            */
/* was: DCC_REFUSEDNODCC        0xc0f            */
/* was: DCC_FILENAMEBADSLASH    0xc10            */
/* was: DCC_MISSINGFILESIZE     0xc11            */
/* was: DCC_FILEEXISTS          0xc12            */
/* was: DCC_CREATEERROR         0xc13            */
/* was: DCC_FILEBEINGSENT       0xc14            */
/* was: DCC_REFUSEDNODCC2       0xc15            */
/* was: DCC_REFUSEDNODCC3       0xc16            */
/* was: DCC_FILETOOLARGE        0xc17            */
/* was: DCC_FILETOOLARGE2       0xc18            */
#define DCC_CONNECTFAILED1      get_language(0xc19)
#define DCC_CONNECTFAILED2      get_language(0xc1a)
#define DCC_CONNECTFAILED3      get_language(0xc22)
/* was: DCC_FILESYSBROKEN                  0xc1b */
#define DCC_ENTERPASS           get_language(0xc1c)
#define DCC_FLOODBOOT           get_language(0xc1d)
#define DCC_BOOTED1             get_language(0xc1e)
#define DCC_BOOTED2             get_language(0xc1f)
#define DCC_BOOTED3             get_language(0xc20)

/* Stuff from chan.c */
/* was: CHAN_LIMBOBOT           0xd00            */

/* BOTNET messages
 */
#define NET_FAKEREJECT          get_language(0xe00)
#define NET_LINKEDTO            get_language(0xe01)
#define NET_WRONGBOT            get_language(0xe02)
#define NET_LEFTTHE             get_language(0xe03)
#define NET_JOINEDTHE           get_language(0xe04)
#define NET_AWAY                get_language(0xe05)
#define NET_UNAWAY              get_language(0xe06)
#define NET_NICKCHANGE          get_language(0xe07)

/* Stuff from dcc.c */
#define DCC_REJECT              get_language(0xe08)
#define DCC_LINKED              get_language(0xe09)
#define DCC_LINKFAIL            get_language(0xe0a)
#define DCC_BADPASS             get_language(0xe0b)
#define DCC_PASSREQ             get_language(0xe0c)
#define DCC_LINKERROR           get_language(0xe0d)
#define DCC_LOSTBOT             get_language(0xe0e)
#define DCC_TIMEOUT             get_language(0xe0f)
#define DCC_LOGGEDIN            get_language(0xe10)
#define DCC_BADLOGIN            get_language(0xe11)
#define DCC_HOUSTON             get_language(0xe12)
#define DCC_JOIN                get_language(0xe13)
#define DCC_LOSTDCC             get_language(0xe14)
#define DCC_PWDTIMEOUT          get_language(0xe15)
#define DCC_CLOSED              get_language(0xe16)
#define DCC_FAILED              get_language(0xe17)
#define DCC_BADSRC              get_language(0xe18)
/* was: DCC_BADIP               0xe19            */
#define DCC_BADHOST             get_language(0xe1a)
#define DCC_TELCONN             get_language(0xe1b)
#define DCC_IDENTFAIL           get_language(0xe1c)
#define DCC_PORTDIE             get_language(0xe1d)
#define DCC_BADNICK             get_language(0xe1e)
#define DCC_NONBOT              get_language(0xe1f)
#define DCC_NONUSER             get_language(0xe20)
#define DCC_INVHANDLE           get_language(0xe21)
#define DCC_DUPLICATE           get_language(0xe22)
#define DCC_NOPASS              get_language(0xe23)
#define DCC_LOSTCON             get_language(0xe24)
#define DCC_TTIMEOUT            get_language(0xe25)
#define DCC_INSTCOMPL           get_language(0xe26)
#define DCC_NEWUSER             get_language(0xe27)
#define DCC_LOSTNEWUSER         get_language(0xe28)
#define DCC_LOSTNEWUSR2         get_language(0xe29)
#define DCC_TIMEOUTUSER         get_language(0xe2a)
#define DCC_TIMEOUTUSR2         get_language(0xe2b)
#define DCC_TCLERROR            get_language(0xe2c)
#define DCC_DEADSOCKET          get_language(0xe2d)
#define DCC_LOSTCONN            get_language(0xe2e)
#define DCC_EOFIDENT            get_language(0xe2f)
#define DCC_LOSTIDENT           get_language(0xe30)
#define DCC_NOACCESS            get_language(0xe31)
#define DCC_MYBOTNETNICK        get_language(0xe32)
#define DCC_LOSTDUP             get_language(0xe33)

#endif /* _EGG_LANG_H */
