/* 
 * cmdsserv.c -- handles:
 * commands from a user via dcc that cause server interaction.
 */
/* 
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

static void cmd_servers(struct userrec *u, int idx, char *par)
{
  struct server_list *x = serverlist;
  int i;
  char s[1024];

  putlog(LOG_CMDS, "*", "#%s# servers", dcc[idx].nick);

  if (!x) {
    dprintf(idx, "No servers.\n");
  } else {
    dprintf(idx, "My server list:\n");
    i = 0;
    while (x != NULL) {
      sprintf(s, "%14s %20.20s:%-10d", (i == curserv) ? "I am here ->" : "",
	      x->name, x->port ? x->port : default_port);
      if (x->realname)
	sprintf(s + 46, " (%-.20s)", x->realname);
      dprintf(idx, "%s\n", s);
      x = x->next;
      i++;
    }
  }
}

static void cmd_dump(struct userrec *u, int idx, char *par)
{
  if (!(isowner(dcc[idx].nick)) && (must_be_owner == 2)) {
    dprintf(idx, MISC_NOSUCHCMD);
    return;
  }
  if (!par[0]) {
    dprintf(idx, "Usage: dump <server stuff>\n");
    return;
  }
  putlog(LOG_CMDS, "*", "#%s# dump %s", dcc[idx].nick, par);
  dprintf(DP_SERVER, "%s\n", par);
}

static void cmd_jump(struct userrec *u, int idx, char *par)
{
  char *other;
  int port;

  if (par[0]) {
    other = newsplit(&par);
    port = atoi(newsplit(&par));
    if (!port)
      port = default_port;
    putlog(LOG_CMDS, "*", "#%s# jump %s %d %s", dcc[idx].nick, other,
	   port, par);
    strncpy(newserver, other, 120);
    newserver[120] = 0;
    newserverport = port;
    strncpy(newserverpass, par, 120);
    newserverpass[120] = 0;
  } else
    putlog(LOG_CMDS, "*", "#%s# jump", dcc[idx].nick);
  dprintf(idx, "%s...\n", IRC_JUMP);
  cycle_time = 0;
  nuke_server("changing servers");
}

 static void cmd_clearqueue (struct userrec * u, int idx, char * par)
 {
 struct msgq *q, *qq;
 int msgs;
 msgs = 0;
 if (!par[0]) {
     dprintf(idx, "Usage: clearqueue <mode|server|help|all>\n");
     putlog(LOG_CMDS, "*", "#%s# clearqueue %s", dcc[idx].nick, par);
     return;
     }
 if (!strcasecmp(par, "all")) {
     msgs = (int) (modeq.tot + mq.tot + hq.tot);
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
     double_warned = 0;
     burst = 0;  
     dprintf(idx, "Removed %d msgs from all queues\n",msgs);
     putlog(LOG_CMDS, "*", "#%s# clearqueue %s", dcc[idx].nick, par);
     return;
     }
 if (!strcasecmp(par, "mode")) {
     q = modeq.head;
     msgs = modeq.tot;
     while (q) {
          qq = q->next;
         nfree(q->msg);
         nfree(q);  
         q = qq;
         }
     if (mq.tot == 0) {
         burst = 0;
         }
     double_warned = 0;
     modeq.tot = modeq.warned = 0;
     modeq.head = modeq.last = 0;
     dprintf(idx, "Removed %d msgs from the mode queue\n",msgs);
     putlog(LOG_CMDS, "*", "#%s# clearqueue %s", dcc[idx].nick, par);
     return;
     }
 if (!strcasecmp(par, "help")) {
     msgs = hq.tot;   
     q = hq.head;
     while (q) {
         qq = q->next; 
         nfree(q->msg);
         nfree(q);
         q = qq;
         }
     double_warned = 0;
     hq.tot = hq.warned = 0;
     hq.head = hq.last = 0;
     dprintf(idx, "Removed %d msgs from the help queue\n",msgs);
     putlog(LOG_CMDS, "*", "#%s# clearqueue %s", dcc[idx].nick, par);
     return;
     }
 if (!strcasecmp(par, "server")) {
     msgs = mq.tot;
     q = mq.head;
     while (q) {      
         qq = q->next;
         nfree(q->msg);
         nfree(q);
         q = qq;
         mq.tot = mq.warned = 0;
         mq.head = mq.last = 0;
         if (modeq.tot == 0) {
             burst = 0;
             }
         }
     double_warned = 0;
     mq.tot = mq.warned = 0;
     mq.head = mq.last = 0;
     dprintf(idx, "Removed %d msgs from the server queue\n",msgs);
     putlog(LOG_CMDS, "*", "#%s# clearqueue %s", dcc[idx].nick, par);
     return;
     }
 dprintf(idx, "Usage: clearqueue <mode|server|help|all>\n");
 putlog(LOG_CMDS, "*", "#%s# clearqueue %s", dcc[idx].nick, par);
 }
         

/* this overrides the default die, handling extra server stuff
 * send a QUIT if on the server */
static void my_cmd_die(struct userrec *u, int idx, char *par)
{
  cycle_time = 100;
  if (server_online) {
    dprintf(-serv, "QUIT :%s\n", par[0] ? par : dcc[idx].nick);
    sleep(3);			/* give the server time to understand */
  }
  nuke_server(NULL);
  cmd_die(u, idx, par);
}

/* DCC CHAT COMMANDS */
/* function call should be: int cmd_whatever(idx,"parameters");
 * as with msg commands, function is responsible for any logging */
/* update the add/rem_builtins in server.c if you add to this list!! */
static cmd_t C_dcc_serv[] =
{
  {"die", "n", (Function) my_cmd_die, "server:die"},
  {"dump", "m", (Function) cmd_dump, NULL},
  {"jump", "m", (Function) cmd_jump, NULL},
  {"servers", "o", (Function) cmd_servers, NULL},
  {"clearqueue", "m", (Function)cmd_clearqueue, NULL },
  {0, 0, 0, 0}
};
