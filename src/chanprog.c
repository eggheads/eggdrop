/*
 * chanprog.c -- handles:
 *   rmspace()
 *   maintaining the server list
 *   revenge punishment
 *   timers, utimers
 *   telling the current programmed settings
 *   initializing a lot of stuff and loading the tcl scripts
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2020 Eggheads Development Team
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

#ifdef HAVE_GETRUSAGE
#  include <sys/resource.h>
#  ifdef HAVE_SYS_RUSAGE_H
#    include <sys/rusage.h>
#  endif
#endif

#include <sys/utsname.h>

#include "modules.h"

extern struct dcc_t *dcc;
extern struct userrec *userlist;
extern log_t *logs;
extern Tcl_Interp *interp;
extern char ver[], botnetnick[], firewall[], motdfile[], userfile[], helpdir[],
            moddir[], notify_new[], configfile[];
extern time_t now, online_since;
extern int backgrd, term_z, con_chan, cache_hit, cache_miss, firewallport,
           default_flags, max_logs, conmask, protect_readonly, make_userfile,
           noshare, ignore_time, max_socks;
#ifdef TLS
extern SSL_CTX *ssl_ctx;
#endif

tcl_timer_t *timer = NULL;         /* Minutely timer               */
tcl_timer_t *utimer = NULL;        /* Secondly timer               */
unsigned long timer_id = 1;        /* Next timer of any sort will
                                    * have this number             */
struct chanset_t *chanset = NULL;  /* Channel list                 */
char admin[121] = "";              /* Admin info                   */
char origbotname[NICKLEN];
char botname[NICKLEN];             /* Primary botname              */
char owner[121] = "";              /* Permanent botowner(s)        */


/* Remove leading and trailing whitespaces.
 */
void rmspace(char *s)
{
  char *p = NULL, *q = NULL;

  if (!s || !*s)
    return;

  /* Remove trailing whitespaces. */
  for (q = s + strlen(s) - 1; q >= s && egg_isspace(*q); q--);
  *(q + 1) = 0;

  /* Remove leading whitespaces. */
  for (p = s; egg_isspace(*p); p++);

  if (p != s)
    memmove(s, p, q - p + 2);
}


/* Returns memberfields if the nick is in the member list.
 */
memberlist *ismember(struct chanset_t *chan, char *nick)
{
  memberlist *x;

  for (x = chan->channel.member; x && x->nick[0]; x = x->next)
    if (!rfc_casecmp(x->nick, nick))
      return x;
  return NULL;
}

/* Find a chanset by channel name as the server knows it (ie !ABCDEchannel)
 */
struct chanset_t *findchan(const char *name)
{
  struct chanset_t *chan;

  for (chan = chanset; chan; chan = chan->next)
    if (!rfc_casecmp(chan->name, name))
      return chan;
  return NULL;
}

/* Find a chanset by display name (ie !channel)
 */
struct chanset_t *findchan_by_dname(const char *name)
{
  struct chanset_t *chan;

  for (chan = chanset; chan; chan = chan->next)
    if (!rfc_casecmp(chan->dname, name))
      return chan;
  return NULL;
}


/*
 *    "caching" functions
 */

/* Shortcut for get_user_by_host -- might have user record in one
 * of the channel caches.
 */
struct userrec *check_chanlist(const char *host)
{
  char *nick, *uhost, buf[UHOSTLEN];
  memberlist *m;
  struct chanset_t *chan;

  strlcpy(buf, host, sizeof buf);
  uhost = buf;
  nick = splitnick(&uhost);
  for (chan = chanset; chan; chan = chan->next)
    for (m = chan->channel.member; m && m->nick[0]; m = m->next)
      if (!rfc_casecmp(nick, m->nick) && !strcasecmp(uhost, m->userhost))
        return m->user;
  return NULL;
}

/* Shortcut for get_user_by_handle -- might have user record in channels
 */
struct userrec *check_chanlist_hand(const char *hand)
{
  struct chanset_t *chan;
  memberlist *m;

  for (chan = chanset; chan; chan = chan->next)
    for (m = chan->channel.member; m && m->nick[0]; m = m->next)
      if (m->user && !strcasecmp(m->user->handle, hand))
        return m->user;
  return NULL;
}

/* Clear the user pointers in the chanlists.
 *
 * Necessary when a hostmask is added/removed, a user is added or a new
 * userfile is loaded.
 */
void clear_chanlist(void)
{
  memberlist *m;
  struct chanset_t *chan;

  for (chan = chanset; chan; chan = chan->next)
    for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
      m->user = NULL;
      m->tried_getuser = 0;
    }
}

/* Clear the user pointer of a specific nick in the chanlists.
 *
 * Necessary when a hostmask is added/removed, a nick changes, etc.
 * Does not completely invalidate the channel cache like clear_chanlist().
 */
void clear_chanlist_member(const char *nick)
{
  memberlist *m;
  struct chanset_t *chan;

  for (chan = chanset; chan; chan = chan->next)
    for (m = chan->channel.member; m && m->nick[0]; m = m->next)
      if (!rfc_casecmp(m->nick, nick)) {
        m->user = NULL;
        m->tried_getuser = 0;
        break;
      }
}

/* If this user@host is in a channel, set it (it was null)
 */
void set_chanlist(const char *host, struct userrec *rec)
{
  char *nick, *uhost, buf[UHOSTLEN];
  memberlist *m;
  struct chanset_t *chan;

  strlcpy(buf, host, sizeof buf);
  uhost = buf;
  nick = splitnick(&uhost);
  for (chan = chanset; chan; chan = chan->next)
    for (m = chan->channel.member; m && m->nick[0]; m = m->next)
      if (!rfc_casecmp(nick, m->nick) && !strcasecmp(uhost, m->userhost))
        m->user = rec;
}

/* Calculate the memory we should be using
 */
int expmem_chanprog()
{
  int tot = 0;
  tcl_timer_t *t;

  for (t = timer; t; t = t->next)
    tot += sizeof(tcl_timer_t) + strlen(t->cmd) + 1;
  for (t = utimer; t; t = t->next)
    tot += sizeof(tcl_timer_t) + strlen(t->cmd) + 1;
  return tot;
}

float getcputime()
{
#ifdef HAVE_GETRUSAGE
  float stime, utime;
  struct rusage ru;

  getrusage(RUSAGE_SELF, &ru);
  utime = ru.ru_utime.tv_sec + (ru.ru_utime.tv_usec / 1000000.00);
  stime = ru.ru_stime.tv_sec + (ru.ru_stime.tv_usec / 1000000.00);
  return (utime + stime);
#else
  return (clock() / (CLOCKS_PER_SEC * 1.00));
#endif
}

/* Dump uptime info out to dcc (guppy 9Jan99)
 */
void tell_verbose_uptime(int idx)
{
  char s[256], s1[121];
  time_t now2, hr, min;

  now2 = now - online_since;
  s[0] = 0;
  if (now2 > 86400) {
    /* days */
    sprintf(s, "%d day", (int) (now2 / 86400));
    if ((int) (now2 / 86400) >= 2)
      strcat(s, "s");
    strcat(s, ", ");
    now2 -= (((int) (now2 / 86400)) * 86400);
  }
  hr = (time_t) ((int) now2 / 3600);
  now2 -= (hr * 3600);
  min = (time_t) ((int) now2 / 60);
  sprintf(&s[strlen(s)], "%02d:%02d", (int) hr, (int) min);
  s1[0] = 0;
  if (backgrd)
    strcpy(s1, MISC_BACKGROUND);
  else {
    if (term_z >= 0)
      strcpy(s1, MISC_TERMMODE);
    else if (con_chan)
      strcpy(s1, MISC_STATMODE);
    else
      strcpy(s1, MISC_LOGMODE);
  }
  dprintf(idx, "%s %s  (%s)\n", MISC_ONLINEFOR, s, s1);
}

/* Dump status info out to dcc
 */
void tell_verbose_status(int idx)
{
  char s[256], s1[121], s2[81];
  char *vers_t, *uni_t;
  int i;
  time_t now2 = now - online_since, hr, min;
  double cputime, cache_total;
  struct utsname un;

  if (uname(&un) < 0) {
    vers_t = " ";
    uni_t  = "*unknown*";
  } else {
    vers_t = un.release;
    uni_t  = un.sysname;
  }

  i = count_users(userlist);
  dprintf(idx, "I am %s, running %s: %d user%s (mem: %uk).\n",
          botnetnick, ver, i, i == 1 ? "" : "s",
          (int) (expected_memory() / 1024));

  s[0] = 0;
  if (now2 > 86400) {
    /* days */
    sprintf(s, "%d day", (int) (now2 / 86400));
    if ((int) (now2 / 86400) >= 2)
      strcat(s, "s");
    strcat(s, ", ");
    now2 -= (((int) (now2 / 86400)) * 86400);
  }
  hr = (time_t) ((int) now2 / 3600);
  now2 -= (hr * 3600);
  min = (time_t) ((int) now2 / 60);
  sprintf(&s[strlen(s)], "%02d:%02d", (int) hr, (int) min);
  s1[0] = 0;
  if (backgrd)
    strlcpy(s1, MISC_BACKGROUND, sizeof s1);
  else {
    if (term_z >= 0)
      strlcpy(s1, MISC_TERMMODE, sizeof s1);
    else if (con_chan)
      strlcpy(s1, MISC_STATMODE, sizeof s1);
    else
      strlcpy(s1, MISC_LOGMODE, sizeof s1);
  }
  cputime = getcputime();
  if (cputime < 0)
    sprintf(s2, "CPU: unknown");
  else {
    hr = cputime / 60;
    cputime -= hr * 60;
    sprintf(s2, "CPU: %02d:%05.2f", (int) hr, cputime); /* Actually min/sec */
  }
  if (cache_hit + cache_miss) {      /* 2019, still can't divide by zero */
    cache_total = 100.0 * (cache_hit) / (cache_hit + cache_miss);
  } else cache_total = 0;
    dprintf(idx, "%s %s (%s) - %s - %s: %4.1f%%\n", MISC_ONLINEFOR,
            s, s1, s2, MISC_CACHEHIT, cache_total);

  dprintf(idx, "Configured with: " EGG_AC_ARGS "\n");
  if (admin[0])
    dprintf(idx, "Admin: %s\n", admin);

  dprintf(idx, "Config file: %s\n", configfile);
  dprintf(idx, "OS: %s %s\n", uni_t, vers_t);
  dprintf(idx, "Process ID: %d (parent %d)\n", getpid(), getppid());

  /* info library */
  dprintf(idx, "%s %s\n", MISC_TCLLIBRARY,
          ((interp) && (Tcl_Eval(interp, "info library") == TCL_OK)) ?
          tcl_resultstring() : "*unknown*");

  /* info tclversion/patchlevel */
  dprintf(idx, "%s %s (%s %s)\n", MISC_TCLVERSION,
          ((interp) && (Tcl_Eval(interp, "info patchlevel") == TCL_OK)) ?
          tcl_resultstring() : (Tcl_Eval(interp, "info tclversion") == TCL_OK) ?
          tcl_resultstring() : "*unknown*", MISC_TCLHVERSION, TCL_PATCH_LEVEL);

  if (tcl_threaded())
    dprintf(idx, "Tcl is threaded.\n");
#ifdef TLS
  dprintf(idx, "TLS support is enabled.\n"
  #if defined HAVE_EVP_PKEY_GET1_EC_KEY && defined HAVE_OPENSSL_MD5
               "TLS library: %s\n",
  #elif !defined HAVE_EVP_PKEY_GET1_EC_KEY && defined HAVE_OPENSSL_MD5
               "TLS library: %s\n             (no elliptic curve support)\n",
  #elif defined HAVE_EVP_PKEY_GET1_EC_KEY && !defined HAVE_OPENSSL_MD5
               "TLS library: %s\n             (no MD5 support)\n",
  #elif !defined HAVE_EVP_PKEY_GET1_EC_KEY && !defined HAVE_OPENSSL_MD5
               "TLS library: %s\n             (no elliptic curve or MD5 support)\n",
  #endif
          SSLeay_version(SSLEAY_VERSION));
#else
  dprintf(idx, "TLS support is not available.\n");
#endif
#ifdef IPV6
  dprintf(idx, "IPv6 support is enabled.\n"
#else
  dprintf(idx, "IPv6 support is not available.\n"
#endif
               "Socket table: %d/%d\n", threaddata()->MAXSOCKS, max_socks);
}

/* Show all internal state variables
 */
void tell_settings(int idx)
{
  char s[1024];
  int i;
  struct flag_record fr = { FR_GLOBAL, 0, 0, 0, 0, 0 };

  dprintf(idx, "Botnet nickname: %s\n", botnetnick);
  if (firewall[0])
    dprintf(idx, "Firewall: %s:%d\n", firewall, firewallport);
  dprintf(idx, "Userfile: %s\n", userfile);
  dprintf(idx, "Motd: %s\n",  motdfile);
  dprintf(idx, "Directories:\n");
#ifndef STATIC
  dprintf(idx, "  Help   : %s\n", helpdir);
  dprintf(idx, "  Modules: %s\n", moddir);
#else
  dprintf(idx, "  Help: %s\n", helpdir);
#endif
  fr.global = default_flags;

  build_flags(s, &fr, NULL);
  dprintf(idx, "%s [%s], %s: %s\n", MISC_NEWUSERFLAGS, s,
          MISC_NOTIFY, notify_new);
  if (owner[0])
    dprintf(idx, "%s: %s\n", MISC_PERMOWNER, owner);
  for (i = 0; i < max_logs; i++)
    if (logs[i].filename != NULL) {
      dprintf(idx, "Logfile #%d: %s on %s (%s: %s)\n", i + 1,
              logs[i].filename, logs[i].chname,
              masktype(logs[i].mask), maskname(logs[i].mask));
    }
  dprintf(idx, "Ignores last %d minute%s.\n", ignore_time,
          (ignore_time != 1) ? "s" : "");
}

void reaffirm_owners()
{
  char *p, *q, s[121];
  struct userrec *u;

  /* Please stop breaking this function. */
  if (owner[0]) {
    q = owner;
    p = strchr(q, ',');
    while (p) {
      strlcpy(s, q, (p - q) + 1);
      rmspace(s);
      u = get_user_by_handle(userlist, s);
      if (u)
        u->flags = sanity_check(u->flags | USER_OWNER);
      q = p + 1;
      p = strchr(q, ',');
    }
    strcpy(s, q);
    rmspace(s);
    u = get_user_by_handle(userlist, s);
    if (u)
      u->flags = sanity_check(u->flags | USER_OWNER);
  }
}

void chanprog()
{
  int i;

  admin[0]   = 0;
  helpdir[0] = 0;
  /* default mkcoblxs */
  conmask = LOG_MSGS|LOG_MODES|LOG_CMDS|LOG_MISC|LOG_BOTS|LOG_BOTMSG|LOG_FILES|LOG_SERV;

  for (i = 0; i < max_logs; i++)
    logs[i].flags |= LF_EXPIRING;

  /* Turn off read-only variables (make them write-able) for rehash */
  protect_readonly = 0;

  /* Now read it */
  if (!readtclprog(configfile))
    fatal(MISC_NOCONFIGFILE, 0);

  for (i = 0; i < max_logs; i++) {
    if (logs[i].flags & LF_EXPIRING) {
      if (logs[i].filename != NULL) {
        nfree(logs[i].filename);
        logs[i].filename = NULL;
      }
      if (logs[i].chname != NULL) {
        nfree(logs[i].chname);
        logs[i].chname = NULL;
      }
      if (logs[i].f != NULL) {
        fclose(logs[i].f);
        logs[i].f = NULL;
      }
      logs[i].mask = 0;
      logs[i].flags = 0;
    }
  }

  /* We should be safe now */
  call_hook(HOOK_REHASH);
  protect_readonly = 1;

  if (!botnetnick[0])
    set_botnetnick(origbotname);

  if (!botnetnick[0])
    fatal("I don't have a botnet nick!!\n", 0);

  if (!userfile[0])
    fatal(MISC_NOUSERFILE2, 0);

  if (!readuserfile(userfile, &userlist)) {
    if (!make_userfile) {
      char tmp[178];

      egg_snprintf(tmp, sizeof tmp, MISC_NOUSERFILE, configfile);
      fatal(tmp, 0);
    }
    printf("\n\n%s\n", MISC_NOUSERFILE2);
    if (module_find("server", 0, 0))
      printf(MISC_USERFCREATE1, origbotname);
    printf("%s\n\n", MISC_USERFCREATE2);
  } else if (make_userfile) {
    make_userfile = 0;
    printf("%s\n", MISC_USERFEXISTS);
  }

  if (helpdir[0])
    if (helpdir[strlen(helpdir) - 1] != '/')
      strcat(helpdir, "/");

  reaffirm_owners();
  check_tcl_event("userfile-loaded");
}

/* Reload the user file from disk
 */
void reload()
{
  if (!file_readable(userfile)) {
    putlog(LOG_MISC, "*", MISC_CANTRELOADUSER);
    return;
  }

  noshare = 1;
  clear_userlist(userlist);
  noshare = 0;
  userlist = NULL;
  if (!readuserfile(userfile, &userlist))
    fatal(MISC_MISSINGUSERF, 0);
  reaffirm_owners();
  add_hq_user();
  check_tcl_event("userfile-loaded");
  call_hook(HOOK_READ_USERFILE);
}

void rehash()
{
  call_hook(HOOK_PRE_REHASH);
  noshare = 1;
  clear_userlist(userlist);
  noshare = 0;
  userlist = NULL;
  chanprog();
  add_hq_user();
}

/*
 *    Brief venture into timers
 */

/* Add a timer
 */
unsigned long add_timer(tcl_timer_t ** stack, int elapse, int count,
                        char *cmd, unsigned long prev_id)
{
  tcl_timer_t *old = (*stack);

  *stack = nmalloc(sizeof **stack);
  (*stack)->next = old;
  (*stack)->mins = (*stack)->interval = elapse;
  (*stack)->count = count;
  (*stack)->cmd = nmalloc(strlen(cmd) + 1);
  strcpy((*stack)->cmd, cmd);
  /* If it's just being added back and already had an id,
   * don't create a new one.
   */
  if (prev_id > 0)
    (*stack)->id = prev_id;
  else
    (*stack)->id = timer_id++;
  return (*stack)->id;
}

/* Remove a timer, by id
 */
int remove_timer(tcl_timer_t ** stack, unsigned long id)
{
  tcl_timer_t *old;
  int ok = 0;

  while (*stack) {
    if ((*stack)->id == id) {
      ok++;
      old = *stack;
      *stack = ((*stack)->next);
      nfree(old->cmd);
      nfree(old);
    } else
      stack = &((*stack)->next);
  }
  return ok;
}

/* Check timers, execute the ones that have expired.
 */
void do_check_timers(tcl_timer_t ** stack)
{
  tcl_timer_t *mark = *stack, *old = NULL;
  char x[26];

  /* New timers could be added by a Tcl script inside a current timer
   * so i'll just clear out the timer list completely, and add any
   * unexpired timers back on.
   */
  *stack = NULL;
  while (mark) {
    if (mark->mins > 0)
      mark->mins--;
    old = mark;
    mark = mark->next;
    if (!old->mins) {
      snprintf(x, sizeof x, "timer%lu", old->id);
      do_tcl(x, old->cmd);
      if (old->count == 1) {
        nfree(old->cmd);
        nfree(old);
        continue;
      } else {
        old->mins = old->interval;
        if (old->count > 1)
          old->count--;
      }
    }
    old->next = *stack;
    *stack = old;
  }
}

/* Wipe all timers.
 */
void wipe_timers(Tcl_Interp *irp, tcl_timer_t **stack)
{
  tcl_timer_t *mark = *stack, *old;

  while (mark) {
    old = mark;
    mark = mark->next;
    nfree(old->cmd);
    nfree(old);
  }
  *stack = NULL;
}

/* Return list of timers
 */
void list_timers(Tcl_Interp *irp, tcl_timer_t *stack)
{
  char mins[10], count[10], id[26], *x;
  EGG_CONST char *argv[4];
  tcl_timer_t *mark;

  for (mark = stack; mark; mark = mark->next) {
    egg_snprintf(mins, sizeof mins, "%u", mark->mins);
    snprintf(id, sizeof id, "timer%lu", mark->id);
    egg_snprintf(count, sizeof count, "%u", mark->count);
    argv[0] = mins;
    argv[1] = mark->cmd;
    argv[2] = id;
    argv[3] = count;
    x = Tcl_Merge(sizeof(argv)/sizeof(*argv), argv);
    Tcl_AppendElement(irp, x);
    Tcl_Free((char *) x);
  }
}

int isowner(char *name) {
  char s[sizeof owner];
  char *sep = ", \t\n\v\f\r";
  char *word;

  strcpy(s, owner);
  for (word = strtok(s, sep); word; word = strtok(NULL, sep)) {
    if (!strcasecmp(name, word)) {
      return 1;
    }
  }
  return 0;
}

/*
 * Adds the -HQ user to the userlist and takes care of needed permissions
 */
void add_hq_user()
{
  if (!backgrd && term_z >= 0) {
    /* HACK: Workaround using dcc[].nick not to pass literal "-HQ" as a non-const arg */
    dcc[term_z].user = get_user_by_handle(userlist, dcc[term_z].nick);
    /* Make sure there's an innocuous -HQ user if needed */
    if (!dcc[term_z].user) {
      userlist = adduser(userlist, dcc[term_z].nick, "none", "-", USER_PARTY);
      dcc[term_z].user = get_user_by_handle(userlist, dcc[term_z].nick);
    }
    /* Give all useful flags: efjlmnoptuvx */
    dcc[term_z].user->flags = USER_EXEMPT | USER_FRIEND | USER_JANITOR |
                              USER_HALFOP | USER_MASTER | USER_OWNER | USER_OP |
                              USER_PARTY | USER_BOTMAST | USER_UNSHARED |
                              USER_VOICE | USER_XFER;
    /* Add to permowner list if there's place */
    if (strlen(owner) + sizeof EGG_BG_HANDLE < sizeof owner)
      strcat(owner, " " EGG_BG_HANDLE);

    /* Update laston info, gets cleared at rehash/reload */
    touch_laston(dcc[term_z].user, "partyline", now);
  }
}
