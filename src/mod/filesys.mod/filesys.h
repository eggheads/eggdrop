/*
 * filesysc.h  - header file for the filesys2 eggdrop module
 */
/*
 * This file is part of the eggdrop source code.
 *
 * Copyright (C) 1997  Robey Pointer
 * Copyright (C) 1999  Eggheads
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

#ifndef _FILESYS_H_
#define _FILESYS_H_

#include "../../lang.h"
#include "../transfer.mod/transfer.h"

#ifdef MAKING_FILESYS
static int too_many_filers();
static int welcome_to_files(int);
static void add_file(char *, char *, char *);
static void incr_file_gots(char *);
static void remote_filereq(int, char *, char *);
static FILE *filedb_open(char *, int);
static void filedb_close(FILE *);
static void filedb_add(FILE *, char *, char *);
static void filedb_ls(FILE *, int, char *, int);
static void filedb_getowner(char *, char *, char **);
static void filedb_setowner(char *, char *, char *);
static void filedb_getdesc(char *, char *, char **);
static void filedb_setdesc(char *, char *, char *);
static int filedb_getgots(char *, char *);
static void filedb_setlink(char *, char *, char *);
static void filedb_getlink(char *, char *, char **);
static void filedb_getfiles(Tcl_Interp *, char *);
static void filedb_getdirs(Tcl_Interp *, char *);
static void filedb_change(char *, char *, int);
static void tell_file_stats(int, char *);
static int do_dcc_send(int, char *, char *);
static int files_get(int, char *, char *);
static void files_setpwd(int, char *);
static int resolve_dir(char *, char *, char **, int);
#else
#define H_fil (*(p_tcl_hash_list *)(filesys_funcs[8]))
#endif
#endif
