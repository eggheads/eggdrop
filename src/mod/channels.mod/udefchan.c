/* 
 * udefchan.c -- part of channels.mod
 *   user definable channel flags/settings
 * 
 * $Id: udefchan.c,v 1.2 2000/01/17 22:36:08 fabian Exp $
 */
/* 
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

static int expmem_udef (struct udef_struct *ul)
{
  int i = 0;

  while (ul) {
    i += sizeof(struct udef_struct);
    i += strlen(ul->name) + 1;
    i += expmem_udef_chans(ul->values);
    ul = ul->next;
  }
  return i;
}

static int expmem_udef_chans (struct udef_chans *ul)
{
  int i = 0;
  
  while (ul) {
    i += sizeof(struct udef_chans);
    i += strlen(ul->chan) + 1;
    ul = ul->next;
  }
  return i;
}

static int getudef(struct udef_chans *ul, char *name)
{
  int val = 0;

  Context;
  while (ul) {
    if (!strcasecmp(ul->chan, name)) {
      val = ul->value;
      break;
    }
    ul = ul->next;
  }
  return val;
}

static void setudef(struct udef_struct *us, struct udef_chans *ul, char *name,
		    int value)
{
  struct udef_chans *ull;

  Context;
  ull = ul;
  while (ul) {
    if (!strcasecmp(ul->chan, name)) {
      ul->value = value;
      return;
    }
    ul = ul->next;
  }
  ul = ull;
  while (ul && ul->next)
    ul = ul->next;
  ull = nmalloc(sizeof(struct udef_chans));
  ull->chan = nmalloc(strlen(name) + 1);
  strcpy(ull->chan, name);
  ull->value = value;
  ull->next = NULL;
  if (ul)
    ul->next = ull;
  else
    us->values = ull;
  return;
}
  
static void initudef(int type, char *name, int defined)
{
  struct udef_struct *ul = udef, *ull = NULL;
  int found = 0;

  Context;
  if (strlen(name) < 1)
    return;
  while (ul) {
    if (ul->name && !strcasecmp(ul->name, name)) {
      if (defined) {
        debug1("UDEF: %s defined", ul->name);
        ul->defined = 1;
      }
      found = 1;
    }
    ul = ul->next;
  }
  if (!found) {
    debug2("Creating %s (type %d)", name, type);
    ull = udef;
    while (ull && ull->next)
      ull = ull->next;
    ul = nmalloc(sizeof(struct udef_struct));
    ul->name = nmalloc(strlen(name) + 1);
    strcpy(ul->name, name);
    if (defined)
      ul->defined = 1;
    else
      ul->defined = 0;
    ul->type = type;
    ul->values = NULL;
    ul->next = NULL;
    if (ull)
      ull->next = ul;
    else
      udef = ul;
  }
  return;
}

static void free_udef(struct udef_struct *ul)
{
  struct udef_struct *ull;

  while (ul) {
    ull = ul->next;
    free_udef_chans(ul->values);
    nfree(ul->name);
    nfree(ul);
    ul = ull;
  }
  return;
}

static void free_udef_chans(struct udef_chans *ul)
{
  struct udef_chans *ull;
  
  while (ul) {
    ull = ul->next;
    nfree(ul->chan);
    nfree(ul);
    ul = ull;
  }
  return;
}
