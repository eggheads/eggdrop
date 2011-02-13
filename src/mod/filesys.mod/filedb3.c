/*
 * filedb3.c -- part of filesys.mod
 *   low level functions for file db handling
 *
 * Rewritten by Fabian Knittel <fknittel@gmx.de>
 *
 * $Id: filedb3.c,v 1.36 2011/02/13 14:19:33 simple Exp $
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

/*  filedb structure:
 *
 *  +---------------+                           _
 *  | filedb_top    |                           _|  DB header
 *  |---------------|
 *  .               .
 *  .               .
 *  | ...           |
 *  |---------------|     _                     _
 *  | filedb_header |     _|  Static length      |
 *  |- --- - --- - -|     _                      |
 *  | filename      |      |                     |
 *  |- - - - - - - -|      |                     |
 *  | description   |      |                     |
 *  |- - - - - - - -|      |  Dynamic length     |  Complete entry
 *  | channel name  |      |                     |
 *  |- - - - - - - -|      |                     |
 *  | uploader      |      |                     |
 *  |- - - - - - - -|      |                     |
 *  | flags_req     |      |                     |
 *  |- - - - - - - -|      |                     |
 *  | share link    |      |                     |
 *  |- - - - - - - -|      |                     |
 *  | buffer        |     _|                    _|
 *  |---------------|
 *  | ...           |
 *  .               .
 *  .               .
 *
 *  To know how long the complete entry is, you need to read out the
 *  header first. This concept allows us to have unlimited filename
 *  lengths, unlimited description lengths, etc.
 *
 *  Buffer is an area which doesn't contain any information and which
 *  is just skipped. It only serves as a placeholder to allow entries
 *  which shrink in size to maintain their position without messing up
 *  the position of all following entries.
 */

/* Number of databases opened simultaneously. More than 2 is
 * be unnormal.
 */
static int count = 0;


/*
 *   Memory management helper functions
 */

/* Frees a filedb entry and all it's elements.
 */
static void free_fdbe(filedb_entry ** fdbe)
{
  if (!fdbe || !*fdbe)
    return;
  if ((*fdbe)->filename)
    my_free((*fdbe)->filename);
  if ((*fdbe)->desc)
    my_free((*fdbe)->desc);
  if ((*fdbe)->sharelink)
    my_free((*fdbe)->sharelink);
  if ((*fdbe)->chan)
    my_free((*fdbe)->chan);
  if ((*fdbe)->uploader)
    my_free((*fdbe)->uploader);
  if ((*fdbe)->flags_req)
    my_free((*fdbe)->flags_req);
  my_free(*fdbe);
}

/* Allocates and initialises a filedb entry
 */
static filedb_entry *_malloc_fdbe(char *file, int line)
{
  filedb_entry *fdbe = NULL;

#ifdef DEBUG_MEM
  /* This is a hack to access the nmalloc function with
   * special file and line information
   */
  fdbe = (((void *(*)())global[0])(sizeof(filedb_entry),MODULE_NAME,file,line));
#else
  fdbe = nmalloc(sizeof(filedb_entry));
#endif
  egg_bzero(fdbe, sizeof(filedb_entry));

  /* Mark as new, will be overwritten if necessary. */
  fdbe->_type = TYPE_NEW;
  return fdbe;
}


/*
 *  File locking
 */

/* Locks the file, using fcntl. Execution is locked until we
 * have exclusive access to it.
 */
static void lockfile(FILE *fdb)
{
  struct flock fl;

  fl.l_type = F_WRLCK;
  fl.l_start = 0;
  fl.l_whence = SEEK_SET;
  fl.l_len = 0;
  fcntl(fileno(fdb), F_SETLKW, &fl);
}

/* Unlocks the file using fcntl.
 */
static void unlockfile(FILE *f)
{
  struct flock fl;

  fl.l_type = F_UNLCK;
  fl.l_start = 0;
  fl.l_whence = SEEK_SET;
  fl.l_len = 0;
  fcntl(fileno(f), F_SETLKW, &fl);
}


/*
 *   filedb functions
 */

/* Copies the DB header to fdbt or just positions the file
 * position pointer onto the first entry after the db header.
 */
static int filedb_readtop(FILE *fdb, filedb_top *fdbt)
{
  if (fdbt) {
    /* Read header */
    fseek(fdb, 0L, SEEK_SET);
    if (feof(fdb))
      return 0;
    fread(fdbt, 1, sizeof(filedb_top), fdb);
  } else
    fseek(fdb, sizeof(filedb_top), SEEK_SET);
  return 1;
}

/* Writes the DB header to the top of the filedb.
 */
static int filedb_writetop(FILE *fdb, filedb_top *fdbt)
{
  fseek(fdb, 0L, SEEK_SET);
  fwrite(fdbt, 1, sizeof(filedb_top), fdb);
  return 1;
}

/* Deletes the entry at 'pos'. It just adjusts the header to
 * mark it as 'unused' and to assign all dynamic space to
 * the buffer.
 */
static int filedb_delfile(FILE *fdb, long pos)
{
  filedb_header fdh;

  fseek(fdb, pos, SEEK_SET);    /* Go to start of entry */
  if (feof(fdb))
    return 0;
  fread(&fdh, 1, sizeof(filedb_header), fdb);   /* Read header          */
  fdh.stat = FILE_UNUSED;

  /* Assign all available space to buffer. Simplifies
   * space calculation later on.
   */
  fdh.buffer_len += filedb_tot_dynspace(fdh);
  filedb_zero_dynspace(fdh);

  fseek(fdb, pos, SEEK_SET);    /* Go back to start     */
  fwrite(&fdh, 1, sizeof(filedb_header), fdb);  /* Write new header     */
  return 1;
}

/* Searches for a suitable place to write an entry which uses tot
 * bytes for dynamic data.
 *
 *  * If there is no such existing entry, it just points to the
 *    end of the DB.
 *  * If it finds an empty entry and it has enough space to fit
 *    in another entry, we split it up and only use the space we
 *    really need.
 *
 * Note: We can assume that empty entries' dyn_lengths are zero.
 *       Therefore we only need to check buf_len.
 */
static filedb_entry *filedb_findempty(FILE *fdb, int tot)
{
  filedb_entry *fdbe;

  filedb_readtop(fdb, NULL);
  fdbe = filedb_getfile(fdb, ftell(fdb), GET_HEADER);
  while (fdbe) {
    /* Found an existing, empty entry? */
    if ((fdbe->stat & FILE_UNUSED) && (fdbe->buf_len >= tot)) {
      /* Do we have enough space to split up the entry to form
       * another empty entry? That way we would use the space
       * more efficiently.
       */
      if (fdbe->buf_len > (tot + sizeof(filedb_header) + FILEDB_ESTDYN)) {
        filedb_entry *fdbe_oe;

        /* Create new entry containing the additional space */
        fdbe_oe = malloc_fdbe();
        fdbe_oe->stat = FILE_UNUSED;
        fdbe_oe->pos = fdbe->pos + sizeof(filedb_header) + tot;
        fdbe_oe->buf_len = fdbe->buf_len - tot - sizeof(filedb_header);
        filedb_movefile(fdb, fdbe_oe->pos, fdbe_oe);
        free_fdbe(&fdbe_oe);

        /* Cut down buf_len of entry as the rest is now used in the new
         * entry.
         */
        fdbe->buf_len = tot;
      }
      return fdbe;
    }
    free_fdbe(&fdbe);
    fdbe = filedb_getfile(fdb, ftell(fdb), GET_HEADER);
  }

  /* No existing entries, so create new entry at end of DB instead. */
  fdbe = malloc_fdbe();
  fseek(fdb, 0L, SEEK_END);
  fdbe->pos = ftell(fdb);
  return fdbe;
}

/* Updates or creates entries and information in the filedb.
 *
 *   * If the new entry is the same size or smaller than the original
 *     one, we use the old position and just adjust the buffer length
 *     apropriately.
 *   * If the entry is completely _new_ or if the entry's new size is
 *     _bigger_ than the old one, we search for a new position which
 *     suits our needs.
 *
 * Note that the available space also includes the buffer.
 *
 * The file pointer will _always_ position directly after the updated
 * entry.
 */
static int _filedb_updatefile(FILE *fdb, long pos, filedb_entry *fdbe,
                              int update, char *file, int line)
{
  filedb_header fdh;
  int reposition = 0;
  int ndyntot, odyntot, nbuftot, obuftot;

  egg_bzero(&fdh, sizeof(filedb_header));
  fdh.uploaded = fdbe->uploaded;
  fdh.size = fdbe->size;
  fdh.stat = fdbe->stat;
  fdh.gots = fdbe->gots;

  /* Only add the buffer length if the buffer is not empty. Otherwise it
   * would result in lots of 1 byte entries which actually don't contain
   * any data.
   */
  if (fdbe->filename)
    fdh.filename_len = strlen(fdbe->filename) + 1;
  if (fdbe->desc)
    fdh.desc_len = strlen(fdbe->desc) + 1;
  if (fdbe->chan)
    fdh.chan_len = strlen(fdbe->chan) + 1;
  if (fdbe->uploader)
    fdh.uploader_len = strlen(fdbe->uploader) + 1;
  if (fdbe->flags_req)
    fdh.flags_req_len = strlen(fdbe->flags_req) + 1;
  if (fdbe->sharelink)
    fdh.sharelink_len = strlen(fdbe->sharelink) + 1;

  odyntot = fdbe->dyn_len;      /* Old length of dynamic data   */
  obuftot = fdbe->buf_len;      /* Old length of spare space    */
  ndyntot = filedb_tot_dynspace(fdh);   /* New length of dynamic data   */
  nbuftot = obuftot;

  if (fdbe->_type == TYPE_EXIST) {
    /* If we only update the header, we don't need to worry about
     * sizes and just use the old place (i.e. the place pointed
     * to by pos).
     */
    if (update < UPDATE_ALL) {
      /* Unless forced to it, we ignore new buffer sizes if we do not
       * run in UPDATE_ALL mode.
       */
      if (update != UPDATE_SIZE) {
        ndyntot = odyntot;
        nbuftot = obuftot;
      }
    } else {
      /* If we have a given/preferred position */
      if ((pos != POS_NEW) &&
          /* and if our new size is smaller than the old size, we
           * just adjust the buffer length and still use the same
           * position
           */
          (ndyntot <= (odyntot + obuftot))) {
        nbuftot = (odyntot + obuftot) - ndyntot;
      } else {
        /* If we have an existing position, but the new entry doesn't
         * fit into it's old home, we need to delete it before
         * repositioning.
         */
        if (pos != POS_NEW)
          filedb_delfile(fdb, pos);
        reposition = 1;
      }
    }
  } else {
    fdbe->_type = TYPE_EXIST;   /* Update type                  */
    reposition = 1;
  }

  /* Search for a new home */
  if (reposition) {
    filedb_entry *n_fdbe;

    n_fdbe = filedb_findempty(fdb, filedb_tot_dynspace(fdh));
    fdbe->pos = pos = n_fdbe->pos;
    /* If we create a new entry (instead of using an existing one),
     * buf_len is zero
     */
    if (n_fdbe->buf_len > 0)
      /* Note: empty entries have dyn_len set to zero, so we only
       *       need to consider buf_len.
       */
      nbuftot = n_fdbe->buf_len - ndyntot;
    else
      nbuftot = 0;
    free_fdbe(&n_fdbe);
  }

  /* Set length of dynamic data and buffer */
  fdbe->dyn_len = ndyntot;
  fdbe->buf_len = fdh.buffer_len = nbuftot;

  /* Write header */
  fseek(fdb, pos, SEEK_SET);
  fwrite(&fdh, 1, sizeof(filedb_header), fdb);
  /* Write dynamic data */
  if (update == UPDATE_ALL) {
    if (fdbe->filename)
      fwrite(fdbe->filename, 1, fdh.filename_len, fdb);
    if (fdbe->desc)
      fwrite(fdbe->desc, 1, fdh.desc_len, fdb);
    if (fdbe->chan)
      fwrite(fdbe->chan, 1, fdh.chan_len, fdb);
    if (fdbe->uploader)
      fwrite(fdbe->uploader, 1, fdh.uploader_len, fdb);
    if (fdbe->flags_req)
      fwrite(fdbe->flags_req, 1, fdh.flags_req_len, fdb);
    if (fdbe->sharelink)
      fwrite(fdbe->sharelink, 1, fdh.sharelink_len, fdb);
  } else
    fseek(fdb, ndyntot, SEEK_CUR);      /* Skip over dynamic data */
  fseek(fdb, nbuftot, SEEK_CUR);        /* Skip over buffer       */
  return 0;
}

/* Moves an existing file entry, saved in fdbe to a new position.
 */
static int _filedb_movefile(FILE *fdb, long pos, filedb_entry *fdbe,
                            char *file, int line)
{
  fdbe->_type = TYPE_EXIST;
  _filedb_updatefile(fdb, pos, fdbe, UPDATE_ALL, file, line);
  return 0;
}

/* Adds a completely new file.
 */
static int _filedb_addfile(FILE *fdb, filedb_entry *fdbe, char *file, int line)
{
  fdbe->_type = TYPE_NEW;
  _filedb_updatefile(fdb, POS_NEW, fdbe, UPDATE_ALL, file, line);
  return 0;
}

/* Short-cut macro to read an entry from disc to memory. Only
 * useful for filedb_getfile().
 */
#define filedb_read(fdb, entry, len)    \
{                                       \
  if ((len) > 0) {                      \
    (entry) = nmalloc((len));           \
    fread((entry), 1, (len), (fdb));    \
  }                                     \
}

/* Reads an entry from the fildb at the specified position. The
 * amount of information returned depends on the get flag.
 * It always positions the file position pointer exactly behind
 * the entry.
 */
static filedb_entry *_filedb_getfile(FILE *fdb, long pos, int get,
                                     char *file, int line)
{
  filedb_entry *fdbe;
  filedb_header fdh;

  /* Read header */
  fseek(fdb, pos, SEEK_SET);
  fread(&fdh, 1, sizeof(filedb_header), fdb);
  if (feof(fdb))
    return NULL;

  /* Allocate memory for file db entry */
  fdbe = _malloc_fdbe(file, line);

  /* Optimisation: maybe use memcpy here? */
  fdbe->uploaded = fdh.uploaded;
  fdbe->size = fdh.size;
  fdbe->stat = fdh.stat;
  fdbe->gots = fdh.gots;

  fdbe->buf_len = fdh.buffer_len;
  fdbe->dyn_len = filedb_tot_dynspace(fdh);
  fdbe->pos = pos;              /* Save position                */
  fdbe->_type = TYPE_EXIST;     /* Entry exists in DB           */

  /* This is useful for cases where we don't read the rest of the
   * data, but need to know whether the file is a link.
   */
  if (fdh.sharelink_len > 0)
    fdbe->stat |= FILE_ISLINK;
  else
    fdbe->stat &= ~FILE_ISLINK;

  /* Read additional data from db */
  if (get >= GET_FILENAME) {
    filedb_read(fdb, fdbe->filename, fdh.filename_len);
  } else
    fseek(fdb, fdh.filename_len, SEEK_CUR);
  if (get < GET_FULL || (fdh.stat & FILE_UNUSED))
    fseek(fdb, filedb_tot_dynspace(fdh) - fdh.filename_len, SEEK_CUR);
  else if (get == GET_FULL) {
    filedb_read(fdb, fdbe->desc, fdh.desc_len);
    filedb_read(fdb, fdbe->chan, fdh.chan_len);
    filedb_read(fdb, fdbe->uploader, fdh.uploader_len);
    filedb_read(fdb, fdbe->flags_req, fdh.flags_req_len);
    filedb_read(fdb, fdbe->sharelink, fdh.sharelink_len);
  }
  fseek(fdb, fdh.buffer_len, SEEK_CUR); /* Skip buffer                  */
  return fdbe;                  /* Return the ready structure   */
}

/* Searches the filedb for a file matching the specified mask, starting
 * at position 'pos'. The first matching file is returned.
 */
static filedb_entry *_filedb_matchfile(FILE *fdb, long pos, char *match,
                                       char *file, int line)
{
  filedb_entry *fdbe = NULL;

  fseek(fdb, pos, SEEK_SET);
  while (!feof(fdb)) {
    pos = ftell(fdb);
    fdbe = filedb_getfile(fdb, pos, GET_FILENAME);
    if (fdbe) {
      if (!(fdbe->stat & FILE_UNUSED) &&        /* Not unused?         */
          wild_match_file(match, fdbe->filename)) {     /* Matches our mask?   */
        free_fdbe(&fdbe);
        fdbe = _filedb_getfile(fdb, pos, GET_FULL, file, line); /* Save all data now   */
        return fdbe;
      }
      free_fdbe(&fdbe);
    }
  }
  return NULL;
}

/* Throws out all entries marked as unused, by walking through the
 * filedb and moving all good ones towards the top.
 */
static void filedb_cleanup(FILE *fdb)
{
  long oldpos, newpos, temppos;
  filedb_entry *fdbe = NULL;

  filedb_readtop(fdb, NULL);    /* Skip DB header  */
  newpos = temppos = oldpos = ftell(fdb);
  fseek(fdb, oldpos, SEEK_SET); /* Go to beginning */
  while (!feof(fdb)) {          /* Loop until EOF  */
    fdbe = filedb_getfile(fdb, oldpos, GET_HEADER);     /* Read header     */
    if (fdbe) {
      if (fdbe->stat & FILE_UNUSED) {   /* Found dirt!     */
        free_fdbe(&fdbe);
        while (!feof(fdb)) {    /* Loop until EOF  */
          newpos = ftell(fdb);
          fdbe = filedb_getfile(fdb, newpos, GET_FULL); /* Read next entry */
          if (!fdbe)
            break;
          if (!(fdbe->stat & FILE_UNUSED)) {    /* Not unused?     */
            temppos = ftell(fdb);
            filedb_movefile(fdb, oldpos, fdbe); /* Move to top     */
            oldpos = ftell(fdb);
            fseek(fdb, temppos, SEEK_SET);
          }
          free_fdbe(&fdbe);
        }
      } else {
        free_fdbe(&fdbe);
        oldpos = ftell(fdb);
      }
    }
  }
  ftruncate(fileno(fdb), oldpos);       /* Shorten file    */
}

/* Merges empty entries to one big entry, if they directly
 * follow each other. Does this for the complete DB.
 * This considerably speeds up several actions performed on
 * the db.
 */
static void filedb_mergeempty(FILE *fdb)
{
  filedb_entry *fdbe_t, *fdbe_i;
  int modified;

  filedb_readtop(fdb, NULL);
  while (!feof(fdb)) {
    fdbe_t = filedb_getfile(fdb, ftell(fdb), GET_HEADER);
    if (fdbe_t) {
      if (fdbe_t->stat & FILE_UNUSED) {
        modified = 0;
        fdbe_i = filedb_getfile(fdb, ftell(fdb), GET_HEADER);
        while (fdbe_i) {
          /* Is this entry in use? */
          if (!(fdbe_i->stat & FILE_UNUSED))
            break;              /* It is, exit loop. */

          /* Woohoo, found an empty entry. Append it's space to
           * our target entry's buffer space.
           */
          fdbe_t->buf_len += sizeof(filedb_header) + fdbe_i->buf_len;
          modified++;
          free_fdbe(&fdbe_i);
          /* Get next file entry */
          fdbe_i = filedb_getfile(fdb, ftell(fdb), GET_HEADER);
        }

        /* Did we exit the loop because of a used entry? */
        if (fdbe_i) {
          free_fdbe(&fdbe_i);
          /* Did we find any empty entries before? */
          if (modified)
            filedb_updatefile(fdb, fdbe_t->pos, fdbe_t, UPDATE_SIZE);
          /* ... or because we hit EOF? */
        } else {
          /* Truncate trailing empty entries and exit. */
          ftruncate(fileno(fdb), fdbe_t->pos);
          free_fdbe(&fdbe_t);
          return;
        }
      }
      free_fdbe(&fdbe_t);
    }
  }
}

/* Returns the filedb entry matching the filename 'fn' in
 * directory 'dir'.
 */
static filedb_entry *filedb_getentry(char *dir, char *fn)
{
  FILE *fdb;
  filedb_entry *fdbe = NULL;

  fdb = filedb_open(dir, 0);
  if (fdb) {
    filedb_readtop(fdb, NULL);
    fdbe = filedb_matchfile(fdb, ftell(fdb), fn);
    filedb_close(fdb);
  }
  return fdbe;
}

/* Initialises a new filedb by writing the db header, etc.
 */
static void filedb_initdb(FILE *fdb)
{
  filedb_top fdbt;

  fdbt.version = FILEDB_NEWEST_VER;
  fdbt.timestamp = now;
  filedb_writetop(fdb, &fdbt);
}

static void filedb_timestamp(FILE *fdb)
{
  filedb_top fdbt;

  filedb_readtop(fdb, &fdbt);
  fdbt.timestamp = time(NULL);
  filedb_writetop(fdb, &fdbt);
}

/* Updates the specified filedb in several ways:
 *
 * 1. Adds all new files from the directory to the db.
 * 2. Removes all stale entries from the db.
 * 3. Optimises the db.
 */
static void filedb_update(char *path, FILE *fdb, int sort)
{
  struct dirent *dd = NULL;
  struct stat st;
  filedb_entry *fdbe = NULL;
  DIR *dir = NULL;
  long where = 0;
  char *name = NULL, *s = NULL;

  /*
   * FIRST: make sure every real file is in the database
   */
  dir = opendir(path);
  if (dir == NULL) {
    putlog(LOG_MISC, "*", FILES_NOUPDATE);
    return;
  }
  dd = readdir(dir);
  while (dd != NULL) {
    malloc_strcpy(name, dd->d_name);
    if (name[0] != '.') {
      s = nmalloc(strlen(path) + strlen(name) + 2);
      sprintf(s, "%s/%s", path, name);
      stat(s, &st);
      my_free(s);
      filedb_readtop(fdb, NULL);
      fdbe = filedb_matchfile(fdb, ftell(fdb), name);
      if (!fdbe) {
        /* new file! */
        fdbe = malloc_fdbe();
        malloc_strcpy(fdbe->filename, name);
        malloc_strcpy(fdbe->uploader, botnetnick);
        fdbe->uploaded = now;
        fdbe->size = st.st_size;
        if (S_ISDIR(st.st_mode))
          fdbe->stat |= FILE_DIR;
        filedb_addfile(fdb, fdbe);
      } else if (fdbe->size != st.st_size) {
        /* update size if needed */
        fdbe->size = st.st_size;
        filedb_updatefile(fdb, fdbe->pos, fdbe, UPDATE_HEADER);
      }
      free_fdbe(&fdbe);
    }
    dd = readdir(dir);
  }
  if (name)
    my_free(name);
  closedir(dir);

  /*
   * SECOND: make sure every db file is real
   */
  filedb_readtop(fdb, NULL);
  fdbe = filedb_getfile(fdb, ftell(fdb), GET_FILENAME);
  while (fdbe) {
    where = ftell(fdb);
    if (!(fdbe->stat & FILE_UNUSED) && !(fdbe->stat & FILE_ISLINK) &&
        fdbe->filename) {
      s = nmalloc(strlen(path) + 1 + strlen(fdbe->filename) + 1);
      sprintf(s, "%s/%s", path, fdbe->filename);
      if (stat(s, &st) != 0)
        /* gone file */
        filedb_delfile(fdb, fdbe->pos);
      my_free(s);
    }
    free_fdbe(&fdbe);
    fdbe = filedb_getfile(fdb, where, GET_FILENAME);
  }

  /*
   * THIRD: optimise database
   *
   * Instead of sorting, we only clean up the db, because sorting is now
   * done on-the-fly when we display the file list.
   */
  if (sort)
    filedb_cleanup(fdb);        /* Cleanup DB           */
  filedb_timestamp(fdb);        /* Write new timestamp  */
}

/* Converts all slashes to dots. Returns an allocated buffer, so
 * do not forget to FREE it after use.
 */
static char *make_point_path(char *path)
{
  char *s2 = NULL, *p = NULL;

  malloc_strcpy(s2, path);
  if (s2[strlen(s2) - 1] == '/')
    s2[strlen(s2) - 1] = 0;     /* remove trailing '/' */
  p = s2;
  while (*p++)
    if (*p == '/')
      *p = '.';
  return s2;
}

/* Opens the filedb responsible to the specified directory.
 */
static FILE *filedb_open(char *path, int sort)
{
  char *s, *npath;
  FILE *fdb;
  filedb_top fdbt;
  struct stat st;

  if (count >= 2)
    putlog(LOG_MISC, "*", "(@) warning: %d open filedb's", count);
  npath = nmalloc(strlen(dccdir) + strlen(path) + 1);
  simple_sprintf(npath, "%s%s", dccdir, path);
  /* Use alternate filename if requested */
  if (filedb_path[0]) {
    char *s2;

    s2 = make_point_path(path);
    s = nmalloc(strlen(filedb_path) + strlen(s2) + 8);
    simple_sprintf(s, "%sfiledb.%s", filedb_path, s2);
    my_free(s2);
  } else {
    s = nmalloc(strlen(npath) + 10);
    simple_sprintf(s, "%s/.filedb", npath);
  }
  fdb = fopen(s, "r+b");
  if (!fdb) {
    if (convert_old_files(npath, s)) {
      fdb = fopen(s, "r+b");
      if (fdb == NULL) {
        putlog(LOG_MISC, "*", FILES_NOCONVERT, npath);
        my_free(s);
        my_free(npath);
        return NULL;
      }
      lockfile(fdb);
      filedb_update(npath, fdb, sort);
      count++;
      my_free(s);
      my_free(npath);
      return fdb;
    } else {
      filedb_top fdbt;

      /* Create new database and fix it up */
      fdb = fopen(s, "w+b");
      if (!fdb) {
        my_free(s);
        my_free(npath);
        return NULL;
      }
      lockfile(fdb);
      fdbt.version = FILEDB_NEWEST_VER;
      fdbt.timestamp = now;
      filedb_writetop(fdb, &fdbt);
      filedb_update(npath, fdb, sort);
      count++;
      my_free(s);
      my_free(npath);
      return fdb;
    }
  }

  lockfile(fdb);                /* Lock it from other bots */
  filedb_readtop(fdb, &fdbt);
  if (fdbt.version < FILEDB_NEWEST_VER) {
    if (!convert_old_db(&fdb, s)) {
      /* Conversion failed. Unlock file again and error out.
       * (convert_old_db() could have modified fdb, so check
       * for fdb != NULL.)
       */
      if (fdb)
        unlockfile(fdb);
      my_free(npath);
      my_free(s);
      return NULL;
    }
    filedb_update(npath, fdb, sort);
  }
  stat(npath, &st);
  /* Update filedb if:
   * + it's been 6 hours since it was last updated
   * + the directory has been visibly modified since then
   * (6 hours may be a bit often)
   */
  if (sort || ((now - fdbt.timestamp) > (6 * 3600)) ||
      (fdbt.timestamp < st.st_mtime) || (fdbt.timestamp < st.st_ctime))
    /* File database isn't up-to-date! */
    filedb_update(npath, fdb, sort & 1);
  else if ((now - fdbt.timestamp) > 300)
    filedb_mergeempty(fdb);

  count++;
  my_free(npath);
  my_free(s);
  return fdb;
}

/* Closes the filedb. Also removes the lock and updates the
 * timestamp.
 */
static void filedb_close(FILE *fdb)
{
  filedb_timestamp(fdb);
  fseek(fdb, 0L, SEEK_END);
  count--;
  unlockfile(fdb);
  fclose(fdb);
}

/* Adds information for a newly added file. Actually the name
 * is misleading, as the file is added in filedb_open() and we
 * only add information in here.
 */
static void filedb_add(FILE *fdb, char *filename, char *nick)
{
  filedb_entry *fdbe = NULL;

  filedb_readtop(fdb, NULL);
  /* When the filedb was opened, a record was already created. */
  fdbe = filedb_matchfile(fdb, ftell(fdb), filename);
  if (!fdbe)
    return;
  my_free(fdbe->uploader);
  malloc_strcpy(fdbe->uploader, nick);
  fdbe->uploaded = now;
  filedb_updatefile(fdb, fdbe->pos, fdbe, UPDATE_ALL);
  free_fdbe(&fdbe);
}

/* Outputs a sorted list of files/directories matching the mask,
 * to idx.
 */
static void filedb_ls(FILE *fdb, int idx, char *mask, int showall)
{
  int ok = 0, cnt = 0, is = 0;
  char s1[81], *p = NULL;
  struct flag_record user = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };
  filedb_entry *fdbe = NULL;
  filelist_t *flist = NULL;

  flist = filelist_new();
  filedb_readtop(fdb, NULL);
  fdbe = filedb_getfile(fdb, ftell(fdb), GET_FULL);
  while (fdbe) {
    ok = 1;
    if (fdbe->stat & FILE_UNUSED)
      ok = 0;
    if (ok && (fdbe->stat & FILE_DIR) && fdbe->flags_req) {
      /* Check permissions */
      struct flag_record req = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

      break_down_flags(fdbe->flags_req, &req, NULL);
      get_user_flagrec(dcc[idx].user, &user, dcc[idx].u.file->chat->con_chan);
      if (!flagrec_ok(&req, &user)) {
        ok = 0;
      }
    }
    if (ok)
      is = 1;
    if (ok && !wild_match_file(mask, fdbe->filename))
      ok = 0;
    if (ok && (fdbe->stat & FILE_HIDDEN) && !(showall))
      ok = 0;
    if (ok) {
      /* Display it! */
      if (cnt == 0) {
        dprintf(idx, FILES_LSHEAD1);
        dprintf(idx, FILES_LSHEAD2);
      }
      filelist_add(flist, fdbe->filename);
      if (fdbe->stat & FILE_DIR) {
        char *s2 = NULL, *s3 = NULL;

        /* Too long? */
        if (strlen(fdbe->filename) > 45) {
          /* Display the filename on its own line. */
          s2 = nmalloc(strlen(fdbe->filename) + 3);
          sprintf(s2, "%s/\n", fdbe->filename);
          filelist_addout(flist, s2);
          my_free(s2);
        } else {
          s2 = nmalloc(strlen(fdbe->filename) + 2);
          sprintf(s2, "%s/", fdbe->filename);
        }
        /* Note: You have to keep the sprintf and the nmalloc statements
         *       in sync, i.e. always check that you allocate enough
         *       memory.
         */
        if ((fdbe->flags_req) && (user.global &(USER_MASTER | USER_JANITOR))) {
          s3 = nmalloc(42 + strlen(s2 ? s2 : "") + 6 +
                       strlen(FILES_REQUIRES) + strlen(fdbe->flags_req) + 1 +
                       strlen(fdbe->chan ? fdbe->chan : "") + 1);
          sprintf(s3, "%-30s <DIR%s>  (%s %s%s%s)\n", s2,
                  fdbe->stat & FILE_SHARE ?
                  " SHARE" : "", FILES_REQUIRES, fdbe->flags_req,
                  fdbe->chan ? " " : "", fdbe->chan ? fdbe->chan : "");
        } else {
          s3 = nmalloc(38 + strlen(s2 ? s2 : ""));
          sprintf(s3, "%-30s <DIR>\n", s2 ? s2 : "");
        }
        if (s2)
          my_free(s2);
        filelist_addout(flist, s3);
        my_free(s3);
      } else {
        char s2[41], t[50], *s3 = NULL, *s4;

        s2[0] = 0;
        if (showall) {
          if (fdbe->stat & FILE_SHARE)
            strcat(s2, " (shr)");
          if (fdbe->stat & FILE_HIDDEN)
            strcat(s2, " (hid)");
        }
        egg_strftime(t, 10, "%d%b%Y", localtime(&fdbe->uploaded));
        if (fdbe->size < 1024)
          sprintf(s1, "%5d", fdbe->size);
        else
          sprintf(s1, "%4dk", (int) (fdbe->size / 1024));
        if (fdbe->sharelink)
          strcpy(s1, "     ");
        /* Too long? */
        if (strlen(fdbe->filename) > 30) {
          s3 = nmalloc(strlen(fdbe->filename) + 2);
          sprintf(s3, "%s\n", fdbe->filename);
          filelist_addout(flist, s3);
          my_free(s3);
          /* Causes filename to be displayed on its own line */
        } else
          malloc_strcpy(s3, fdbe->filename);
        s4 = nmalloc(69 + strlen(s3 ? s3 : "") + strlen(s1) +
                     strlen(fdbe->uploader) + strlen(t) + strlen(s2));
        sprintf(s4, "%-30s %s  %-9s (%s)  %6d%s\n", s3 ? s3 : "", s1,
                fdbe->uploader, t, fdbe->gots, s2);
        if (s3)
          my_free(s3);
        filelist_addout(flist, s4);
        my_free(s4);
        if (fdbe->sharelink) {
          s4 = nmalloc(9 + strlen(fdbe->sharelink));
          sprintf(s4, "   --> %s\n", fdbe->sharelink);
          filelist_addout(flist, s4);
          my_free(s4);
        }
      }
      if (fdbe->desc) {
        p = strchr(fdbe->desc, '\n');
        while (p != NULL) {
          *p = 0;
          if ((fdbe->desc)[0]) {
            char *sd;

            sd = nmalloc(strlen(fdbe->desc) + 5);
            sprintf(sd, "   %s\n", fdbe->desc);
            filelist_addout(flist, sd);
            my_free(sd);
          }
          strcpy(fdbe->desc, p + 1);
          p = strchr(fdbe->desc, '\n');
        }
        if ((fdbe->desc)[0]) {
          char *sd;

          sd = nmalloc(strlen(fdbe->desc) + 5);
          sprintf(sd, "   %s\n", fdbe->desc);
          filelist_addout(flist, sd);
          my_free(sd);
        }
      }
      cnt++;
    }
    free_fdbe(&fdbe);
    fdbe = filedb_getfile(fdb, ftell(fdb), GET_FULL);
  }
  if (is == 0)
    dprintf(idx, FILES_NOFILES);
  else if (cnt == 0)
    dprintf(idx, FILES_NOMATCH);
  else {
    filelist_sort(flist);
    filelist_idxshow(flist, idx);
    dprintf(idx, "--- %d file%s.\n", cnt, cnt != 1 ? "s" : "");
  }
  filelist_free(flist);
}

static void remote_filereq(int idx, char *from, char *file)
{
  char *p = NULL, *what = NULL, *dir = NULL,
    *s1 = NULL, *reject = NULL, *s = NULL;
  FILE *fdb = NULL;
  int i = 0;
  filedb_entry *fdbe = NULL;

  malloc_strcpy(what, file);
  p = strrchr(what, '/');
  if (p) {
    *p = 0;
    malloc_strcpy(dir, what);
    strcpy(what, p + 1);
  } else {
    malloc_strcpy(dir, "");
  }
  fdb = filedb_open(dir, 0);
  if (!fdb) {
    reject = FILES_DIRDNE;
  } else {
    filedb_readtop(fdb, NULL);
    fdbe = filedb_matchfile(fdb, ftell(fdb), what);
    filedb_close(fdb);
    if (!fdbe) {
      reject = FILES_FILEDNE;
    } else {
      if ((!(fdbe->stat & FILE_SHARE)) ||
          (fdbe->stat & (FILE_HIDDEN | FILE_DIR)))
        reject = FILES_NOSHARE;
      else {
        s1 = nmalloc(strlen(dccdir) + strlen(dir) + strlen(what) + 2);
        /* Copy to /tmp if needed */
        sprintf(s1, "%s%s%s%s", dccdir, dir, dir[0] ? "/" : "", what);
        if (copy_to_tmp) {
          s = nmalloc(strlen(tempdir) + strlen(what) + 1);
          sprintf(s, "%s%s", tempdir, what);
          copyfile(s1, s);
        } else
          s = s1;
        i = raw_dcc_send(s, "*remote", FILES_REMOTE, s);
        if (i > 0) {
          wipe_tmp_filename(s, -1);
          reject = FILES_SENDERR;
        }
        if (s1 != s)
          my_free(s);
        my_free(s1);
      }
      free_fdbe(&fdbe);
    }
  }
  s1 = nmalloc(strlen(botnetnick) + strlen(dir) + strlen(what) + 3);
  simple_sprintf(s1, "%s:%s/%s", botnetnick, dir, what);
  if (reject) {
    botnet_send_filereject(idx, s1, from, reject);
    my_free(s1);
    my_free(what);
    my_free(dir);
    return;
  }
  /* Grab info from dcc struct and bounce real request across net */
  i = dcc_total - 1;
  s = nmalloc(40);              /* Enough? */
  simple_sprintf(s, "%d %u %d", iptolong(getmyip()), dcc[i].port,
                 dcc[i].u.xfer->length);
  botnet_send_filesend(idx, s1, from, s);
  putlog(LOG_FILES, "*", FILES_REMOTEREQ, dir, dir[0] ? "/" : "", what);
  my_free(s1);
  my_free(s);
  my_free(what);
  my_free(dir);
}


/*
 *    Tcl functions
 */

static void filedb_getdesc(char *dir, char *fn, char **desc)
{
  filedb_entry *fdbe = NULL;

  fdbe = filedb_getentry(dir, fn);
  if (fdbe) {
    if (fdbe->desc) {
      *desc = nmalloc(strlen(fdbe->desc) + 1);
      strcpy(*desc, fdbe->desc);
    }
    free_fdbe(&fdbe);
  } else
    *desc = NULL;
}

static void filedb_getowner(char *dir, char *fn, char **owner)
{
  filedb_entry *fdbe = NULL;

  fdbe = filedb_getentry(dir, fn);
  if (fdbe) {
    *owner = nmalloc(strlen(fdbe->uploader) + 1);
    strcpy(*owner, fdbe->uploader);
    free_fdbe(&fdbe);
  } else
    *owner = NULL;
}

static int filedb_getgots(char *dir, char *fn)
{
  filedb_entry *fdbe = NULL;
  int gots = 0;

  fdbe = filedb_getentry(dir, fn);
  if (fdbe) {
    gots = fdbe->gots;
    free_fdbe(&fdbe);
  }
  return gots;
}

static void filedb_setdesc(char *dir, char *fn, char *desc)
{
  filedb_entry *fdbe = NULL;
  FILE *fdb = NULL;

  fdb = filedb_open(dir, 0);
  if (!fdb)
    return;
  filedb_readtop(fdb, NULL);
  fdbe = filedb_matchfile(fdb, ftell(fdb), fn);
  if (fdbe) {
    my_free(fdbe->desc);
    malloc_strcpy(fdbe->desc, desc);
    filedb_updatefile(fdb, fdbe->pos, fdbe, UPDATE_ALL);
    free_fdbe(&fdbe);
  }
  filedb_close(fdb);
}

static void filedb_setowner(char *dir, char *fn, char *owner)
{
  filedb_entry *fdbe = NULL;
  FILE *fdb = NULL;

  fdb = filedb_open(dir, 0);
  if (!fdb)
    return;
  filedb_readtop(fdb, NULL);
  fdbe = filedb_matchfile(fdb, ftell(fdb), fn);
  if (fdbe) {
    my_free(fdbe->uploader);
    malloc_strcpy(fdbe->uploader, owner);
    filedb_updatefile(fdb, fdbe->pos, fdbe, UPDATE_ALL);
    free_fdbe(&fdbe);
  }
  filedb_close(fdb);
}

static void filedb_setlink(char *dir, char *fn, char *link)
{
  filedb_entry *fdbe = NULL;
  FILE *fdb = NULL;

  fdb = filedb_open(dir, 0);
  if (!fdb)
    return;
  filedb_readtop(fdb, NULL);
  fdbe = filedb_matchfile(fdb, ftell(fdb), fn);
  if (fdbe) {
    /* Change existing one? */
    if ((fdbe->stat & FILE_DIR) || !fdbe->sharelink)
      return;
    if (!link || !link[0])
      filedb_delfile(fdb, fdbe->pos);
    else {
      my_free(fdbe->sharelink);
      malloc_strcpy(fdbe->sharelink, link);
      filedb_updatefile(fdb, fdbe->pos, fdbe, UPDATE_ALL);
    }
    free_fdbe(&fdbe);
    return;
  }

  fdbe = malloc_fdbe();
  malloc_strcpy(fdbe->uploader, botnetnick);
  malloc_strcpy(fdbe->filename, fn);
  malloc_strcpy(fdbe->sharelink, link);
  fdbe->uploaded = now;
  filedb_addfile(fdb, fdbe);
  free_fdbe(&fdbe);
  filedb_close(fdb);
}

static void filedb_getlink(char *dir, char *fn, char **link)
{
  filedb_entry *fdbe = NULL;

  fdbe = filedb_getentry(dir, fn);
  if (fdbe && (!(fdbe->stat & FILE_DIR))) {
    malloc_strcpy(*link, fdbe->sharelink);
  } else
    *link = NULL;
  if (fdbe)
    free_fdbe(&fdbe);
  return;
}

static void filedb_getfiles(Tcl_Interp *irp, char *dir)
{
  FILE *fdb;
  filedb_entry *fdbe;

  fdb = filedb_open(dir, 0);
  if (!fdb)
    return;
  filedb_readtop(fdb, NULL);
  while (!feof(fdb)) {
    fdbe = filedb_getfile(fdb, ftell(fdb), GET_FILENAME);
    if (fdbe) {
      if (!(fdbe->stat & (FILE_DIR | FILE_UNUSED)))
        Tcl_AppendElement(irp, fdbe->filename);
      free_fdbe(&fdbe);
    }
  }
  filedb_close(fdb);
}

static void filedb_getdirs(Tcl_Interp *irp, char *dir)
{
  FILE *fdb;
  filedb_entry *fdbe;

  fdb = filedb_open(dir, 0);
  if (!fdb)
    return;
  filedb_readtop(fdb, NULL);
  while (!feof(fdb)) {
    fdbe = filedb_getfile(fdb, ftell(fdb), GET_FILENAME);
    if (fdbe) {
      if ((!(fdbe->stat & FILE_UNUSED)) && (fdbe->stat & FILE_DIR))
        Tcl_AppendElement(irp, fdbe->filename);
      free_fdbe(&fdbe);
    }
  }
  filedb_close(fdb);
}

static void filedb_change(char *dir, char *fn, int what)
{
  FILE *fdb;
  filedb_entry *fdbe;
  int changed = 0;

  fdb = filedb_open(dir, 0);
  if (!fdb)
    return;
  filedb_readtop(fdb, NULL);
  fdbe = filedb_matchfile(fdb, ftell(fdb), fn);
  if (fdbe) {
    if (!(fdbe->stat & FILE_DIR)) {
      switch (what) {
      case FILEDB_SHARE:
        fdbe->stat |= FILE_SHARE;
        break;
      case FILEDB_UNSHARE:
        fdbe->stat &= ~FILE_SHARE;
        break;
      }
      changed = 1;
    }
    switch (what) {
    case FILEDB_HIDE:
      fdbe->stat |= FILE_HIDDEN;
      changed = 1;
      break;
    case FILEDB_UNHIDE:
      fdbe->stat &= ~FILE_HIDDEN;
      changed = 1;
      break;
    }
    if (changed)
      filedb_updatefile(fdb, fdbe->pos, fdbe, UPDATE_HEADER);
    free_fdbe(&fdbe);
  }
  filedb_close(fdb);
}
