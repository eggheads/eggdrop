/*
 * transferqueue.c -- part of transfer.mod
 *
 * $Id: transferqueue.c,v 1.11 2011/02/13 14:19:34 simple Exp $
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

static int expmem_fileq()
{
  fileq_t *q;
  int tot = 0;

  for (q = fileq; q; q = q->next)
    tot += strlen(q->dir) + strlen(q->file) + 2 + sizeof(fileq_t);

  return tot;
}

static void queue_file(char *dir, char *file, char *from, char *to)
{
  fileq_t *q = fileq;

  fileq = nmalloc(sizeof *fileq);
  fileq->next = q;
  fileq->dir = nmalloc(strlen(dir) + 1);
  fileq->file = nmalloc(strlen(file) + 1);
  strcpy(fileq->dir, dir);
  strcpy(fileq->file, file);
  strcpy(fileq->nick, from);
  strcpy(fileq->to, to);
}

static void deq_this(fileq_t *this)
{
  fileq_t *q = fileq, *last = NULL;

  while (q && q != this) {
    last = q;
    q = q->next;
  }

  if (!q)
    return;

  if (last)
    last->next = q->next;
  else
    fileq = q->next;
  nfree(q->dir);
  nfree(q->file);
  nfree(q);
}

/* Remove all files queued to a certain user.
 */
static void flush_fileq(char *to)
{
  fileq_t *q = fileq;
  int fnd = 1;

  while (fnd) {
    q = fileq;
    fnd = 0;
    while (q != NULL) {
      if (!egg_strcasecmp(q->to, to)) {
        deq_this(q);
        q = NULL;
        fnd = 1;
      }
      if (q != NULL)
        q = q->next;
    }
  }
}

static void send_next_file(char *to)
{
  fileq_t *q, *this = NULL;
  char *s, *s1;
  int x;

  for (q = fileq; q; q = q->next)
    if (!egg_strcasecmp(q->to, to))
      this = q;

  if (this == NULL)
    return;

  if (this->dir[0] == '*') { /* Absolute path */
    s = nmalloc(strlen(&this->dir[1]) + strlen(this->file) + 2);
    sprintf(s, "%s/%s", &this->dir[1], this->file);
  } else {
    char *p = strchr(this->dir, '*');

    if (p == NULL) {
      send_next_file(to);
      return;
    }

    p++;
    s = nmalloc(strlen(p) + strlen(this->file) + 2);
    sprintf(s, "%s%s%s", p, p[0] ? "/" : "", this->file);
    strcpy(this->dir, &(p[atoi(this->dir)]));
  }
  if (copy_to_tmp) {
    s1 = nmalloc(strlen(tempdir) + strlen(this->file) + 1);
    sprintf(s1, "%s%s", tempdir, this->file);
    if (copyfile(s, s1) != 0) {
      putlog(LOG_FILES | LOG_MISC, "*", TRANSFER_COPY_FAILED, this->file,
             tempdir);
      dprintf(DP_HELP, TRANSFER_FILESYS_BROKEN, this->to);
      strcpy(s, this->to);
      flush_fileq(s);
      nfree(s1);
      nfree(s);
      return;
    }
  } else {
    s1 = nmalloc(strlen(s) + 1);
    strcpy(s1, s);
  }
  if (this->dir[0] == '*') {
    s = nrealloc(s, strlen(&this->dir[1]) + strlen(this->file) + 2);
    sprintf(s, "%s/%s", &this->dir[1], this->file);
  } else {
    s = nrealloc(s, strlen(this->dir) + strlen(this->file) + 2);
    sprintf(s, "%s%s%s", this->dir, this->dir[0] ? "/" : "", this->file);
  }
  x = raw_dcc_send(s1, this->to, this->nick, s);
  if (x == DCCSEND_OK) {
    if (egg_strcasecmp(this->to, this->nick))
      dprintf(DP_HELP, TRANSFER_FILE_ARRIVE, this->to, this->nick);
    deq_this(this);
    nfree(s);
    nfree(s1);
    return;
  }
  wipe_tmp_filename(s1, -1);
  if (x == DCCSEND_FULL) {
    putlog(LOG_FILES, "*", TRANSFER_LOG_CONFULL, s1, this->nick);
    dprintf(DP_HELP, TRANSFER_NOTICE_CONFULL, this->to);
    strcpy(s, this->to);
    flush_fileq(s);
  } else if (x == DCCSEND_NOSOCK) {
    putlog(LOG_FILES, "*", TRANSFER_LOG_SOCKERR, s1, this->nick);
    dprintf(DP_HELP, TRANSFER_NOTICE_SOCKERR, this->to);
    strcpy(s, this->to);
    flush_fileq(s);
  } else {
    if (x == DCCSEND_FEMPTY) {
      putlog(LOG_FILES, "*", TRANSFER_LOG_FILEEMPTY, this->file);
      dprintf(DP_HELP, TRANSFER_NOTICE_FILEEMPTY, this->to, this->file);
    }
    deq_this(this);
  }
  nfree(s);
  nfree(s1);

  return;
}

static void show_queued_files(int idx)
{
  int i, cnt = 0, len;
  char spaces[] = "                                 ";
  fileq_t *q;

  for (q = fileq; q; q = q->next) {
    if (!egg_strcasecmp(q->nick, dcc[idx].nick)) {
      if (!cnt) {
        spaces[HANDLEN - 9] = 0;
        dprintf(idx, TRANSFER_SEND_TO, spaces);
        dprintf(idx, TRANSFER_LINES, spaces);
        spaces[HANDLEN - 9] = ' ';
      }
      cnt++;
      spaces[len = HANDLEN - strlen(q->to)] = 0;
      if (q->dir[0] == '*')
        dprintf(idx, "  %s%s  %s/%s\n", q->to, spaces, &q->dir[1], q->file);
      else
        dprintf(idx, "  %s%s  /%s%s%s\n", q->to, spaces, q->dir,
                q->dir[0] ? "/" : "", q->file);
      spaces[len] = ' ';
    }
  }
  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type == &DCC_GET_PENDING || dcc[i].type == &DCC_GET) &&
        (!egg_strcasecmp(dcc[i].nick, dcc[idx].nick) ||
         !egg_strcasecmp(dcc[i].u.xfer->from, dcc[idx].nick))) {
      char *nfn;

      if (!cnt) {
        spaces[HANDLEN - 9] = 0;
        dprintf(idx, TRANSFER_SEND_TO, spaces);
        dprintf(idx, TRANSFER_LINES, spaces);
        spaces[HANDLEN - 9] = ' ';
      }
      nfn = strrchr(dcc[i].u.xfer->origname, '/');
      if (nfn == NULL)
        nfn = dcc[i].u.xfer->origname;
      else
        nfn++;
      cnt++;
      spaces[len = HANDLEN - strlen(dcc[i].nick)] = 0;
      if (dcc[i].type == &DCC_GET_PENDING)
        dprintf(idx, TRANSFER_WAITING, dcc[i].nick, spaces, nfn);
      else
        dprintf(idx, TRANSFER_DONE, dcc[i].nick, spaces, nfn, (100.0 *
                ((float) dcc[i].status / (float) dcc[i].u.xfer->length)));
      spaces[len] = ' ';
    }
  }
  if (!cnt)
    dprintf(idx, TRANSFER_QUEUED_UP);
  else
    dprintf(idx, TRANSFER_TOTAL, cnt);
}

static void fileq_cancel(int idx, char *par)
{
  int fnd = 1, matches = 0, atot = 0, i;
  fileq_t *q;
  char *s = NULL;

  while (fnd) {
    q = fileq;
    fnd = 0;
    while (q != NULL) {
      if (!egg_strcasecmp(dcc[idx].nick, q->nick)) {
        s = nrealloc(s, strlen(q->dir) + strlen(q->file) + 3);
        if (q->dir[0] == '*')
          sprintf(s, "%s/%s", &q->dir[1], q->file);
        else
          sprintf(s, "/%s%s%s", q->dir, q->dir[0] ? "/" : "", q->file);
        if (wild_match_file(par, s)) {
          dprintf(idx, TRANSFER_CANCELLED, s, q->to);
          fnd = 1;
          deq_this(q);
          q = NULL;
          matches++;
        }
        if (!fnd && wild_match_file(par, q->file)) {
          dprintf(idx, TRANSFER_CANCELLED, s, q->to);
          fnd = 1;
          deq_this(q);
          q = NULL;
          matches++;
        }
      }
      if (q != NULL)
        q = q->next;
    }
  }

  if (s)
    nfree(s);

  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type == &DCC_GET_PENDING || dcc[i].type == &DCC_GET) &&
        (!egg_strcasecmp(dcc[i].nick, dcc[idx].nick) ||
         !egg_strcasecmp(dcc[i].u.xfer->from, dcc[idx].nick))) {
      char *nfn = strrchr(dcc[i].u.xfer->origname, '/');

      if (nfn == NULL)
        nfn = dcc[i].u.xfer->origname;
      else
        nfn++;
      if (wild_match_file(par, nfn)) {
        dprintf(idx, TRANSFER_ABORT_DCCSEND, nfn);
        if (egg_strcasecmp(dcc[i].nick, dcc[idx].nick))
          dprintf(DP_HELP, TRANSFER_NOTICE_ABORT, dcc[i].nick, nfn,
                  dcc[idx].nick);
        if (dcc[i].type == &DCC_GET)
          putlog(LOG_FILES, "*", TRANSFER_DCC_CANCEL, nfn, dcc[i].nick,
                 dcc[i].status, dcc[i].u.xfer->length);
        wipe_tmp_filename(dcc[i].u.xfer->filename, i);
        atot++;
        matches++;
        killsock(dcc[i].sock);
        lostdcc(i);
      }
    }
  }
  if (!matches)
    dprintf(idx, TRANSFER_NO_MATCHES);
  else
    dprintf(idx, TRANSFER_CANCELLED_FILE, matches, (matches != 1) ? "s" : "");
  for (i = 0; i < atot; i++)
    if (!at_limit(dcc[idx].nick))
      send_next_file(dcc[idx].nick);
}
