/* 
 * This file is part of the eggdrop source code copyright (c) 1997 Robey
 * Pointer and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called COPYING
 * that was distributed with this code.
 */

#ifndef _H_FILES
#define _H_FILES

/* language file additions for the file area */
#define FILES_CONVERT    get_language(0x300)
#define FILES_NOUPDATE   get_language(0x301)
#define FILES_NOCONVERT  get_language(0x302)
#define FILES_LSHEAD1    get_language(0x303)
#define FILES_LSHEAD2    get_language(0x304)
#define FILES_NOFILES    get_language(0x305)
#define FILES_NOMATCH    get_language(0x306)
#define FILES_DIRDNE     get_language(0x307)
#define FILES_FILEDNE    get_language(0x308)
#define FILES_NOSHARE    get_language(0x309)
#define FILES_REMOTE     get_language(0x30a)
#define FILES_SENDERR    get_language(0x30b)
#define FILES_SENDING    get_language(0x30c)
#define FILES_REMOTEREQ  get_language(0x30d)
#define FILES_BROKEN     get_language(0x30e)
#define FILES_INVPATH    get_language(0x30f)
#define FILES_CURDIR     get_language(0x310)
#define FILES_NEWCURDIR  get_language(0x311)
#define FILES_NOSUCHDIR  get_language(0x312)
#define FILES_ILLDIR     get_language(0x313)
#define FILES_BADNICK    get_language(0x314)
#define FILES_NOTAVAIL   get_language(0x315)
#define FILES_REQUESTED  get_language(0x316)
#define FILES_NORMAL     get_language(0x317)
#define FILES_CHGLINK    get_language(0x318)
#define FILES_NOTOWNER   get_language(0x319)
#define FILES_CREADIR    get_language(0x31a)
#define FILES_REQACCESS  get_language(0x31b)
#define FILES_CHGACCESS  get_language(0x31c)
#define FILES_CHGNACCESS get_language(0x31d)
#define FILES_REMDIR     get_language(0x31e)
#define FILES_ILLSOURCE  get_language(0x31f)
#define FILES_ILLDEST    get_language(0x320)
#define FILES_STUPID     get_language(0x321)
#define FILES_EXISTDIR   get_language(0x322)
#define FILES_SKIPSTUPID get_language(0x323)
#define FILES_DEST       get_language(0x324)
#define FILES_COPY       get_language(0x325)
#define FILES_COPIED     get_language(0x326)
#define FILES_MOVE       get_language(0x327)
#define FILES_MOVED      get_language(0x328)
#define FILES_CANTWRITE  get_language(0x329)
#define FILES_REQUIRES   get_language(0x32a)
#define FILES_HID        get_language(0x32b)
#define FILES_UNHID      get_language(0x32c)
#define FILES_SHARED     get_language(0x32d)
#define FILES_UNSHARED   get_language(0x32e)
#define FILES_ADDLINK    get_language(0x32f)
#define FILES_CHANGED    get_language(0x330)
#define FILES_BLANKED    get_language(0x331)
#define FILES_ERASED     get_language(0x332)
#define FILES_WELCOME    get_language(0x33a)
#define FILES_WELCOME1   get_language(0x33b)

/* structure for file database (per directory) */
struct filler_old {
  char xxx[1 + 61 + 301 + 10 + 11 + 61];
  unsigned short int uuu[2];
  time_t ttt[2];
  unsigned int iii[2];
};

typedef struct {
  char version;
  unsigned short int stat;	/* misc */
  time_t timestamp;		/* last time this db was updated */
  char filename[61];
  char desc[301];		/* should be plenty */
  char uploader[10];		/* where this file came from */
  unsigned char flags_req[11];	/* access flags required */
  time_t uploaded;		/* time it was uploaded */
  unsigned int size;		/* file length */
  unsigned short int gots;	/* times the file was downloaded */
  char sharelink[61];		/* points to where? */
  char unused[512 - sizeof(struct filler_old)];
} filedb_old;

struct filler {
  char xxx[1 + 61 + 186 + 81 + 33 + 22 + 61];
  unsigned short int uuu[2];
  time_t ttt[2];
  unsigned int iii[1];
};

typedef struct {
  char version;
  unsigned short int stat;	/* misc */
  time_t timestamp;		/* last time this db was updated */
  char filename[61];
  char desc[186];		/* should be plenty - shrink it, we need
				 * the  space :) */
  char chname[81];		/* channel for chan spec stuff */
  char uploader[33];		/* where this file came from */
  char flags_req[22];		/* access flags required */
  time_t uploaded;		/* time it was uploaded */
  unsigned int size;		/* file length */
  unsigned short int gots;	/* times the file was downloaded */
  char sharelink[61];		/* points to where? */
  char unused[512 - sizeof(struct filler)];
} filedb;

#define FILEVERSION_OLD    0x01
#define FILEVERSION        0x2

#define FILE_UNUSED     0x0001	/* (deleted entry) */
#define FILE_DIR        0x0002	/* it's actually a directory */
#define FILE_SHARE      0x0004	/* can be shared on the botnet */
#define FILE_HIDDEN     0x0008	/* hidden file */

/* prototypes */
static int findmatch(FILE *, char *, long *, filedb *);
#endif
