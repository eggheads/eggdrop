/* 
 * transfer.h -- part of transfer.mod
 * 
 * $Id: transfer.h,v 1.6 2000/01/02 02:42:13 fabian Exp $
 */
/* 
 * Copyright (C) 1997  Robey Pointer
 * Copyright (C) 1999  Eggheads
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

enum {
  DCCSEND_OK = 0,
  DCCSEND_FULL,		/* DCC table is full			*/
  DCCSEND_NOSOCK,	/* Can not open a listening socket	*/
  DCCSEND_BADFN,	/* No such file				*/
};

#ifndef MAKING_TRANSFER
/* 4 - 7 */
#define DCC_FORK_SEND (*(struct dcc_table *)(transfer_funcs[4]))
#define at_limit(a) (((int (*) (char *))transfer_funcs[5])(a))
#define copy_to_tmp (*(int *)(transfer_funcs[6]))
#define fileq_cancel(a,b) (((void (*) (int,char *))transfer_funcs[7])(a,b))
/* 8 - 11 */
#define queue_file(a,b,c,d) (((void (*)(char *,char *,char *,char *))transfer_funcs[8])(a,b,c,d))
#define raw_dcc_send(a,b,c,d) (((int (*) (char *,char *,char *,char *))transfer_funcs[9])(a,b,c,d))
#define show_queued_files(a) (((void (*) (int))transfer_funcs[10])(a))
#define wild_match_file(a,b) (((int (*)(register char * m, register char * n))transfer_funcs[11])(a,b))
/* 12 - 15 */
#define wipe_tmp_filename(a,b) (((void (*) (char *,int))transfer_funcs[12])(a,b))
#define DCC_GET (*(struct dcc_table *)(transfer_funcs[13]))
#define H_rcvd (*(p_tcl_bind_list*)(transfer_funcs[14]))
#define H_sent (*(p_tcl_bind_list*)(transfer_funcs[15]))
/* 16 - 19 */
#define USERENTRY_FSTAT (*(struct user_entry_type *)(transfer_funcs[16]))
#define quiet_reject (*(int *)(transfer_funcs[17]))
#define raw_dcc_resend(a,b,c,d) (((int (*) (char *,char *,char *,char *))transfer_funcs[18])(a,b,c,d))

#else	/* MAKING_TRANSFER */

static int raw_dcc_resend(char *, char *, char *, char *);
static int raw_dcc_send(char *, char *, char *, char *);

#define TRANSFER_REGET_PACKETID 0xfeab

typedef struct {
  u_16bit_t packet_id;		/* Identification ID, should be equal
	 			   to TRANSFER_REGET_PACKETID		*/
  u_8bit_t  byte_order;		/* Byte ordering, see byte_order_test()	*/
  u_32bit_t byte_offset;	/* Number of bytes to skip relative to
				   the file beginning			*/
} transfer_reget;

typedef struct zarrf {
  char *dir;			/* Absolute dir if it starts with '*',
				   otherwise dcc dir.			*/
  char *file;
  char nick[NICKLEN];		/* Who queued this file			*/
  char to[NICKLEN];		/* Who will it be sent to		*/
  struct zarrf *next;
} fileq_t;

#endif				/* MAKING_TRANSFER */

#endif				/* _EGG_MOD_TRANSFER_TRANSFER_H */
