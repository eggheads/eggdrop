/*
 * dbcompat.h - this header file contains old db formats which are
 *              needed or converting old dbs to the new format.
 */
/* 
 * This file is part of the eggdrop source code copyright (c) 1997 Robey
 * Pointer and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called COPYING
 * that was distributed with this code.
 */

#ifndef DBCOMPAT_H
#define DBCOMPAT_H

/*
 *    DB entry structures for v1 and v2
 */

/* structure for file database (per directory) */
struct filler1 {
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
  char unused[512 - sizeof(struct filler1)];
} filedb1;

struct filler2 {
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
  char unused[512 - sizeof(struct filler2)];
} filedb2;


/*
 *    Prototypes
 */

static int convert_old_db(FILE **fdb, char *s);
static int convert_old_files(char *npath, char *s);

#endif	/* DBCOMPAT.H */
