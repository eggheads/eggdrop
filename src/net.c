/*
 * net.c -- handles:
 *   all raw network i/o
 */
/*
 * This is hereby released into the public domain.
 * Robey Pointer, robey@netcom.com
 *
 * Changes after Feb 23, 1999 Copyright Eggheads Development Team
 *
 * Copyright (C) 1999 - 2019 Eggheads Development Team
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

#include <fcntl.h>
#include "main.h"
#include <limits.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#if HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#if HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <setjmp.h>

#ifdef TLS
#  include <openssl/err.h>
#endif

extern struct dcc_t *dcc;
extern int backgrd, use_stderr, resolve_timeout, dcc_total;
extern unsigned long otraffic_irc_today, otraffic_bn_today, otraffic_dcc_today,
                     otraffic_filesys_today, otraffic_trans_today,
                     otraffic_unknown_today;

char natip[121] = "";         /* Public IPv4 to report for systems behind NAT */
char listen_ip[121] = "";     /* IP (or hostname) for listening sockets       */
char vhost[121] = "";         /* IPv4 vhost for outgoing connections          */
#ifdef IPV6
char vhost6[121] = "";        /* IPv6 vhost for outgoing connections          */
int pref_af = 0;              /* Prefer IPv6 over IPv4?                       */
#endif
char firewall[121] = "";      /* Socks server for firewall.                   */
int firewallport = 1080;      /* Default port of socks 4/5 firewalls.         */
char botuser[11] = "eggdrop"; /* Username of the user running the bot.        */
int dcc_sanitycheck = 0;      /* Do some sanity checking on dcc connections.  */

sock_list *socklist = NULL;   /* Enough to be safe.                           */
sigjmp_buf alarmret;          /* Env buffer for alarm() returns.              */

/* Types of proxies */
#define PROXY_SOCKS   1
#define PROXY_SUN     2


/* I need an UNSIGNED long for dcc type stuff
 */
IP my_atoul(char *s)
{
  IP ret = 0;

  while ((*s >= '0') && (*s <= '9')) {
    ret *= 10;
    ret += ((*s) - '0');
    s++;
  }
  return ret;
}

int expmem_net()
{
  int i, tot = 0;
  struct threaddata *td = threaddata();

  for (i = 0; i < td->MAXSOCKS; i++) {
    if (!(td->socklist[i].flags & (SOCK_UNUSED | SOCK_TCL))) {
      if (td->socklist[i].handler.sock.inbuf != NULL)
        tot += strlen(td->socklist[i].handler.sock.inbuf) + 1;
      if (td->socklist[i].handler.sock.outbuf != NULL)
        tot += td->socklist[i].handler.sock.outbuflen;
    }
  }
  return tot;
}

/* Extract the IP address from a sockaddr struct and convert it
 * to presentation format.
 */
char *iptostr(struct sockaddr *sa)
{
#ifdef IPV6
  static char s[INET6_ADDRSTRLEN] = "";
  if (sa->sa_family == AF_INET6)
    inet_ntop(AF_INET6, &((struct sockaddr_in6 *)sa)->sin6_addr,
              s, sizeof s);
  else
#else
  static char s[INET_ADDRSTRLEN] = "";
#endif
    inet_ntop(AF_INET, &((struct sockaddr_in *)sa)->sin_addr.s_addr, s,
              sizeof s);
  return s;
}

/* Fills in a sockname struct with the given server and port. If the string
 * pointed by src isn't an IP address and allowres is not null, the function
 * will assume it's a hostname and will attempt to resolve it. This is
 * convenient, but you should use the async dns functions where possible, to
 * avoid blocking the bot while the lookup is performed.
 */
int setsockname(sockname_t *addr, char *src, int port, int allowres)
{
  struct hostent *hp;
  int af = AF_UNSPEC;
#ifdef IPV6
  int pref;

  /* Clean start */
  egg_bzero(addr, sizeof(sockname_t));
  af = pref = pref_af ? AF_INET6 : AF_INET;
  if (pref == AF_INET) {
    if (!egg_inet_aton(src, &addr->addr.s4.sin_addr))
      af = AF_INET6;
  } else {
    if (inet_pton(af, src, &addr->addr.s6.sin6_addr) != 1)
      af = AF_INET;
  }
  if (af != pref)
    if (((af == AF_INET6) &&
         (inet_pton(af, src, &addr->addr.s6.sin6_addr) != 1)) ||
        ((af == AF_INET)  &&
         !egg_inet_aton(src, &addr->addr.s4.sin_addr)))
      af = AF_UNSPEC;

  if (af == AF_UNSPEC && allowres && *src) {
    /* src is a hostname. Attempt to resolve it.. */
    if (!sigsetjmp(alarmret, 1)) {
      alarm(resolve_timeout);
      hp = gethostbyname2(src, pref_af ? AF_INET6 : AF_INET);
      if (!hp)
        hp = gethostbyname2(src, pref_af ? AF_INET : AF_INET6);
      alarm(0);
    } else
      hp = NULL;
    if (hp) {
      if (hp->h_addrtype == AF_INET)
        memcpy(&addr->addr.s4.sin_addr, hp->h_addr, hp->h_length);
      else
        memcpy(&addr->addr.s6.sin6_addr, hp->h_addr, hp->h_length);
      af = hp->h_addrtype;
    }
  }

  addr->family = (af == AF_UNSPEC) ? pref : af;
  addr->addr.sa.sa_family = addr->family;
  if (addr->family == AF_INET6) {
    addr->addrlen = sizeof(struct sockaddr_in6);
    addr->addr.s6.sin6_port = htons(port);
    addr->addr.s6.sin6_family = AF_INET6;
  } else {
    addr->addrlen = sizeof(struct sockaddr_in);
    addr->addr.s4.sin_port = htons(port);
    addr->addr.s4.sin_family = AF_INET;
  }
#else
  int i, count;

  egg_bzero(addr, sizeof(sockname_t));

/* If it's not an IPv4 address, check if its IPv6 (so it can fail/error
 * appropriately). If it's not, and allowres is 1, use gethostbyname()
 * to try and resolve. If allowres is 0, return AF_UNSPEC to allow
 * dns.mod to do it's thing.

 * Also, because we can't be sure inet_pton exists on the system, we
 * have to resort to hackishly counting :s to see if its IPv6 or not.
 * Go internet.
 */
  if (!inet_pton(AF_INET, src, &addr->addr.s4.sin_addr)) {
    /* Boring way to count :s */
    count = 0;
    for (i = 0; src[i]; i++) {
      if (src[i] == ':') {
        count++;
        if (count == 2)
          break;
      }
    }
    if (count > 1) {
      putlog(LOG_MISC, "*", "ERROR: This looks like an IPv6 address, \
but this Eggdrop was not compiled with IPv6 support.");
      af = AF_UNSPEC;
    }
    else if (allowres) {
    /* src is a hostname. Attempt to resolve it.. */
      if (!sigsetjmp(alarmret, 1)) {
        alarm(resolve_timeout);
        hp = gethostbyname(src);
        alarm(0);
      } else
        hp = NULL;
      if (hp) {
        memcpy(&addr->addr.s4.sin_addr, hp->h_addr, hp->h_length);
        af = hp->h_addrtype;
      }
    } else
        af = AF_UNSPEC;
  } else
      af = AF_INET;

  addr->family = addr->addr.s4.sin_family = AF_INET;
  addr->addr.sa.sa_family = addr->family;
  addr->addrlen = sizeof(struct sockaddr_in);
  addr->addr.s4.sin_port = htons(port);
#endif
  return af;
}

/* Get socket address to bind to for outbound connections
 */
void getvhost(sockname_t *addr, int af)
{
  char *h = NULL;

  if (af == AF_INET)
    h = vhost;
#ifdef IPV6
  else
    h = vhost6;
#endif
  if (setsockname(addr, (h ? h : ""), 0, 1) != af)
    setsockname(addr, (af == AF_INET ? "0" : "::"), 0, 0);
  /* Remember this 'self-lookup failed' thingie?
     I have good news - you won't see it again ;) */
}

/* Sets/Unsets options for a specific socket.
 *
 * Returns:  0   - on success
 *           -1  - socket not found
 *           -2  - illegal operation
 */
int sockoptions(int sock, int operation, int sock_options)
{
  int i;
  struct threaddata *td = threaddata();

  for (i = 0; i < td->MAXSOCKS; i++)
    if ((td->socklist[i].sock == sock) &&
        !(td->socklist[i].flags & SOCK_UNUSED)) {
      if (operation == EGG_OPTION_SET)
        td->socklist[i].flags |= sock_options;
      else if (operation == EGG_OPTION_UNSET)
        td->socklist[i].flags &= ~sock_options;
      else
        return -2;
      return 0;
    }
  return -1;
}

/* Return a free entry in the socket entry
 */
int allocsock(int sock, int options)
{
  int i;
  struct threaddata *td = threaddata();

  for (i = 0; i < td->MAXSOCKS; i++) {
    if (td->socklist[i].flags & SOCK_UNUSED) {
      /* yay!  there is table space */
      td->socklist[i].handler.sock.inbuf = NULL;
      td->socklist[i].handler.sock.outbuf = NULL;
      td->socklist[i].handler.sock.inbuflen = 0;
      td->socklist[i].handler.sock.outbuflen = 0;
      td->socklist[i].flags = options;
      td->socklist[i].sock = sock;
#ifdef TLS
      td->socklist[i].ssl = 0;
#endif
      return i;
    }
  }
  /* Try again if enlarging socketlist works */
  if (increase_socks_max())
    return -1;
  else
    return allocsock(sock, options);
}

/* Return a free entry in the socket entry for a tcl socket
 *
 * alloctclsock() can be called by Tcl threads
 */
int alloctclsock(int sock, int mask, Tcl_FileProc *proc, ClientData cd)
{
  int f = -1;
  int i;
  struct threaddata *td = threaddata();

  for (i = 0; i < td->MAXSOCKS; i++) {
    if (td->socklist[i].flags & SOCK_UNUSED) {
      if (f == -1)
        f = i;
    } else if ((td->socklist[i].flags & SOCK_TCL) &&
               td->socklist[i].sock == sock) {
      f = i;
      break;
    }
  }
  if (f != -1) {
    td->socklist[f].sock = sock;
    td->socklist[f].flags = SOCK_TCL;
    td->socklist[f].handler.tclsock.mask = mask;
    td->socklist[f].handler.tclsock.proc = proc;
    td->socklist[f].handler.tclsock.cd = cd;
    return f;
  }
  /* Try again if enlarging socketlist works */
  if (increase_socks_max())
    return -1;
  else
    return alloctclsock(sock, mask, proc, cd);
}

/* Request a normal socket for i/o
 */
void setsock(int sock, int options)
{
  int i = allocsock(sock, options), parm;
  struct threaddata *td = threaddata();
  int res;

  if (i == -1) {
    putlog(LOG_MISC, "*", "Sockettable full.");
    return;
  }
  if (((sock != STDOUT) || backgrd) && !(td->socklist[i].flags & SOCK_NONSOCK)) {
    parm = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *) &parm, sizeof(int));

    parm = 0;
    setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *) &parm, sizeof(int));

    /* Turn off Nagle's algorithm, see man tcp */
    parm = 1;
    if ((res = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &parm, sizeof parm)))
      debug2("net: setsock(): setsockopt() s %i level IPPROTO_TCP optname TCP_NODELAY error %i", sock, res);
  }
  if (options & SOCK_LISTEN) {
    /* Tris says this lets us grab the same port again next time */
    parm = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &parm, sizeof(int));
  }
  /* Yay async i/o ! */
  if ((sock != STDOUT) || backgrd)
    fcntl(sock, F_SETFL, O_NONBLOCK);
}

int getsock(int af, int options)
{
  int sock = socket(af, SOCK_STREAM, 0);

  if (sock >= 0)
    setsock(sock, options);
  else
    putlog(LOG_MISC, "*", "Warning: Can't create new socket: %s!",
           strerror(errno));
  return sock;
}

/* Done with a socket
 */
void killsock(int sock)
{
  int i;
  struct threaddata *td = threaddata();

  /* Ignore invalid sockets.  */
  if (sock < 0)
    return;

  for (i = 0; i < td->MAXSOCKS; i++) {
    if ((td->socklist[i].sock == sock) && !(td->socklist[i].flags & SOCK_UNUSED)) {
      if (!(td->socklist[i].flags & SOCK_TCL)) { /* nothing to free for tclsocks */
#ifdef TLS
        if (td->socklist[i].ssl) {
          SSL_shutdown(td->socklist[i].ssl);
          nfree(SSL_get_app_data(td->socklist[i].ssl));
          SSL_free(td->socklist[i].ssl);
          td->socklist[i].ssl = NULL;
        }
#endif
        close(td->socklist[i].sock);
        if (td->socklist[i].handler.sock.inbuf != NULL) {
          nfree(td->socklist[i].handler.sock.inbuf);
          td->socklist[i].handler.sock.inbuf = NULL;
        }
        if (td->socklist[i].handler.sock.outbuf != NULL) {
          nfree(td->socklist[i].handler.sock.outbuf);
          td->socklist[i].handler.sock.outbuf = NULL;
          td->socklist[i].handler.sock.outbuflen = 0;
        }
      }
      td->socklist[i].flags = SOCK_UNUSED;
      return;
    }
  }
  putlog(LOG_MISC, "*", "Warning: Attempt to kill un-allocated socket %d!", sock);
}

/* Done with a tcl socket
 *
 * killtclsock() can be called by Tcl threads
 */
void killtclsock(int sock)
{
  int i;
  struct threaddata *td = threaddata();

  if (sock < 0)
    return;

  for (i = 0; i < td->MAXSOCKS; i++) {
    if ((td->socklist[i].flags & SOCK_TCL) && td->socklist[i].sock == sock) {
      td->socklist[i].flags = SOCK_UNUSED;
      return;
    }
  }
}

/* Send connection request to proxy
 */
static int proxy_connect(int sock, sockname_t *addr)
{
  sockname_t name;
  char host[121], s[256];
  int i, port, proxy;

  if (!firewall[0])
    return -2;
#ifdef IPV6
  if (addr->family == AF_INET6) {
    putlog(LOG_MISC, "*", "Eggdrop doesn't support IPv6 connections "
           "through proxies yet.");
    return -1;
  }
#endif
  if (firewall[0] == '!') {
    proxy = PROXY_SUN;
    strlcpy(host, &firewall[1], sizeof host);
  } else {
    proxy = PROXY_SOCKS;
    strcpy(host, firewall);
  }
  port = addr->addr.s4.sin_port;
  setsockname(&name, host, firewallport, 1);
  if (connect(sock, &name.addr.sa, name.addrlen) < 0 && errno != EINPROGRESS)
    return -1;
  if (proxy == PROXY_SOCKS) {
    for (i = 0; i < threaddata()->MAXSOCKS; i++)
      if (!(socklist[i].flags & SOCK_UNUSED) && socklist[i].sock == sock)
        socklist[i].flags |= SOCK_PROXYWAIT;    /* drummer */
    memcpy(host, &addr->addr.s4.sin_addr.s_addr, 4);
    egg_snprintf(s, sizeof s, "\004\001%c%c%c%c%c%c%s", port % 256,
                 (port >> 8) % 256, host[0], host[1], host[2], host[3], botuser);
    tputs(sock, s, strlen(botuser) + 9);        /* drummer */
  } else if (proxy == PROXY_SUN) {
    inet_ntop(AF_INET, &addr->addr.s4.sin_addr, host, sizeof host);
    egg_snprintf(s, sizeof s, "%s %d\n", host, port);
    tputs(sock, s, strlen(s));  /* drummer */
  }
  return sock;
}

/* FIXME: if we can break compatibility for 1.9 or 2.0, we can replace this
 * workaround with an additional port parameter for functions in need
 */
static int get_port_from_addr(const sockname_t *addr)
{
#ifdef IPV6
  return ntohs((addr->family == AF_INET) ? addr->addr.s4.sin_port : addr->addr.s6.sin6_port);
#else
  return addr->addr.s4.sin_port;
#endif
}

/* Starts a connection attempt through a socket
 *
 * The server address should be filled in addr by setsockname() or by the
 * non-blocking dns functions and setsnport().
 *
 * returns < 0 if connection refused:
 *   -1  strerror() type error
 */
int open_telnet_raw(int sock, sockname_t *addr)
{
  sockname_t name;
  socklen_t res_len;
  fd_set sockset;
  struct timeval tv;
  int i, rc, res;

  for (i = 0; i < dcc_total; i++)
    if (dcc[i].sock == sock) { /* Got idx from sock ? */
#ifdef TLS
      debug4("net: open_telnet_raw(): idx %i host %s port %i ssl %i",
             i, dcc[i].host, dcc[i].port, dcc[i].ssl);
#else
      debug3("net: open_telnet_raw(): idx %i host %s port %i",
             i, dcc[i].host, dcc[i].port);
#endif
      break;
    }
  getvhost(&name, addr->family);
  if (bind(sock, &name.addr.sa, name.addrlen) < 0) {
    return -1;
  }
  for (i = 0; i < threaddata()->MAXSOCKS; i++) {
    if (!(socklist[i].flags & SOCK_UNUSED) && (socklist[i].sock == sock))
      socklist[i].flags = (socklist[i].flags & ~SOCK_VIRTUAL) | SOCK_CONNECT;
  }
  if (addr->family == AF_INET && firewall[0])
    return proxy_connect(sock, addr);
  rc = connect(sock, &addr->addr.sa, addr->addrlen);
  if (rc < 0) {
    if (errno == EINPROGRESS) {
      /* Async connection... don't return socket descriptor
       * until after we confirm if it was successful or not */
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      FD_ZERO(&sockset);
      FD_SET(sock, &sockset);
      select(sock + 1, &sockset, NULL, NULL, &tv);
      res_len = sizeof(res);
      getsockopt(sock, SOL_SOCKET, SO_ERROR, &res, &res_len);
      if (res == EINPROGRESS) /* Operation now in progress */
        return sock; /* This could probably fail somewhere */
      if (res == ECONNREFUSED) { /* Connection refused */
        debug2("net: attempted socket connection refused: %s:%i",
               iptostr(&addr->addr.sa), get_port_from_addr(addr));
        return -4;
      }
      if (res != 0) {
        debug1("net: getsockopt error %d", res);
        return -1;
      }
      return sock; /* async success! */
    }
    else {
      return -1;
    }
  }
  return sock;
}

/* Ordinary non-binary connection attempt
 * Return values:
 *   >=0: connect successful, returned is the socket number
 *    -1: look at errno or use strerror()
 *    -2: lookup failed or server is not a valid IP string
 *    -3: could not allocate socket
 *    -4: connection failed
 */
int open_telnet(int idx, char *server, int port)
{
  int ret;

  ret = setsockname(&dcc[idx].sockname, server, port, 1);
  if (ret == AF_UNSPEC)
    return -2;
  dcc[idx].port = port;
  dcc[idx].sock = getsock(ret, 0);
  if (dcc[idx].sock < 0)
    return -3;
  ret = open_telnet_raw(dcc[idx].sock, &dcc[idx].sockname);
  if (ret < 0) {
    if (findidx(dcc[idx].sock) >= 0) {
      killsock(dcc[idx].sock);
    }
  }
  return ret;
}

/* Returns a socket number for a listening socket that will accept any
 * connection on the given address. The address can be filled in by
 * setsockname().
 */
int open_address_listen(sockname_t *addr)
{
  int sock = 0;

  sock = getsock(addr->family, SOCK_LISTEN);
  if (sock < 0)
    return -1;
#if defined IPV6 && IPV6_V6ONLY
  if (addr->family == AF_INET6) {
    int on = 0;
    setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &on, sizeof(on));
  }
#endif
  if (bind(sock, &addr->addr.sa, addr->addrlen) < 0) {
    killsock(sock);
    return -2;
  }

  if (getsockname(sock, &addr->addr.sa, &addr->addrlen) < 0) {
    killsock(sock);
    return -1;
  }
  if (listen(sock, 1) < 0) {
    killsock(sock);
    return -1;
  }

  return sock;
}

/* Returns a socket number for a listening socket that will accept any
 * connection -- port # is returned in port
 */
int open_listen(int *port)
{
  int sock;
  sockname_t name;

  (void) setsockname(&name, listen_ip, *port, 1);
  sock = open_address_listen(&name);
  if (name.addr.sa.sa_family == AF_INET)
    *port = ntohs(name.addr.s4.sin_port);
#ifdef IPV6
  else
    *port = ntohs(name.addr.s6.sin6_port);
#endif
  return sock;
}

/* Short routine to answer a connect received on a listening socket.
 * Returned is the new socket.
 * If port is not NULL, it points to an integer to hold the port number
 * of the caller.
 */
int answer(int sock, sockname_t *caller, uint16_t *port, int binary)
{
  int new_sock;
  caller->addrlen = sizeof(caller->addr);
  new_sock = accept(sock, &caller->addr.sa, &caller->addrlen);

  if (new_sock < 0)
    return -1;

  caller->family = caller->addr.sa.sa_family;
  if (port) {
    if (caller->family == AF_INET)
      *port = ntohs(caller->addr.s4.sin_port);
#ifdef IPV6
    else
      *port = ntohs(caller->addr.s6.sin6_port);
#endif
  }
  setsock(new_sock, (binary ? SOCK_BINARY : 0));
  return new_sock;
}

int getdccaddr(sockname_t *addr, char *s, size_t l)
{
  return getdccfamilyaddr(addr, s, l, AF_UNSPEC);
}

/* Get DCC compatible address for a client to connect (e.g. 1660944385)
 * If addr is not NULL, it should point to the listening socket's address.
 * Otherwise, this function will try to figure out the public address of the
 * machine, using listen_ip and natip. If restrict_af is set, it will limit
 * the possible IPs to the specified family. The result is a string usable
 * for DCC requests
 */
int getdccfamilyaddr(sockname_t *addr, char *s, size_t l, int restrict_af)
{
  char h[121];
  sockname_t name, *r = &name;
  int af = AF_UNSPEC;
#ifdef IPV6
  IP ip = 0;
#endif

  if (addr)
    r = addr;
  else
    setsockname(r, listen_ip, 0, 1);
  if (
#ifdef IPV6
      ((r->family == AF_INET6) &&
      IN6_IS_ADDR_UNSPECIFIED(&r->addr.s6.sin6_addr)) ||
#endif
      (r->family == AF_INET && !r->addr.s4.sin_addr.s_addr)) {
      /* We can't send :: or 0.0.0.0 for dcc, so try
         to figure out some real address */
#ifdef IPV6
     /* If it's listening on an IPv6 :: address,
        try using vhost6 as the source IP */
    if (r->family == AF_INET6 && restrict_af != AF_INET) {
      if (inet_pton(AF_INET6, vhost6, &r->addr.s6.sin6_addr) != 1) {
        r = &name;
        gethostname(h, sizeof h);
        setsockname(r, h, 0, 1);
        if (r->family == AF_INET) {
        /* setsockname tries to resolve both ipv4 and ipv6. ipv4 dns
           resolution comes later in precedence, so if we get an ipv4
           back, reset it to the original addr struct and try
           again */
          if (addr) {
            r = addr;
          } else {
            setsockname(r, listen_ip, 0, 1);
          }
        } else
          af = AF_INET6;
      }
    }
#endif
     /* If IPv6 didn't work or is disabled, or it's listening on an IPv4
        0.0.0.0 address, try using vhost4 as the source */
    if (!af
#ifdef IPV6
        && restrict_af != AF_INET6
#endif
       ) {
      if (!egg_inet_aton(vhost, &r->addr.s4.sin_addr)) {
        /* And if THAT fails, try DNS resolution of hostname */
        r = &name;
        gethostname(h, sizeof h);
        setsockname(r, h, 0, 1);
      }
    }
  }

  if (
#ifdef IPV6
      ((r->family == AF_INET6) &&
      IN6_IS_ADDR_UNSPECIFIED(&r->addr.s6.sin6_addr)) ||
      ((r->family == AF_INET6) && (restrict_af == AF_INET)) ||
      ((r->family == AF_INET) && (restrict_af == AF_INET6)) ||
#endif
      (!natip[0] && (r->family == AF_INET) && !r->addr.s4.sin_addr.s_addr))
    return 0;

#ifdef IPV6
  if (r->family == AF_INET6) {
    if (IN6_IS_ADDR_V4MAPPED(&r->addr.s6.sin6_addr) ||
        IN6_IS_ADDR_UNSPECIFIED(&r->addr.s6.sin6_addr)) {
      memcpy(&ip, r->addr.s6.sin6_addr.s6_addr + 12, sizeof ip);
      egg_snprintf(s, l, "%lu", natip[0] ? iptolong(inet_addr(natip)) :
               ntohl(ip));
    } else
      inet_ntop(AF_INET6, &r->addr.s6.sin6_addr, s, l);
  } else
#endif
  egg_snprintf(s, l, "%lu", natip[0] ? iptolong(inet_addr(natip)) :
             ntohl(r->addr.s4.sin_addr.s_addr));
  return 1;
}

/* Builds the fd_sets for select(). Eggdrop only cares about readable
 * sockets, but tcl also cares for writable/exceptions.
 * preparefdset() can be called by Tcl Threads
 */
static int preparefdset(fd_set *fds, sock_list *slist, int slistmax, int tclonly, int tclmask)
{
  int fd, i, nfds = 0;

  FD_ZERO(fds);
  for (i = 0; i < slistmax; i++) {
    if (!(slist[i].flags & (SOCK_UNUSED | SOCK_VIRTUAL))) {
      if ((slist[i].sock == STDOUT) && !backgrd)
        fd = STDIN;
      else
        fd = slist[i].sock;
      /*
       * Looks like that having more than a call, in the same
       * program, to the FD_SET macro, triggers a bug in gcc.
       * SIGBUS crashing binaries used to be produced on a number
       * (prolly all?) of 64 bits architectures.
       * Make your best to avoid to make it happen again.
       *
       * ITE
       */
      if (slist[i].flags & SOCK_TCL) {
        if (!(slist[i].handler.tclsock.mask & tclmask))
          continue;
      } else if (tclonly)
        continue;
      if (fd > nfds)
        nfds = fd;
      FD_SET(fd, fds);
    }
  }
  return nfds;
}

/* A safer version of write() that deals with partial writes. */
void safe_write(int fd, const void *buf, size_t count)
{
  const char *bytes = buf;
  ssize_t ret;
  static int inhere = 0;

  do {
    if ((ret = write(fd, bytes, count)) == -1 && errno != EINTR) {
      if (!inhere) {
        inhere = 1;
        putlog(LOG_MISC, "*", "Unexpected write() failure on attempt to write %zd bytes to fd %d: %s.", count, fd, strerror(errno));
        inhere = 0;
      }
      break;
    }
  } while ((bytes += ret, count -= ret));
}

/* Attempts to read from all sockets in slist (upper array boundary slistmax-1)
 * fills s with up to 511 bytes if available, and returns the array index
 * Also calls all handler procs for Tcl sockets
 * sockread() can be called by Tcl threads
 *
 *              on EOF:  returns -1, with socket in len
 *     on socket error:  returns -2
 * if nothing is ready:  returns -3
 *    tcl sockets busy:  returns -5
 */
int sockread(char *s, int *len, sock_list *slist, int slistmax, int tclonly)
{
  struct timeval t;
  fd_set fdr, fdw, fde;
  int i, x, nfds_r, nfds_w, nfds_e;
  int grab = 511, tclsock = -1, events = 0;
  struct threaddata *td = threaddata();
  int nfds;

  nfds_r = preparefdset(&fdr, slist, slistmax, tclonly, TCL_READABLE);
  nfds_w = preparefdset(&fdw, slist, slistmax, 1, TCL_WRITABLE);
  nfds_e = preparefdset(&fde, slist, slistmax, 1, TCL_EXCEPTION);

  nfds = nfds_r;
  if (nfds_w > nfds)
    nfds = nfds_w;
  if (nfds_e > nfds)
    nfds = nfds_e;

  /* select() may modify the timeval argument - copy it */
  t.tv_sec = td->blocktime.tv_sec;
  t.tv_usec = td->blocktime.tv_usec;

  x = select((SELECT_TYPE_ARG1) nfds + 1,
             SELECT_TYPE_ARG234 (nfds_r ? &fdr : NULL),
             SELECT_TYPE_ARG234 (nfds_w ? &fdw : NULL),
             SELECT_TYPE_ARG234 (nfds_e ? &fde : NULL),
             SELECT_TYPE_ARG5 &t);
  if (x == -1)
    return -2;                  /* socket error */

  for (i = 0; i < slistmax; i++) {
    if (!tclonly && ((!(slist[i].flags & (SOCK_UNUSED | SOCK_TCL))) &&
        ((FD_ISSET(slist[i].sock, &fdr)) ||
#ifdef TLS
        (slist[i].ssl && (SSL_pending(slist[i].ssl) ||
         !SSL_is_init_finished(slist[i].ssl))) ||
#endif
        ((slist[i].sock == STDOUT) && (!backgrd) &&
         (FD_ISSET(STDIN, &fdr)))))) {
      if (slist[i].flags & (SOCK_LISTEN | SOCK_CONNECT)) {
        /* Listening socket -- don't read, just return activity */
        /* Same for connection attempt */
        /* (for strong connections, require a read to succeed first) */
        if (slist[i].flags & SOCK_PROXYWAIT) /* drummer */
          /* Hang around to get the return code from proxy */
          grab = 10;
#ifdef TLS
        else if (!(slist[i].flags & SOCK_STRONGCONN) &&
                 (!(slist[i].ssl) || SSL_is_init_finished(slist[i].ssl))) {
#else
        else if (!(slist[i].flags & SOCK_STRONGCONN)) {
#endif
          debug1("net: connect! sock %d", slist[i].sock);
          s[0] = 0;
          *len = 0;
          return i;
        }
      } else if (slist[i].flags & SOCK_PASS) {
        s[0] = 0;
        *len = 0;
        return i;
      }
      errno = 0;
      if ((slist[i].sock == STDOUT) && !backgrd)
        x = read(STDIN, s, grab);
      else
#ifdef TLS
      {
        if (slist[i].ssl) {
          x = SSL_read(slist[i].ssl, s, grab);
          if (x < 0) {
            int err = SSL_get_error(slist[i].ssl, x);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
              errno = EAGAIN;
            else if (err == SSL_ERROR_SYSCALL) {
              debug0("net: sockread(): SSL_read() SSL_ERROR_SYSCALL");
              putlog(LOG_MISC, "*", "NET: SSL read failed. Non-SSL connection?");
            }
            else
              debug2("net: sockread(): SSL_read() error = %s (%i)",
                     ERR_error_string(ERR_get_error(), 0), err);
            x = -1;
          }
        } else if (slist[i].flags & SOCK_SENTTLS) {
          /* We are awaiting a reply on our "starttls", only read
           * strlen("starttls -\n") bytes so we don't accidently
           * read the Client Hello from the ssl handshake */
          x = read(slist[i].sock, s, strlen("starttls -\n"));
          slist[i].flags &= ~SOCK_SENTTLS;
        } else
          x = read(slist[i].sock, s, grab);
      }
#else
        x = read(slist[i].sock, s, grab);
#endif
      if (x <= 0) {           /* eof */
        if (errno != EAGAIN) { /* EAGAIN happens when the operation would
                                * block on a non-blocking socket, if the
                                * socket is going to die, it will die later,
                                * otherwise it will connect. */
          *len = slist[i].sock;
          slist[i].flags &= ~SOCK_CONNECT;
          debug1("net: eof!(read) socket %d", slist[i].sock);
          return -1;
        } else {
          debug3("sockread EAGAIN: %d %d (%s)", slist[i].sock, errno,
                 strerror(errno));
          continue;           /* EAGAIN */
        }
      }
      s[x] = 0;
      *len = x;
      if (slist[i].flags & SOCK_PROXYWAIT) {
        debug2("net: socket: %d proxy errno: %d", slist[i].sock, s[1]);
        slist[i].flags &= ~(SOCK_CONNECT | SOCK_PROXYWAIT);
        switch (s[1]) {
        case 90:             /* Success */
          s[0] = 0;
          *len = 0;
          return i;
        case 91:             /* Failed */
          errno = ECONNREFUSED;
          break;
        case 92:             /* No identd */
        case 93:             /* Identd said wrong username */
          /* A better error message would be "socks misconfigured"
           * or "identd not working" but this is simplest.
           */
          errno = ENETUNREACH;
          break;
        }
        *len = slist[i].sock;
        return -1;
      }
      return i;
    } else if (tclsock == -1 && (slist[i].flags & SOCK_TCL)) {
      events = FD_ISSET(slist[i].sock, &fdr) ? TCL_READABLE : 0;
      events |= FD_ISSET(slist[i].sock, &fdw) ? TCL_WRITABLE : 0;
      events |= FD_ISSET(slist[i].sock, &fde) ? TCL_EXCEPTION : 0;
      events &= slist[i].handler.tclsock.mask;
      if (events)
        tclsock = i;
    }
  }
  if (!tclonly) {
    s[0] = 0;
    *len = 0;
  }
  if (tclsock != -1) {
    (*slist[tclsock].handler.tclsock.proc)(slist[tclsock].handler.tclsock.cd,
                                           events);
    return -5;
  }
  return -3;
}

/* sockgets: buffer and read from sockets
 *
 * Attempts to read from all registered sockets for up to one second.  if
 * after one second, no complete data has been received from any of the
 * sockets, 's' will be empty, 'len' will be 0, and sockgets will return -3.
 * if there is returnable data received from a socket, the data will be
 * in 's' (null-terminated if non-binary), the length will be returned
 * in len, and the socket number will be returned.
 * normal sockets have their input buffered, and each call to sockgets
 * will return one line terminated with a '\n'.  binary sockets are not
 * buffered and return whatever coems in as soon as it arrives.
 * listening sockets will return an empty string when a connection comes in.
 * connecting sockets will return an empty string on a successful connect,
 * or EOF on a failed connect.
 * if an EOF is detected from any of the sockets, that socket number will be
 * put in len, and -1 will be returned.
 * the maximum length of the string returned is 512 (including null)
 *
 * Returns -4 if we handled something that shouldn't be handled by the
 * dcc functions. Simply ignore it.
 * Returns -5 if tcl sockets are busy but not eggdrop sockets.
 */

int sockgets(char *s, int *len)
{
  char xx[514], *p, *px;
  int ret, i, data = 0;
  size_t len2;

  for (i = 0; i < threaddata()->MAXSOCKS; i++) {
    /* Check for stored-up data waiting to be processed */
    if (!(socklist[i].flags & (SOCK_UNUSED | SOCK_TCL | SOCK_BUFFER)) &&
        (socklist[i].handler.sock.inbuf != NULL)) {
      if (!(socklist[i].flags & SOCK_BINARY)) {
        /* IRC messages are always lines of characters terminated with a CR-LF
         * (Carriage Return - Line Feed) pair.
         */
        p = strpbrk(socklist[i].handler.sock.inbuf, "\r\n");
        if (p != NULL) {
          *p++ = 0;
          while (*p == '\n' || *p == '\r')
            p++;
          strlcpy(s, socklist[i].handler.sock.inbuf, 511);
          if (*p) {
            len2 = strlen(p) + 1;
            px = nmalloc(len2);
            memcpy(px, p, len2);
            nfree(socklist[i].handler.sock.inbuf);
            socklist[i].handler.sock.inbuf = px;
          } else {
            nfree(socklist[i].handler.sock.inbuf);
            socklist[i].handler.sock.inbuf = NULL;
          }
          *len = strlen(s);
          return socklist[i].sock;
        }
      } else {
        /* Handling buffered binary data (must have been SOCK_BUFFER before). */
        if (socklist[i].handler.sock.inbuflen <= 510) {
          *len = socklist[i].handler.sock.inbuflen;
          memcpy(s, socklist[i].handler.sock.inbuf, socklist[i].handler.sock.inbuflen);
          nfree(socklist[i].handler.sock.inbuf);
          socklist[i].handler.sock.inbuf = NULL;
          socklist[i].handler.sock.inbuflen = 0;
        } else {
          /* Split up into chunks of 510 bytes. */
          *len = 510;
          memcpy(s, socklist[i].handler.sock.inbuf, *len);
          memcpy(socklist[i].handler.sock.inbuf, socklist[i].handler.sock.inbuf + *len, *len);
          socklist[i].handler.sock.inbuflen -= *len;
          socklist[i].handler.sock.inbuf = nrealloc(socklist[i].handler.sock.inbuf, socklist[i].handler.sock.inbuflen);
        }
        return socklist[i].sock;
      }
    }
    /* Also check any sockets that might have EOF'd during write */
    if (!(socklist[i].flags & SOCK_UNUSED) && (socklist[i].flags & SOCK_EOFD)) {
      s[0] = 0;
      *len = socklist[i].sock;
      return -1;
    }
  }
  /* No pent-up data of any worth -- down to business */
  *len = 0;
  ret = sockread(xx, len, socklist, threaddata()->MAXSOCKS, 0);
  if (ret < 0) {
    s[0] = 0;
    return ret;
  }
  /* sockread can return binary data while socket still has connectflag, process first */
  if (socklist[ret].flags & SOCK_BINARY && *len > 0) {
    socklist[ret].flags &= ~SOCK_CONNECT;
    memcpy(s, xx, *len);
    return socklist[ret].sock;
  }
  /* Binary, listening and passed on sockets don't get buffered. */
  if (socklist[ret].flags & SOCK_CONNECT) {
    if (socklist[ret].flags & SOCK_STRONGCONN) {
      socklist[ret].flags &= ~SOCK_STRONGCONN;
      /* Buffer any data that came in, for future read. */
      socklist[ret].handler.sock.inbuflen = *len;
      socklist[ret].handler.sock.inbuf = nmalloc(*len + 1);
      /* It might be binary data. You never know. */
      memcpy(socklist[ret].handler.sock.inbuf, xx, *len);
      socklist[ret].handler.sock.inbuf[*len] = 0;
    }
    socklist[ret].flags &= ~SOCK_CONNECT;
    s[0] = 0;
    return socklist[ret].sock;
  }
  if (socklist[ret].flags & (SOCK_LISTEN | SOCK_PASS | SOCK_TCL)) {
    s[0] = 0; /* for the dcc traffic counters in the mainloop */
    return socklist[ret].sock;
  }
  if (socklist[ret].flags & SOCK_BUFFER) {
    socklist[ret].handler.sock.inbuf = (char *) nrealloc(socklist[ret].handler.sock.inbuf,
                                            socklist[ret].handler.sock.inbuflen + *len + 1);
    memcpy(socklist[ret].handler.sock.inbuf + socklist[ret].handler.sock.inbuflen, xx, *len);
    socklist[ret].handler.sock.inbuflen += *len;
    /* We don't know whether it's binary data. Make sure normal strings
     * will be handled properly later on too. */
    socklist[ret].handler.sock.inbuf[socklist[ret].handler.sock.inbuflen] = 0;
    return -4;                  /* Ignore this one. */
  }
  /* Might be necessary to prepend stored-up data! */
  if (socklist[ret].handler.sock.inbuf != NULL) {
    p = socklist[ret].handler.sock.inbuf;
    socklist[ret].handler.sock.inbuf = nmalloc(strlen(p) + strlen(xx) + 1);
    strcpy(socklist[ret].handler.sock.inbuf, p);
    strcat(socklist[ret].handler.sock.inbuf, xx);
    nfree(p);
    if (strlen(socklist[ret].handler.sock.inbuf) < 512) {
      strcpy(xx, socklist[ret].handler.sock.inbuf);
      nfree(socklist[ret].handler.sock.inbuf);
      socklist[ret].handler.sock.inbuf = NULL;
      socklist[ret].handler.sock.inbuflen = 0;
    } else {
      p = socklist[ret].handler.sock.inbuf;
      socklist[ret].handler.sock.inbuflen = strlen(p) - 510;
      socklist[ret].handler.sock.inbuf = nmalloc(socklist[ret].handler.sock.inbuflen + 1);
      strcpy(socklist[ret].handler.sock.inbuf, p + 510);
      *(p + 510) = 0;
      strcpy(xx, p);
      nfree(p);
      /* (leave the rest to be post-pended later) */
    }
  }
  /* Look for EOL marker; if it's there, i have something to show */
  for (p = xx; *p != 0 ; p++) {
    if ((*p == '\r') || (*p == '\n')) {
      memcpy(s, xx, p - xx);
      s[p - xx] = 0;
      for (p++; (*p == '\r') || (*p == '\n'); p++);
      memmove(xx, p, strlen(p) + 1);
      data = 1; /* DCC_CHAT may now need to process a blank line */
      break;
    }
  }
/* NO! */
/* if (!s[0]) strcpy(s," ");  */
  if (!data) { 
    s[0] = 0;
    if (strlen(xx) >= 510) {
      /* String is too long, so just insert fake \n */
      strcpy(s, xx);
      xx[0] = 0;
      data = 1;
    }
  }
  *len = strlen(s);
  /* Anything left that needs to be saved? */
  if (!xx[0]) {
    if (data)
      return socklist[ret].sock;
    else
      return -3;
  }
  /* Prepend old data back */
  if (socklist[ret].handler.sock.inbuf != NULL) {
    p = socklist[ret].handler.sock.inbuf;
    socklist[ret].handler.sock.inbuflen = strlen(p) + strlen(xx);
    socklist[ret].handler.sock.inbuf = nmalloc(socklist[ret].handler.sock.inbuflen + 1);
    strcpy(socklist[ret].handler.sock.inbuf, xx);
    strcat(socklist[ret].handler.sock.inbuf, p);
    nfree(p);
  } else {
    socklist[ret].handler.sock.inbuflen = strlen(xx);
    len2 = socklist[ret].handler.sock.inbuflen + 1;
    socklist[ret].handler.sock.inbuf = nmalloc(len2);
    memcpy(socklist[ret].handler.sock.inbuf, xx, len2);
  }
  if (data)
    return socklist[ret].sock;
  else
    return -3;
}

/* Dump something to a socket
 *
 * NOTE: Do NOT put Contexts in here if you want DEBUG to be meaningful!!
 */
void tputs(int z, char *s, unsigned int len)
{
  int i, x, idx;
  char *p;
  static int inhere = 0;

  if (z < 0) /* um... HELLO?! sanity check please! */
    return;

  if (((z == STDOUT) || (z == STDERR)) && (!backgrd || use_stderr)) {
    safe_write(z, s, len);
    return;
  }

  for (i = 0; i < threaddata()->MAXSOCKS; i++) {
    if (!(socklist[i].flags & SOCK_UNUSED) && (socklist[i].sock == z)) {
      for (idx = 0; idx < dcc_total; idx++) {
        if ((dcc[idx].sock == z) && dcc[idx].type && dcc[idx].type->name) {
          if (!strncmp(dcc[idx].type->name, "BOT", 3))
            otraffic_bn_today += len;
          else if (!strcmp(dcc[idx].type->name, "SERVER"))
            otraffic_irc_today += len;
          else if (!strncmp(dcc[idx].type->name, "CHAT", 4))
            otraffic_dcc_today += len;
          else if (!strncmp(dcc[idx].type->name, "FILES", 5))
            otraffic_filesys_today += len;
          else if (!strcmp(dcc[idx].type->name, "SEND"))
            otraffic_trans_today += len;
          else if (!strcmp(dcc[idx].type->name, "FORK_SEND"))
            otraffic_trans_today += len;
          else if (!strncmp(dcc[idx].type->name, "GET", 3))
            otraffic_trans_today += len;
          else
            otraffic_unknown_today += len;
          break;
        }
      }

      if (socklist[i].handler.sock.outbuf != NULL) {
        /* Already queueing: just add it */
        p = (char *) nrealloc(socklist[i].handler.sock.outbuf, socklist[i].handler.sock.outbuflen + len);
        memcpy(p + socklist[i].handler.sock.outbuflen, s, len);
        socklist[i].handler.sock.outbuf = p;
        socklist[i].handler.sock.outbuflen += len;
        return;
      }
#ifdef TLS
      if (socklist[i].ssl) {
        x = SSL_write(socklist[i].ssl, s, len);
        if (x < 0) {
          int err = SSL_get_error(socklist[i].ssl, x);
          if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ)
            errno = EAGAIN;
          else if (!inhere) { /* Out there, somewhere */
            inhere = 1;
            debug1("tputs(): SSL error = %s",
                   ERR_error_string(ERR_get_error(), 0));
            inhere = 0;
          }
          x = -1;
        }
      } else /* not ssl, use regular write() */
#endif
      /* Try. */
      x = write(z, s, len);
      if (x == -1)
        x = 0;
      if (x < len) {
        /* Socket is full, queue it */
        socklist[i].handler.sock.outbuf = nmalloc(len - x);
        memcpy(socklist[i].handler.sock.outbuf, &s[x], len - x);
        socklist[i].handler.sock.outbuflen = len - x;
      }
      return;
    }
  }
  /* Make sure we don't cause a crash by looping here */
  if (!inhere) {
    inhere = 1;

    putlog(LOG_MISC, "*", "!!! writing to nonexistent socket: %d", z);
    s[strlen(s) - 1] = 0;
    putlog(LOG_MISC, "*", "!-> '%s'", s);

    inhere = 0;
  }
}

/* tputs might queue data for sockets, let's dump as much of it as
 * possible.
 */
void dequeue_sockets()
{
  int i, x;
  int z = 0;
  fd_set wfds;
  struct timeval tv;
  int nfds = 0;

/* ^-- start poptix test code, this should avoid writes to sockets not ready to be written to. */

  FD_ZERO(&wfds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;               /* we only want to see if it's ready for writing, no need to actually wait.. */
  for (i = 0; i < threaddata()->MAXSOCKS; i++)
    if (!(socklist[i].flags & (SOCK_UNUSED | SOCK_TCL)) &&
        (socklist[i].handler.sock.outbuf != NULL)) {
      if (socklist[i].sock > nfds)
        nfds = socklist[i].sock;
      FD_SET(socklist[i].sock, &wfds);
      z = 1;
    }
  if (!z)
    return;                     /* nothing to write */

  select((SELECT_TYPE_ARG1) nfds + 1, SELECT_TYPE_ARG234 NULL,
         SELECT_TYPE_ARG234 &wfds, SELECT_TYPE_ARG234 NULL,
         SELECT_TYPE_ARG5 &tv);

/* end poptix */

  for (i = 0; i < threaddata()->MAXSOCKS; i++) {
    if (!(socklist[i].flags & (SOCK_UNUSED | SOCK_TCL)) &&
        (socklist[i].handler.sock.outbuf != NULL) && (FD_ISSET(socklist[i].sock, &wfds))) {
      /* Trick tputs into doing the work */
      errno = 0;
#ifdef TLS
      if (socklist[i].ssl) {
        x = SSL_write(socklist[i].ssl, socklist[i].handler.sock.outbuf,
                      socklist[i].handler.sock.outbuflen);
        if (x < 0) {
          int err = SSL_get_error(socklist[i].ssl, x);
          if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ)
            errno = EAGAIN;
          else
            debug1("dequeue_sockets(): SSL error = %s",
                   ERR_error_string(ERR_get_error(), 0));
          x = -1;
        }
      } else
#endif
      x = write(socklist[i].sock, socklist[i].handler.sock.outbuf,
                socklist[i].handler.sock.outbuflen);
      if ((x < 0) && (errno != EAGAIN)
#ifdef EBADSLT
          && (errno != EBADSLT)
#endif
#ifdef ENOTCONN
          && (errno != ENOTCONN)
#endif
        ) {
        /* This detects an EOF during writing */
        debug3("net: eof!(write) socket %d (%s,%d)", socklist[i].sock,
               strerror(errno), errno);
        socklist[i].flags |= SOCK_EOFD;
      } else if (x == socklist[i].handler.sock.outbuflen) {
        /* If the whole buffer was sent, nuke it */
        nfree(socklist[i].handler.sock.outbuf);
        socklist[i].handler.sock.outbuf = NULL;
        socklist[i].handler.sock.outbuflen = 0;
      } else if (x > 0) {
        char *p = socklist[i].handler.sock.outbuf;

        /* This removes any sent bytes from the beginning of the buffer */
        socklist[i].handler.sock.outbuf =
                            nmalloc(socklist[i].handler.sock.outbuflen - x);
        memcpy(socklist[i].handler.sock.outbuf, p + x,
                   socklist[i].handler.sock.outbuflen - x);
        socklist[i].handler.sock.outbuflen -= x;
        nfree(p);
      } else {
        debug3("dequeue_sockets(): errno = %d (%s) on %d", errno,
               strerror(errno), socklist[i].sock);
      }
      /* All queued data was sent. Call handler if one exists and the
       * dcc entry wants it.
       */
      if (!socklist[i].handler.sock.outbuf) {
        int idx = findanyidx(socklist[i].sock);

        if (idx > 0 && dcc[idx].type && dcc[idx].type->outdone)
          dcc[idx].type->outdone(idx);
      }
    }
  }
}


/*
 *      Debugging stuff
 */

void tell_netdebug(int idx)
{
  int i;
  char s[80];

  dprintf(idx, "Open sockets:");
  for (i = 0; i < threaddata()->MAXSOCKS; i++) {
    if (!(socklist[i].flags & SOCK_UNUSED)) {
      sprintf(s, " %d", socklist[i].sock);
      if (socklist[i].flags & SOCK_BINARY)
        strcat(s, " (binary)");
      if (socklist[i].flags & SOCK_LISTEN)
        strcat(s, " (listen)");
      if (socklist[i].flags & SOCK_PASS)
        strcat(s, " (passed on)");
      if (socklist[i].flags & SOCK_CONNECT)
        strcat(s, " (connecting)");
      if (socklist[i].flags & SOCK_STRONGCONN)
        strcat(s, " (strong)");
      if (socklist[i].flags & SOCK_NONSOCK)
        strcat(s, " (file)");
#ifdef TLS
      if (socklist[i].ssl)
        strcat(s, " (TLS)");
#endif
      if (socklist[i].flags & SOCK_TCL)
        strcat(s, " (tcl)");
      if (!(socklist[i].flags & SOCK_TCL)) {
        if (socklist[i].handler.sock.inbuf != NULL)
          sprintf(&s[strlen(s)], " (inbuf: %04X)",
                  (unsigned int) strlen(socklist[i].handler.sock.inbuf));
        if (socklist[i].handler.sock.outbuf != NULL)
          sprintf(&s[strlen(s)], " (outbuf: %06lX)", socklist[i].handler.sock.outbuflen);
      }
      strcat(s, ",");
      dprintf(idx, "%s", s);
    }
  }
  dprintf(idx, " done.\n");
}

/* Security-flavoured sanity checking on DCC connections of all sorts can be
 * done with this routine.  Feed it the proper information from your DCC
 * before you attempt the connection, and this will make an attempt at
 * figuring out if the connection is really that person, or someone screwing
 * around.  It's not foolproof, but anything that fails this check probably
 * isn't going to work anyway due to masquerading firewalls, NAT routers,
 * or bugs in mIRC.
 */
int sanitycheck_dcc(char *nick, char *from, char *ipaddy, char *port)
{
  /* According to the latest RFC, the clients SHOULD be able to handle
   * DNS names that are up to 255 characters long.  This is not broken.
   */

#ifdef IPV6
  char badaddress[INET6_ADDRSTRLEN];
  sockname_t name;
  IP ip = 0;
#else
  char badaddress[INET_ADDRSTRLEN];
  IP ip = my_atoul(ipaddy);
#endif
  int prt = atoi(port);

  /* It is disabled HERE so we only have to check in *one* spot! */
  if (!dcc_sanitycheck)
    return 1;

  if (prt < 1) {
    putlog(LOG_MISC, "*", "ALERT: (%s!%s) specified an impossible port of %u!",
           nick, from, prt);
    return 0;
  }
#ifdef IPV6
  if (strchr(ipaddy, ':')) {
    if (inet_pton(AF_INET6, ipaddy, &name.addr.s6.sin6_addr) != 1) {
      putlog(LOG_MISC, "*", "ALERT: (%s!%s) specified an invalid IPv6 "
             "address of %s!", nick, from, ipaddy);
      return 0;
    }
    if (IN6_IS_ADDR_V4MAPPED(&name.addr.s6.sin6_addr)) {
      memcpy(&ip, name.addr.s6.sin6_addr.s6_addr + 12, sizeof ip);
      ip = ntohl(ip);
    }
  }
#endif
  if (ip && inet_ntop(AF_INET, &ip, badaddress, sizeof badaddress) &&
      (ip < (1 << 24))) {
    putlog(LOG_MISC, "*", "ALERT: (%s!%s) specified an impossible IP of %s!",
           nick, from, badaddress);
    return 0;
  }
  return 1;
}

int hostsanitycheck_dcc(char *nick, char *from, sockname_t *ip, char *dnsname,
                        char *prt)
{
  char badaddress[INET6_ADDRSTRLEN];

  /* According to the latest RFC, the clients SHOULD be able to handle
   * DNS names that are up to 255 characters long.  This is not broken.
   */
  char hostn[256];

  /* It is disabled HERE so we only have to check in *one* spot! */
  if (!dcc_sanitycheck)
    return 1;
  strcpy(badaddress, iptostr(&ip->addr.sa));
  /* These should pad like crazy with zeros, since 120 bytes or so is
   * where the routines providing our data currently lose interest. I'm
   * using the n-variant in case someone changes that...
   */
  strlcpy(hostn, extracthostname(from), sizeof hostn);
  if (!egg_strcasecmp(hostn, dnsname)) {
    putlog(LOG_DEBUG, "*", "DNS information for submitted IP checks out.");
    return 1;
  }
  if (!strcmp(badaddress, dnsname))
    putlog(LOG_MISC, "*", "ALERT: (%s!%s) sent a DCC request with bogus IP "
           "information of %s port %s. %s does not resolve to %s!", nick, from,
           badaddress, prt, from, badaddress);
  else
    return 1;                   /* <- usually happens when we have
                                 * a user with an unresolved hostmask! */
  return 0;
}

/* Checks whether the referenced socket has data queued.
 *
 * Returns true if the incoming/outgoing (depending on 'type') queues
 * contain data, otherwise false.
 */
int sock_has_data(int type, int sock)
{
  int ret = 0, i;

  for (i = 0; i < threaddata()->MAXSOCKS; i++)
    if (!(socklist[i].flags & SOCK_UNUSED) && socklist[i].sock == sock)
      break;
  if (i < threaddata()->MAXSOCKS) {
    switch (type) {
    case SOCK_DATA_OUTGOING:
      ret = (socklist[i].handler.sock.outbuf != NULL);
      break;
    case SOCK_DATA_INCOMING:
      ret = (socklist[i].handler.sock.inbuf != NULL);
      break;
    }
  } else
    debug1("sock_has_data: could not find socket #%d, returning false.", sock);
  return ret;
}

/* flush_inbuf():
 * checks if there's data in the incoming buffer of an connection
 * and flushes the buffer if possible
 *
 * returns: -1 if the dcc entry wasn't found
 *          -2 if dcc[idx].type->activity doesn't exist and the data couldn't
 *             be handled
 *          0 if buffer was empty
 *          otherwise length of flushed buffer
 */
int flush_inbuf(int idx)
{
  int i, len;
  char *inbuf;

  Assert((idx >= 0) && (idx < dcc_total));
  for (i = 0; i < threaddata()->MAXSOCKS; i++) {
    if ((dcc[idx].sock == socklist[i].sock) &&
        !(socklist[i].flags & SOCK_UNUSED)) {
      len = socklist[i].handler.sock.inbuflen;
      if ((len > 0) && socklist[i].handler.sock.inbuf) {
        if (dcc[idx].type && dcc[idx].type->activity) {
          inbuf = socklist[i].handler.sock.inbuf;
          socklist[i].handler.sock.inbuf = NULL;
          dcc[idx].type->activity(idx, inbuf, len);
          nfree(inbuf);
          return len;
        } else
          return -2;
      } else
        return 0;
    }
  }
  return -1;
}

/* Find sock in socklist.
 *
 * Returns index in socklist or -1 if not found.
 */
int findsock(int sock)
{
  int i;
  struct threaddata *td = threaddata();

  for (i = 0; i < td->MAXSOCKS; i++)
    if (td->socklist[i].sock == sock)
      break;
  if (i == td->MAXSOCKS)
    return -1;
  return i;
}

/* Trace on my-ip and my-hostname variable to handle transition into vhost4/vhost6/listen-addr.
 */
char *traced_myiphostname(ClientData cd, Tcl_Interp *irp, EGG_CONST char *name1, EGG_CONST char *name2, int flags)
{
  const char *value;

  if (flags & TCL_INTERP_DESTROYED)
    return NULL;
  /* Recover trace in case of unset. */
  if (flags & TCL_TRACE_DESTROYED) {
    Tcl_TraceVar2(irp, name1, name2, TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS, traced_myiphostname, cd);
    return NULL;
  }

  value = Tcl_GetVar2(irp, name1, name2, TCL_GLOBAL_ONLY);
  strlcpy(vhost, value, sizeof vhost);
  strlcpy(listen_ip, value, sizeof listen_ip);
  putlog(LOG_MISC, "*", "WARNING: You are using the DEPRECATED variable '%s' in your config file.\n", name1);
  putlog(LOG_MISC, "*", "    To prevent future incompatibility, please use the vhost4/listen-addr variables instead.\n");
  putlog(LOG_MISC, "*", "    More information on this subject can be found in the eggdrop/doc/IPV6 file, or\n");
  putlog(LOG_MISC, "*", "    in the comments above those settings in the example eggdrop.conf that is included with Eggdrop.\n");
  return NULL;
}
