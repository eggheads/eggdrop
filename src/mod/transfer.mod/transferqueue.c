/*
 * transferqueue.c -- part of transfer.mod
 *
 * Copyright (C) 2003 - 2021 Eggheads Development Team
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
  size_t l;

  fileq = nmalloc(sizeof *fileq);
  fileq->next = q;
  l = strlen(dir) + 1;
  fileq->dir = nmalloc(l);
  strlcpy(fileq->dir, dir, l);
  l = strlen(file) + 1;
  fileq->file = nmalloc(l);
  strlcpy(fileq->file, file, l);
  strlcpy(fileq->nick, from, sizeof fileq->nick);
  strlcpy(fileq->to, to, sizeof fileq->to);
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
  fileq_t *q;
  int fnd = 1;

  while (fnd) {
    q = fileq;
    fnd = 0;
    while (q != NULL) {
      if (!strcasecmp(q->to, to)) {
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
  char *s;
  int x;

  for (q = fileq; q; q = q->next)
    if (!strcasecmp(q->to, to))
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
  if (this->dir[0] == '*') {
    s = nrealloc(s, strlen(&this->dir[1]) + strlen(this->file) + 2);
    sprintf(s, "%s/%s", &this->dir[1], this->file);
  } else {
    s = nrealloc(s, strlen(this->dir) + strlen(this->file) + 2);
    sprintf(s, "%s%s%s", this->dir, this->dir[0] ? "/" : "", this->file);
  }
  x = raw_dcc_send(s, this->to, this->nick);
  if (x == DCCSEND_OK) {
    if (strcasecmp(this->to, this->nick))
      dprintf(DP_HELP, TRANSFER_FILE_ARRIVE, this->to, this->nick);
    deq_this(this);
  } else if (x == DCCSEND_FULL) {
    putlog(LOG_FILES, "*", TRANSFER_LOG_CONFULL, s, this->nick);
    dprintf(DP_HELP, TRANSFER_NOTICE_CONFULL, this->to);
    strcpy(s, this->to);
    flush_fileq(s);
  } else if (x == DCCSEND_NOSOCK) {
    putlog(LOG_FILES, "*", TRANSFER_LOG_SOCKERR, s, this->nick);
    dprintf(DP_HELP, TRANSFER_NOTICE_SOCKERR, this->to);
    strcpy(s, this->to);
    flush_fileq(s);
  } else if (x == DCCSEND_FCOPY) {
    putlog(LOG_FILES | LOG_MISC, "*", TRANSFER_COPY_FAILED, this->file);
    dprintf(DP_HELP, TRANSFER_FILESYS_BROKEN, this->to);
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

  return;
}

static void show_queued_files(int idx)
{
  int i, cnt = 0, len;
  char spaces[] = "                                 ";
  fileq_t *q;

  for (q = fileq; q; q = q->next) {
    if (!strcasecmp(q->nick, dcc[idx].nick)) {
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
        (!strcasecmp(dcc[i].nick, dcc[idx].nick) ||
         !strcasecmp(dcc[i].u.xfer->from, dcc[idx].nick))) {
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
      if (!strcasecmp(dcc[idx].nick, q->nick)) {
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
        (!strcasecmp(dcc[i].nick, dcc[idx].nick) ||
         !strcasecmp(dcc[i].u.xfer->from, dcc[idx].nick))) {
      char *nfn = strrchr(dcc[i].u.xfer->origname, '/');

      if (nfn == NULL)
        nfn = dcc[i].u.xfer->origname;
      else
        nfn++;
      if (wild_match_file(par, nfn)) {
        dprintf(idx, TRANSFER_ABORT_DCCSEND, nfn);
        if (strcasecmp(dcc[i].nick, dcc[idx].nick))
          dprintf(DP_HELP, TRANSFER_NOTICE_ABORT, dcc[i].nick, nfn,
                  dcc[idx].nick);
        if (dcc[i].type == &DCC_GET)
          putlog(LOG_FILES, "*", TRANSFER_DCC_CANCEL, nfn, dcc[i].nick,
                 dcc[i].status, dcc[i].u.xfer->length);
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
