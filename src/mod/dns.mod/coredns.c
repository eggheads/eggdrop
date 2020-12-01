/*
 * coredns.c -- part of dns.mod
 *   This file contains all core functions needed for the eggdrop dns module.
 *   Many of them are only minimaly modified from the original source.
 *
 * Modified/written by Fabian Knittel <fknittel@gmx.de>
 * IPv6 support added by pseudo <pseudo@egg6.net>
 * /etc/hosts support added by Michael Ortmann
 */
/*
 * Portions Copyright (C) 1999 - 2020 Eggheads Development Team
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

/*
 * Borrowed from mtr  --  a network diagnostic tool
 * Copyright (C) 1997,1998  Matt Kimball <mkimball@xmission.com>
 * Released under the GPL, as above.
 *
 * Non-blocking DNS portion --
 * Copyright (C) 1998  Simon Kirby <sim@neato.org>
 * Released under the GPL, as above.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#undef answer /* before resolv.h because it could collide with src/mod/module.h
                 (dietlibc) */
#include <resolv.h>
#include <errno.h>

/* OpenBSD */
#ifndef NS_GET16
#define NS_GET16 GETSHORT
#endif
#ifndef NS_GET32
#define NS_GET32 GETLONG
#endif

/* Defines */

#define BASH_SIZE        8192   /* Size of hash tables */
#define HOSTNAMELEN       255   /* From RFC */

#define RES_ERR "DNS Resolver error: "
#define RES_MSG "DNS Resolver: "
#define RES_WRN "DNS Resolver warning: "

#define MAX_PACKETSIZE (PACKETSZ)
#define MAX_DOMAINLEN (MAXDNAME)

/* Macros */

#define nonull(s) (s) ? s : nullstring
#define BASH_MODULO(x) ((x) & 8191)     /* Modulo for hash table size */

/* Non-blocking nameserver interface routines */

#ifdef DEBUG_DNS
#  define RESPONSECODES_COUNT 6
static char *responsecodes[RESPONSECODES_COUNT + 1] = {
  "no error",
  "format error in query",
  "server failure",
  "queried domain name does not exist",
  "requested query type not implemented",
  "refused by name server",
  "unknown error",
};

#  ifdef IPV6
#    define RESOURCETYPES_COUNT 29
#  else
#    define RESOURCETYPES_COUNT 17
#  endif
static const char *resourcetypes[RESOURCETYPES_COUNT + 1] = {
  "unknown type",
  "A: host address",
  "NS: authoritative name server",
  "MD: mail destination (OBSOLETE)",
  "MF: mail forwarder (OBSOLETE)",
  "CNAME: name alias",
  "SOA: authority record",
  "MB: mailbox domain name (EXPERIMENTAL)",
  "MG: mail group member (EXPERIMENTAL)",
  "MR: mail rename domain name (EXPERIMENTAL)",
  "NULL: NULL RR (EXPERIMENTAL)",
  "WKS: well known service description",
  "PTR: domain name pointer",
  "HINFO: host information",
  "MINFO: mailbox or mail list information",
  "MX: mail exchange",
  "TXT: text string",
#  ifdef IPV6
  "RP: responsible person",
  "AFSDB: AFS cell database",
  "X25: X_25 calling address",
  "ISDN: ISDN calling address",
  "RT: router",
  "NSAP: NSAP address",
  "NSAP_PTR: reverse NSAP lookup (deprecated)",
  "SIG: security signature",
  "KEY: security key",
  "PX: X.400 mail mapping",
  "GPOS: geographical position (withdrawn)",
  "AAAA: IP6 Address",
#  endif /* IPV6 */
  "unknown type",
};

#  define CLASSTYPES_COUNT 5
static const char *classtypes[CLASSTYPES_COUNT + 1] = {
  "unknown class",
  "IN: the Internet",
  "CS: CSNET (OBSOLETE)",
  "CH: CHAOS",
  "HS: Hesiod [Dyer 87]",
  "unknown class"
};

#endif /* DEBUG_DNS */

typedef struct {
  uint16_t id;                 /* Packet id */
  uint8_t databyte_a;
  /* rd:1                             recursion desired
   * tc:1                             truncated message
   * aa:1                             authoritative answer
   * opcode:4                         purpose of message
   * qr:1                             response flag
   */
  uint8_t databyte_b;
  /* rcode:4                          response code
   * unassigned:2                     unassigned bits
   * pr:1                             primary server required (non standard)
   * ra:1                             recursion available
   */
  uint16_t qdcount;            /* Query record count */
  uint16_t ancount;            /* Answer record count */
  uint16_t nscount;            /* Authority reference record count */
  uint16_t arcount;            /* Resource reference record count */
} packetheader;

#ifndef HFIXEDSZ
#define HFIXEDSZ (sizeof(packetheader))
#endif

/*
 * Byte order independent macros for packetheader
 */
#define getheader_rd(x) (x->databyte_a & 1)
#define getheader_tc(x) ((x->databyte_a >> 1) & 1)
#define getheader_aa(x) ((x->databyte_a >> 2) & 1)
#define getheader_opcode(x) ((x->databyte_a >> 3) & 15)
#define getheader_qr(x) (x->databyte_a >> 7)
#define getheader_rcode(x) (x->databyte_b & 15)
#define getheader_pr(x) ((x->databyte_b >> 6) & 1)
#define getheader_ra(x) (x->databyte_b >> 7)

static uint32_t resrecvbuf[(MAX_PACKETSIZE + 7) >> 2]; /* MUST BE DWORD ALIGNED */

static struct resolve *idbash[BASH_SIZE];
static struct resolve *ipbash[BASH_SIZE];
#ifdef IPV6
static struct resolve *ip6bash[BASH_SIZE];
#endif
static struct resolve *hostbash[BASH_SIZE];
static struct resolve *expireresolves = NULL;

static IP localhost;

static long idseed = 0xdeadbeef;
static long aseed;

static int resfd;

static char tempstring[512];
static char namestring[1024 + 1];
static char stackstring[1024 + 1];

#ifdef DEBUG_DNS
static char sendstring[1024 + 1];
#endif /* DEBUG_DNS */

static const char nullstring[] = "";

#ifdef res_ninit
#define MY_RES_INIT() res_ninit(&myres);
#define RES_MKQUERY(a, b, c, d, e, f, g, h, i) \
    res_nmkquery(&myres, a, b, c, d, e, f, g, h, i)
struct __res_state myres;
#else
#define MY_RES_INIT() res_init();
#define RES_MKQUERY(a, b, c, d, e, f, g, h, i) \
    res_mkquery(a, b, c, d, e, f, g, h, i)
#define myres _res
#endif


/*
 *    Miscellaneous helper functions
 */

#ifdef DEBUG_DNS
/* Displays the time difference passed in signeddiff.
 */
static char *strtdiff(char *d, long signeddiff)
{
  uint32_t diff;
  uint32_t seconds, minutes, hours;
  long day;

  if ((diff = labs(signeddiff))) {
    seconds = diff % 60;
    diff /= 60;
    minutes = diff % 60;
    diff /= 60;
    hours = diff % 24;
    day = signeddiff / (60 * 60 * 24);
    if (day)
      sprintf(d, "%lid", day);
    else
      *d = '\0';
    if (hours)
      sprintf(d + strlen(d), "%uh", hours);
    if (minutes)
      sprintf(d + strlen(d), "%um", minutes);
    if (seconds)
      sprintf(d + strlen(d), "%us", seconds);
  } else
    sprintf(d, "0s");
  return d;
}
#endif /* DEBUG_DNS */

/* Allocate memory to hold one resolve request structure.
 */
static struct resolve *allocresolve()
{
  struct resolve *rp;

  rp = nmalloc(sizeof *rp);
  egg_bzero(rp, sizeof(struct resolve));
  return rp;
}

/*
 *    Hash and linked-list related functions
 */

/* Return the hash bucket number for id.
 */
static uint32_t getidbash(uint16_t id)
{
  return (uint32_t) BASH_MODULO(id);
}

/* Return the hash bucket number for ip.
 */
static uint32_t getipbash(IP ip)
{
  return (uint32_t) BASH_MODULO(ip);
}

#ifdef IPV6
static unsigned long getip6bash(struct in6_addr *ip6) {
  uint32_t x, y;
  memcpy(&x, ip6->s6_addr     , sizeof x);
  memcpy(&y, ip6->s6_addr + 12, sizeof y);
  x ^= y;
  return (unsigned long) BASH_MODULO(x);
}
#endif

/* Return the hash bucket number for host.
 */
static uint32_t gethostbash(char *host)
{
  uint32_t bashvalue = 0;

  for (; *host; host++) {
    bashvalue ^= *host;
    bashvalue += (*host >> 1) + (bashvalue >> 1);
  }
  return BASH_MODULO(bashvalue);
}

/* Insert request structure addrp into the id hash table.
 */
static void linkresolveid(struct resolve *addrp)
{
  struct resolve *rp;
  uint32_t bashnum;

  bashnum = getidbash(addrp->id);
  rp = idbash[bashnum];
  if (rp) {
    while ((rp->nextid) && (addrp->id > rp->nextid->id))
      rp = rp->nextid;
    while ((rp->previousid) && (addrp->id < rp->previousid->id))
      rp = rp->previousid;
    if (rp->id < addrp->id) {
      addrp->previousid = rp;
      addrp->nextid = rp->nextid;
      if (rp->nextid)
        rp->nextid->previousid = addrp;
      rp->nextid = addrp;
    } else if (rp->id > addrp->id) {
      addrp->previousid = rp->previousid;
      addrp->nextid = rp;
      if (rp->previousid)
        rp->previousid->nextid = addrp;
      rp->previousid = addrp;
    } else                        /* Trying to add the same id! */
      return;
  } else
    addrp->nextid = addrp->previousid = NULL;
  idbash[bashnum] = addrp;
}

/* Remove request structure rp from the id hash table.
 */
static void unlinkresolveid(struct resolve *rp)
{
  uint32_t bashnum;

  bashnum = getidbash(rp->id);
  if (idbash[bashnum] == rp) {
    if (rp->previousid)
      idbash[bashnum] = rp->previousid;
    else
      idbash[bashnum] = rp->nextid;
  }
  if (rp->nextid)
    rp->nextid->previousid = rp->previousid;
  if (rp->previousid)
    rp->previousid->nextid = rp->nextid;
}

/* Insert request structure addrp into the host hash table.
 */
static void linkresolvehost(struct resolve *addrp)
{
  struct resolve *rp;
  uint32_t bashnum;
  int ret;

  bashnum = gethostbash(addrp->hostn);
  rp = hostbash[bashnum];
  if (rp) {
    while ((rp->nexthost) &&
           (strcasecmp(addrp->hostn, rp->nexthost->hostn) < 0))
      rp = rp->nexthost;
    while ((rp->previoushost) &&
           (strcasecmp(addrp->hostn, rp->previoushost->hostn) > 0))
      rp = rp->previoushost;
    ret = strcasecmp(addrp->hostn, rp->hostn);
    if (ret < 0) {
      addrp->previoushost = rp;
      addrp->nexthost = rp->nexthost;
      if (rp->nexthost)
        rp->nexthost->previoushost = addrp;
      rp->nexthost = addrp;
    } else if (ret > 0) {
      addrp->previoushost = rp->previoushost;
      addrp->nexthost = rp;
      if (rp->previoushost)
        rp->previoushost->nexthost = addrp;
      rp->previoushost = addrp;
    } else                        /* Trying to add the same host! */
      return;
  } else
    addrp->nexthost = addrp->previoushost = NULL;
  hostbash[bashnum] = addrp;
}

/* Remove request structure rp from the host hash table.
 */
static void unlinkresolvehost(struct resolve *rp)
{
  uint32_t bashnum;

  bashnum = gethostbash(rp->hostn);
  if (hostbash[bashnum] == rp) {
    if (rp->previoushost)
      hostbash[bashnum] = rp->previoushost;
    else
      hostbash[bashnum] = rp->nexthost;
  }
  if (rp->nexthost)
    rp->nexthost->previoushost = rp->previoushost;
  if (rp->previoushost)
    rp->previoushost->nexthost = rp->nexthost;
  nfree(rp->hostn);
}

/* Insert request structure addrp into the ip hash table.
 */
static void linkresolveip(struct resolve *addrp)
{
  struct resolve *rp;
  uint32_t bashnum;

  bashnum = getipbash(addrp->ip);
  rp = ipbash[bashnum];
  if (rp) {
    while ((rp->nextip) && (addrp->ip > rp->nextip->ip))
      rp = rp->nextip;
    while ((rp->previousip) && (addrp->ip < rp->previousip->ip))
      rp = rp->previousip;
    if (rp->ip < addrp->ip) {
      addrp->previousip = rp;
      addrp->nextip = rp->nextip;
      if (rp->nextip)
        rp->nextip->previousip = addrp;
      rp->nextip = addrp;
    } else if (rp->ip > addrp->ip) {
      addrp->previousip = rp->previousip;
      addrp->nextip = rp;
      if (rp->previousip)
        rp->previousip->nextip = addrp;
      rp->previousip = addrp;
    } else                        /* Trying to add the same ip! */
      return;
  } else
    addrp->nextip = addrp->previousip = NULL;
  ipbash[bashnum] = addrp;
}

#ifdef IPV6
static void linkresolveip6(struct resolve *addrp){
  struct resolve *rp;
  unsigned long bashnum;
  bashnum = getip6bash(&addrp->sockname.addr.s6.sin6_addr);
  rp = ip6bash[bashnum];
  if (rp){
    while ((rp->nextip) &&
           (addrp->sockname.addr.s6.sin6_addr.s6_addr[15] >
            rp->nextip->sockname.addr.s6.sin6_addr.s6_addr[15]))
      rp = rp->nextip;
    while ((rp->previousip) &&
           (addrp->sockname.addr.s6.sin6_addr.s6_addr[15] <
            rp->previousip->sockname.addr.s6.sin6_addr.s6_addr[15]))
      rp = rp->previousip;
    if (rp->sockname.addr.s6.sin6_addr.s6_addr[15] <
        addrp->sockname.addr.s6.sin6_addr.s6_addr[15]) {
      addrp->previousip = rp;
      addrp->nextip = rp->nextip;
      if (rp->nextip)
        rp->nextip->previousip = addrp;
      rp->nextip = addrp;
    } else {
      addrp->previousip = rp->previousip;
      addrp->nextip = rp;
      if (rp->previousip)
        rp->previousip->nextip = addrp;
      rp->previousip = addrp;
    }
  } else
    addrp->nextip = addrp->previousip = NULL;
  ip6bash[bashnum] = addrp;
}

static void unlinkresolveip6(struct resolve *rp)
{
  uint32_t bashnum;

  bashnum = getip6bash(&rp->sockname.addr.s6.sin6_addr);
  if (ip6bash[bashnum] == rp) {
    if (rp->previousip)
      ip6bash[bashnum] = rp->previousip;
    else
      ip6bash[bashnum] = rp->nextip;
  }
  if (rp->nextip)
    rp->nextip->previousip = rp->previousip;
  if (rp->previousip)
    rp->previousip->nextip = rp->nextip;
}
#endif

/* Remove request structure rp from the ip hash table.
 */
static void unlinkresolveip(struct resolve *rp)
{
  uint32_t bashnum;

  bashnum = getipbash(rp->ip);
  if (ipbash[bashnum] == rp) {
    if (rp->previousip)
      ipbash[bashnum] = rp->previousip;
    else
      ipbash[bashnum] = rp->nextip;
  }
  if (rp->nextip)
    rp->nextip->previousip = rp->previousip;
  if (rp->previousip)
    rp->previousip->nextip = rp->nextip;
}

/* Add request structure rp to the expireresolves list. Entries are sorted
 * by expire time.
 */
static void linkresolve(struct resolve *rp)
{
  struct resolve *irp;

  if (expireresolves) {
    irp = expireresolves;
    while ((irp->next) && (rp->expiretime >= irp->expiretime))
      irp = irp->next;
    if (rp->expiretime >= irp->expiretime) {
      rp->next = NULL;
      rp->previous = irp;
      irp->next = rp;
    } else {
      rp->previous = irp->previous;
      rp->next = irp;
      if (irp->previous)
        irp->previous->next = rp;
      else
        expireresolves = rp;
      irp->previous = rp;
    }
  } else {
    rp->next = NULL;
    rp->previous = NULL;
    expireresolves = rp;
  }
}

/* Remove request structure rp from the expireresolves list.
 */
static void untieresolve(struct resolve *rp)
{
  if (rp->previous)
    rp->previous->next = rp->next;
  else
    expireresolves = rp->next;
  if (rp->next)
    rp->next->previous = rp->previous;
}

/* Remove request structure rp from all lists and hash tables and
 * then delete and free the structure
 */
static void unlinkresolve(struct resolve *rp)
{

  untieresolve(rp);             /* Not really needed. Left in to be on the
                                 * safe side. */
  unlinkresolveid(rp);
#ifdef IPV6
  if (rp->sockname.family == AF_INET6)
    unlinkresolveip6(rp);
  else
#endif
  unlinkresolveip(rp);
  if (rp->hostn)
    unlinkresolvehost(rp);
  nfree(rp);
}

/* Find request structure using the id.
 */
static struct resolve *findid(uint16_t id)
{
  struct resolve *rp;
  int bashnum;

  bashnum = getidbash(id);
  rp = idbash[bashnum];
  if (rp) {
    while ((rp->nextid) && (id >= rp->nextid->id))
      rp = rp->nextid;
    while ((rp->previousid) && (id <= rp->previousid->id))
      rp = rp->previousid;
    if (id == rp->id) {
      idbash[bashnum] = rp;
      return rp;
    } else
      return NULL;
  }
  return rp;                    /* NULL */
}

/* Find request structure using the host.
 */
static struct resolve *findhost(char *hostn)
{
  struct resolve *rp;
  int bashnum;

  bashnum = gethostbash(hostn);
  rp = hostbash[bashnum];
  if (rp) {
    while ((rp->nexthost) &&
          (strcasecmp(hostn, rp->nexthost->hostn) >= 0))
      rp = rp->nexthost;
    while ((rp->previoushost) &&
           (strcasecmp(hostn, rp->previoushost->hostn) <= 0))
      rp = rp->previoushost;
    if (strcasecmp(hostn, rp->hostn))
      return NULL;
    else {
      hostbash[bashnum] = rp;
      return rp;
    }
  }
  return rp;                    /* NULL */
}

/* Find request structure using the ip.
 */
#ifdef IPV6
static struct resolve *findip6(struct in6_addr *ip6)
{
  struct resolve *rp;
  unsigned long bashnum;
  bashnum = getip6bash(ip6);
  rp = ip6bash[bashnum];
  if (rp){
    while ((rp->nextip) && (ip6->s6_addr[15] >=
           (rp->nextip->sockname.addr.s6.sin6_addr.s6_addr[15])))
      rp = rp->nextip;
    while ((rp->previousip) && (ip6->s6_addr[15] <=
           (rp->previousip->sockname.addr.s6.sin6_addr.s6_addr[15])))
      rp = rp->previousip;
    if (IN6_ARE_ADDR_EQUAL(ip6, &rp->sockname.addr.s6.sin6_addr)) {
      ip6bash[bashnum] = rp;
      return rp;
    } else return NULL;
  }
  return rp; /* NULL */
}
#endif

static struct resolve *findip(IP ip)
{
  struct resolve *rp;
  uint32_t bashnum;
  bashnum = getipbash(ip);
  rp = ipbash[bashnum];
  if (rp) {
    while ((rp->nextip) && (ip >= rp->nextip->ip))
      rp = rp->nextip;
    while ((rp->previousip) && (ip <= rp->previousip->ip))
      rp = rp->previousip;
    if (ip == rp->ip) {
      ipbash[bashnum] = rp;
      return rp;
    } else
      return NULL;
  }
  return rp;                    /* NULL */
}

static void ptrstring4(IP *ip, char *buf, size_t sz)
{
  egg_snprintf(buf, sz, "%u.%u.%u.%u.in-addr.arpa",
           ((uint8_t *) ip)[3],
           ((uint8_t *) ip)[2],
           ((uint8_t *) ip)[1],
           ((uint8_t *) ip)[0]);
}

#ifdef IPV6
static void ptrstring6(struct in6_addr *ip6, char *buf, size_t sz)
{
  int i;
  char *p, *q;
  const char *hex = "0123456789abcdef";

  p = buf;
  q = buf + sz;
  for (i = sizeof(struct in6_addr) - 1; i >= 0 && p < q; i--) {
     *p++ = hex[ip6->s6_addr[i] & 0x0f];
     *p++ = '.';
     *p++ = hex[(ip6->s6_addr[i] >> 4) & 0x0f];
     *p++ = '.';
  }
  strcpy(p, "ip6.arpa"); /* ip6.int is deprecated */
}
#endif

static void ptrstring(struct sockaddr *addr, char *buf, size_t sz)
{
  if (addr->sa_family == AF_INET)
    ptrstring4((IP *) &((struct sockaddr_in *)addr)->sin_addr.s_addr,
               buf, sz);
#ifdef IPV6
  else if (IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)addr)->sin6_addr))
    ptrstring4((IP *) &((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr[12],
               buf, sz);
  else ptrstring6(&((struct sockaddr_in6 *)addr)->sin6_addr, buf, sz);
#endif
}

/*
 *    Network and resolver related functions
 */

/* Create packet for the request and send it to all available nameservers.
 */
static void dorequest(char *s, int type, uint16_t id)
{
  packetheader *hp;
  int r, i;
  uint8_t *buf;

  /* Use malloc here instead of a static buffer, as per res_mkquery()'s manual
   * buf should be aligned on an eight byte boundary. malloc() should return a
   * pointer to an address properly aligned for any data type. Failing to
   * provide a aligned buffer will result in a SIGBUS crash at least on SPARC
   * CPUs.
   */
  buf = nmalloc(MAX_PACKETSIZE + 1);
  r = RES_MKQUERY(QUERY, s, C_IN, type, NULL, 0, NULL, buf, MAX_PACKETSIZE);
  if (r == -1) {
    nfree(buf);
    ddebug0(RES_ERR "Query too large.");
    return;
  }
  hp = (packetheader *) buf;
  hp->id = id;                  /* htons() deliberately left out (redundant) */
  for (i = 0; i < myres.nscount; i++)
    (void) sendto(resfd, buf, r, 0,
                  (struct sockaddr *) &myres.nsaddr_list[i],
                  sizeof(struct sockaddr));
  nfree(buf);
}

/* (Re-)send request with existing id.
 */
static void resendrequest(struct resolve *rp, int type)
{
  rp->sends++;
  /* Update expire time */
  rp->expiretime = now + (dns_retrydelay * rp->sends);
  /* Add (back) to expire list */
  linkresolve(rp);

  if (type == T_A) {
#ifdef IPV6
    dorequest(rp->hostn, T_AAAA, rp->id);
#endif
    dorequest(rp->hostn, type, rp->id);
    ddebug1(RES_MSG "Sent domain lookup request for \"%s\".", rp->hostn);
  } else if (type == T_PTR) {
    ptrstring(&rp->sockname.addr.sa, tempstring, sizeof tempstring);
    dorequest(tempstring, type, rp->id);
    ddebug1(RES_MSG "Sent domain lookup request for \"%s\".",
            iptostr(&rp->sockname.addr.sa));
  }
}

/* Send request for the first time.
 */
static void sendrequest(struct resolve *rp, int type)
{
  /* Create unique id */
  do {
    idseed = (((idseed + idseed) | (long) time(NULL))
              + idseed - 0x54bad4a) ^ aseed;
    aseed ^= idseed;
    rp->id = (uint16_t) idseed;
  } while (findid(rp->id));
  linkresolveid(rp);            /* Add id to id hash table */
  resendrequest(rp, type);      /* Send request */
}

/* Gets called as soon as the request turns out to have failed. Calls
 * the eggdrop hook */
static void failrp(struct resolve *rp, int type)
{
  if (rp->state == STATE_FINISHED)
    return;
  rp->expiretime = now + dns_negcache;
  rp->state = STATE_FAILED;

  /* Expire time was changed, reinsert entry to maintain order */
  untieresolve(rp);
  linkresolve(rp);

  ddebug0(RES_MSG "Lookup failed.");
  dns_event_failure(rp, type);
}

/* Gets called as soon as the request turns out to be successful. Calls
 * the eggdrop hook.
 */
static void passrp(struct resolve *rp, long ttl, int type)
{
  rp->state = STATE_FINISHED;

  /* Do not cache entries for too long. */
  if (ttl < dns_cache)
    rp->expiretime = now + (time_t) ttl;
  else
    rp->expiretime = now + dns_cache;

  /* Expire time was changed, reinsert entry to maintain order */
  untieresolve(rp);
  linkresolve(rp);

  ddebug1(RES_MSG "Lookup successful: %s", rp->hostn);
  dns_event_success(rp, type);
}

/* Parses the response packets received.
 */
static void parserespacket(uint8_t *response, int len)
{
#ifdef IPV6
  int ready = 0;
#endif
  int r, rcount;
  packetheader *hdr;
  struct resolve *rp;
  uint8_t rc, *c = response;
  uint16_t qdatatype, qclass, datatype, class, datalength;
  uint32_t ttl;

  if (len < sizeof(packetheader)) {
    debug1(RES_ERR "Packet smaller than standard header size: %d bytes.", len);
    return;
  }
  hdr = ((packetheader *) response);
  rp = findid(hdr->id);
  if (!rp || (rp->state == STATE_FINISHED) || (rp->state == STATE_FAILED))
    return;
  if (getheader_tc(hdr)) {
    ddebug0(RES_ERR "Nameserver packet truncated.");
    return;
  }
  if (!getheader_qr(hdr)) {
    ddebug0(RES_ERR "Query packet received on nameserver communication "
            "socket.");
    return;
  }
  if (getheader_opcode(hdr)) {
    ddebug0(RES_ERR "Invalid opcode in response packet.");
    return;
  }
  rc = getheader_rcode(hdr);
  switch (rc) {
    case NOERROR:
      break;
    case NXDOMAIN:
      ddebug0(RES_MSG "Domain does not exist.");
      failrp(rp, IS_A(rp) ? T_A : T_PTR);
      return;
    default:
      ddebug2(RES_ERR "Received error response %u. (%s)",
              rc, rc < RESPONSECODES_COUNT ?
              responsecodes[rc] : responsecodes[RESPONSECODES_COUNT]);
      return;
  }
  hdr->qdcount = ntohs(hdr->qdcount);
  hdr->ancount = ntohs(hdr->ancount);
  hdr->nscount = ntohs(hdr->nscount);
  hdr->arcount = ntohs(hdr->arcount);
  ddebug4(RES_MSG "Received nameserver reply. (qd:%u an:%u ns:%u ar:%u)",
          hdr->qdcount, hdr->ancount, hdr->nscount, hdr->arcount);
  if (hdr->qdcount != 1) {
    ddebug0(RES_ERR "Reply does not contain one query.");
    return;
  }
  switch (rp->state) {
    case STATE_PTRREQ:
      if (!hdr->ancount) {
        ddebug0(RES_ERR "No error returned but no answers given.");
        return;
      }
      ptrstring(&rp->sockname.addr.sa, stackstring, sizeof stackstring);
      break;
    case STATE_AREQ:
      strlcpy(stackstring, rp->hostn, sizeof stackstring);
  }
  c = response + HFIXEDSZ;
  r = dn_expand(response, response + len, c, namestring, MAXDNAME);
  if (r == -1) {
    ddebug0(RES_ERR "dn_expand() failed while expanding query domain.");
    return;
  }
  if (strcasecmp(stackstring, namestring)) {
    ddebug2(RES_MSG "Unknown query packet dropped. (\"%s\" does not "
            "match \"%s\")", stackstring, namestring);
    return;
  }
  c += r;
  if (c + 4 > response + len) {
    ddebug0(RES_ERR "Query resource record truncated.");
    return;
  }
  NS_GET16(qdatatype, c);
  NS_GET16(qclass, c);
  if (qclass != C_IN) {
    ddebug2(RES_ERR "Received unsupported query class: %u (%s)",
            qclass, (qclass < CLASSTYPES_COUNT) ?
            classtypes[qclass] : classtypes[CLASSTYPES_COUNT]);
    return;
  }
  switch (qdatatype) {
    case T_A:
      if (!IS_A(rp)) {
        ddebug0(RES_WRN "Ignoring response with unexpected query type \"A\".");
        return;
      }
#ifndef IPV6
      rp->sockname.family = AF_INET;
      break;
#else
      if (rp->sockname.family == AF_INET6)
        ready = 1;
      rp->sockname.family = AF_INET;
      break;
    case T_AAAA:
      if (!IS_A(rp)) {
        ddebug0(RES_WRN "Ignoring response with unexpected query type "
                "\"AAAA\".");
        return;
      }
      if (rp->sockname.family == AF_INET)
        ready = 1;
      rp->sockname.family = AF_INET6;
      break;
#endif
    case T_PTR:
      if (!IS_PTR(rp)) {
        ddebug0(RES_WRN "Ignoring response with unexpected query type "
                "\"PTR\".");
        return;
      }
      break;
    default:
      ddebug2(RES_ERR "Received unimplemented query type: %u (%s)",
              qdatatype, (qdatatype < RESOURCETYPES_COUNT) ?
              resourcetypes[qdatatype] : resourcetypes[RESOURCETYPES_COUNT]);
      return;
  }
  for (rcount = hdr->ancount + hdr->nscount + hdr->arcount; rcount; rcount--) {
    r = dn_expand(response, response + len, c, namestring, MAXDNAME);
    if (r == -1) {
      ddebug0(RES_ERR "dn_expand() failed while expanding answer domain.");
      return;
    }
    ddebug1(RES_MSG "answered domain query: \"%s\"", namestring);
    c += r;
    if (c + 10 > response + len) {
      ddebug0(RES_ERR "Resource record truncated.");
      return;
    }
    NS_GET16(datatype, c);
    NS_GET16(class, c);
    NS_GET32(ttl, c);
    NS_GET16(datalength, c);
    if (class != qclass) {
      ddebug2(RES_ERR "Answered class (%s) does not match queried class (%s).",
              (class < CLASSTYPES_COUNT) ?
              classtypes[class] : classtypes[CLASSTYPES_COUNT],
              (qclass < CLASSTYPES_COUNT) ?
              classtypes[qclass] : classtypes[CLASSTYPES_COUNT]);
      return;
    }
    c += datalength;
    if (c > response + len) {
      ddebug0(RES_ERR "Specified rdata length exceeds packet size.");
      return;
    }
    if (strcasecmp(stackstring, namestring))
      continue;
    if (datatype != qdatatype && datatype != T_CNAME) {
      ddebug2(RES_MSG "Ignoring resource type %u. (%s)",
              datatype, (datatype < RESOURCETYPES_COUNT) ?
              resourcetypes[datatype] :
              resourcetypes[RESOURCETYPES_COUNT]);
      continue;
    }
    ddebug1(RES_MSG "TTL: %s", strtdiff(sendstring, ttl));
    ddebug1(RES_MSG "TYPE: %s", (datatype < RESOURCETYPES_COUNT) ?
            resourcetypes[datatype] : resourcetypes[RESOURCETYPES_COUNT]);
    switch (datatype) {
      case T_A:
        if (datalength != 4) {
          ddebug1(RES_ERR "Unsupported rdata format for \"A\" type. "
                  "(%u bytes)", datalength);
          return;
        }
        rp->ttl = ttl;
        rp->sockname.addrlen = sizeof(struct sockaddr_in);
        rp->sockname.addr.sa.sa_family = AF_INET;
        memcpy(&rp->sockname.addr.s4.sin_addr, c, 4);
#ifndef IPV6
        passrp(rp, ttl, T_A);
        return;
#else
        if (ready || !pref_af) {
          passrp(rp, ttl, T_A);
          return;
        }
        break;
      case T_AAAA:
        if (datalength != 16) {
          ddebug1(RES_ERR "Unsupported rdata format for \"AAAA\" type. "
                  "(%u bytes)", datalength);
          return;
        }
        rp->ttl = ttl;
        rp->sockname.addrlen = sizeof(struct sockaddr_in6);
        rp->sockname.addr.sa.sa_family = AF_INET6;
        memcpy(&rp->sockname.addr.s6.sin6_addr, c, 16);
        if (ready || pref_af) {
          passrp(rp, ttl, T_A);
          return;
        }
        break;
#endif
      case T_PTR:
        r = dn_expand(response, response + len, c, namestring, MAXDNAME);
        if (r == -1) {
          ddebug0(RES_ERR "dn_expand() failed while expanding domain in "
                  "rdata.");
          return;
        }
        ddebug1(RES_MSG "Answered domain: \"%s\"", namestring);
        if (r > HOSTNAMELEN) {
          ddebug0(RES_ERR "Domain name too long.");
          failrp(rp, T_PTR);
          return;
        }
        if (!rp->hostn) {
          rp->hostn = nmalloc(strlen(namestring) + 1);
          strcpy(rp->hostn, namestring);
          linkresolvehost(rp);
          passrp(rp, ttl, T_PTR);
          return;
        }
        break;
      case T_CNAME:
        r = dn_expand(response, response + len, c, namestring, MAXDNAME);
        if (r == -1) {
          ddebug0(RES_ERR "dn_expand() failed while expanding domain in "
                  "rdata.");
          return;
        }
        ddebug1(RES_MSG "answered domain is CNAME for: %s", namestring);
        strlcpy(stackstring, namestring, sizeof stackstring);
        break;
      default:
        ddebug2(RES_ERR "Received unimplemented data type: %u (%s)",
                datatype, (datatype < RESOURCETYPES_COUNT) ?
                resourcetypes[datatype] :
                resourcetypes[RESOURCETYPES_COUNT]);
    }
  }
#ifdef IPV6
  if (IS_A(rp) && ready) {
#else
  if (IS_A(rp)) {
#endif
    rp->sockname.family = rp->sockname.addr.sa.sa_family;
    if (rp->sockname.addrlen)
      passrp(rp, rp->ttl, T_A);
    else
      failrp(rp, T_A);
  }
}

/* Read data received on our dns socket. This function is called
 * as soon as traffic is detected.
 */
static void dns_ack(void)
{
  struct sockaddr_in from;
  socklen_t fromlen = sizeof(struct sockaddr_in);
  int r, i;

  r = recvfrom(resfd, (uint8_t *) resrecvbuf, MAX_PACKETSIZE, 0,
               (struct sockaddr *) &from, &fromlen);
  if (r <= 0) {
    ddebug1(RES_MSG "Socket error: %s", strerror(errno));
    return;
  }
  /* Check to see if this server is actually one we sent to */
  if (from.sin_addr.s_addr == localhost) {
    for (i = 0; i < myres.nscount; i++)
      /* 0.0.0.0 replies as 127.0.0.1 */
      if ((myres.nsaddr_list[i].sin_addr.s_addr == from.sin_addr.s_addr) ||
          (!myres.nsaddr_list[i].sin_addr.s_addr))
        break;
  } else {
    for (i = 0; i < myres.nscount; i++)
      if (myres.nsaddr_list[i].sin_addr.s_addr == from.sin_addr.s_addr)
        break;
  }
  if (i == myres.nscount)
    ddebug1(RES_ERR "Received reply from unknown source: %s",
                iptostr((struct sockaddr *) &from));
  else
    parserespacket((uint8_t *) resrecvbuf, r);
}

/* Remove or resend expired requests. Called once a second.
 */
static void dns_check_expires(void)
{
  struct resolve *rp, *nextrp;

  /* Walk through sorted list ... */
  for (rp = expireresolves; (rp) && (now >= rp->expiretime); rp = nextrp) {
    nextrp = rp->next;
    untieresolve(rp);
    switch (rp->state) {
    case STATE_FINISHED:       /* TTL has expired */
    case STATE_FAILED:         /* Fake TTL has expired */
      ddebug4(RES_MSG "Cache record for \"%s\" (%s) has expired. (state: %u) "
              "Marked for expire at: %ld.", nonull(rp->hostn),
              iptostr(&rp->sockname.addr.sa), rp->state, rp->expiretime);
      unlinkresolve(rp);
      break;
    case STATE_PTRREQ:         /* T_PTR send timed out */
      if (rp->sends <= dns_maxsends) {
        ddebug1(RES_MSG "Resend #%d for \"PTR\" query...", rp->sends - 1);
        resendrequest(rp, T_PTR);
      } else {
        ddebug0(RES_MSG "\"PTR\" query timed out.");
        failrp(rp, T_PTR);
      }
      break;
    case STATE_AREQ:           /* T_A send timed out */
      if (rp->sends <= dns_maxsends) {
        ddebug1(RES_MSG "Resend #%d for \"A\" query...", rp->sends - 1);
        resendrequest(rp, T_A);
      } else {
        ddebug0(RES_MSG "\"A\" query timed out.");
        failrp(rp, T_A);
      }
      break;
    default:                   /* Unknown state, let it expire */
      ddebug1(RES_WRN "Unknown request state %d. Request expired.", rp->state);
      failrp(rp, 0);
    }
  }
}

/* Start searching for a host-name, using it's ip-address.
 */
static void dns_lookup(sockname_t *addr)
{
  struct resolve *rp = NULL;

  if (addr->family == AF_INET)
    rp = findip((IP)addr->addr.s4.sin_addr.s_addr);
#ifdef IPV6
  else
    rp = findip6(&addr->addr.s6.sin6_addr);
#endif
  if (rp) {
    if (rp->state == STATE_FINISHED || rp->state == STATE_FAILED) {
      if (rp->state == STATE_FINISHED && rp->hostn) {
        ddebug2(RES_MSG "Used cached record: %s == \"%s\".",
                iptostr(&addr->addr.sa), rp->hostn);
        dns_event_success(rp, T_PTR);
      } else {
        ddebug1(RES_MSG "Used failed record: %s == ???",
                iptostr(&addr->addr.sa));
        dns_event_failure(rp, T_PTR);
      }
    }
    return;
  }

  ddebug0(RES_MSG "Creating new record");
  rp = allocresolve();
  rp->state = STATE_PTRREQ;
  rp->sends = 1;
  rp->type = T_PTR;
  memcpy(&rp->sockname, addr, sizeof(sockname_t));
  if (addr->family == AF_INET) {
    rp->ip = addr->addr.s4.sin_addr.s_addr;
    linkresolveip(rp);
  }
#ifdef IPV6
  else
    linkresolveip6(rp);
#endif
  sendrequest(rp, T_PTR);
}

/* Read /etc/hosts */
static int dns_hosts(char *hostn) {
  #define PATH "/etc/hosts"
  size_t hostn_len;
  int i;
  char hostn_lower[256], hostn_upper[256], line[8 * 1024], *p1, *p2, *p3, *p4;
  FILE *hostf;
  sockname_t name;
#ifdef IPV6
  int af, fallback_set = 0;
  char fallback_ip[sizeof line];
#endif

  if (!*hostn) {
    ddebug0(RES_MSG "bogus empty hostname input");
    return 1;
  }
  hostn_len = strlen(hostn);
  if (hostn_len > 255) {
    ddebug0(RES_MSG "bogus len of hostname > 255 input");
    return 1;
  }
  /* precalculate lower and upper string from hostn and compare with handcrafted
   * code, due to strncasecmp() and strncmp() being slow if used in loop */
  for (i = 0; i < hostn_len; i++) {
      /* while at it, reject hostnames with bogus chars, see rfc 952, 1123 and 2181 */
      if (!strchr("-.0123456789ABCDEFGHIJABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz", hostn[i])) {
        ddebug0(RES_MSG "bogus char in hostname input");
        return 1;
      }
      hostn_lower[i] = tolower((unsigned char) hostn[i]);
      hostn_upper[i] = toupper((unsigned char) hostn[i]);
  }
  hostn_lower[i] = 0;
  hostn_upper[i] = 0;
  if (!(hostf = fopen(PATH, "r"))) {
    ddebug1(RES_MSG "fopen(" PATH "): %s", strerror(errno));
    return 0;
  }
  /* p1 is used for finding ip
   * p2 is used for finding hostname
   * p3 and p4 are used to compare against hostn_lower and hostn_upper
   */
  while ((p1 = fgets(line, sizeof line, hostf))) {
    /* postel's law */
    while ((*p1 == ' ') || (*p1 == '\t'))
      p1++;
    p2 = p1;
    while ((*p2 != '\n') && (*p2 != '#') && (*p2 != '\0')) {
      if ((*p2 == ' ') || (*p2 == '\t')) {
        *p2++ = '\0'; /* null-terminate ip */
        /* skip tab and space */
        while ((*p2 == ' ') || (*p2 == '\t'))
          p2++;
        /* if (!strncasecmp(c, hostn, hostn_len)) { */
        p3 = hostn_lower;
        p4 = hostn_upper;
        while ((*p2 == *p3) || (*p2 == *p4) ||
               ((*p3 == '\0') && ((*p2 == '\n') || (*p2 == ' ') || (*p2 == '\t') || (*p2 == '#')))) {
          if (*p3 == '\0') {
            /* string compare was successful, found hostname */
#ifdef IPV6
            if ((af = setsockname(&name, p1, 0, 0)) == (pref_af ? AF_INET6 : AF_INET)) {
#else
            if ((strchr(p1, '.')) && (setsockname(&name, p1, 0, 0) != AF_UNSPEC)) {
#endif
              call_ipbyhost(hostn, &name, 1);
              ddebug2(RES_MSG "Used /etc/hosts: %s == %s", hostn, p1);
              fclose(hostf);
              return 1;
            }
#ifdef IPV6
            else if ((af != AF_UNSPEC) && !fallback_set) {
              fallback_set = 1;
              strlcpy(fallback_ip, line, sizeof fallback_ip);
            }
#endif
            p2++;
            break;
          }
          p2++;
          p3++;
          p4++;
        }
      }
      p2++;
    }
  }
  if (ferror(hostf)) {
    ddebug0(RES_MSG "fgets(" PATH ")");
    return 0;
  }
#ifdef IPV6
  if (fallback_set) {
    call_ipbyhost(hostn, &name, 1);
    ddebug2(RES_MSG "Used /etc/hosts: %s == %s", hostn, fallback_ip);
    fclose(hostf);
    return 1;
  }
#endif
  fclose(hostf);
  return 0;
}

/* Start searching for an ip-address, using it's host-name.
 */
static void dns_forward(char *hostn)
{
  struct resolve *rp;
  sockname_t name;

  if (setsockname(&name, hostn, 0, 0) != AF_UNSPEC) {
    call_ipbyhost(hostn, &name, 1);
    return;
  }
  if ((rp = findhost(hostn))) {
    if (rp->state == STATE_FINISHED || rp->state == STATE_FAILED) {
      if (rp->state == STATE_FINISHED) {
        ddebug2(RES_MSG "Used cached record: %s == \"%s\".", hostn,
                iptostr(&rp->sockname.addr.sa));
        dns_event_success(rp, T_A);
      } else {
        ddebug1(RES_MSG "Used failed record: %s == ???", hostn);
        dns_event_failure(rp, T_A);
      }
    }
    return;
  }
  if (dns_hosts(hostn))
    return;
  ddebug0(RES_MSG "Creating new record");
  rp = allocresolve();
  rp->state = STATE_AREQ;
  rp->sends = 1;
  rp->hostn = nmalloc(strlen(hostn) + 1);
  strcpy(rp->hostn, hostn);
  rp->type = T_A;
  linkresolvehost(rp);
  sendrequest(rp, T_A);
}

/* Initialise the network.
 */
static int init_dns_network(void)
{
  int option = 1;

  resfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (resfd == -1) {
    putlog(LOG_MISC, "*",
           "Unable to allocate socket for nameserver communication: %s",
           strerror(errno));
    return 0;
  }
  if (allocsock(resfd, SOCK_PASS) == -1) {
    putlog(LOG_MISC, "*",
           "Unable to allocate socket in socklist for nameserver communication");
    killsock(resfd);
    return 0;
  }
  if (setsockopt(resfd, SOL_SOCKET, SO_BROADCAST, &option, sizeof option)) {
    if (errno != ENOSYS) {
      putlog(LOG_MISC, "*",
             "Unable to setsockopt() on nameserver communication socket: %s",
             strerror(errno));
      killsock(resfd);
      return 0;
    }
  }
  localhost = htonl(INADDR_LOOPBACK);
  return 1;
}

/* Initialise the core dns system, returns 1 if all goes well, 0 if not.
 */
static int init_dns_core(void)
{
  int i;

  /* Initialise the resolv library. */
  MY_RES_INIT();
  if (!myres.nscount) {
    putlog(LOG_MISC, "*", "No nameservers defined.");
    return 0;
  }
  myres.options |= RES_RECURSE | RES_DEFNAMES | RES_DNSRCH;
  for (i = 0; i < myres.nscount; i++)
    myres.nsaddr_list[i].sin_family = AF_INET;

  if (!init_dns_network())
    return 0;

  /* Initialise the hash tables. */
  aseed = time(NULL) ^ (time(NULL) << 3) ^ (uint32_t) getpid();
  for (i = 0; i < BASH_SIZE; i++) {
    idbash[i] = NULL;
    ipbash[i] = NULL;
    hostbash[i] = NULL;
  }
  expireresolves = NULL;
  return 1;
}
