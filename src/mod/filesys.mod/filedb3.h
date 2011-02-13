/*
 * filedb3.h -- part of filesys.mod
 *   filedb header file
 *
 * Written by Fabian Knittel <fknittel@gmx.de>
 *
 * $Id: filedb3.h,v 1.25 2011/02/13 14:19:33 simple Exp $
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

#ifndef _EGG_MOD_FILESYS_FILEDB3_H
#define _EGG_MOD_FILESYS_FILEDB3_H

/* Top of each DB */
typedef struct {
  char version;                 /* DB version                   */
  time_t timestamp;             /* Last updated                 */
} filedb_top;

/* Header of each entry */
typedef struct {
  time_t uploaded;              /* Upload time                  */
  unsigned int size;            /* File size                    */
  unsigned short int stat;      /* Misc information             */
  unsigned short int gots;      /* Number of gets               */
  unsigned short int filename_len;      /* Length of filename buf       */
  unsigned short int desc_len;  /* Length of description buf    */
  unsigned short int sharelink_len;     /* Length of sharelink buf      */
  unsigned short int chan_len;  /* Length of channel name buf   */
  unsigned short int uploader_len;      /* Length of uploader buf       */
  unsigned short int flags_req_len;     /* Length of flags buf          */
  unsigned short int buffer_len;        /* Length of additional buffer  */
} filedb_header;

/* Structure used to pass data between lower level
 * and higher level functions.
 */
typedef struct {
  time_t uploaded;              /* Upload time                  */
  unsigned int size;            /* File size                    */
  unsigned short int stat;      /* Misc information             */
  unsigned short int gots;      /* Number of gets               */
  unsigned short int _type;     /* Type of entry (private)      */

  /* NOTE: These three are only valid during one db open/close. Entry
   *       movements often invalidate them too, so make sure you know
   *       what you're doing before using/relying on them.
   */
  long pos;                     /* Last position in the filedb  */
  unsigned short int dyn_len;   /* Length of dynamic data in DB */
  unsigned short int buf_len;   /* Length of additional buffer  */

  char *filename;               /* Filename                     */
  char *desc;                   /* Description                  */
  char *sharelink;              /* Share link. Points to remote
                                 * file on linked bot.          */
  char *chan;                   /* Channel name                 */
  char *uploader;               /* Uploader                     */
  char *flags_req;              /* Required flags               */
} filedb_entry;


/*
 *   Macros
 */

#define my_free(ptr)                                                    \
  if (ptr) {                                                            \
    nfree(ptr);                                                         \
    ptr = NULL;                                                         \
  }

/* Copy entry to target -- Uses dynamic memory allocation, which
 * means you'll eventually have to free the memory again. 'target'
 * will be overwritten.
 */
#define malloc_strcpy(target, entry)                                    \
do {                                                                    \
  if (entry) {                                                          \
    (target) = nrealloc((target), strlen(entry) + 1);                   \
    strcpy((target), (entry));                                          \
  } else                                                                \
    my_free(target);                                                    \
} while (0)

#define malloc_strcpy_nocheck(target, entry)                            \
do {                                                                    \
  (target) = nrealloc((target), strlen(entry) + 1);                     \
  strcpy((target), (entry));                                            \
} while (0)

/* Macro to calculate the total length of dynamic data. */
#define filedb_tot_dynspace(fdh) ((fdh).filename_len + (fdh).desc_len + \
        (fdh).chan_len + (fdh).uploader_len + (fdh).flags_req_len + \
        (fdh).sharelink_len)

#define filedb_zero_dynspace(fdh) {                                     \
        (fdh).filename_len      = 0;                                    \
        (fdh).desc_len          = 0;                                    \
        (fdh).chan_len          = 0;                                    \
        (fdh).uploader_len      = 0;                                    \
        (fdh).flags_req_len     = 0;                                    \
        (fdh).sharelink_len     = 0;                                    \
}

/* Memory debugging makros */
#define malloc_fdbe() _malloc_fdbe(__FILE__, __LINE__)
#define filedb_getfile(fdb, pos, get) _filedb_getfile(fdb, pos, get, __FILE__, __LINE__)
#define filedb_matchfile(fdb, pos, match) _filedb_matchfile(fdb, pos, match, __FILE__, __LINE__)
#define filedb_updatefile(fdb, pos, fdbe, update) _filedb_updatefile(fdb, pos, fdbe, update, __FILE__, __LINE__)
#define filedb_addfile(fdb, fdbe) _filedb_addfile(fdb, fdbe, __FILE__, __LINE__)
#define filedb_movefile(fdb, pos, fdbe) _filedb_movefile(fdb, pos, fdbe, __FILE__, __LINE__)


/*
 *  Constants
 */

#define FILEDB_VERSION1 0x0001
#define FILEDB_VERSION2 0x0002  /* DB version used for 1.3, 1.4         */
#define FILEDB_VERSION3 0x0003
#define FILEDB_NEWEST_VER FILEDB_VERSION3       /* Newest DB version    */

#define POS_NEW         0       /* Position which indicates that the
                                 * entry wants to be repositioned.      */

#define FILE_UNUSED     0x0001  /* Deleted entry.                       */
#define FILE_DIR        0x0002  /* It's actually a directory.           */
#define FILE_SHARE      0x0004  /* Can be shared on the botnet.         */
#define FILE_HIDDEN     0x0008  /* Hidden file.                         */
#define FILE_ISLINK     0x0010  /* The file is a link to another bot.   */

#define FILEDB_ESTDYN   50      /* Estimated dynamic length of an entry */

enum {
  GET_HEADER,                   /* Only save minimal data               */
  GET_FILENAME,                 /* Additionally save filename           */
  GET_FULL,                     /* Save all data                        */

  UPDATE_HEADER,                /* Only update header                   */
  UPDATE_SIZE,                  /* Update header, enforce new buf sizes */
  UPDATE_ALL,                   /* Update additional data too           */

  TYPE_NEW,                     /* New entry                            */
  TYPE_EXIST                    /* Existing entry                       */
};


/*
 *  filedb3.c prototypes
 */

static void free_fdbe(filedb_entry **);
static filedb_entry *_malloc_fdbe(char *, int);
static int filedb_readtop(FILE *, filedb_top *);
static int filedb_writetop(FILE *, filedb_top *);
static int filedb_delfile(FILE *, long);
static filedb_entry *filedb_findempty(FILE *, int);
static int _filedb_updatefile(FILE *, long, filedb_entry *, int, char *, int);
static int _filedb_movefile(FILE *, long, filedb_entry *, char *, int);
static int _filedb_addfile(FILE *, filedb_entry *, char *, int);
static filedb_entry *_filedb_getfile(FILE *, long, int, char *, int);
static filedb_entry *_filedb_matchfile(FILE *, long, char *, char *, int);
static filedb_entry *filedb_getentry(char *, char *);

#endif /* _EGG_MOD_FILESYS_FILEDB3_H */
