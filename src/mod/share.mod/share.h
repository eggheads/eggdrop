/* 
 * share.h -- part of share.mod
 * 
 * $Id: share.h,v 1.1 2000/01/22 23:04:04 fabian Exp $
 */
/* 
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

#ifndef _EGG_MOD_SHARE_SHARE_H
#define _EGG_MOD_SHARE_SHARE_H

typedef struct {
  char	 *feature;		/* Name of the feature			*/
  long	  flag;			/* Flag representing the feature	*/
  int	(*ask_func)(int);	/* Pointer to the function that tells
				   us wether the feature should be
				   considered as on.			*/
} uff_table_t;

#ifndef MAKING_SHARE
/* 4 - 7 */
#define finish_share ((void (*) (int))share_funcs[4])
#define dump_resync ((void (*) (int))share_funcs[5])
#define uff_addtable ((void (*) (uff_table_t *))share_funcs[6])
#define uff_deltable ((void (*) (uff_table_t *))share_funcs[7])
/* 8 - 11 */
#endif				/* !MAKING_SHARE */

#endif				/* _EGG_MOD_SHARE_SHARE_H */
