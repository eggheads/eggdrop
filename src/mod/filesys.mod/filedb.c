/* 
 * filedb.c -- part of filesys.mod
 *   low-level manipulation of the filesystem database
 *   files reaction to remote requests for files 
 * 
 * dprintf'ized, 25feb1996
 * english, 5mar1996
 * 
 * $Id: filedb.c,v 1.7 2000/01/08 21:23:15 per Exp $
 */
/* 
 * Copyright (C) 1997  Robey Pointer
 * Copyright (C) 1999, 2000  Eggheads
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

/* lock the file, using fcntl */
static void lockfile(FILE * f)
{
  struct flock fl;

  fl.l_type = F_WRLCK;
  fl.l_start = 0;
  fl.l_whence = SEEK_SET;
  fl.l_len = 0;
  /* block on lock: */
  fcntl(fileno(f), F_SETLKW, &fl);
}

/* unlock the file */
static void unlockfile(FILE * f)
{
  struct flock fl;

  fl.l_type = F_UNLCK;
  fl.l_start = 0;
  fl.l_whence = SEEK_SET;
  fl.l_len = 0;
  fcntl(fileno(f), F_SETLKW, &fl);
}

/* use a where of 0 to start out, then increment 1 space for each next */
static int findmatch(FILE * f, char *lookfor, long *where, filedb * fdb)
{
  char match[256];
  int l;

  strncpy(match, lookfor, 255);
  match[255] = 0;
  /* clip any trailing / */
  l = strlen(match) - 1;
  if (match[l] == '/')
    match[l] = 0;
  fseek(f, *where, SEEK_SET);
  while (!feof(f)) {
    *where = ftell(f);
    fread(fdb, sizeof(filedb), 1, f);
    if (!feof(f)) {
      if (!(fdb->stat & FILE_UNUSED) &&
	  wild_match_file(match, fdb->filename))
	return 1;
    }
  }
  return 0;
}

static long findempty(FILE * f)
{
  long where = 0L;
  filedb fdb;

  rewind(f);
  while (!feof(f)) {
    where = ftell(f);
    fread(&fdb, sizeof(filedb), 1, f);
    if (!feof(f)) {
      if (fdb.stat & FILE_UNUSED)
	return where;
    }
  }
  fseek(f, 0L, SEEK_END);
  where = ftell(f);
  return where;
}

static void filedb_timestamp(FILE * f)
{
  filedb fdb;
  int x;

  /* read 1st filedb entry if it's there */
  rewind(f);
  x = fread(&fdb, sizeof(filedb), 1, f);
  if (x < 1) {
    /* MAKE 1st filedb entry then! */
    fdb.version = FILEVERSION;
    fdb.stat = FILE_UNUSED;
  }
  fdb.timestamp = now;
  rewind(f);
  fwrite(&fdb, sizeof(filedb), 1, f);
}

/* return 1 if i find a '.files' and convert it */
static int convert_old_db(char *path, char *newfiledb)
{
  FILE *f, *g;
  char s[256], *fn, *nick, *tm, *s1 = s;

  filedb fdb;
  int in_file = 0, i;
  long where;
  struct stat st;

  sprintf(s, "%s/.files", path);
  f = fopen(s, "r");
  if (f == NULL)
    return 0;
  g = fopen(newfiledb, "w+b");
  if (g == NULL) {
    putlog(LOG_MISC, "(!) Can't create filedb in %s", newfiledb);
    fclose(f);
    return 0;
  }
  putlog(LOG_FILES, "*", FILES_CONVERT, path);
  where = ftell(g);
  /* scan contents of .files and painstakingly create .filedb entries */
  while (!feof(f)) {
    fgets(s, 120, f);
    if (s[strlen(s) - 1] == '\n')
      s[strlen(s) - 1] = 0;
    if (!feof(f)) {
      fn = newsplit(&s1);
      rmspace(fn);
      if ((fn[0]) && (fn[0] != ';') && (fn[0] != '#')) {
	/* not comment */
	if (fn[0] == '-') {
	  /* adjust comment for current file */
	  if (in_file) {
	    rmspace(s);
	    if (strlen(s) + strlen(fdb.desc) <= 600) {
	      strcat(fdb.desc, "\n");
	      strcat(fdb.desc, s);
	      fseek(g, where, SEEK_SET);
	      fwrite(&fdb, sizeof(filedb), 1, g);
	    }
	  }
	} else {
	  in_file = 1;
	  where = ftell(g);
	  nick = newsplit(&s1);
	  rmspace(nick);
	  tm = newsplit(&s1);
	  rmspace(tm);
	  rmspace(s1);
	  i = strlen(fn) - 1;
	  if (fn[i] == '/')
	    fn[i] = 0;
	  fdb.version = FILEVERSION;
	  fdb.stat = 0;
	  fdb.desc[0] = 0;
	  fdb.chname[0] = 0;
	  fdb.flags_req[0] = 0;
	  strcpy(fdb.filename, fn);
	  strcpy(fdb.uploader, nick);
	  fdb.gots = atoi(s1);
	  fdb.sharelink[0] = 0;
	  fdb.uploaded = atoi(tm);
	  sprintf(s, "%s/%s", path, fn);
	  if (stat(s, &st) == 0) {
	    /* file is okay */
	    if (S_ISDIR(st.st_mode)) {
	      fdb.stat |= FILE_DIR;
	      if (nick[0] == '+') {
		char x[100];

		/* only do global flags, it's an old one */
		struct flag_record fr =
		{FR_GLOBAL, 0, 0, 0, 0, 0};

		break_down_flags(nick + 1, &fr, NULL);
		build_flags(x, &fr, NULL);
		/* we only want valid flags */
		strncpy(fdb.flags_req, x, 21);
		fdb.flags_req[21] = 0;
	      }
	    }
	    fdb.size = st.st_size;
	    fwrite(&fdb, sizeof(filedb), 1, g);
	  } else
	    in_file = 0;	/* skip */
	}
      }
    }
  }
  fseek(g, 0, SEEK_END);
  fclose(g);
  fclose(f);
  return 1;
}

static void filedb_update(char *path, FILE * f, int sort)
{
  struct dirent *dd;
  DIR *dir;
  filedb fdb[2];
  char name[61];
  long where, oldwhere;
  struct stat st;
  char s[512];
  int ret;

  /* FIRST: make sure every real file is in the database */
  dir = opendir(path);
  if (dir == NULL) {
    putlog(LOG_MISC, "*", FILES_NOUPDATE);
    return;
  }
  dd = readdir(dir);
  while (dd != NULL) {
    strncpy(name, dd->d_name, 60);
    name[60] = 0;
    if (NAMLEN(dd) <= 60)
      name[NAMLEN(dd)] = 0;
    else {
      /* truncate name on disk */
      char s1[512], s2[256];

      strcpy(s1, path);
      strcat(s1, "/");
      strncat(s1, dd->d_name, NAMLEN(dd));
      s1[strlen(path) + NAMLEN(dd) + 1] = 0;
      sprintf(s2, "%s/%s", path, name);
      movefile(s1, s2);
    }
    if (name[0] != '.') {
      sprintf(s, "%s/%s", path, name);
      stat(s, &st);
      where = 0;
      ret = findmatch(f, name, &where, &fdb[0]);
      if (!ret) {
	/* new file! */
	where = findempty(f);
	fseek(f, where, SEEK_SET);
	fdb[0].version = FILEVERSION;
	fdb[0].stat = 0;	/* by default, visible regular file */
	strcpy(fdb[0].filename, name);
	fdb[0].desc[0] = 0;
	strcpy(fdb[0].uploader, botnetnick);
	fdb[0].gots = 0;
	fdb[0].flags_req[0] = 0;
	fdb[0].uploaded = now;
	fdb[0].size = st.st_size;
	fdb[0].sharelink[0] = 0;
	if (S_ISDIR(st.st_mode))
	  fdb[0].stat |= FILE_DIR;
	fwrite(&fdb[0], sizeof(filedb), 1, f);
      } else if (fdb[0].version < FILEVERSION) {
	/* old version filedb, do the dirty */
	if (fdb[0].version == FILEVERSION_OLD) {
	  filedb_old *fdbo = (filedb_old *) & fdb[0];

	  fdb[0].desc[185] = 0;	/* truncate it */
	  fdb[0].chname[0] = 0;	/* new entry */
	  strcpy(fdb[0].uploader, fdbo->uploader);	/* moved forward
							 * a few bytes */
	  strcpy(fdb[0].flags_req, fdbo->flags_req);	/* and again */
	  fdb[0].version = FILEVERSION;
	  fdb[0].size = st.st_size;
	  fseek(f, where, SEEK_SET);
	  fwrite(&fdb[0], sizeof(filedb), 1, f);
	} else {
	  putlog(LOG_MISC, "*", "!!! Unknown filedb type !");
	}
      } else {
	/* update size if needed */
	fdb[0].size = st.st_size;
	fseek(f, where, SEEK_SET);
	fwrite(&fdb[0], sizeof(filedb), 1, f);
      }
    }
    dd = readdir(dir);
  }
  closedir(dir);
  /* SECOND: make sure every db file is real, and sort as we go,
   * if we're sorting */
  rewind(f);
  while (!feof(f)) {
    where = ftell(f);
    fread(&fdb[0], sizeof(filedb), 1, f);
    if (!feof(f)) {
      if (!(fdb[0].stat & FILE_UNUSED) && !fdb[0].sharelink[0]) {
	sprintf(s, "%s/%s", path, fdb[0].filename);
	if (stat(s, &st) != 0) {
	  /* gone file */
	  fseek(f, where, SEEK_SET);
	  fdb[0].stat |= FILE_UNUSED;
	  fwrite(&fdb[0], sizeof(filedb), 1, f);
	  /* sunos and others will puke bloody chunks if you write the
	   * last record in a file and then attempt to read to EOF: */
	  fseek(f, where, SEEK_SET);
	  continue;		/* cycle to next one */
	}
      }
      if (sort && !(fdb[0].stat & FILE_UNUSED)) {
	rewind(f);
	oldwhere = ftell(f);
	ret = 0;
	while (!feof(f) && (oldwhere < where)) {
	  fread(&fdb[1 - ret], sizeof(filedb), 1, f);
	  if (!feof(f)) {
	    if ((fdb[0].stat & FILE_UNUSED) ||
		(strcasecmp(fdb[ret].filename,
			    fdb[1 - ret].filename) < 0)) {
	      /* our current is < the checked one, insert here */
	      fseek(f, oldwhere, SEEK_SET);
	      fwrite(&fdb[ret], sizeof(filedb), 1, f);
	      ret = 1 - ret;
	      /* and fall out */
	    }
	    /* otherwise read next entry */
	    oldwhere = ftell(f);
	  }
	}
	/* here, either we've found a place to insert, or got to
	 * the end of the list */
	/* if we've got to the end of the current list oldwhere == where
	 * so we fall through ret will point to the current valid one */
	while (!feof(f) && (oldwhere < where)) {
	  /* need to move this entry up 1 .. */
	  fread(&fdb[1 - ret], sizeof(filedb), 1, f);
	  oldwhere = ftell(f);
	  /* write lower record here */
	  fseek(f, oldwhere, SEEK_SET);
	  fwrite(&fdb[ret], sizeof(filedb), 1, f);
	  ret = 1 - ret;
	}
	/* when we get here fdb[ret] holds the last record,
	 * which needs  to be written where we first grabbed the
	 * record from */
	fseek(f, where, SEEK_SET);
	fwrite(&fdb[ret], sizeof(filedb), 1, f);
      }
    }
  }
  /* write new timestamp */
  filedb_timestamp(f);
}

static int count = 0;

static FILE *filedb_open(char *path, int sort)
{
  char s[DIRLEN], npath[DIRLEN];
  FILE *f;
  filedb fdb;
  struct stat st;

  if (count >= 2)
    putlog(LOG_MISC, "*", "(@) warning: %d open filedb's", count);
  simple_sprintf(npath, "%s%s", dccdir, path);
  /* use alternate filename if requested */
  if (filedb_path[0]) {
    char s2[DIRLEN], *p;

    strcpy(s2, path);
    p = s2;
    while (*p++)
      if (*p == '/')
	*p = '.';
    simple_sprintf(s, "%sfiledb.%s", filedb_path, s2);
    if (s[strlen(s) - 1] == '.')
      s[strlen(s) - 1] = 0;
  } else
    simple_sprintf(s, "%s/.filedb", npath);
  f = fopen(s, "r+b");
  if (!f) {
    /* attempt to convert */
    if (convert_old_db(npath, s)) {
      f = fopen(s, "r+b");
      if (f == NULL) {
	putlog(LOG_MISC, FILES_NOCONVERT, npath);
	return NULL;
      }
      lockfile(f);
      filedb_update(npath, f, sort);	/* make it correct */
      count++;
      return f;
    }
    /* create new database and fix it up */
    f = fopen(s, "w+b");
    if (!f)
      return NULL;
    lockfile(f);
    filedb_update(npath, f, sort);
    count++;
    return f;
  }
  /* lock it from other bots: */
  lockfile(f);
  /* check the timestamp... */
  fread(&fdb, sizeof(filedb), 1, f);
  stat(npath, &st);
  /* update filedb if:
   * + it's been 6 hours since it was last updated
   * + the directory has been visibly modified since then
   * (6 hours may be a bit often) */
  if (sort || ((now - fdb.timestamp) > (6 * 3600)) ||
      (fdb.timestamp < st.st_mtime) ||
      (fdb.timestamp < st.st_ctime) ||
      (fdb.version <= FILEVERSION_OLD))
    /* file database isn't up-to-date! */
    filedb_update(npath, f, sort & 1);
  count++;
  return f;
}

static void filedb_close(FILE * f)
{
  filedb_timestamp(f);
  fseek(f, 0L, SEEK_END);
  count--;
  unlockfile(f);
  fclose(f);
}

static void filedb_add(FILE * f, char *filename, char *nick)
{
  long where;
  filedb fdb;

  /* when the filedb was opened, a record was already created */
  where = 0;
  if (!findmatch(f, filename, &where, &fdb))
    return;
  strncpy(fdb.uploader, nick, HANDLEN);
  fdb.uploader[HANDLEN] = 0;
  fdb.uploaded = now;
  fseek(f, where, SEEK_SET);
  fwrite(&fdb, sizeof(filedb), 1, f);
}

static void filedb_ls(FILE *f, int idx, char *mask, int showall)
{
  filedb fdb;
  int ok = 0, cnt = 0, is = 0;
  char s[81], s1[81], *p;
  struct flag_record user = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  rewind(f);
  while (!feof(f)) {
    fread(&fdb, sizeof(filedb), 1, f);
    if (!feof(f)) {
      ok = 1;
      if (fdb.stat & FILE_UNUSED)
	ok = 0;
      if (fdb.stat & FILE_DIR) {
	/* check permissions */
	struct flag_record req = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

	break_down_flags(fdb.flags_req, &req, NULL);
	get_user_flagrec(dcc[idx].user, &user,
			 dcc[idx].u.file->chat->con_chan);
	if (!flagrec_ok(&req, &user))
	  ok = 0;
      }
      if (ok)
	is = 1;
      if (!wild_match_file(mask, fdb.filename))
	ok = 0;
      if ((fdb.stat & FILE_HIDDEN) && !(showall))
	ok = 0;
      if (ok) {
	/* display it! */
	if (cnt == 0) {
	  dprintf(idx, FILES_LSHEAD1);
	  dprintf(idx, FILES_LSHEAD2);
	}
	if (fdb.stat & FILE_DIR) {
	  char s2[50];

	  /* too long? */
	  if (strlen(fdb.filename) > 45) {
	    dprintf(idx, "%s/\n", fdb.filename);
	    s2[0] = 0;
	    /* causes filename to be displayed on its own line */
	  } else
	    sprintf(s2, "%s/", fdb.filename);
	  if ((fdb.flags_req[0]) &&
	      (user.global &(USER_MASTER | USER_JANITOR))) {
	    dprintf(idx, "%-30s <DIR%s>  (%s %s%s%s)\n", s2,
		    fdb.stat & FILE_SHARE ?
		    " SHARE" : "", FILES_REQUIRES, fdb.flags_req,
		    fdb.chname[0] ? " " : "", fdb.chname);
	  } else
	    dprintf(idx, "%-30s <DIR>\n", s2);
	} else {
	  char s2[41];

	  s2[0] = 0;
	  if (showall) {
	    if (fdb.stat & FILE_SHARE)
	      strcat(s2, " (shr)");
	    if (fdb.stat & FILE_HIDDEN)
	      strcat(s2, " (hid)");
	  }
	  strcpy(s, ctime(&fdb.uploaded));
	  s[10] = 0;
	  s[7] = 0;
	  s[24] = 0;
	  strcpy(s, &s[8]);
	  strcpy(&s[2], &s[4]);
	  strcpy(&s[5], &s[22]);
	  if (fdb.size < 1024)
	    sprintf(s1, "%5d", fdb.size);
	  else
	    sprintf(s1, "%4dk", (int) (fdb.size / 1024));
	  if (fdb.sharelink[0])
	    strcpy(s1, "     ");
	  /* too long? */
	  if (strlen(fdb.filename) > 30) {
	    dprintf(idx, "%s\n", fdb.filename);
	    fdb.filename[0] = 0;
	    /* causes filename to be displayed on its own line */
	  }
	  dprintf(idx, "%-30s %s  %-9s (%s)  %6d%s\n", fdb.filename, s1,
		  fdb.uploader, s, fdb.gots, s2);
	  if (fdb.sharelink[0]) {
	    dprintf(idx, "   --> %s\n", fdb.sharelink);
	  }
	}
	if (fdb.desc[0]) {
	  p = strchr(fdb.desc, '\n');
	  while (p != NULL) {
	    *p = 0;
	    if (fdb.desc[0])
	      dprintf(idx, "   %s\n", fdb.desc);
	    strcpy(fdb.desc, p + 1);
	    p = strchr(fdb.desc, '\n');
	  }
	  if (fdb.desc[0])
	    dprintf(idx, "   %s\n", fdb.desc);
	}
	cnt++;
      }
    }
  }
  if (is == 0)
    dprintf(idx, FILES_NOFILES);
  else if (cnt == 0)
    dprintf(idx, FILES_NOMATCH);
  else
    dprintf(idx, "--- %d file%s.\n", cnt, cnt > 1 ? "s" : "");
}

static void remote_filereq(int idx, char *from, char *file)
{
  char *p, what[256], dir[256], s[256], s1[256], *reject;
  FILE *f;
  filedb fdb;
  long i = 0;

  strcpy(what, file);
  p = strrchr(what, '/');
  if (p == NULL)
    dir[0] = 0;
  else {
    *p = 0;
    strcpy(dir, what);
    strcpy(what, p + 1);
  }
  f = filedb_open(dir, 0);
  reject = NULL;
  if (f == NULL) {
    reject = FILES_DIRDNE;
  } else {
    if (!findmatch(f, what, &i, &fdb)) {
      reject = FILES_FILEDNE;
      filedb_close(f);
    } else {
      if ((!(fdb.stat & FILE_SHARE)) ||
	  (fdb.stat & (FILE_HIDDEN | FILE_DIR))) {
	reject = FILES_NOSHARE;
	filedb_close(f);
      } else {
	filedb_close(f);
	/* copy to /tmp if needed */
	sprintf(s1, "%s%s%s%s", dccdir, dir, dir[0] ? "/" : "", what);
	if (copy_to_tmp) {
	  sprintf(s, "%s%s", tempdir, what);
	  copyfile(s1, s);
	} else
	  strcpy(s, s1);
	i = raw_dcc_send(s, "*remote", FILES_REMOTE, s);
	if (i > 0) {
	  wipe_tmp_filename(s, -1);
	  reject = FILES_SENDERR;
	}
      }
    }
  }
  simple_sprintf(s1, "%s:%s/%s", botnetnick, dir, what);
  if (reject) {
    botnet_send_filereject(idx, s1, from, reject);
    return;
  }
  /* grab info from dcc struct and bounce real request across net */
  i = dcc_total - 1;
  simple_sprintf(s, "%d %u %d", iptolong(getmyip()), dcc[i].port,
		dcc[i].u.xfer->length);
  botnet_send_filesend(idx, s1, from, s);
  putlog(LOG_FILES, "*", FILES_REMOTEREQ, dir, dir[0] ? "/" : "", what);
}

/*** for tcl: ***/

static void filedb_getdesc(char *dir, char *fn, char *desc)
{
  FILE *f;
  filedb fdb;
  long i = 0;

  f = filedb_open(dir, 0);
  if (!f) {
    desc[0] = 0;
    return;
  }
  if (!findmatch(f, fn, &i, &fdb))
    desc[0] = 0;
  else
    strcpy(desc, fdb.desc);
  filedb_close(f);
  return;
}

static void filedb_getowner(char *dir, char *fn, char *owner)
{
  FILE *f;
  filedb fdb;
  long i = 0;

  f = filedb_open(dir, 0);
  if (!f) {
    owner[0] = 0;
    return;
  }
  if (!findmatch(f, fn, &i, &fdb))
    owner[0] = 0;
  else {
    strncpy(owner, fdb.uploader, HANDLEN);
    owner[HANDLEN] = 0;
  }
  filedb_close(f);
  return;
}

static int filedb_getgots(char *dir, char *fn)
{
  FILE *f;
  filedb fdb;
  long i = 0;

  f = filedb_open(dir, 0);
  if (!f)
    return 0;
  if (!findmatch(f, fn, &i, &fdb))
    fdb.gots = 0;
  filedb_close(f);
  return fdb.gots;
}

static void filedb_setdesc(char *dir, char *fn, char *desc)
{
  FILE *f;
  filedb fdb;
  long where = 0;

  f = filedb_open(dir, 0);
  if (!f)
    return;
  if (findmatch(f, fn, &where, &fdb)) {
    strncpy(fdb.desc, desc, 185);
    fdb.desc[185] = 0;
    fseek(f, where, SEEK_SET);
    fwrite(&fdb, sizeof(filedb), 1, f);
  }
  filedb_close(f);
  return;
}

static void filedb_setowner(char *dir, char *fn, char *owner)
{
  FILE *f;
  filedb fdb;
  long where = 0;

  f = filedb_open(dir, 0);
  if (!f)
    return;
  if (findmatch(f, fn, &where, &fdb)) {
    strncpy(fdb.uploader, owner, HANDLEN);
    fdb.uploader[HANDLEN] = 0;
    fseek(f, where, SEEK_SET);
    fwrite(&fdb, sizeof(filedb), 1, f);
  }
  filedb_close(f);
  return;
}

static void filedb_setlink(char *dir, char *fn, char *link)
{
  FILE *f;
  filedb fdb;
  long where = 0;

  f = filedb_open(dir, 0);
  if (!f)
    return;
  if (findmatch(f, fn, &where, &fdb)) {
    /* change existing one? */
    if ((fdb.stat & FILE_DIR) || !(fdb.sharelink[0]))
      return;
    if (!link[0]) {
      /* erasing file */
      fdb.stat |= FILE_UNUSED;
    } else {
      strncpy(fdb.sharelink, link, 60);
      fdb.sharelink[60] = 0;
    }
    fseek(f, where, SEEK_SET);
    fwrite(&fdb, sizeof(filedb), 1, f);
    filedb_close(f);
    return;
  }
  fdb.version = FILEVERSION;
  fdb.stat = 0;
  fdb.desc[0] = 0;
  strcpy(fdb.uploader, botnetnick);
  strncpy(fdb.filename, fn, 30);
  fdb.filename[30] = 0;
  fdb.flags_req[0] = 0;
  fdb.uploaded = now;
  fdb.size = 0;
  fdb.gots = 0;
  fdb.chname[0] = 0;
  strncpy(fdb.sharelink, link, 60);
  fdb.sharelink[60] = 0;
  where = findempty(f);
  fseek(f, where, SEEK_SET);
  fwrite(&fdb, sizeof(filedb), 1, f);
  filedb_close(f);
}

static void filedb_getlink(char *dir, char *fn, char *link)
{
  FILE *f;
  filedb fdb;
  long i = 0;

  f = filedb_open(dir, 0);
  link[0] = 0;
  if (!f)
    return;
  if (findmatch(f, fn, &i, &fdb) && !(fdb.stat & FILE_DIR))
    strcpy(link, fdb.sharelink);
  filedb_close(f);
  return;
}

static void filedb_getfiles(Tcl_Interp * irp, char *dir)
{
  FILE *f;
  filedb fdb;

  f = filedb_open(dir, 0);
  if (!f)
    return;
  rewind(f);
  while (!feof(f)) {
    fread(&fdb, sizeof(filedb), 1, f);
    if (!feof(f)) {
      if (!(fdb.stat & (FILE_DIR | FILE_UNUSED)))
	Tcl_AppendElement(irp, fdb.filename);
    }
  }
  filedb_close(f);
}

static void filedb_getdirs(Tcl_Interp * irp, char *dir)
{
  FILE *f;
  filedb fdb;

  f = filedb_open(dir, 0);
  if (!f)
    return;
  rewind(f);
  while (!feof(f)) {
    fread(&fdb, sizeof(filedb), 1, f);
    if (!feof(f)) {
      if ((!(fdb.stat & FILE_UNUSED)) && (fdb.stat & FILE_DIR))
	Tcl_AppendElement(irp, fdb.filename);
    }
  }
  filedb_close(f);
}

static void filedb_change(char *dir, char *fn, int what)
{
  FILE *f;
  filedb fdb;
  long where;

  f = filedb_open(dir, 0);
  if (f) {
    if (findmatch(f, fn, &where, &fdb) && !(fdb.stat & FILE_DIR)) {
      switch (what) {
      case FILEDB_HIDE:
	fdb.stat |= FILE_HIDDEN;
	break;
      case FILEDB_UNHIDE:
	fdb.stat &= ~FILE_HIDDEN;
	break;
      case FILEDB_SHARE:
	fdb.stat |= FILE_SHARE;
	break;
      case FILEDB_UNSHARE:
	fdb.stat &= ~FILE_SHARE;
	break;
      }
      fseek(f, where, SEEK_SET);
      fwrite(&fdb, sizeof(filedb), 1, f);
    }
    filedb_close(f);
  }
  return;
}
