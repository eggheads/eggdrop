/* 
 * cmdsnote.c -- part of notes.mod
 *   handles all notes interaction over the party line
 * 
 * $Id: cmdsnote.c,v 1.6 2000/01/08 21:23:16 per Exp $
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

static void cmd_pls_noteign(struct userrec *u, int idx, char *par)
{
  struct userrec *u2;
  char *handle, *mask, *buf, *p;
  if (!par[0]) {
    dprintf(idx, "Usage: +noteign [handle] <ignoremask>\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# +noteign %s", dcc[idx].nick, par);

  p = buf = nmalloc(strlen(par)+1);
  strcpy(p, par);
  handle = newsplit(&p);
  mask = newsplit(&p);
  if (mask[0]) {
    u2 = get_user_by_handle(userlist, handle);
    if (u != u2) {
      struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
      get_user_flagrec(u, &fr, dcc[idx].u.chat->con_chan);
      if (!(glob_master(fr) || glob_owner(fr))) {
	dprintf(idx, "You are not allowed to change note ignores for %s\n",
		handle);
	nfree(buf);
        return;
      }
    }
    if (!u2) {
      dprintf(idx, "User %s does not exist.\n", handle);
      nfree(buf);
      return;
    }
  } else {
    u2 = u;
    mask = handle;
  }
  if (add_note_ignore(u2, mask))
    dprintf(idx, "Now ignoring notes from %s\n", mask);
  else
    dprintf(idx, "Already ignoring %s\n", mask);
  nfree(buf);
  return;
}

static void cmd_mns_noteign(struct userrec *u, int idx, char *par)
{
  struct userrec *u2;
  char *handle, *mask, *buf, *p;
  if (!par[0]) {
    dprintf(idx, "Usage: -noteign [handle] <ignoremask>\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# -noteignore %s", dcc[idx].nick, par);
  p = buf = nmalloc(strlen(par)+1);
  strcpy(p, par);
  handle = newsplit(&p);
  mask = newsplit(&p);
  if (mask[0]) {
    u2 = get_user_by_handle(userlist, handle);
    if (u != u2) {
      struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
      get_user_flagrec(u, &fr, dcc[idx].u.chat->con_chan);
      if (!(glob_master(fr) || glob_owner(fr))) {
	dprintf(idx, "You are not allowed to change note ignores for %s\n",
		handle);
	nfree(buf);
        return;
      }
    }
    if (!u2) {
      dprintf(idx, "User %s does not exist.\n", handle);
      nfree(buf);
      return;
    }
  } else {
    u2 = u;
    mask = handle;
  }

  if (del_note_ignore(u2, mask))
    dprintf(idx, "No longer ignoring notes from %s\n", mask);
  else
    dprintf(idx, "Note ignore %s not found in list.\n", mask);
  nfree(buf);
  return;
}

static void cmd_noteigns(struct userrec *u, int idx, char *par)
{
  struct userrec *u2;
  char **ignores;
  int ignoresn, i;

  Context;
  if (par[0]) {
    u2 = get_user_by_handle(userlist, par);
    if (u != u2) {
      struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};
      get_user_flagrec(u, &fr, dcc[idx].u.chat->con_chan);
      if (!(glob_master(fr) || glob_owner(fr))) {
	dprintf(idx, "You are not allowed to change note ignores for %s\n",
		par);
        return;
      }
    }
    if (!u2) {
      dprintf(idx, "User %s does not exist.\n", par);
      return;
    }
  } else
    u2 = u;

  ignoresn = get_note_ignores(u2, &ignores);
  if (!ignoresn) {
    dprintf(idx, "No note ignores present.\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# noteigns %s", dcc[idx].nick, par);
  dprintf(idx, "Note ignores for %s:\n", u2->handle);
  for (i = 0; i < ignoresn; i++)
    dprintf(idx, " %s", ignores[i]);
  dprintf(idx, "\n");
  Context;
  nfree(ignores[0]); /* free the string buffer */
  nfree(ignores); /* free the ptr array */
}

static void cmd_fwd(struct userrec *u, int idx, char *par)
{
  char *handle;
  struct userrec *u1;

  if (!par[0]) {
    dprintf(idx, "Usage: fwd <handle> [user@bot]\n");
    return;
  }
  handle = newsplit(&par);
  u1 = get_user_by_handle(userlist, handle);
  if (!u1) {
    dprintf(idx, "No such user.\n");
    return;
  }
  if ((u1->flags & USER_OWNER) && strcasecmp(handle, dcc[idx].nick)) {
    dprintf(idx, "Can't change notes forwarding of the bot owner.\n");
    return;
  }
  if (!par[0]) {
    putlog(LOG_CMDS, "*", "#%s# fwd %s", dcc[idx].nick, handle);
    dprintf(idx, "Wiped notes forwarding for %s\n", handle);
    set_user(&USERENTRY_FWD, u1, NULL);
    return;
  }
  /* thanks to vertex & dw */
  if (strchr(par, '@') == NULL) {
    dprintf(idx, "You must supply a botname to forward to.\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# fwd %s %s", dcc[idx].nick, handle, par);
  dprintf(idx, "Changed notes forwarding for %s to: %s\n", handle, par);
  set_user(&USERENTRY_FWD, u1, par);
}

static void cmd_notes(struct userrec *u, int idx, char *par)
{
  char *fcn;

  if (!par[0]) {
    dprintf(idx, "Usage: notes index\n");
    dprintf(idx, "       notes read <# or ALL>\n");
    dprintf(idx, "       notes erase <# or ALL>\n");
    dprintf(idx, "       # may be numbers and/or intervals separated by ;\n");
    dprintf(idx, "       ex: notes erase 2-4;8;16-\n");
    return;
  }
  fcn = newsplit(&par);
  if (!strcasecmp(fcn, "index"))
    notes_read(dcc[idx].nick, "", "+", idx);
  else if (!strcasecmp(fcn, "read")) {
    if (!strcasecmp(par, "all"))
      notes_read(dcc[idx].nick, "", "-", idx);
    else
      notes_read(dcc[idx].nick, "", par, idx);
  } else if (!strcasecmp(fcn, "erase")) {
    if (!strcasecmp(par, "all"))
      notes_del(dcc[idx].nick, "", "-", idx);
    else
      notes_del(dcc[idx].nick, "", par, idx);
  } else {
    dprintf(idx, "Function must be one of INDEX, READ, or ERASE.\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# notes %s %s", dcc[idx].nick, fcn, par);
}

void cmd_note(struct userrec *u, int idx, char *par)
{
  char handle[512], *p;
  int echo;

  if (!par[0]) {
    dprintf(idx, "Usage: note <to-whom> <message>\n");
    return;
  }
  /* could be file system user */
  p = newsplit(&par);
  while ((*par == ' ') || (*par == '<') || (*par == '>'))
    par++;			/* these are now illegal *starting* notes
				 * characters */
  echo = (dcc[idx].status & STAT_ECHO);
  splitc(handle, p, ',');
  while (handle[0]) {
    rmspace(handle);
    add_note(handle, dcc[idx].nick, par, idx, echo);
    splitc(handle, p, ',');
  }
  rmspace(p);
  add_note(p, dcc[idx].nick, par, idx, echo);
}

cmd_t notes_cmds[] =
{
  {"fwd", "m", (Function) cmd_fwd, NULL},
  {"notes", "", (Function) cmd_notes, NULL},
  {"+noteign", "", (Function) cmd_pls_noteign, NULL},
  {"-noteign", "", (Function) cmd_mns_noteign, NULL},
  {"noteigns", "", (Function) cmd_noteigns, NULL},
  {"note", "", (Function) cmd_note, NULL},
  {0, 0, 0, 0}
};
