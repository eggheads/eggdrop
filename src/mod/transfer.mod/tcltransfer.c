/*
 * tcltransfer.c -- part of transfer.mod
 *
 * $Id: tcltransfer.c,v 1.11 2011/02/13 14:19:34 simple Exp $
 *
 * Copyright (C) 2003 - 2011 Eggheads Development Team
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

static int tcl_dccsend STDVAR
{
  char s[10], *sys, *nfn;
  int i;

  BADARGS(3, 3, " filename ircnick");

  if (!file_readable(argv[1])) {
    Tcl_AppendResult(irp, "3", NULL);
    return TCL_OK;
  }
  nfn = strrchr(argv[1], '/');

  if (nfn == NULL)
    nfn = argv[1];
  else
    nfn++;
  if (at_limit(argv[2])) {
    if (nfn == argv[1])
      queue_file("*", nfn, "(script)", argv[2]);
    else {
      nfn--;
      *nfn = 0;
      nfn++;
      sys = nmalloc(strlen(argv[1]) + 2);
      sprintf(sys, "*%s", argv[1]);
      queue_file(sys, nfn, "(script)", argv[2]);
      nfree(sys);
    }
    Tcl_AppendResult(irp, "4", NULL);
    return TCL_OK;
  }
  if (copy_to_tmp) {
    sys = nmalloc(strlen(tempdir) + strlen(nfn) + 1);
    sprintf(sys, "%s%s", tempdir, nfn);
    if (file_readable(sys)) {
      Tcl_AppendResult(irp, "5", NULL);
      return TCL_OK;
    }
    copyfile(argv[1], sys);
  } else {
    sys = nmalloc(strlen(argv[1]) + 1);
    strcpy(sys, argv[1]);
  }
  i = raw_dcc_send(sys, argv[2], "*", argv[1]);
  if (i > 0)
    wipe_tmp_filename(sys, -1);
  egg_snprintf(s, sizeof s, "%d", i);
  Tcl_AppendResult(irp, s, NULL);
  nfree(sys);
  return TCL_OK;
}

static int tcl_getfileq STDVAR
{
  char *s = NULL;
  fileq_t *q;

  BADARGS(2, 2, " handle");

  for (q = fileq; q; q = q->next) {
    if (!egg_strcasecmp(q->nick, argv[1])) {
      s = nrealloc(s, strlen(q->to) + strlen(q->dir) + strlen(q->file) + 4);
      if (q->dir[0] == '*')
        sprintf(s, "%s %s/%s", q->to, &q->dir[1], q->file);
      else
        sprintf(s, "%s /%s%s%s", q->to, q->dir, q->dir[0] ? "/" : "", q->file);
      Tcl_AppendElement(irp, s);
    }
  }
  if (s)
    nfree(s);
  return TCL_OK;
}

static int tcl_getfilesendtime STDVAR
{
  int sock, i;
  char s[15];

  BADARGS(2, 2, " idx");

  sock = atoi(argv[1]);
  for (i = 0; i < dcc_total; i++) {
    if (dcc[i].sock == sock) {
      if (dcc[i].type == &DCC_SEND || dcc[i].type == &DCC_GET) {
        egg_snprintf(s, sizeof s, "%lu", dcc[i].u.xfer->start_time);
        Tcl_AppendResult(irp, s, NULL);
      } else
        Tcl_AppendResult(irp, "-2", NULL); /* Not a valid file transfer */
      return TCL_OK;
    }
  }
  Tcl_AppendResult(irp, "-1", NULL); /* No matching entry found. */
  return TCL_OK;
}

static tcl_cmds mytcls[] = {
  {"dccsend",                 tcl_dccsend},
  {"getfileq",               tcl_getfileq},
  {"getfilesendtime", tcl_getfilesendtime},
  {NULL,                             NULL}
};
