/*
 * net.c -- handles:
 *   all raw network i/o
 *
 * $Id: net.c,v 1.87 2011/02/13 14:19:33 simple Exp $
 */
/*
 * This is hereby released into the public domain.
 * Robey Pointer, robey@netcom.com
 *
 * Changes after Feb 23, 1999 Copyright Eggheads Development Team
 *
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
#include <arpa/inet.h>
#include <errno.h>
#if HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <setjmp.h>

#ifndef HAVE_GETDTABLESIZE
#  ifdef FD_SETSIZE
#    define getdtablesize() FD_SETSIZE
#  else
#    define getdtablesize() 200
#  endif
#endif

extern struct dcc_t *dcc;
extern int backgrd, use_stderr, resolve_timeout, dcc_total;
extern unsigned long otraffic_irc_today, otraffic_bn_today, otraffic_dcc_today,
                     otraffic_filesys_today, otraffic_trans_today,
                     otraffic_unknown_today;

char hostname[121] = "";      /* Hostname can be specified in the config file.*/
char myip[121] = "";          /* IP can be specified in the config file.      */
char firewall[121] = "";      /* Socks server for firewall.                   */
int firewallport = 1080;      /* Default port of socks 4/5 firewalls.         */
char botuser[21] = "eggdrop"; /* Username of the user running the bot.        */
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

/* Get my ip number
 */
IP getmyip()
{
  struct hostent *hp;
  char s[121];
  IP ip;
  struct in_addr *in;
  if (myip[0]) {
    if ((myip[strlen(myip) - 1] >= '0') && (myip[strlen(myip) - 1] <= '9'))
      return (IP) inet_addr(myip);
  }
  /* Also could be pre-defined */
  if (hostname[0])
    hp = gethostbyname(hostname);
  else {
    gethostname(s, 120);
    hp = gethostbyname(s);
  }
  if (hp == NULL)
    fatal("Hostname self-lookup failed. Please set 'my-ip' in the config "
          "file.", 0);
  in = (struct in_addr *) (hp->h_addr_list[0]);
  ip = (IP) (in->s_addr);
  return ip;
}

void neterror(char *s)
{
  switch (errno) {
  case EADDRINUSE:
    strcpy(s, "Address already in use");
    break;
  case EADDRNOTAVAIL:
    strcpy(s, "Cannot assign requested address");
    break;
  case EAFNOSUPPORT:
    strcpy(s, "Address family not supported");
    break;
  case EALREADY:
    strcpy(s, "Socket already in use");
    break;
  case EBADF:
    strcpy(s, "Socket descriptor is bad");
    break;
  case ECONNREFUSED:
    strcpy(s, "Connection refused");
    break;
  case EFAULT:
    strcpy(s, "Bad address");
    break;
  case EINPROGRESS:
    strcpy(s, "Operation in progress");
    break;
  case EINTR:
    strcpy(s, "Timeout");
    break;
  case EINVAL:
    strcpy(s, "Invalid argument");
    break;
  case EISCONN:
    strcpy(s, "Socket already connected");
    break;
  case ENETUNREACH:
    strcpy(s, "Network unreachable");
    break;
  case ENOTSOCK:
    strcpy(s, "Socket operation on non-socket");
    break;
  case ETIMEDOUT:
    strcpy(s, "Connection timed out");
    break;
  case ENOTCONN:
    strcpy(s, "Socket is not connected");
    break;
  case EHOSTUNREACH:
    strcpy(s, "No route to host");
    break;
  case EPIPE:
    strcpy(s, "Broken pipe");
    break;
#ifdef ECONNRESET
  case ECONNRESET:
    strcpy(s, "Connection reset by peer");
    break;
#endif
#ifdef EACCES
  case EACCES:
    strcpy(s, "Permission denied");
    break;
#endif
#ifdef EMFILE
  case EMFILE:
    strcpy(s, "Too many open files");
    break;
#endif
  case 0:
    strcpy(s, "Error 0");
    break;
  default:
    sprintf(s, "Unforseen error %d", errno);
    break;
  }
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
int alloctclsock(register int sock, int mask, Tcl_FileProc *proc, ClientData cd)
{
  int f = -1;
  register int i;
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

  if (i == -1) {
    putlog(LOG_MISC, "*", "Sockettable full.");
    return;
  }
  if (((sock != STDOUT) || backgrd) && !(td->socklist[i].flags & SOCK_NONSOCK)) {
    parm = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *) &parm, sizeof(int));

    parm = 0;
    setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *) &parm, sizeof(int));
  }
  if (options & SOCK_LISTEN) {
    /* Tris says this lets us grab the same port again next time */
    parm = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &parm, sizeof(int));
  }
  /* Yay async i/o ! */
  fcntl(sock, F_SETFL, O_NONBLOCK);
}

int getsock(int options)
{
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock >= 0)
    setsock(sock, options);
  else
    putlog(LOG_MISC, "*", "Warning: Can't create new socket!");
  return sock;
}

/* Done with a socket
 */
void killsock(register int sock)
{
  register int i;
  struct threaddata *td = threaddata();

  /* Ignore invalid sockets.  */
  if (sock < 0)
    return;

  for (i = 0; i < td->MAXSOCKS; i++) {
    if ((td->socklist[i].sock == sock) && !(td->socklist[i].flags & SOCK_UNUSED)) {
      if (!(td->socklist[i].flags & SOCK_TCL)) { /* nothing to free for tclsocks */
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
void killtclsock(register int sock)
{
  register int i;
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
static int proxy_connect(int sock, char *host, int port, int proxy)
{
  unsigned char x[10];
  struct hostent *hp;
  char s[256];
  int i;

  /* socks proxy */
  if (proxy == PROXY_SOCKS) {
    /* numeric IP? */
    if (host[strlen(host) - 1] >= '0' && host[strlen(host) - 1] <= '9') {
      IP ip = ((IP) inet_addr(host));
      egg_memcpy(x, &ip, 4);
    } else {
      /* no, must be host.domain */
      if (!sigsetjmp(alarmret, 1)) {
        alarm(resolve_timeout);
        hp = gethostbyname(host);
        alarm(0);
      } else
        hp = NULL;
      if (hp == NULL) {
        killsock(sock);
        return -2;
      }
      egg_memcpy(x, hp->h_addr, hp->h_length);
    }
    for (i = 0; i < threaddata()->MAXSOCKS; i++)
      if (!(socklist[i].flags & SOCK_UNUSED) && socklist[i].sock == sock)
        socklist[i].flags |= SOCK_PROXYWAIT;    /* drummer */
      egg_snprintf(s, sizeof s, "\004\001%c%c%c%c%c%c%s", (port >> 8) % 256,
                   (port % 256), x[0], x[1], x[2], x[3], botuser);
    tputs(sock, s, strlen(botuser) + 9);        /* drummer */
  } else if (proxy == PROXY_SUN) {
    egg_snprintf(s, sizeof s, "%s %d\n", host, port);
    tputs(sock, s, strlen(s));  /* drummer */
  }
  return sock;
}

/* Starts a connection attempt to a socket
 *
 * If given a normal hostname, this will be resolved to the corresponding
 * IP address first. PLEASE try to use the non-blocking dns functions
 * instead and then call this function with the IP address to avoid blocking.
 *
 * returns <0 if connection refused:
 *   -1  neterror() type error
 *   -2  can't resolve hostname
 */
int open_telnet_raw(int sock, char *server, int sport)
{
  struct sockaddr_in name;
  struct hostent *hp;
  char host[121];
  int i, port, rc;
  volatile int proxy;

  /* firewall?  use socks */
  if (firewall[0]) {
    if (firewall[0] == '!') {
      proxy = PROXY_SUN;
      strcpy(host, &firewall[1]);
    } else {
      proxy = PROXY_SOCKS;
      strcpy(host, firewall);
    }
    port = firewallport;
  } else {
    proxy = 0;
    strcpy(host, server);
    port = sport;
  }
  egg_bzero((char *) &name, sizeof(struct sockaddr_in));

  name.sin_family = AF_INET;
  name.sin_addr.s_addr = (myip[0] ? getmyip() : INADDR_ANY);
  if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
    killsock(sock);
    return -1;
  }
  egg_bzero((char *) &name, sizeof(struct sockaddr_in));

  name.sin_family = AF_INET;
  name.sin_port = htons(port);
  /* Numeric IP? */
  if ((host[strlen(host) - 1] >= '0') && (host[strlen(host) - 1] <= '9'))
    name.sin_addr.s_addr = inet_addr(host);
  else {
    /* No, must be host.domain */
    debug0("WARNING: open_telnet_raw() is about to block in gethostbyname()!");
    if (!sigsetjmp(alarmret, 1)) {
      alarm(resolve_timeout);
      hp = gethostbyname(host);
      alarm(0);
    } else
      hp = NULL;
    if (hp == NULL)
      return -2;
    egg_memcpy(&name.sin_addr, hp->h_addr, hp->h_length);
    name.sin_family = hp->h_addrtype;
  }
  for (i = 0; i < threaddata()->MAXSOCKS; i++) {
    if (!(socklist[i].flags & SOCK_UNUSED) && (socklist[i].sock == sock))
      socklist[i].flags = (socklist[i].flags & ~SOCK_VIRTUAL) | SOCK_CONNECT;
  }
  rc = connect(sock, (struct sockaddr *) &name, sizeof(struct sockaddr_in));
  if (rc < 0) {
    if (errno == EINPROGRESS) {
      /* Firewall?  announce connect attempt to proxy */
      if (firewall[0])
        return proxy_connect(sock, server, sport, proxy);
      return sock; /* async success! */
    } else
      return -1;
  }
  /* Synchronous? :/ */
  if (firewall[0])
    return proxy_connect(sock, server, sport, proxy);
  return sock;
}

/* Ordinary non-binary connection attempt */
int open_telnet(char *server, int port)
{
  int sock = getsock(0), ret = open_telnet_raw(sock, server, port);

  return ret;
}

/* Returns a socket number for a listening socket that will accept any
 * connection on a certain address -- port # is returned in port
 */
int open_address_listen(IP addr, int *port)
 {
  int sock = 0;
  socklen_t addrlen;
  struct sockaddr_in name;

  if (firewall[0]) {
    /* FIXME: can't do listen port thru firewall yet */
    putlog(LOG_MISC, "*", "Can't open a listen port (you are using a "
           "firewall).");
    return -1;
  }

  if (getmyip() > 0) {
    sock = getsock(SOCK_LISTEN);
    if (sock < 1)
      return -1;

    egg_bzero((char *) &name, sizeof(struct sockaddr_in));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port); /* 0 = just assign us a port */
    name.sin_addr.s_addr = addr;
    if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
      killsock(sock);
      return -1;
    }
    /* what port are we on? */
    addrlen = sizeof(name);
    if (getsockname(sock, (struct sockaddr *) &name, &addrlen) < 0) {
      killsock(sock);
      return -1;
    }
    *port = ntohs(name.sin_port);
    if (listen(sock, 1) < 0) {
      killsock(sock);
      return -1;
    }
  }

  return sock;
}

/* Returns a socket number for a listening socket that will accept any
 * connection -- port # is returned in port
 */
inline int open_listen(int *port)
{
  return open_address_listen(myip[0] ? getmyip() : INADDR_ANY, port);
}

/* Returns the given network byte order IP address in the
 * dotted format - "##.##.##.##"
 */
char *iptostr(IP ip)
{
  struct in_addr a;

  a.s_addr = ip;
  return inet_ntoa(a);
}

/* Short routine to answer a connect received on a socket made previously
 * by open_listen ... returns hostname of the caller & the new socket
 * does NOT dispose of old "public" socket!
 */
int answer(int sock, char *caller, unsigned long *ip, unsigned short *port,
           int binary)
{
  int new_sock;
  socklen_t addrlen;
  struct sockaddr_in from;

  addrlen = sizeof(struct sockaddr);
  new_sock = accept(sock, (struct sockaddr *) &from, &addrlen);

  if (new_sock < 0)
    return -1;
  if (ip != NULL) {
    *ip = from.sin_addr.s_addr;
    /* DNS is now done asynchronously. We now only provide the IP address. */
    strncpyz(caller, iptostr(*ip), 121);
    *ip = ntohl(*ip);
  }
  if (port != NULL)
    *port = ntohs(from.sin_port);
  /* Set up all the normal socket crap */
  setsock(new_sock, (binary ? SOCK_BINARY : 0));
  return new_sock;
}

/* Like open_telnet, but uses server & port specifications of dcc
 */
int open_telnet_dcc(int sock, char *server, char *port)
{
  int p;
  unsigned long addr;
  char sv[500];
  unsigned char c[4];

  if (port != NULL)
    p = atoi(port);
  else
    p = 2000;
  if (server != NULL)
    addr = my_atoul(server);
  else
    addr = 0L;
  if (addr < (1 << 24))
    return -3;                  /* fake address */
  c[0] = (addr >> 24) & 0xff;
  c[1] = (addr >> 16) & 0xff;
  c[2] = (addr >> 8) & 0xff;
  c[3] = addr & 0xff;
  sprintf(sv, "%u.%u.%u.%u", c[0], c[1], c[2], c[3]);
  p = open_telnet_raw(sock, sv, p);
  return p;
}

/* Builds the fd_sets for select(). Eggdrop only cares about readable
 * sockets, but tcl also cares for writable/exceptions.
 * preparefdset() can be called by Tcl Threads
 */
int preparefdset(fd_set *fd, sock_list *slist, int slistmax, int tclonly, int tclmask)
{
  int fdtmp, i, foundsocks = 0;

  FD_ZERO(fd);
  for (i = 0; i < slistmax; i++) {
    if (!(slist[i].flags & (SOCK_UNUSED | SOCK_VIRTUAL))) {
      if ((slist[i].sock == STDOUT) && !backgrd)
        fdtmp = STDIN;
      else
        fdtmp = slist[i].sock;
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
      foundsocks = 1;
      FD_SET(fdtmp, fd);
    }
  }
  return foundsocks;
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
  int fds, i, x, have_r, have_w, have_e;
  int grab = 511, tclsock = -1, events = 0;
  struct threaddata *td = threaddata();

  fds = getdtablesize();
#ifdef FD_SETSIZE
  if (fds > FD_SETSIZE)
    fds = FD_SETSIZE;           /* Fixes YET ANOTHER freebsd bug!!! */
#endif

  have_r = preparefdset(&fdr, slist, slistmax, tclonly, TCL_READABLE);
  have_w = preparefdset(&fdw, slist, slistmax, 1, TCL_WRITABLE);
  have_e = preparefdset(&fde, slist, slistmax, 1, TCL_EXCEPTION);

  /* select() may modify the timeval argument - copy it */
  t.tv_sec = td->blocktime.tv_sec;
  t.tv_usec = td->blocktime.tv_usec;

  x = select((SELECT_TYPE_ARG1) fds,
             SELECT_TYPE_ARG234 (have_r ? &fdr : NULL),
             SELECT_TYPE_ARG234 (have_w ? &fdw : NULL),
             SELECT_TYPE_ARG234 (have_e ? &fde : NULL),
             SELECT_TYPE_ARG5 &t);
  if (x > 0) {
    /* Something happened */
    for (i = 0; i < slistmax; i++) {
      if (!tclonly && ((!(slist[i].flags & (SOCK_UNUSED | SOCK_TCL))) &&
          ((FD_ISSET(slist[i].sock, &fdr)) ||
          ((slist[i].sock == STDOUT) && (!backgrd) &&
          (FD_ISSET(STDIN, &fdr)))))) {
        if (slist[i].flags & (SOCK_LISTEN | SOCK_CONNECT)) {
          /* Listening socket -- don't read, just return activity */
          /* Same for connection attempt */
          /* (for strong connections, require a read to succeed first) */
          if (slist[i].flags & SOCK_PROXYWAIT) /* drummer */
            /* Hang around to get the return code from proxy */
            grab = 10;
          else if (!(slist[i].flags & SOCK_STRONGCONN)) {
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
          x = read(slist[i].sock, s, grab);
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
  } else if (x == -1)
    return -2;                  /* socket error */
  else if (!tclonly) {
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

  for (i = 0; i < threaddata()->MAXSOCKS; i++) {
    /* Check for stored-up data waiting to be processed */
    if (!(socklist[i].flags & (SOCK_UNUSED | SOCK_TCL | SOCK_BUFFER)) &&
        (socklist[i].handler.sock.inbuf != NULL)) {
      if (!(socklist[i].flags & SOCK_BINARY)) {
        /* look for \r too cos windows can't follow RFCs */
        p = strchr(socklist[i].handler.sock.inbuf, '\n');
        if (p == NULL)
          p = strchr(socklist[i].handler.sock.inbuf, '\r');
        if (p != NULL) {
          *p = 0;
          if (strlen(socklist[i].handler.sock.inbuf) > 510)
            socklist[i].handler.sock.inbuf[510] = 0;
          strcpy(s, socklist[i].handler.sock.inbuf);
          px = nmalloc(strlen(p + 1) + 1);
          strcpy(px, p + 1);
          nfree(socklist[i].handler.sock.inbuf);
          if (px[0])
            socklist[i].handler.sock.inbuf = px;
          else {
            nfree(px);
            socklist[i].handler.sock.inbuf = NULL;
          }
          /* Strip CR if this was CR/LF combo */
          if (s[strlen(s) - 1] == '\r')
            s[strlen(s) - 1] = 0;
          *len = strlen(s);
          return socklist[i].sock;
        }
      } else {
        /* Handling buffered binary data (must have been SOCK_BUFFER before). */
        if (socklist[i].handler.sock.inbuflen <= 510) {
          *len = socklist[i].handler.sock.inbuflen;
          egg_memcpy(s, socklist[i].handler.sock.inbuf, socklist[i].handler.sock.inbuflen);
          nfree(socklist[i].handler.sock.inbuf);
          socklist[i].handler.sock.inbuf = NULL;
          socklist[i].handler.sock.inbuflen = 0;
        } else {
          /* Split up into chunks of 510 bytes. */
          *len = 510;
          egg_memcpy(s, socklist[i].handler.sock.inbuf, *len);
          egg_memcpy(socklist[i].handler.sock.inbuf, socklist[i].handler.sock.inbuf + *len, *len);
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
  /* Binary, listening and passed on sockets don't get buffered. */
  if (socklist[ret].flags & SOCK_CONNECT) {
    if (socklist[ret].flags & SOCK_STRONGCONN) {
      socklist[ret].flags &= ~SOCK_STRONGCONN;
      /* Buffer any data that came in, for future read. */
      socklist[ret].handler.sock.inbuflen = *len;
      socklist[ret].handler.sock.inbuf = nmalloc(*len + 1);
      /* It might be binary data. You never know. */
      egg_memcpy(socklist[ret].handler.sock.inbuf, xx, *len);
      socklist[ret].handler.sock.inbuf[*len] = 0;
    }
    socklist[ret].flags &= ~SOCK_CONNECT;
    s[0] = 0;
    return socklist[ret].sock;
  }
  if (socklist[ret].flags & SOCK_BINARY) {
    egg_memcpy(s, xx, *len);
    return socklist[ret].sock;
  }
  if (socklist[ret].flags & (SOCK_LISTEN | SOCK_PASS | SOCK_TCL)) {
    s[0] = 0; /* for the dcc traffic counters in the mainloop */
    return socklist[ret].sock;
  }
  if (socklist[ret].flags & SOCK_BUFFER) {
    socklist[ret].handler.sock.inbuf = (char *) nrealloc(socklist[ret].handler.sock.inbuf,
                                            socklist[ret].handler.sock.inbuflen + *len + 1);
    egg_memcpy(socklist[ret].handler.sock.inbuf + socklist[ret].handler.sock.inbuflen, xx, *len);
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
  p = strchr(xx, '\n');
  if (p == NULL)
    p = strchr(xx, '\r');
  if (p != NULL) {
    *p = 0;
    strcpy(s, xx);
    memmove(xx, p + 1, strlen(p + 1) + 1);
    if (s[strlen(s) - 1] == '\r')
      s[strlen(s) - 1] = 0;
    data = 1; /* DCC_CHAT may now need to process a blank line */
/* NO! */
/* if (!s[0]) strcpy(s," ");  */
  } else {
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
    socklist[ret].handler.sock.inbuf = nmalloc(socklist[ret].handler.sock.inbuflen + 1);
    strcpy(socklist[ret].handler.sock.inbuf, xx);
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
void tputs(register int z, char *s, unsigned int len)
{
  register int i, x, idx;
  char *p;
  static int inhere = 0;

  if (z < 0) /* um... HELLO?! sanity check please! */
    return;

  if (((z == STDOUT) || (z == STDERR)) && (!backgrd || use_stderr)) {
    write(z, s, len);
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
        egg_memcpy(p + socklist[i].handler.sock.outbuflen, s, len);
        socklist[i].handler.sock.outbuf = p;
        socklist[i].handler.sock.outbuflen += len;
        return;
      }
      /* Try. */
      x = write(z, s, len);
      if (x == -1)
        x = 0;
      if (x < len) {
        /* Socket is full, queue it */
        socklist[i].handler.sock.outbuf = nmalloc(len - x);
        egg_memcpy(socklist[i].handler.sock.outbuf, &s[x], len - x);
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

  int z = 0, fds;
  fd_set wfds;
  struct timeval tv;

/* ^-- start poptix test code, this should avoid writes to sockets not ready to be written to. */
  fds = getdtablesize();

#ifdef FD_SETSIZE
  if (fds > FD_SETSIZE)
    fds = FD_SETSIZE;           /* Fixes YET ANOTHER freebsd bug!!! */
#endif
  FD_ZERO(&wfds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;               /* we only want to see if it's ready for writing, no need to actually wait.. */
  for (i = 0; i < threaddata()->MAXSOCKS; i++) {
    if (!(socklist[i].flags & (SOCK_UNUSED | SOCK_TCL)) &&
        socklist[i].handler.sock.outbuf != NULL) {
      FD_SET(socklist[i].sock, &wfds);
      z = 1;
    }
  }
  if (!z)
    return;                     /* nothing to write */

  select((SELECT_TYPE_ARG1) fds, SELECT_TYPE_ARG234 NULL,
         SELECT_TYPE_ARG234 &wfds, SELECT_TYPE_ARG234 NULL,
         SELECT_TYPE_ARG5 &tv);

/* end poptix */

  for (i = 0; i < threaddata()->MAXSOCKS; i++) {
    if (!(socklist[i].flags & (SOCK_UNUSED | SOCK_TCL)) &&
        (socklist[i].handler.sock.outbuf != NULL) && (FD_ISSET(socklist[i].sock, &wfds))) {
      /* Trick tputs into doing the work */
      errno = 0;
      x = write(socklist[i].sock, socklist[i].handler.sock.outbuf, socklist[i].handler.sock.outbuflen);
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
        socklist[i].handler.sock.outbuf = nmalloc(socklist[i].handler.sock.outbuflen - x);
        egg_memcpy(socklist[i].handler.sock.outbuf, p + x, socklist[i].handler.sock.outbuflen - x);
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

  char badaddress[16];
  IP ip = my_atoul(ipaddy);
  int prt = atoi(port);

  /* It is disabled HERE so we only have to check in *one* spot! */
  if (!dcc_sanitycheck)
    return 1;

  if (prt < 1) {
    putlog(LOG_MISC, "*", "ALERT: (%s!%s) specified an impossible port of %u!",
           nick, from, prt);
    return 0;
  }
  sprintf(badaddress, "%u.%u.%u.%u", (ip >> 24) & 0xff, (ip >> 16) & 0xff,
          (ip >> 8) & 0xff, ip & 0xff);
  if (ip < (1 << 24)) {
    putlog(LOG_MISC, "*", "ALERT: (%s!%s) specified an impossible IP of %s!",
           nick, from, badaddress);
    return 0;
  }
  return 1;
}

int hostsanitycheck_dcc(char *nick, char *from, IP ip, char *dnsname,
                        char *prt)
{
  /* According to the latest RFC, the clients SHOULD be able to handle
   * DNS names that are up to 255 characters long.  This is not broken.
   */
  char hostn[256], badaddress[16];

  /* It is disabled HERE so we only have to check in *one* spot! */
  if (!dcc_sanitycheck)
    return 1;
  sprintf(badaddress, "%u.%u.%u.%u", (ip >> 24) & 0xff, (ip >> 16) & 0xff,
          (ip >> 8) & 0xff, ip & 0xff);
  /* These should pad like crazy with zeros, since 120 bytes or so is
   * where the routines providing our data currently lose interest. I'm
   * using the n-variant in case someone changes that...
   */
  strncpyz(hostn, extracthostname(from), sizeof hostn);
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

/* Checks wether the referenced socket has data queued.
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
 * and flushs the buffer if possible
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
