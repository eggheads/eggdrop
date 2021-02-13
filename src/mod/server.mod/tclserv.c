/*
 * tclserv.c -- part of server.mod
 *
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2021 Eggheads Development Team
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

static int tcl_isbotnick STDVAR
{
  BADARGS(2, 2, " nick");

  if (match_my_nick(argv[1]))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_putnow STDVAR
{
  int len;
  char buf[MSGMAX], *p, *q, *r;

  BADARGS(2, 3, " text ?options?");

  if ((argc == 3) && strcasecmp(argv[2], "-oneline")) {
    Tcl_AppendResult(irp, "unknown putnow option: should be ",
                     "-oneline", NULL);
    return TCL_ERROR;
  }
  if (!serv) /* no server - no output */
    return TCL_OK;

  for (p = r = argv[1], q = buf; ; p++) {
    if (*p && *p != '\r' && *p != '\n')
      continue; /* look for message delimiters */
    if (p == r) { /* empty message */
      if (*p) {
        r++;
        continue;
      } else
        break;
    }
    if ((p - r) > (sizeof(buf) - 2 - (q - buf)))
      break; /* That's all folks, no space left */
    len = p - r + 1; /* leave space for '\0' */
    strlcpy(q, r, len);
    if (check_tcl_out(0, q, 0)) {
      if (!*p || ((argc == 3) && !strcasecmp(argv[2], "-oneline")))
        break;
      r = p + 1;
      continue;
    }
    check_tcl_out(0, q, 1);
    if (q == buf)
      putlog(LOG_SRVOUT, "*", "[r->] %s", q);
    else
      putlog(LOG_SRVOUT, "*", "[+r->] %s", q);
    q += len - 1; /* the '\0' must be overwritten */
    *q++ = '\r';
    *q++ = '\n'; /* comply with the RFC */
    if (!*p || ((argc == 3) && !strcasecmp(argv[2], "-oneline")))
      break; /* cut on newline requested or message ended */
    r = p + 1;
  }
  tputs(serv, buf, q - buf); /* q points after the last '\n' */
  return TCL_OK;
}

static int tcl_putquick STDVAR
{
  char s[MSGMAX], *p;

  BADARGS(2, 3, " text ?options?");

  if ((argc == 3) && strcasecmp(argv[2], "-next") &&
      strcasecmp(argv[2], "-normal")) {
    Tcl_AppendResult(irp, "unknown putquick option: should be one of: ",
                     "-normal -next", NULL);
    return TCL_ERROR;
  }
  strlcpy(s, argv[1], sizeof s);

  p = strchr(s, '\n');
  if (p != NULL)
    *p = 0;
  p = strchr(s, '\r');
  if (p != NULL)
    *p = 0;
  if (argc == 3 && !strcasecmp(argv[2], "-next"))
    dprintf(DP_MODE_NEXT, "%s\n", s);
  else
    dprintf(DP_MODE, "%s\n", s);
  return TCL_OK;
}

static int tcl_putserv STDVAR
{
  char s[MSGMAX], *p;

  BADARGS(2, 3, " text ?options?");

  if ((argc == 3) && strcasecmp(argv[2], "-next") &&
      strcasecmp(argv[2], "-normal")) {
    Tcl_AppendResult(irp, "unknown putserv option: should be one of: ",
                     "-normal -next", NULL);
    return TCL_ERROR;
  }
  strlcpy(s, argv[1], sizeof s);

  p = strchr(s, '\n');
  if (p != NULL)
    *p = 0;
  p = strchr(s, '\r');
  if (p != NULL)
    *p = 0;
  if (argc == 3 && !strcasecmp(argv[2], "-next"))
    dprintf(DP_SERVER_NEXT, "%s\n", s);
  else
    dprintf(DP_SERVER, "%s\n", s);
  return TCL_OK;
}

static int tcl_puthelp STDVAR
{
  char s[MSGMAX], *p;

  BADARGS(2, 3, " text ?options?");

  if ((argc == 3) && strcasecmp(argv[2], "-next") &&
      strcasecmp(argv[2], "-normal")) {
    Tcl_AppendResult(irp, "unknown puthelp option: should be one of: ",
                     "-normal -next", NULL);
    return TCL_ERROR;
  }
  strlcpy(s, argv[1], sizeof s);

  p = strchr(s, '\n');
  if (p != NULL)
    *p = 0;
  p = strchr(s, '\r');
  if (p != NULL)
    *p = 0;
  if (argc == 3 && !strcasecmp(argv[2], "-next"))
    dprintf(DP_HELP_NEXT, "%s\n", s);
  else
    dprintf(DP_HELP, "%s\n", s);
  return TCL_OK;
}

/* Get the user's account name from Eggdrop's internal list if a) they are
  * logged in and b) Eggdrop has seen it.
  */
static int tcl_getaccount STDVAR {
  memberlist *m;
  struct chanset_t *chan, *thechan = NULL;

  BADARGS(2, 3, " nickname ?channel?");

  if (argc > 2) {
    chan = findchan_by_dname(argv[2]);
    thechan = chan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else {
    chan = chanset;
  }
  while (chan && (thechan == NULL || thechan == chan)) {
    if ((m = ismember(chan, argv[1]))) {
      Tcl_AppendResult(irp, m->account, NULL);
      return TCL_OK;
    }
    chan = chan->next;
  }
  Tcl_AppendResult(irp, "", NULL);
  return TCL_OK;
}

static int tcl_isidentified STDVAR {
  memberlist *m;
  struct chanset_t *chan, *thechan = NULL;

  BADARGS(2, 3, " nickname ?channel?");

  if (argc > 2) {
    chan = findchan_by_dname(argv[2]);
    thechan = chan;
    if (!thechan) {
      Tcl_AppendResult(irp, "illegal channel: ", argv[2], NULL);
      return TCL_ERROR;
    }
  } else {
    chan = chanset;
  }
  while (chan && (thechan == NULL || thechan == chan)) {
    if ((m = ismember(chan, argv[1]))) {
      if (strcmp(m->account, "")) {
        Tcl_AppendResult(irp, "1", NULL);
        return TCL_OK;
      }
    }
    chan = chan->next;
  }
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

/* Send a msg to the server prefixed with an IRCv3 message-tag */
static int tcl_tagmsg STDVAR {
  char tag[CLITAGMAX-9];    /* minus @, TAGMSG and two spaces */
  char tagdict[CLITAGMAX-9];
  char target[MSGMAX];
  char *p;
  int taglen = 0, i = 1;
  BADARGS(3, 3, " tag target");

  if (!msgtag) {
    Tcl_AppendResult(irp, "message-tags not enabled, cannot send tag", NULL);
    return TCL_ERROR;
  }
  strlcpy(tagdict, argv[1], sizeof tag);
  strlcpy(target, argv[2], sizeof target);
  p = strtok(tagdict, " ");
  while (p != NULL) {
    if ((i % 2) != 0) {
      taglen += egg_snprintf(tag + taglen, CLITAGMAX - 9 - taglen, "%s", p);
    } else {
      if (strcmp(p, "{}") != 0) {
        taglen += egg_snprintf(tag + taglen, CLITAGMAX - 9 - taglen, "=%s;", p);
      } else {
        taglen += egg_snprintf(tag + taglen, CLITAGMAX - 9 - taglen, ";");
      }
    }
    i++;
    p = strtok(NULL, " ");
  }
  p = strchr(target, '\n');
  if (p != NULL)
    *p = 0;
  p = strchr(target, '\r');
  if (p != NULL)
    *p = 0;
  dprintf(DP_SERVER, "@%s TAGMSG %s\n", tag, target);
  return TCL_OK;
}


/* Tcl interface to send CAP messages to server */
static int tcl_cap STDVAR {
  char s[CAPMAX];
  BADARGS(2, 3, " sub-cmd ?arg?");

  /* List capabilities available on server */
  if (!strcasecmp(argv[1], "ls")) {
    Tcl_AppendResult(irp, cap.supported, NULL);
  /* List capabilities Eggdrop is internally tracking as enabled with server */
  } else if (!strcasecmp(argv[1], "enabled")) {
    Tcl_AppendResult(irp, cap.negotiated, NULL);
  /* Send a request to negotiate a capability with server */
  } else if (!strcasecmp(argv[1], "req")) {
    if (argc != 3) {
      Tcl_AppendResult(irp, "No CAP request provided", NULL);
      return TCL_ERROR;
    } else {
      simple_sprintf(s, "CAP REQ :%s", argv[2]);
      dprintf(DP_SERVER, "%s\n", s);
    }
  /* Send a raw CAP command to the server */
  } else if (!strcasecmp(argv[1], "raw")) {
    if (argc == 3) {
      simple_sprintf(s, "CAP %s", argv[2]);
      dprintf(DP_SERVER, "%s\n", s);
    } else {
      Tcl_AppendResult(irp, "Raw requires a CAP sub-command to be provided",
        NULL);
      return TCL_ERROR;
    }
  } else {
      Tcl_AppendResult(irp, "Invalid cap command", NULL);
  }
  return TCL_OK;
}


static int tcl_jump STDVAR
{
  BADARGS(1, 4, " ?server? ?port? ?pass?");

  if (argc >= 2) {
    strlcpy(newserver, argv[1], sizeof newserver);
    if (argc >= 3)
#ifdef TLS
    {
      if (*argv[2] == '+')
        use_ssl = 1;
      else
        use_ssl = 0;
      newserverport = atoi(argv[2]);
    }
#else
      newserverport = atoi(argv[2]);
#endif
    else
      newserverport = default_port;
    if (argc == 4)
      strlcpy(newserverpass, argv[3], sizeof newserverpass);
  }
  cycle_time = 0;

  nuke_server("changing servers\n");
  return TCL_OK;
}

static int tcl_clearqueue STDVAR
{
  struct msgq *q, *qq;
  int msgs = 0;
  char s[20];

  BADARGS(2, 2, " queue");

  if (!strcmp(argv[1], "all")) {
    msgs = (int) (modeq.tot + mq.tot + hq.tot);
    for (q = modeq.head; q; q = qq) {
      qq = q->next;
      nfree(q->msg);
      nfree(q);
    }
    for (q = mq.head; q; q = qq) {
      qq = q->next;
      nfree(q->msg);
      nfree(q);
    }
    for (q = hq.head; q; q = qq) {
      qq = q->next;
      nfree(q->msg);
      nfree(q);
    }
    modeq.tot = mq.tot = hq.tot = modeq.warned = mq.warned = hq.warned = 0;
    mq.head = hq.head = modeq.head = mq.last = hq.last = modeq.last = 0;
    double_warned = 0;
    burst = 0;
    simple_sprintf(s, "%d", msgs);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  } else if (!strncmp(argv[1], "serv", 4)) {
    msgs = mq.tot;
    for (q = mq.head; q; q = qq) {
      qq = q->next;
      nfree(q->msg);
      nfree(q);
    }
    mq.tot = mq.warned = 0;
    mq.head = mq.last = 0;
    if (modeq.tot == 0)
      burst = 0;
    double_warned = 0;
    mq.tot = mq.warned = 0;
    mq.head = mq.last = 0;
    simple_sprintf(s, "%d", msgs);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  } else if (!strcmp(argv[1], "mode")) {
    msgs = modeq.tot;
    for (q = modeq.head; q; q = qq) {
      qq = q->next;
      nfree(q->msg);
      nfree(q);
    }
    if (mq.tot == 0)
      burst = 0;
    double_warned = 0;
    modeq.tot = modeq.warned = 0;
    modeq.head = modeq.last = 0;
    simple_sprintf(s, "%d", msgs);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  } else if (!strcmp(argv[1], "help")) {
    msgs = hq.tot;
    for (q = hq.head; q; q = qq) {
      qq = q->next;
      nfree(q->msg);
      nfree(q);
    }
    double_warned = 0;
    hq.tot = hq.warned = 0;
    hq.head = hq.last = 0;
    simple_sprintf(s, "%d", msgs);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  }
  Tcl_AppendResult(irp, "bad option \"", argv[1],
                   "\": must be mode, server, help, or all", NULL);
  return TCL_ERROR;
}

static int tcl_queuesize STDVAR
{
  char s[20];
  int x;

  BADARGS(1, 2, " ?queue?");

  if (argc == 1) {
    x = (int) (modeq.tot + hq.tot + mq.tot);
    simple_sprintf(s, "%d", x);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  } else if (!strncmp(argv[1], "serv", 4)) {
    x = (int) (mq.tot);
    simple_sprintf(s, "%d", x);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  } else if (!strcmp(argv[1], "mode")) {
    x = (int) (modeq.tot);
    simple_sprintf(s, "%d", x);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  } else if (!strcmp(argv[1], "help")) {
    x = (int) (hq.tot);
    simple_sprintf(s, "%d", x);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  }

  Tcl_AppendResult(irp, "bad option \"", argv[1],
                   "\": must be mode, server, or help", NULL);
  return TCL_ERROR;
}

static int tcl_server STDVAR {
  int ret;

  BADARGS(2, 5, " subcommand host ?port ?password??");
  if (!strcmp(argv[1], "add")) {
    ret = add_server(argv[2], argv[3] ? argv[3] : "", argv[4] ? argv[4] : "");
  } else if (!strcmp(argv[1], "remove")) {
    ret = del_server(argv[2], argv[3] ? argv[3] : "");
  } else {
    Tcl_AppendResult(irp, "Invalid subcommand: ", argv[1],
        ". Should be \"add\" or \"remove\"", NULL);
    return TCL_ERROR;
  }
  if (ret == 0) {
    return TCL_OK;
  }
  if (ret == 1) {
    Tcl_AppendResult(irp, "A ':' was detected in the non-IPv6 address ", argv[2],
            " Make sure the port is separated by a space, not a ':'. ", NULL);
  } else if (ret == 2) {
    Tcl_AppendResult(irp, "Attempted to add SSL-enabled server, but Eggdrop "
            "was not compiled with SSL libraries.", NULL);
  } else if (ret == 3) {    /* del_server only */
    Tcl_AppendResult(irp, "Server ", argv[2], argv[3] ? ":" : "",
            argv[3] ? argv[3] : ""," not found.", NULL);
  }
  return TCL_ERROR;
}

static tcl_cmds my_tcl_cmds[] = {
  {"jump",          tcl_jump},
  {"cap",           tcl_cap},
  {"isbotnick",     tcl_isbotnick},
  {"clearqueue",    tcl_clearqueue},
  {"queuesize",     tcl_queuesize},
  {"puthelp",       tcl_puthelp},
  {"putserv",       tcl_putserv},
  {"putquick",      tcl_putquick},
  {"putnow",        tcl_putnow},
  {"tagmsg",        tcl_tagmsg},
  {"server",        tcl_server},
  {"getaccount",    tcl_getaccount},
  {"isidentified",  tcl_isidentified},
  {NULL,         NULL}
};
