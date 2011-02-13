/*
 * misc_file.c -- handles:
 *   copyfile() movefile() file_readable()
 *
 * $Id: misc_file.c,v 1.18 2011/02/13 14:19:33 simple Exp $
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

#include "main.h"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "stat.h"


/* Copy a file from one place to another (possibly erasing old copy).
 *
 * returns:  0 if OK
 *           1 if can't open original file
 *           2 if can't open new file
 *           3 if original file isn't normal
 *           4 if ran out of disk space
 */
int copyfile(char *oldpath, char *newpath)
{
  int fi, fo, x;
  char buf[512];
  struct stat st;

#ifndef CYGWIN_HACKS
  fi = open(oldpath, O_RDONLY, 0);
#else
  fi = open(oldpath, O_RDONLY | O_BINARY, 0);
#endif
  if (fi < 0)
    return 1;
  fstat(fi, &st);
  if (!(st.st_mode & S_IFREG))
    return 3;
  fo = creat(newpath, (int) (st.st_mode & 0777));
  if (fo < 0) {
    close(fi);
    return 2;
  }
  for (x = 1; x > 0;) {
    x = read(fi, buf, 512);
    if (x > 0) {
      if (write(fo, buf, x) < x) {      /* Couldn't write */
        close(fo);
        close(fi);
        unlink(newpath);
        return 4;
      }
    }
  }
#ifdef HAVE_FSYNC
  fsync(fo);
#endif /* HAVE_FSYNC */
  close(fo);
  close(fi);
  return 0;
}

int movefile(char *oldpath, char *newpath)
{
  int ret;

#ifdef HAVE_RENAME
  /* Try to use rename first */
  if (!rename(oldpath, newpath))
    return 0;
#endif /* HAVE_RENAME */

  /* If that fails, fall back to just copying and then
   * deleting the file.
   */
  ret = copyfile(oldpath, newpath);
  if (!ret)
    unlink(oldpath);
  return ret;
}

int file_readable(char *file)
{
  FILE *fp;

  if (!(fp = fopen(file, "r")))
    return 0;

  fclose(fp);
  return 1;
}
