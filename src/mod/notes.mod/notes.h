/*
 * notes.h -- part of notes.mod
 *
 * $Id: notes.h,v 1.19 2011/02/13 14:19:34 simple Exp $
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

#ifndef _EGG_MOD_NOTES_NOTES_H
#define _EGG_MOD_NOTES_NOTES_H

#define NOTES_IGNKEY "NOTESIGNORE"

/* language #define's */

#define NOTES_USAGE                     MISC_USAGE
#define NOTES_USERF_UNKNOWN             USERF_UNKNOWN
#define NOTES_FORWARD_TO                get_language(0xc000)
#define NOTES_SWITCHED_NOTES            get_language(0xc001)
#define NOTES_EXPIRED                   get_language(0xc002)
#define NOTES_FORWARD_NOTONLINE         get_language(0xc003)
#define NOTES_UNSUPPORTED               get_language(0xc004)
#define NOTES_NOTES2MANY                get_language(0xc005)
#define NOTES_NOTEFILE_FAILED           get_language(0xc006)
#define NOTES_NOTEFILE_UNREACHABLE      get_language(0xc007)
#define NOTES_STORED_MESSAGE            get_language(0xc008)
#define NOTES_NO_MESSAGES               get_language(0xc009)
#define NOTES_EXPIRE_TODAY              get_language(0xc00a)
#define NOTES_EXPIRE_XDAYS              get_language(0xc00b)
#define NOTES_WAITING                   get_language(0xc00c)
#define NOTES_NOT_THAT_MANY             get_language(0xc00d)
#define NOTES_DCC_USAGE_READ            get_language(0xc00e)
#define NOTES_FAILED_CHMOD              get_language(0xc00f)
#define NOTES_ERASED_ALL                get_language(0xc010)
#define NOTES_ERASED                    get_language(0xc011)
#define NOTES_LEFT                      get_language(0xc012)
#define NOTES_MAYBE                     get_language(0xc013)
#define NOTES_NOTTO_BOT                 get_language(0xc014)
#define NOTES_OUTSIDE                   get_language(0xc015)
#define NOTES_DELIVERED                 get_language(0xc016)
#define NOTES_FORLIST                   get_language(0xc017)
/* WAS: 0xc018 NOTES_WAITING_ON */
#define NOTES_WAITING2                  get_language(0xc019)
/* WAS: 0xc01a NOTES_DCC_USAGE_READ2 */
#define NOTES_STORED                    get_language(0xc01b)
#define NOTES_IGN_OTHERS                get_language(0xc01c)
#define NOTES_UNKNOWN_USER              get_language(0xc01d)
#define NOTES_IGN_NEW                   get_language(0xc01e)
#define NOTES_IGN_ALREADY               get_language(0xc01f)
#define NOTES_IGN_REM                   get_language(0xc020)
#define NOTES_IGN_NOTFOUND              get_language(0xc021)
#define NOTES_IGN_NONE                  get_language(0xc022)
#define NOTES_IGN_FOR                   get_language(0xc023)
#define NOTES_NO_SUCH_USER              get_language(0xc024)
#define NOTES_FWD_OWNER                 get_language(0xc025)
#define NOTES_FWD_FOR                   get_language(0xc026)
#define NOTES_FWD_BOTNAME               get_language(0xc027)
#define NOTES_FWD_CHANGED               get_language(0xc028)
#define NOTES_MUSTBE                    get_language(0xc029)
#define NOTES_DCC_USAGE_READ2           get_language(0xc02a)
#define NOTES_WAITING_NOTICE            get_language(0xc02b)

#ifdef MAKING_NOTES
static int get_note_ignores(struct userrec *, char ***);
static int add_note_ignore(struct userrec *, char *);
static int del_note_ignore(struct userrec *, char *);
static int match_note_ignore(struct userrec *, char *);
static void notes_read(char *, char *, char *, int);
static void notes_del(char *, char *, char *, int);
static void fwd_display(int, struct user_entry *);
#endif /* MAKING_NOTES */

#endif /* _EGG_MOD_NOTES_H */
