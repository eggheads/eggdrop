/*
 * filedb3.h  - filedb header file
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


#ifndef FILEDB3_H
#define FILEDB3_H

#include <time.h>	/* for time_t */

/* Top of each DB */
typedef struct {
  char version;				/* db version			*/
  time_t timestamp;			/* last updated			*/
} filedb_top;

/* Header of each entry */
typedef struct {
  time_t uploaded;			/* upload time			*/
  unsigned int size;			/* file size			*/
  unsigned short int stat;		/* misc information		*/
  unsigned short int gots;		/* number of gets		*/
  unsigned short int filename_len; 	/* length of filename buf	*/
  unsigned short int desc_len;	 	/* length of description buf	*/
  unsigned short int sharelink_len;	/* length of sharelink buf	*/
  unsigned short int chan_len;		/* length of channel name buf	*/
  unsigned short int uploader_len;	/* length of uploader buf	*/
  unsigned short int flags_req_len;	/* length of flags buf		*/
  unsigned short int buffer_len;	/* length of additional buffer	*/
} filedb_header;

/* Structure used to pass data between lower level
 * and higher level functions. */
typedef struct {
  time_t uploaded;			/* upload time			*/
  unsigned int size;			/* file size			*/
  unsigned short int stat;		/* misc information		*/
  unsigned short int gots;		/* number of gets		*/
  unsigned short int _type;		/* type of entry (private)	*/

  /* NOTE: These two are only valid during one db open/close */
  long pos;				/* last position in the filedb	*/
  unsigned short int dyn_len;		/* length of dynamic data in DB	*/
  unsigned short int buf_len;		/* length of additional buffer	*/

  char *filename;			/* filename			*/
  char *desc;				/* description			*/
  char *sharelink;			/* share link			*/
  char *chan;				/* channel name			*/
  char *uploader;			/* uploader			*/
  char *flags_req;			/* required flags		*/
} filedb_entry;


/*
 *   Macros
 */

/* Copy entry to target -- Uses dynamic memory allocation, which
 * means you'll eventually have to free the memory again. 'target'
 * has to be defined, either NULL or with allocated memory.
 */
#define malloc_strcpy(target, entry)					\
{									\
  if (entry) {								\
    target = nrealloc(target, strlen(entry) + 1);			\
    strcpy(target, entry);						\
  } else if (target) {							\
    nfree(target);							\
    target = NULL;							\
  }									\
}

#define filedb_tot_dynspace(fdh) ((fdh).filename_len + (fdh).desc_len +	\
	(fdh).chan_len + (fdh).uploader_len + (fdh).flags_req_len)

#define filedb_zero_dynspace(fdh) {					\
	(fdh).filename_len  = 0;					\
	(fdh).desc_len	    = 0;					\
	(fdh).chan_len	    = 0;					\
	(fdh).uploader_len  = 0;					\
	(fdh).flags_req_len = 0;					\
}

/* Free several memory blocks in one go. */
#define nfree2(x1, x2) { nfree(x1); nfree(x2); }
#define nfree3(x1, x2, x3) { nfree(x1); nfree(x2); nfree(x3); }
#define nfree4(x1, x2, x3, x4) { nfree(x1); nfree(x2); nfree(x3); nfree(x4); }
#define nfree5(x1, x2, x3, x4, x5) { nfree(x1); nfree(x2); nfree(x3); \
    				     nfree(x4); nfree(x5); }

/* Memory debugging makros */
#define malloc_fdbe() _malloc_fdbe(__FILE__, __LINE__)
#define filedb_getfile(fdb, pos, get) _filedb_getfile(fdb, pos, get, __FILE__, __LINE__)
#define filedb_matchfile(fdb, pos, match) _filedb_matchfile(fdb, pos, match, __FILE__, __LINE__)
#define filedb_updatefile(fdb, pos, fdbe, update) _filedb_updatefile(fdb, pos, fdbe, update, __FILE__, __LINE__)
#define filedb_addfile(fdb, pos, fdbe) _filedb_addfile(fdb, pos, fdbe, __FILE__, __LINE__)


/*
 *  Constants
 */

#define FILEDB_VERSION1	0x0001
#define FILEDB_VERSION2	0x0002
#define FILEDB_VERSION3	0x0003	/* Newest DB version			*/

#define POS_NEW		0	/* Position which indicates that the
				   entry wants to be repositioned.	*/

#define FILE_UNUSED	0x0001	/* deleted entry */
#define FILE_DIR	0x0002	/* it's actually a directory */
#define FILE_SHARE	0x0004	/* can be shared on the botnet */
#define FILE_HIDDEN	0x0008	/* hidden file */

#define FILEDB_ESTDYN	50	/* estimated dynamic length of an entry	*/

enum {
  GET_HEADER,			/* Only save minimal data		*/
  GET_FILENAME,			/* Additionally save filename		*/
  GET_FULL,			/* Save all data			*/

  UPDATE_HEADER,		/* Only update header			*/
  UPDATE_SIZE,			/* Update header, enforce new buf sizes	*/
  UPDATE_ALL,			/* Update additional data too		*/

  TYPE_NEW,			/* New entry				*/
  TYPE_EXIST			/* Existing entry			*/
};


/*
 *  filedb3.c prototypes
 */

static void free_fdbe(filedb_entry *);
static filedb_entry *_malloc_fdbe(char *, int);
static int filedb_readtop(FILE *, filedb_top *);
static int filedb_writetop(FILE *, filedb_top *);
static int filedb_delfile(FILE *, long);
static filedb_entry *filedb_findempty(FILE *, int);
static int _filedb_updatefile(FILE *, long, filedb_entry *, int, char *, int);
static int _filedb_addfile(FILE *, long, filedb_entry *, char *, int);
static filedb_entry *_filedb_getfile(FILE *, long, int, char *, int);
static filedb_entry *_filedb_matchfile(FILE *, long, char *, char *, int);
static filedb_entry *filedb_getentry(char *, char *);

#endif 	/* FILEDB3_H */
