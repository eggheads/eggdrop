/* 
 * tclfiles.c -- handles: Tcl stubs for file system commands moved here to 
 * support modules
 * dprintf'ized, 1aug1996
 */
/* 
 * This file is part of the eggdrop source code copyright (c) 1997 Robey
 * Pointer and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called COPYING
 * that was distributed with this code.
 */

static int tcl_getdesc STDVAR
{
  char s[186];

  BADARGS(3, 3, " dir file");
  filedb_getdesc(argv[1], argv[2], s);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_setdesc STDVAR
{
  BADARGS(4, 4, " dir file desc");
  filedb_setdesc(argv[1], argv[2], argv[3]);
  return TCL_OK;
}

static int tcl_getowner STDVAR
{
  char s[HANDLEN + 1];

  BADARGS(3, 3, " dir file");
  filedb_getowner(argv[1], argv[2], s);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_setowner STDVAR
{
  BADARGS(4, 4, " dir file owner");
  filedb_setowner(argv[1], argv[2], argv[3]);
  return TCL_OK;
}

static int tcl_getgots STDVAR
{
  int i;
  char s[10];

  BADARGS(3, 3, " dir file");
  i = filedb_getgots(argv[1], argv[2]);
  sprintf(s, "%d", i);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_setlink STDVAR
{
  BADARGS(4, 4, " dir file link");
  filedb_setlink(argv[1], argv[2], argv[3]);
  return TCL_OK;
}

static int tcl_getlink STDVAR
{
  char s[121];

  BADARGS(3, 3, " dir file");
  filedb_getlink(argv[1], argv[2], s);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_setpwd STDVAR
{
  int i, idx;

  BADARGS(3, 3, " idx dir");
  i = atoi(argv[1]);
  idx = findanyidx(i);
  if ((idx < 0) || (dcc[idx].type != &DCC_FILES)) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  files_setpwd(idx, argv[2]);

  return TCL_OK;
}

static int tcl_getpwd STDVAR
{
  int i, idx;

  BADARGS(2, 2, " idx");
  i = atoi(argv[1]);
  idx = findanyidx(i);
  if ((idx < 0) || (dcc[idx].type != &DCC_FILES)) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(irp, dcc[idx].u.file->dir, NULL);

  return TCL_OK;
}

static int tcl_getfiles STDVAR
{
  BADARGS(2, 2, " dir");
  filedb_getfiles(irp, argv[1]);
  return TCL_OK;
}

static int tcl_getdirs STDVAR
{
  BADARGS(2, 2, " dir");
  filedb_getdirs(irp, argv[1]);
  return TCL_OK;
}

static int tcl_hide STDVAR
{
  BADARGS(3, 3, " dir file");
  filedb_change(argv[1], argv[2], FILEDB_HIDE);
  return TCL_OK;
}

static int tcl_unhide STDVAR
{
  BADARGS(3, 3, " dir file");
  filedb_change(argv[1], argv[2], FILEDB_UNHIDE);
  return TCL_OK;
}

static int tcl_share STDVAR
{
  BADARGS(3, 3, " dir file");
  filedb_change(argv[1], argv[2], FILEDB_SHARE);
  return TCL_OK;
}

static int tcl_unshare STDVAR
{
  BADARGS(3, 3, " dir file");
  filedb_change(argv[1], argv[2], FILEDB_UNSHARE);
  return TCL_OK;
}

static int tcl_setflags STDVAR
{
  FILE *f;
  filedb fdb;
  long where = 0;
  char s[512], *p, *d;

  BADARGS(2, 3, " dir ?flags ?channel??");
  strcpy(s, argv[1]);
  if (s[strlen(s) - 1] == '/')
     s[strlen(s) - 1] = 0;
  p = strrchr(s, '/');
  if (p == NULL) {
    p = s;
    d = "";
  } else {
    *p = 0;
    p++;
    d = s;
  }
  f = filedb_open(d, 0);
  if (!findmatch(f, p, &where, &fdb))
    Tcl_AppendResult(irp, "-1", NULL);	/* no such dir */
  else if (!(fdb.stat & FILE_DIR))
    Tcl_AppendResult(irp, "-2", NULL);	/* not a dir */
  else {
    if (argc >= 3) {
      struct flag_record fr =
      {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

      break_down_flags(argv[2], &fr, NULL);
      build_flags(s, &fr, NULL);
      strncpy(fdb.flags_req, s, 21);
      fdb.flags_req[21] = 0;
    } else
      fdb.flags_req[0] = 0;
    if (argc == 4) {
      strncpy(fdb.chname, argv[3], 80);
      fdb.chname[80] = 0;
    }
  }
  fseek(f, where, SEEK_SET);
  fwrite(&fdb, sizeof(filedb), 1, f);
  filedb_close(f);
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_getflags STDVAR
{
  FILE *f;
  filedb fdb;
  long where = 0;
  char s[512], *p, *d;

  BADARGS(2, 2, " dir");
  strcpy(s, argv[1]);
  if (s[strlen(s) - 1] == '/')
     s[strlen(s) - 1] = 0;
  p = strrchr(s, '/');
  if (p == NULL) {
    p = s;
    d = "";
  } else {
    *p = 0;
    p++;
    d = s;
  }
  f = filedb_open(d, 0);
  if (!findmatch(f, p, &where, &fdb))
    Tcl_AppendResult(irp, "", NULL);	/* no such dir */
  else if (!(fdb.stat & FILE_DIR))
    Tcl_AppendResult(irp, "", NULL);	/* not a dir */
  else {
    strcpy(s, fdb.flags_req);
    if (s[0] == '-')
      s[0] = 0;
    Tcl_AppendElement(irp, s);
    Tcl_AppendElement(irp, fdb.chname);
  }
  filedb_close(f);
  return TCL_OK;
}

static int tcl_mkdir STDVAR
{
  char s[512], t[512], *d, *p;
  FILE *f;
  filedb fdb;
  long where = 0;
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  BADARGS(2, 3, " dir ?required-flags ?channel??");
  strcpy(s, argv[1]);
  if (s[strlen(s) - 1] == '/')
     s[strlen(s) - 1] = 0;
  p = strrchr(s, '/');
  if (p == NULL) {
    p = s;
    d = "";
  } else {
    *p = 0;
    p++;
    d = s;
  }
  f = filedb_open(d, 0);
  if (!findmatch(f, p, &where, &fdb)) {
    sprintf(t, "%s%s/%s", dccdir, d, p);
    if (mkdir(t, 0755) != 0) {
      Tcl_AppendResult(irp, "1", NULL);
      filedb_close(f);
      return TCL_OK;
    }
    fdb.version = FILEVERSION;
    fdb.stat = FILE_DIR;
    fdb.desc[0] = 0;
    fdb.uploader[0] = 0;
    strcpy(fdb.filename, argv[1]);
    fdb.flags_req[0] = 0;
    fdb.uploaded = now;
    fdb.size = 0;
    fdb.gots = 0;
    fdb.sharelink[0] = 0;
    where = findempty(f);
  } else if (!(fdb.stat & FILE_DIR)) {
    Tcl_AppendResult(irp, "2", NULL);
    filedb_close(f);
    return TCL_OK;
  }
  if (argc >= 3) {
    break_down_flags(argv[2], &fr, NULL);
    build_flags(s, &fr, NULL);
    strncpy(fdb.flags_req, s, 21);
    fdb.flags_req[21] = 0;
  } else {
    fdb.flags_req[0] = 0;
  }
  if (argc == 4) {
    strncpy(fdb.chname, argv[3], 80);
    fdb.chname[80] = 0;
  } else
    fdb.chname[0] = 0;
  Tcl_AppendResult(irp, "0", NULL);
  fseek(f, where, SEEK_SET);
  fwrite(&fdb, sizeof(filedb), 1, f);
  filedb_close(f);
  return TCL_OK;
}

static int tcl_rmdir STDVAR
{
  FILE *f;
  filedb fdb;
  long where = 0;
  char s[256], t[512], *d, *p;

  BADARGS(2, 2, " dir");
  if (strlen(argv[1]) > 80)
     argv[1][80] = 0;
  strcpy(s, argv[1]);
  if (s[strlen(s) - 1] == '/')
     s[strlen(s) - 1] = 0;
  p = strrchr(s, '/');
  if (p == NULL) {
    p = s;
    d = "";
  } else {
    *p = 0;
    p++;
    d = s;
  }
  f = filedb_open(d, 0);
  if (!findmatch(f, p, &where, &fdb)) {
    Tcl_AppendResult(irp, "1", NULL);
    filedb_close(f);
    return TCL_OK;
  }
  if (!(fdb.stat & FILE_DIR)) {
    Tcl_AppendResult(irp, "1", NULL);
    filedb_close(f);
    return TCL_OK;
  }
  /* erase '.filedb' and '.files' if they exist */
  sprintf(t, "%s%s/%s/.filedb", dccdir, d, p);
  unlink(t);
  sprintf(t, "%s%s/%s/.files", dccdir, d, p);
  unlink(t);
  sprintf(t, "%s%s/%s", dccdir, d, p);
  if (rmdir(t) == 0) {
    fdb.stat |= FILE_UNUSED;
    fseek(f, where, SEEK_SET);
    fwrite(&fdb, sizeof(filedb), 1, f);
    filedb_close(f);
    Tcl_AppendResult(irp, "0", NULL);
    return TCL_OK;
  }
  Tcl_AppendResult(irp, "1", NULL);
  filedb_close(f);
  return TCL_OK;
}

static int tcl_mv_cp(Tcl_Interp * irp, int argc, char **argv, int copy)
{
  char *p, fn[161], oldpath[161], s[161], s1[161], newfn[161], newpath[161];
  int ok, only_first, skip_this, ret, ret2;
  FILE *f, *g;
  filedb fdb, x;
  long where, gwhere, wherez;

  BADARGS(3, 3, " oldfilepath newfilepath");
  context;
  strcpy(fn, argv[1]);
  p = strrchr(fn, '/');
  if (p != NULL) {
    *p = 0;
    strncpy(s, fn, 160);
    s[160] = 0;
    strcpy(fn, p + 1);
    if (!resolve_dir("/", s, oldpath, -1)) {
      /* tcl can do * anything */
      Tcl_AppendResult(irp, "-1", NULL);	/* invalid source */
      return TCL_OK;
    }
  } else
    strcpy(oldpath, "/");
  strncpy(s, argv[2], 160);
  s[160] = 0;
  if (!resolve_dir("/", s, newpath, -1)) {
    /* destination is not just a directory */
    p = strrchr(s, '/');
    if (p == NULL) {
      strcpy(newfn, s);
      s[0] = 0;
    } else {
      *p = 0;
      strcpy(newfn, p + 1);
    }
    if (!resolve_dir("/", s, newpath, -1)) {
      Tcl_AppendResult(irp, "-2", NULL);	/* invalid desto */
      return TCL_OK;
    }
  } else
    newfn[0] = 0;
  /* stupidness checks */
  if ((!strcmp(oldpath, newpath)) &&
      ((!newfn[0]) || (!strcmp(newfn, fn)))) {
    Tcl_AppendResult(irp, "-3", NULL);	/* stoopid copy to self */
    return TCL_OK;
  }
  /* be aware of 'cp * this.file' possibility: ONLY COPY FIRST ONE */
  if (((strchr(fn, '?') != NULL) || (strchr(fn, '*') != NULL)) && (newfn[0]))
    only_first = 1;
  else
    only_first = 0;
  f = filedb_open(oldpath, 0);
  if (!strcmp(oldpath, newpath))
    g = NULL;
  else
    g = filedb_open(newpath, 0);
  where = 0L;
  ok = 0;
  ret = findmatch(f, fn, &where, &fdb);
  if (!ret) {
    Tcl_AppendResult(irp, "-4", NULL);	/* nomatch */
    filedb_close(f);
    if (g != NULL)
      filedb_close(g);
    return TCL_OK;
  }
  while (ret) {
    skip_this = 0;
    if (!(fdb.stat & (FILE_HIDDEN | FILE_DIR))) {
      sprintf(s, "%s%s%s%s", dccdir, oldpath,
	      oldpath[0] ? "/" : "", fdb.filename);
      sprintf(s1, "%s%s%s%s", dccdir, newpath,
	      newpath[0] ? "/" : "", newfn[0] ? newfn : fdb.filename);
      if (!strcmp(s, s1)) {
	Tcl_AppendResult(irp, "-3", NULL);	/* stoopid copy to self */
	skip_this = 1;
      }
      /* check for existence of file with same name in new dir */
      wherez = 0;
      if (!g)
	ret2 = findmatch(f, newfn[0] ? newfn : fdb.filename, &wherez, &x);
      else
	ret2 = findmatch(g, newfn[0] ? newfn : fdb.filename, &wherez, &x);
      if (ret2) {
	/* it's ok if the entry in the new dir is a normal file (we'll
	 * just scrap the old entry and overwrite the file) -- but if
	 * it's a directory, this file has to be skipped */
	if (x.stat & FILE_DIR) {
	  /* skip */
	  skip_this = 1;
	} else {
	  x.stat |= FILE_UNUSED;
	  if (!g) {
	    fseek(f, wherez, SEEK_SET);
	    fwrite(&x, sizeof(filedb), 1, f);
	  } else {
	    fseek(g, wherez, SEEK_SET);
	    fwrite(&x, sizeof(filedb), 1, g);
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
	  x.version = FILEVERSION;
	  x.stat = fdb.stat;
	  strcpy(x.flags_req, fdb.flags_req);
	  strcpy(x.chname, fdb.chname);
	  strcpy(x.filename, fdb.filename);
	  strcpy(x.desc, fdb.desc);
	  if (newfn[0])
	    strcpy(x.filename, newfn);
	  strcpy(x.uploader, fdb.uploader);
	  x.uploaded = fdb.uploaded;
	  x.size = fdb.size;
	  x.gots = fdb.gots;
	  strcpy(x.sharelink, fdb.sharelink);
	  if (g == NULL) {
	    fseek(f, gwhere, SEEK_SET);
	    fwrite(&x, sizeof(filedb), 1, f);
	  } else {
	    fseek(g, gwhere, SEEK_SET);
	    fwrite(&x, sizeof(filedb), 1, g);
	  }
	  if (!copy) {
	    unlink(s);
	    fdb.stat |= FILE_UNUSED;
	    fseek(f, where, SEEK_SET);
	    fwrite(&fdb, sizeof(filedb), 1, f);
	  }
	}
      }
    }
    where += sizeof(filedb);
    ret = findmatch(f, fn, &where, &fdb);
    if ((ok) && (only_first))
      ret = 0;
  }
  if (!ok)
    Tcl_AppendResult(irp, "-4", NULL);	/* nomatch */
  else {
    char x[30];

    sprintf(x, "%d", ok);
    Tcl_AppendResult(irp, x, NULL);
  }
  filedb_close(f);
  if (g)
    filedb_close(g);
  return TCL_OK;
}

static int tcl_mv STDVAR
{
  return tcl_mv_cp(irp, argc, argv, 0);
}

static int tcl_cp STDVAR
{
  return tcl_mv_cp(irp, argc, argv, 1);
}

static int tcl_filesend STDVAR
{
  int i, idx;
  char s[HANDLEN + 1];

  BADARGS(3, 4, " idx filename ?nick?");
  i = atoi(argv[1]);
  idx = findanyidx(i);
  if ((idx < 0) || (dcc[idx].type != &DCC_FILES)) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (argc == 4)
     i = files_get(idx, argv[2], argv[3]);

  else
    i = files_get(idx, argv[2], "");
  sprintf(s, "%d", i);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static tcl_cmds mytcls[] =
{
  {"getdesc", tcl_getdesc},
  {"getowner", tcl_getowner},
  {"setdesc", tcl_setdesc},
  {"setowner", tcl_setowner},
  {"getgots", tcl_getgots},
  {"getpwd", tcl_getpwd},
  {"setpwd", tcl_setpwd},
  {"getlink", tcl_getlink},
  {"setlink", tcl_setlink},
  {"getfiles", tcl_getfiles},
  {"getdirs", tcl_getdirs},
  {"hide", tcl_hide},
  {"unhide", tcl_unhide},
  {"share", tcl_share},
  {"unshare", tcl_unshare},
  {"filesend", tcl_filesend},
  {"mkdir", tcl_mkdir},
  {"rmdir", tcl_rmdir},
  {"cp", tcl_cp},
  {"mv", tcl_mv},
  {"getflags", tcl_getflags},
  {"setflags", tcl_setflags},
  {0, 0}
};
