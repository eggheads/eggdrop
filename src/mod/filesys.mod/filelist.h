/*
 * filelist.h  - header file for filelist.c
 */
/*
 * This file is part of the eggdrop source code.
 * 
 * Copyright (C) 1999  Eggheads
 * Written by Fabian Knittel <fknittel@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef FILELIST_H
#define FILELIST_H

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

#endif /* FILELIST_H */
