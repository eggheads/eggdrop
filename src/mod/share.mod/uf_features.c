/* 
 * uf_features.c -- part of share.mod
 * 
 * $Id: uf_features.c,v 1.1 2000/01/22 23:04:04 fabian Exp $
 */
/* 
 * Copyright (C) 2000  Eggheads
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
 *		 LEAF				       HUB
 *
 *   uf_features_dump():
 *	Finds out which features
 *	it supports / wants to use
 *	and then dumps those. The
 *	list is appended to the
 *	userfile send ack.
 *
 *	"s uy <features>"   --+
 *			      |
 *			      +-->   uf_features_parse():
 *					Parses the given list of features,
 *					given in a string, seperated with
 *					spaces. Decides which features to
 *					accept/use. Those features are then
 *					locally set:
 *
 *					dcc[idx].status |= STAT_UFF_<feature>
 *
 *					and sent back to the LEAF:
 *
 *				+---	"s feats <accepted_features>"
 *				|
 *   uf_features_check():    <--+
 *	Checks wether the responded
 *	features are still accepted
 *	by us. If they are, we set
 *	the flags locally:
 *
 *	dcc[idx].status |= STAT_UFF_<feature>
 */


typedef struct uff_list_struct {
  struct uff_list_struct *next;	/* Pointer to next entry		*/
  uff_table_t *entry;		/* Pointer to entry in table. This is
				   not copied or anything, we just refer
				   to the original table entry.		*/
} uff_list_t;

uff_list_t *uff_list = NULL;
static char uff_sbuf[512];


/*
 *    Userfile features management functions
 */

/* Calculate memory used for list.
 */
static int uff_expmem(void)
{
  uff_list_t *ul;
  int tot = 0;

  for (ul = uff_list; ul; ul = ul->next)
    tot += sizeof(uff_list_t);
  return tot;
}

/* Search for a feature in the uff feature list. Returns a pointer to the
 * entry in the list or NULL if no such feature exists.
 */
static uff_list_t *uff_findentry(char *feature)
{
  uff_list_t *ul;

  for (ul = uff_list; ul; ul = ul->next)
    if (!strcmp(ul->entry->feature, feature))
      return ul;
  return NULL;
}

/* Add a single feature to the list.
 */
static void uff_addfeature(uff_table_t *ut)
{
  uff_list_t *ul;

  if (uff_findentry(ut->feature))
    return;
  ul = nmalloc(sizeof(uff_list_t));
  ul->next = uff_list;
  uff_list = ul;
  ul->entry = ut;
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
  uff_list_t *ul, *oul;

  for (ul = uff_list, oul = NULL; ul; oul = ul, ul = ul->next)
    if (!strcmp(ul->entry->feature, ut->feature)) {
      if (!oul)
	uff_list = ul->next;
      else
	oul->next = ul->next;
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

/* Clear all flags.
 */
static void uff_clear(int idx)
{
  uff_list_t *ul;

  for (ul = uff_list; ul; ul = ul->next)
    dcc[idx].status &= ~ul->entry->flag;
}

/* Parse the given features string, set internal flags apropriately and
 * eventually respond with all features we will use.
 */
static void uf_features_parse(int idx, char *par)
{
  char *buf, *s, *p;
  uff_list_t *ul;

  uff_sbuf[0] = 0;				/* Reset static buffer	*/
  p = s = buf = nmalloc(strlen(par) + 1);	/* Allocate temp buffer	*/
  strcpy(buf, par);

  /* Clear all currently set features. */
  uff_clear(idx);

  /* Parse string */
  while ((s = strchr(s, ' ')) != NULL) {
    *s = '\0';

    /* Is the feature available and active? */
    ul = uff_findentry(p);
    if (ul && ul->entry->ask_func(idx)) {
      dcc[idx].status |= ul->entry->flag;	/* Set flag		*/
      strcat(uff_sbuf, ul->entry->feature);	/* Add feature to list	*/
      strcat(uff_sbuf, " ");
    }
    p = ++s;
  }
  nfree(buf);

  /* Send response string						*/
  if (uff_sbuf[0])
    dprintf(idx, "s feats %s\n", uff_sbuf);
}

/* Return a list of features we are supporting.
 */
static char *uf_features_dump(int idx)
{
  uff_list_t *ul;

  uff_sbuf[0] = 0;
  for (ul = uff_list; ul; ul = ul->next)
    if (ul->entry->ask_func(idx)) {
      strcat(uff_sbuf, ul->entry->feature);	/* Add feature to list	*/
      strcat(uff_sbuf, " ");
    }
  return uff_sbuf;
}

static int uf_features_check(int idx, char *par)
{
  char *buf, *s, *p;
  uff_list_t *ul;

  uff_sbuf[0] = 0;				/* Reset static buffer	*/
  p = s = buf = nmalloc(strlen(par) + 1);	/* Allocate temp buffer	*/
  strcpy(buf, par);

  /* Clear all currently set features. */
  uff_clear(idx);

  /* Parse string */
  while ((s = strchr(s, ' ')) != NULL) {
    *s = '\0';

    /* Is the feature available and active? */
    ul = uff_findentry(p);
    if (ul && ul->entry->ask_func(idx))
      dcc[idx].status |= ul->entry->flag;	/* Set flag		*/
    else {
      /* It isn't, and our hub wants to use it! This either happens
       * because the hub doesn't look at the features we suggested to
       * use or because our admin changed the flags, so that formerly
       * active features are now deactivated.
       *
       * In any case, we abort userfile sharing.
       */
      putlog(LOG_BOTS, "*", "Bot %s tried unsupported feature!");
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


/* Feature `invite'
 */

static int uff_ask_invite(int idx)
{
  return 1;
}


/* Feature `exempt'
 */

static int uff_ask_exempt(int idx)
{
  return 1;
}


/*
 *     Internal userfile feature table
 */

static uff_table_t internal_uff_table[] = {
  {"overbots",		STAT_UFF_OVERRIDE,	uff_ask_override_bots},
  {"invites",		STAT_UFF_INVITE,	uff_ask_invite},
  {"exempts",		STAT_UFF_EXEMPT,	uff_ask_exempt},
  {NULL,		0,			NULL}
};
