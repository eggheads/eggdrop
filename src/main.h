/*
 * main.h - include file to include most other include files
 * 
 */
#ifndef MAKING_MODS
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#endif

#ifdef HAVE_STDARG_H		/* do we have stdarg.h ? */
#ifndef _STDARG_H		/* is stdarg.h already included ? */
#include <stdarg.h>
#endif				/* _STDARG_H */
#define VARARGS(type, name) (type name, ...)
#define VARARGS_DEF(type, name) (type name, ...)
#define VARARGS_START(type, name, list) (va_start(list, name), name)
#else				/* guess not, fall back on varargs.h */
#ifndef _VARARGS_H		/* is varargs.h already included ? */
#include <varargs.h>
#endif				/* _VARARGS_H */
#define VARARGS(type, name) ()
#define VARARGS_DEF(type, name) (va_alist) va_dcl
#define VARARGS_START(type, name, list) (va_start(list), va_arg(list,type))
#endif				/* HAVE_STDARG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "lang.h"
#include "eggdrop.h"
#include "flags.h"
#ifndef MAKING_MODS
#include "proto.h"
#endif
#include "cmdt.h"
#include "tclegg.h"
#include "tclhash.h"
#include "chan.h"
#include "users.h"
#include "rfc1459.h"

#ifndef MAKING_MODS
extern struct dcc_table DCC_CHAT, DCC_BOT, DCC_LOST, DCC_SCRIPT, DCC_BOT_NEW,
 DCC_RELAY, DCC_RELAYING, DCC_FORK_RELAY, DCC_PRE_RELAY, DCC_CHAT_PASS,
 DCC_FORK_BOT, DCC_SOCKET, DCC_TELNET_ID, DCC_TELNET_NEW, DCC_TELNET_PW,
 DCC_TELNET, DCC_IDENT, DCC_IDENTWAIT;

#endif

/* from net.h */

/* my own byte swappers */
#ifdef WORDS_BIGENDIAN
#define swap_short(sh) (sh)
#define swap_long(ln) (ln)
#else
#define swap_short(sh) ((((sh) & 0xff00) >> 8) | (((sh) & 0x00ff) << 8))
#define swap_long(ln) (swap_short(((ln)&0xffff0000)>>16) | \
                       (swap_short((ln)&0x0000ffff)<<16))
#endif
#define iptolong(a) swap_long((unsigned long)a)
#define fixcolon(x) if (x[0]==':') {x++;} else {x=newsplit(&x);}

/* Stupid Borg Cube crap ;p */
#ifdef BORGCUBES

/* net.h needs this */
#define O_NONBLOCK      00000004	/* POSIX non-blocking I/O       */

/* mod/filesys.mod/filedb.c needs this */
#define _S_IFMT         0170000		/* type of file */
#define _S_IFDIR        0040000		/*   directory */
#define S_ISDIR(m)      (((m)&(_S_IFMT)) == (_S_IFDIR))

#endif
