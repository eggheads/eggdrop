/*
 * dns.c - handles DNS calls. Also provides the code used by the bot
 * if the DNS module is not loaded.
 */
/*
 * This file is part of the eggdrop source code.
 *
 * Copyright (C) 1999  Eggheads
 * Written by Fabian Knittel
 *
 * Distributed according to the GNU General Public License. For full
 * details, read the top of 'main.c' or the file called COPYING that
 * was distributed with this code.
 */


#include "main.h"
#include <netdb.h>
#include <setjmp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern struct dcc_t *dcc;
extern int dcc_total;
extern int resolve_timeout;
extern time_t now;
extern jmp_buf alarmret;


void dcc_dnswait(int idx, char *buf, int len)
{
  /* ignore anything now */
  context;
}

void eof_dcc_dnswait(int idx)
{
  context;
  putlog(LOG_MISC, "*", "Lost connection while resolving hostname [%s/%d]",
	 iptostr(dcc[idx].addr), dcc[idx].port);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void display_dcc_dnswait(int idx, char *buf)
{
  sprintf(buf, "dns   waited %lus", now - dcc[idx].timeval);
}

static int expmem_dcc_dnswait(void *x)
{
  register struct dns_info *p = (struct dns_info *) x;
  int size = 0;

  context;
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
  register struct dns_info *p = (struct dns_info *) x;

  context;
  if (p) {
    if (p->host)
      nfree(p->host);
    if (p->cbuf)
      nfree(p->cbuf);
    nfree(p);
  }
}

struct dcc_table DCC_DNSWAIT =
{
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

/* Walk through every dcc entry and look for waiting DNS requests
 * of RES_HOSTBYIP for our IP address */
void call_hostbyip(IP ip, char *hostn, int ok)
{
  int idx;

  context;
  for (idx = 0; idx < dcc_total; idx++) {
    if ((dcc[idx].type == &DCC_DNSWAIT) &&
        (dcc[idx].u.dns->dns_type == RES_HOSTBYIP) &&
        (dcc[idx].u.dns->ip == ip)) {
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
 * of RES_IPBYHOST for our hostname */
void call_ipbyhost(char *hostn, IP ip, int ok)
{
  int idx;

  context;
  for (idx = 0; idx < dcc_total; idx++) {
    if ((dcc[idx].type == &DCC_DNSWAIT) &&
        (dcc[idx].u.dns->dns_type == RES_IPBYHOST) &&
        !strcmp(dcc[idx].u.dns->host, hostn)) {
      dcc[idx].u.dns->ip = ip;
      if (ok)
        dcc[idx].u.dns->dns_success(idx);
      else
        dcc[idx].u.dns->dns_failure(idx);
    }
  }
}


/*
 *    Async DNS emulation
 */

void block_dns_hostbyip(IP ip)
{
  struct hostent *hp;
  unsigned long addr = my_htonl(ip);
  static char s[UHOSTLEN];

  context;
  if (!setjmp(alarmret)) {
    alarm(resolve_timeout);
    hp = gethostbyaddr((char *) &addr, sizeof(addr), AF_INET);
    alarm(0);
    if (hp) {
      strncpy(s, hp->h_name, UHOSTLEN - 1);
      s[UHOSTLEN - 1] = 0;
    } else
      strcpy(s, iptostr(addr));
  } else {
    hp = NULL;
    strcpy(s, iptostr(addr));
  }
  /* call hooks */
  call_hostbyip(ip, s, hp ? 1 : 0);
  context;
}

void block_dns_ipbyhost(char *host)
{
  struct in_addr inaddr;

  context;
  /* Check if someone passed us an IP address as hostname 
   * and return it straight away */
  if (inet_aton(host, &inaddr)) {
    call_ipbyhost(host, my_ntohl(inaddr.s_addr), 1);
    return;
  }
  if (!setjmp(alarmret)) {
    struct hostent *hp;
    struct in_addr *in;
    IP ip = 0;

    alarm(resolve_timeout);
    hp = gethostbyname(host);
    alarm(0);

    if (hp) {
      in = (struct in_addr *) (hp->h_addr_list[0]);
      ip = (IP) (in->s_addr);
      call_ipbyhost(host, my_ntohl(ip), 1);
      return;
    }
  }
  /* Fall through */
  call_ipbyhost(host, 0, 0);
  context;
}
