/*
 * transferfstat.c -- part of transfer.mod
 *
 * $Id: transferfstat.c,v 1.12 2011/02/13 14:19:34 simple Exp $
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

static int fstat_unpack(struct userrec *u, struct user_entry *e)
{
  char *par, *arg;
  struct filesys_stats *fs;

  fs = user_malloc(sizeof(struct filesys_stats));
  egg_bzero(fs, sizeof(struct filesys_stats));
  par = e->u.list->extra;

  arg = newsplit(&par);
  if (arg[0])
    fs->uploads = atoi(arg);

  arg = newsplit(&par);
  if (arg[0])
    fs->upload_ks = atoi(arg);

  arg = newsplit(&par);
  if (arg[0])
    fs->dnloads = atoi(arg);

  arg = newsplit(&par);
  if (arg[0])
    fs->dnload_ks = atoi(arg);

  list_type_kill(e->u.list);
  e->u.extra = fs;

  return 1;
}

static int fstat_pack(struct userrec *u, struct user_entry *e)
{
  register struct filesys_stats *fs;
  struct list_type *l = user_malloc(sizeof(struct list_type));

  fs = e->u.extra;
  l->extra = user_malloc(41);
  egg_snprintf(l->extra, 41, "%09u %09u %09u %09u", fs->uploads, fs->upload_ks,
               fs->dnloads, fs->dnload_ks);
  l->next = NULL;
  e->u.list = l;
  nfree(fs);

  return 1;
}

static int fstat_write_userfile(FILE *f, struct userrec *u,
                                struct user_entry *e)
{
  register struct filesys_stats *fs;

  fs = e->u.extra;
  if (fprintf(f, "--FSTAT %09u %09u %09u %09u\n", fs->uploads, fs->upload_ks,
              fs->dnloads, fs->dnload_ks) == EOF)
    return 0;

  return 1;
}

static int fstat_set(struct userrec *u, struct user_entry *e, void *buf)
{
  register struct filesys_stats *fs = buf;

  if (e->u.extra != fs) {
    if (e->u.extra)
      nfree(e->u.extra);
    e->u.extra = fs;
  } else if (!fs) /* e->u.extra == NULL && fs == NULL */
    return 1;

  if (!noshare && !(u->flags & (USER_BOT | USER_UNSHARED))) {
    if (fs)
      /* Don't check here for something like:
       *  ofs->uploads != fs->uploads || ofs->upload_ks != fs->upload_ks ||
       *  ofs->dnloads != fs->dnloads || ofs->dnload_ks != fs->dnload_ks
       *
       * Someone could do:
       *  e->u.extra->uploads = 12345;
       *  fs = user_malloc(sizeof(struct filesys_stats));
       *  my_memcpy(...e->u.extra...fs...);
       *  set_user(&USERENTRY_FSTAT, u, fs);
       *
       * Then we wouldn't detect here that something's changed.
       * --rtc
       */
      shareout(NULL, "ch fstat %09u %09u %09u %09u\n", fs->uploads,
               fs->upload_ks, fs->dnloads, fs->dnload_ks);
    else
      shareout(NULL, "ch fstat r\n");
  }

  return 1;
}

static int fstat_tcl_get(Tcl_Interp *irp, struct userrec *u,
                         struct user_entry *e, int argc, char **argv)
{
  register struct filesys_stats *fs;
  char d[50];

  BADARGS(3, 4, " handle FSTAT ?u/d?");

  fs = e->u.extra;
  if (argc == 3)
    egg_snprintf(d, sizeof d, "%u %u %u %u", fs->uploads, fs->upload_ks,
                 fs->dnloads, fs->dnload_ks);
  else
    switch (argv[3][0]) {
    case 'u':
      egg_snprintf(d, sizeof d, "%u %u", fs->uploads, fs->upload_ks);
      break;
    case 'd':
      egg_snprintf(d, sizeof d, "%u %u", fs->dnloads, fs->dnload_ks);
      break;
    }

  Tcl_AppendResult(irp, d, NULL);
  return TCL_OK;
}

static int fstat_kill(struct user_entry *e)
{
  if (e->u.extra)
    nfree(e->u.extra);
  nfree(e);

  return 1;
}

static int fstat_expmem(struct user_entry *e)
{
  return sizeof(struct filesys_stats);
}

static void fstat_display(int idx, struct user_entry *e)
{
  struct filesys_stats *fs;

  fs = e->u.extra;
  dprintf(idx, "  FILES: %u download%s (%luk), %u upload%s (%luk)\n",
          fs->dnloads, (fs->dnloads == 1) ? "" : "s", fs->dnload_ks,
          fs->uploads, (fs->uploads == 1) ? "" : "s", fs->upload_ks);
}

static struct user_entry_type USERENTRY_FSTAT = {
  NULL,
  fstat_gotshare,
  fstat_dupuser,
  fstat_unpack,
  fstat_pack,
  fstat_write_userfile,
  fstat_kill,
  NULL,
  fstat_set,
  fstat_tcl_get,
  fstat_tcl_set,
  fstat_expmem,
  fstat_display,
  "FSTAT"
};

static int fstat_gotshare(struct userrec *u, struct user_entry *e,
                          char *par, int idx)
{
  char *p;
  struct filesys_stats *fs;

  noshare = 1;
  switch (par[0]) {
  case 'u':
  case 'd':
    break;
  case 'r':
    set_user(&USERENTRY_FSTAT, u, NULL);
    break;
  default:
    if (!(fs = e->u.extra)) {
      fs = user_malloc(sizeof(struct filesys_stats));
      egg_bzero(fs, sizeof(struct filesys_stats));
    }

    p = newsplit(&par);
    if (p[0])
      fs->uploads = atoi(p);

    p = newsplit(&par);
    if (p[0])
      fs->upload_ks = atoi(p);

    p = newsplit(&par);
    if (p[0])
      fs->dnloads = atoi(p);

    p = newsplit(&par);
    if (p[0])
      fs->dnload_ks = atoi(p);

    set_user(&USERENTRY_FSTAT, u, fs);
    break;
  }
  noshare = 0;

  return 1;
}

static int fstat_dupuser(struct userrec *u, struct userrec *o,
                         struct user_entry *e)
{
  struct filesys_stats *fs;

  if (e->u.extra) {
    fs = user_malloc(sizeof(struct filesys_stats));
    my_memcpy(fs, e->u.extra, sizeof(struct filesys_stats));
    return set_user(&USERENTRY_FSTAT, u, fs);
  }

  return 0;
}

static void stats_add_dnload(struct userrec *u, unsigned long bytes)
{
  struct user_entry *ue;
  register struct filesys_stats *fs;

  if (u) {
    if (!(ue = find_user_entry(&USERENTRY_FSTAT, u)) || !(fs = ue->u.extra)) {
      fs = user_malloc(sizeof(struct filesys_stats));
      egg_bzero(fs, sizeof(struct filesys_stats));
    }
    fs->dnloads++;
    fs->dnload_ks += ((bytes + 512) / 1024);
    set_user(&USERENTRY_FSTAT, u, fs);
    /* No shareout here, set_user already sends info... --rtc */
  }
}

static void stats_add_upload(struct userrec *u, unsigned long bytes)
{
  struct user_entry *ue;
  register struct filesys_stats *fs;

  if (u) {
    if (!(ue = find_user_entry(&USERENTRY_FSTAT, u)) || !(fs = ue->u.extra)) {
      fs = user_malloc(sizeof(struct filesys_stats));
      egg_bzero(fs, sizeof(struct filesys_stats));
    }
    fs->uploads++;
    fs->upload_ks += ((bytes + 512) / 1024);
    set_user(&USERENTRY_FSTAT, u, fs);
    /* No shareout here, set_user already sends info... --rtc */
  }
}

static int fstat_tcl_set(Tcl_Interp *irp, struct userrec *u,
                         struct user_entry *e, int argc, char **argv)
{
  register struct filesys_stats *fs;
  int f = 0, k = 0;

  BADARGS(4, 6, " handle FSTAT u/d ?files ?ks??");

  if (argc > 4)
    f = atoi(argv[4]);
  if (argc > 5)
    k = atoi(argv[5]);

  switch (argv[3][0]) {
  case 'u':
  case 'd':
    if (!(fs = e->u.extra)) {
      fs = user_malloc(sizeof(struct filesys_stats));
      egg_bzero(fs, sizeof(struct filesys_stats));
    }
    switch (argv[3][0]) {
    case 'u':
      fs->uploads = f;
      fs->upload_ks = k;
      break;
    case 'd':
      fs->dnloads = f;
      fs->dnload_ks = k;
      break;
    }
    set_user(&USERENTRY_FSTAT, u, fs);
    break;
  case 'r':
    set_user(&USERENTRY_FSTAT, u, NULL);
    break;
  }

  return TCL_OK;
}
