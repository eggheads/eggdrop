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
static cmd_t C_dcc_serv[4] =
{
  {"die", "n", (Function) my_cmd_die, "server:die"},
  {"dump", "m", (Function) cmd_dump, NULL},
  {"jump", "m", (Function) cmd_jump, NULL},
  {"servers", "o", (Function) cmd_servers, NULL},
};
