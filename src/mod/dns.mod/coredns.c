/*
 * dnscore.c -- part of dns.mod
 *   This file contains all core functions needed for the eggdrop dns module.
 *   Many of them are only minimaly modified from the original source.
 *
 * Modified/written by Fabian Knittel <fknittel@gmx.de>
 *
 * $Id: coredns.c,v 1.37 2011/04/01 11:59:49 pseudo Exp $
 */
/*
 * Portions Copyright (C) 1999 - 2011 Eggheads Development Team
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
#include <resolv.h>
#include <errno.h>


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
#endif /* DEBUG_DNS */

#ifdef DEBUG_DNS
#  define RESOURCETYPES_COUNT 17
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
  "unknown type",
};
#endif /* DEBUG_DNS */

#ifdef DEBUG_DNS
#  define CLASSTYPES_COUNT 5
static const char *classtypes[CLASSTYPES_COUNT + 1] = {
  "unknown class",
  "IN: the Internet",
  "CS: CSNET (OBSOLETE)",
  "CH: CHAOS",
  "HS: Hesoid [Dyer 87]",
  "unknown class"
};
#endif /* DEBUG_DNS */

typedef struct {
  u_16bit_t id;                 /* Packet id */
  u_8bit_t databyte_a;
  /* rd:1                             recursion desired
   * tc:1                             truncated message
   * aa:1                             authoritive answer
   * opcode:4                         purpose of message
   * qr:1                             response flag
   */
  u_8bit_t databyte_b;
  /* rcode:4                          response code
   * unassigned:2                     unassigned bits
   * pr:1                             primary server required (non standard)
   * ra:1                             recursion available
   */
  u_16bit_t qdcount;            /* Query record count */
  u_16bit_t ancount;            /* Answer record count */
  u_16bit_t nscount;            /* Authority reference record count */
  u_16bit_t arcount;            /* Resource reference record count */
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

#define sucknetword(x)  ((x)+=2,((u_16bit_t)  (((x)[-2] <<  8) | ((x)[-1] <<  0))))
#define sucknetshort(x) ((x)+=2,((short) (((x)[-2] <<  8) | ((x)[-1] <<  0))))
#define sucknetdword(x) ((x)+=4,((dword) (((x)[-4] << 24) | ((x)[-3] << 16) | \
                                          ((x)[-2] <<  8) | ((x)[-1] <<  0))))
#define sucknetlong(x)  ((x)+=4,((long)  (((x)[-4] << 24) | ((x)[-3] << 16) | \
                                          ((x)[-2] <<  8) | ((x)[-1] <<  0))))


static u_32bit_t resrecvbuf[(MAX_PACKETSIZE + 7) >> 2]; /* MUST BE DWORD ALIGNED */

static struct resolve *idbash[BASH_SIZE];
static struct resolve *ipbash[BASH_SIZE];
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


/*
 *    Miscellaneous helper functions
 */

#ifdef DEBUG_DNS
/* Displays the time difference passed in signeddiff.
 */
static char *strtdiff(char *d, long signeddiff)
{
  u_32bit_t diff;
  u_32bit_t seconds, minutes, hours;
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
inline static u_32bit_t getidbash(u_16bit_t id)
{
  return (u_32bit_t) BASH_MODULO(id);
}

/* Return the hash bucket number for ip.
 */
inline static u_32bit_t getipbash(IP ip)
{
  return (u_32bit_t) BASH_MODULO(ip);
}

/* Return the hash bucket number for host.
 */
static u_32bit_t gethostbash(char *host)
{
  u_32bit_t bashvalue = 0;

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
  u_32bit_t bashnum;

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
  u_32bit_t bashnum;

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
  u_32bit_t bashnum;
  int ret;

  bashnum = gethostbash(addrp->hostn);
  rp = hostbash[bashnum];
  if (rp) {
    while ((rp->nexthost) &&
           (egg_strcasecmp(addrp->hostn, rp->nexthost->hostn) < 0))
      rp = rp->nexthost;
    while ((rp->previoushost) &&
           (egg_strcasecmp(addrp->hostn, rp->previoushost->hostn) > 0))
      rp = rp->previoushost;
    ret = egg_strcasecmp(addrp->hostn, rp->hostn);
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
  u_32bit_t bashnum;

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
  u_32bit_t bashnum;

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

/* Remove request structure rp from the ip hash table.
 */
static void unlinkresolveip(struct resolve *rp)
{
  u_32bit_t bashnum;

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

/* Remove reqeust structure rp from the expireresolves list.
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
  unlinkresolveip(rp);
  if (rp->hostn)
    unlinkresolvehost(rp);
  nfree(rp);
}

/* Find request structure using the id.
 */
static struct resolve *findid(u_16bit_t id)
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
          (egg_strcasecmp(hostn, rp->nexthost->hostn) >= 0))
      rp = rp->nexthost;
    while ((rp->previoushost) &&
           (egg_strcasecmp(hostn, rp->previoushost->hostn) <= 0))
      rp = rp->previoushost;
    if (egg_strcasecmp(hostn, rp->hostn))
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
static struct resolve *findip(IP ip)
{
  struct resolve *rp;
  u_32bit_t bashnum;

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


/*
 *    Network and resolver related functions
 */

/* Create packet for the request and send it to all available nameservers.
 */
static void dorequest(char *s, int type, u_16bit_t id)
{
  packetheader *hp;
  int r, i;
  u_8bit_t *buf;

  /* Use malloc here instead of a static buffer, as per res_mkquery()'s manual
   * buf should be aligned on an eight byte boundary. malloc() should return a
   * pointer to an address properly aligned for any data type. Failing to
   * provide a aligned buffer will result in a SIGBUS crash atleast on SPARC
   * CPUs.
   */
  buf = nmalloc(MAX_PACKETSIZE + 1);
  r = res_mkquery(QUERY, s, C_IN, type, NULL, 0, NULL, buf, MAX_PACKETSIZE);
  if (r == -1) {
    ddebug0(RES_ERR "Query too large.");
    return;
  }
  hp = (packetheader *) buf;
  hp->id = id;                  /* htons() deliberately left out (redundant) */
  for (i = 0; i < _res.nscount; i++)
    (void) sendto(resfd, buf, r, 0,
                  (struct sockaddr *) &_res.nsaddr_list[i],
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
    dorequest(rp->hostn, type, rp->id);
    ddebug1(RES_MSG "Sent domain lookup request for \"%s\".", rp->hostn);
  } else if (type == T_PTR) {
    sprintf(tempstring, "%u.%u.%u.%u.in-addr.arpa",
            ((u_8bit_t *) & rp->ip)[3],
            ((u_8bit_t *) & rp->ip)[2],
            ((u_8bit_t *) & rp->ip)[1], ((u_8bit_t *) & rp->ip)[0]);
    dorequest(tempstring, type, rp->id);
    ddebug1(RES_MSG "Sent domain lookup request for \"%s\".", iptostr(rp->ip));
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
    rp->id = (u_16bit_t) idseed;
  } while (findid(rp->id));
  linkresolveid(rp);            /* Add id to id hash table */
  resendrequest(rp, type);      /* Send request */
}

/* Gets called as soon as the request turns out to have failed. Calls
 * the eggdrop hook.
 */
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
static void parserespacket(u_8bit_t *s, int l)
{
  struct resolve *rp;
  packetheader *hp;
  u_8bit_t *eob;
  u_8bit_t *c;
  long ttl;
  int r, usefulanswer;
  u_16bit_t rr, datatype, class, qdatatype, qclass;
  u_8bit_t rdatalength;

  if (l < sizeof(packetheader)) {
    debug0(RES_ERR "Packet smaller than standard header size.");
    return;
  }
  if (l == sizeof(packetheader)) {
    debug0(RES_ERR "Packet has empty body.");
    return;
  }
  hp = (packetheader *) s;
  /* Convert data to host byte order
   *
   * hp->id does not need to be redundantly byte-order flipped, it
   * is only echoed by nameserver
   */
  rp = findid(hp->id);
  if (!rp)
    return;
  if ((rp->state == STATE_FINISHED) || (rp->state == STATE_FAILED))
    return;
  hp->qdcount = ntohs(hp->qdcount);
  hp->ancount = ntohs(hp->ancount);
  hp->nscount = ntohs(hp->nscount);
  hp->arcount = ntohs(hp->arcount);
  if (getheader_tc(hp)) {       /* Packet truncated */
    ddebug0(RES_ERR "Nameserver packet truncated.");
    return;
  }
  if (!getheader_qr(hp)) {      /* Not a reply */
    ddebug0(RES_ERR "Query packet received on nameserver communication "
            "socket.");
    return;
  }
  if (getheader_opcode(hp)) {   /* Not opcode 0 (standard query) */
    ddebug0(RES_ERR "Invalid opcode in response packet.");
    return;
  }
  eob = s + l;
  c = s + HFIXEDSZ;
  switch (getheader_rcode(hp)) {
  case NOERROR:
    if (hp->ancount) {
      ddebug4(RES_MSG "Received nameserver reply. (qd:%u an:%u ns:%u ar:%u)",
              hp->qdcount, hp->ancount, hp->nscount, hp->arcount);
      if (hp->qdcount != 1) {
        ddebug0(RES_ERR "Reply does not contain one query.");
        return;
      }
      if (c > eob) {
        ddebug0(RES_ERR "Reply too short.");
        return;
      }
      switch (rp->state) {      /* Construct expected query reply */
      case STATE_PTRREQ:
        sprintf(stackstring,
                "%u.%u.%u.%u.in-addr.arpa",
                ((u_8bit_t *) & rp->ip)[3],
                ((u_8bit_t *) & rp->ip)[2],
                ((u_8bit_t *) & rp->ip)[1], ((u_8bit_t *) & rp->ip)[0]);
        break;
      case STATE_AREQ:
        strncpy(stackstring, rp->hostn, 1024);
      }
      *namestring = '\0';
      r = dn_expand(s, s + l, c, namestring, MAXDNAME);
      if (r == -1) {
        ddebug0(RES_ERR "dn_expand() failed while expanding query domain.");
        return;
      }
      namestring[strlen(stackstring)] = '\0';
      if (egg_strcasecmp(stackstring, namestring)) {
        ddebug2(RES_MSG "Unknown query packet dropped. (\"%s\" does not "
                "match \"%s\")", stackstring, namestring);
        return;
      }
      ddebug1(RES_MSG "Queried domain name: \"%s\"", namestring);
      c += r;
      if (c + 4 > eob) {
        ddebug0(RES_ERR "Query resource record truncated.");
        return;
      }
      qdatatype = sucknetword(c);
      qclass = sucknetword(c);
      if (qclass != C_IN) {
        ddebug2(RES_ERR "Received unsupported query class: %u (%s)",
                qclass, (qclass < CLASSTYPES_COUNT) ?
                classtypes[qclass] : classtypes[CLASSTYPES_COUNT]);
      }
      switch (qdatatype) {
      case T_PTR:
        if (!IS_PTR(rp)) {
          ddebug0(RES_WRN "Ignoring response with unexpected query type "
                  "\"PTR\".");
          return;
        }
        break;
      case T_A:
        if (!IS_A(rp)) {
          ddebug0(RES_WRN "Ignoring response with unexpected query type "
                  "\"PTR\".");
          return;
        }
        break;
      default:
        ddebug2(RES_ERR "Received unimplemented query type: %u (%s)",
                qdatatype, (qdatatype < RESOURCETYPES_COUNT) ?
                resourcetypes[qdatatype] : resourcetypes[RESOURCETYPES_COUNT]);
      }
      for (rr = hp->ancount + hp->nscount + hp->arcount; rr; rr--) {
        if (c > eob) {
          ddebug0(RES_ERR "Packet does not contain all specified resouce "
                  "records.");
          return;
        }
        *namestring = '\0';
        r = dn_expand(s, s + l, c, namestring, MAXDNAME);
        if (r == -1) {
          ddebug0(RES_ERR "dn_expand() failed while expanding answer domain.");
          return;
        }
        namestring[strlen(stackstring)] = '\0';
        if (egg_strcasecmp(stackstring, namestring))
          usefulanswer = 0;
        else
          usefulanswer = 1;
        ddebug1(RES_MSG "answered domain query: \"%s\"", namestring);
        c += r;
        if (c + 10 > eob) {
          ddebug0(RES_ERR "Resource record truncated.");
          return;
        }
        datatype = sucknetword(c);
        class = sucknetword(c);
        ttl = sucknetlong(c);
        rdatalength = sucknetword(c);
        if (class != qclass) {
          ddebug2(RES_MSG "query class: %u (%s)",
                  qclass, (qclass < CLASSTYPES_COUNT) ?
                  classtypes[qclass] : classtypes[CLASSTYPES_COUNT]);
          ddebug2(RES_MSG "rr class: %u (%s)", class,
                  (class < CLASSTYPES_COUNT) ?
                  classtypes[class] : classtypes[CLASSTYPES_COUNT]);
          ddebug0(RES_ERR "Answered class does not match queried class.");
          return;
        }
        if (!rdatalength) {
          ddebug0(RES_ERR "Zero size rdata.");
          return;
        }
        if (c + rdatalength > eob) {
          ddebug0(RES_ERR "Specified rdata length exceeds packet size.");
          return;
        }
        if (datatype == qdatatype) {
          ddebug1(RES_MSG "TTL: %s", strtdiff(sendstring, ttl));
          ddebug1(RES_MSG "TYPE: %s", (datatype < RESOURCETYPES_COUNT) ?
                  resourcetypes[datatype] :
                  resourcetypes[RESOURCETYPES_COUNT]);
          if (usefulanswer)
            switch (datatype) {
            case T_A:
              if (rdatalength != 4) {
                ddebug1(RES_ERR "Unsupported rdata format for \"A\" type. "
                        "(%u bytes)", rdatalength);
                return;
              }
              my_memcpy(&rp->ip, (IP *) c, sizeof(IP));
              linkresolveip(rp);
              passrp(rp, ttl, T_A);
              return;
            case T_PTR:
              *namestring = '\0';
              r = dn_expand(s, s + l, c, namestring, MAXDNAME);
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
            default:
              ddebug2(RES_ERR "Received unimplemented data type: %u (%s)",
                      datatype, (datatype < RESOURCETYPES_COUNT) ?
                      resourcetypes[datatype] :
                      resourcetypes[RESOURCETYPES_COUNT]);
            }
        } else if (datatype == T_CNAME) {
          *namestring = '\0';
          r = dn_expand(s, s + l, c, namestring, MAXDNAME);
          if (r == -1) {
            ddebug0(RES_ERR "dn_expand() failed while expanding domain in "
                    "rdata.");
            return;
          }
          ddebug1(RES_MSG "answered domain is CNAME for: %s", namestring);
          /* The next responses will be related to the domain
           * pointed to by CNAME, so we need to update which
           * respones we regard as important.
           */
          strncpy(stackstring, namestring, 1024);
        } else {
          ddebug2(RES_MSG "Ignoring resource type %u. (%s)",
                  datatype, (datatype < RESOURCETYPES_COUNT) ?
                  resourcetypes[datatype] :
                  resourcetypes[RESOURCETYPES_COUNT]);
        }
        c += rdatalength;
      }
    } else
      ddebug0(RES_ERR "No error returned but no answers given.");
    break;
  case NXDOMAIN:
    ddebug0(RES_MSG "Host not found.");
    switch (rp->state) {
    case STATE_PTRREQ:
      failrp(rp, T_PTR);
      break;
    case STATE_AREQ:
      failrp(rp, T_A);
      break;
    default:
      failrp(rp, 0);
      break;
    }
    break;
  default:
    ddebug2(RES_MSG "Received error response %u. (%s)",
            getheader_rcode(hp), (getheader_rcode(hp) < RESPONSECODES_COUNT) ?
            responsecodes[getheader_rcode(hp)] :
            responsecodes[RESPONSECODES_COUNT]);
  }
}

/* Read data received on our dns socket. This function is called
 * as soon as traffic is detected.
 */
static void dns_ack(void)
{
  struct sockaddr_in from;
  unsigned int fromlen = sizeof(struct sockaddr_in);
  int r, i;

  r = recvfrom(resfd, (u_8bit_t *) resrecvbuf, MAX_PACKETSIZE, 0,
               (struct sockaddr *) &from, &fromlen);
  if (r <= 0) {
    ddebug1(RES_MSG "Socket error: %s", strerror(errno));
    return;
  }
  /* Check to see if this server is actually one we sent to */
  if (from.sin_addr.s_addr == localhost) {
    for (i = 0; i < _res.nscount; i++)
      /* 0.0.0.0 replies as 127.0.0.1 */
      if ((_res.nsaddr_list[i].sin_addr.s_addr == from.sin_addr.s_addr) ||
          (!_res.nsaddr_list[i].sin_addr.s_addr))
        break;
  } else {
    for (i = 0; i < _res.nscount; i++)
      if (_res.nsaddr_list[i].sin_addr.s_addr == from.sin_addr.s_addr)
        break;
  }
  if (i == _res.nscount) {
    ddebug1(RES_ERR "Received reply from unknown source: %s",
            iptostr(from.sin_addr.s_addr));
  } else
    parserespacket((u_8bit_t *) resrecvbuf, r);
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
      ddebug4(RES_MSG
              "Cache record for \"%s\" (%s) has expired. (state: %u)  Marked for expire at: %ld.",
              nonull(rp->hostn), iptostr(rp->ip), rp->state, rp->expiretime);
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
static void dns_lookup(IP ip)
{
  struct resolve *rp;

  ip = htonl(ip);
  if ((rp = findip(ip))) {
    if (rp->state == STATE_FINISHED || rp->state == STATE_FAILED) {
      if (rp->state == STATE_FINISHED && rp->hostn) {
        ddebug2(RES_MSG "Used cached record: %s == \"%s\".",
                iptostr(ip), rp->hostn);
        dns_event_success(rp, T_PTR);
      } else {
        ddebug1(RES_MSG "Used failed record: %s == ???", iptostr(ip));
        dns_event_failure(rp, T_PTR);
      }
    }
    return;
  }

  ddebug0(RES_MSG "Creating new record");
  rp = allocresolve();
  rp->state = STATE_PTRREQ;
  rp->sends = 1;
  rp->ip = ip;
  linkresolveip(rp);
  sendrequest(rp, T_PTR);
}

/* Start searching for an ip-address, using it's host-name.
 */
static void dns_forward(char *hostn)
{
  struct resolve *rp;
  struct in_addr inaddr;

  /* Check if someone passed us an IP address as hostname
   * and return it straight away.
   */
  if (egg_inet_aton(hostn, &inaddr)) {
    call_ipbyhost(hostn, ntohl(inaddr.s_addr), 1);
    return;
  }
  if ((rp = findhost(hostn))) {
    if (rp->state == STATE_FINISHED || rp->state == STATE_FAILED) {
      if (rp->state == STATE_FINISHED && rp->ip) {
        ddebug2(RES_MSG "Used cached record: %s == \"%s\".", hostn,
                iptostr(rp->ip));
        dns_event_success(rp, T_A);
      } else {
        ddebug1(RES_MSG "Used failed record: %s == ???", hostn);
        dns_event_failure(rp, T_A);
      }
    }
    return;
  }
  ddebug0(RES_MSG "Creating new record");
  rp = allocresolve();
  rp->state = STATE_AREQ;
  rp->sends = 1;
  rp->hostn = nmalloc(strlen(hostn) + 1);
  strcpy(rp->hostn, hostn);
  linkresolvehost(rp);
  sendrequest(rp, T_A);
}

/* Initialise the network.
 */
static int init_dns_network(void)
{
  int option;
  struct in_addr inaddr;

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
  option = 1;
  if (setsockopt(resfd, SOL_SOCKET, SO_BROADCAST, (char *) &option,
                 sizeof(option))) {
    putlog(LOG_MISC, "*",
           "Unable to setsockopt() on nameserver communication socket: %s",
           strerror(errno));
    killsock(resfd);
    return 0;
  }

  egg_inet_aton("127.0.0.1", &inaddr);
  localhost = inaddr.s_addr;
  return 1;
}

/* Initialise the core dns system, returns 1 if all goes well, 0 if not.
 */
static int init_dns_core(void)
{
  int i;

  /* Initialise the resolv library. */
  res_init();
  if (!_res.nscount)
    putlog(LOG_MISC, "*", "No nameservers found.");
  _res.options |= RES_RECURSE | RES_DEFNAMES | RES_DNSRCH;
  for (i = 0; i < _res.nscount; i++)
    _res.nsaddr_list[i].sin_family = AF_INET;

  if (!init_dns_network())
    return 0;

  /* Initialise the hash tables. */
  aseed = time(NULL) ^ (time(NULL) << 3) ^ (u_32bit_t) getpid();
  for (i = 0; i < BASH_SIZE; i++) {
    idbash[i] = NULL;
    ipbash[i] = NULL;
    hostbash[i] = NULL;
  }
  expireresolves = NULL;
  return 1;
}
