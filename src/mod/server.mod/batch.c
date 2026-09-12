/*
 * batch.c -- part of server.mod
 *   support for the IRCv3 batch capability
 *
 * https://ircv3.net/specs/extensions/batch
 */
/*
 * Copyright (C) 2026 Eggheads Development Team
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

static int batchcount = 0;
static batch_t *batchlist = NULL;       /* List of batches the server has opened but not yet closed.  */
static batch_t *current_batch = NULL;   /* The batch that the line currently being dispatched belongs */
                                        /* to, or NULL if that line carried no batch tag.             */

static void batch_end(batch_t *b);

static int batch_valid_reftag(const char *reftag)
{
  if (!reftag || !*reftag)
    return 0;
  if (strlen(reftag) > BATCHREFMAX)
    return 0;
  /* Valid if the run of allowed characters extends to the terminator. */
  return reftag[strspn(reftag, BATCHREFCHARS)] == '\0';
}

/* Look up an open batch by reference tag. Comparison is case-sensitive */
static batch_t *batch_find(const char *reftag)
{
  batch_t *b;

  if (!reftag || !*reftag)
    return NULL;
  for (b = batchlist; b; b = b->next) {
    if (!strcmp(reftag, b->reftag))
      return b;
  }
  return NULL;
}

/* Exported so handlers that take no tag arg can still see the batch context */
/* TODO: Is this right? */
static batch_t *batch_get_current(void)
{
  return current_batch;
}

/* Free every open batch, good for disconnections/capability removal */
static void batch_free_all(void)
{
  batch_t *b, *next;

  for (b = batchlist; b; b = next) {
    next = b->next;
    putlog(LOG_DEBUG, "*", "BATCH: discarding unterminated batch %s (type %s)",
           b->reftag, b->type);
    nfree(b);
  }
  batchlist = NULL;
  current_batch = NULL;
  batchcount = 0;
}

/* Unlink and free a single batch record, without touching its children */
static void batch_free_one(batch_t *b)
{
  batch_t **prev;

  if (current_batch == b)
    current_batch = NULL;
  for (prev = &batchlist; *prev; prev = &(*prev)->next) {
    if (*prev == b) {
      *prev = b->next;
      batchcount--;
      nfree(b);
      return;
    }
  }
}

/* Is maybe an ancestor of b, or b itself? */
static int batch_is_ancestor(const batch_t *maybe, const batch_t *b)
{
  for (; b; b = b->parent) {
    if (b == maybe)
      return 1;
  }
  return 0;
}

/* The batch open the longest, ignoring protect and everything it is nested
 * inside. Evicting a batch cascades to its children, so an ancestor of the
 * batch we are about to open must be excluded, or the new record would be
 * left with a parent pointer into freed memory.
 */
static batch_t *batch_oldest(const batch_t *protect)
{
  batch_t *b, *oldest = NULL;

  for (b = batchlist; b; b = b->next) {
    if (batch_is_ancestor(b, protect))
      continue;
    if (!oldest || b->started < oldest->started)
      oldest = b;
  }
  return oldest;
}

/* Open a batch. parent is the batch that the BATCH + line itself was tagged
 * with, or NULL for a top-level batch.
 */
static batch_t *batch_start(const char *reftag, const char *type,
                            const char *args, batch_t *parent)
{
  batch_t *b;

  /* Drop the stalest batch rather than refusing the new one, so a server that
   * leaks batches costs us one old record instead of the ability to track any
   * new ones until we disconnect.
   */
  while (batchcount >= BATCHMAX) {
    batch_t *old = batch_oldest(parent);

    if (!old) {
      putlog(LOG_DEBUG, "*", "BATCH: refusing to open %s, the %d open batch "
             "limit is entirely nesting above it", reftag, BATCHMAX);
      return NULL;
    }
    putlog(LOG_DEBUG, "*", "BATCH: at the %d open batch limit, discarding "
           "oldest batch %s (type %s) to make room for %s", BATCHMAX,
           old->reftag, old->type, reftag);
    batch_end(old);
  }
  b = nmalloc(sizeof *b);
  memset(b, 0, sizeof *b);
  strlcpy(b->reftag, reftag, sizeof b->reftag);
  strlcpy(b->type, type, sizeof b->type);
  if (args)
    strlcpy(b->args, args, sizeof b->args);
  b->parent = parent;
  b->started = now;
  b->next = batchlist;
  batchlist = b;
  batchcount++;
  return b;
}

/* Close a batch and free it, along with anything nested inside it.
 *
 * A nested batch cannot outlive its parent: its end line would have to carry
 * a tag naming the parent, and the spec forbids referring to a batch after it
 * has ended, so once the parent closes no legal line can ever close the
 * child. Those records are unreachable, so they go now rather than sitting in
 * the list until we disconnect.
 *
 * The scan restarts after each removal because freeing a child also frees its
 * own descendants, which can invalidate a saved next pointer.
 */
static void batch_end(batch_t *b)
{
  batch_t *cur;
  int found;

  do {
    found = 0;
    for (cur = batchlist; cur; cur = cur->next) {
      if (cur->parent == b) {
        putlog(LOG_DEBUG, "*", "BATCH: discarding nested batch %s, its parent "
               "%s closed first", cur->reftag, b->reftag);
        batch_end(cur);
        found = 1;
        break;
      }
    }
  } while (found);
  batch_free_one(b);
}

/* Resolve the batch tag on an incoming line to an open batch record. Returns
 * NULL if the line carried no batch tag, or if it named a batch we have no
 * record of.
 */
static batch_t *batch_from_tagdict(Tcl_Obj *tagdict)
{
  Tcl_Obj *key, *value = NULL;
  batch_t *b;
  char *reftag;

  if (!tagdict)
    return NULL;
  key = Tcl_NewStringObj("batch", -1);
  Tcl_IncrRefCount(key);
  if ((Tcl_DictObjGet(interp, tagdict, key, &value) != TCL_OK) || !value) {
    Tcl_DecrRefCount(key);
    return NULL;
  }
  Tcl_DecrRefCount(key);
  reftag = Tcl_GetString(value);
  if (!*reftag)
    return NULL;
  b = batch_find(reftag);
  if (!b)
    putlog(LOG_DEBUG, "*", "BATCH: received a line tagged for batch %s, but I "
           "have no record of that batch", reftag);
  return b;
}

/* Got BATCH
 *   :server BATCH +<reference tag> <type> [<parameter> ...]
 *   :server BATCH -<reference tag>
 */
static int gotbatch(char *from, char *msg)
{
  char *reftag, *type;
  char prefix;
  batch_t *b;

  // Check for malformed states. Probably need to add more handling later
  reftag = newsplit(&msg);
  if (!*reftag) {
    putlog(LOG_DEBUG, "*", "BATCH: %s sent a BATCH with no reference tag", from);
    return 0;
  }
  prefix = *reftag++;
  if (!batch_valid_reftag(reftag)) {
    putlog(LOG_DEBUG, "*", "BATCH: %s sent an invalid reference tag, ignoring",
           from);
    return 0;
  }
  if (prefix == '+') {
    if (batch_find(reftag)) {
      putlog(LOG_DEBUG, "*", "BATCH: %s tried to open batch %s, which is "
             "already open", from, reftag);
      return 0;
    }
    type = newsplit(&msg);
    if (!*type) {
      putlog(LOG_DEBUG, "*", "BATCH: %s opened batch %s with no type", from,
             reftag);
      return 0;
    }
    b = batch_start(reftag, type, msg, current_batch);
    if (b)
      putlog(LOG_DEBUG, "*", "BATCH: opened %s (type %s)%s%s", b->reftag,
             b->type, b->parent ? ", nested in " : "",
             b->parent ? b->parent->reftag : "");
  } else if (prefix == '-') {
    b = batch_find(reftag);
    if (!b) {
      putlog(LOG_DEBUG, "*", "BATCH: %s closed batch %s, but I have no record "
             "of it being opened", from, reftag);
      return 0;
    }

 /* The spec requires the start and end lines of a batch to refer to the
  * same parent batch. Mismatches are logged but not fatal.
  */
    if (current_batch != b->parent)
      putlog(LOG_DEBUG, "*", "BATCH: %s closed batch %s from a different batch "
             "context than it was opened in", from, reftag);
    putlog(LOG_DEBUG, "*", "BATCH: closed %s (type %s)", b->reftag, b->type);
    batch_end(b);
  } else {
    putlog(LOG_DEBUG, "*", "BATCH: %s sent a BATCH with an unrecognized prefix "
           "'%c'", from, prefix);
  }
  return 0;
}
