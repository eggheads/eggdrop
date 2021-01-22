/*
 * dns.c -- handles:
 *   DNS resolve calls and events
 *   provides the code used by the bot if the DNS module is not loaded
 *   DNS Tcl commands
 */
/*
 * Written by Fabian Knittel <fknittel@gmx.de>
 *
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

#include "main.h"
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "dns.h"
#ifdef EGG_TDNS
  #include <errno.h>
  #include <pthread.h>
#else
  #include <setjmp.h>
#endif

extern struct dcc_t *dcc;
extern int dcc_total;
extern time_t now;
extern Tcl_Interp *interp;
#ifdef EGG_TDNS
  struct dns_thread_node *dns_thread_head;
  extern int pref_af;
#else
  extern int resolve_timeout;
  extern sigjmp_buf alarmret;
#endif

devent_t *dns_events = NULL;


/*
 *   DCC functions
 */

static void dcc_dnswait(int idx, char *buf, int len)
{
  /* Ignore anything now. */
}

static void eof_dcc_dnswait(int idx)
{
  putlog(LOG_MISC, "*", "Lost connection while resolving hostname [%s/%d]",
         iptostr(&dcc[idx].sockname.addr.sa), dcc[idx].port);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void display_dcc_dnswait(int idx, char *buf)
{
  sprintf(buf, "dns   waited %lis", (long) (now - dcc[idx].timeval));
}

static int expmem_dcc_dnswait(void *x)
{
  struct dns_info *p = (struct dns_info *) x;
  int size = 0;

  if (p) {
    size = sizeof(struct dns_info);
    if (p->host)
      size += strlen(p->host) + 1;
    if (p->cbuf)
      size += strlen(p->cbuf) + 1;
  }
  return size;
}

static void kill_dcc_dnswait(int idx, void *x)
{
  struct dns_info *p = (struct dns_info *) x;

  if (p) {
    if (p->host)
      nfree(p->host);
    if (p->cbuf)
      nfree(p->cbuf);
    nfree(p);
  }
}

struct dcc_table DCC_DNSWAIT = {
  "DNSWAIT",
  DCT_VALIDIDX,
  eof_dcc_dnswait,
  dcc_dnswait,
  0,
  0,
  display_dcc_dnswait,
  expmem_dcc_dnswait,
  kill_dcc_dnswait,
  0
};


/*
 *   DCC events
 */

/* Walk through every dcc entry and look for waiting DNS requests
 * of RES_HOSTBYIP for our IP address.
 */
static void dns_dcchostbyip(sockname_t *ip, char *hostn, int ok, void *other)
{
  int idx;

  for (idx = 0; idx < dcc_total; idx++) {
    if ((dcc[idx].type == &DCC_DNSWAIT) &&
        (dcc[idx].u.dns->dns_type == RES_HOSTBYIP) && (
#ifdef IPV6
        (ip->family == AF_INET6 &&
          IN6_ARE_ADDR_EQUAL(&dcc[idx].u.dns->ip->addr.s6.sin6_addr,
                             &ip->addr.s6.sin6_addr)) ||
        (ip->family == AF_INET &&
#endif
          (dcc[idx].u.dns->ip->addr.s4.sin_addr.s_addr ==
                              ip->addr.s4.sin_addr.s_addr)))
#ifdef IPV6
       )
#endif
    {
      if (dcc[idx].u.dns->host)
        nfree(dcc[idx].u.dns->host);
      dcc[idx].u.dns->host = get_data_ptr(strlen(hostn) + 1);
      strcpy(dcc[idx].u.dns->host, hostn);
      if (ok)
        dcc[idx].u.dns->dns_success(idx);
      else
        dcc[idx].u.dns->dns_failure(idx);
    }
  }
}

/* Walk through every dcc entry and look for waiting DNS requests
 * of RES_IPBYHOST for our hostname.
 */
static void dns_dccipbyhost(sockname_t *ip, char *hostn, int ok, void *other)
{
  int idx;

  for (idx = 0; idx < dcc_total; idx++) {
    if ((dcc[idx].type == &DCC_DNSWAIT) &&
        (dcc[idx].u.dns->dns_type == RES_IPBYHOST) &&
        !strcasecmp(dcc[idx].u.dns->host, hostn)) {
      if (ok) {
        if (dcc[idx].u.dns->ip)
          memcpy(dcc[idx].u.dns->ip, ip, sizeof(sockname_t));
        else
          memcpy(&dcc[idx].sockname, ip, sizeof(sockname_t));
        dcc[idx].u.dns->dns_success(idx);
      } else
        dcc[idx].u.dns->dns_failure(idx);
    }
  }
}

static int dns_dccexpmem(void *other)
{
  return 0;
}

devent_type DNS_DCCEVENT_HOSTBYIP = {
  "DCCEVENT_HOSTBYIP",
  dns_dccexpmem,
  dns_dcchostbyip
};

devent_type DNS_DCCEVENT_IPBYHOST = {
  "DCCEVENT_IPBYHOST",
  dns_dccexpmem,
  dns_dccipbyhost
};

void dcc_dnsipbyhost(char *hostn)
{
  devent_t *de;

  for (de = dns_events; de; de = de->next) {
    if (de->type && (de->type == &DNS_DCCEVENT_IPBYHOST) &&
        (de->lookup == RES_IPBYHOST)) {
      if (de->res_data.hostname &&
          !strcasecmp(de->res_data.hostname, hostn))
        /* No need to add anymore. */
        return;
    }
  }

  de = nmalloc(sizeof(devent_t));
  egg_bzero(de, sizeof(devent_t));

  /* Link into list. */
  de->next = dns_events;
  dns_events = de;

  de->type = &DNS_DCCEVENT_IPBYHOST;
  de->lookup = RES_IPBYHOST;
  de->res_data.hostname = nmalloc(strlen(hostn) + 1);
  strcpy(de->res_data.hostname, hostn);

  /* Send request. */
  dns_ipbyhost(hostn);
}

void dcc_dnshostbyip(sockname_t *ip)
{
  devent_t *de;

  for (de = dns_events; de; de = de->next) {
    if (de->type && (de->type == &DNS_DCCEVENT_HOSTBYIP) &&
        (de->lookup == RES_HOSTBYIP)) {
      if (de->res_data.ip_addr == ip)
        /* No need to add anymore. */
        return;
    }
  }

  de = nmalloc(sizeof(devent_t));
  egg_bzero(de, sizeof(devent_t));

  /* Link into list. */
  de->next = dns_events;
  dns_events = de;

  de->type = &DNS_DCCEVENT_HOSTBYIP;
  de->lookup = RES_HOSTBYIP;
  de->res_data.ip_addr = ip;

  /* Send request. */
  dns_hostbyip(ip);
}


/*
 *   Tcl events
 */

static void dns_tcl_iporhostres(sockname_t *ip, char *hostn, int ok, void *other)
{
  devent_tclinfo_t *tclinfo = (devent_tclinfo_t *) other;
  Tcl_DString list;

  Tcl_DStringInit(&list);
  Tcl_DStringAppendElement(&list, tclinfo->proc);
  Tcl_DStringAppendElement(&list, iptostr(&ip->addr.sa));
  Tcl_DStringAppendElement(&list, hostn);
  Tcl_DStringAppendElement(&list, ok ? "1" : "0");

  if (tclinfo->paras) {
    EGG_CONST char *argv[2];
    char *output;

    argv[0] = Tcl_DStringValue(&list);
    argv[1] = tclinfo->paras;
    output = Tcl_Concat(2, argv);

    if (Tcl_Eval(interp, output) == TCL_ERROR) {
      putlog(LOG_MISC, "*", DCC_TCLERROR, tclinfo->proc, tcl_resultstring());
      Tcl_BackgroundError(interp);
    }
    Tcl_Free(output);
  } else if (Tcl_Eval(interp, Tcl_DStringValue(&list)) == TCL_ERROR) {
    putlog(LOG_MISC, "*", DCC_TCLERROR, tclinfo->proc, tcl_resultstring());
    Tcl_BackgroundError(interp);
  }

  Tcl_DStringFree(&list);

  nfree(tclinfo->proc);
  if (tclinfo->paras)
    nfree(tclinfo->paras);
  nfree(tclinfo);
}

static int dns_tclexpmem(void *other)
{
  devent_tclinfo_t *tclinfo = (devent_tclinfo_t *) other;
  int l = 0;

  if (tclinfo) {
    l = sizeof(devent_tclinfo_t);
    if (tclinfo->proc)
      l += strlen(tclinfo->proc) + 1;
    if (tclinfo->paras)
      l += strlen(tclinfo->paras) + 1;
  }
  return l;
}

devent_type DNS_TCLEVENT_HOSTBYIP = {
  "TCLEVENT_HOSTBYIP",
  dns_tclexpmem,
  dns_tcl_iporhostres
};

devent_type DNS_TCLEVENT_IPBYHOST = {
  "TCLEVENT_IPBYHOST",
  dns_tclexpmem,
  dns_tcl_iporhostres
};

static void tcl_dnsipbyhost(char *hostn, char *proc, char *paras)
{
  devent_t *de;
  devent_tclinfo_t *tclinfo;

  de = nmalloc(sizeof(devent_t));
  egg_bzero(de, sizeof(devent_t));

  /* Link into list. */
  de->next = dns_events;
  dns_events = de;

  de->type = &DNS_TCLEVENT_IPBYHOST;
  de->lookup = RES_IPBYHOST;
  de->res_data.hostname = nmalloc(strlen(hostn) + 1);
  strcpy(de->res_data.hostname, hostn);

  /* Store additional data. */
  tclinfo = nmalloc(sizeof(devent_tclinfo_t));
  tclinfo->proc = nmalloc(strlen(proc) + 1);
  strcpy(tclinfo->proc, proc);
  if (paras) {
    tclinfo->paras = nmalloc(strlen(paras) + 1);
    strcpy(tclinfo->paras, paras);
  } else
    tclinfo->paras = NULL;
  de->other = tclinfo;

  /* Send request. */
  dns_ipbyhost(hostn);
}

static void tcl_dnshostbyip(sockname_t *ip, char *proc, char *paras)
{
  devent_t *de;
  devent_tclinfo_t *tclinfo;

  de = nmalloc(sizeof(devent_t));
  egg_bzero(de, sizeof(devent_t));

  /* Link into list. */
  de->next = dns_events;
  dns_events = de;

  de->type = &DNS_TCLEVENT_HOSTBYIP;
  de->lookup = RES_HOSTBYIP;
  de->res_data.ip_addr = ip;

  /* Store additional data. */
  tclinfo = nmalloc(sizeof(devent_tclinfo_t));
  tclinfo->proc = nmalloc(strlen(proc) + 1);
  strcpy(tclinfo->proc, proc);
  memcpy(&tclinfo->sockname, ip, sizeof(sockname_t));
  de->res_data.ip_addr = &tclinfo->sockname;
  if (paras) {
    tclinfo->paras = nmalloc(strlen(paras) + 1);
    strcpy(tclinfo->paras, paras);
  } else
    tclinfo->paras = NULL;
  de->other = tclinfo;

  /* Send request. */
  dns_hostbyip(ip);
}


/*
 *    Event functions
 */

static int dnsevent_expmem(void)
{
  devent_t *de;
  int tot = 0;

  for (de = dns_events; de; de = de->next) {
    tot += sizeof(devent_t);
    if ((de->lookup == RES_IPBYHOST) && de->res_data.hostname)
      tot += strlen(de->res_data.hostname) + 1;
    if (de->type && de->type->expmem)
      tot += de->type->expmem(de->other);
  }
  return tot;
}

void call_hostbyip(sockname_t *ip, char *hostn, int ok)
{
  devent_t *de = dns_events, *ode = NULL, *nde = NULL;

  while (de) {
    nde = de->next;
    if ((de->lookup == RES_HOSTBYIP) && (
#ifdef IPV6
        (ip->family == AF_INET6 &&
          IN6_ARE_ADDR_EQUAL(&de->res_data.ip_addr->addr.s6.sin6_addr,
                             &ip->addr.s6.sin6_addr)) ||
        (ip->family == AF_INET &&
#endif
          (de->res_data.ip_addr->addr.s4.sin_addr.s_addr ==
                              ip->addr.s4.sin_addr.s_addr)))
#ifdef IPV6
        )
#endif
    {
        /* A memcmp() could have perfectly done it .. */
      /* Remove the event from the list here, to avoid conflicts if one of
       * the event handlers re-adds another event. */
      if (ode)
        ode->next = de->next;
      else
        dns_events = de->next;

      if (de->type && de->type->event)
        de->type->event(ip, hostn, ok, de->other);
      else
        putlog(LOG_MISC, "*", "(!) Unknown DNS event type found: %s",
               (de->type && de->type->name) ? de->type->name : "<empty>");
      nfree(de);
      de = ode;
    }
    ode = de;
    de = nde;
  }
}

void call_ipbyhost(char *hostn, sockname_t *ip, int ok)
{
  devent_t *de = dns_events, *ode = NULL, *nde = NULL;

  while (de) {
    nde = de->next;
    if ((de->lookup == RES_IPBYHOST) && (!de->res_data.hostname ||
        !strcasecmp(de->res_data.hostname, hostn))) {
      /* Remove the event from the list here, to avoid conflicts if one of
       * the event handlers re-adds another event. */
      if (ode)
        ode->next = de->next;
      else
        dns_events = de->next;

      if (de->type && de->type->event)
        de->type->event(ip, hostn, ok, de->other);
      else
        putlog(LOG_MISC, "*", "(!) Unknown DNS event type found: %s",
               (de->type && de->type->name) ? de->type->name : "<empty>");

      if (de->res_data.hostname)
        nfree(de->res_data.hostname);
      nfree(de);
      de = ode;
    }
    ode = de;
    de = nde;
  }
}

#ifdef EGG_TDNS
/* The following 2 threads work like this: a libc resolver function is called,
 * that blocks the thread and returns the result or after timeout. The default
 * is RES_TIMEOUT, which is generally 5, the allowed maximum is RES_MAXRETRANS
 * (see <resolv.h>). The result is written to the threads dns_thread_node. There
 * is 1 node per thread in a linked list, which is MT-safe. One end of the pipe
 * is closed and the thread is ended by return. The other end will make
 * eggdrops mainloop select() return, read the result from the dns_thread_node
 * and call call_hostbyip() or call_ipbyhost(). No signal or tcl thread problem.
 */

void *thread_dns_hostbyip(void *arg)
{
  struct dns_thread_node *dtn = (struct dns_thread_node *) arg;
  sockname_t *addr = &dtn->addr;
  int i = 0; /* make codacy happy */

  i = getnameinfo((const struct sockaddr *) &addr->addr.sa, addr->addrlen,
                  dtn->host, sizeof dtn->host, NULL, 0, 0);
  if (i) {
#ifdef IPV6
    if (addr->family == AF_INET6)
      inet_ntop(AF_INET6, &addr->addr.s6.sin6_addr, dtn->host, sizeof dtn->host);
    else
#endif
      inet_ntop(AF_INET, &addr->addr.s4.sin_addr.s_addr, dtn->host, sizeof dtn->host);
  }
  dtn->ok = !i;
  close(dtn->fildes[1]);
  return NULL;
}

void *thread_dns_ipbyhost(void *arg)
{
  struct dns_thread_node *dtn = (struct dns_thread_node *) arg;
  struct addrinfo *res0, *res;
  int i;
  sockname_t *addr = &dtn->addr;

  i = getaddrinfo(dtn->host, NULL, NULL, &res0);
  memset(addr, 0, sizeof *addr);
  if (!i) {
#ifdef IPV6
    for (res = res0; res; res = res->ai_next) {
      if (res == res0 || res->ai_family == (pref_af ? AF_INET6 : AF_INET)) {
        addr->family = res->ai_family;
        addr->addrlen = res->ai_addrlen;
        memcpy(&addr->addr.sa, res->ai_addr, res->ai_addrlen);
        if (res->ai_family == (pref_af ? AF_INET6 : AF_INET))
          break;
      }
    }
#else
    i = 1;
    for (res = res0; res; res = res->ai_next) {
      if (res->ai_family == AF_INET) {
        addr->family = res->ai_family;
        addr->addrlen = res->ai_addrlen;
        memcpy(&addr->addr.sa, res->ai_addr, res->ai_addrlen);
        i = 0;
        break;
      }
    }
#endif
    freeaddrinfo(res0);
  }
  dtn->ok = !i;
  close(dtn->fildes[1]);
  return NULL;
}

void core_dns_hostbyip(sockname_t *addr)
{
  struct dns_thread_node *dtn = nmalloc(sizeof(struct dns_thread_node));
  pthread_t thread; /* only used by pthread_create(), no need to save */

  if (pipe(dtn->fildes) < 0) {
    putlog(LOG_MISC, "*", "core_dns_hostbyip(): pipe(): error: %s", strerror(errno));
    call_hostbyip(addr, iptostr(&addr->addr.sa), 0);
    nfree(dtn);
    return;
  }
  memcpy(&dtn->addr, addr, sizeof *addr);
  if (pthread_create(&thread, NULL, thread_dns_hostbyip, (void *) dtn)) {
    putlog(LOG_MISC, "*", "core_dns_hostbyip(): pthread_create(): error = %s", strerror(errno));
    call_hostbyip(addr, iptostr(&addr->addr.sa), 0);
    close(dtn->fildes[0]);
    close(dtn->fildes[1]);
    nfree(dtn);
    return;
  }
  dtn->type = DTN_TYPE_HOSTBYIP;
  dtn->next = dns_thread_head->next;
  dns_thread_head->next = dtn;
}

void core_dns_ipbyhost(char *host)
{
  sockname_t addr;
  struct dns_thread_node *dtn;
  pthread_t thread; /* only used by pthread_create(), no need to save  */

  /* if addr is ip instead of host */
  if (setsockname(&addr, host, 0, 0) != AF_UNSPEC) {
    call_ipbyhost(host, &addr, 1);
    return;
  }
  dtn = nmalloc(sizeof(struct dns_thread_node));
  if (pipe(dtn->fildes) < 0) {
    putlog(LOG_MISC, "*", "core_dns_ipbyhost(): pipe(): error: %s", strerror(errno));
    call_ipbyhost(host, &addr, 0);
    nfree(dtn);
    return;
  }
  dtn->next = dns_thread_head->next;
  dns_thread_head->next = dtn;
  strlcpy(dtn->host, host, sizeof dtn->host);
  if (pthread_create(&thread, NULL, thread_dns_ipbyhost, (void *) dtn)) {
    putlog(LOG_MISC, "*", "core_dns_ipbyhost(): pthread_create(): error = %s", strerror(errno));
    call_ipbyhost(host, &addr, 0);
    close(dtn->fildes[0]);
    close(dtn->fildes[1]);
    dns_thread_head->next = dtn->next;
    nfree(dtn);
    return;
  }
  dtn->type = DTN_TYPE_IPBYHOST;
}
#else /* EGG_TDNS */
/*
 *    Async DNS emulation functions
 */
void core_dns_hostbyip(sockname_t *addr)
{
  char host[256] = "";
  volatile int i = 1;

  if (addr->family == AF_INET) {
    if (!sigsetjmp(alarmret, 1)) {
      alarm(resolve_timeout);
      i = getnameinfo((const struct sockaddr *) &addr->addr.s4,
                      sizeof (struct sockaddr_in), host, sizeof host, NULL, 0, 0);
      alarm(0);
      if (i)
        debug1("dns: getnameinfo(): error = %s", gai_strerror(i));
    }
    if (i)
      inet_ntop(AF_INET, &addr->addr.s4.sin_addr.s_addr, host, sizeof host);
#ifdef IPV6
  } else {
    if (!sigsetjmp(alarmret, 1)) {
      alarm(resolve_timeout);
      i = getnameinfo((const struct sockaddr *) &addr->addr.s6,
                      sizeof (struct sockaddr_in6), host, sizeof host, NULL, 0, 0);
      alarm(0);
      if (i)
        debug1("dns: getnameinfo(): error = %s", gai_strerror(i));
    }
    if (i)
      inet_ntop(AF_INET6, &addr->addr.s6.sin6_addr, host, sizeof host);
  }
#else
  }
#endif
  call_hostbyip(addr, host, !i);
}

void core_dns_ipbyhost(char *host)
{
  sockname_t name;

  if (setsockname(&name, host, 0, 1) == AF_UNSPEC)
    call_ipbyhost(host, &name, 0);
  else
    call_ipbyhost(host, &name, 1);
}
#endif /* EGG_TDNS */

/*
 *   Misc functions
 */

int expmem_dns(void)
{
  return dnsevent_expmem();
}


/*
 *   Tcl functions
 */

/* dnslookup <ip-address> <proc> */
static int tcl_dnslookup STDVAR
{
  sockname_t addr;
  Tcl_DString paras;

  if (argc < 3) {
    Tcl_AppendResult(irp, "wrong # args: should be \"", argv[0],
                     " ip-address/hostname proc ?args...?\"", NULL);
    return TCL_ERROR;
  }

  Tcl_DStringInit(&paras);
  if (argc > 3) {
    int p;

    for (p = 3; p < argc; p++)
      Tcl_DStringAppendElement(&paras, argv[p]);
  }

  if (setsockname(&addr, argv[1], 0, 0) != AF_UNSPEC)
    tcl_dnshostbyip(&addr, argv[2], Tcl_DStringValue(&paras));
  else {
    if (strlen(argv[1]) > 255) {
      Tcl_AppendResult(irp, "hostname too long. max 255 chars.", NULL);
      return TCL_ERROR;
    }
    tcl_dnsipbyhost(argv[1], argv[2], Tcl_DStringValue(&paras));
  }

  Tcl_DStringFree(&paras);
  return TCL_OK;
}

tcl_cmds tcldns_cmds[] = {
  {"dnslookup", tcl_dnslookup},
  {NULL,                 NULL}
};
