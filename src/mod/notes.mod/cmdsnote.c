/*
 * cmdsnote.c - handling all interaction over the party line */
/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
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

  context;
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
  context;
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

cmd_t notes_cmds[] =
{
  {"fwd", "m", (Function) cmd_fwd, NULL},
  {"notes", "", (Function) cmd_notes, NULL},
  {"+noteign", "", (Function) cmd_pls_noteign, NULL},
  {"-noteign", "", (Function) cmd_mns_noteign, NULL},
  {"noteigns", "", (Function) cmd_noteigns, NULL},
};
