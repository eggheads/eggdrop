/* Original Copyright (c) 2000-2001 proton
 * 
 * $Id: uptime.c,v 1.9 2001/07/17 19:53:43 guppy Exp $
 * Borrowed from Emech, reports to http://uptime.energymech.net, feel free to opt out if you
 * dont like it by not loading the module.
 * 
 *				-poptix (poptix@poptix.net) 6June2001
 */
/*
 * Eggdrop modifications Copyright (C) 1999, 2000, 2001 Eggheads Development Team
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

#define MODULE_NAME "uptime"
#define MAKING_UPTIME
#include "uptime.h" 
#include "../module.h"
#include "../server.mod/server.h"
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct PackUp
{
        int     regnr;
        int     pid;
        int     type;
        unsigned long   cookie;
        unsigned long   uptime;
        unsigned long   ontime;
        unsigned long   now2;
        unsigned long   sysup;
        char    string[3];

} PackUp;

PackUp upPack;

static Function *global = NULL, *server_funcs = NULL;

char *uptime_host;
int uptimeport = 9969;
int hours=0;
int uptimesock;
int uptimecount;
unsigned long uptimeip;
unsigned long uptimecookie;
time_t uptimelast;
char uptime_version[50]="";

static int uptime_expmem() {
	int size = 256;
	return size;
}

static void uptime_report(int idx, int details)
{
  int size;

  Context;
  size = uptime_expmem();
  if (details)
    dprintf(idx, "   using %d bytes\n", size);
}
	

unsigned long get_ip()
{
  struct hostent *hp;
  IP ip;  
  struct in_addr *in;
    
  /* could be pre-defined */
  if (uptime_host[0]) {
    if ((uptime_host[strlen(uptime_host) - 1] >= '0') && (uptime_host[strlen(uptime_host) - 1] <= '9'))
      return (IP) inet_addr(uptime_host);    
  }  
  hp = gethostbyname(uptime_host);
  if (hp == NULL)
    return -1;
  in = (struct in_addr *) (hp->h_addr_list[0]);
  ip = (IP) (in->s_addr);
  return ip;
}       

int init_uptime(void)
{
	struct  sockaddr_in sai;
	char temp[50]="";
	upPack.regnr = 0;
	upPack.pid = htonl(getpid());
	upPack.type = htonl(UPTIME_EGGDROP);
	upPack.cookie = 0;
	upPack.uptime = htonl(online_since);
	uptimecookie = rand();
	uptimecount = 0;
	uptimelast = 0;
	uptimeip = -1;


	strcpy(temp,ver);
	splitc(uptime_version,temp,' ');
	strcpy(uptime_version,temp);

	if ((uptimesock = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		putlog(LOG_DEBUG, "*", "init_uptime socket returned <0 %d",uptimesock);
		return((uptimesock = -1));
	}
	memset(&sai,0,sizeof(sai));
	sai.sin_addr.s_addr = INADDR_ANY;
	sai.sin_family = AF_INET;
	if (bind(uptimesock,(struct sockaddr*)&sai,sizeof(sai)) < 0) {
		close(uptimesock);
		return((uptimesock = -1));
	}
	fcntl(uptimesock,F_SETFL,O_NONBLOCK | fcntl(uptimesock,F_GETFL));
	return(0);
}


int send_uptime(void)
{
	struct  sockaddr_in sai;
	struct  stat st;
	PackUp  *mem;
	int     len;
	int servidx = findanyidx(serv);

	uptimecookie = (uptimecookie + 1) * 18457;
	upPack.cookie = htonl(uptimecookie);
	upPack.now2 = htonl(time(NULL));
	if (stat("/proc",&st) < 0)
		upPack.sysup = 0;
	else
		upPack.sysup = htonl(st.st_ctime);
	upPack.uptime = htonl(online_since);
	upPack.ontime = htonl(server_online);
	uptimecount++;
	if (((uptimecount & 0x7) == 0) || (uptimeip == -1)) {
		uptimeip = get_ip();
		if (uptimeip == -1)
			return -2;
	}
	len = sizeof(upPack) + strlen(botnetnick) + strlen(dcc[servidx].host) + strlen(uptime_version);
	putlog(LOG_DEBUG, "*", "len = %d",len);
	mem = (PackUp*)nmalloc(len);
	memcpy(mem,&upPack,sizeof(upPack));
	sprintf(mem->string,"%s %s %s",botnetnick,dcc[servidx].host,uptime_version);
	memset(&sai,0,sizeof(sai));
	sai.sin_family = AF_INET;
	sai.sin_addr.s_addr = uptimeip;
	sai.sin_port = htons(uptimeport);
	len = sendto(uptimesock,(void*)mem,len,0,(struct sockaddr*)&sai,sizeof(sai));
	putlog(LOG_DEBUG, "*", "len = %d",len);
	nfree(mem);
	return len;
}

void check_hourly() {
	hours++;
	if (hours==6) {
		send_uptime();
		hours=0;
	}
}

static int uptime_set_send(struct userrec *u, int idx, char *par)
{
	int servidx = findanyidx(serv);
	Context;
	dprintf(idx,"Nick %s Ontime %lu Server %s Version %s Result %d\n", botnetnick, online_since, dcc[servidx].host,
	        uptime_version, send_uptime()); 
	return 1;
}

static cmd_t mydcc[] =
    {
        {"usetsend", "", uptime_set_send, NULL},
        {0, 0, 0, 0}
    };

static tcl_strings mystrings[] =
    {
        {0, 0, 0, 0}
    };

static tcl_ints myints[] =
    {
        {0, 0}
    };

static char *uptime_close()
{
	rem_tcl_strings(mystrings);
	rem_tcl_ints(myints);
	rem_builtins(H_dcc, mydcc);
	nfree(uptime_host);
	close(uptimesock);
	del_hook(HOOK_HOURLY, (Function) check_hourly);
	module_undepend(MODULE_NAME);
	return NULL;
}

EXPORT_SCOPE char *uptime_start();

static Function uptime_table[] =
    {
        (Function) uptime_start,
        (Function) uptime_close,
        (Function) uptime_expmem,
        (Function) uptime_report,
    };

char *uptime_start(Function * global_funcs)
{
	global = global_funcs;

	Context;
	if (!(server_funcs = module_depend(MODULE_NAME, "server", 1, 0)))
		return "You need the server module to use the uptime module.";
	module_register(MODULE_NAME, uptime_table, 1, 1);
	add_tcl_strings(mystrings);
	add_tcl_ints(myints);
	add_builtins(H_dcc, mydcc);
	add_hook(HOOK_HOURLY, (Function) check_hourly);
	uptime_host=nmalloc(256);
	strcpy(uptime_host, UPTIME_HOST);
	init_uptime();
	return NULL;
}
