/*
 * dns.c -- part of dns.mod
 *   domain lookup glue code for eggdrop
 *
 * Written by Fabian Knittel <fknittel@gmx.de>
 *
 * $Id: dns.c,v 1.43 2011/08/08 22:37:02 pseudo Exp $
 */
/*
 * Copyright (C) 1999 - 2011 Eggheads Development Team
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

static void dns_event_success(struct resolve *rp, int type);
static void dns_event_failure(struct resolve *rp, int type);

static Function *global = NULL;

static int dns_maxsends = 4;
static int dns_retrydelay = 3;
static int dns_cache = 86400;
static int dns_negcache = 600;

static char dns_servers[121] = "";

#include "coredns.c"


/*
 *    DNS event related code
 */
static void dns_event_success(struct resolve *rp, int type)
{
  if (!rp)
    return;

  if (type == T_PTR) {
    debug2("DNS resolved %s to %s", iptostr(rp->ip), rp->hostn);
    call_hostbyip(ntohl(rp->ip), rp->hostn, 1);
  } else if (type == T_A) {
    debug2("DNS resolved %s to %s", rp->hostn, iptostr(rp->ip));
    call_ipbyhost(rp->hostn, ntohl(rp->ip), 1);
  }
}

static void dns_event_failure(struct resolve *rp, int type)
{
  if (!rp)
    return;

  if (type == T_PTR) {
    static char s[UHOSTLEN];

    debug1("DNS resolve failed for %s", iptostr(rp->ip));
    strcpy(s, iptostr(rp->ip));
    call_hostbyip(ntohl(rp->ip), s, 0);
  } else if (type == T_A) {
    debug1("DNS resolve failed for %s", rp->hostn);
    call_ipbyhost(rp->hostn, 0, 0);
  } else
    debug2("DNS resolve failed for unknown %s / %s", iptostr(rp->ip),
           nonull(rp->hostn));
  return;
}


/*
 *    DNS Socket related code
 */

static void eof_dns_socket(int idx)
{
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

static struct dcc_table DCC_DNS = {
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

static tcl_ints dnsints[] = {
  {"dns-maxsends",   &dns_maxsends,   0},
  {"dns-retrydelay", &dns_retrydelay, 0},
  {"dns-cache",      &dns_cache,      0},
  {"dns-negcache",   &dns_negcache,   0},
  {NULL,             NULL,            0}
};

static tcl_strings dnsstrings[] = {
  {"dns-servers", dns_servers, 120,           0},
  {NULL,          NULL,          0,           0}
};

static char *dns_change(ClientData cdata, Tcl_Interp *irp,
                           EGG_CONST char *name1,
                           EGG_CONST char *name2, int flags)
{
  char buf[121], *p;
  unsigned short port;
  int i, lc, code;
  EGG_CONST char **list, *slist;

  if (flags & (TCL_TRACE_READS | TCL_TRACE_UNSETS)) {
    Tcl_DString ds;
    
    Tcl_DStringInit(&ds);
    for (i = 0; i < _res.nscount; i++) {
      snprintf(buf, sizeof buf, "%s:%d",
               iptostr(_res.nsaddr_list[i].sin_addr.s_addr),
               ntohs(_res.nsaddr_list[i].sin_port));
      Tcl_DStringAppendElement(&ds, buf);
    }
    slist = Tcl_DStringValue(&ds);
    Tcl_SetVar2(interp, name1, name2, slist, TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&ds);
  } else { /* TCL_TRACE_WRITES */
    slist = Tcl_GetVar2(interp, name1, name2, TCL_GLOBAL_ONLY);
    code = Tcl_SplitList(interp, slist, &lc, &list);
    if (code == TCL_ERROR)
      return "variable must be a list";
    /* reinitialize the list */
    _res.nscount = 0;
    for (i = 0; i < lc; i++) {
      if ((p = strchr(list[i], ':'))) {
        *p++ = 0;
        /* allow non-standard ports */
        port = atoi(p);
      } else
        port = NAMESERVER_PORT; /* port 53 */
      /* Ignore invalid addresses */
      if (egg_inet_aton(list[i], &_res.nsaddr_list[_res.nscount].sin_addr)) {
        _res.nsaddr_list[_res.nscount].sin_port = htons(port);
        _res.nsaddr_list[_res.nscount].sin_family = AF_INET;
        _res.nscount++;
      }
    }
    Tcl_Free((char *) list);
  }
  return NULL;
}


/*
 *    DNS module related code
 */

static void dns_free_cache(void)
{
  struct resolve *rp, *rpnext;

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
  if (details) {
    int i, size = dns_expmem();

    dprintf(idx, "    Async DNS resolver is active.\n");
    dprintf(idx, "    DNS server list:");
    for (i = 0; i < _res.nscount; i++)
      dprintf(idx, " %s:%d", iptostr(_res.nsaddr_list[i].sin_addr.s_addr),
              ntohs(_res.nsaddr_list[i].sin_port));
    dprintf(idx, "\n");
    dprintf(idx, "    Using %d byte%s of memory\n", size,
            (size != 1) ? "s" : "");
  }
  return 0;
}

static char *dns_close()
{
  int i;

  del_hook(HOOK_DNS_HOSTBYIP, (Function) dns_lookup);
  del_hook(HOOK_DNS_IPBYHOST, (Function) dns_forward);
  del_hook(HOOK_SECONDLY, (Function) dns_check_expires);
  rem_tcl_ints(dnsints);
  rem_tcl_strings(dnsstrings);
  Tcl_UntraceVar(interp, "dns-servers",
                 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                 dns_change, NULL);

  for (i = 0; i < dcc_total; i++) {
    if (dcc[i].type == &DCC_DNS && dcc[i].sock == resfd) {
      killsock(dcc[i].sock);
      lostdcc(i);
      break;
    }
  }

  dns_free_cache();
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *dns_start();

static Function dns_table[] = {
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

  module_register(MODULE_NAME, dns_table, 1, 0);
  if (!module_depend(MODULE_NAME, "eggdrop", 106, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.6.0 or later.";
  }

  idx = new_dcc(&DCC_DNS, 0);
  if (idx < 0)
    return "NO MORE DCC CONNECTIONS -- Can't create DNS socket.";
  if (!init_dns_core()) {
    lostdcc(idx);
    return "DNS initialisation failed.";
  }
  dcc[idx].sock = resfd;
  dcc[idx].timeval = now;
  strcpy(dcc[idx].nick, "(dns)");

  Tcl_TraceVar(interp, "dns-servers",
               TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
               dns_change, NULL);
  add_hook(HOOK_SECONDLY, (Function) dns_check_expires);
  add_hook(HOOK_DNS_HOSTBYIP, (Function) dns_lookup);
  add_hook(HOOK_DNS_IPBYHOST, (Function) dns_forward);
  add_tcl_ints(dnsints);
  add_tcl_strings(dnsstrings);
  return NULL;
}
