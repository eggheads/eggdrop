/* 
 * server.c - part of server.mod
 * basic irc server support
 */

#define MODULE_NAME "server"
#define MAKING_SERVER
#include "../module.h"
#include "server.h"
#include <netdb.h>

static int ctcp_mode;
static int serv;		/* sock # of server currently */
static int strict_host;		/* strict masking of hosts ? */
static char newserver[121];	/* new server? */
static int newserverport;	/* new server port? */
static char newserverpass[121];	/* new server password? */
static time_t trying_server;	/* trying to connect to a server right now? */
static int server_lag;		/* how lagged (in seconds) is the server? */
static char altnick[NICKLEN];	/* possible alternate nickname to use */
static char raltnick[NICKLEN];	/* random nick created from altnick */
static int curserv;		/* current position in server list: */
static int flud_thr;		/* msg flood threshold */
static int flud_time;		/* msg flood time */
static int flud_ctcp_thr;	/* ctcp flood threshold */
static int flud_ctcp_time;	/* ctcp flood time */
static char initserver[121];	/* what, if anything, to send to the
				 * server on connection */
static char botuserhost[121];	/* bot's user@host (refreshed whenever the
				 * bot joins a channel) */
				/* may not be correct user@host BUT it's
				 * how the server sees it */
static int keepnick;		/* keep trying to regain my intended
				 * nickname? */
static int check_stoned;	/* Check for a stoned server? */
static int serverror_quit;	/* Disconnect from server if ERROR
				 * messages received? */
static int quiet_reject;	/* Quietly reject dcc chat or sends from
				 * users without access? */
static int waiting_for_awake;	/* set when i unidle myself, cleared when
				 * i get the response */
static time_t server_online;	/* server connection time */
static time_t server_cycle_wait;	/* seconds to wait before
					 * re-beginning the server list */
static char botrealname[121];	/* realname of bot */
static int min_servs;		/* minimum number of servers to be around */
static int server_timeout;	/* server timeout for connecting */
static int never_give_up;	/* never give up when connecting to servers? */
static int strict_servernames;	/* don't update server list */
static struct server_list *serverlist;	/* old-style queue, still used by
					 * server list */
static int cycle_time;		/* cycle time till next server connect */
static int default_port;	/* default IRC port */
static char oldnick[NICKLEN];	/* previous nickname *before* rehash */
static int trigger_on_ignore;	/* trigger bindings if user is ignored ? */
static int answer_ctcp;		/* answer how many stacked ctcp's ? */
static int lowercase_ctcp;	/* answer lowercase CTCP's (non-standard) */
static char bothost[81];	/* dont mind me, Im stupid */
static int check_mode_r;	/* check for IRCNET +r modes */
static int use_ison;		/* arthur2 static */
static int net_type;
static int must_be_owner;	/* arthur2 */
static char connectserver[121];	/* what, if anything, to do before connect
				 * to the server */

/* allow a msgs being twice in a queue ? */
static int double_mode;
static int double_server;
static int double_help;
static int double_warned;

static int lastpingtime = 0;	/* IRCNet LAGmeter support -- drummer */

static Function *global = NULL;

static p_tcl_bind_list H_wall, H_raw, H_notc, H_msgm, H_msg, H_flud,
 H_ctcr, H_ctcp;

static void empty_msgq(void);
static void next_server(int *, char *, unsigned int *, char *);
static char *get_altbotnick(void);

#include "servmsg.c"

/* number of seconds to wait between transmitting queued lines to the server
 * lower this value at your own risk.  ircd is known to start flood control
 * at 512 bytes/2 seconds */
#define msgrate 2

/* maximum messages to store in each queue */
static int maxqmsg;
static struct msgq_head mq, hq, modeq;
static int burst;

#include "cmdsserv.c"
#include "tclserv.c"

/***** BOT SERVER QUEUES *****/

/* called periodically to shove out another queued item */
/* mode queue gets priority now */
/* most servers will allow 'busts' of upto 5 msgs...so let's put something
 * in to support flushing modeq a little faster if possible ...
 * will send upto 4 msgs from modeq, and then send 1 msg every time
 * it will *not* send anything from hq until the 'burst' value drops
 * down to 0 again (allowing a sudden mq flood to sneak through) */

static void deq_msg()
{
  struct msgq *q;
  static time_t last_time = 0;
  int ok = 0;

  /* now < last_time tested 'cause clock adjustments could mess it up */
  if (((now - last_time) >= msgrate) || (now < last_time)) {
    last_time = now;
    if (burst > 0)
      burst--;
    ok = 1;
  }
  if (serv < 0)
    return;
  /* send upto 4 msgs to server if the *critical queue* has anything in it */
  if (modeq.head) {
    while (modeq.head && (burst < 4)) {
      tputs(serv, modeq.head->msg, modeq.head->len);
      modeq.tot--;
      q = modeq.head->next;
      nfree(modeq.head->msg);
      nfree(modeq.head);
      modeq.head = q;
      burst++;
    }
    if (!modeq.head)
      modeq.last = 0;
    return;
  }
  /* send something from the normal msg q even if we're slightly bursting */
  if (burst > 1)
    return;
  if (mq.head) {
    burst++;
    tputs(serv, mq.head->msg, mq.head->len);
    mq.tot--;
    q = mq.head->next;
    nfree(mq.head->msg);
    nfree(mq.head);
    mq.head = q;
    if (!mq.head)
      mq.last = 0;
    return;
  }
  /* never send anything from the help queue unless everything else is
   * finished */
  if (!hq.head || burst || !ok)
    return;
  tputs(serv, hq.head->msg, hq.head->len);
  hq.tot--;
  q = hq.head->next;
  nfree(hq.head->msg);
  nfree(hq.head);
  hq.head = q;
  if (!hq.head)
    hq.last = 0;
}

/* clean out the msg queues (like when changing servers) */
static void empty_msgq()
{
  struct msgq *q, *qq;

  q = modeq.head;
  while (q) {
    qq = q->next;
    nfree(q->msg);
    nfree(q);
    q = qq;
  }
  q = mq.head;
  while (q) {
    qq = q->next;
    nfree(q->msg);
    nfree(q);
    q = qq;
  }
  q = hq.head;
  while (q) {
    qq = q->next;
    nfree(q->msg);
    nfree(q);
    q = qq;
  }
  modeq.tot = mq.tot = hq.tot = modeq.warned = mq.warned = hq.warned = 0;
  mq.head = hq.head = modeq.head = mq.last = hq.last = modeq.last = 0;
  burst = 0;
}

/* use when sending msgs... will spread them out so there's no flooding */
static void queue_server(int which, char *buf, int len)
{
  struct msgq_head *h = 0;
  struct msgq *q;
  struct msgq_head tempq;
  struct msgq *tq, *tqq;
  int doublemsg;
  doublemsg = 0;
  if (serv < 0)
    return;			/* don't even BOTHER if there's no server
				 * online */
  /* patch by drummer - no queue for PING and PONG */
  if ((strncasecmp(buf, "PING", 4) == 0) || (strncasecmp(buf, "PONG", 4) == 0)) {
    if ((buf[1] == 73) || (buf[1] == 105)) {
      lastpingtime = now;
    }				/* lagmeter */
    tputs(serv, buf, len);
    return;
  }
  switch (which) {
  case DP_MODE:
    h = &modeq;
    tempq = modeq;
    if (double_mode) doublemsg = 1;
    break;
  case DP_SERVER:
    h = &mq;
    tempq = mq;
    if (double_server) doublemsg = 1;
    break;
  case DP_HELP:
    h = &hq;
    tempq = hq;
    if (double_help) doublemsg = 1;
    break;
  default:
    putlog(LOG_MISC, "*", "!!! queueing unknown type to server!!");
    return;
  }
  if (h->tot < maxqmsg) {
    if (!doublemsg) {   /* Don't queue msg if it's already queued */
      tq = tempq.head;
      while (tq) {
	tqq = tq->next;
	if (!strcasecmp(tq->msg, buf)) {
	  if (!double_warned) {
	    if (buf[len - 1] == '\n')
	      buf[len - 1] = 0;
	    debug1("msg already queued. skipping: %s", buf);
	    double_warned = 1;
	  }
	  return;
	}
	tq = tqq;
      }
    }
    q = nmalloc(sizeof(struct msgq));
    q->next = NULL;
    if (h->head)
      h->last->next = q;
    else
      h->head = q;
    h->last = q;
    q->len = len;
    q->msg = nmalloc(len + 1);
    strcpy(q->msg, buf);
    h->tot++;
    h->warned = 0;
    double_warned = 0;
  } else {
    if (!h->warned)
      putlog(LOG_MISC, "*", "!!! OVER MAXIMUM MODE QUEUE");
    h->warned = 1;
  }
  if (debug_output) {
    if (buf[len - 1] == '\n')
      buf[len - 1] = 0;
    switch (which) {
    case DP_MODE:
      putlog(LOG_SRVOUT, "*", "[!m] %s", buf);
      break;
    case DP_SERVER:
      putlog(LOG_SRVOUT, "*", "[!s] %s", buf);
      break;
    case DP_HELP:
      putlog(LOG_SRVOUT, "*", "[!h] %s", buf);
      break;
    }
  }
  if (which == DP_MODE)
    deq_msg();			/* DP_MODE needs to be ASAP, flush if
				 * possible */
}

/* add someone to a queue */
/* new server to the list */
static void add_server(char *ss)
{
  struct server_list *x, *z = serverlist;
  char *p, *q;

  Context;
  while (z && z->next)
    z = z->next;
  while (ss) {
    p = strchr(ss, ',');
    if (p)
      *p++ = 0;
    x = nmalloc(sizeof(struct server_list));

    x->next = 0;
    x->realname = 0;
    if (z)
      z->next = x;
    else
      serverlist = x;
    z = x;
    q = strchr(ss, ':');
    if (!q) {
      x->port = default_port;
      x->pass = 0;
      x->name = nmalloc(strlen(ss) + 1);
      strcpy(x->name, ss);
    } else {
      *q++ = 0;
      x->name = nmalloc(q - ss);
      strcpy(x->name, ss);
      ss = q;
      q = strchr(ss, ':');
      if (!q) {
	x->pass = 0;
      } else {
	*q++ = 0;
	x->pass = nmalloc(strlen(q) + 1);
	strcpy(x->pass, q);
      }
      x->port = atoi(ss);
    }
    ss = p;
  }
}

/* clear out a list */
static void clearq(struct server_list *xx)
{
  struct server_list *x;

  while (xx) {
    x = xx->next;
    if (xx->name)
      nfree(xx->name);
    if (xx->pass)
      nfree(xx->pass);
    if (xx->realname)
      nfree(xx->realname);
    nfree(xx);
    xx = x;
  }
}

/* set botserver to the next available server
 * -> if (*ptr == -1) then jump to that particular server */
static void next_server(int *ptr, char *serv, unsigned int *port, char *pass)
{
  struct server_list *x = serverlist;
  int i = 0;

  if (x == NULL)
    return;
  /* -1  -->  go to specified server */
  if (*ptr == (-1)) {
    while (x) {
      if (x->port == *port) {
	if (!strcasecmp(x->name, serv)) {
	  *ptr = i;
	  return;
	} else if (x->realname && !strcasecmp(x->realname, serv)) {
	  *ptr = i;
	  strncpy(serv, x->realname, 120);
	  serv[120] = 0;
	  return;
	}
      }
      x = x->next;
      i++;
    }
    /* gotta add it : */
    x = nmalloc(sizeof(struct server_list));

    x->next = 0;
    x->realname = 0;
    x->name = nmalloc(strlen(serv) + 1);
    strcpy(x->name, serv);
    x->port = *port ? *port : default_port;
    if (pass && pass[0]) {
      x->pass = nmalloc(strlen(pass) + 1);
      strcpy(x->pass, pass);
    } else
      x->pass = NULL;
    list_append((struct list_type **) (&serverlist), (struct list_type *) x);
    *ptr = i;
    return;
  }
  /* find where i am and boogie */
  i = (*ptr);
  while ((i > 0) && (x != NULL)) {
    x = x->next;
    i--;
  }
  if (x != NULL) {
    x = x->next;
    (*ptr)++;
  }				/* go to next server */
  if (x == NULL) {
    x = serverlist;
    *ptr = 0;
  }				/* start over at the beginning */
  strcpy(serv, x->name);
  *port = x->port ? x->port : default_port;
  if (x->pass)
    strcpy(pass, x->pass);
  else
    pass[0] = 0;
}

static int server_6char STDVAR {
  Function F = (Function) cd;
  char x[20];

  BADARGS(7, 7, " nick user@host handle desto/chan keyword/nick text");
  CHECKVALIDITY(server_6char);
  sprintf(x, "%d", F(argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]));
  Tcl_AppendResult(irp, x, NULL);
  return TCL_OK;
}

static int server_5char STDVAR {
  Function F = (Function) cd;

  BADARGS(6, 6, " nick user@host handle channel text");
  CHECKVALIDITY(server_5char);
  F(argv[1], argv[2], argv[3], argv[4], argv[5]);
  return TCL_OK;
}

static int server_2char STDVAR {
  Function F = (Function) cd;

  BADARGS(3, 3, " nick msg");
  CHECKVALIDITY(server_2char);
  F(argv[1], argv[2]);
  return TCL_OK;
}

static int server_msg STDVAR {
  Function F = (Function) cd;

  BADARGS(5, 5, " nick uhost hand buffer");
  CHECKVALIDITY(server_msg);
  F(argv[1], argv[2], get_user_by_handle(userlist, argv[3]), argv[4]);
  return TCL_OK;
}

static int server_raw STDVAR {
  Function F = (Function) cd;

  BADARGS(4, 4, " from code args");
  CHECKVALIDITY(server_raw);
  Tcl_AppendResult(irp, int_to_base10(F(argv[1], argv[3])), NULL);
  return TCL_OK;
}

/* read/write normal string variable */
static char *nick_change(ClientData cdata, Tcl_Interp * irp, char *name1,
			 char *name2, int flags)
{
  char *new;

  if (flags & (TCL_TRACE_READS | TCL_TRACE_UNSETS)) {
    Tcl_SetVar2(interp, name1, name2, origbotname, TCL_GLOBAL_ONLY);
    if (flags & TCL_TRACE_UNSETS)
      Tcl_TraceVar(irp, name1, TCL_TRACE_READS | TCL_TRACE_WRITES |
        	   TCL_TRACE_UNSETS, nick_change, cdata);
  } else {			/* writes */
    new = Tcl_GetVar2(interp, name1, name2, TCL_GLOBAL_ONLY);
    if (rfc_casecmp(origbotname, new)) {
      if (origbotname[0])
	putlog(LOG_MISC, "*", "* IRC NICK CHANGE: %s -> %s",
	       origbotname, new);
      strncpy(origbotname, new, NICKMAX);
      origbotname[NICKMAX] = 0;
      if (server_online)
	dprintf(DP_MODE, "NICK %s\n", origbotname);
    }
  }
  return NULL;
}

/* replace all '?'s in s with a random number */
static void rand_nick(char *nick)
{
  char *p = nick;
  while ((p = strchr(p, '?')) != NULL) {
    *p = '0' + rand() % 10;
    p++;
  }
}

/* return the alternative bot nick */
static char *get_altbotnick(void)
{
  Context;
  /* a random-number nick? */
  if (strchr(altnick, '?')) {
    if (!raltnick[0]) {
      strncpy(raltnick, altnick, NICKMAX);
      raltnick[NICKMAX] = 0;
      rand_nick(raltnick);
    }
    return raltnick;
  } else
    return altnick;
}

static char *altnick_change(ClientData cdata, Tcl_Interp * irp, char *name1,
			    char *name2, int flags)
{
  Context;
  /* always unset raltnick. Will be regenerated when needed. */
  raltnick[0] = 0;
  return NULL;
}

static char *traced_server(ClientData cdata, Tcl_Interp * irp, char *name1,
			   char *name2, int flags)
{
  char s[1024];

  if (server_online) {
    int servidx = findanyidx(serv);
    simple_sprintf(s, "%s:%u", dcc[servidx].host, dcc[servidx].port);
  } else
    s[0] = 0;
  Tcl_SetVar2(interp, name1, name2, s, TCL_GLOBAL_ONLY);
  if (flags & TCL_TRACE_UNSETS)
    Tcl_TraceVar(irp, name1, TCL_TRACE_READS | TCL_TRACE_WRITES |
		 TCL_TRACE_UNSETS, traced_server, cdata);
  return NULL;
}

static char *traced_botname(ClientData cdata, Tcl_Interp * irp, char *name1,
			    char *name2, int flags)
{
  char s[1024];

  simple_sprintf(s, "%s!%s", botname, botuserhost);
  Tcl_SetVar2(interp, name1, name2, s, TCL_GLOBAL_ONLY);
  if (flags & TCL_TRACE_UNSETS)
    Tcl_TraceVar(irp, name1, TCL_TRACE_READS | TCL_TRACE_WRITES |
		 TCL_TRACE_UNSETS, traced_botname, cdata);
  return NULL;
}

static char *traced_nettype(ClientData cdata, Tcl_Interp * irp, char *name1,
			    char *name2, int flags)
{
  switch (net_type) {
  case 0:
    use_silence = 0;
    check_mode_r = 0;
    break;
  case 1:
    use_silence = 0;
    check_mode_r = 1;
    break;
  case 2:
    use_silence = 1;
    check_mode_r = 0;
    break;
  case 3:
    use_silence = 0;
    check_mode_r = 0;
    break;
  case 4:
    use_silence = 0;
    check_mode_r = 0;
    break;
  default:
    break;
  }
  return NULL;
}

static tcl_strings my_tcl_strings[] =
{
  {"botnick", 0, 0, STR_PROTECT},
  {"altnick", altnick, NICKMAX, 0},
  {"realname", botrealname, 80, 0},
  {"init-server", initserver, 120, 0},
  {"connect-server", connectserver, 120, 0},
  {0, 0, 0, 0}
};

static tcl_coups my_tcl_coups[] =
{
  {"flood-msg", &flud_thr, &flud_time},
  {"flood-ctcp", &flud_ctcp_thr, &flud_ctcp_time},
  {0, 0, 0}
};

static tcl_ints my_tcl_ints[] =
{
  {"use-silence", 0, 0},
  {"use-console-r", 0, 1},
  {"servlimit", &min_servs, 0},
  {"server-timeout", &server_timeout, 0},
  {"lowercase-ctcp", &lowercase_ctcp, 0},
  {"server-online", (int *) &server_online, 2},
  {"strict-host", &strict_host, 0},
  {"never-give-up", &never_give_up, 0},
  {"keep-nick", &keepnick, 0},
  {"strict-servernames", &strict_servernames, 0},
  {"check-stoned", &check_stoned, 0},
  {"serverror-quit", &serverror_quit, 0},
  {"quiet-reject", &quiet_reject, 0},
  {"max-queue-msg", &maxqmsg, 0},
  {"trigger-on-ignore", &trigger_on_ignore, 0},
  {"answer-ctcp", &answer_ctcp, 0},
  {"server-cycle-wait", (int *) &server_cycle_wait, 0},
  {"default-port", &default_port, 0},
  {"check-mode-r", &check_mode_r, 0},
  {"use-ison", &use_ison, 0},
  {"net-type", &net_type, 0},
  {"must-be-owner", &must_be_owner, 0},		/* arthur2 */
  {"ctcp-mode", &ctcp_mode, 0},
  {"double-mode", &double_mode, 0}, /* G`Quann */
  {"double-server", &double_server, 0},
  {"double-help", &double_help, 0},
  {0, 0, 0}
};

/**********************************************************************/

/* oddballs */

/* read/write the server list */
static char *tcl_eggserver(ClientData cdata, Tcl_Interp * irp, char *name1,
			   char *name2, int flags)
{
  Tcl_DString ds;
  char *slist, **list, x[1024];
  struct server_list *q;
  int lc, code, i;

  Context;
  if (flags & (TCL_TRACE_READS | TCL_TRACE_UNSETS)) {
    /* create server list */
    Tcl_DStringInit(&ds);
    for (q = serverlist; q; q = q->next) {
      sprintf(x, "%s:%d%s%s %s", q->name, q->port ? q->port : default_port,
	      q->pass ? ":" : "", q->pass ? q->pass : "",
	      q->realname ? q->realname : "");
      Tcl_DStringAppendElement(&ds, x);
    }
    slist = Tcl_DStringValue(&ds);
    Tcl_SetVar2(interp, name1, name2, slist, TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&ds);
  } else {			/* writes */
    if (serverlist) {
      clearq(serverlist);
      serverlist = NULL;
    }
    slist = Tcl_GetVar2(interp, name1, name2, TCL_GLOBAL_ONLY);
    if (slist != NULL) {
      code = Tcl_SplitList(interp, slist, &lc, &list);
      if (code == TCL_ERROR) {
	return interp->result;
      }
      for (i = 0; i < lc && i < 50; i++) {
	add_server(list[i]);
      }
      /* tricky way to make the bot reset its server pointers
       * perform part of a '.jump <current-server>': */
      if (server_online) {
	int servidx = findanyidx(serv);

	curserv = (-1);
	next_server(&curserv, dcc[servidx].host, &dcc[servidx].port, "");
      }
      Tcl_Free((char *) list);
    }
  }
  Context;
  return NULL;
}

/* trace the servers */
#define tcl_traceserver(name,ptr) \
  Tcl_TraceVar(interp,name,TCL_TRACE_READS|TCL_TRACE_WRITES|TCL_TRACE_UNSETS, \
	       tcl_eggserver,(ClientData)ptr)

#define tcl_untraceserver(name,ptr) \
  Tcl_UntraceVar(interp,name,TCL_TRACE_READS|TCL_TRACE_WRITES|TCL_TRACE_UNSETS, \
	       tcl_eggserver,(ClientData)ptr)


/* this only handles CHAT requests, otherwise it's handled in filesys */
static int ctcp_DCC_CHAT(char *nick, char *from, char *handle,
			 char *object, char *keyword, char *text)
{
  char *action, *param, *ip, *prt, buf[512], *msg = buf;
  int i, sock;
  struct userrec *u = get_user_by_handle(userlist, handle);
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

  strcpy(msg, text);
  action = newsplit(&msg);
  param = newsplit(&msg);
  ip = newsplit(&msg);
  prt = newsplit(&msg);
  Context;
  if (strcasecmp(action, "CHAT") || !u)
    return 0;
  get_user_flagrec(u, &fr, 0);
  if (dcc_total == max_dcc) {
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s\n", nick, DCC_TOOMANYDCCS1);
    putlog(LOG_MISC, "*", DCC_TOOMANYDCCS2, "CHAT", param, nick, from);
  } else if (!(glob_party(fr) || (!require_p && chan_op(fr)))) {
    if (glob_xfer(fr))
      return 0;			/* allow filesys to pick up the chat */
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s.\n", nick, DCC_REFUSED2);
    putlog(LOG_MISC, "*", "%s: %s!%s", DCC_REFUSED, nick, from);
  } else if (u_pass_match(u, "-")) {
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s.\n", nick, DCC_REFUSED3);
    putlog(LOG_MISC, "*", "%s: %s!%s", DCC_REFUSED4, nick, from);
  } else if ((atoi(prt) < min_dcc_port) || (atoi(prt) > max_dcc_port)) {
    /* invalid port range, do clients even use over 5000?? */
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s (invalid port)\n", nick,
	      DCC_CONNECTFAILED1);
    putlog(LOG_MISC, "*", "%s: CHAT (%s!%s)", DCC_CONNECTFAILED3,
	   nick, from);
  } else {
    if (!sanitycheck_dcc(nick, from, ip, prt))
      return 1;
    sock = getsock(0);
    if (open_telnet_dcc(sock, ip, prt) < 0) {
      neterror(buf);
      if(!quiet_reject)
	dprintf(DP_HELP, "NOTICE %s :%s (%s)\n", nick,
		DCC_CONNECTFAILED1, buf);
      putlog(LOG_MISC, "*", "%s: CHAT (%s!%s)", DCC_CONNECTFAILED2,
	     nick, from);
      putlog(LOG_MISC, "*", "    (%s)", buf);
      killsock(sock);
    } else {
      i = new_dcc(&DCC_CHAT_PASS, sizeof(struct chat_info));

      dcc[i].addr = my_atoul(ip);
      dcc[i].port = atoi(prt);
      dcc[i].sock = sock;
      strcpy(dcc[i].nick, u->handle);
      strcpy(dcc[i].host, from);
      dcc[i].status = STAT_ECHO;
      if (glob_party(fr))
	dcc[i].status |= STAT_PARTY;
      strcpy(dcc[i].u.chat->con_chan, chanset ? chanset->name : "*");
      dcc[i].timeval = now;
      dcc[i].user = u;
      /* ok, we're satisfied with them now: attempt the connect */
      putlog(LOG_MISC, "*", "DCC connection: CHAT (%s!%s)", nick, from);
      dprintf(i, "%s\n", DCC_ENTERPASS);
    }
  }
  return 1;
}

static void server_secondly()
{
  if (cycle_time)
    cycle_time--;
  deq_msg();
  if (serv < 0)
    connect_server();
}

static void server_5minutely()
{
  if (check_stoned) {
    if (waiting_for_awake) {
      /* uh oh!  never got pong from last time, five minutes ago! */
      /* server is probably stoned */
      int servidx = findanyidx(serv);
      lostdcc(servidx);
      putlog(LOG_SERV, "*", IRC_SERVERSTONED);
    } else if (!trying_server) {
      /* check for server being stoned */
      dprintf(DP_MODE, "PING :%lu\n", (unsigned long) now);
      waiting_for_awake = 1;
    }
  }
}

static void server_prerehash()
{
  strcpy(oldnick, botname);
}

static void server_postrehash()
{
  strncpy(botname, origbotname, NICKMAX);
  botname[NICKMAX] = 0;
  if (!botname[0])
    fatal("NO BOT NAME.", 0);
  if (serverlist == NULL)
    fatal("NO SERVER.", 0);
    if (oldnick[0] && !rfc_casecmp(oldnick, botname)
       && !rfc_casecmp(oldnick, get_altbotnick())) {
    /* change botname back, don't be premature */
    strcpy(botname, oldnick);
    dprintf(DP_MODE, "NICK %s\n", origbotname);
  }
  /* change botname back incase we were using altnick previous to rehash */
  else if (oldnick[0])
    strcpy(botname, oldnick);
  if (initserver[0])
    do_tcl("init-server", initserver);
}

/* a report on the module status */
static void server_report(int idx, int details)
{
  char s1[128], s[128];

  dprintf(idx, "    Online as: %s!%s (%s)\n", botname, botuserhost,
	  botrealname);
  if (!trying_server) {
    daysdur(now, server_online, s1);
    sprintf(s, "(connected %s)", s1);
    if ((server_lag) && !(waiting_for_awake)) {
      sprintf(s1, " (lag: %ds)", server_lag);
      if (server_lag == (-1))
	sprintf(s1, " (bad pong replies)");
      strcat(s, s1);
    }
  }
  if (server_online) {
    int servidx = findanyidx(serv);

    dprintf(idx, "    Server %s:%d %s\n", dcc[servidx].host, dcc[servidx].port,
	    trying_server ? "(trying)" : s);
  } else
    dprintf(idx, "    %s\n", IRC_NOSERVER);
  if (modeq.tot)
    dprintf(idx, "    %s %d%%, %d msgs\n", IRC_MODEQUEUE,
            (int) ((float) (modeq.tot * 100.0) / (float) maxqmsg), (int) modeq.tot);
  if (mq.tot)
    dprintf(idx, "    %s %d%%, %d msgs\n", IRC_SERVERQUEUE,
           (int) ((float) (mq.tot * 100.0) / (float) maxqmsg), (int) mq.tot);
  if (hq.tot)
    dprintf(idx, "    %s %d%%, %d msgs\n", IRC_HELPQUEUE,
           (int) ((float) (hq.tot * 100.0) / (float) maxqmsg), (int) hq.tot);
  if (details) {
    if (min_servs)
      dprintf(idx, "    Requiring a net of at least %d server(s)\n", min_servs);
    if (initserver[0])
      dprintf(idx, "    On connect, I do: %s\n", initserver);
    if (connectserver[0])
      dprintf(idx, "    Before connect, I do: %s\n", connectserver);
    dprintf(idx, "    Flood is: %d msg/%ds, %d ctcp/%ds\n",
	    flud_thr, flud_time, flud_ctcp_thr, flud_ctcp_time);
  }
}

static int server_expmem()
{
  int tot = 0;
  struct msgq *m = mq.head;
  struct server_list *s = serverlist;

  Context;
  for (; s; s = s->next) {
    if (s->name)
      tot += strlen(s->name) + 1;
    if (s->pass)
      tot += strlen(s->pass) + 1;
    if (s->realname)
      tot += strlen(s->realname) + 1;
    tot += sizeof(struct server_list);
  }
  while (m != NULL) {
    tot += m->len + 1;
    tot += sizeof(struct msgq);

    m = m->next;
  }
  m = hq.head;
  while (m != NULL) {
    tot += m->len + 1;
    tot += sizeof(struct msgq);

    m = m->next;
  }
  m = modeq.head;
  while (m != NULL) {
    tot += m->len + 1;
    tot += sizeof(struct msgq);

    m = m->next;
  }
  return tot;
}

/* puts full hostname in s */
static void getmyhostname(char *s)
{
  struct hostent *hp;
  char *p;

  if (hostname[0]) {
    strcpy(s, hostname);
    return;
  }
  p = getenv("HOSTNAME");
  if (p != NULL) {
    strncpy(s, p, 80);
    s[80] = 0;
    if (strchr(s, '.') != NULL)
      return;
  }
  gethostname(s, 80);
  if (strchr(s, '.') != NULL)
    return;
  hp = gethostbyname(s);
  if (hp == NULL)
    fatal("Hostname self-lookup failed.", 0);
  strcpy(s, hp->h_name);
  if (strchr(s, '.') != NULL)
    return;
  if (hp->h_aliases[0] == NULL)
    fatal("Can't determine your hostname!", 0);
  strcpy(s, hp->h_aliases[0]);
  if (strchr(s, '.') == NULL)
    fatal("Can't determine your hostname!", 0);
}

/* update the add/rem_builtins in server.c if you add to this list!! */
static cmd_t my_ctcps[] =
{
  {"DCC", "", ctcp_DCC_CHAT, "server:DCC"},
  {0, 0, 0, 0}
};

static char *server_close()
{
  cmd_t C_t[] =
  {
    {"die", "m", (Function) cmd_die, NULL},
    {0, 0, 0, 0}
  };

  Context;
  cycle_time = 100;
  nuke_server("Connection reset by phear");
  clearq(serverlist);
  Context;
  rem_builtins(H_dcc, C_dcc_serv);
  rem_builtins(H_raw, my_raw_binds);
  rem_builtins(H_ctcp, my_ctcps);
  Context;
  add_builtins(H_dcc, C_t);
  Context;
  del_bind_table(H_wall);
  del_bind_table(H_raw);
  del_bind_table(H_notc);
  del_bind_table(H_msgm);
  del_bind_table(H_msg);
  del_bind_table(H_flud);
  del_bind_table(H_ctcr);
  del_bind_table(H_ctcp);
  Context;
  rem_tcl_coups(my_tcl_coups);
  rem_tcl_strings(my_tcl_strings);
  rem_tcl_ints(my_tcl_ints);
  rem_help_reference("server.help");
  rem_tcl_commands(my_tcl_cmds);
  Context;
  Tcl_UntraceVar(interp, "nick",
		 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		 nick_change, NULL);
  Tcl_UntraceVar(interp, "altnick",
		 TCL_TRACE_WRITES | TCL_TRACE_UNSETS, altnick_change, NULL);
  Tcl_UntraceVar(interp, "botname",
		 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		 traced_botname, NULL);
  Tcl_UntraceVar(interp, "server",
		 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		 traced_server, NULL);
  Tcl_UntraceVar(interp, "net-type",
		 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		 traced_nettype, NULL);
  tcl_untraceserver("servers", NULL);
  Context;
  empty_msgq();
  Context;
  del_hook(HOOK_SECONDLY, server_secondly);
  del_hook(HOOK_5MINUTELY, server_5minutely);
  del_hook(HOOK_QSERV, queue_server);
  del_hook(HOOK_MINUTELY, minutely_checks);
  del_hook(HOOK_PRE_REHASH, server_prerehash);
  del_hook(HOOK_REHASH, server_postrehash);
  Context;
  module_undepend(MODULE_NAME);
  return NULL;
}

char *server_start();

static Function server_table[] =
{
  (Function) server_start,
  (Function) server_close,
  (Function) server_expmem,
  (Function) server_report,
  /* 4 - 7 */
  (Function) 0,			/* char * (points to botname later on) */
  (Function) botuserhost,	/* char * */
  (Function) & quiet_reject,	/* int */
  (Function) & serv,
  /* 8 - 11 */
  (Function) & flud_thr,	/* int */
  (Function) & flud_time,	/* int */
  (Function) & flud_ctcp_thr,	/* int */
  (Function) & flud_ctcp_time,	/* int */
  /* 12 - 15 */
  (Function) match_my_nick,
  (Function) check_tcl_flud,
  (Function) fixfrom,
  (Function) & answer_ctcp,	/* int */
  /* 16 - 19 */
  (Function) & trigger_on_ignore,	/* int */
  (Function) check_tcl_ctcpr,
  (Function) detect_avalanche,
  (Function) nuke_server,
  /* 20 - 23 */
  (Function) newserver,		/* char * */
  (Function) & newserverport,	/* int */
  (Function) newserverpass,	/* char * */
  /* 24 - 27 */
  (Function) & cycle_time,	/* int */
  (Function) & default_port,	/* int */
  (Function) & server_online,	/* int */
  (Function) & min_servs,
  /* 28 - 31 */
  (Function) & H_raw,
  (Function) & H_wall,
  (Function) & H_msg,
  (Function) & H_msgm,
  /* 32 - 35 */
  (Function) & H_notc,
  (Function) & H_flud,
  (Function) & H_ctcp,
  (Function) & H_ctcr,
  /* 36 - 39 */
  (Function) ctcp_reply,
  (Function) get_altbotnick,	/* char * */
};

char *server_start(Function * global_funcs)
{
  char *s;

  global = global_funcs;

  /* init all the variables *must* be done in _start rather than globally */
  serv = -1;
  strict_host = 1;
  botname[0] = 0;
  trying_server = 0L;
  server_lag = 0;
  altnick[0] = 0;
  raltnick[0] = 0;
  curserv = 0;
  flud_thr = 5;
  flud_time = 60;
  flud_ctcp_thr = 3;
  flud_ctcp_time = 60;
  initserver[0] = 0;
  connectserver[0] = 0;		/* drummer */
  botuserhost[0] = 0;
  keepnick = 1;
  check_stoned = 1;
  serverror_quit = 1;
  quiet_reject = 1;
  waiting_for_awake = 0;
  server_online = 0;
  server_cycle_wait = 60;
  strcpy(botrealname, "A deranged product of evil coders");
  min_servs = 0;
  server_timeout = 60;
  never_give_up = 0;
  strict_servernames = 0;
  serverlist = NULL;
  cycle_time = 0;
  default_port = 6667;
  oldnick[0] = 0;
  trigger_on_ignore = 0;
  answer_ctcp = 1;
  lowercase_ctcp = 0;
  bothost[0] = 0;
  check_mode_r = 0;
  maxqmsg = 300;
  burst = 0;
  use_ison = 1;
  net_type = 0;
  double_mode = 0;
  double_server = 0;
  double_help = 0;
  Context;
  server_table[4] = (Function) botname;
  module_register(MODULE_NAME, server_table, 1, 0);
  if (!module_depend(MODULE_NAME, "eggdrop", 104, 0))
    return "This module requires eggdrop1.4.0 or later";
  /* weird ones */
  Context;
  /* fool bot in reading the values */
  tcl_eggserver(NULL, interp, "servers", NULL, 0);
  tcl_traceserver("servers", NULL);
  s = Tcl_GetVar(interp, "nick", TCL_GLOBAL_ONLY);
  if (s) {
    strncpy(origbotname, s, NICKMAX);
    origbotname[NICKMAX] = 0;
  }
  Tcl_TraceVar(interp, "nick",
	       TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
	       nick_change, NULL);
  Tcl_TraceVar(interp, "altnick",
	       TCL_TRACE_WRITES | TCL_TRACE_UNSETS, altnick_change, NULL);
  Tcl_TraceVar(interp, "botname",
	       TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
	       traced_botname, NULL);
  Tcl_TraceVar(interp, "server",
	       TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
	       traced_server, NULL);
  Tcl_TraceVar(interp, "net-type",
	       TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
	       traced_nettype, NULL);
  Context;
  H_wall = add_bind_table("wall", HT_STACKABLE, server_2char);
  H_raw = add_bind_table("raw", HT_STACKABLE, server_raw);
  H_notc = add_bind_table("notc", HT_STACKABLE, server_5char);
  H_msgm = add_bind_table("msgm", HT_STACKABLE, server_5char);
  H_msg = add_bind_table("msg", 0, server_msg);
  H_flud = add_bind_table("flud", HT_STACKABLE, server_5char);
  H_ctcr = add_bind_table("ctcr", HT_STACKABLE, server_6char);
  H_ctcp = add_bind_table("ctcp", HT_STACKABLE, server_6char);
  Context;
  add_builtins(H_raw, my_raw_binds);
  add_builtins(H_dcc, C_dcc_serv);
  add_builtins(H_ctcp, my_ctcps);
  add_help_reference("server.help");
  Context;
  my_tcl_strings[0].buf = botname;
  add_tcl_strings(my_tcl_strings);
  my_tcl_ints[0].val = &use_silence;
  my_tcl_ints[1].val = &use_console_r;
  add_tcl_ints(my_tcl_ints);
  add_tcl_commands(my_tcl_cmds);
  add_tcl_coups(my_tcl_coups);
  Context;
  add_hook(HOOK_SECONDLY, server_secondly);
  add_hook(HOOK_5MINUTELY, server_5minutely);
  add_hook(HOOK_MINUTELY, minutely_checks);
  add_hook(HOOK_QSERV, queue_server);
  add_hook(HOOK_PRE_REHASH, server_prerehash);
  add_hook(HOOK_REHASH, server_postrehash);
  Context;
  mq.head = hq.head = modeq.head = 0;
  mq.last = hq.last = modeq.last = 0;
  mq.tot = hq.tot = modeq.tot = 0;
  mq.warned = hq.warned = modeq.warned = 0;
  double_warned = 0;
  Context;
  newserver[0] = 0;
  newserverport = 0;
  getmyhostname(bothost);
  Context;
  sprintf(botuserhost, "%s@%s", botuser, bothost);	/* wishful thinking */
  curserv = 999;
  if (net_type == 0) {		/* EfNet except new +e/+I hybrid */
    use_silence = 0;
    check_mode_r = 0;
  }
  if (net_type == 1) {		/* Ircnet */
    use_silence = 0;
    check_mode_r = 1;
  }
  if (net_type == 2) {		/* Undernet */
    use_silence = 1;
    check_mode_r = 0;
  }
  if (net_type == 3) {		/* Dalnet */
    use_silence = 0;
    check_mode_r = 0;
  }
  if (net_type == 4) {		/* new +e/+I Efnet hybrid */
    use_silence = 0;
    check_mode_r = 0;
  }
  putlog(LOG_MISC, "*", "=== SERVER SUPPORT LOADED");
  return NULL;
}
