/*
 * main.h
 *   include file to include most other include files
 *
 * $Id: main.h,v 1.43 2011/07/09 15:07:48 thommey Exp $
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

#ifndef _EGG_MAIN_H
#define _EGG_MAIN_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "lush.h"

#if ((TCL_MAJOR_VERSION > 7) || ((TCL_MAJOR_VERSION == 7) && (TCL_MINOR_VERSION >= 5)))
#  define USE_TCL_EVENTS
#  define USE_TCL_FINDEXEC
#  define USE_TCL_PACKAGE
#  define USE_TCL_VARARGS
#endif

#if (TCL_MAJOR_VERSION >= 8)
#  define USE_TCL_OBJ
#endif

#if (((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 1)) || (TCL_MAJOR_VERSION > 8))
#  define USE_TCL_BYTE_ARRAYS
#  define USE_TCL_ENCODING
#endif

#if defined(HAVE_TCL_SETNOTIFIER) && defined(HAVE_TCL_GETTHREADDATA) && defined(HAVE_TCL_NOTIFIER_INIT)
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
#if defined(USE_TCL_VARARGS) && (defined(__STDC__) || defined(HAS_STDARG))
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

/* For pre Tcl7.5p1 versions */
#ifndef HAVE_TCL_FREE
#  define Tcl_Free(x) n_free(x, "", 0)
#endif

/* For pre7.6 Tcl versions */
#ifndef TCL_PATCH_LEVEL
#  define TCL_PATCH_LEVEL Tcl_GetVar(interp, "tcl_patchLevel", TCL_GLOBAL_ONLY)
#endif

#ifndef MAKING_MODS
extern struct dcc_table DCC_CHAT, DCC_BOT, DCC_LOST, DCC_SCRIPT, DCC_BOT_NEW,
                        DCC_RELAY, DCC_RELAYING, DCC_FORK_RELAY, DCC_PRE_RELAY,
                        DCC_CHAT_PASS, DCC_FORK_BOT, DCC_SOCKET, DCC_TELNET_ID,
                        DCC_TELNET_NEW, DCC_TELNET_PW, DCC_TELNET, DCC_IDENT,
                        DCC_IDENTWAIT, DCC_DNSWAIT;
#endif

#define iptolong(a) (0xffffffff & (long) (htonl((unsigned long) a)))
#define fixcolon(x) do {                                                \
        if ((x)[0] == ':')                                              \
          (x)++;                                                        \
        else                                                            \
          (x) = newsplit(&(x));                                         \
} while (0)

/* This macro copies (_len - 1) bytes from _source to _target. The
 * target string is NULL-terminated.
 */
#define strncpyz(_target, _source, _len) do {                           \
        strncpy((_target), (_source), (_len) - 1);                      \
        (_target)[(_len) - 1] = 0;                                      \
} while (0)

#ifdef BORGCUBES
#  define O_NONBLOCK 00000004 /* POSIX non-blocking I/O */
#endif /* BORGCUBES */

#endif /* _EGG_MAIN_H */
