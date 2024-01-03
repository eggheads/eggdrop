/*
 * This module reports uptime information about your bot to
 * http://uptime.eggheads.org. The purpose for this is to see how your bot rates
 * against many others (including EnergyMechs and Eggdrops) -- It is a fun
 * little project, jointly run by Eggheads.org and EnergyMech.net.
 *
 * If you don't like being a part of it please just unload this module.
 *
 * Also for bot developers feel free to modify this code to make it a part of
 * your bot and e-mail webmaster@eggheads.org for more information on
 * registering your bot type. See how your bot's stability rates against ours
 * and ours against yours <g>.
 */
/*
 * Copyright (C) 2001 proton
 * Copyright (C) 2001 - 2023 Eggheads Development Team
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

#include <fcntl.h>
#include <sys/stat.h>
#include "uptime.h"
#include "../module.h"
#include "../server.mod/server.h"

#define UPDATE_INTERVAL (12 * 60) /* random(0..12) hours: ~6 hour average. */

/*
 * regnr is unused; however, it must be here inorder for us to create a proper
 * struct for the uptime server.
 *
 * "packets_sent" was originally defined as "cookie", however this field was
 * deprecated and set to zero for most versions of the uptime client. It has
 * been repurposed and renamed as of uptime v1.3 to reflect the number of
 * packets the client thinks it has sent over the life of the module. Only the
 * name has changed - the type is still the same.
 */
typedef struct PackUp {
  uint32_t regnr;
  uint32_t pid;
  uint32_t type;
  uint32_t packets_sent;
  uint32_t uptime;
  uint32_t ontime;
  uint32_t now2;
  uint32_t sysup;
  char string[FLEXIBLE_ARRAY_MEMBER];
} PackUp;

PackUp upPack;

static Function *global = NULL;

static int minutes = 0;
static int seconds = 0;
static int next_seconds = 0;
static int next_minutes = 0;
static time_t next_update = 0;
static int uptimesock;
static int uptimecount;
static unsigned long uptimeip;
static char uptime_version[48] = "";

static void check_secondly(void);
static void check_minutely(void);

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

static int init_uptime(void)
{
  struct sockaddr_in sai;
  char x[64], *z = x;

  upPack.regnr = 0;  /* unused */
  upPack.pid = 0;    /* must set this later */
  upPack.type = htonl(UPTIME_TYPE);
  upPack.packets_sent = 0; /* reused (abused?) to send our packet count */
  upPack.uptime = 0; /* must set this later */
  uptimecount = 0;
  uptimeip = -1;

  strlcpy(x, ver, sizeof x);
  newsplit(&z);
  strlcpy(uptime_version, z, sizeof uptime_version);

  if ((uptimesock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    putlog(LOG_DEBUG, "*", "init_uptime socket returned < 0 %d", uptimesock);
    return ((uptimesock = -1));
  }
  egg_bzero(&sai, sizeof(sai));
  sai.sin_addr.s_addr = INADDR_ANY;
  sai.sin_family = AF_INET;
  if (bind(uptimesock, (struct sockaddr *) &sai, sizeof(sai)) < 0) {
    close(uptimesock);
    return ((uptimesock = -1));
  }
  fcntl(uptimesock, F_SETFL, O_NONBLOCK | fcntl(uptimesock, F_GETFL));

  next_minutes = random() % UPDATE_INTERVAL; /* Initial update delay */
  next_seconds = random() % 59;
  next_update = (time_t) ((time(NULL) / 60 * 60) + (next_minutes * 60) +
    next_seconds);

  return 0;
}


static int send_uptime(void)
{
  struct addrinfo hints, *res0;
  int error;
  struct stat st;
  PackUp *mem;
  int len, servidx;
  char *servhost = "none";
  module_entry *me;

  egg_bzero(&hints, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  error = getaddrinfo(UPTIME_HOST, UPTIME_PORT, &hints, &res0);
  if (error) {
    if (error == EAI_NONAME)
      putlog(LOG_MISC, "*",
             "send_uptime(): getaddrinfo(): hostname:port '%s:%s' not known",
             UPTIME_HOST, UPTIME_PORT);
    else
      putlog(LOG_MISC, "*", "send_uptime(): getaddrinfo(): error = %s",
             gai_strerror(error));
    return -2;
  }

  uptimecount++;
  upPack.packets_sent = htonl(uptimecount); /* Tell the server how many uptime
                                               packets we've sent. */
  upPack.now2 = htonl(time(NULL));
  upPack.ontime = 0;

  if ((me = module_find("server", 1, 0))) {
    Function *server_funcs = me->funcs;

    if (server_online) {
      servidx = findanyidx(serv);
      servhost = dcc[servidx].host;
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

  len = offsetof(struct PackUp, string) + strlen(botnetnick) + strlen(servhost)
        + strlen(uptime_version) + 3; /* whitespace + whitespace + \0 */
  mem = (PackUp *) nmalloc(len);
  memcpy(mem, &upPack, sizeof(upPack));
  sprintf(mem->string, "%s %s %s", botnetnick, servhost, uptime_version);
  len = sendto(uptimesock, (void *) mem, len, 0, res0->ai_addr,
               res0->ai_addrlen);
  if (len < 0)
    putlog(LOG_DEBUG, "*", "send_uptime(): sendto(): %s", gai_strerror(error));
  nfree(mem);
  return len;
}

static void check_minutely()
{
  minutes++;
  if (minutes >= next_minutes) {
    /* We're down to zero minutes.  Now do the seconds. */
    del_hook(HOOK_MINUTELY, (Function) check_minutely);
    add_hook(HOOK_SECONDLY, (Function) check_secondly);
  }
}

static void check_secondly()
{
  seconds++;
  if (seconds >= next_seconds) {  /* DING! */
    del_hook(HOOK_SECONDLY, (Function) check_secondly);

    send_uptime();

    minutes = 0; /* Reset for the next countdown. */
    seconds = 0;
    next_minutes = random() % UPDATE_INTERVAL;
    next_seconds = random() % 59;
    next_update = (time_t) ((time(NULL) / 60 * 60) + (next_minutes * 60) +
      next_seconds);

    /* Go back to checking every minute. */
    add_hook(HOOK_MINUTELY, (Function) check_minutely);
  }
}

void expire_dnscache()
{
  uptimeip = -1;
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

    module_register(MODULE_NAME, uptime_table, 1, 5);
    if (!module_depend(MODULE_NAME, "eggdrop", 108, 0)) {
      module_undepend(MODULE_NAME);
      return "This module requires Eggdrop 1.8.0 or later.";
    }

    add_help_reference("uptime.help");
    add_hook(HOOK_MINUTELY, (Function) check_minutely);
    add_hook(HOOK_DAILY, (Function) expire_dnscache);
    init_uptime();
  }
  return NULL;
}
