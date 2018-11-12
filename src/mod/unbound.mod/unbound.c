/*
 * unbound.c -- part of unbound.mod
 *   Asynchronous dns resolve via libunbound
 *
 * Written by Michael Ortmann 11 November 2018
 */
/*
 * Copyright (C) 1999 - 2018 Eggheads Development Team
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

#define MODULE_NAME "unbound"

#include <unbound.h>
#include "src/mod/module.h"

static Function *global = NULL;

static struct ub_ctx* ctx;

static void mycallback(void* mydata, int err, struct ub_result* result)
{
  int *done = (int *) mydata;

  putlog(LOG_MISC, "*", "Unbound: mycallback(): start");
  *done = 1;
  if(err) {
    putlog(LOG_MISC, "*", "Unbound: resolve error: %s\n", ub_strerror(err));
    return;
  }
  /* show first result */
  if(result->havedata)
    putlog(LOG_MISC, "*", "Unbound: The address of %s is %s\n", result->qname,
           inet_ntoa(*(struct in_addr *)result->data[0]));
  else
    putlog(LOG_MISC, "*", "Unbound: TODO: No address for %s", result->qname);

  /* TODO see and implement:
   *   dns.mod/dns.c:
   *     dns_event_success()
   *     dns_event_failure()
   *   dns.c:
   *     call_ipbyhost(i, &rp->sockname, 1);
   */

  ub_resolve_free(result);
}

static void unbound_resolve(char *name)
{
  int retval;
  volatile int done = 0;

  putlog(LOG_MISC, "*", "Unbound: unbound_resolve(): name = %s", name);
  /* TODO IPV6 rrtype = 28 AAAA */
  retval = ub_resolve_async(ctx, name,
                            1 /* TYPE A (IPv4 address) */,
                            1 /* CLASS IN (internet) */,
                            (void *) &done, mycallback, NULL);
  if (retval) {
    putlog(LOG_MISC, "*", "Unbound: resolve error: %s\n", ub_strerror(retval));
    return;
  }
}

static void unbound_secondly(void)
{
  int retval = ub_process(ctx);

  if (retval)
    putlog(LOG_MISC, "*", "Unbound: resolve error: %s\n", ub_strerror(retval));
}

static int unbound_expmem()
{
  int size = 0;

  return size;
}

static void unbound_report(int idx, int details)
{
  if (!details) {
    dprintf(idx, "    Unbound version %s\n", ub_version()); 
    int size = unbound_expmem();

    dprintf(idx, "    Using %d byte%s of memory\n", size,
            (size != 1) ? "s" : "");
  }
}

static char *unbound_close()
{
  del_hook(HOOK_DNS_IPBYHOST, (Function) unbound_resolve);
  del_hook(HOOK_SECONDLY, (Function) unbound_secondly);
  module_undepend(MODULE_NAME);
  return NULL;
}

EXPORT_SCOPE char *unbound_start();

static Function unbound_table[] = {
  (Function) unbound_start,
  (Function) unbound_close,
  (Function) unbound_expmem,
  (Function) unbound_report,
};

char *unbound_start(Function *global_funcs)
{
  int retval;

  global = global_funcs;

  module_register(MODULE_NAME, unbound_table, 0, 1);

  if (!module_depend(MODULE_NAME, "eggdrop", 108, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.8.0 or later.";
  }

  ctx = ub_ctx_create();
  if(!ctx)
    return "Unbound error: could not create unbound context.";
  retval = ub_ctx_resolvconf(ctx, "/etc/resolv.conf");
  if(retval)
    putlog(LOG_MISC, "*", "Unbound: resolve warning: %s\n", ub_strerror(retval));
  retval = ub_ctx_hosts(ctx, "/etc/hosts");
  if(retval)
    putlog(LOG_MISC, "*", "Unbound: resolve warning: %s\n", ub_strerror(retval));

  add_hook(HOOK_DNS_IPBYHOST, (Function) unbound_resolve);
  add_hook(HOOK_SECONDLY, (Function) unbound_secondly);

  return NULL;
}
