/*
 * transfer.h -- part of transfer.mod
 *
 * $Id: transfer.h,v 1.28 2011/02/13 14:19:34 simple Exp $
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

#ifndef _EGG_MOD_TRANSFER_TRANSFER_H
#define _EGG_MOD_TRANSFER_TRANSFER_H

enum dccsend_types {
  DCCSEND_OK = 0,
  DCCSEND_FULL,                 /* DCC table is full                    */
  DCCSEND_NOSOCK,               /* Can not open a listening socket      */
  DCCSEND_BADFN,                /* No such file                         */
  DCCSEND_FEMPTY                /* File is empty                        */
};

/* File matching */
#define FILEMATCH (match+sofar)
#define FILEQUOTE '\\'
#define FILEWILDS '*'
#define FILEWILDQ '?'

/* Language data */
#define TRANSFER_COPY_FAILED        get_language(0xf00)
#define TRANSFER_FILESYS_BROKEN     get_language(0xf01)
#define TRANSFER_FILE_ARRIVE        get_language(0xf02)
#define TRANSFER_LOG_CONFULL        get_language(0xf03)
#define TRANSFER_NOTICE_CONFULL     get_language(0xf04)
#define TRANSFER_LOG_SOCKERR        get_language(0xf05)
#define TRANSFER_NOTICE_SOCKERR     get_language(0xf06)
#define TRANSFER_LOG_FILEEMPTY      get_language(0xf07)
#define TRANSFER_NOTICE_FILEEMPTY   get_language(0xf08)
#define TRANSFER_SEND_TO            get_language(0xf09)
#define TRANSFER_LINES              get_language(0xf0a)
#define TRANSFER_WAITING            get_language(0xf0b)
#define TRANSFER_DONE               get_language(0xf0c)
#define TRANSFER_QUEUED_UP          get_language(0xf0d)
#define TRANSFER_TOTAL              get_language(0xf0e)
#define TRANSFER_CANCELLED          get_language(0xf0f)
#define TRANSFER_ABORT_DCCSEND      get_language(0xf10)
#define TRANSFER_NOTICE_ABORT       get_language(0xf11)
#define TRANSFER_DCC_CANCEL         get_language(0xf12)
#define TRANSFER_NO_MATCHES         get_language(0xf13)
#define TRANSFER_CANCELLED_FILE     get_language(0xf14)
#define TRANSFER_COMPLETED_DCC      get_language(0xf15)
#define TRANSFER_FILENAME_TOOLONG   get_language(0xf16)
#define TRANSFER_NOTICE_FNTOOLONG   get_language(0xf17)
#define TRANSFER_TOO_BAD            get_language(0xf18)
#define TRANSFER_NOTICE_TOOBAD      get_language(0xf19)
#define TRANSFER_FAILED_MOVE        get_language(0xf1a)
#define TRANSFER_THANKS             get_language(0xf1b)
#define TRANSFER_NOTICE_THANKS      get_language(0xf1c)
#define TRANSFER_USERFILE_LOST      get_language(0xf1d)
/* #define TRANSFER_BYE             get_language(0xf1e) */
#define TRANSFER_USERFILE_DISCON    get_language(0xf1f)
#define TRANSFER_LOST_DCCSEND       get_language(0xf20)
#define TRANSFER_REGET_PACKET       get_language(0xf21)
#define TRANSFER_BEHIND_FILEEND     get_language(0xf22)
#define TRANSFER_TRY_SKIP_AHEAD     get_language(0xf23)
#define TRANSFER_RESUME_FILE        get_language(0xf24)
#define TRANSFER_COMPLETED_USERFILE get_language(0xf25)
#define TRANSFER_FINISHED_DCCSEND   get_language(0xf26)
#define TRANSFER_ABORT_USERFILE     get_language(0xf27)
#define TRANSFER_LOST_DCCGET        get_language(0xf28)
#define TRANSFER_BOGUS_FILE_LENGTH  get_language(0xf29)
#define TRANSFER_FILE_TOO_LONG      get_language(0xf2a)
#define TRANSFER_USERFILE_TIMEOUT   get_language(0xf2b)
#define TRANSFER_DICONNECT_TIMEOUT  get_language(0xf2c)
#define TRANSFER_NOTICE_TIMEOUT     get_language(0xf2d)
#define TRANSFER_LOG_TIMEOUT        get_language(0xf2e)
#define TRANSFER_DCC_GET_TIMEOUT    get_language(0xf2f)
#define TRANSFER_DCC_SEND_TIMEOUT   get_language(0xf30)
#define TRANSFER_SEND               get_language(0xf31)
#define TRANSFER_SEND_WAITED        get_language(0xf32)
#define TRANSFER_CONN_SEND          get_language(0xf33)
#define TRANSFER_DCC_CONN           get_language(0xf34)
#define TRANSFER_NOTICE_BAD_CONN    get_language(0xf35)
#define TRANSFER_LOG_BAD_CONN       get_language(0xf36)
#define TRANSFER_BEGIN_DCC          get_language(0xf37)
#define TRANSFER_RE                 get_language(0xf38)
#define TRANSFER_DCC_IGNORED        get_language(0xf39)
#define TRANSFER_UNLOADING          get_language(0xf40)
#define TRANSFER_STAT_1             get_language(0xf41)
#define TRANSFER_STAT_2             get_language(0xf42)

#ifndef MAKING_TRANSFER
/* 4 - 7 */
#define DCC_FORK_SEND (*(struct dcc_table *)(transfer_funcs[4]))
#define at_limit(a) (((int (*) (char *))transfer_funcs[5])(a))
/* Was copy_to_tmp (moved to core) <Wcc[01/20/03]>. */
#define fileq_cancel(a,b) (((void (*) (int,char *))transfer_funcs[7])(a,b))
/* 8 - 11 */
#define queue_file(a,b,c,d) (((void (*)(char *,char *,char *,char *))transfer_funcs[8])(a,b,c,d))
#define raw_dcc_send(a,b,c,d) (((int (*) (char *,char *,char *,char *))transfer_funcs[9])(a,b,c,d))
#define show_queued_files(a) (((void (*) (int))transfer_funcs[10])(a))
#define wild_match_file(a,b) (((int (*)(register char *, register char *))transfer_funcs[11])(a,b))
/* 12 - 15 */
#define wipe_tmp_filename(a,b) (((void (*) (char *,int))transfer_funcs[12])(a,b))
#define DCC_GET (*(struct dcc_table *)(transfer_funcs[13]))
#define H_rcvd (*(p_tcl_bind_list*)(transfer_funcs[14]))
#define H_sent (*(p_tcl_bind_list*)(transfer_funcs[15]))
/* 16 - 19 */
#define USERENTRY_FSTAT (*(struct user_entry_type *)(transfer_funcs[16]))
/* Was quiet_reject (moved to core) <Wcc[01/20/03]>. */
#define raw_dcc_resend(a,b,c,d) (((int (*) (char *,char *,char *,char *))transfer_funcs[18])(a,b,c,d))
#define H_lost (*(p_tcl_bind_list*)(transfer_funcs[19]))
/* 20 - 23 */
#define H_tout (*(p_tcl_bind_list*)(transfer_funcs[20]))
#define DCC_SEND (*(struct dcc_table *)(transfer_funcs[21]))
#define DCC_GET_PENDING (*(struct dcc_table *)(transfer_funcs[22]))

#else /* MAKING_TRANSFER */

static void dcc_fork_send(int, char *, int);
static void stats_add_dnload(struct userrec *, unsigned long);
static void stats_add_upload(struct userrec *, unsigned long);
static void wipe_tmp_filename(char *, int);
static void dcc_get_pending(int, char *, int);
static void queue_file(char *, char *, char *, char *);
static int raw_dcc_resend(char *, char *, char *, char *);
static int raw_dcc_send(char *, char *, char *, char *);
static int at_limit(char *);
static int fstat_gotshare(struct userrec *u, struct user_entry *e, char *par,
                          int idx);
static int fstat_dupuser(struct userrec *u, struct userrec *o,
                         struct user_entry *e);
static int fstat_tcl_set(Tcl_Interp *irp, struct userrec *u,
                         struct user_entry *e, int argc, char **argv);
static void stats_add_dnload(struct userrec *u, unsigned long bytes);
static void stats_add_upload(struct userrec *u, unsigned long bytes);
static int wild_match_file(register char *, register char *);
static int server_transfer_setup(char *);

#define TRANSFER_REGET_PACKETID 0xfeab

typedef struct {
  u_16bit_t packet_id;          /* Identification ID, should be equal
                                 * to TRANSFER_REGET_PACKETID           */
  u_8bit_t byte_order;          /* Byte ordering, see byte_order_test() */
  u_32bit_t byte_offset;        /* Number of bytes to skip relative to
                                 * the file beginning                   */
} transfer_reget;

typedef struct zarrf {
  char *dir;                    /* Absolute dir if it starts with '*',
                                 * otherwise dcc dir.                   */
  char *file;
  char nick[NICKLEN];           /* Who queued this file                 */
  char to[NICKLEN];             /* Who will it be sent to               */
  struct zarrf *next;
} fileq_t;

#endif /* MAKING_TRANSFER */
#endif /* _EGG_MOD_TRANSFER_TRANSFER_H */
