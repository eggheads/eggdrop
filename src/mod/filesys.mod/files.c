/* 
 * files.c -- part of filesys.mod
 *   all the file system commands
 * 
 * dprintf'ized, 4nov1995 rewritten, 26feb1996
 * 
 * $Id: files.c,v 1.12 2000/01/11 13:37:19 per Exp $
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

/* 
 * 'configure' is supposed to make things easier for me now
 * PLEASE don't fail me, 'configure'! :)
 */
#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

/* goddamn stupid sunos 4 */
#ifndef S_IFREG
#define S_IFREG 0100000
#endif
#ifndef S_IFLNK
#define S_IFLNK 0120000
#endif

/* are there too many people in the file system? */
static int too_many_filers()
{
  int i, n = 0;

  if (dcc_users == 0)
    return 0;
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_FILES)
      n++;
  return (n >= dcc_users);
}

/* someone uploaded a file -- add it */
static void add_file(char *dir, char *file, char *nick)
{
  FILE *f;

  /* gave me a full pathname */
  /* only continue if the destination is within the visible file system */
  if (!strncmp(dccdir, dir, strlen(dccdir)) &&
      (f = filedb_open(&dir[strlen(dccdir)], 2))) {
    filedb_add(f, file, nick);
    filedb_close(f);
  }
}

static int welcome_to_files(int idx)
{
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  FILE *f;
  char *p = get_user(&USERENTRY_DCCDIR, dcc[idx].user);

  dprintf(idx, "\n");
  if (fr.global &USER_JANITOR)
    fr.global |=USER_MASTER;

  /* show motd if the user went straight here without going thru the
   * party line */
  if (!(dcc[idx].status & STAT_CHAT))
    show_motd(idx);
  sub_lang(idx, FILES_WELCOME);
  sub_lang(idx, FILES_WELCOME1);
  if (p)
    strcpy(dcc[idx].u.file->dir, p);
  else
    dcc[idx].u.file->dir[0] = 0;
  /* does this dir even exist any more? */
  f = filedb_open(dcc[idx].u.file->dir, 0);
  if (f == NULL) {
    dcc[idx].u.file->dir[0] = 0;
    f = filedb_open(dcc[idx].u.file->dir, 0);
    if (f == NULL) {
      dprintf(idx, FILES_BROKEN);
      dprintf(idx, FILES_INVPATH);
      dprintf(idx, "\n\n");
      dccdir[0] = 0;
      chanout_but(-1, dcc[idx].u.file->chat->channel,
		  "*** %s rejoined the party line.\n",
		  dcc[idx].nick);
      botnet_send_join_idx(idx, dcc[idx].u.file->chat->channel);
      return 0;			/* failed */
    }
  }
  filedb_close(f);
  dprintf(idx, "%s: /%s\n\n", FILES_CURDIR, dcc[idx].u.file->dir);
  return 1;
}

static void cmd_sort(int idx, char *par)
{
  FILE *f;
  struct userrec *u = get_user_by_handle(userlist, dcc[idx].nick);
  char *p;

  putlog(LOG_FILES, "*", "files: #%s# sort", dcc[idx].nick);
  p = get_user(&USERENTRY_DCCDIR, u);
  /* does this dir even exist any more? */
  f = filedb_open(p, 1);
  if (f == NULL) {
    set_user(&USERENTRY_DCCDIR, u, NULL);
    f = filedb_open("", 1);
    if (!f) {
      dprintf(idx, FILES_ILLDIR);
      return;
    }
  }
  filedb_close(f);
  dprintf(idx, "Current directory has been sorted.\n");
}

/* given current directory, and the desired changes, fill 'real' with
 * the new current directory.  check directory parmissions along the
 * way.  return 1 if the change can happen, 0 if not. */
static int resolve_dir(char *current, char *change, char *real, int idx)
{
  char elem[512], s[1024], new[1024], work[1024], *p;
  FILE *f;
  filedb fdb;
  struct flag_record user =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  struct flag_record req =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
  int ret;
  long i = 0;

  Context;
  strncpy(real, current, DIRMAX);
  real[DIRMAX] = 0;
  strcpy(new, change);
  if (!new[0])
    return 1;			/* no change? */
  if (new[0] == '/') {
    /* EVERYONE has access here */
    real[0] = 0;
    strcpy(new, &new[1]);
  }
  /* cycle thru the elements */
  strcat(new, "/");
  p = strchr(new, '/');
  while (p != NULL) {
    *p = 0;
    p++;
    strcpy(elem, new);
    strcpy(new, p);
    if (!(strcmp(elem, ".")) || (!elem[0])) {	/* do nothing */
    } else if (!strcmp(elem, "..")) {	/* go back */
      /* always allowed */
      p = strrchr(real, '/');
      if (p == NULL) {
	/* can't go back from here? */
	if (!real[0]) {
	  strncpy(real, current, DIRMAX);
	  real[DIRMAX] = 0;
	  return 0;
	}
	real[0] = 0;
      } else
	*p = 0;
    } else {
      /* allowed access here? */
      f = filedb_open(real, 0);
      if (f == NULL) {
	/* non-existent starting point! */
	strncpy(real, current, DIRMAX);
	real[DIRMAX] = 0;
	return 0;
      }
      ret = findmatch(f, elem, &i, &fdb);
      filedb_close(f);
      if (!ret) {
	/* non-existent */
	strncpy(real, current, DIRMAX);
	real[DIRMAX] = 0;
	return 0;
      }
      if (!(fdb.stat & FILE_DIR) || fdb.sharelink[0]) {
	/* not a dir */
	strncpy(real, current, DIRMAX);
	real[DIRMAX] = 0;
	return 0;
      }
      if (idx >= 0)
	get_user_flagrec(dcc[idx].user, &user, fdb.chname);
      else
	user.global = USER_OWNER | USER_BOT | USER_MASTER |
	USER_OP | USER_FRIEND;

      break_down_flags(fdb.flags_req, &req, NULL);
      if (!flagrec_ok(&req, &user)) {
	strncpy(real, current, DIRMAX);
	real[DIRMAX] = 0;
	return 0;
      }
      strcpy(s, real);
      if (s[0])
	if (s[strlen(s) - 1] != '/')
	  strcat(s, "/");
      sprintf(work, "%s%s", s, elem);
      strncpy(real, work, DIRMAX);
      real[DIRMAX] = 0;
      sprintf(s, "%s%s", dccdir, real);
    }
    p = strchr(new, '/');
  }
  /* sanity check: does this dir exist? */
  sprintf(s, "%s%s", dccdir, real);
  f = fopen(s, "r");
  if (f == NULL)
    return 0;
  fclose(f);
  Context;
  return 1;
}

static void incr_file_gots(char *ppath)
{
  char *p, path[256], destdir[121], fn[81];
  filedb fdb;
  FILE *f;
  long where = 0;

  /* absolute dir?  probably a tcl script sending it, and it might not
   * be in the file system at all, so just leave it alone */
  if ((ppath[0] == '*') || (ppath[0] == '/'))
    return;
  strcpy(path, ppath);
  p = strrchr(path, '/');
  if (p != NULL) {
    *p = 0;
    strncpy(destdir, path, 120);
    destdir[120] = 0;
    strncpy(fn, p + 1, 80);
    fn[80] = 0;
    *p = '/';
  } else {
    destdir[0] = 0;
    strncpy(fn, path, 80);
    fn[80] = 0;
  }
  f = filedb_open(destdir, 0);
  if (!f)
    return;			/* not my concern, then */
  if (findmatch(f, fn, &where, &fdb)) {
    fdb.gots++;
    fseek(f, where, SEEK_SET);
    fwrite(&fdb, sizeof(filedb), 1, f);
  }
  filedb_close(f);
}

/*** COMMANDS ***/

static void cmd_pwd(int idx, char *par)
{
  putlog(LOG_FILES, "*", "files: #%s# pwd", dcc[idx].nick);
  dprintf(idx, "%s: /%s\n", FILES_CURDIR, dcc[idx].u.file->dir);
}

static void cmd_pending(int idx, char *par)
{
  show_queued_files(idx);
  putlog(LOG_FILES, "*", "files: #%s# pending", dcc[idx].nick);
}

static void cmd_cancel(int idx, char *par)
{
  if (!par[0]) {
    dprintf(idx, "%s: cancel <file-mask>\n", USAGE);
    return;
  }
  fileq_cancel(idx, par);
  putlog(LOG_FILES, "*", "files: #%s# cancel %s", dcc[idx].nick, par);
}

static void cmd_chdir(int idx, char *msg)
{
  char s[DIRLEN];

  if (!msg[0]) {
    dprintf(idx, "%s: cd <new-dir>\n", USAGE);
    return;
  }
  if (!resolve_dir(dcc[idx].u.file->dir, msg, s, idx)) {
    dprintf(idx, FILES_NOSUCHDIR);
    return;
  }
  strncpy(dcc[idx].u.file->dir, s, 160);
  dcc[idx].u.file->dir[160] = 0;
  set_user(&USERENTRY_DCCDIR, dcc[idx].user, dcc[idx].u.file->dir);
  putlog(LOG_FILES, "*", "files: #%s# cd /%s", dcc[idx].nick,
	 dcc[idx].u.file->dir);
  dprintf(idx, "%s: /%s\n", FILES_NEWCURDIR, dcc[idx].u.file->dir);
  Context;
}

static void files_ls(int idx, char *par, int showall)
{
  char *p, s[DIRLEN], destdir[DIRLEN], mask[81];
  FILE *f;

  Context;
  if (par[0]) {
    putlog(LOG_FILES, "*", "files: #%s# ls %s", dcc[idx].nick, par);
    p = strrchr(par, '/');
    if (p != NULL) {
      *p = 0;
      strncpy(s, par, DIRMAX);
      s[DIRMAX] = 0;
      strncpy(mask, p + 1, 80);
      mask[80] = 0;
      if (!resolve_dir(dcc[idx].u.file->dir, s, destdir, idx)) {
	dprintf(idx, FILES_ILLDIR);
	return;
      }
    } else {
      strcpy(destdir, dcc[idx].u.file->dir);
      strncpy(mask, par, 80);
      mask[80] = 0;
    }
    /* might be 'ls dir'? */
    if (resolve_dir(destdir, mask, s, idx)) {
      /* aha! it was! */
      strcpy(destdir, s);
      strcpy(mask, "*");
    }
    f = filedb_open(destdir, 0);
    if (!f) {
      dprintf(idx, FILES_ILLDIR);
      return;
    }
    filedb_ls(f, idx, mask, showall);
    filedb_close(f);
  } else {
    putlog(LOG_FILES, "*", "files: #%s# ls", dcc[idx].nick);
    f = filedb_open(dcc[idx].u.file->dir, 0);
    if (!f) {
      dprintf(idx, FILES_ILLDIR);
      return;
    }
    filedb_ls(f, idx, "*", showall);
    filedb_close(f);
  }
  Context;
}

static void cmd_ls(int idx, char *par)
{
  files_ls(idx, par, 0);
}

static void cmd_lsa(int idx, char *par)
{
  files_ls(idx, par, 1);
}

static void cmd_get(int idx, char *par)
{
  int ok = 0, ok2 = 1, i;
  char *p, *what, destdir[DIRLEN], s[DIRLEN];
  filedb fdb;
  FILE *f;
  long where = 0;

  if (!par[0]) {
    dprintf(idx, "%s: get <file(s)> [nickname]\n", USAGE);
    return;
  }
  what = newsplit(&par);
  if (strlen(par) > NICKMAX) {
    dprintf(idx, FILES_BADNICK);
    return;
  }
  p = strrchr(what, '/');
  if (p != NULL) {
    *p = 0;
    strncpy(s, what, 120);
    s[120] = 0;
    strcpy(what, p + 1);
    if (!resolve_dir(dcc[idx].u.file->dir, s, destdir, idx)) {
      dprintf(idx, FILES_ILLDIR);
      return;
    }
  } else {
    strncpy(destdir, dcc[idx].u.file->dir, 121);
    destdir[121] = 0;
  }
  f = filedb_open(destdir, 0);
  if (!f) {
    dprintf(idx, FILES_ILLDIR);
    return;
  }
  where = 0L;
  if (!findmatch(f, what, &where, &fdb)) {
    filedb_close(f);
    dprintf(idx, FILES_NOMATCH);
    return;
  }
  while (ok2) {
    if (!(fdb.stat & (FILE_HIDDEN | FILE_DIR))) {
      ok = 1;
      if (fdb.sharelink[0]) {
	char bot[121], whoto[NICKLEN];

	/* this is a link to a file on another bot... */
	splitc(bot, fdb.sharelink, ':');
	if (!strcasecmp(bot, botnetnick)) {
	  dprintf(idx, "Can't get that file, it's linked to this bot!\n");
	} else if (!in_chain(bot)) {
	  dprintf(idx, FILES_NOTAVAIL, fdb.filename);
	} else {
	  i = nextbot(bot);
	  strcpy(whoto, par);
	  if (!whoto[0])
	    strcpy(whoto, dcc[idx].nick);
	  simple_sprintf(s, "%d:%s@%s", dcc[idx].sock, whoto, botnetnick);
	  botnet_send_filereq(i, s, bot, fdb.sharelink);
	  dprintf(idx, FILES_REQUESTED, fdb.sharelink, bot);
	  /* increase got count now (or never) */
	  fdb.gots++;
	  sprintf(s, "%s:%s", bot, fdb.sharelink);
	  strcpy(fdb.sharelink, s);
	  fseek(f, where, SEEK_SET);
	  fwrite(&fdb, sizeof(filedb), 1, f);
	}
      } else {
	do_dcc_send(idx, destdir, fdb.filename, par);
	/* don't increase got count till later */
      }
    }
    where += sizeof(filedb);
    ok2 = findmatch(f, what, &where, &fdb);
  }
  filedb_close(f);
  if (!ok)
    dprintf(idx, FILES_NOMATCH);
  else
    putlog(LOG_FILES, "*", "files: #%s# get %s %s", dcc[idx].nick, what, par);
}

static void cmd_file_help(int idx, char *par)
{
  char s[1024];
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.file->chat->con_chan);
  if (par[0]) {
    putlog(LOG_FILES, "*", "files: #%s# help %s", dcc[idx].nick, par);
    sprintf(s, "filesys/%s", par);
    s[256] = 0;
    tellhelp(idx, s, &fr, 0);
  } else {
    putlog(LOG_FILES, "*", "files: #%s# help", dcc[idx].nick);
    tellhelp(idx, "filesys/help", &fr, 0);
  }
}

static void cmd_hide(int idx, char *par)
{
  FILE *f;
  filedb fdb;
  long where = 0;
  int ok = 0, ret;

  if (!par[0]) {
    dprintf(idx, "%s: hide <file(s)>\n", USAGE);
    return;
  }
  where = 0L;
  f = filedb_open(dcc[idx].u.file->dir, 0);
  if (!f) {
    dprintf(idx, FILES_ILLDIR);
    return;
  }
  ret = findmatch(f, par, &where, &fdb);
  if (!ret) {
    filedb_close(f);
    dprintf(idx, FILES_NOMATCH);
    return;
  }
  while (ret) {
    if (!(fdb.stat & FILE_HIDDEN)) {
      fdb.stat |= FILE_HIDDEN;
      ok++;
      dprintf(idx, "%s: %s\n", FILES_HID, fdb.filename);
      fseek(f, where, SEEK_SET);
      fwrite(&fdb, sizeof(filedb), 1, f);
    }
    where += sizeof(filedb);
    ret = findmatch(f, par, &where, &fdb);
  }
  filedb_close(f);
  if (!ok)
    dprintf(idx, FILES_NOMATCH);
  else {
    putlog(LOG_FILES, "*", "files: #%s# hide %s", dcc[idx].nick, par);
    if (ok > 1)
      dprintf(idx, "%s %d file%s.\n", FILES_HID, ok, ok == 1 ? "" : "s");
  }
}

static void cmd_unhide(int idx, char *par)
{
  FILE *f;
  filedb fdb;
  long where;
  int ok = 0, ret;

  if (!par[0]) {
    dprintf(idx, "%s: unhide <file(s)>\n", USAGE);
    return;
  }
  where = 0L;
  f = filedb_open(dcc[idx].u.file->dir, 0);
  if (!f) {
    dprintf(idx, FILES_ILLDIR);
    return;
  }
  ret = findmatch(f, par, &where, &fdb);
  if (!ret) {
    filedb_close(f);
    dprintf(idx, FILES_NOMATCH);
    return;
  }
  while (ret) {
    if (fdb.stat & FILE_HIDDEN) {
      fdb.stat &= ~FILE_HIDDEN;
      ok++;
      dprintf(idx, "%s: %s\n", FILES_UNHID, fdb.filename);
      fseek(f, where, SEEK_SET);
      fwrite(&fdb, sizeof(filedb), 1, f);
    }
    where += sizeof(filedb);
    ret = findmatch(f, par, &where, &fdb);
  }
  filedb_close(f);
  if (!ok)
    dprintf(idx, FILES_NOMATCH);
  else {
    putlog(LOG_FILES, "*", "files: #%s# unhide %s", dcc[idx].nick, par);
    if (ok > 1)
      dprintf(idx, "%s %d file%s.\n", FILES_UNHID, ok, ok == 1 ? "" : "s");
  }
}

static void cmd_share(int idx, char *par)
{
  FILE *f;
  filedb fdb;
  long where;
  int ok = 0, ret;

  if (!par[0]) {
    dprintf(idx, "%s: share <file(s)>\n", USAGE);
    return;
  }
  where = 0L;
  f = filedb_open(dcc[idx].u.file->dir, 0);
  if (!f) {
    dprintf(idx, FILES_ILLDIR);
    return;
  }
  ret = findmatch(f, par, &where, &fdb);
  if (!ret) {
    filedb_close(f);
    dprintf(idx, FILES_NOMATCH);
    return;
  }
  while (ret) {
    if (!(fdb.stat & (FILE_HIDDEN | FILE_DIR | FILE_SHARE))) {
      fdb.stat |= FILE_SHARE;
      ok++;
      dprintf(idx, "%s: %s\n", FILES_SHARED, fdb.filename);
      fseek(f, where, SEEK_SET);
      fwrite(&fdb, sizeof(filedb), 1, f);
    }
    where += sizeof(filedb);
    ret = findmatch(f, par, &where, &fdb);
  }
  filedb_close(f);
  if (!ok)
    dprintf(idx, FILES_NOMATCH);
  else {
    putlog(LOG_FILES, "*", "files: #%s# share %s", dcc[idx].nick, par);
    if (ok > 1)
      dprintf(idx, "%s %d file%s.\n", FILES_SHARED, ok, ok == 1 ? "" : "s");
  }
}

static void cmd_unshare(int idx, char *par)
{
  FILE *f;
  filedb fdb;
  long where;
  int ok = 0, ret;

  if (!par[0]) {
    dprintf(idx, "%s: unshare <file(s)>\n", USAGE);
    return;
  }
  where = 0L;
  f = filedb_open(dcc[idx].u.file->dir, 0);
  if (!f) {
    dprintf(idx, FILES_ILLDIR);
    return;
  }
  ret = findmatch(f, par, &where, &fdb);
  if (!ret) {
    filedb_close(f);
    dprintf(idx, FILES_NOMATCH);
    return;
  }
  while (ret) {
    if ((fdb.stat & FILE_SHARE) &&
	!(fdb.stat & (FILE_DIR | FILE_HIDDEN))) {
      fdb.stat &= ~FILE_SHARE;
      ok++;
      dprintf(idx, "%s: %s\n", FILES_UNSHARED, fdb.filename);
      fseek(f, where, SEEK_SET);
      fwrite(&fdb, sizeof(filedb), 1, f);
    }
    where += sizeof(filedb);
    ret = findmatch(f, par, &where, &fdb);
  }
  filedb_close(f);
  if (!ok)
    dprintf(idx, FILES_NOMATCH);
  else {
    putlog(LOG_FILES, "*", "files: #%s# unshare %s", dcc[idx].nick, par);
    if (ok > 1)
      dprintf(idx, "%s %d file%s.\n", FILES_UNSHARED, ok,
	      ok == 1 ? "" : "s");
  }
}

/* link a file from another bot */
static void cmd_ln(int idx, char *par)
{
  char *share, newpath[DIRLEN], newfn[81], *p;
  FILE *f;
  filedb fdb;
  long where = 0;
  int ret;

  share = newsplit(&par);
  if (strlen(share) > 60)
    share[60] = 0;
  /* correct format? */
  if (!(p = strchr(share, ':')) || !par[0])
    dprintf(idx, "%s: ln <bot:path> <localfile>\n", USAGE);
  else if (p[1] != '/')
    dprintf(idx, "Links to other bots must have absolute paths.\n");
  else {
    if ((p = strrchr(par, '/'))) {
      *p = 0;
      strncpy(newfn, p + 1, 80);
      newfn[80] = 0;
      if (!resolve_dir(dcc[idx].u.file->dir, par, newpath, idx)) {
	dprintf(idx, FILES_NOSUCHDIR);
	return;
      }
    } else {
      strncpy(newpath, dcc[idx].u.file->dir, 121);
      newpath[121] = 0;
      strncpy(newfn, par, 80);
      newfn[80] = 0;
    }
    f = filedb_open(newpath, 0);
    if (!f) {
      dprintf(idx, FILES_ILLDIR);
      return;
    }
    ret = findmatch(f, newfn, &where, &fdb);
    if (ret) {
      if (!fdb.sharelink[0]) {
	dprintf(idx, FILES_NORMAL, newfn);
	filedb_close(f);
      } else {
	strcpy(fdb.sharelink, share);
	fseek(f, where, SEEK_SET);
	fwrite(&fdb, sizeof(filedb), 1, f);
	filedb_close(f);
	dprintf(idx, FILES_CHGLINK, share);
	putlog(LOG_FILES, "*", "files: #%s# ln %s %s",
	       dcc[idx].nick, par, share);
      }
    } else {
      /* new entry */
      where = findempty(f);
      fdb.version = FILEVERSION;
      fdb.desc[0] = 0;
      fdb.flags_req[0] = 0;
      fdb.chname[0] = 0;
      fdb.size = 0;
      fdb.gots = 0;
      strncpy(fdb.filename, newfn, 30);
      fdb.filename[30] = 0;
      strncpy(fdb.uploader, dcc[idx].nick, HANDLEN);
      fdb.uploader[HANDLEN] = 0;
      fdb.uploaded = now;
      strcpy(fdb.sharelink, share);
      fdb.stat = 0;
      fseek(f, where, SEEK_SET);
      fwrite(&fdb, sizeof(filedb), 1, f);
      filedb_close(f);
      dprintf(idx, "%s %s -> %s\n", FILES_ADDLINK, fdb.filename, share);
      putlog(LOG_FILES, "*", "files: #%s# ln /%s%s%s %s", dcc[idx].nick,
	     newpath, newpath[0] ? "/" : "", newfn, share);
    }
  }
}

static void cmd_desc(int idx, char *par)
{
  char *fn, desc[186], *p, *q;
  int ok = 0, lin, ret;
  FILE *f;
  filedb fdb;
  long where;

  fn = newsplit(&par);
  if (!fn[0]) {
    dprintf(idx, "%s: desc <filename> <new description>\n", USAGE);
    return;
  }
  /* fix up desc */
  strncpy(desc, par, 185);
  desc[185] = 0;
  strcat(desc, "|");
  /* replace | with linefeeds, limit 5 lines */
  lin = 0;
  q = desc;
  while ((*q <= 32) && (*q))
    strcpy(q, &q[1]);		/* zapf leading spaces */
  p = strchr(q, '|');
  while (p != NULL) {
    /* check length */
    *p = 0;
    if (strlen(q) > 60) {
      /* cut off at last space or truncate */
      *p = '|';
      p = q + 60;
      while ((*p != ' ') && (p != q))
	p--;
      if (p == q)
	*(q + 60) = '|';	/* no space, so truncate it */
      else
	*p = '|';
      p = strchr(q, '|');	/* go back, find my place, and continue */
    }
    *p = '\n';
    q = p + 1;
    lin++;
    while ((*q <= 32) && (*q))
      strcpy(q, &q[1]);
    if (lin == 5) {
      *p = 0;
      p = NULL;
    } else
      p = strchr(q, '|');
  }
  /* (whew!) */
  if (desc[strlen(desc) - 1] == '\n')
    desc[strlen(desc) - 1] = 0;
  f = filedb_open(dcc[idx].u.file->dir, 0);
  if (!f) {
    dprintf(idx, FILES_ILLDIR);
    return;
  }
  where = 0L;
  ret = findmatch(f, fn, &where, &fdb);
  if (!ret) {
    filedb_close(f);
    dprintf(idx, FILES_NOMATCH);
    return;
  }
  while (ret) {
    if (!(fdb.stat & FILE_HIDDEN)) {
      ok = 1;
      if ((!(dcc[idx].user->flags & USER_JANITOR)) &&
	  (strcasecmp(fdb.uploader, dcc[idx].nick)))
	dprintf(idx, FILES_NOTOWNER, fdb.filename);
      else {
	strcpy(fdb.desc, desc);
	fseek(f, where, SEEK_SET);
	fwrite(&fdb, sizeof(filedb), 1, f);
	if (par[0])
	  dprintf(idx, "%s: %s\n", FILES_CHANGED, fdb.filename);
	else
	  dprintf(idx, "%s: %s\n", FILES_BLANKED, fdb.filename);
      }
    }
    where += sizeof(filedb);
    ret = findmatch(f, fn, &where, &fdb);
  }
  filedb_close(f);
  if (!ok)
    dprintf(idx, FILES_NOMATCH);
  else
    putlog(LOG_FILES, "*", "files: #%s# desc %s", dcc[idx].nick, fn);
}

static void cmd_rm(int idx, char *par)
{
  FILE *f;
  filedb fdb;
  long where;
  int ok = 0, ret;
  char s[256];

  if (!par[0]) {
    dprintf(idx, "%s: rm <file(s)>\n", USAGE);
    return;
  }
  f = filedb_open(dcc[idx].u.file->dir, 0);
  if (!f) {
    dprintf(idx, FILES_ILLDIR);
    return;
  }
  where = 0L;
  ret = findmatch(f, par, &where, &fdb);
  if (!ret) {
    filedb_close(f);
    dprintf(idx, FILES_NOMATCH);
    return;
  }
  while (ret) {
    if (!(fdb.stat & (FILE_HIDDEN | FILE_DIR))) {
      sprintf(s, "%s%s/%s", dccdir, dcc[idx].u.file->dir, fdb.filename);
      fdb.stat |= FILE_UNUSED;
      ok++;
      fseek(f, where, SEEK_SET);
      fwrite(&fdb, sizeof(filedb), 1, f);
      /* shared file links won't be able to be unlinked */
      if (!(fdb.sharelink[0]))
	unlink(s);
      dprintf(idx, "%s: %s\n", FILES_ERASED, fdb.filename);
    }
    where += sizeof(filedb);
    ret = findmatch(f, par, &where, &fdb);
  }
  filedb_close(f);
  if (!ok)
    dprintf(idx, FILES_NOMATCH);
  else {
    putlog(LOG_FILES, "*", "files: #%s# rm %s", dcc[idx].nick, par);
    if (ok > 1)
      dprintf(idx, "%s %d file%s.\n", FILES_ERASED, ok, ok == 1 ? "" : "s");
  }
}

static void cmd_mkdir(int idx, char *par)
{
  char *name, *flags, *chan, s[512];
  FILE *f;
  filedb fdb;
  long where = 0;
  int ret;

  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  if (!par[0]) {
    dprintf(idx, "%s: mkdir <dir> [required-flags] [channel]\n", USAGE);
    return;
  }
  name = newsplit(&par);
  ret = strlen(name);
  if (ret > 60)
    name[(ret = 60)] = 0;
  if (name[ret] == '/')
    name[ret] = 0;
  if (strchr(name, '/'))
    dprintf(idx, "You can only create directories in the current directory\n");
  else {
    flags = newsplit(&par);
    chan = newsplit(&par);
    if (!chan[0] && flags[0] && (strchr(CHANMETA, flags[0]) != NULL)) {
      /* Need some extra checking here to makesure we dont mix up
       * the flags with a +channel. <cybah> */
      if(!findchan(flags) && flags[0] != '+') {
	dprintf(idx, "Invalid channel!\n");
	return;
      } else if(findchan(flags)) {
	/* flags is a channel. */
	chan = flags;
	flags = par;
      }				/* (else) Couldnt find the channel and
				 * flags[0] is a '+', these are flags. */
    }
    if (chan[0] && !findchan(chan)) {
      dprintf(idx, "Invalid channel!\n");
      return;
    }
    f = filedb_open(dcc[idx].u.file->dir, 0);
    if (!f) {
      dprintf(idx, FILES_ILLDIR);
      return;
    }
    ret = findmatch(f, name, &where, &fdb);
    if (!ret) {
      sprintf(s, "%s%s/%s", dccdir, dcc[idx].u.file->dir, name);
      if (mkdir(s, 0755) != 0) {
	dprintf(idx, FAILED);
	filedb_close(f);
	return;
      }
      fdb.version = FILEVERSION;
      fdb.stat = FILE_DIR;
      fdb.desc[0] = 0;
      fdb.uploader[0] = 0;
      strcpy(fdb.filename, name);
      fdb.flags_req[0] = 0;
      fdb.chname[0] = 0;
      fdb.uploaded = now;
      fdb.size = 0;
      fdb.gots = 0;
      fdb.sharelink[0] = 0;
      dprintf(idx, "%s /%s%s%s\n", FILES_CREADIR, dcc[idx].u.file->dir,
	      dcc[idx].u.file->dir[0] ? "/" : "", name);
      where = findempty(f);
    } else if (!(fdb.stat & FILE_DIR)) {
      dprintf(idx, FILES_NOSUCHDIR);
      filedb_close(f);
      return;
    }
    if (flags[0]) {
      break_down_flags(flags, &fr, NULL);
      build_flags(s, &fr, NULL);
      strncpy(fdb.flags_req, s, 21);
      fdb.flags_req[21] = 0;
      dprintf(idx, FILES_CHGACCESS, name, s);
    } else if (!chan[0]) {
      fdb.flags_req[0] = 0;
      dprintf(idx, FILES_CHGNACCESS, name);
    }
    if (chan[0]) {
      strncpy(fdb.chname, chan, 80);
      fdb.chname[80] = 0;
      dprintf(idx, "Access set to channel: %s\n", chan);
    } else if (!flags[0]) {
      fdb.chname[0] = 0;
      dprintf(idx, "Access set to all channels.\n");
    }
    fseek(f, where, SEEK_SET);
    fwrite(&fdb, sizeof(filedb), 1, f);
    filedb_close(f);
    putlog(LOG_FILES, "*", "files: #%s# mkdir %s %s", dcc[idx].nick, name, par);
  }
}

static void cmd_rmdir(int idx, char *par)
{
  FILE *f;
  filedb fdb;
  long where = 0;
  char s[256], name[80];

  strncpy(name, par, 80);
  name[80] = 0;
  if (name[strlen(name) - 1] == '/')
    name[strlen(name) - 1] = 0;
  if (strchr(name, '/'))
    dprintf(idx, "You can only create directories in the current directory\n");
  else {
    f = filedb_open(dcc[idx].u.file->dir, 0);
    if (!f) {
      dprintf(idx, FILES_ILLDIR);
      return;
    }
    if (!findmatch(f, name, &where, &fdb)) {
      dprintf(idx, FILES_NOSUCHDIR);
      filedb_close(f);
      return;
    }
    if (!(fdb.stat & FILE_DIR)) {
      dprintf(idx, FILES_NOSUCHDIR);
      filedb_close(f);
      return;
    }
    /* erase '.filedb' and '.files' if they exist */
    sprintf(s, "%s%s/%s/.filedb", dccdir, dcc[idx].u.file->dir, name);
    unlink(s);
    sprintf(s, "%s%s/%s/.files", dccdir, dcc[idx].u.file->dir, name);
    unlink(s);
    sprintf(s, "%s%s/%s", dccdir, dcc[idx].u.file->dir, name);
    if (rmdir(s) == 0) {
      dprintf(idx, "%s /%s%s%s\n", FILES_REMDIR, dcc[idx].u.file->dir,
	      dcc[idx].u.file->dir[0] ? "/" : "", name);
      fdb.stat |= FILE_UNUSED;
      fseek(f, where, SEEK_SET);
      fwrite(&fdb, sizeof(filedb), 1, f);
      filedb_close(f);
      putlog(LOG_FILES, "*", "files: #%s# rmdir %s", dcc[idx].nick, name);
      return;
    }
    dprintf(idx, FAILED);
    filedb_close(f);
  }
}

static void cmd_mv_cp(int idx, char *par, int copy)
{
  char *p, *fn, oldpath[DIRLEN], s[161], s1[161], newfn[161];
  char newpath[DIRLEN];
  int ok, only_first, skip_this, ret, ret2;
  FILE *f, *g;
  filedb fdb, z;
  long where, gwhere, wherez;

  fn = newsplit(&par);
  if (!par[0]) {
    dprintf(idx, "%s: %s <oldfilepath> <newfilepath>\n",
	    USAGE, copy ? "cp" : "mv");
    return;
  }
  p = strrchr(fn, '/');
  if (p != NULL) {
    *p = 0;
    strncpy(s, fn, 160);
    s[160] = 0;
    strcpy(fn, p + 1);
    if (!resolve_dir(dcc[idx].u.file->dir, s, oldpath, idx)) {
      dprintf(idx, FILES_ILLSOURCE);
      return;
    }
  } else
    strcpy(oldpath, dcc[idx].u.file->dir);
  strncpy(s, par, 160);
  s[160] = 0;
  if (!resolve_dir(dcc[idx].u.file->dir, s, newpath, idx)) {
    /* destination is not just a directory */
    p = strrchr(s, '/');
    if (p == NULL) {
      strcpy(newfn, s);
      s[0] = 0;
    } else {
      *p = 0;
      strcpy(newfn, p + 1);
    }
    if (!resolve_dir(dcc[idx].u.file->dir, s, newpath, idx)) {
      dprintf(idx, FILES_ILLDEST);
      return;
    }
  } else
    newfn[0] = 0;
  /* stupidness checks */
  if ((!strcmp(oldpath, newpath)) &&
      ((!newfn[0]) || (!strcmp(newfn, fn)))) {
    dprintf(idx, FILES_STUPID, copy ? FILES_COPY : FILES_MOVE);
    return;
  }
  /* be aware of 'cp * this.file' possibility: ONLY COPY FIRST ONE */
  if (((strchr(fn, '?') != NULL) || (strchr(fn, '*') != NULL)) && (newfn[0]))
    only_first = 1;
  else
    only_first = 0;
  f = filedb_open(oldpath, 0);
  if (!f) {
    dprintf(idx, FILES_ILLDIR);
    return;
  }
  if (!strcmp(oldpath, newpath))
    g = NULL;
  else {
    g = filedb_open(newpath, 0);
    if (!g) {
      dprintf(idx, FILES_ILLDIR);
      return;
    }
  }
  where = 0L;
  ok = 0;
  ret = findmatch(f, fn, &where, &fdb);
  if (!ret) {
    dprintf(idx, FILES_NOMATCH);
    filedb_close(f);
    if (g != NULL)
      filedb_close(g);
    return;
  }
  while (ret) {
    skip_this = 0;
    if (!(fdb.stat & (FILE_HIDDEN | FILE_DIR))) {
      sprintf(s, "%s%s%s%s", dccdir, oldpath,
	      oldpath[0] ? "/" : "", fdb.filename);
      sprintf(s1, "%s%s%s%s", dccdir, newpath,
	      newpath[0] ? "/" : "", newfn[0] ? newfn : fdb.filename);
      if (!strcmp(s, s1)) {
	dprintf(idx, "%s /%s%s%s %s\n", FILES_SKIPSTUPID,
		copy ? FILES_COPY : FILES_MOVE, newpath,
		newpath[0] ? "/" : "", newfn[0] ? newfn : fdb.filename);
	skip_this = 1;
      }
      /* check for existence of file with same name in new dir */
      wherez = 0;
      if (!g)
	ret2 = findmatch(f, newfn[0] ? newfn : fdb.filename, &wherez,
			 &z);
      else
	ret2 = findmatch(g, newfn[0] ? newfn : fdb.filename, &wherez,
			 &z);
      if (ret) {
	/* it's ok if the entry in the new dir is a normal file (we'll
	 * just scrap the old entry and overwrite the file) -- but if
	 * it's a directory, this file has to be skipped */
	if (z.stat & FILE_DIR) {
	  /* skip */
	  skip_this = 1;
	  dprintf(idx, "%s /%s%s%s %s\n", FILES_DEST,
		  newpath, newpath[0] ? "/" : "",
		  newfn[0] ? newfn : fdb.filename,
		  FILES_EXISTDIR);
	} else {
	  z.stat |= FILE_UNUSED;
	  if (!g) {
	    fseek(f, wherez, SEEK_SET);
	    fwrite(&z, sizeof(filedb), 1, f);
	  } else {
	    fseek(g, wherez, SEEK_SET);
	    fwrite(&z, sizeof(filedb), 1, g);
	  }
	}
      }
      if (!skip_this) {
	if ((fdb.sharelink[0]) || (copyfile(s, s1) == 0)) {
	  /* raw file moved okay: create new entry for it */
	  ok++;
	  if (!g)
	    gwhere = findempty(f);
	  else
	    gwhere = findempty(g);
	  z.version = FILEVERSION;
	  z.stat = fdb.stat;
	  strcpy(z.flags_req, fdb.flags_req);
	  strcpy(z.chname, fdb.chname);
	  strcpy(z.filename, fdb.filename);
	  strcpy(z.desc, fdb.desc);
	  if (newfn[0])
	    strcpy(z.filename, newfn);
	  strcpy(z.uploader, fdb.uploader);
	  z.uploaded = fdb.uploaded;
	  z.size = fdb.size;
	  z.gots = fdb.gots;
	  strcpy(z.sharelink, fdb.sharelink);
	  if (!g) {
	    fseek(f, gwhere, SEEK_SET);
	    fwrite(&z, sizeof(filedb), 1, f);
	  } else {
	    fseek(g, gwhere, SEEK_SET);
	    fwrite(&z, sizeof(filedb), 1, g);
	  }
	  if (!copy) {
	    unlink(s);
	    fdb.stat |= FILE_UNUSED;
	    fseek(f, where, SEEK_SET);
	    fwrite(&fdb, sizeof(filedb), 1, f);
	  }
	  dprintf(idx, "%s /%s%s%s to /%s%s%s\n",
		  copy ? FILES_COPIED : FILES_MOVED,
		  oldpath, oldpath[0] ? "/" : "", fdb.filename,
		  newpath, newpath[0] ? "/" : "",
		  newfn[0] ? newfn : fdb.filename);
	} else
	  dprintf(idx, "%s /%s%s%s\n", FILES_CANTWRITE,
		  newpath, newpath[0] ? "/" : "",
		  newfn[0] ? newfn : fdb.filename);
      }
    }
    where += sizeof(filedb);
    ret = findmatch(f, fn, &where, &fdb);
    if ((ok) && (only_first))
      ret = 0;
  }
  if (!ok)
    dprintf(idx, FILES_NOMATCH);
  else {
    putlog(LOG_FILES, "*", "files: #%s# %s %s%s%s %s", dcc[idx].nick,
	   copy ? "cp" : "mv", oldpath, oldpath[0] ? "/" : "", fn, par);
    if (ok > 1)
      dprintf(idx, "%s %d file%s.\n",
	      copy ? FILES_COPIED : FILES_MOVED, ok, ok == 1 ? "" : "s");
  }
  filedb_close(f);
  if (g)
    filedb_close(g);
}

static void cmd_mv(int idx, char *par)
{
  cmd_mv_cp(idx, par, 0);
}

static void cmd_cp(int idx, char *par)
{
  cmd_mv_cp(idx, par, 1);
}

static int cmd_stats(int idx, char *par)
{
  putlog(LOG_FILES, "*", "#%s# stats", dcc[idx].nick);
  tell_file_stats(idx, dcc[idx].nick);
  return 0;
}

static int cmd_filestats(int idx, char *par)
{
  char *nick;
  struct userrec *u;

  Context;
  if (!par[0]) {
    dprintf(idx, "Usage: filestats <user>\n");
    return 0;
  }
  nick = newsplit(&par);
  putlog(LOG_FILES, "*", "#%s# filestats %s", dcc[idx].nick, nick);
  if (nick[0] == 0)
    tell_file_stats(idx, dcc[idx].nick);
  else if (!(u = get_user_by_handle(userlist, nick)))
    dprintf(idx, "No such user.\n");
  else if (!strcmp(par, "clear") && dcc[idx].user &&
	   (dcc[idx].user->flags & USER_JANITOR)) {
    set_user (&USERENTRY_FSTAT, u, NULL);
    dprintf(idx, "Cleared filestats for %s.\n", nick);
  } else
    tell_file_stats(idx, nick);
  return 0;
}

/* this function relays the dcc call to cmd_note() in the notes module,
 * if loaded */
static void filesys_note(int idx, char *par)
{
  struct userrec *u = get_user_by_handle(userlist, dcc[idx].nick);
  module_entry *me = module_find("notes", 2, 1);

  if (me && me->funcs) {
    Function f = me->funcs[NOTES_CMD_NOTE];

    (f) (u, idx, par);
  } else {
    dprintf(idx, "Sending of notes is not supported.\n");
  }
}

static cmd_t myfiles[] =
{
  {"cancel", "", (Function) cmd_cancel, NULL},
  {"cd", "", (Function) cmd_chdir, NULL},
  {"chdir", "", (Function) cmd_chdir, NULL},
  {"cp", "j", (Function) cmd_cp, NULL},
  {"desc", "", (Function) cmd_desc, NULL},
  {"filestats", "j", cmd_filestats, NULL},
  {"get", "", (Function) cmd_get, NULL},
  {"help", "", (Function) cmd_file_help, NULL},
  {"hide", "j", (Function) cmd_hide, NULL},
  {"ln", "j", (Function) cmd_ln, NULL},
  {"ls", "", (Function) cmd_ls, NULL},
  {"lsa", "j", (Function) cmd_lsa, NULL},
  {"mkdir", "j", (Function) cmd_mkdir, NULL},
  {"mv", "j", (Function) cmd_mv, NULL},
  {"note", "", (Function) filesys_note, NULL},
  {"pending", "", (Function) cmd_pending, NULL},
  {"pwd", "", (Function) cmd_pwd, NULL},
  {"quit", "", (Function) CMD_LEAVE, NULL},
  {"rm", "j", (Function) cmd_rm, NULL},
  {"rmdir", "j", (Function) cmd_rmdir, NULL},
  {"share", "j", (Function) cmd_share, NULL},
  {"sort", "j", (Function) cmd_sort, NULL},
  {"stats", "", cmd_stats, NULL},
  {"unhide", "j", (Function) cmd_unhide, NULL},
  {"unshare", "j", (Function) cmd_unshare, NULL},
  {0, 0, 0, 0}
};

/***** Tcl stub functions *****/

static int files_get(int idx, char *fn, char *nick)
{
  int i;
  char *p, what[512], destdir[DIRLEN], s[256];
  filedb fdb;
  FILE *f;
  long where = 0;

  p = strrchr(fn, '/');
  if (p != NULL) {
    *p = 0;
    strncpy(s, fn, 120);
    s[120] = 0;
    strncpy(what, p + 1, 80);
    what[80] = 0;
    if (!resolve_dir(dcc[idx].u.file->dir, s, destdir, idx))
      return 0;
  } else {
    strncpy(destdir, dcc[idx].u.file->dir, 121);
    destdir[121] = 0;
    strncpy(what, fn, 80);
    what[80] = 0;
  }
  f = filedb_open(destdir, 0);
  if (!f) {
    dprintf(idx, FILES_ILLDIR);
    return 0;
  }
  if (!findmatch(f, what, &where, &fdb)) {
    filedb_close(f);
    return 0;
  }
  if (fdb.stat & (FILE_HIDDEN | FILE_DIR)) {
    filedb_close(f);
    return 0;
  }
  if (fdb.sharelink[0]) {
    char bot[121], whoto[NICKLEN];

    /* this is a link to a file on another bot... */
    splitc(bot, fdb.sharelink, ':');
    if (!strcasecmp(bot, botnetnick)) {
      /* linked to myself *duh* */
      filedb_close(f);
      return 0;
    } else if (!in_chain(bot)) {
      filedb_close(f);
      return 0;
    } else {
      i = nextbot(bot);
      strcpy(whoto, nick);
      if (!whoto[0])
	strcpy(whoto, dcc[idx].nick);
      simple_sprintf(s, "%d:%s@%s", dcc[idx].sock, whoto, botnetnick);
      botnet_send_filereq(i, s, bot, fdb.sharelink);
      dprintf(idx, FILES_REQUESTED, fdb.sharelink, bot);
      /* increase got count now (or never) */
      fdb.gots++;
      sprintf(s, "%s:%s", bot, fdb.sharelink);
      strcpy(fdb.sharelink, s);
      fseek(f, where, SEEK_SET);
      fwrite(&fdb, sizeof(filedb), 1, f);
      filedb_close(f);
      return 1;
    }
  }
  filedb_close(f);
  do_dcc_send(idx, destdir, fdb.filename, nick);
  /* don't increase got count till later */
  return 1;
}

static void files_setpwd(int idx, char *where)
{
  char s[DIRLEN];

  if (!resolve_dir(dcc[idx].u.file->dir, where, s, idx))
    return;
  strcpy(dcc[idx].u.file->dir, s);
  set_user(&USERENTRY_DCCDIR, get_user_by_handle(userlist, dcc[idx].nick),
	   dcc[idx].u.file->dir);
}
