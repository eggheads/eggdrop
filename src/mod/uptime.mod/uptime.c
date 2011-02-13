/*
 * $Id: uptime.c,v 1.41 2011/02/13 14:19:34 simple Exp $
 *
 * This module reports uptime information about your bot to http://uptime.eggheads.org. The
 * purpose for this is to see how your bot rates against many others (including EnergyMechs
 * and Eggdrops) -- It is a fun little project, jointly run by Eggheads.org and EnergyMech.net.
 *
 * If you don't like being a part of it please just unload this module.
 *
 * Also for bot developers feel free to modify this code to make it a part of your bot and
 * e-mail webmaster@eggheads.org for more information on registering your bot type. See how
 * your bot's stability rates against ours and ours against yours <g>.
 */
/*
 * Copyright (C) 2001 proton
 * Copyright (C) 2001 - 2011 Eggheads Development Team
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

/*
 * regnr is unused; however, it must be here inorder for
 * us to create a proper struct for the uptime server.
 *
 * "packets_sent" was originally defined as "cookie",
 * however this field was deprecated and set to zero
 * for most versions of the uptime client.  It has been
 * repurposed and renamed as of uptime v1.3 to reflect
 * the number of packets the client thinks it has sent
 * over the life of the module.  Only the name has changed -
 * the type (unsigned long) is still the same.
 */

typedef struct PackUp {
  int regnr;
  int pid;
  int type;
  unsigned long packets_sent;
  unsigned long uptime;
  unsigned long ontime;
  unsigned long now2;
  unsigned long sysup;
  char string[3];
} PackUp;

PackUp upPack;

static Function *global = NULL;

static int minutes = 0;
static int seconds = 0;
static int next_seconds = 0;
static int next_minutes = 0;
static int update_interval = 720; /* rand(0..12) hours: ~6 hour average. */
static time_t next_update = 0;
static int uptimesock;
static int uptimecount;
static unsigned long uptimeip;
static char uptime_version[48] = "";

void check_secondly(void);
void check_minutely(void);

static int uptime_expmem()
{
  return 0;
}

static void uptime_report(int idx, int details)
{
  int delta_seconds;
  char *next_update_at;

  if (details) {
    delta_seconds = (int) (next_update - time(NULL));
    next_update_at = ctime(&next_update);
    next_update_at[strlen(next_update_at) - 1] = 0;

    dprintf(idx, "      %d uptime packet%s sent\n", uptimecount,
            (uptimecount != 1) ? "s" : "");
    dprintf(idx, "      Approximately %-.2f hours until next update "
            "(at %s)\n", delta_seconds / 3600.0, next_update_at);
  }
}

unsigned long get_ip()
{
  struct hostent *hp;
  IP ip;
  struct in_addr *in;

  /* could be pre-defined */
  if (uptime_host[0]) {
    if ((uptime_host[strlen(uptime_host) - 1] >= '0') &&
        (uptime_host[strlen(uptime_host) - 1] <= '9'))
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
  struct sockaddr_in sai;
  char x[64], *z = x;

  upPack.regnr = 0;  /* unused */
  upPack.pid = 0;    /* must set this later */
  upPack.type = htonl(uptime_type);
  upPack.packets_sent = 0; /* reused (abused?) to send our packet count */
  upPack.uptime = 0; /* must set this later */
  uptimecount = 0;
  uptimeip = -1;

  strncpyz(x, ver, sizeof x);
  newsplit(&z);
  strncpyz(uptime_version, z, sizeof uptime_version);

  if ((uptimesock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    putlog(LOG_DEBUG, "*", "init_uptime socket returned < 0 %d", uptimesock);
    return ((uptimesock = -1));
  }
  egg_memset(&sai, 0, sizeof(sai));
  sai.sin_addr.s_addr = INADDR_ANY;
  sai.sin_family = AF_INET;
  if (bind(uptimesock, (struct sockaddr *) &sai, sizeof(sai)) < 0) {
    close(uptimesock);
    return ((uptimesock = -1));
  }
  fcntl(uptimesock, F_SETFL, O_NONBLOCK | fcntl(uptimesock, F_GETFL));

  next_minutes = rand() % update_interval; /* Initial update delay */
  next_seconds = rand() % 59;
  next_update = (time_t) ((time(NULL) / 60 * 60) + (next_minutes * 60) +
    next_seconds);

  return 0;
}


int send_uptime(void)
{
  struct sockaddr_in sai;
  struct stat st;
  PackUp *mem;
  int len, servidx;
  char servhost[UHOSTLEN] = "none";
  module_entry *me;

  if (uptimeip == -1) {
    uptimeip = get_ip();
    if (uptimeip == -1)
      return -2;
  }

  uptimecount++;
  upPack.packets_sent = htonl(uptimecount); /* Tell the server how many
					       uptime packets we've sent. */
  upPack.now2 = htonl(time(NULL));
  upPack.ontime = 0;

  if ((me = module_find("server", 1, 0))) {
    Function *server_funcs = me->funcs;

    if (server_online) {
      servidx = findanyidx(serv);
      strncpyz(servhost, dcc[servidx].host, sizeof servhost);
      upPack.ontime = htonl(server_online);
    }
  }

  if (!upPack.pid)
    upPack.pid = htonl(getpid());

  if (!upPack.uptime)
    upPack.uptime = htonl(online_since);

  if (stat("/proc", &st) < 0)
    upPack.sysup = 0;
  else
    upPack.sysup = htonl(st.st_ctime);

  len = sizeof(upPack) + strlen(botnetnick) + strlen(servhost) +
        strlen(uptime_version);
  mem = (PackUp *) nmalloc(len);
  egg_bzero(mem, len); /* mem *should* be completely filled before it's
                             * sent to the server.  But belt-and-suspenders
                             * is always good.
                             */
  my_memcpy(mem, &upPack, sizeof(upPack));
  sprintf(mem->string, "%s %s %s", botnetnick, servhost, uptime_version);
  egg_bzero(&sai, sizeof(sai));
  sai.sin_family = AF_INET;
  sai.sin_addr.s_addr = uptimeip;
  sai.sin_port = htons(uptime_port);
  len = sendto(uptimesock, (void *) mem, len, 0, (struct sockaddr *) &sai,
               sizeof(sai));
  nfree(mem);
  return len;
}

void check_minutely()
{
  minutes++;
  if (minutes >= next_minutes) {
    /* We're down to zero minutes.  Now do the seconds. */
    del_hook(HOOK_MINUTELY, (Function) check_minutely);
    add_hook(HOOK_SECONDLY, (Function) check_secondly);
  }
}

void check_secondly()
{
  seconds++;
  if (seconds >= next_seconds) {  /* DING! */
    del_hook(HOOK_SECONDLY, (Function) check_secondly);

    send_uptime();

    minutes = 0; /* Reset for the next countdown. */
    seconds = 0;
    next_minutes = rand() % update_interval;
    next_seconds = rand() % 59;
    next_update = (time_t) ((time(NULL) / 60 * 60) + (next_minutes * 60) +
      next_seconds);

    /* Go back to checking every minute. */
    add_hook(HOOK_MINUTELY, (Function) check_minutely);
  }
}

static char *uptime_close()
{
  return "You cannot unload the uptime module "
         "(doing so will reset your stats).";
}

EXPORT_SCOPE char *uptime_start(Function *);

static Function uptime_table[] = {
  (Function) uptime_start,
  (Function) uptime_close,
  (Function) uptime_expmem,
  (Function) uptime_report,
};

char *uptime_start(Function *global_funcs)
{
  if (global_funcs) {
    global = global_funcs;

    module_register(MODULE_NAME, uptime_table, 1, 3);
    if (!module_depend(MODULE_NAME, "eggdrop", 106, 11)) {
      module_undepend(MODULE_NAME);
      return "This module requires Eggdrop 1.6.11 or later.";
    }

    add_help_reference("uptime.help");
    add_hook(HOOK_MINUTELY, (Function) check_minutely);
    init_uptime();
  }
  return NULL;
}
