/* 
 * net.c -- handles:
 *   all raw network i/o
 * 
 * $Id: net.c,v 1.49 2002/11/06 03:56:43 wcc Exp $
 */
/* 
 * This is hereby released into the public domain.
 * Robey Pointer, robey@netcom.com
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
#include <arpa/inet.h>		/* is this really necessary? */
#include <errno.h>
#if HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <setjmp.h>

#if !HAVE_GETDTABLESIZE
#  ifdef FD_SETSIZE
#    define getdtablesize() FD_SETSIZE
#  else
#    define getdtablesize() 200
#  endif
#endif

extern struct dcc_t	*dcc;
extern int		 backgrd, use_stderr, resolve_timeout, dcc_total;
extern unsigned long	 otraffic_irc_today, otraffic_bn_today,
			 otraffic_dcc_today, otraffic_filesys_today,
			 otraffic_trans_today, otraffic_unknown_today;

char	hostname[121] = "";	/* Hostname can be specified in the config
				   file					    */
char	myip[121] = "";		/* IP can be specified in the config file   */
char	firewall[121] = "";	/* Socks server for firewall		    */
int	firewallport = 1080;	/* Default port of Sock4/5 firewalls	    */
char	botuser[21] = "eggdrop"; /* Username of the user running the bot    */
int	dcc_sanitycheck = 0;	/* We should do some sanity checking on dcc
				   connections.				    */
sock_list *socklist = NULL;	/* Enough to be safe			    */
int	MAXSOCKS = 0;
jmp_buf	alarmret;		/* Env buffer for alarm() returns	    */

/* Types of proxy */
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

/* Initialize the socklist
 */
void init_net()
{
  int i;

  for (i = 0; i < MAXSOCKS; i++) {
    socklist[i].flags = SOCK_UNUSED;
  }
}

int expmem_net()
{
  int i, tot = 0;

  for (i = 0; i < MAXSOCKS; i++) {
    if (!(socklist[i].flags & SOCK_UNUSED)) {
      if (socklist[i].inbuf != NULL)
	tot += strlen(socklist[i].inbuf) + 1;
      if (socklist[i].outbuf != NULL)
	tot += socklist[i].outbuflen;
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

  /* Could be pre-defined */
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
    strcpy(s, "Address invalid on remote machine");
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
    strcpy(s, "Namespace segment violation");
    break;
  case EINPROGRESS:
    strcpy(s, "Operation in progress");
    break;
  case EINTR:
    strcpy(s, "Timeout");
    break;
  case EINVAL:
    strcpy(s, "Invalid namespace");
    break;
  case EISCONN:
    strcpy(s, "Socket already connected");
    break;
  case ENETUNREACH:
    strcpy(s, "Network unreachable");
    break;
  case ENOTSOCK:
    strcpy(s, "File descriptor, not a socket");
    break;
  case ETIMEDOUT:
    strcpy(s, "Connection timed out");
    break;
  case ENOTCONN:
    strcpy(s, "Socket is not connected");
    break;
  case EHOSTUNREACH:
    strcpy(s, "Host is unreachable");
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

  for (i = 0; i < MAXSOCKS; i++)
    if ((socklist[i].sock == sock) && !(socklist[i].flags & SOCK_UNUSED)) {
      if (operation == EGG_OPTION_SET)
	      socklist[i].flags |= sock_options;
      else if (operation == EGG_OPTION_UNSET)
	      socklist[i].flags &= ~sock_options;
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

  for (i = 0; i < MAXSOCKS; i++) {
    if (socklist[i].flags & SOCK_UNUSED) {
      /* yay!  there is table space */
      socklist[i].inbuf = socklist[i].outbuf = NULL;
      socklist[i].inbuflen = socklist[i].outbuflen = 0;
      socklist[i].flags = options;
      socklist[i].sock = sock;
      return i;
    }
  }
  fatal("Socket table is full!", 0);
  return -1; /* Never reached */
}

/* Request a normal socket for i/o
 */
void setsock(int sock, int options)
{
  int i = allocsock(sock, options);
  int parm;

  if (((sock != STDOUT) || backgrd) &&
      !(socklist[i].flags & SOCK_NONSOCK)) {
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
  register int	i;

  /* Ignore invalid sockets.  */
  if (sock < 0)
    return;
  for (i = 0; i < MAXSOCKS; i++) {
    if ((socklist[i].sock == sock) && !(socklist[i].flags & SOCK_UNUSED)) {
      close(socklist[i].sock);
      if (socklist[i].inbuf != NULL) {
	nfree(socklist[i].inbuf);
	socklist[i].inbuf = NULL;
      }
      if (socklist[i].outbuf != NULL) {
	nfree(socklist[i].outbuf);
	socklist[i].outbuf = NULL;
	socklist[i].outbuflen = 0;
      }
      socklist[i].flags = SOCK_UNUSED;
      return;
    }
  }
  putlog(LOG_MISC, "*", "Attempt to kill un-allocated socket %d !!", sock);
}

/* convert a socklist sock idx to dcc idx
*/
static int sock_to_dcc(int sock)
{
  int i;
  
  for (i = 0; i < dcc_total; i++)
    if (sock == dcc[i].sock)
      return i;
  return -1;
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
      IP ip = ((IP) inet_addr(host)); /* drummer */      
      egg_memcpy(x, &ip, 4);	/* Beige@Efnet */
    } else {
      /* no, must be host.domain */
      if (!setjmp(alarmret)) {
	alarm(resolve_timeout);
	hp = gethostbyname(host);
	alarm(0);
      } else {
	hp = NULL;
      }
      if (hp == NULL) {
	killsock(sock);
	return -2;
      }
      egg_memcpy(x, hp->h_addr, hp->h_length);
    }
    for (i = 0; i < MAXSOCKS; i++)
      if (!(socklist[i].flags & SOCK_UNUSED) && socklist[i].sock == sock)
	socklist[i].flags |= SOCK_PROXYWAIT; /* drummer */
    egg_snprintf(s, sizeof s, "\004\001%c%c%c%c%c%c%s", (port >> 8) % 256,
		 (port % 256), x[0], x[1], x[2], x[3], botuser);
    tputs(sock, s, strlen(botuser) + 9); /* drummer */
  } else if (proxy == PROXY_SUN) {
    egg_snprintf(s, sizeof s, "%s %d\n", host, port);
    tputs(sock, s, strlen(s)); /* drummer */
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
  int i, port;
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
  /* patch by tris for multi-hosted machines: */
  egg_bzero((char *) &name, sizeof(struct sockaddr_in));

  name.sin_family = AF_INET;
  name.sin_addr.s_addr = (myip[0] ? getmyip() : INADDR_ANY);
  if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0)
    return -1;
  egg_bzero((char *) &name, sizeof(struct sockaddr_in));

  name.sin_family = AF_INET;
  name.sin_port = htons(port);
  /* Numeric IP? */
  if ((host[strlen(host) - 1] >= '0') && (host[strlen(host) - 1] <= '9'))
    name.sin_addr.s_addr = inet_addr(host);
  else {
    /* No, must be host.domain */
    debug0("WARNING: open_telnet_raw() is about to block in gethostbyname()!");
    if (!setjmp(alarmret)) {
      alarm(resolve_timeout);
      hp = gethostbyname(host);
      alarm(0);
    } else {
      hp = NULL;
    }
    if (hp == NULL)
      return -2;
    egg_memcpy(&name.sin_addr, hp->h_addr, hp->h_length);
    name.sin_family = hp->h_addrtype;
  }
  for (i = 0; i < MAXSOCKS; i++) {
    if (!(socklist[i].flags & SOCK_UNUSED) && (socklist[i].sock == sock))
      socklist[i].flags = (socklist[i].flags & ~SOCK_VIRTUAL) | SOCK_CONNECT;
  }
  if (connect(sock, (struct sockaddr *) &name,
	      sizeof(struct sockaddr_in)) < 0) {
    if (errno == EINPROGRESS) {
      /* Firewall?  announce connect attempt to proxy */
      if (firewall[0])
	return proxy_connect(sock, server, sport, proxy);
      return sock;		/* async success! */
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
  int sock = getsock(0),
      ret = open_telnet_raw(sock, server, port);

  if (ret < 0)
    killsock(sock);
  return ret;
}

/* Returns a socket number for a listening socket that will accept any
 * connection on a certain address -- port # is returned in port
 */
int open_address_listen(IP addr, int *port)
{
  int sock;
  unsigned int addrlen;
  struct sockaddr_in name;

  if (firewall[0]) {
    /* FIXME: can't do listen port thru firewall yet */
    putlog(LOG_MISC, "*", "!! Cant open a listen port (you are using a firewall)");
    return -1;
  }

  sock = getsock(SOCK_LISTEN);
  if (sock < 1)
    return -1;

  egg_bzero((char *) &name, sizeof(struct sockaddr_in));
  name.sin_family = AF_INET;
  name.sin_port = htons(*port);	/* 0 = just assign us a port */
  name.sin_addr.s_addr = addr;
  if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
    killsock(sock);
    if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT)
      return -2;
    else
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
  return sock;
}

/* Returns a socket number for a listening socket that will accept any
 * connection -- port # is returned in port
 */
inline int open_listen(int *port)
{
  return open_address_listen(myip[0] ? getmyip() : INADDR_ANY, port);
}

/* Given a network-style IP address, returns the hostname. The hostname
 * will be in the "##.##.##.##" format if there was an error.
 * 
 * NOTE: This function is depreciated. Try using the async dns approach
 *       instead.
 */
char *hostnamefromip(unsigned long ip)
{
  struct hostent *hp;
  unsigned long addr = ip;
  unsigned char *p;
  static char s[UHOSTLEN];

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
    return s;
  }
  strncpyz(s, hp->h_name, sizeof s);
  return s;
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
  unsigned int addrlen;
  struct sockaddr_in from;

  addrlen = sizeof(struct sockaddr);
  new_sock = accept(sock, (struct sockaddr *) &from, &addrlen);
  if (new_sock < 0)
    return -1;
  if (ip != NULL) {
    *ip = from.sin_addr.s_addr;
    /* This is now done asynchronously. We now only provide the IP address.
     *
     * strncpy(caller, hostnamefromip(*ip), 120);
     */
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
  char sv[121];
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
    return -3;			/* fake address */
  c[0] = (addr >> 24) & 0xff;
  c[1] = (addr >> 16) & 0xff;
  c[2] = (addr >> 8) & 0xff;
  c[3] = addr & 0xff;
  sprintf(sv, "%u.%u.%u.%u", c[0], c[1], c[2], c[3]);
  /* strcpy(sv,hostnamefromip(addr)); */
  p = open_telnet_raw(sock, sv, p);
  return p;
}

/* Attempts to read from all the sockets in socklist
 * fills s with up to 511 bytes if available, and returns the array index
 * 
 * 		on EOF:  returns -1, with socket in len
 *     on socket error:  returns -2
 * if nothing is ready:  returns -3
 */
static int sockread(char *s, int *len)
{
  fd_set fd;
  int fds, i, x, fdtmp;
  struct timeval t;
  int grab = 511;

  fds = getdtablesize();
#ifdef FD_SETSIZE
  if (fds > FD_SETSIZE)
    fds = FD_SETSIZE;		/* Fixes YET ANOTHER freebsd bug!!! */
#endif
  /* timeout: 1 sec */
  t.tv_sec = 1;
  t.tv_usec = 0;
  FD_ZERO(&fd);
  
  for (i = 0; i < MAXSOCKS; i++)
    if (!(socklist[i].flags & (SOCK_UNUSED | SOCK_VIRTUAL))) {
      if ((socklist[i].sock == STDOUT) && !backgrd)
	fdtmp = STDIN;
      else
	fdtmp = socklist[i].sock;
      /* 
       * Looks like that having more than a call, in the same
       * program, to the FD_SET macro, triggers a bug in gcc.
       * SIGBUS crashing binaries used to be produced on a number
       * (prolly all?) of 64 bits architectures.
       * Make your best to avoid to make it happen again.
       * 
       * ITE
       */
      FD_SET(fdtmp , &fd);
    }
#ifdef HPUX_HACKS
#ifndef HPUX10_HACKS
  x = select(fds, (int *) &fd, (int *) NULL, (int *) NULL, &t);
#else
  x = select(fds, &fd, NULL, NULL, &t);
#endif
#else
  x = select(fds, &fd, NULL, NULL, &t);
#endif
  if (x > 0) {
    /* Something happened */
    for (i = 0; i < MAXSOCKS; i++) {
      if ((!(socklist[i].flags & SOCK_UNUSED)) &&
	  ((FD_ISSET(socklist[i].sock, &fd)) ||
	   ((socklist[i].sock == STDOUT) && (!backgrd) &&
	    (FD_ISSET(STDIN, &fd))))) {
	if (socklist[i].flags & (SOCK_LISTEN | SOCK_CONNECT)) {
	  /* Listening socket -- don't read, just return activity */
	  /* Same for connection attempt */
	  /* (for strong connections, require a read to succeed first) */
	  if (socklist[i].flags & SOCK_PROXYWAIT) { /* drummer */
	    /* Hang around to get the return code from proxy */
	    grab = 10;
	  } else if (!(socklist[i].flags & SOCK_STRONGCONN)) {
	    debug1("net: connect! sock %d", socklist[i].sock);
	    s[0] = 0;
	    *len = 0;
	    return i;
	  }
	} else if (socklist[i].flags & SOCK_PASS) {
	  s[0] = 0;
	  *len = 0;
	  return i;
	}
	errno = 0;
	if ((socklist[i].sock == STDOUT) && !backgrd)
	  x = read(STDIN, s, grab);
	else
	  x = read(socklist[i].sock, s, grab);
	if (x <= 0) {		/* eof */
	  if (errno != EAGAIN) { /* EAGAIN happens when the operation would block 
				    on a non-blocking socket, if the socket is going
				    to die, it will die later, otherwise it will connect */
	    *len = socklist[i].sock;
	    socklist[i].flags &= ~SOCK_CONNECT;
	    debug1("net: eof!(read) socket %d", socklist[i].sock);
	    return -1;
	  } else {
	    debug3("sockread EAGAIN: %d %d (%s)",socklist[i].sock,errno,strerror(errno));
	    continue; /* EAGAIN */
	  }
	}
	s[x] = 0;
	*len = x;
	if (socklist[i].flags & SOCK_PROXYWAIT) {
	  debug2("net: socket: %d proxy errno: %d", socklist[i].sock, s[1]);
	  socklist[i].flags &= ~(SOCK_CONNECT | SOCK_PROXYWAIT);
	  switch (s[1]) {
	  case 90:		/* Success */
	    s[0] = 0;
	    *len = 0;
	    return i;
	  case 91:		/* Failed */
	    errno = ECONNREFUSED;
	    break;
	  case 92:		/* No identd */
	  case 93:		/* Identd said wrong username */
	    /* A better error message would be "socks misconfigured"
	     * or "identd not working" but this is simplest.
	     */
	    errno = ENETUNREACH;
	    break;
	  }
	  *len = socklist[i].sock;
	  return -1;
	}
	return i;
      }
    }
  } else if (x == -1)
    return -2;			/* socket error */
  else {
    s[0] = 0;
    *len = 0;
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
 */

int sockgets(char *s, int *len)
{
  char xx[514], *p, *px;
  int ret, i, j, data = 0;
  int chk = 0;

  for (i = 0; i < MAXSOCKS; i++) {
    j = sock_to_dcc(socklist[i].sock);
    /* Check for stored-up data waiting to be processed */
    if (!(socklist[i].flags & SOCK_UNUSED) &&
	!(socklist[i].flags & SOCK_BUFFER) && (socklist[i].inbuf != NULL)) {
      if (!(socklist[i].flags & SOCK_BINARY)) {
	/* look for \r too cos windows can't follow RFCs */
        while (chk == 0) {
	  p = strchr(socklist[i].inbuf, '\n');
	  if (p == NULL)
	    p = strchr(socklist[i].inbuf, '\r');
	  if (p != NULL) {
	    *p = 0;
	    if (strlen(socklist[i].inbuf) > 510)
	      socklist[i].inbuf[510] = 0;
	    strcpy(s, socklist[i].inbuf);
	    px = (char *) nmalloc(strlen(p + 1) + 1);
	    strcpy(px, p + 1);
	    nfree(socklist[i].inbuf);
	    if (px[0])
	      socklist[i].inbuf = px;
	    else {
	      nfree(px);
	      socklist[i].inbuf = NULL;
	    }
	    /* Strip CR if this was CR/LF combo */
	    if (s[strlen(s) - 1] == '\r')
	      s[strlen(s) - 1] = 0;
            /* if s is null, we can't use it for connect/control...-sL */
            if ((j != -1) && (dcc[j].type == &DCC_SCRIPT) && ((s == NULL) || (s[0] == 0))) {
              if (socklist[i].inbuf == NULL)
                chk = 1;
            } else
              chk = 1;
            if (chk) {
              *len = strlen(s);
       	      return socklist[i].sock;
  	    }
          } else {
            if ((j != -1) && (dcc[j].type == &DCC_SCRIPT) && ((s == NULL) || (s[0] == 0))) {
              if (socklist[i].inbuf != NULL) {
                strcpy(s, socklist[i].inbuf);
                *len = strlen(s);
                nfree(socklist[i].inbuf);
                socklist[i].inbuf = NULL;
                socklist[i].inbuflen = 0;
                return socklist[i].sock;
              }
            }
            chk = 1;
          }
        }
      } else {
	/* Handling buffered binary data (must have been SOCK_BUFFER before). */
	if (socklist[i].inbuflen <= 510) {
	  *len = socklist[i].inbuflen;
	  egg_memcpy(s, socklist[i].inbuf, socklist[i].inbuflen);
	  nfree(socklist[i].inbuf);
          socklist[i].inbuf = NULL;
	  socklist[i].inbuflen = 0;
	} else {
	  /* Split up into chunks of 510 bytes. */
	  *len = 510;
	  egg_memcpy(s, socklist[i].inbuf, *len);
	  egg_memcpy(socklist[i].inbuf, socklist[i].inbuf + *len, *len);
	  socklist[i].inbuflen -= *len;
	  socklist[i].inbuf = nrealloc(socklist[i].inbuf,
				       socklist[i].inbuflen);
	}
	return socklist[i].sock;
      }
    }
    /* Also check any sockets that might have EOF'd during write */
    if (!(socklist[i].flags & SOCK_UNUSED)
	&& (socklist[i].flags & SOCK_EOFD)) {
      s[0] = 0;
      *len = socklist[i].sock;
      return -1;
    }
  }
  /* No pent-up data of any worth -- down to business */
  *len = 0;
  ret = sockread(xx, len);
  if (ret < 0) {
    s[0] = 0;
    return ret;
  }
  /* Binary, listening and passed on sockets don't get buffered. */
  if (socklist[ret].flags & SOCK_CONNECT) {
    if (socklist[ret].flags & SOCK_STRONGCONN) {
      socklist[ret].flags &= ~SOCK_STRONGCONN;
      /* Buffer any data that came in, for future read. */
      socklist[ret].inbuflen = *len;
      socklist[ret].inbuf = (char *) nmalloc(*len + 1);
      /* It might be binary data. You never know. */
      egg_memcpy(socklist[ret].inbuf, xx, *len);
      socklist[ret].inbuf[*len] = 0;
    }
    socklist[ret].flags &= ~SOCK_CONNECT;
    s[0] = 0;
    return socklist[ret].sock;
  }
  if (socklist[ret].flags & SOCK_BINARY) {
    egg_memcpy(s, xx, *len);
    return socklist[ret].sock;
  }
  if ((socklist[ret].flags & SOCK_LISTEN) ||
      (socklist[ret].flags & SOCK_PASS))
    return socklist[ret].sock;
  if (socklist[ret].flags & SOCK_BUFFER) {
    socklist[ret].inbuf = (char *) nrealloc(socklist[ret].inbuf,
		    			    socklist[ret].inbuflen + *len + 1);
    egg_memcpy(socklist[ret].inbuf + socklist[ret].inbuflen, xx, *len);
    socklist[ret].inbuflen += *len;
    /* We don't know whether it's binary data. Make sure normal strings
       will be handled properly later on too. */
    socklist[ret].inbuf[socklist[ret].inbuflen] = 0;
    return -4;	/* Ignore this one. */
  }
  /* Might be necessary to prepend stored-up data! */
  if (socklist[ret].inbuf != NULL) {
    p = socklist[ret].inbuf;
    socklist[ret].inbuf = (char *) nmalloc(strlen(p) + strlen(xx) + 1);
    strcpy(socklist[ret].inbuf, p);
    strcat(socklist[ret].inbuf, xx);
    nfree(p);
    if (strlen(socklist[ret].inbuf) < 512) {
      strcpy(xx, socklist[ret].inbuf);
      nfree(socklist[ret].inbuf);
      socklist[ret].inbuf = NULL;
      socklist[ret].inbuflen = 0;
    } else {
      p = socklist[ret].inbuf;
      socklist[ret].inbuflen = strlen(p) - 510;
      socklist[ret].inbuf = (char *) nmalloc(socklist[ret].inbuflen + 1);
      strcpy(socklist[ret].inbuf, p + 510);
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
    strcpy(xx, p + 1);
    if (s[strlen(s) - 1] == '\r')
      s[strlen(s) - 1] = 0;
    data = 1;			/* DCC_CHAT may now need to process a
				   blank line */
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
  j = sock_to_dcc(socklist[ret].sock);
  if (socklist[ret].inbuf != NULL) {
    p = socklist[ret].inbuf;
    socklist[ret].inbuflen = strlen(p) + strlen(xx);
    socklist[ret].inbuf = (char *) nmalloc(socklist[ret].inbuflen + 1);
    strcpy(socklist[ret].inbuf, xx);
    strcat(socklist[ret].inbuf, p);
    nfree(p);
  } else {
    if ((dcc[j].type == &DCC_SCRIPT) && (xx[0] || xx[1]) && (data != 1))
      data = 1;
    socklist[ret].inbuflen = strlen(xx);
    socklist[ret].inbuf = (char *) nmalloc(socklist[ret].inbuflen + 1);
    strcpy(socklist[ret].inbuf, xx);
  }
  if (data) {
    if ((j != -1) && (dcc[j].type == &DCC_SCRIPT) && ((s == NULL) || (s[0] == 0))) {
      if (socklist[ret].inbuf != NULL) {
        strcpy(s, socklist[ret].inbuf);
        *len = strlen(s);
        nfree(socklist[ret].inbuf);
        socklist[ret].inbuf = NULL;
        socklist[ret].inbuflen = 0;
      }
    }
    return socklist[ret].sock;
  } else {
    return -3;
  }
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

  if (z < 0)
    return;			/* um... HELLO?!  sanity check please! */
  if (((z == STDOUT) || (z == STDERR)) && (!backgrd || use_stderr)) {
    write(z, s, len);
    return;
  }
  for (i = 0; i < MAXSOCKS; i++) {
    if (!(socklist[i].flags & SOCK_UNUSED) && (socklist[i].sock == z)) {
      for (idx = 0; idx < dcc_total; idx++) {
        if (dcc[idx].sock == z) {
          if (dcc[idx].type) {
            if (dcc[idx].type->name) {
              if (!strncmp(dcc[idx].type->name, "BOT", 3)) {
                otraffic_bn_today += len;
                break;
              } else if (!strcmp(dcc[idx].type->name, "SERVER")) {
                otraffic_irc_today += len;
                break;
              } else if (!strncmp(dcc[idx].type->name, "CHAT", 4)) {
                otraffic_dcc_today += len;
                break;
              } else if (!strncmp(dcc[idx].type->name, "FILES", 5)) {
                otraffic_filesys_today += len;
                break;
              } else if (!strcmp(dcc[idx].type->name, "SEND")) {
                otraffic_trans_today += len;
                break;
              } else if (!strncmp(dcc[idx].type->name, "GET", 3)) {
                otraffic_trans_today += len;
                break;
              } else {
                otraffic_unknown_today += len;
                break;
              }
            }
          }
        }
      }
      
      if (socklist[i].outbuf != NULL) {
	/* Already queueing: just add it */
	p = (char *) nrealloc(socklist[i].outbuf, socklist[i].outbuflen + len);
	egg_memcpy(p + socklist[i].outbuflen, s, len);
	socklist[i].outbuf = p;
	socklist[i].outbuflen += len;
	return;
      }
      /* Try. */
      x = write(z, s, len);
      if (x == (-1))
	x = 0;
      if (x < len) {
	/* Socket is full, queue it */
	socklist[i].outbuf = (char *) nmalloc(len - x);
	egg_memcpy(socklist[i].outbuf, &s[x], len - x);
	socklist[i].outbuflen = len - x;
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

  int z=0, fds;
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
  tv.tv_usec = 0; /* we only want to see if it's ready for writing, no need to actually wait.. */
  for (i = 0; i < MAXSOCKS; i++) { 
    if (!(socklist[i].flags & SOCK_UNUSED) &&
	socklist[i].outbuf != NULL) {
	  FD_SET(socklist[i].sock, &wfds);
	  z=1; 
	}
  }
  if (!z) { 
	return; /* nothing to write */
  }
#ifdef HPUX_HACKS
 #ifndef HPUX10_HACKS
  select(fds, (int *) NULL, (int *) &wfds, (int *) NULL, &tv);
 #else
  select(fds, NULL, &wfds, NULL, &tv);
 #endif
#else
  select(fds, NULL, &wfds, NULL, &tv);
#endif

/* end poptix */

  for (i = 0; i < MAXSOCKS; i++) { 
    if (!(socklist[i].flags & SOCK_UNUSED) &&
	(socklist[i].outbuf != NULL) && (FD_ISSET(socklist[i].sock, &wfds))) {
      /* Trick tputs into doing the work */
      errno = 0;
      x = write(socklist[i].sock, socklist[i].outbuf,
		socklist[i].outbuflen);
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
      } else if (x == socklist[i].outbuflen) {
	/* If the whole buffer was sent, nuke it */
	nfree(socklist[i].outbuf);
	socklist[i].outbuf = NULL;
	socklist[i].outbuflen = 0;
      } else if (x > 0) {
	char *p = socklist[i].outbuf;

	/* This removes any sent bytes from the beginning of the buffer */
	socklist[i].outbuf = (char *) nmalloc(socklist[i].outbuflen - x);
	egg_memcpy(socklist[i].outbuf, p + x, socklist[i].outbuflen - x);
	socklist[i].outbuflen -= x;
	nfree(p);
      } else {
	debug3("dequeue_sockets(): errno = %d (%s) on %d",errno,strerror(errno),socklist[i].sock);
      }
      /* All queued data was sent. Call handler if one exists and the
       * dcc entry wants it.
       */
      if (!socklist[i].outbuf) {
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
  for (i = 0; i < MAXSOCKS; i++) {
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
      if (socklist[i].inbuf != NULL)
	sprintf(&s[strlen(s)], " (inbuf: %04X)", strlen(socklist[i].inbuf));
      if (socklist[i].outbuf != NULL)
	sprintf(&s[strlen(s)], " (outbuf: %06lX)", socklist[i].outbuflen);
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
    return 1; /* <- usually happens when we have 
		    a user with an unresolved hostmask! */
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

  for (i = 0; i < MAXSOCKS; i++)
    if (!(socklist[i].flags & SOCK_UNUSED) && socklist[i].sock == sock)
      break;
  if (i < MAXSOCKS) {
    switch (type) {
      case SOCK_DATA_OUTGOING:
	ret = (socklist[i].outbuf != NULL);
	break;
      case SOCK_DATA_INCOMING:
	ret = (socklist[i].inbuf != NULL);
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
  for (i = 0; i < MAXSOCKS; i++) {
    if ((dcc[idx].sock == socklist[i].sock)
        && !(socklist[i].flags & SOCK_UNUSED)) {
      len = socklist[i].inbuflen;
      if ((len > 0) && socklist[i].inbuf) {
        if (dcc[idx].type && dcc[idx].type->activity) {
          inbuf = socklist[i].inbuf;
          socklist[i].inbuf = NULL;
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
