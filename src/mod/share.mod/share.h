/*
 * share.h -- part of share.mod
 *
 * $Id: share.h,v 1.14 2011/02/13 14:19:34 simple Exp $
 */
/*
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

#ifndef _EGG_MOD_SHARE_SHARE_H
#define _EGG_MOD_SHARE_SHARE_H

#define UFF_OVERRIDE  0x000001  /* Override existing bot entries    */
#define UFF_INVITE    0x000002  /* Send invites in user file        */
#define UFF_EXEMPT    0x000004  /* Send exempts in user file        */
/* Currently reserved flags for other modules:
 *      UFF_COMPRESS    0x000008           Compress the user file
 *      UFF_ENCRYPT     0x000010           Encrypt the user file
 */

/* Currently used priorities:
 *        0             UFF_OVERRIDE
 *        0             UFF_INVITE
 *        0             UFF_EXEMPT
 *       90             UFF_ENCRYPT
 *      100             UFF_COMPRESS
 */

typedef struct {
  char *feature;            /* Name of the feature                           */
  int flag;                 /* Flag representing the feature                 */
  int (*ask_func) (int);    /* Pointer to the function that tells us wether
                             * the feature should be considered as on.       */
  int priority;             /* Priority with which this entry gets called.   */
  int (*snd) (int, char *); /* Called before sending. Handled according to
                             * `priority'. */
  int (*rcv) (int, char *); /* Called on receive. Handled according to
                             * `priority'.                                   */
} uff_table_t;

#ifndef MAKING_SHARE
/* 4 - 7 */
#define finish_share ((void (*) (int))share_funcs[4])
#define dump_resync ((void (*) (int))share_funcs[5])
#define uff_addtable ((void (*) (uff_table_t *))share_funcs[6])
#define uff_deltable ((void (*) (uff_table_t *))share_funcs[7])
/* 8 - 11 */
#endif /* !MAKING_SHARE */

#endif /* _EGG_MOD_SHARE_SHARE_H */
