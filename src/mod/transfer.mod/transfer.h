/* 
 * transfer.h -- part of transfer.mod
 * 
 * $Id: transfer.h,v 1.3 1999/12/15 02:33:00 guppy Exp $
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

#define DCCSEND_OK     0
#define DCCSEND_FULL   1	/* dcc table is full */
#define DCCSEND_NOSOCK 2	/* can't open a listening socket */
#define DCCSEND_BADFN  3	/* no such file */
#define MAX_FILENAME_LENGTH 40	/* max file lengh */

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
#define wild_match_file(a,b) (((int (*)(register unsigned char * m, register unsigned char * n))transfer_funcs[11])(a,b))
/* 12 - 15 */
#define wipe_tmp_filename(a,b) (((void (*) (char *,int))transfer_funcs[12])(a,b))
#define DCC_GET (*(struct dcc_table *)(transfer_funcs[13]))
#define H_rcvd (*(p_tcl_bind_list*)(transfer_funcs[14]))
#define H_sent (*(p_tcl_bind_list*)(transfer_funcs[15]))
/* 16 - 19 */
#define USERENTRY_FSTAT (*(struct user_entry_type *)(transfer_funcs[16]))
#define quiet_reject (*(int *)(transfer_funcs[17]))

#else
static int raw_dcc_send(char *, char *, char *, char *);

#endif				/* MAKING_TRANSFER */

#endif				/* _EGG_MOD_TRANSFER_TRANSFER_H */
