/*
 * dcc.c -- handles:
 *   activity on a dcc socket
 *   disconnect on a dcc socket
 *   ...and that's it!  (but it's a LOT)
 *
 * $Id: dcc.c,v 1.95 2011/10/02 18:21:35 pseudo Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
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

#include "main.h"
#include <ctype.h>
#include <errno.h>
#include "modules.h"
#include "tandem.h"

/* Includes for botnet md5 challenge/response code <cybah> */
#include "md5/md5.h"

extern struct userrec *userlist;
extern struct chanset_t *chanset;
extern Tcl_Interp *interp;
extern time_t now;
extern char botnetnick[], ver[], origbotname[], notify_new[];
extern int egg_numver, connect_timeout, conmask, backgrd, max_dcc,
           make_userfile, default_flags, raw_log, ignore_time,
           par_telnet_flood;

struct dcc_t *dcc = NULL;       /* DCC list                                */
int dcc_total = 0;              /* Total dcc's                             */
char tempdir[121] = "";         /* Temporary directory
                                 * (default: current directory)            */
int require_p = 0;              /* Require 'p' access to get on the
                                 * party line?                             */
int allow_new_telnets = 0;      /* Allow people to introduce themselves
                                 * via telnet                              */
int stealth_telnets = 0;        /* Be paranoid? <cybah>                    */
int use_telnet_banner = 0;      /* Display telnet banner?                  */
char network[41] = "unknown-net";       /* Name of the IRC network you're on  */
int password_timeout = 180;     /* Time to wait for a password from a user */
int bot_timeout = 60;           /* Bot timeout value                       */
int identtimeout = 5;           /* Timeout value for ident lookups         */
int dupwait_timeout = 5;        /* Timeout for rejecting duplicate entries */
int protect_telnet = 1;         /* Even bother with ident lookups :)       */
int flood_telnet_thr = 5;       /* Number of telnet connections to be
                                 * considered a flood                      */
int flood_telnet_time = 60;     /* In how many seconds?                    */
char bannerfile[121] = "text/banner";   /* File displayed on telnet login */

static void dcc_telnet_hostresolved(int);
static void dcc_telnet_got_ident(int, char *);
static void dcc_telnet_pass(int, int);


/* This is not a universal telnet detector. You need to send WILL STATUS to the
 * other side and pass the reply to this function. A telnet client will respond
 * to this with either DO or DONT STATUS.
 */
static int detect_telnet(unsigned char *buf)
{
  if (!buf || !buf[0] || !buf[1])
    return 0;
  while (buf[2]) {
    if (buf[0] == TLN_IAC && (buf[1] == TLN_DO || buf[1] == TLN_DONT) &&
        buf[2] == TLN_STATUS)
      return 1;
    buf++;
  }
  return 0;
}

/* Escape telnet IAC and prepend CR to LF */
static char *escape_telnet(char *s)
{
  static char buf[1024];
  char *p;

  for (p = buf; *s && (p < (buf + sizeof(buf) - 2)); *p++ = *s++)
    if ((unsigned char) *s == TLN_IAC)
      *p++ = *s;
    else if (*s == '\n')
      *p++ = '\r';
  *p = 0;

  return buf;
}

static void strip_telnet(int sock, char *buf, int *len)
{
  unsigned char *p = (unsigned char *) buf, *o = (unsigned char *) buf;
  int mark;

  while (*p != 0) {
    while ((*p != TLN_IAC) && (*p != 0))
      *o++ = *p++;
    if (*p == TLN_IAC) {
      p++;
      mark = 2;
      if (!*p)
        mark = 1;               /* bogus */
      if ((*p >= TLN_WILL) && (*p <= TLN_DONT)) {
        mark = 3;
        if (!*(p + 1))
          mark = 2;             /* bogus */
      } else if (*p == TLN_WILL) {
        /* WILL X -> response: DONT X */
        /* except WILL ECHO which we just smile and ignore */
        if (*(p + 1) != TLN_ECHO) {
          write(sock, TLN_IAC_C TLN_DONT_C, 2);
          write(sock, p + 1, 1);
        }
      } else if (*p == TLN_DO) {
        /* DO X -> response: WONT X */
        /* except DO ECHO which we just smile and ignore */
        if (*(p + 1) != TLN_ECHO) {
          write(sock, TLN_IAC_C TLN_WONT_C, 2);
          write(sock, p + 1, 1);
        }
      } else if (*p == TLN_AYT) {
        /* "Are You There?" */
        /* response is: "Hell, yes!" */
        write(sock, "\r\nHell, yes!\r\n", 14);
      } else if (*p == TLN_IAC) {
        /* IAC character in data, escaped with another IAC */
        *o++ = *p++;
        mark = 1;
      }
      /* Anything else can probably be ignored */
      p += mark - 1;
      *len = *len - mark;
    }
  }
  *o = *p;
}

static void greet_new_bot(int idx)
{
  int bfl = bot_flags(dcc[idx].user);
  int i;

  dcc[idx].timeval = now;
  dcc[idx].u.bot->version[0] = 0;
  dcc[idx].u.bot->numver = 0;
  if (bfl & BOT_REJECT) {
    putlog(LOG_BOTS, "*", DCC_REJECT, dcc[idx].nick);
    dprintf(idx, "bye %s\n", "rejected");
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return;
  }
  if (bfl & BOT_LEAF)
    dcc[idx].status |= STAT_LEAF;
  dcc[idx].status |= STAT_LINKING;
#ifndef NO_OLD_BOTNET
  dprintf(idx, "version %d %d %s <%s>\n", egg_numver, HANDLEN, ver, network);
#else
  dprintf(idx, "v %d %d %s <%s>\n", egg_numver, HANDLEN, ver, network);
#endif
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_FORK_BOT) {
      killsock(dcc[i].sock);
      lostdcc(i);
    }
}

static void bot_version(int idx, char *par)
{
  char x[1024];
  int l;

  dcc[idx].timeval = now;
  if (in_chain(dcc[idx].nick)) {
    dprintf(idx, "error Sorry, already connected.\n");
    dprintf(idx, "bye\n");
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return;
  }
  if ((par[0] >= '0') && (par[0] <= '9')) {
    char *work;

    work = newsplit(&par);
    dcc[idx].u.bot->numver = atoi(work);
  } else
    dcc[idx].u.bot->numver = 0;

#ifndef NO_OLD_BOTNET
  if (b_numver(idx) < NEAT_BOTNET) {
#if HANDLEN != 9
    putlog(LOG_BOTS, "*", "Non-matching handle lengths with %s, they use 9 "
           "characters.", dcc[idx].nick);
    dprintf(idx, "error Non-matching handle length: mine %d, yours 9\n",
            HANDLEN);
    dprintf(idx, "bye %s\n", "bad handlen");
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return;
#else
    dprintf(idx, "thisbot %s\n", botnetnick);
#endif
  } else {
#endif
    dprintf(idx, "tb %s\n", botnetnick);
    l = atoi(newsplit(&par));
    if (l != HANDLEN) {
      putlog(LOG_BOTS, "*", "Non-matching handle lengths with %s, they use %d "
             "characters.", dcc[idx].nick, l);
      dprintf(idx, "error Non-matching handle length: mine %d, yours %d\n",
              HANDLEN, l);
      dprintf(idx, "bye %s\n", "bad handlen");
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return;
    }
#ifndef NO_OLD_BOTNET
  }
#endif
  strncpyz(dcc[idx].u.bot->version, par, 120);
  putlog(LOG_BOTS, "*", DCC_LINKED, dcc[idx].nick);
  chatout("*** Linked to %s\n", dcc[idx].nick);
  botnet_send_nlinked(idx, dcc[idx].nick, botnetnick, '!',
                      dcc[idx].u.bot->numver);
  touch_laston(dcc[idx].user, "linked", now);
  dump_links(idx);
  dcc[idx].type = &DCC_BOT;
  addbot(dcc[idx].nick, dcc[idx].nick, botnetnick, '-', dcc[idx].u.bot->numver);
  check_tcl_link(dcc[idx].nick, botnetnick);
  egg_snprintf(x, sizeof x, "v %d", dcc[idx].u.bot->numver);
  bot_share(idx, x);
  dprintf(idx, "el\n");
}

void failed_link(int idx)
{
  char s[81], s1[512];

  if (dcc[idx].port >= dcc[idx].u.bot->port + 3) {
    if (dcc[idx].u.bot->linker[0]) {
      egg_snprintf(s, sizeof s, "Couldn't link to %s.", dcc[idx].nick);
      strcpy(s1, dcc[idx].u.bot->linker);
      add_note(s1, botnetnick, s, -2, 0);
    }
    if (dcc[idx].u.bot->numver >= -1)
      putlog(LOG_BOTS, "*", DCC_LINKFAIL, dcc[idx].nick);
    killsock(dcc[idx].sock);
    strcpy(s, dcc[idx].nick);
    lostdcc(idx);
    autolink_cycle(s);          /* Check for more auto-connections */
    return;
  }

  /* Try next port */
  killsock(dcc[idx].sock);
  dcc[idx].sock = getsock(SOCK_STRONGCONN);
  dcc[idx].port++;
  dcc[idx].timeval = now;
  if (dcc[idx].sock < 0 ||
      open_telnet_raw(dcc[idx].sock, dcc[idx].addr ?
                      iptostr(htonl(dcc[idx].addr)) : dcc[idx].host,
                      dcc[idx].port) < 0) {
    failed_link(idx);
  }
}

static void cont_link(int idx, char *buf, int i)
{
  char x[1024];
  int atr = bot_flags(dcc[idx].user);
  int users, bots;

  if (atr & BOT_HUB) {
    /* Disconnect all +a bots because we just got a hub */
    for (i = 0; i < dcc_total; i++) {
      if ((i != idx) && (bot_flags(dcc[i].user) & BOT_ALT)) {
        if ((dcc[i].type == &DCC_FORK_BOT) || (dcc[i].type == &DCC_BOT_NEW)) {
          killsock(dcc[i].sock);
          lostdcc(i);
        }
      }
    }
    /* Just those currently in the process of linking */
    if (in_chain(dcc[idx].nick)) {
      i = nextbot(dcc[idx].nick);
      if (i > 0) {
        bots = bots_in_subtree(findbot(dcc[idx].nick));
        users = users_in_subtree(findbot(dcc[idx].nick));
        egg_snprintf(x, sizeof x,
                     "Unlinked %s (restructure) (lost %d bot%s and %d user%s)",
                     dcc[i].nick, bots, (bots != 1) ? "s" : "",
                     users, (users != 1) ? "s" : "");
        chatout("*** %s\n", x);
        botnet_send_unlinked(i, dcc[i].nick, x);
        dprintf(i, "bye %s\n", "restructure");
        killsock(dcc[i].sock);
        lostdcc(i);
      }
    }
  }
  dcc[idx].type = &DCC_BOT_NEW;
  dcc[idx].u.bot->numver = 0;

  /* Don't send our password here, just the username. The code later on
   * will determine if the password needs to be sent in cleartext or if
   * we can send an MD5 digest. <cybah>
   */
  dprintf(idx, "%s\n", botnetnick);
  return;
}

/* This function generates a digest by combining 'challenge' with
 * 'password' and then sends it to the other bot. <Cybah>
 */
static void dcc_bot_digest(int idx, char *challenge, char *password)
{
  MD5_CTX md5context;
  char digest_string[33];       /* 32 for digest in hex + null */
  unsigned char digest[16];
  int i;

  MD5_Init(&md5context);
  MD5_Update(&md5context, (unsigned char *) challenge, strlen(challenge));
  MD5_Update(&md5context, (unsigned char *) password, strlen(password));
  MD5_Final(digest, &md5context);

  for (i = 0; i < 16; i++)
    sprintf(digest_string + (i * 2), "%.2x", digest[i]);
  dprintf(idx, "digest %s\n", digest_string);
  putlog(LOG_BOTS, "*", "Received challenge from %s... sending response ...",
         dcc[idx].nick);
}

static void dcc_bot_new(int idx, char *buf, int x)
{
  struct userrec *u = get_user_by_handle(userlist, dcc[idx].nick);
  char *code;

  code = newsplit(&buf);
  if (!egg_strcasecmp(code, "*hello!"))
    greet_new_bot(idx);
  else if (!egg_strcasecmp(code, "version") || !egg_strcasecmp(code, "v"))
    bot_version(idx, buf);
  else if (!egg_strcasecmp(code, "badpass"))
    /* We entered the wrong password */
    putlog(LOG_BOTS, "*", DCC_BADPASS, dcc[idx].nick);
  else if (!egg_strcasecmp(code, "passreq")) {
    char *pass = get_user(&USERENTRY_PASS, u);

    if (!pass || !strcmp(pass, "-")) {
      putlog(LOG_BOTS, "*", DCC_PASSREQ, dcc[idx].nick);
      dprintf(idx, "-\n");
    } else {
      /* Determine if the other end supports an MD5 digest instead of a
       * cleartext password. <Cybah>
       */
      if (buf && buf[0] && strchr(buf, '<') && strchr(buf + 1, '>'))
        dcc_bot_digest(idx, buf, pass);
      else
        dprintf(idx, "%s\n", pass);
    }
  } else if (!egg_strcasecmp(code, "error"))
    putlog(LOG_BOTS, "*", DCC_LINKERROR, dcc[idx].nick, buf);
  /* Ignore otherwise */
}

static void eof_dcc_bot_new(int idx)
{
  putlog(LOG_BOTS, "*", DCC_LOSTBOT, dcc[idx].nick, dcc[idx].port);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void timeout_dcc_bot_new(int idx)
{
  putlog(LOG_BOTS, "*", DCC_TIMEOUT, dcc[idx].nick,
         dcc[idx].host, dcc[idx].port);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void display_dcc_bot_new(int idx, char *buf)
{
  long tv;

  tv = now - dcc[idx].timeval;
  sprintf(buf, "bot*  waited %lis", tv);
}

static int expmem_dcc_bot_(void *x)
{
  return sizeof(struct bot_info);
}

static void free_dcc_bot_(int n, void *x)
{
  if (dcc[n].type == &DCC_BOT) {
    unvia(n, findbot(dcc[n].nick));
    rembot(dcc[n].nick);
  }
  nfree(x);
}

struct dcc_table DCC_BOT_NEW = {
  "BOT_NEW",
  0,
  eof_dcc_bot_new,
  dcc_bot_new,
  &bot_timeout,
  timeout_dcc_bot_new,
  display_dcc_bot_new,
  expmem_dcc_bot_,
  free_dcc_bot_,
  NULL
};

/* Hash function for tandem bot commands */
extern botcmd_t C_bot[];

static void dcc_bot(int idx, char *code, int i)
{
  char *msg;
  int f;

  if (raw_log) {
    if (code[0] == 's')
      putlog(LOG_BOTSHARE, "*", "{%s} %s", dcc[idx].nick, code + 2);
    else
      putlog(LOG_BOTNET, "*", "[%s] %s", dcc[idx].nick, code);
  }
  msg = strchr(code, ' ');
  if (msg) {
    *msg = 0;
    msg++;
  } else
    msg = "";
  for (f = i = 0; C_bot[i].name && !f; i++) {
    int y = egg_strcasecmp(code, C_bot[i].name);

    if (!y) {
      /* Found a match */
      (C_bot[i].func) (idx, msg);
      f = 1;
    } else if (y < 0)
      return;
  }
}

static void eof_dcc_bot(int idx)
{
  char x[1024];
  int bots, users;

  bots = bots_in_subtree(findbot(dcc[idx].nick));
  users = users_in_subtree(findbot(dcc[idx].nick));
  egg_snprintf(x, sizeof x,
               "Lost bot: %s (lost %d bot%s and %d user%s)",
               dcc[idx].nick, bots, (bots != 1) ? "s" : "", users,
               (users != 1) ? "s" : "");
  putlog(LOG_BOTS, "*", "%s.", x);
  chatout("*** %s\n", x);
  botnet_send_unlinked(idx, dcc[idx].nick, x);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void display_dcc_bot(int idx, char *buf)
{
  int i = simple_sprintf(buf, "bot   flags: ");

  buf[i++] = b_status(idx) & STAT_PINGED ? 'P' : 'p';
  buf[i++] = b_status(idx) & STAT_SHARE ? 'U' : 'u';
  buf[i++] = b_status(idx) & STAT_CALLED ? 'C' : 'c';
  buf[i++] = b_status(idx) & STAT_OFFERED ? 'O' : 'o';
  buf[i++] = b_status(idx) & STAT_SENDING ? 'S' : 's';
  buf[i++] = b_status(idx) & STAT_GETTING ? 'G' : 'g';
  buf[i++] = b_status(idx) & STAT_WARNED ? 'W' : 'w';
  buf[i++] = b_status(idx) & STAT_LEAF ? 'L' : 'l';
  buf[i++] = b_status(idx) & STAT_LINKING ? 'I' : 'i';
  buf[i++] = b_status(idx) & STAT_AGGRESSIVE ? 'a' : 'A';
  buf[i++] = 0;
}

static void display_dcc_fork_bot(int idx, char *buf)
{
  sprintf(buf, "conn  bot");
}

struct dcc_table DCC_BOT = {
  "BOT",
  DCT_BOT | DCT_VALIDIDX,
  eof_dcc_bot,
  dcc_bot,
  NULL,
  NULL,
  display_dcc_bot,
  expmem_dcc_bot_,
  free_dcc_bot_,
  NULL
};

struct dcc_table DCC_FORK_BOT = {
  "FORK_BOT",
  0,
  failed_link,
  cont_link,
  &connect_timeout,
  failed_link,
  display_dcc_fork_bot,
  expmem_dcc_bot_,
  free_dcc_bot_,
  NULL
};

/* This function generates a digest by combining a challenge consisting
 * of our process id + connection time + botnetnick.  The digest is then
 * compared to the one given by the remote bot.
 *
 * Returns 1 if the digest matches, otherwise returns 0.
 * <Cybah>
 */
static int dcc_bot_check_digest(int idx, char *remote_digest)
{
  MD5_CTX md5context;
  char digest_string[33];       /* 32 for digest in hex + null */
  unsigned char digest[16];
  int i;
  char *password = get_user(&USERENTRY_PASS, dcc[idx].user);

  if (!password)
    return 1;

  MD5_Init(&md5context);

  egg_snprintf(digest_string, 33, "<%x%x@", getpid(),
               (unsigned int) dcc[idx].timeval);
  MD5_Update(&md5context, (unsigned char *) digest_string,
             strlen(digest_string));
  MD5_Update(&md5context, (unsigned char *) botnetnick, strlen(botnetnick));
  MD5_Update(&md5context, (unsigned char *) ">", 1);
  MD5_Update(&md5context, (unsigned char *) password, strlen(password));

  MD5_Final(digest, &md5context);

  for (i = 0; i < 16; i++)
    sprintf(digest_string + (i * 2), "%.2x", digest[i]);

  if (!strcmp(digest_string, remote_digest))
    return 1;

  putlog(LOG_BOTS, "*", "Response (password hash) from %s incorrect",
         dcc[idx].nick);
  return 0;
}

static void dcc_chat_pass(int idx, char *buf, int atr)
{
  if (!atr)
    return;
  if (dcc[idx].status & STAT_TELNET)
    strip_telnet(dcc[idx].sock, buf, &atr);
  else if (detect_telnet((unsigned char *) buf))
    buf += 3; /* 'IAC','DO(DONT)','STATUS' */
  atr = dcc[idx].user ? dcc[idx].user->flags : 0;

  /* Check for MD5 digest from remote _bot_. <cybah> */
  if ((atr & USER_BOT) && !egg_strncasecmp(buf, "digest ", 7)) {
    if (dcc_bot_check_digest(idx, buf + 7)) {
      nfree(dcc[idx].u.chat);
      dcc[idx].type = &DCC_BOT_NEW;
      dcc[idx].u.bot = get_data_ptr(sizeof(struct bot_info));
      dcc[idx].status = STAT_CALLED;
      dprintf(idx, "*hello!\n");
      greet_new_bot(idx);
      return;
    } else {
      /* Invalid password/digest */
      dprintf(idx, "badpass\n");
      putlog(LOG_MISC, "*", DCC_BADLOGIN, dcc[idx].nick, dcc[idx].host,
             dcc[idx].port);
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return;
    }
  }

  if (u_pass_match(dcc[idx].user, buf)) {
    if (atr & USER_BOT) {
      nfree(dcc[idx].u.chat);
      dcc[idx].type = &DCC_BOT_NEW;
      dcc[idx].u.bot = get_data_ptr(sizeof(struct bot_info));

      dcc[idx].status = STAT_CALLED;
      dprintf(idx, "*hello!\n");
      greet_new_bot(idx);
    } else {
      /* Log entry for successful login -slennox 3/28/1999 */
      putlog(LOG_MISC, "*", DCC_LOGGEDIN, dcc[idx].nick,
             dcc[idx].host, dcc[idx].port);
      if (dcc[idx].u.chat->away) {
        nfree(dcc[idx].u.chat->away);
        dcc[idx].u.chat->away = NULL;
      }
      dcc[idx].type = &DCC_CHAT;
      dcc[idx].status &= ~STAT_CHAT;
      dcc[idx].u.chat->con_flags = (atr & USER_MASTER) ? conmask : 0;
      dcc[idx].u.chat->channel = -2;
      /* Turn echo back on for telnet sessions (send IAC WON'T ECHO). */
      if (dcc[idx].status & STAT_TELNET)
        tputs(dcc[idx].sock, TLN_IAC_C TLN_WONT_C TLN_ECHO_C "\n", 4);
      dcc_chatter(idx);
    }
  } else {
    if (atr & USER_BOT)
      dprintf(idx, "badpass\n");
    else
      dprintf(idx, DCC_HOUSTON);
    putlog(LOG_MISC, "*", DCC_BADLOGIN, dcc[idx].nick,
           dcc[idx].host, dcc[idx].port);
    if (dcc[idx].u.chat->away) {        /* su from a dumb user */
      /* Turn echo back on for telnet sessions (send IAC WON'T ECHO). */
      if (dcc[idx].status & STAT_TELNET)
        tputs(dcc[idx].sock, TLN_IAC_C TLN_WONT_C TLN_ECHO_C "\n", 4);
      dcc[idx].user = get_user_by_handle(userlist, dcc[idx].u.chat->away);
      strcpy(dcc[idx].nick, dcc[idx].u.chat->away);
      nfree(dcc[idx].u.chat->away);
      nfree(dcc[idx].u.chat->su_nick);
      dcc[idx].u.chat->away = NULL;
      dcc[idx].u.chat->su_nick = NULL;
      dcc[idx].type = &DCC_CHAT;
      if (dcc[idx].u.chat->channel < GLOBAL_CHANS)
        botnet_send_join_idx(idx, -1);
      chanout_but(-1, dcc[idx].u.chat->channel, DCC_JOIN, dcc[idx].nick);
    } else {
      killsock(dcc[idx].sock);
      lostdcc(idx);
    }
  }
}

static void eof_dcc_general(int idx)
{
  putlog(LOG_MISC, "*", DCC_LOSTDCC, dcc[idx].nick,
         dcc[idx].host, dcc[idx].port);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void tout_dcc_chat_pass(int idx)
{
  dprintf(idx, "Timeout.\n");
  putlog(LOG_MISC, "*", DCC_PWDTIMEOUT, dcc[idx].nick, dcc[idx].host);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void display_dcc_chat_pass(int idx, char *buf)
{
  long tv;

  tv = now - dcc[idx].timeval;
  sprintf(buf, "pass  waited %lis", tv);
}

static int expmem_dcc_general(void *x)
{
  register struct chat_info *p = (struct chat_info *) x;
  int tot = sizeof(struct chat_info);

  if (p->away)
    tot += strlen(p->away) + 1;
  if (p->buffer) {
    struct msgq *q = p->buffer;

    while (q) {
      tot += sizeof(struct list_type);

      tot += q->len + 1;
      q = q->next;
    }
  }
  if (p->su_nick)
    tot += strlen(p->su_nick) + 1;
  return tot;
}

static void kill_dcc_general(int idx, void *x)
{
  register struct chat_info *p = (struct chat_info *) x;

  if (p) {
    if (p->buffer) {
      struct msgq *r, *q;

      for (r = dcc[idx].u.chat->buffer; r; r = q) {
        q = r->next;
        nfree(r->msg);
        nfree(r);
      }
    }
    if (p->away) {
      nfree(p->away);
    }
    nfree(p);
  }
}

/* Remove the color control codes that mIRC,pIRCh etc use to make
 * their client seem so fecking cool! (Sorry, Khaled, you are a nice
 * guy, but when you added this feature you forced people to either
 * use your *SHAREWARE* client or face screenfulls of crap!)
 */
void strip_mirc_codes(int flags, char *text)
{
  char *dd = text;

  while (*text) {
    switch (*text) {
    case 2:                    /* Bold text */
      if (flags & STRIP_BOLD) {
        text++;
        continue;
      }
      break;
    case 3:                    /* mIRC colors? */
      if (flags & STRIP_COLOR) {
        if (egg_isdigit(text[1])) { /* Is the first char a number? */
          text += 2;            /* Skip over the ^C and the first digit */
          if (egg_isdigit(*text))
            text++;             /* Is this a double digit number? */
          if (*text == ',') {   /* Do we have a background color next? */
            if (egg_isdigit(text[1]))
              text += 2;        /* Skip over the first background digit */
            if (egg_isdigit(*text))
              text++;           /* Is it a double digit? */
          }
        } else
          text++;
        continue;
      }
      break;
    case 7:
      if (flags & STRIP_BELLS) {
        text++;
        continue;
      }
      break;
    case 0x16:                 /* Reverse video */
      if (flags & STRIP_REV) {
        text++;
        continue;
      }
      break;
    case 0x1f:                 /* Underlined text */
      if (flags & STRIP_UNDER) {
        text++;
        continue;
      }
      break;
    case 033:
      if (flags & STRIP_ANSI) {
        text++;
        if (*text == '[') {
          text++;
          while ((*text == ';') || egg_isdigit(*text))
            text++;
          if (*text)
            text++;             /* also kill the following char */
        }
        continue;
      }
      break;
    }
    *dd++ = *text++;            /* Move on to the next char */
  }
  *dd = 0;
}

static void append_line(int idx, char *line)
{
  int l = strlen(line);
  struct msgq *p, *q;
  struct chat_info *c = (dcc[idx].type == &DCC_CHAT) ? dcc[idx].u.chat :
                        dcc[idx].u.file->chat;

  if (c->current_lines > 1000) {
    /* They're probably trying to fill up the bot nuke the sods :) */
    for (p = c->buffer; p; p = q) {
      q = p->next;
      nfree(p->msg);
      nfree(p);
    }
    c->buffer = 0;
    dcc[idx].status &= ~STAT_PAGE;
    do_boot(idx, botnetnick, "too many pages - sendq full");
    return;
  }
  if ((c->line_count < c->max_line) && (c->buffer == NULL)) {
    c->line_count++;
    tputs(dcc[idx].sock, line, l);
  } else {
    c->current_lines++;
    if (c->buffer == NULL)
      q = NULL;
    else
      for (q = c->buffer; q->next; q = q->next);

    p = get_data_ptr(sizeof(struct msgq));

    p->len = l;
    p->msg = get_data_ptr(l + 1);
    p->next = NULL;
    strcpy(p->msg, line);
    if (q == NULL)
      c->buffer = p;
    else
      q->next = p;
  }
}


static void out_dcc_general(int idx, char *buf, void *x)
{
  register struct chat_info *p = (struct chat_info *) x;
  char *y = buf;

  strip_mirc_codes(p->strip_flags, buf);
  if (dcc[idx].status & STAT_TELNET)
    y = escape_telnet(buf);
  if (dcc[idx].status & STAT_PAGE)
    append_line(idx, y);
  else
    tputs(dcc[idx].sock, y, strlen(y));
}

struct dcc_table DCC_CHAT_PASS = {
  "CHAT_PASS",
  0,
  eof_dcc_general,
  dcc_chat_pass,
  &password_timeout,
  tout_dcc_chat_pass,
  display_dcc_chat_pass,
  expmem_dcc_general,
  kill_dcc_general,
  out_dcc_general
};

/* Make sure ANSI code is just for color-changing */
int check_ansi(char *v)
{
  int count = 2;

  if (*v++ != '\033')
    return 1;
  if (*v++ != '[')
    return 1;
  while (*v) {
    if (*v == 'm')
      return 0;
    if ((*v != ';') && ((*v < '0') || (*v > '9')))
      return count;
    v++;
    count++;
  }
  return count;
}

static void eof_dcc_chat(int idx)
{
  putlog(LOG_MISC, "*", DCC_LOSTDCC, dcc[idx].nick,
         dcc[idx].host, dcc[idx].port);
  if (dcc[idx].u.chat->channel >= 0) {
    chanout_but(idx, dcc[idx].u.chat->channel, "*** %s lost dcc link.\n",
                dcc[idx].nick);
    if (dcc[idx].u.chat->channel < GLOBAL_CHANS)
      botnet_send_part_idx(idx, "lost dcc link");
    check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock,
                   dcc[idx].u.chat->channel);
  }
  check_tcl_chof(dcc[idx].nick, dcc[idx].sock);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void dcc_chat(int idx, char *buf, int i)
{
  int nathan = 0, doron = 0, fixed = 0;
  char *v, *d, filtbuf[2048];

  if (dcc[idx].status & STAT_TELNET)
    strip_telnet(dcc[idx].sock, buf, &i);
  if (buf[0] && (buf[0] != '.') &&
      detect_dcc_flood(&dcc[idx].timeval, dcc[idx].u.chat, idx))
    return;
  dcc[idx].timeval = now;
  if (buf[0]) {
    const char *filt = check_tcl_filt(idx, buf);
    if (filt != buf) {
      strncpyz(filtbuf, filt, sizeof(filtbuf));
      buf = filtbuf;
    }
  }
  if (buf[0]) {
    /* Check for beeps and cancel annoying ones */
    v = buf;
    d = buf;
    while (*v)
      switch (*v) {
      case 7:                  /* Beep - no more than 3 */
        nathan++;
        if (nathan > 3)
          v++;
        else
          *d++ = *v++;
        break;
      case 8:                  /* Backspace - for lame telnet's :) */
        if (d > buf) {
          d--;
        }
        v++;
        break;
      case 27:                 /* ESC - ansi code? */
        doron = check_ansi(v);
        /* If it's valid, append a return-to-normal code at the end */
        if (!doron) {
          *d++ = *v++;
          fixed = 1;
        } else
          v += doron;
        break;
      case '\r':               /* Weird pseudo-linefeed */
        v++;
        break;
      default:
        *d++ = *v++;
      }
    if (fixed)
      strcpy(d, "\033[0m");
    else
      *d = 0;
    if (buf[0]) {               /* Nothing to say - maybe paging... */
      if ((buf[0] == '.') || (dcc[idx].u.chat->channel < 0)) {
        if (buf[0] == '.')
          buf++;
        v = newsplit(&buf);
        rmspace(buf);
        if (check_tcl_dcc(v, idx, buf)) {
          if (dcc[idx].u.chat->channel >= 0)
            check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock,
                           dcc[idx].u.chat->channel);
          check_tcl_chof(dcc[idx].nick, dcc[idx].sock);
          dprintf(idx, "*** Ja mata!\n");
          flush_lines(idx, dcc[idx].u.chat);
          putlog(LOG_MISC, "*", DCC_CLOSED, dcc[idx].nick, dcc[idx].host);
          if (dcc[idx].u.chat->channel >= 0) {
            chanout_but(-1, dcc[idx].u.chat->channel,
                        "*** %s left the party line%s%s\n",
                        dcc[idx].nick, buf[0] ? ": " : ".", buf);
            if (dcc[idx].u.chat->channel < GLOBAL_CHANS)
              botnet_send_part_idx(idx, buf);
          }
          if (dcc[idx].u.chat->su_nick) {
            dcc[idx].user = get_user_by_handle(userlist,
                                               dcc[idx].u.chat->su_nick);
            strcpy(dcc[idx].nick, dcc[idx].u.chat->su_nick);
            dcc[idx].type = &DCC_CHAT;
            dprintf(idx, "Returning to real nick %s!\n",
                    dcc[idx].u.chat->su_nick);
            nfree(dcc[idx].u.chat->su_nick);
            dcc[idx].u.chat->su_nick = NULL;
            dcc_chatter(idx);
            if (dcc[idx].u.chat->channel < GLOBAL_CHANS &&
                dcc[idx].u.chat->channel >= 0)
              botnet_send_join_idx(idx, -1);
            return;
          } else if ((dcc[idx].sock != STDOUT) || backgrd) {
            killsock(dcc[idx].sock);
            lostdcc(idx);
            return;
          } else {
            dprintf(DP_STDOUT, "\n### SIMULATION RESET\n\n");
            dcc_chatter(idx);
            return;
          }
        }
      } else if (buf[0] == ',') {
        int me = 0;

        if ((buf[1] == 'm') && (buf[2] == 'e') && buf[3] == ' ')
          me = 1;
        for (i = 0; i < dcc_total; i++) {
          int ok = 0;

          if ((dcc[i].type->flags & DCT_MASTER) &&
              ((dcc[i].type != &DCC_CHAT) || (dcc[i].u.chat->channel >= 0)) &&
              ((i != idx) || (dcc[idx].status & STAT_ECHO)))
            ok = 1;
          if (ok) {
            struct userrec *u = get_user_by_handle(userlist, dcc[i].nick);

            if (u && (u->flags & USER_MASTER)) {
              if (me)
                dprintf(i, "-> %s%s\n", dcc[idx].nick, buf + 3);
              else
                dprintf(i, "-%s-> %s\n", dcc[idx].nick, buf + 1);
            }
          }
        }
      } else if (buf[0] == '\'') {
        int me = 0;

        if ((buf[1] == 'm') && (buf[2] == 'e') &&
            ((buf[3] == ' ') || (buf[3] == '\'') || (buf[3] == ',')))
          me = 1;
        for (i = 0; i < dcc_total; i++) {
          if (dcc[i].type->flags & DCT_CHAT) {
            if (me)
              dprintf(i, "=> %s%s\n", dcc[idx].nick, buf + 3);
            else
              dprintf(i, "=%s=> %s\n", dcc[idx].nick, buf + 1);
          }
        }
      } else {
        if (dcc[idx].u.chat->away != NULL)
          not_away(idx);
        if (dcc[idx].status & STAT_ECHO)
          chanout_but(-1, dcc[idx].u.chat->channel,
                      "<%s> %s\n", dcc[idx].nick, buf);
        else
          chanout_but(idx, dcc[idx].u.chat->channel, "<%s> %s\n",
                      dcc[idx].nick, buf);
        botnet_send_chan(-1, botnetnick, dcc[idx].nick,
                         dcc[idx].u.chat->channel, buf);
        check_tcl_chat(dcc[idx].nick, dcc[idx].u.chat->channel, buf);
      }
    }
  }
  if (dcc[idx].type == &DCC_CHAT)       /* Could have change to files */
    if (dcc[idx].status & STAT_PAGE)
      flush_lines(idx, dcc[idx].u.chat);
}

static void display_dcc_chat(int idx, char *buf)
{
  int i = simple_sprintf(buf, "chat  flags: ");

  buf[i++] = dcc[idx].status & STAT_CHAT ? 'C' : 'c';
  buf[i++] = dcc[idx].status & STAT_PARTY ? 'P' : 'p';
  buf[i++] = dcc[idx].status & STAT_TELNET ? 'T' : 't';
  buf[i++] = dcc[idx].status & STAT_ECHO ? 'E' : 'e';
  buf[i++] = dcc[idx].status & STAT_PAGE ? 'P' : 'p';
  simple_sprintf(buf + i, "/%d", dcc[idx].u.chat->channel);
}

struct dcc_table DCC_CHAT = {
  "CHAT",
  DCT_CHAT | DCT_MASTER | DCT_SHOWWHO | DCT_VALIDIDX | DCT_SIMUL |
    DCT_CANBOOT | DCT_REMOTEWHO,
  eof_dcc_chat,
  dcc_chat,
  NULL,
  NULL,
  display_dcc_chat,
  expmem_dcc_general,
  kill_dcc_general,
  out_dcc_general
};

static int lasttelnets;
static char lasttelnethost[81];
static time_t lasttelnettime;

/* A modified detect_flood for incoming telnet flood protection.
 */
static int detect_telnet_flood(char *floodhost)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };

  get_user_flagrec(get_user_by_host(floodhost), &fr, NULL);
  if (!flood_telnet_thr || (glob_friend(fr) && !par_telnet_flood))
    return 0;                   /* No flood protection */
  if (egg_strcasecmp(lasttelnethost, floodhost)) {      /* New... */
    strcpy(lasttelnethost, floodhost);
    lasttelnettime = now;
    lasttelnets = 0;
    return 0;
  }
  if (lasttelnettime < now - flood_telnet_time) {
    /* Flood timer expired, reset it */
    lasttelnettime = now;
    lasttelnets = 0;
    return 0;
  }
  lasttelnets++;
  if (lasttelnets >= flood_telnet_thr) {        /* FLOOD! */
    /* Reset counters */
    lasttelnets = 0;
    lasttelnettime = 0;
    lasttelnethost[0] = 0;
    putlog(LOG_MISC, "*", IRC_TELNETFLOOD, floodhost);
    addignore(floodhost, origbotname, "Telnet connection flood",
              now + (60 * ignore_time));
    return 1;
  }
  return 0;
}

static void dcc_telnet(int idx, char *buf, int i)
{
  unsigned long ip;
  unsigned short port;
  int j = 0, sock;
  char s[UHOSTLEN + 1];

  if (dcc_total + 1 > max_dcc && increase_socks_max()) {
    j = answer(dcc[idx].sock, s, &ip, &port, 0);
    if (j != -1) {
      dprintf(-j, "Sorry, too many connections already.\r\n");
      killsock(j);
    }
    return;
  }
  sock = answer(dcc[idx].sock, s, &ip, &port, 0);
  while ((sock == -1) && (errno == EAGAIN))
    sock = answer(sock, s, &ip, &port, 0);
  if (sock < 0) {
    neterror(s);
    putlog(LOG_MISC, "*", DCC_FAILED, s);
    return;
  }
  /* Buffer data received on this socket.  */
  sockoptions(sock, EGG_OPTION_SET, SOCK_BUFFER);

#if (SIZEOF_SHORT == 2)
  if (port < 1024) {
#else
  if (port < 1024 || port > 65535) {
#endif
    putlog(LOG_BOTS, "*", DCC_BADSRC, s, port);
    killsock(sock);
    return;
  }

  i = new_dcc(&DCC_DNSWAIT, sizeof(struct dns_info));
  dcc[i].sock = sock;
  dcc[i].addr = ip;
  dcc[i].port = port;
  dcc[i].timeval = now;
  strcpy(dcc[i].nick, "*");
  dcc[i].u.dns->ip = ip;
  dcc[i].u.dns->dns_success = dcc_telnet_hostresolved;
  dcc[i].u.dns->dns_failure = dcc_telnet_hostresolved;
  dcc[i].u.dns->dns_type = RES_HOSTBYIP;
  dcc[i].u.dns->ibuf = dcc[idx].sock;
  dcc[i].u.dns->type = &DCC_IDENTWAIT;
  dcc_dnshostbyip(ip);
}

static void dcc_telnet_hostresolved(int i)
{
  int idx;
  int j = 0, sock;
  char s[UHOSTLEN], s2[UHOSTLEN + 20];

  strncpyz(dcc[i].host, dcc[i].u.dns->host, UHOSTLEN);

  for (idx = 0; idx < dcc_total; idx++)
    if ((dcc[idx].type == &DCC_TELNET) &&
        (dcc[idx].sock == dcc[i].u.dns->ibuf)) {
      break;
    }
  if (dcc_total == idx) {
    putlog(LOG_BOTS, "*", "Lost listening socket while resolving %s",
           dcc[i].host);
    killsock(dcc[i].sock);
    lostdcc(i);
    return;
  }
  if (dcc[idx].host[0] == '@') {
    /* Restrict by hostname */
    if (!wild_match(dcc[idx].host + 1, dcc[i].host)) {
      putlog(LOG_BOTS, "*", DCC_BADHOST, dcc[i].host);
      killsock(dcc[i].sock);
      lostdcc(i);
      return;
    }
  }
  sprintf(s2, "-telnet!telnet@%s", dcc[i].host);
  if (match_ignore(s2) || detect_telnet_flood(s2)) {
    killsock(dcc[i].sock);
    lostdcc(i);
    return;
  }

  changeover_dcc(i, &DCC_IDENTWAIT, 0);
  dcc[i].timeval = now;
  dcc[i].u.ident_sock = dcc[idx].sock;
  sock = open_telnet(iptostr(htonl(dcc[i].addr)), 113);
  putlog(LOG_MISC, "*", DCC_TELCONN, dcc[i].host, dcc[i].port);
  s[0] = 0;
  if (sock < 0) {
    if (sock == -2)
      strcpy(s, "DNS lookup failed for ident");
    else
      neterror(s);
  } else {
    j = new_dcc(&DCC_IDENT, 0);
    if (j < 0) {
      killsock(sock);
      strcpy(s, "No Free DCC's");
    }
  }
  if (s[0]) {
    putlog(LOG_MISC, "*", DCC_IDENTFAIL, dcc[i].host, s);
    sprintf(s, "telnet@%s", dcc[i].host);
    dcc_telnet_got_ident(i, s);
    return;
  }
  dcc[j].sock = sock;
  dcc[j].port = 113;
  dcc[j].addr = dcc[i].addr;
  strcpy(dcc[j].host, dcc[i].host);
  strcpy(dcc[j].nick, "*");
  dcc[j].u.ident_sock = dcc[i].sock;
  dcc[j].timeval = now;
  dprintf(j, "%d, %d\n", dcc[i].port, dcc[idx].port);
}

static void eof_dcc_telnet(int idx)
{
  putlog(LOG_MISC, "*", DCC_PORTDIE, dcc[idx].port);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void display_telnet(int idx, char *buf)
{
  sprintf(buf, "lstn  %d%s", dcc[idx].port,
          (dcc[idx].status & LSTN_PUBLIC) ? " pub" : "");
}

struct dcc_table DCC_TELNET = {
  "TELNET",
  DCT_LISTEN,
  eof_dcc_telnet,
  dcc_telnet,
  NULL,
  NULL,
  display_telnet,
  NULL,
  NULL,
  NULL
};

static void eof_dcc_dupwait(int idx)
{
  putlog(LOG_BOTS, "*", DCC_LOSTDUP, dcc[idx].host);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void dcc_dupwait(int idx, char *buf, int i)
{
  /* We just ignore any data at this point. */
  return;
}

/* We now check again. If the bot is still marked as duplicate, there is no
 * botnet lag we could push it on, so we just drop the connection.
 */
static void timeout_dupwait(int idx)
{
  char x[100];

  /* Still duplicate? */
  if (in_chain(dcc[idx].nick)) {
    egg_snprintf(x, sizeof x, "%s!%s", dcc[idx].nick, dcc[idx].host);
    dprintf(idx, "error Already connected.\n");
    putlog(LOG_BOTS, "*", DCC_DUPLICATE, x);
    killsock(dcc[idx].sock);
    lostdcc(idx);
  } else {
    /* Ha! Now it's gone and we can grant this bot access. */
    dcc_telnet_pass(idx, dcc[idx].u.dupwait->atr);
  }
}

static void display_dupwait(int idx, char *buf)
{
  sprintf(buf, "wait  duplicate?");
}

static int expmem_dupwait(void *x)
{
  register struct dupwait_info *p = (struct dupwait_info *) x;
  int tot = sizeof(struct dupwait_info);

  if (p && p->chat && DCC_CHAT.expmem)
    tot += DCC_CHAT.expmem(p->chat);
  return tot;
}

static void kill_dupwait(int idx, void *x)
{
  register struct dupwait_info *p = (struct dupwait_info *) x;

  if (p) {
    if (p->chat && DCC_CHAT.kill)
      DCC_CHAT.kill(idx, p->chat);
    nfree(p);
  }
}

struct dcc_table DCC_DUPWAIT = {
  "DUPWAIT",
  DCT_VALIDIDX,
  eof_dcc_dupwait,
  dcc_dupwait,
  &dupwait_timeout,
  timeout_dupwait,
  display_dupwait,
  expmem_dupwait,
  kill_dupwait,
  NULL
};

/* This function is called if a bot gets removed from the list. It checks
 * wether we have a pending duplicate connection for that bot and continues
 * with the login in that case.
 */
void dupwait_notify(char *who)
{
  register int idx;

  Assert(who);
  for (idx = 0; idx < dcc_total; idx++)
    if ((dcc[idx].type == &DCC_DUPWAIT) &&
        !egg_strcasecmp(dcc[idx].nick, who)) {
      dcc_telnet_pass(idx, dcc[idx].u.dupwait->atr);
      break;
    }
}

static void dcc_telnet_id(int idx, char *buf, int atr)
{
  int ok = 0;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };

  if (detect_telnet((unsigned char *) buf)) {
    dcc[idx].status |= STAT_TELNET;
    strip_telnet(dcc[idx].sock, buf, &atr);
  } else
    dcc[idx].status &= ~STAT_TELNET;
  buf[HANDLEN] = 0;
  /* Toss out bad nicknames */
  if (dcc[idx].nick[0] != '@' && !wild_match(dcc[idx].nick, buf)) {
    dprintf(idx, "Sorry, that nickname format is invalid.\n");
    putlog(LOG_BOTS, "*", DCC_BADNICK, dcc[idx].host);
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return;
  }
  dcc[idx].user = get_user_by_handle(userlist, buf);
  get_user_flagrec(dcc[idx].user, &fr, NULL);
  /* Make sure users-only/bots-only connects are honored */
  if ((dcc[idx].status & STAT_BOTONLY) && !glob_bot(fr)) {
    dprintf(idx, "This telnet port is for bots only.\n");
    putlog(LOG_BOTS, "*", DCC_NONBOT, dcc[idx].host);
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return;
  }
  if ((dcc[idx].status & STAT_USRONLY) && glob_bot(fr)) {
    dprintf(idx, "error Only users may connect at this port.\n");
    putlog(LOG_BOTS, "*", DCC_NONUSER, dcc[idx].host);
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return;
  }
  dcc[idx].status &= ~(STAT_BOTONLY | STAT_USRONLY);
  if (!egg_strcasecmp(buf, "NEW") && (allow_new_telnets || make_userfile)) {
    dcc[idx].type = &DCC_TELNET_NEW;
    dcc[idx].timeval = now;
    dprintf(idx, "\n");
    dprintf(idx, IRC_TELNET, botnetnick);
    dprintf(idx, IRC_TELNET1);
    dprintf(idx, "\nEnter the nickname you would like to use.\n");
    return;
  }
  if (chan_op(fr)) {
    if (!require_p)
      ok = 1;
  }
  if (!ok && (glob_party(fr) || glob_bot(fr)))
    ok = 1;

  if (!ok) {
    dprintf(idx, "You don't have access.\n");
    putlog(LOG_BOTS, "*", DCC_INVHANDLE, dcc[idx].host, buf);
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return;
  }
  correct_handle(buf);
  strcpy(dcc[idx].nick, buf);
  if (glob_bot(fr)) {
    if (!egg_strcasecmp(botnetnick, dcc[idx].nick)) {
      dprintf(idx, "error You cannot link using my botnetnick.\n");
      putlog(LOG_BOTS, "*", DCC_MYBOTNETNICK, dcc[idx].host);
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return;
    } else if (in_chain(dcc[idx].nick)) {
      struct chat_info *ci;

      ci = dcc[idx].u.chat;
      dcc[idx].type = &DCC_DUPWAIT;
      dcc[idx].u.dupwait = get_data_ptr(sizeof(struct dupwait_info));
      dcc[idx].u.dupwait->chat = ci;
      dcc[idx].u.dupwait->atr = atr;
      return;
    }
  }
  dcc_telnet_pass(idx, atr);
}

static void dcc_telnet_pass(int idx, int atr)
{
  int ok = 0;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };

  get_user_flagrec(dcc[idx].user, &fr, NULL);
  /* No password set? */
  if (u_pass_match(dcc[idx].user, "-")) {
    if (glob_bot(fr)) {
      char ps[20];

      makepass(ps);
      set_user(&USERENTRY_PASS, dcc[idx].user, ps);
      changeover_dcc(idx, &DCC_BOT_NEW, sizeof(struct bot_info));

      dcc[idx].status = STAT_CALLED;
      dprintf(idx, "*hello!\n");
      greet_new_bot(idx);
#ifdef NO_OLD_BOTNET
      dprintf(idx, "h %s\n", ps);
#else
      dprintf(idx, "handshake %s\n", ps);
#endif
      return;
    }
    dprintf(idx, "Can't telnet until you have a password set.\n");
    putlog(LOG_MISC, "*", DCC_NOPASS, dcc[idx].nick, dcc[idx].host);
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return;
  }
  ok = 0;
  if (dcc[idx].type == &DCC_DUPWAIT) {
    struct chat_info *ci;

    ci = dcc[idx].u.dupwait->chat;
    nfree(dcc[idx].u.dupwait);
    dcc[idx].u.chat = ci;
  }
  dcc[idx].type = &DCC_CHAT_PASS;
  dcc[idx].timeval = now;
  if (glob_botmast(fr))
    ok = 1;
  else if (chan_op(fr)) {
    if (!require_p)
      ok = 1;
    else if (glob_party(fr))
      ok = 1;
  } else if (glob_party(fr)) {
    ok = 1;
    dcc[idx].status |= STAT_PARTY;
  }
  if (glob_bot(fr))
    ok = 1;
  if (!ok) {
    struct chat_info *ci;

    ci = dcc[idx].u.chat;
    dcc[idx].u.file = get_data_ptr(sizeof(struct file_info));
    dcc[idx].u.file->chat = ci;
  }

  if (glob_bot(fr)) {
    /* Must generate a string consisting of our process ID and the current
     * time. The bot will add it's password to the end and use it to generate
     * an MD5 checksum (always 128bit). The checksum is sent back and this
     * end does the same. The remote bot is only allowed access if the
     * checksums match.
     *
     * Please don't fuck with 'timeval', or the digest we generate later for
     * authentication will not be correct - you've been warned ;)
     * <Cybah>
     */
    putlog(LOG_BOTS, "*", "Challenging %s...", dcc[idx].nick);
    dprintf(idx, "passreq <%x%x@%s>\n", getpid(), dcc[idx].timeval, botnetnick);
  } else {
    /* NOTE: The MD5 digest used above to prevent cleartext passwords being
     *       sent across the net will _only_ work when we have the cleartext
     *       password. User passwords are encrypted (with blowfish usually)
     *       so the same thing cant be done. Botnet passwords are always
     *       stored in cleartext, or at least something that can be reversed.
     *       <Cybah>
     */

    /* Turn off remote telnet echo (send IAC WILL ECHO). */
    if (dcc[idx].status & STAT_TELNET) {
      char buf[1030];
      snprintf(buf, sizeof buf, "\n%s%s\r\n", escape_telnet(DCC_ENTERPASS),
               TLN_IAC_C TLN_WILL_C TLN_ECHO_C);
      tputs(dcc[idx].sock, buf, strlen(buf));
    } else
      dprintf(idx, "\n%s\n", DCC_ENTERPASS);
  }
}

static void eof_dcc_telnet_id(int idx)
{
  putlog(LOG_MISC, "*", DCC_LOSTCON, dcc[idx].host, dcc[idx].port);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void timeout_dcc_telnet_id(int idx)
{
  dprintf(idx, "Timeout.\n");
  putlog(LOG_MISC, "*", DCC_TTIMEOUT, dcc[idx].host);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void display_dcc_telnet_id(int idx, char *buf)
{
  long tv;

  tv = now - dcc[idx].timeval;
  sprintf(buf, "t-in  waited %lis", tv);
}

struct dcc_table DCC_TELNET_ID = {
  "TELNET_ID",
  0,
  eof_dcc_telnet_id,
  dcc_telnet_id,
  &password_timeout,
  timeout_dcc_telnet_id,
  display_dcc_telnet_id,
  expmem_dcc_general,
  kill_dcc_general,
  out_dcc_general
};

static void dcc_telnet_new(int idx, char *buf, int x)
{
  int ok = 1;
  char work[1024], *p, *q, *r;

  buf[HANDLEN] = 0;
  if (dcc[idx].status & STAT_TELNET)
    strip_telnet(dcc[idx].sock, buf, &x);
  dcc[idx].timeval = now;
  for (x = 0; x < strlen(buf); x++)
    if (buf[x] <= 32)
      ok = 0;
  if (!ok) {
    dprintf(idx, "\nYou can't use weird symbols in your nick.\n");
    dprintf(idx, "Try another one please:\n");
  } else if (strchr(BADHANDCHARS, buf[0]) != NULL) {
    dprintf(idx, "\nYou can't start your nick with the character '%c'\n",
            buf[0]);
    dprintf(idx, "Try another one please:\n");
  } else if (get_user_by_handle(userlist, buf)) {
    dprintf(idx, "\nSorry, that nickname is taken already.\n");
    dprintf(idx, "Try another one please:\n");
    return;
  } else if (!egg_strcasecmp(buf, botnetnick))
    dprintf(idx, "Sorry, can't use my name for a nick.\n");
  else {
    strcpy(dcc[idx].nick, buf);
    if (make_userfile)
      userlist = adduser(userlist,
                 buf, "-telnet!*@*", "-", sanity_check(default_flags |
                 USER_PARTY | USER_MASTER | USER_OWNER));
    else {
      p = strchr(dcc[idx].host, '@');
      if (p) {
        q = p;
        *q = 0;
        p++;
        r = strchr(p, '.');
        if (!r)
          simple_sprintf(work, "-telnet!%s@%s", dcc[idx].host, p);
        else
          simple_sprintf(work, "-telnet!%s@*%s", dcc[idx].host, r);
        *q = '@';
      } else
        simple_sprintf(work, "-telnet!*@*%s", dcc[idx].host);
      userlist = adduser(userlist, buf, work, "-",
                         sanity_check(USER_PARTY | default_flags));
    }
    reaffirm_owners();
    dcc[idx].status = STAT_ECHO | STAT_TELNET;
    dcc[idx].type = &DCC_CHAT;  /* Just so next line will work */
    dcc[idx].user = get_user_by_handle(userlist, buf);
    check_dcc_attrs(dcc[idx].user, USER_PARTY | default_flags);
    dcc[idx].type = &DCC_TELNET_PW;
    if (make_userfile) {
      dprintf(idx, "\nYOU ARE THE MASTER/OWNER ON THIS BOT NOW\n");
      dprintf(idx, IRC_LIMBO);
      putlog(LOG_MISC, "*", DCC_INSTCOMPL, buf);
      make_userfile = 0;
      write_userfile(-1);
      add_note(buf, botnetnick, "Welcome to eggdrop! :)", -1, 0);
    }
    dprintf(idx, "\nOkay, now choose and enter a password:\n");
    dprintf(idx, "(Only the first 15 letters are significant.)\n");
  }
}

static void dcc_telnet_pw(int idx, char *buf, int x)
{
  char *newpass;
  int ok;

  if (dcc[idx].status & STAT_TELNET)
    strip_telnet(dcc[idx].sock, buf, &x);
  buf[16] = 0;
  ok = 1;
  if (strlen(buf) < 4) {
    dprintf(idx, "\nTry to use at least 4 characters in your password.\n");
    dprintf(idx, "Choose and enter a password:\n");
    return;
  }
  for (x = 0; x < strlen(buf); x++)
    if ((buf[x] <= 32) || (buf[x] == 127))
      ok = 0;
  if (!ok) {
    dprintf(idx, "\nYou can't use weird symbols in your password.\n");
    dprintf(idx, "Try another one please:\n");
    return;
  }
  putlog(LOG_MISC, "*", DCC_NEWUSER, dcc[idx].nick, dcc[idx].host,
         dcc[idx].port);
  if (notify_new[0]) {
    char s[121], s1[121], s2[121];

    sprintf(s, "Introduced to %s, %s", dcc[idx].nick, dcc[idx].host);
    strcpy(s1, notify_new);
    splitc(s2, s1, ',');
    while (s2[0]) {
      rmspace(s2);
      add_note(s2, botnetnick, s, -1, 0);
      splitc(s2, s1, ',');
    }
    rmspace(s1);
    add_note(s1, botnetnick, s, -1, 0);
  }
  newpass = newsplit(&buf);
  set_user(&USERENTRY_PASS, dcc[idx].user, newpass);
  dprintf(idx, "\nRemember that!  You'll need it next time you log in.\n");
  dprintf(idx, "You now have an account on %s...\n\n\n", botnetnick);
  dcc[idx].type = &DCC_CHAT;
  dcc[idx].u.chat->channel = -2;
  dcc_chatter(idx);
}

static void eof_dcc_telnet_new(int idx)
{
  putlog(LOG_MISC, "*", DCC_LOSTNEWUSER, dcc[idx].host, dcc[idx].port);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void eof_dcc_telnet_pw(int idx)
{
  putlog(LOG_MISC, "*", DCC_LOSTNEWUSR2, dcc[idx].nick, dcc[idx].host,
         dcc[idx].port);
  deluser(dcc[idx].nick);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void tout_dcc_telnet_new(int idx)
{
  dprintf(idx, "Guess you're not there.  Bye.\n");
  putlog(LOG_MISC, "*", DCC_TIMEOUTUSER, dcc[idx].host, dcc[idx].port);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void tout_dcc_telnet_pw(int idx)
{
  dprintf(idx, "Guess you're not there.  Bye.\n");
  putlog(LOG_MISC, "*", DCC_TIMEOUTUSR2, dcc[idx].nick,
         dcc[idx].host, dcc[idx].port);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void display_dcc_telnet_new(int idx, char *buf)
{
  long tv;

  tv = now - dcc[idx].timeval;
  sprintf(buf, "new   waited %lis", tv);
}

static void display_dcc_telnet_pw(int idx, char *buf)
{
  long tv;

  tv = now - dcc[idx].timeval;
  sprintf(buf, "newp  waited %lis", tv);
}

struct dcc_table DCC_TELNET_NEW = {
  "TELNET_NEW",
  0,
  eof_dcc_telnet_new,
  dcc_telnet_new,
  &password_timeout,
  tout_dcc_telnet_new,
  display_dcc_telnet_new,
  expmem_dcc_general,
  kill_dcc_general,
  out_dcc_general
};

struct dcc_table DCC_TELNET_PW = {
  "TELNET_PW",
  0,
  eof_dcc_telnet_pw,
  dcc_telnet_pw,
  &password_timeout,
  tout_dcc_telnet_pw,
  display_dcc_telnet_pw,
  expmem_dcc_general,
  kill_dcc_general,
  out_dcc_general
};

static int call_tcl_func(char *name, int idx, char *args)
{
  char s[11];

  sprintf(s, "%d", idx);
  Tcl_SetVar(interp, "_n", s, 0);
  Tcl_SetVar(interp, "_a", args, 0);
  if (Tcl_VarEval(interp, name, " $_n $_a", NULL) == TCL_ERROR) {
    putlog(LOG_MISC, "*", DCC_TCLERROR, name, tcl_resultstring());
    return -1;
  }
  return tcl_resultint();
}

static void dcc_script(int idx, char *buf, int len)
{
  long oldsock;

  if (dcc[idx].status & STAT_TELNET)
    strip_telnet(dcc[idx].sock, buf, &len);
  if (!len)
    return;

  dcc[idx].timeval = now;
  oldsock = dcc[idx].sock;      /* Remember the socket number.  */
  if (call_tcl_func(dcc[idx].u.script->command, dcc[idx].sock, buf)) {
    void *old_other = NULL;

    /* Check whether the socket and dcc entry are still valid. They
     * might have been killed by `killdcc'. */
    if (dcc[idx].sock != oldsock || idx > max_dcc)
      return;

    old_other = dcc[idx].u.script->u.other;
    dcc[idx].type = dcc[idx].u.script->type;
    nfree(dcc[idx].u.script);
    dcc[idx].u.other = old_other;
    if (dcc[idx].type == &DCC_SOCKET) {
      /* Kill the whole thing off */
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return;
    }
    if (dcc[idx].type == &DCC_CHAT) {
      if (dcc[idx].u.chat->channel >= 0) {
        chanout_but(-1, dcc[idx].u.chat->channel, DCC_JOIN, dcc[idx].nick);
        if (dcc[idx].u.chat->channel < 10000)
          botnet_send_join_idx(idx, -1);
        check_tcl_chjn(botnetnick, dcc[idx].nick, dcc[idx].u.chat->channel,
                       geticon(idx), dcc[idx].sock, dcc[idx].host);
      }
      check_tcl_chon(dcc[idx].nick, dcc[idx].sock);
    }
  }
}

static void eof_dcc_script(int idx)
{
  void *old;
  int oldflags;

  /* This will stop a killdcc from working, incase the script tries
   * to kill it's controlling socket while handling an EOF <cybah>
   */
  oldflags = dcc[idx].type->flags;
  dcc[idx].type->flags &= ~(DCT_VALIDIDX);
  /* Tell the script they're gone: */
  call_tcl_func(dcc[idx].u.script->command, dcc[idx].sock, "");
  /* Restore the flags */
  dcc[idx].type->flags = oldflags;
  old = dcc[idx].u.script->u.other;
  dcc[idx].type = dcc[idx].u.script->type;
  nfree(dcc[idx].u.script);
  dcc[idx].u.other = old;
  /* Then let it fall thru to the real one */
  if (dcc[idx].type && dcc[idx].type->eof)
    dcc[idx].type->eof(idx);
  else {
    putlog(LOG_MISC, "*", DCC_DEADSOCKET, dcc[idx].sock, dcc[idx].type->name);
    killsock(dcc[idx].sock);
    lostdcc(idx);
  }
}

static void display_dcc_script(int idx, char *buf)
{
  sprintf(buf, "scri  %s", dcc[idx].u.script->command);
}

static int expmem_dcc_script(void *x)
{
  register struct script_info *p = (struct script_info *) x;
  int tot = sizeof(struct script_info);

  if (p->type && p->u.other)
    tot += p->type->expmem(p->u.other);
  return tot;
}

static void kill_dcc_script(int idx, void *x)
{
  register struct script_info *p = (struct script_info *) x;

  if (p->type && p->u.other)
    p->type->kill(idx, p->u.other);
  nfree(p);
}

static void out_dcc_script(int idx, char *buf, void *x)
{
  register struct script_info *p = (struct script_info *) x;

  if (p && p->type && p->u.other)
    p->type->output(idx, buf, p->u.other);
  else
    tputs(dcc[idx].sock, buf, strlen(buf));
}

struct dcc_table DCC_SCRIPT = {
  "SCRIPT",
  DCT_VALIDIDX,
  eof_dcc_script,
  dcc_script,
  NULL,
  NULL,
  display_dcc_script,
  expmem_dcc_script,
  kill_dcc_script,
  out_dcc_script
};

static void dcc_socket(int idx, char *buf, int len)
{
}

static void eof_dcc_socket(int idx)
{
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void display_dcc_socket(int idx, char *buf)
{
  strcpy(buf, "sock  (stranded)");
}

struct dcc_table DCC_SOCKET = {
  "SOCKET",
  DCT_VALIDIDX,
  eof_dcc_socket,
  dcc_socket,
  NULL,
  NULL,
  display_dcc_socket,
  NULL,
  NULL,
  NULL
};

static void display_dcc_lost(int idx, char *buf)
{
  strcpy(buf, "lost");
}

struct dcc_table DCC_LOST = {
  "LOST",
  0,
  NULL,
  dcc_socket,
  NULL,
  NULL,
  display_dcc_lost,
  NULL,
  NULL,
  NULL
};

void dcc_identwait(int idx, char *buf, int len)
{
  /* Ignore anything now */
}

void eof_dcc_identwait(int idx)
{
  int i;

  putlog(LOG_MISC, "*", DCC_LOSTCONN, dcc[idx].host, dcc[idx].port);
  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_IDENT) &&
        (dcc[i].u.ident_sock == dcc[idx].sock)) {
      killsock(dcc[i].sock);    /* Cleanup ident socket */
      dcc[i].u.other = 0;
      lostdcc(i);
      break;
    }
  killsock(dcc[idx].sock);      /* Cleanup waiting socket */
  dcc[idx].u.other = 0;
  lostdcc(idx);
}

static void display_dcc_identwait(int idx, char *buf)
{
  long tv;

  tv = now - dcc[idx].timeval;
  sprintf(buf, "idtw  waited %lis", tv);
}

struct dcc_table DCC_IDENTWAIT = {
  "IDENTWAIT",
  0,
  eof_dcc_identwait,
  dcc_identwait,
  NULL,
  NULL,
  display_dcc_identwait,
  NULL,
  NULL,
  NULL
};

void dcc_ident(int idx, char *buf, int len)
{
  char response[512], uid[512], buf1[UHOSTLEN];
  int i;

  *response = *uid = '\0';
  sscanf(buf, "%*[^:]:%[^:]:%*[^:]:%[^\n]\n", response, uid);
  rmspace(response);
  if (response[0] != 'U') {
    dcc[idx].timeval = now;
    return;
  }
  rmspace(uid);
  uid[20] = 0;                  /* 20 character ident max */
  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_IDENTWAIT) &&
        (dcc[i].sock == dcc[idx].u.ident_sock)) {
      simple_sprintf(buf1, "%s@%s", uid, dcc[idx].host);
      dcc_telnet_got_ident(i, buf1);
    }
  dcc[idx].u.other = 0;
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

void eof_dcc_ident(int idx)
{
  char buf[UHOSTLEN];
  int i;

  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_IDENTWAIT) &&
        (dcc[i].sock == dcc[idx].u.ident_sock)) {
      putlog(LOG_MISC, "*", DCC_EOFIDENT);
      simple_sprintf(buf, "telnet@%s", dcc[idx].host);
      dcc_telnet_got_ident(i, buf);
    }
  killsock(dcc[idx].sock);
  dcc[idx].u.other = 0;
  lostdcc(idx);
}

static void display_dcc_ident(int idx, char *buf)
{
  sprintf(buf, "idnt  (sock %d)", dcc[idx].u.ident_sock);
}

struct dcc_table DCC_IDENT = {
  "IDENT",
  0,
  eof_dcc_ident,
  dcc_ident,
  &identtimeout,
  eof_dcc_ident,
  display_dcc_ident,
  NULL,
  NULL,
  NULL
};

static void dcc_telnet_got_ident(int i, char *host)
{
  int idx;
  char x[1024];

  for (idx = 0; idx < dcc_total; idx++)
    if ((dcc[idx].type == &DCC_TELNET) &&
        (dcc[idx].sock == dcc[i].u.ident_sock))
      break;
  dcc[i].u.other = 0;
  if (dcc_total == idx) {
    putlog(LOG_MISC, "*", DCC_LOSTIDENT);
    killsock(dcc[i].sock);
    lostdcc(i);
    return;
  }
  strncpyz(dcc[i].host, host, UHOSTLEN);
  egg_snprintf(x, sizeof x, "-telnet!%s", dcc[i].host);
  if (protect_telnet && !make_userfile) {
    struct userrec *u;
    int ok = 1;

    u = get_user_by_host(x);
    /* Not a user or +p & require p OR +o */
    if (!u)
      ok = 0;
    else if (require_p && !(u->flags & USER_PARTY))
      ok = 0;
    else if (!require_p && !(u->flags & USER_OP))
      ok = 0;
    if (!ok && u && (u->flags & USER_BOT))
      ok = 1;
    if (!ok && (dcc[idx].status & LSTN_PUBLIC))
      ok = 1;
    if (!ok) {
      putlog(LOG_MISC, "*", DCC_NOACCESS, dcc[i].host);
      killsock(dcc[i].sock);
      lostdcc(i);
      return;
    }
  }
  if (match_ignore(x)) {
    killsock(dcc[i].sock);
    lostdcc(i);
    return;
  }
  /* Script? */
  if (!strcmp(dcc[idx].nick, "(script)")) {
    dcc[i].type = &DCC_SOCKET;
    dcc[i].u.other = NULL;
    strcpy(dcc[i].nick, "*");
    check_tcl_listen(dcc[idx].host, dcc[i].sock);
    return;
  }
  /* Do not buffer data anymore. All received and stored data is passed
   * over to the dcc functions from now on.  */
  sockoptions(dcc[i].sock, EGG_OPTION_UNSET, SOCK_BUFFER);

  dcc[i].type = &DCC_TELNET_ID;
  dcc[i].u.chat = get_data_ptr(sizeof(struct chat_info));
  egg_bzero(dcc[i].u.chat, sizeof(struct chat_info));

  /* Note: we don't really care about telnet status here. We use the
   * STATUS option as a hopefully harmless way to detect if the other
   * side is a telnet client or not. */
  dprintf(i, TLN_IAC_C TLN_WILL_C TLN_STATUS_C "\n");

  /* Copy acceptable-nick/host mask */
  dcc[i].status = STAT_TELNET | STAT_ECHO;
  if (!strcmp(dcc[idx].nick, "(bots)"))
    dcc[i].status |= STAT_BOTONLY;
  if (!strcmp(dcc[idx].nick, "(users)"))
    dcc[i].status |= STAT_USRONLY;
  /* Copy acceptable-nick/host mask */
  strncpyz(dcc[i].nick, dcc[idx].host, HANDLEN);
  dcc[i].timeval = now;
  strcpy(dcc[i].u.chat->con_chan, chanset ? chanset->dname : "*");
  /* Displays a customizable banner. */
  if (use_telnet_banner)
    show_banner(i);
  /* This is so we dont tell someone doing a portscan anything
   * about ourselves. <cybah>
   */
  if (stealth_telnets)
    sub_lang(i, MISC_BANNER_STEALTH);
  else {
    dprintf(i, "\n\n");
    sub_lang(i, MISC_BANNER);
  }
  if (allow_new_telnets)
    dprintf(i, "(If you are new, enter 'NEW' here.)\n");
}
