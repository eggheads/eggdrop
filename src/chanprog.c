/* 
 * chanprog.c -- handles:
 *   rmspace()
 *   maintaining the server list
 *   revenge punishment
 *   timers, utimers
 *   telling the current programmed settings
 *   initializing a lot of stuff and loading the tcl scripts
 * 
 * config file format changed 27jan1994 (Tcl outdates that)
 * dprintf'ized, 1nov1995
 * 
 * $Id: chanprog.c,v 1.18 1999/12/22 20:30:03 guppy Exp $
 */
/* 
 * Copyright (C) 1997  Robey Pointer
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
#if HAVE_GETRUSAGE
#include <sys/resource.h>
#if HAVE_SYS_RUSAGE_H
#include <sys/rusage.h>
#endif
#endif
#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif
#include "modules.h"

extern struct userrec *userlist;
extern log_t *logs;
extern Tcl_Interp *interp;
extern char ver[], botnetnick[], firewall[];
extern char motdfile[], userfile[], helpdir[], tempdir[];
extern char moddir[], notify_new[], owner[], configfile[];
extern time_t now, online_since;
extern int backgrd, term_z, con_chan, cache_hit, cache_miss, firewallport;
extern int default_flags, max_logs, conmask, protect_readonly, make_userfile;
extern int noshare, ignore_time;

tcl_timer_t *timer = NULL, *utimer = NULL;	/* timers (minutely) and
						 * utimers (secondly) */
unsigned long timer_id = 1;	/* next timer of any sort will have this
				 * number */
struct chanset_t *chanset = NULL;	/* channel list */
char admin[121] = "";		/* admin info */
char origbotname[NICKLEN + 1];
char botname[NICKLEN + 1];	/* primary botname */

/* remove space characters from beginning and end of string */
/* (more efficent by Fred1) */
void rmspace(char *s)
{
#define whitespace(c) ( ((c)==32) || ((c)==9) || ((c)==13) || ((c)==10) )
  char *p;

  if (*s == '\0')
	return;

  /* wipe end of string */
  for (p = s + strlen(s) - 1; ((whitespace(*p)) && (p >= s)); p--);
  if (p != s + strlen(s) - 1)
    *(p + 1) = 0;
  for (p = s; ((whitespace(*p)) && (*p)); p++);
  if (p != s)
    strcpy(s, p);
}

/* returns memberfields if the nick is in the member list */
memberlist *ismember(struct chanset_t *chan, char *nick)
{
  memberlist *x;

  x = chan->channel.member;
  while (x && x->nick[0] && rfc_casecmp(x->nick, nick))
    x = x->next;
  if (!x->nick[0])
    return NULL;
  return x;
}

/* find a chanset by channel name */
struct chanset_t *findchan(char *name)
{
  struct chanset_t *chan = chanset;

  while (chan != NULL) {
    if (!rfc_casecmp(chan->name, name))
      return chan;
    chan = chan->next;
  }
  return NULL;
}

/* stupid "caching" functions */
/* shortcut for get_user_by_host -- might have user record in one
 * of the channel caches */
struct userrec *check_chanlist(char *host)
{
  char *nick, *uhost, buf[UHOSTLEN];
  memberlist *m;
  struct chanset_t *chan;

  strncpy(buf, host, UHOSTMAX);
  buf[UHOSTMAX] = 0;	/* why is this case sanely done, when there
			 * are so many others? */
  uhost = buf;
  nick = splitnick(&uhost);
  for (chan = chanset; chan; chan = chan->next) {
    m = chan->channel.member;
    while (m && m->nick[0]) {
      if (!rfc_casecmp(nick, m->nick) &&
	  !strcasecmp(uhost, m->userhost))
	return m->user;
      m = m->next;
    }
  }
  return NULL;
}

/* shortcut for get_user_by_handle -- might have user record in channels */
struct userrec *check_chanlist_hand(char *hand)
{
  struct chanset_t *chan = chanset;
  memberlist *m;

  while (chan) {
    m = chan->channel.member;
    while (m && m->nick[0]) {
      if (m->user)
	if (!strcasecmp(m->user->handle, hand))
	  return m->user;
      m = m->next;
    }
    chan = chan->next;
  }
  return NULL;
}

/* clear the user pointers in the chanlists */
/* (necessary when a hostmask is added/removed or a user is added) */
void clear_chanlist()
{
  memberlist *m;
  struct chanset_t *chan = chanset;

  while (chan) {
    m = chan->channel.member;
    while (m && m->nick[0]) {
      m->user = NULL;
      m = m->next;
    }
    chan = chan->next;
  }
}

/* if this user@host is in a channel, set it (it was null) */
void set_chanlist(char *host, struct userrec *rec)
{
  char *nick, *uhost, buf[UHOSTLEN];
  memberlist *m;
  struct chanset_t *chan = chanset;

  Context;
  strncpy(buf, host, UHOSTMAX);
  buf[UHOSTMAX] = 0;
  uhost = buf;
  nick = splitnick(&uhost);
  while (chan) {
    m = chan->channel.member;
    while (m && m->nick[0]) {
      if (!rfc_casecmp(nick, m->nick) &&
	  !strcasecmp(uhost, m->userhost))
	m->user = rec;
      m = m->next;
    }
    chan = chan->next;
  }
}

/* memory we should be using */
int expmem_chanprog()
{
  int tot;
  tcl_timer_t *t;

  Context;
  tot = 0;
  for (t = timer; t; t = t->next) {
    tot += sizeof(tcl_timer_t);
    tot += strlen(t->cmd) + 1;
  }
  for (t = utimer; t; t = t->next) {
    tot += sizeof(tcl_timer_t);
    tot += strlen(t->cmd) + 1;
  }
  return tot;
}

/* dump uptime info out to dcc (guppy 9Jan99) */
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
    if (term_z)
      strcpy(s1, MISC_TERMMODE);
    else if (con_chan)
      strcpy(s1, MISC_STATMODE);
    else
      strcpy(s1, MISC_LOGMODE);
  }
  dprintf(idx, "%s %s  (%s)\n", MISC_ONLINEFOR, s, s1);
}

/* dump status info out to dcc */
void tell_verbose_status(int idx)
{
  char s[256], s1[121], s2[81];
  char *vers_t, *uni_t;
  int i;
  time_t now2, hr, min;

#if HAVE_GETRUSAGE
  struct rusage ru;

#else
#if HAVE_CLOCK
  clock_t cl;

#endif
#endif
#ifdef HAVE_UNAME
  struct utsname un;

  if (!uname(&un) < 0) {
#endif
    vers_t = " ";
    uni_t = "*unknown*";
#ifdef HAVE_UNAME
  } else {
    vers_t = un.release;
    uni_t = un.sysname;
  }
#endif

  i = count_users(userlist);
  dprintf(idx, "I am %s, running %s:  %d user%s (mem: %uk)\n",
	  botnetnick, ver, i, i == 1 ? "" : "s",
          (int) (expected_memory() / 1024));
  dprintf(idx, "Running on %s %s\n", uni_t, vers_t);
  if (admin[0])
    dprintf(idx, "Admin: %s\n", admin);
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
    if (term_z)
      strcpy(s1, MISC_TERMMODE);
    else if (con_chan)
      strcpy(s1, MISC_STATMODE);
    else
      strcpy(s1, MISC_LOGMODE);
  }
#if HAVE_GETRUSAGE
  getrusage(RUSAGE_SELF, &ru);
  hr = (int) ((ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) / 60);
  min = (int) ((ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) - (hr * 60));
  sprintf(s2, "CPU %02d:%02d", (int) hr, (int) min);	/* actally min/sec */
#else
#if HAVE_CLOCK
  cl = (clock() / CLOCKS_PER_SEC);
  hr = (int) (cl / 60);
  min = (int) (cl - (hr * 60));
  sprintf(s2, "CPU %02d:%02d", (int) hr, (int) min);	/* actually min/sec */
#else
  sprintf(s2, "CPU ???");
#endif
#endif
  dprintf(idx, "%s %s  (%s)  %s  %s %4.1f%%\n", MISC_ONLINEFOR,
	  s, s1, s2, MISC_CACHEHIT,
       100.0 * ((float) cache_hit) / ((float) (cache_hit + cache_miss)));
  strcpy(s, "info library");
  if ((interp) && (Tcl_Eval(interp, s) == TCL_OK))
    dprintf(idx, "%s %s\n", MISC_TCLLIBVER, interp->result);
}

/* show all internal state variables */
void tell_settings(int idx)
{
  char s[1024];
  int i;
  struct flag_record fr =
  {FR_GLOBAL, 0, 0, 0, 0, 0};

  dprintf(idx, "Botnet Nickname: %s\n", botnetnick);
  if (firewall[0])
    dprintf(idx, "Firewall: %s, port %d\n", firewall, firewallport);
  dprintf(idx, "Userfile: %s   Motd: %s\n", userfile, motdfile);
  dprintf(idx, "Directories:\n");
  dprintf(idx, "  Help    : %s\n", helpdir);
  dprintf(idx, "  Temp    : %s\n", tempdir);
#ifndef STATIC
  dprintf(idx, "  Modules : %s\n", moddir);
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
  dprintf(idx, "Ignores last %d mins\n", ignore_time);
}

void reaffirm_owners()
{
  char *p, *q, s[121];
  struct userrec *u;

  /* make sure default owners are +n */
  if (owner[0]) {
    q = owner;
    p = strchr(q, ',');
    while (p) {
      strncpy(s, q, p - q);
      s[p - q] = 0;
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

  admin[0] = 0;
  helpdir[0] = 0;
  tempdir[0] = 0;
  for (i = 0; i < max_logs; i++)
    logs[i].flags |= LF_EXPIRING;
  conmask = 0;
  /* turn off read-only variables (make them write-able) for rehash */
  protect_readonly = 0;
  /* now read it */
  Context;
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
  Context;
  protect_readonly = 1;
  if (!userfile[0])
    fatal(MISC_NOUSERFILE2, 0);
  if ((int) getuid() == 0) {
    /* perhaps you should make it run something innocent here ;)
     * like rm -rf /etc :) */
    printf("\n\n%s\n", MISC_ROOTWARN);
  }
  if (!readuserfile(userfile, &userlist)) {
    if (!make_userfile)
      fatal(MISC_NOUSERFILE, 0);
    printf("\n\n%s\n", MISC_USERFCREATE);
    if (module_find("server", 0, 0))
      printf(MISC_USERFCREATE1, origbotname);
    printf("%s\n\n", MISC_USERFCREATE2);
  } else if (make_userfile) {
     make_userfile = 0;
     printf("%s\n", MISC_USERFEXISTS);
  }
  Context;
  if (helpdir[0])
    if (helpdir[strlen(helpdir) - 1] != '/')
      strcat(helpdir, "/");
  if (tempdir[0])
    if (tempdir[strlen(tempdir) - 1] != '/')
      strcat(tempdir, "/");
  if (!botnetnick[0]) {
    strncpy(botnetnick, origbotname, HANDLEN);
    botnetnick[HANDLEN] = 0;
  }
  if (!botnetnick[0])
    fatal("I don't have a botnet nick!!\n", 0);
  Context;
  /* test tempdir: it's vital */
  {
    FILE *f;
    char s[161], rands[8];

    /* possible file race condition solved by using a random string 
     * and the process id in the filename */
    make_rand_str(rands, 7); /* create random string */
    sprintf(s, "%s.test-%u-%s", tempdir, getpid(), rands);
    f = fopen(s, "w");
    if (f == NULL)
      fatal(MISC_CANTWRITETEMP, 0);
    fclose(f);
    unlink(s);
  }
  Context;
  reaffirm_owners();
}

/* reload the user file from disk */
void reload()
{
  FILE *f;

  f = fopen(userfile, "r");
  if (f == NULL) {
    putlog(LOG_MISC, "*", MISC_CANTRELOADUSER);
    return;
  }
  fclose(f);
  noshare = 1;
  clear_userlist(userlist);
  noshare = 0;
  userlist = NULL;
  if (!readuserfile(userfile, &userlist))
    fatal(MISC_MISSINGUSERF, 0);
  Context;
  reaffirm_owners();
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
}

/* brief venture into timers */

/* add a timer */
unsigned long add_timer(tcl_timer_t ** stack, int elapse, char *cmd,
			unsigned long prev_id)
{
  tcl_timer_t *old = (*stack);

  *stack = (tcl_timer_t *) nmalloc(sizeof(tcl_timer_t));
  (*stack)->next = old;
  (*stack)->mins = elapse;
  (*stack)->cmd = (char *) nmalloc(strlen(cmd) + 1);
  strcpy((*stack)->cmd, cmd);
  /* if it's just being added back and already had an id,
   * don't create a new one */
  if (prev_id > 0)
    (*stack)->id = prev_id;
  else
    (*stack)->id = timer_id++;
  return (*stack)->id;
}

/* remove a timer, by id */
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

/* check timers, execute the ones that have expired */
void do_check_timers(tcl_timer_t ** stack)
{
  tcl_timer_t *mark = *stack, *old = NULL;
  char x[30];

  /* new timers could be added by a Tcl script inside a current timer */
  /* so i'll just clear out the timer list completely, and add any
   * unexpired timers back on */
  Context;
  *stack = NULL;
  while (mark) {
    Context;
    if (mark->mins > 0)
      mark->mins--;
    old = mark;
    mark = mark->next;
    if (old->mins == 0) {
      Context;
      simple_sprintf(x, "timer%d", old->id);
      do_tcl(x, old->cmd);
      nfree(old->cmd);
      nfree(old);
    } else {
      Context;
      old->next = *stack;
      *stack = old;
    }
  }
}

/* wipe all timers */
void wipe_timers(Tcl_Interp * irp, tcl_timer_t ** stack)
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

/* return list of timers */
void list_timers(Tcl_Interp * irp, tcl_timer_t * stack)
{
  tcl_timer_t *mark = stack;
  char mins[10], id[20], *argv[3], *x;

  while (mark != NULL) {
    sprintf(mins, "%u", mark->mins);
    sprintf(id, "timer%lu", mark->id);
    argv[0] = mins;
    argv[1] = mark->cmd;
    argv[2] = id;
    x = Tcl_Merge(3, argv);
    Tcl_AppendElement(irp, x);
    Tcl_Free((char *) x);
    mark = mark->next;
  }
}

/* Oddly enough, written by proton (Emech's coder) */

int isowner(char *name)
{
  char *pa, *pb;
  char nl, pl;

  if (!owner || !*owner)
    return (0);
  if (!name || !*name)
    return (0);
  nl = strlen(name);
  pa = owner;
  pb = owner;
  while (1) {
    while (1) {
      if ((*pb == 0) || (*pb == ',') || (*pb == ' '))
	break;
      pb++;
    }
    pl = (unsigned int) pb - (unsigned int) pa;
    if ((pl == nl) && (!strncasecmp(pa, name, nl)))
      return (1);
    while (1) {
      if ((*pb == 0) || ((*pb != ',') && (*pb != ' ')))
	break;
      pb++;
    }
    if (*pb == 0)
      return (0);
    pa = pb;
  }
}
