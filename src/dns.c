/* 
 * dns.c -- handles:
 *   DNS calls
 *   provides the code used by the bot if the DNS module is not loaded
 * 
 * $Id: dns.c,v 1.8 1999/12/21 17:35:09 fabian Exp $
 */
/* 
 * Written by Fabian Knittel
 * 
 * Copyright (C) 1999  Eggheads
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
  Context;
}

void eof_dcc_dnswait(int idx)
{
  Context;
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

  Context;
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

  Context;
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

  Context;
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

  Context;
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

  Context;
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
  Context;
}

void block_dns_ipbyhost(char *host)
{
  struct in_addr inaddr;

  Context;
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
  Context;
}
