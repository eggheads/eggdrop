/* 
 * dns.c -- part of dns.mod
 *   domain lookup glue code for eggdrop
 * 
 * Written by Fabian Knittel <fknittel@gmx.de>
 * 
 * $Id: dns.c,v 1.14 2000/09/02 19:45:23 fabian Exp $
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

#define MODULE_NAME "dns"

#include "src/mod/module.h"
#include "dns.h"

static void dns_event_success();
static void dns_event_failure();


static Function *global = NULL;

#include "coredns.c"


/* 
 *    DNS event related code
 */

static void dns_event_success(struct resolve *rp, int type)
{
  if (!rp)
    return;

  Context;
  if (type == T_PTR) {
    debug2("DNS resolved %s to %s", iptostr(rp->ip), rp->hostn);
    call_hostbyip(my_ntohl(rp->ip), rp->hostn, 1);
  } else if (type == T_A) {
    debug2("DNS resolved %s to %s", rp->hostn, iptostr(rp->ip));
    call_ipbyhost(rp->hostn, my_ntohl(rp->ip), 1);
  }
}

static void dns_event_failure(struct resolve *rp)
{
  if (!rp)
    return;

  Context;
  /* T_PTR */
  if (rp->ip) {
    static char s[UHOSTLEN];

    debug1("DNS resolve failed for %s", iptostr(rp->ip));
    strcpy(s, iptostr(rp->ip));
    call_hostbyip(my_ntohl(rp->ip), s, 0);
  }  
  /* T_A */
  else if (rp->hostn) {
    debug1("DNS resolve failed for %s", rp->hostn);
    call_ipbyhost(rp->hostn, 0, 0);
  }
  return;
}


/* 
 *    DNS Socket related code
 */

static void eof_dns_socket(int idx)
{
  Context;
  putlog(LOG_MISC, "*", "DNS Error: socket closed.");
  killsock(dcc[idx].sock);
  /* Try to reopen socket */
  if (init_dns_network()) {
    putlog(LOG_MISC, "*", "DNS socket successfully reopened!");
    dcc[idx].sock = resfd;
    dcc[idx].timeval = now;
  } else
    lostdcc(idx);
}

static void dns_socket(int idx, char *buf, int len)
{
  dns_ack();
}

static void display_dns_socket(int idx, char *buf)
{
  strcpy(buf, "dns   (ready)");
}

struct dcc_table DCC_DNS =
{
  "DNS",
  DCT_LISTEN,
  eof_dns_socket,
  dns_socket,
  NULL,
  NULL,
  display_dns_socket,
  NULL,
  NULL,
  NULL
};


/* 
 *    DNS module related code
 */

static void cmd_resolve(struct userrec *u, int idx, char *par)
{
  struct in_addr inaddr;

  Context;
  if (egg_inet_aton(par, &inaddr))
    dns_lookup(my_ntohl(inaddr.s_addr));
  else
    dns_forward(par);
  return;
}


static void dns_free_cache(void)
{
  struct resolve *rp, *rpnext;

  Context;
  for (rp = expireresolves; rp; rp = rpnext) {
    rpnext = rp->next;
    if (rp->hostn)
      nfree(rp->hostn);
    nfree(rp);
  } 
  expireresolves = NULL;
}

static int dns_cache_expmem(void)
{
  struct resolve *rp;
  int size = 0;

  for (rp = expireresolves; rp; rp = rp->next) {
    size += sizeof(struct resolve);
    if (rp->hostn)
      size += strlen(rp->hostn) + 1;
  } 
  return size;
}

static int dns_expmem(void)
{
  return dns_cache_expmem();
}

static int dns_report(int idx, int details)
{
  dprintf(idx, "    DNS resolver is active.\n");
  if (details)
    dprintf(idx, "    (cache uses %d bytes of memory)\n", dns_cache_expmem());
  return 0;
}

static cmd_t dns_dcc[] = {
  {"resolve",		"",	(Function) cmd_resolve,		NULL},
  {NULL,		NULL,	NULL,				NULL}
};

static char *dns_close()
{
  int i;

  Context;
  del_hook(HOOK_DNS_HOSTBYIP, (Function) dns_lookup);
  del_hook(HOOK_DNS_IPBYHOST, (Function) dns_forward);
  del_hook(HOOK_SECONDLY, (Function) dns_check_expires);
  rem_builtins(H_dcc, dns_dcc);

  for (i = 0; i < dcc_total; i++) {
    if (dcc[i].type == &DCC_DNS &&
	dcc[i].sock == resfd) {
      killsock(dcc[i].sock);
      lostdcc(i);
      break;
    }
  }

  dns_free_cache();
  module_undepend(MODULE_NAME);
  Context;
  return NULL;
}

char *dns_start();

static Function dns_table[] =
{
  /* 0 - 3 */
  (Function) dns_start,
  (Function) dns_close,
  (Function) dns_expmem,
  (Function) dns_report,
  /* 4 - 7 */
};

char *dns_start(Function *global_funcs)
{
  int idx;
  
  global = global_funcs;
  Context;
  module_register(MODULE_NAME, dns_table, 1, 0);
  if (!module_depend(MODULE_NAME, "eggdrop", 105, 3)) {
    module_undepend(MODULE_NAME);
    return "This module requires eggdrop1.5.3 or later";
  }

  if (!init_dns_core())
    return "DNS initialisation failed.";
  idx = new_dcc(&DCC_DNS, 0);
  dcc[idx].sock = resfd;
  dcc[idx].timeval = now;
  strcpy(dcc[idx].nick, "(dns)");

  add_hook(HOOK_SECONDLY, (Function) dns_check_expires);
  add_hook(HOOK_DNS_HOSTBYIP, (Function) dns_lookup);
  add_hook(HOOK_DNS_IPBYHOST, (Function) dns_forward);
  add_builtins(H_dcc, dns_dcc);
  Context;
  return NULL;
}
