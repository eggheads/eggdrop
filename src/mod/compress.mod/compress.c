/* 
 * compress.c -- part of compress.mod
 *   uses the compression library libz to compress and uncompress the
 *   userfiles during the sharing process
 * 
 * Written by Fabian Knittel <fknittel@gmx.de>
 * 
 * $Id: compress.c,v 1.1 2000/03/01 17:54:37 fabian Exp $
 */
/* 
 * Copyright (C) 2000  Eggheads
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

#define MODULE_NAME "compress"
#define MAKING_COMPRESS

#include <string.h>
#include <errno.h>
#include <zlib.h>

#include "../module.h"
#include "../share.mod/share.h"
#include "compress.h"

#define BUFLEN	512


static Function *global = NULL,
		*share_funcs = NULL;

static int compressed_files;		/* Number of files compressed.      */
static int uncompressed_files;		/* Number of files uncompressed.    */
static int share_compressed;		/* Compress userfiles when sharing? */
static int userfile_comp_mode;		/* Compression used for userfiles.  */


static int uncompress_to_file(char *f_src, char *f_target);
static int compress_to_file(char *f_src, char *f_target, char *mode);
static int compress_file(char *filename, char *mode);
static int uncompress_file(char *filename);
static void adjust_mode_num(int *mode);

#include "tclcompress.c"


/*
 *    General compression / uncompression functions
 */

/* Uncompresses a file `f_src' and saved it as `f_target'.
 */
static int uncompress_to_file(char *f_src, char *f_target)
{
  char buf[BUFLEN];
  int len;
  FILE *fin, *fout;

  fin = gzopen(f_src, "rb");
  if (!fin) {
    putlog(LOG_MISC, "*", "Failed to uncompress file `%s': gzopen failed.",
	   f_src);
    return COMPF_ERROR;
  }
  fout = fopen(f_target, "wb");
  if (!fout) {
    putlog(LOG_MISC, "*", "Failed to uncompress file `%s': open failed: %s",
	   f_src, strerror(errno));
    return COMPF_ERROR;
  }

  while (1) {
    len = gzread(fin, buf, sizeof(buf));
    if (len < 0) {
      putlog(LOG_MISC, "*", "Failed to uncompress file `%s': gzread failed.",
	     f_src);
      return COMPF_ERROR;
    }
    if (!len)
      break;
    if ((int) fwrite(buf, 1, (unsigned int) len, fout) != len) {
      putlog(LOG_MISC, "*", "Failed to uncompress file `%s': fwrite failed: %s",
	     f_src, strerror(errno));
      return COMPF_ERROR;
    }
  }
  if (fclose(fout)) {
    putlog(LOG_MISC, "*", "Failed to uncompress file `%s': fclose failed: %s",
	   f_src, strerror(errno));
    return COMPF_ERROR;
  }
  if (gzclose(fin) != Z_OK) {
    putlog(LOG_MISC, "*", "Failed to uncompress file `%s': gzclose failed.",
	   f_src);
    return COMPF_ERROR;
  }
  uncompressed_files++;
  return COMPF_SUCCESS;
}

/* Compresses a file `f_src' and saved it as `f_target'.
 */
static int compress_to_file(char *f_src, char *f_target, char *mode)
{
  char buf[BUFLEN];
  int len;
  FILE *fin, *fout;

  fin = fopen(f_src, "rb");
  if (!fin) {
    putlog(LOG_MISC, "*", "Failed to compress file `%s': open failed: %s",
	   f_src, strerror(errno));
    return COMPF_ERROR;
  }
  fout = gzopen(f_target, mode);
  if (!fout) {
    putlog(LOG_MISC, "*", "Failed to compress file `%s': gzopen failed.",
	   f_src);
    return COMPF_ERROR;
  }

  while (1) {
    len = fread(buf, 1, sizeof(buf), fin);
    if (ferror(fin)) {
      putlog(LOG_MISC, "*", "Failed to compress file `%s': fread failed: %s",
	     f_src, strerror(errno));
      return COMPF_ERROR;
    }
    if (!len)
      break;
    if (gzwrite(fout, buf, (unsigned int) len) != len) {
      putlog(LOG_MISC, "*", "Failed to compress file `%s': gzwrite failed.",
	     f_src);
      return COMPF_ERROR;
    }
  }
  fclose(fin);
  if (gzclose(fout) != Z_OK) {
    putlog(LOG_MISC, "*", "Failed to compress file `%s': gzclose failed.",
	   f_src);
    return COMPF_ERROR;
  }
  compressed_files++;
  return COMPF_SUCCESS;
}

/* Compresses a file `filename' and saves it as `filename'.
 */
static int compress_file(char *filename, char *mode)
{
  char *temp_fn, randstr[5];
  int   ret;

  /* Create temporary filename. */
  temp_fn = nmalloc(strlen(filename) + 5);
  make_rand_str(randstr, 4);
  strcpy(temp_fn, filename);
  strcat(temp_fn, randstr);

  /* Compress file. */
  ret = compress_to_file(filename, temp_fn, mode);

  /* Overwrite old file with compressed version.  Only do so
   * if the compression routine succeeded.
   */
  if (ret == COMPF_SUCCESS)
    movefile(temp_fn, filename);

  nfree(temp_fn);
  return COMPF_SUCCESS;
}

/* Uncompresses a file `filename' and saves it as `filename'.
 */
static int uncompress_file(char *filename)
{
  char *temp_fn, randstr[5];
  int   ret;

  /* Create temporary filename. */
  temp_fn = nmalloc(strlen(filename) + 5);
  make_rand_str(randstr, 4);
  strcpy(temp_fn, filename);
  strcat(temp_fn, randstr);

  /* Uncompress file. */
  ret = uncompress_to_file(filename, temp_fn);

  /* Overwrite old file with uncompressed version.  Only do so
   * if the uncompression routine succeeded.
   */
  if (ret == COMPF_SUCCESS)
    movefile(temp_fn, filename);

  nfree(temp_fn);
  return COMPF_SUCCESS;
}

/* Enforce limits.
 */
static void adjust_mode_num(int *mode)
{
  if (*mode > 9)
    *mode = 9;
  else if (*mode < 0)
    *mode = 0;
}


/*
 *    Userfile specific compression / decompression functions
 */

static int compress_userfile(char *filename)
{
  char	 mode[5];

  adjust_mode_num(&userfile_comp_mode);
  sprintf(mode, "wb%d", userfile_comp_mode);
  return compress_file(filename, mode);
}

static int uncompress_userfile(char *filename)
{
  return uncompress_file(filename);
}


/*
 *    Userfile feature releated functions
 */

static int uff_ask_compress(int idx)
{
  if (share_compressed)
    return 1;
  else
    return 0;
}

static uff_table_t compress_uff_table[] = {
  {"compress",		STAT_UFF_COMPRESS,	uff_ask_compress},
  {NULL,		0,			NULL}
};

/* 
 *    Compress module related code
 */

static tcl_ints my_tcl_ints[] = {
  {"share-compressed",  	&share_compressed},
  {"share-compress-level",	&userfile_comp_mode},
  {NULL,                	NULL}
};

static int compress_expmem(void)
{
  return 0;
}

static int compress_report(int idx, int details)
{
  if (details)
    dprintf(idx, "    Compressed %d file(s), uncompressed %d file(s).\n",
	    compressed_files, uncompressed_files);
  return 0;
}

static char *compress_close()
{
  Context;
  rem_help_reference("compress.help");
  rem_tcl_commands(my_tcl_cmds);
  rem_tcl_ints(my_tcl_ints);
  uff_deltable(compress_uff_table);

  Context;
  module_undepend(MODULE_NAME);
  Context;
  return NULL;
}

char *compress_start();

static Function compress_table[] =
{
  /* 0 - 3 */
  (Function) compress_start,
  (Function) compress_close,
  (Function) compress_expmem,
  (Function) compress_report,
  /* 4 - 7 */
  (Function) compress_to_file,
  (Function) compress_file,
  (Function) uncompress_to_file,
  (Function) uncompress_file,
  /* 8 - 11 */
  (Function) compress_userfile,
  (Function) uncompress_userfile,
};

char *compress_start(Function *global_funcs)
{
  global = global_funcs;
  Context;
  compressed_files	= 0;
  uncompressed_files	= 0;
  share_compressed	= 0;
  userfile_comp_mode	= 9;

  Context;
  module_register(MODULE_NAME, compress_table, 1, 1);
  share_funcs = module_depend(MODULE_NAME, "share", 2, 2);
  if (!share_funcs) {
    module_undepend(MODULE_NAME);
    return "You need share module version 2.2 to use the compress module.";
  }

  Context;
  uff_addtable(compress_uff_table);
  add_tcl_ints(my_tcl_ints);
  add_tcl_commands(my_tcl_cmds);
  add_help_reference("compress.help");

  Context;
  return NULL;
}
