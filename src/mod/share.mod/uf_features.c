/*
 * uf_features.c -- part of share.mod
 *
 * $Id: uf_features.c,v 1.20 2011/02/13 14:19:34 simple Exp $
 */
/*
 * Copyright (C) 2000 - 2011 Eggheads Development Team
 * Written by Fabian Knittel <fknittel@gmx.de>
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
 * Userfile feature protocol description:
 *
 *
 *               LEAF                                  HUB
 *
 *   uf_features_dump():
 *      Finds out which features
 *      it supports / wants to use
 *      and then dumps those. The
 *      list is appended to the
 *      user file send ack.
 *
 *      "s uy <features>"   --+
 *                            |
 *                            +-->   uf_features_parse():
 *                                      Parses the given list of features,
 *                                      given in a string, seperated with
 *                                      spaces. Decides which features to
 *                                      accept/use. Those features are then
 *                                      locally set:
 *
 *                                      dcc[idx].u.bot->uff_flags |= <feature_flag>
 *
 *                                      and sent back to the LEAF:
 *
 *                              +---    "s feats <accepted_features>"
 *                              |
 *   uf_features_check():    <--+
 *      Checks wether the responded
 *      features are still accepted
 *      by us. If they are, we set
 *      the flags locally:
 *
 *      dcc[idx].u.bot->uff_flags |= <feature_flag>
 */


typedef struct uff_list_struct {
  struct uff_list_struct *next; /* Pointer to next entry                */
  struct uff_list_struct *prev; /* Pointer to previous entry            */
  uff_table_t *entry;           /* Pointer to entry in table. This is
                                 * not copied or anything, we just refer
                                 * to the original table entry.         */
} uff_list_t;

typedef struct {
  uff_list_t *start;
  uff_list_t *end;
} uff_head_t;

static uff_head_t uff_list;
static char uff_sbuf[512];


/*
 *    Userfile features management functions
 */

static void uff_init(void)
{
  egg_bzero(&uff_list, sizeof(uff_head_t));
}

/* Calculate memory used for list.
 */
static int uff_expmem(void)
{
  uff_list_t *ul;
  int tot = 0;

  for (ul = uff_list.start; ul; ul = ul->next)
    tot += sizeof(uff_list_t);
  return tot;
}

/* Search for a feature in the uff feature list that matches a supplied
 * feature flag. Returns a pointer to the entry in the list or NULL if
 * no feature uses the flag.
 */
static uff_list_t *uff_findentry_byflag(int flag)
{
  uff_list_t *ul;

  for (ul = uff_list.start; ul; ul = ul->next)
    if (ul->entry->flag & flag)
      return ul;
  return NULL;
}

/* Search for a feature in the uff feature list. Returns a pointer to the
 * entry in the list or NULL if no such feature exists.
 */
static uff_list_t *uff_findentry_byname(char *feature)
{
  uff_list_t *ul;

  for (ul = uff_list.start; ul; ul = ul->next)
    if (!strcmp(ul->entry->feature, feature))
      return ul;
  return NULL;
}

/* Insert entry into sorted list.
 */
static void uff_insert_entry(uff_list_t *nul)
{
  uff_list_t *ul, *lul = NULL;

  ul = uff_list.start;
  while (ul && ul->entry->priority < nul->entry->priority) {
    lul = ul;
    ul = ul->next;
  }

  nul->prev = NULL;
  nul->next = NULL;
  if (lul) {
    if (lul->next)
      lul->next->prev = nul;
    nul->next = lul->next;
    nul->prev = lul;
    lul->next = nul;
  } else if (ul) {
    uff_list.start->prev = nul;
    nul->next = uff_list.start;
    uff_list.start = nul;
  } else
    uff_list.start = nul;
  if (!nul->next)
    uff_list.end = nul;
}

/* Remove entry from sorted list.
 */
static void uff_remove_entry(uff_list_t *ul)
{
  if (!ul->next)
    uff_list.end = ul->prev;
  else
    ul->next->prev = ul->prev;
  if (!ul->prev)
    uff_list.start = ul->next;
  else
    ul->prev->next = ul->next;
}

/* Add a single feature to the list.
 */
static void uff_addfeature(uff_table_t *ut)
{
  uff_list_t *ul;

  if (uff_findentry_byname(ut->feature)) {
    putlog(LOG_MISC, "*", "(!) share: same feature name used twice: %s",
           ut->feature);
    return;
  }
  ul = uff_findentry_byflag(ut->flag);
  if (ul) {
    putlog(LOG_MISC, "*", "(!) share: feature flag %d used twice by %s and %s",
           ut->flag, ut->feature, ul->entry->feature);
    return;
  }
  ul = nmalloc(sizeof(uff_list_t));
  ul->entry = ut;
  uff_insert_entry(ul);
}

/* Add a complete table to the list.
 */
static void uff_addtable(uff_table_t *ut)
{
  if (!ut)
    return;
  for (; ut->feature; ++ut)
    uff_addfeature(ut);
}

/* Remove a single feature from the list.
 */
static int uff_delfeature(uff_table_t *ut)
{
  uff_list_t *ul;

  for (ul = uff_list.start; ul; ul = ul->next)
    if (!strcmp(ul->entry->feature, ut->feature)) {
      uff_remove_entry(ul);
      nfree(ul);
      return 1;
    }
  return 0;
}

/* Remove a complete table from the list.
 */
static void uff_deltable(uff_table_t *ut)
{
  if (!ut)
    return;
  for (; ut->feature; ++ut)
    (int) uff_delfeature(ut);
}


/*
 *    Userfile feature parsing functions
 */

/* Parse the given features string, set internal flags apropriately and
 * eventually respond with all features we will use.
 */
static void uf_features_parse(int idx, char *par)
{
  char *buf, *s, *p;
  uff_list_t *ul;

  uff_sbuf[0] = 0; /* Reset static buffer  */
  p = s = buf = nmalloc(strlen(par) + 1); /* Allocate temp buffer */
  strcpy(buf, par);

  /* Clear all currently set features. */
  dcc[idx].u.bot->uff_flags = 0;

  /* Parse string */
  while ((s = strchr(s, ' ')) != NULL) {
    *s = '\0';

    /* Is the feature available and active? */
    ul = uff_findentry_byname(p);
    if (ul && (ul->entry->ask_func == NULL || ul->entry->ask_func(idx))) {
      dcc[idx].u.bot->uff_flags |= ul->entry->flag; /* Set flag */
      strcat(uff_sbuf, ul->entry->feature); /* Add feature to list */
      strcat(uff_sbuf, " ");
    }
    p = ++s;
  }
  nfree(buf);

  /* Send response string                                               */
  if (uff_sbuf[0])
    dprintf(idx, "s feats %s\n", uff_sbuf);
}

/* Return a list of features we are supporting.
 */
static char *uf_features_dump(int idx)
{
  uff_list_t *ul;

  uff_sbuf[0] = 0;
  for (ul = uff_list.start; ul; ul = ul->next)
    if (ul->entry->ask_func == NULL || ul->entry->ask_func(idx)) {
      strcat(uff_sbuf, ul->entry->feature); /* Add feature to list  */
      strcat(uff_sbuf, " ");
    }
  return uff_sbuf;
}

static int uf_features_check(int idx, char *par)
{
  char *buf, *s, *p;
  uff_list_t *ul;

  uff_sbuf[0] = 0; /* Reset static buffer  */
  p = s = buf = nmalloc(strlen(par) + 1); /* Allocate temp buffer */
  strcpy(buf, par);

  /* Clear all currently set features. */
  dcc[idx].u.bot->uff_flags = 0;

  /* Parse string */
  while ((s = strchr(s, ' ')) != NULL) {
    *s = '\0';

    /* Is the feature available and active? */
    ul = uff_findentry_byname(p);
    if (ul && (ul->entry->ask_func == NULL || ul->entry->ask_func(idx)))
      dcc[idx].u.bot->uff_flags |= ul->entry->flag; /* Set flag */
    else {
      /* It isn't, and our hub wants to use it! This either happens
       * because the hub doesn't look at the features we suggested to
       * use or because our admin changed the flags, so that formerly
       * active features are now deactivated.
       *
       * In any case, we abort user file sharing.
       */
      putlog(LOG_BOTS, "*", "Bot %s tried unsupported feature!", dcc[idx].nick);
      dprintf(idx, "s e Attempt to use an unsupported feature\n");
      zapfbot(idx);

      nfree(buf);
      return 0;
    }
    p = ++s;
  }
  nfree(buf);
  return 1;
}

/* Call all active feature functions, sorted by their priority. This
 * should be called when we're about to send a user file.
 */
static int uff_call_sending(int idx, char *user_file)
{
  uff_list_t *ul;

  for (ul = uff_list.start; ul; ul = ul->next)
    if (ul->entry && ul->entry->snd &&
        (dcc[idx].u.bot->uff_flags & ul->entry->flag))
      if (!(ul->entry->snd(idx, user_file)))
        return 0; /* Failed! */
  return 1;
}

/* Call all active feature functions, sorted by their priority. This
 * should be called when we've received a user file and are about to
 * parse it.
 */
static int uff_call_receiving(int idx, char *user_file)
{
  uff_list_t *ul;

  for (ul = uff_list.end; ul; ul = ul->prev)
    if (ul->entry && ul->entry->rcv &&
        (dcc[idx].u.bot->uff_flags & ul->entry->flag))
      if (!(ul->entry->rcv(idx, user_file)))
        return 0; /* Failed! */
  return 1;
}


/*
 *    Userfile feature handlers
 */


/* Feature `overbots'
 */

static int uff_ask_override_bots(int idx)
{
  if (overr_local_bots)
    return 1;
  else
    return 0;
}


/*
 *     Internal user file feature table
 */

static uff_table_t internal_uff_table[] = {
  {"overbots", UFF_OVERRIDE, uff_ask_override_bots, 0, NULL, NULL},
  {"invites",  UFF_INVITE,   NULL,                  0, NULL, NULL},
  {"exempts",  UFF_EXEMPT,   NULL,                  0, NULL, NULL},
  {NULL,       0,            NULL,                  0, NULL, NULL}
};
