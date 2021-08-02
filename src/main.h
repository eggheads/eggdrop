/*
 * main.h
 *   include file to include most other include files
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

#ifndef _EGG_MAIN_H
#define _EGG_MAIN_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "eggint.h"
#include "lush.h"

#ifndef TCL_PATCH_LEVEL
#  define TCL_PATCH_LEVEL "*unknown*"
#endif

#if defined(HAVE_TCL_NOTIFIER_INIT)
#  define REPLACE_NOTIFIER
#endif

#if (((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)) || (TCL_MAJOR_VERSION > 8))
#  ifdef CONST
#    define EGG_CONST CONST
#  else
#    define EGG_CONST
#  endif
#else
#  define EGG_CONST
#endif

#ifdef CONST86
#  define TCL_CONST86 CONST86
#else
#  define TCL_CONST86
#endif

/* UGH! Why couldn't Tcl pick a standard? */
#if defined(__STDC__) || defined(HAS_STDARG)
#  ifdef HAVE_STDARG_H
#    include <stdarg.h>
#  endif
#  define EGG_VARARGS(type, name) (type name, ...)
#  define EGG_VARARGS_DEF(type, name) (type name, ...)
#  define EGG_VARARGS_START(type, name, list) (va_start(list, name), name)
#else
#  ifndef MAKING_DEPEND /* Allows 'make depend' to work on newer GCC versions. */
#    include <varargs.h>
#    define EGG_VARARGS(type, name) ()
#    define EGG_VARARGS_DEF(type, name) (va_alist) va_dcl
#    define EGG_VARARGS_START(type, name, list) (va_start(list), va_arg(list,type))
#  endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#  include <strings.h>
#endif

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif

#include <sys/types.h>
#include "lush.h"
#include "lang.h"
#include "eggdrop.h"
#include "flags.h"

#ifndef MAKING_MODS
#  include "proto.h"
#endif

#include "cmdt.h"
#include "tclegg.h"
#include "tclhash.h"
#include "chan.h"
#include "users.h"
#include "compat/compat.h"

#ifndef MAKING_MODS
extern struct dcc_table DCC_CHAT, DCC_BOT, DCC_LOST, DCC_SCRIPT, DCC_BOT_NEW,
                        DCC_RELAY, DCC_RELAYING, DCC_FORK_RELAY, DCC_PRE_RELAY,
                        DCC_CHAT_PASS, DCC_FORK_BOT, DCC_SOCKET, DCC_TELNET_ID,
                        DCC_TELNET_NEW, DCC_TELNET_PW, DCC_TELNET, DCC_IDENT,
                        DCC_IDENTWAIT, DCC_DNSWAIT;
#endif

#define iptolong(a) (0xffffffff & (long) (htonl((unsigned long) a)))

#ifdef IPV6
# define setsnport(s, p) do {                                           \
  if ((s).family == AF_INET6)                                           \
    (s).addr.s6.sin6_port = htons((p));                                 \
  else                                                                  \
    (s).addr.s4.sin_port = htons((p));                                  \
} while (0)
#else
# define setsnport(s, p) (s.addr.s4.sin_port = htons(p))
#endif

#define fixcolon(x) do {                                                \
        if ((x)[0] == ':')                                              \
          (x)++;                                                        \
        else                                                            \
          (x) = newsplit(&(x));                                         \
} while (0)

#ifdef BORGCUBES
#  define O_NONBLOCK 00000004 /* POSIX non-blocking I/O */
#endif /* BORGCUBES */

/* Handle for the user that's used when starting eggdrop with -tn */
#define EGG_BG_HANDLE "-HQ"
/* Default recommended flags for this user, use | as splitter */
#define EGG_BG_CONMASK LOG_MISC /* "o" */

/* Stringify macros */
#define EGG_MACRO_STR(x) EGG_STR(x)
#define EGG_STR(x) #x

#define EGG_AC_ARGS EGG_MACRO_STR(EGG_AC_ARGS_RAW)

#define ARRAY_SIZE(x) (sizeof (x) / sizeof *(x))

#endif /* _EGG_MAIN_H */
