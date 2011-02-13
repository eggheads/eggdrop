/*
 * filelist.h -- part of filesys.mod
 *   header file for filelist.c
 *
 * Written by Fabian Knittel <fknittel@gmx.de>
 *
 * $Id: filelist.h,v 1.15 2011/02/13 14:19:33 simple Exp $
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

#ifndef _EGG_MOD_FILESYS_FILELIST_H
#define _EGG_MOD_FILESYS_FILELIST_H

typedef struct {
  char *fn;
  char *output;
} filelist_element_t;

typedef struct {
  int tot;
  filelist_element_t *elements;
} filelist_t;

/* Short-cut to access the last element in filelist */
#define FILELIST_LE(flist) ((flist)->elements[(flist)->tot - 1])

static filelist_t *filelist_new(void);
static void filelist_free(filelist_t *);
static void filelist_add(filelist_t *, char *);
static inline void filelist_addout(filelist_t *, char *);
static inline void filelist_idxshow(filelist_t *, int);
static void filelist_sort(filelist_t *);

#endif /* _EGG_MOD_FILESYS_FILELIST_H */
