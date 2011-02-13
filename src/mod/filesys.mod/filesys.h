/*
 * filesysc.h -- part of filesys.mod
 *   header file for the filesys2 eggdrop module
 *
 * $Id: filesys.h,v 1.19 2011/02/13 14:19:33 simple Exp $
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

#ifndef _EGG_MOD_FILESYS_FILESYS_H
#define _EGG_MOD_FILESYS_FILESYS_H

#include "src/lang.h"
#include "transfer.mod/transfer.h"

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
static int do_dcc_send(int, char *, char *, char *, int);
static int files_reget(int, char *, char *, int);
static void files_setpwd(int, char *);
static int resolve_dir(char *, char *, char **, int);

#else
#define H_fil (*(p_tcl_hash_list *)(filesys_funcs[8]))
#endif /* MAKING_FILESYS */

#endif /* _EGG_MOD_FILESYS_FILESYS_H */
