
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
static long findempty(FILE *);
static FILE *filedb_open(char *, int);
static void filedb_close(FILE *);
static void filedb_add(FILE *, char *, char *);
static void filedb_ls(FILE *, int, char *, int);
static void filedb_getowner(char *, char *, char *);
static void filedb_setowner(char *, char *, char *);
static void filedb_getdesc(char *, char *, char *);
static void filedb_setdesc(char *, char *, char *);
static int filedb_getgots(char *, char *);
static void filedb_setlink(char *, char *, char *);
static void filedb_getlink(char *, char *, char *);
static void filedb_getfiles(Tcl_Interp *, char *);
static void filedb_getdirs(Tcl_Interp *, char *);
static void filedb_change(char *, char *, int);
static void tell_file_stats(int, char *);
static int do_dcc_send(int, char *, char *);
static int files_get(int, char *, char *);
static void files_setpwd(int, char *);
static int resolve_dir(char *, char *, char *, int);
static void set_handle_uploads(struct userrec *bu, char *hand,
			       unsigned int ups, unsigned long upk);
static void set_handle_dnloads(struct userrec *bu, char *hand,
			       unsigned int dns, unsigned long dnk);
#else
#define H_fil (*(p_tcl_hash_list *)(filesys_funcs[8]))
#endif
#endif
