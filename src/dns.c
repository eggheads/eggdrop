/*
 * dns.c - handles DNS calls. Also provides the code used by the bot
 * if the DNS module is not loaded.
 */
/*
 * This file is part of the eggdrop source code.
 *
 * Copyright (C) 1999  Eggheads
 * Copyright (C) 1997  Robey Pointer
 *
 * Distributed according to the GNU General Public License. For full
 * details, read the top of 'main.c' or the file called COPYING that
 * was distributed with this code.
 */
/*
 * Mon Oct 04 22:24:51 1999  Fabian Knittel
 *     * minor fixes
 * Sun Oct 03 18:34:41 1999  Fabian Knittel
 *     * Initial release
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
  unsigned long addr = htonl(ip);
  unsigned char *p;
  static char s[UHOSTLEN];

  /*
   * This actually copies hostnamefromip(), which is ugly, but there
   * is no way to determine if the lookup was successful or not, using
   * that interface.
   */
  if (!setjmp(alarmret)) {
    alarm(resolve_timeout);
    hp = gethostbyaddr((char *) &addr, sizeof(addr), AF_INET);
    alarm(0);
  } else {
    hp = NULL;
  }
  if (hp == NULL) {
    p = (unsigned char *) &addr;
    sprintf(s, "%u.%u.%u.%u", p[0], p[1], p[2], p[3]);
  } else {
    strncpy(s, hp->h_name, UHOSTLEN - 1);
    s[UHOSTLEN - 1] = 0;
  }
  /* call hooks */
  call_hostbyip(ip, s, hp ? 1 : 0);
}

void block_dns_ipbyhost(char *host)
{
  if (!setjmp(alarmret)) {
    struct hostent *hp;
    struct in_addr *in;
    IP ip = 0;

    alarm(resolve_timeout);
    hp = gethostbyname(host);
    alarm(0);

    in = (struct in_addr *) (hp->h_addr_list[0]);
    ip = (IP) (in->s_addr);
    call_ipbyhost(host, ip, 1);
  } else {
    call_ipbyhost(host, 0, 0);
  }
}
