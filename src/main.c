/* 
 * main.c -- handles:
 * changing nicknames when the desired nick is in use
 * flood detection
 * signal handling
 * command line arguments
 * 
 * dprintf'ized, 15nov1995
 */
/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * 
 * Parts of this eggdrop source code are copyright (c)1999 Eggheads
 * and is distributed according to the GNU general public license.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * The author (Robey Pointer) can be reached at:  robey@netcom.com
 * NOTE: Robey is no long working on this code, there is a discussion
 * list avaliable at eggheads@eggheads.org.
 */

#include "main.h"
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <setjmp.h>
#ifdef STOP_UAC			/* osf/1 complains a lot */
#include <sys/sysinfo.h>
#define UAC_NOPRINT    0x00000001	/* Don't report unaligned fixups */
#endif
/* some systems have a working sys/wait.h even though configure will
 * decide it's not bsd compatable.  oh well. */
#include "chan.h"
#include "modules.h"
#include "tandem.h"

#ifndef _POSIX_SOURCE
/* solaris needs this */
#define _POSIX_SOURCE 1
#endif

extern char origbotname[];
extern int dcc_total;
extern struct dcc_t *dcc;
extern int conmask;
extern struct userrec *userlist;
extern int cache_hit, cache_miss;
extern char userfile[];
extern struct chanset_t *chanset;
extern char botnetnick[];
extern log_t *logs;
extern Tcl_Interp *interp;
extern int max_logs;
extern tcl_timer_t *timer, *utimer;
extern jmp_buf alarmret;
extern int quick_logs;		/* dw */

/*
 * Please use the PATCH macro instead of directly altering the version
 * string from now on (it makes it much easier to maintain patches).
 * Also please read the README file regarding your rights to distribute
 * modified versions of this bot.
 */

char egg_version[1024] = "1.3.29";
int egg_numver = 1032900;

char notify_new[121] = "";	/* person to send a note to for new users */
int default_flags = 0;		/* default user flags and */
int default_uflags = 0;		/* default userdefinied flags for people */
				/* who say 'hello' or for .adduser */

int backgrd = 1;		/* run in the background? */
int con_chan = 0;		/* foreground: constantly display channel
				 * stats? */
int term_z = 0;			/* foreground: use the terminal as a party
				 * line? */
char configfile[121] = "egg.config";	/* name of the config file */
char helpdir[121];		/* directory of help files (if used) */
char textdir[121] = "";		/* directory for text files that get dumped */
int keep_all_logs = 0;		/* never erase logfiles, no matter how old
				 * they are? */
time_t online_since;		/* unix-time that the bot loaded up */
int make_userfile = 0;		/* using bot in make-userfile mode? (first
				 * user to 'hello' becomes master) */
char owner[121] = "";		/* permanent owner(s) of the bot */
char pid_file[40];		/* name of the file for the pid to be
				 * stored in */
int save_users_at = 0;		/* how many minutes past the hour to
				 * save the userfile? */
int notify_users_at = 0;	/* how many minutes past the hour to
				 * notify users of notes? */
int switch_logfiles_at = 300;	/* when (military time) to switch logfiles */
char version[81];		/* version info (long form) */
char ver[41];			/* version info (short form) */
char egg_xtra[2048];		/* patch info */
int use_stderr = 1;		/* send stuff to stderr instead of logfiles? */
int do_restart = 0;		/* .restart has been called, restart asap */
int die_on_sighup = 0;		/* die if bot receives SIGHUP */
int die_on_sigterm = 0;		/* die if bot receives SIGTERM */
int resolve_timeout = 15;	/* hostname/address lookup timeout */
time_t now;			/* duh, now :) */

/* context storage for fatal crashes */
char cx_file[16][30];
char cx_note[16][256];
int cx_line[16];
int cx_ptr = 0;

void fatal(char *s, int recoverable)
{
  int i;

  putlog(LOG_MISC, "*", "* %s", s);
  flushlogs();
  for (i = 0; i < dcc_total; i++)
    killsock(dcc[i].sock);
  unlink(pid_file);
  if (!recoverable)
    exit(1);
}

int expmem_chanprog(), expmem_users(), expmem_misc(), expmem_dccutil(),
 expmem_botnet(), expmem_tcl(), expmem_tclhash(), expmem_net(),
 expmem_modules(int), expmem_language(), expmem_tcldcc();

/* for mem.c : calculate memory we SHOULD be using */
int expected_memory()
{
  int tot;

  context;
  tot = expmem_chanprog() + expmem_users() + expmem_misc() +
    expmem_dccutil() + expmem_botnet() + expmem_tcl() + expmem_tclhash() +
    expmem_net() + expmem_modules(0) + expmem_language() + expmem_tcldcc();
  return tot;
}

static void check_expired_dcc()
{
  int i;

  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type && dcc[i].type->timeout_val &&
	((now - dcc[i].timeval) > *(dcc[i].type->timeout_val))) {
      if (dcc[i].type->timeout)
	dcc[i].type->timeout(i);
      else if (dcc[i].type->eof)
	dcc[i].type->eof(i);
      else
	continue;
      /* only timeout 1 socket per cycle, too risky for more */
      return;
    }
}

static int nested_debug = 0;

void write_debug()
{
  int x;
  char s[80];
  int y;

  if (nested_debug) {
    /* yoicks, if we have this there's serious trouble */
    /* all of these are pretty reliable, so we'll try these */
    /* dont try and display context-notes in here, it's _not_ safe <cybah> */
    x = creat("DEBUG.DEBUG", 0644);
    setsock(x, SOCK_NONSOCK);
    if (x >= 0) {
      strcpy(s, ctime(&now));
      dprintf(-x, "Debug (%s) written %s", ver, s);
      dprintf(-x, "Please report problem to eggheads@eggheads.org");
      dprintf(-x, "after a visit to http://www.eggheads.org/bugs.html");
      dprintf(-x, "Full Patch List: %s\n", egg_xtra);
      dprintf(-x, "Context: ");
      cx_ptr = cx_ptr & 15;
      for (y = ((cx_ptr + 1) & 15); y != cx_ptr; y = ((y + 1) & 15))
	dprintf(-x, "%s/%d,\n         ", cx_file[y], cx_line[y]);
      dprintf(-x, "%s/%d\n\n", cx_file[y], cx_line[y]);
      killsock(x);
      close(x);
    }
    exit(1);			/* dont even try & tell people about, that may
				 * have caused the fault last time */
  } else
    nested_debug = 1;
  putlog(LOG_MISC, "*", "* Last context: %s/%d [%s]", cx_file[cx_ptr],
	 cx_line[cx_ptr], cx_note[cx_ptr][0] ? cx_note[cx_ptr] : "");
  putlog(LOG_MISC, "*", "* Please REPORT this BUG!");
  putlog(LOG_MISC, "*", "* Check doc/BUG-REPORT on how to do so.");
  
x = creat("DEBUG", 0644);
  setsock(x, SOCK_NONSOCK);
  if (x < 0) {
    putlog(LOG_MISC, "*", "* Failed to write DEBUG");
  } else {
    strcpy(s, ctime(&now));
    dprintf(-x, "Debug (%s) written %s", ver, s);
    dprintf(-x, "Full Patch List: %s\n", egg_xtra);
#ifdef STATIC
    dprintf(-x, "STATICALLY LINKED\n");
#endif
    strcpy(s, "info library");
    if (interp && (Tcl_Eval(interp, s) == TCL_OK))
      dprintf(-x, "Using tcl library: %s (header version %s)\n",
	      interp->result, TCL_VERSION);
    dprintf(-x, "Compile flags: %s\n", CCFLAGS);
    dprintf(-x, "Link flags   : %s\n", LDFLAGS);
    dprintf(-x, "Strip flags  : %s\n", STRIPFLAGS);
    dprintf(-x, "Context: ");
    cx_ptr = cx_ptr & 15;
    for (y = ((cx_ptr + 1) & 15); y != cx_ptr; y = ((y + 1) & 15))
      dprintf(-x, "%s/%d, [%s]\n         ", cx_file[y], cx_line[y],
	      (cx_note[y][0]) ? cx_note[y] : "");
    dprintf(-x, "%s/%d [%s]\n\n", cx_file[cx_ptr], cx_line[cx_ptr],
	    (cx_note[cx_ptr][0]) ? cx_note[cx_ptr] : "");
    tell_dcc(-x);
    dprintf(-x, "\n");
    debug_mem_to_dcc(-x);
    killsock(x);
    close(x);
    putlog(LOG_MISC, "*", "* Wrote DEBUG");
  }
}

static void got_bus(int z)
{
  write_debug();
  fatal("BUS ERROR -- CRASHING!", 1);
#ifdef SA_RESETHAND
  kill(getpid(), SIGBUS);
#else
  exit(1);
#endif
}

static void got_segv(int z)
{
  write_debug();
  fatal("SEGMENT VIOLATION -- CRASHING!", 1);
#ifdef SA_RESETHAND
  kill(getpid(), SIGSEGV);
#else
  exit(1);
#endif
}

static void got_fpe(int z)
{
  write_debug();
  fatal("FLOATING POINT ERROR -- CRASHING!", 0);
}

static void got_term(int z)
{
  write_userfile(-1);
  check_tcl_event("sigterm");
  if (die_on_sigterm) {
    botnet_send_chat(-1, botnetnick, "ACK, I've been terminated!");
    fatal("TERMINATE SIGNAL -- SIGNING OFF", 0);
  } else {
    putlog(LOG_MISC, "*", "RECEIVED TERMINATE SIGNAL (IGNORING)");
  }
}

static void got_quit(int z)
{
  check_tcl_event("sigquit");
  putlog(LOG_MISC, "*", "RECEIVED QUIT SIGNAL (IGNORING)");
  return;
}

static void got_hup(int z)
{
  write_userfile(-1);
  check_tcl_event("sighup");
  if (die_on_sighup) {
    fatal("HANGUP SIGNAL -- SIGNING OFF", 0);
  } else
    putlog(LOG_MISC, "*", "Received HUP signal: rehashing...");
  do_restart = -2;
  return;
}

static void got_alarm(int z)
{
  /* a call to resolver (gethostbyname, etc) timed out */
  longjmp(alarmret, 1);
  /* STUPID STUPID STUPID */
  /* return; */
}

/* got ILL signal -- log context and continue */
static void got_ill(int z)
{
  check_tcl_event("sigill");
  putlog(LOG_MISC, "*", "* Context: %s/%d [%s]", cx_file[cx_ptr],
	 cx_line[cx_ptr], (cx_note[cx_ptr][0]) ? cx_note[cx_ptr] : "");
}

static void do_arg(char *s)
{
  int i;

  if (s[0] == '-')
    for (i = 1; i < strlen(s); i++) {
      if (s[i] == 'n')
	backgrd = 0;
      if (s[i] == 'c') {
	con_chan = 1;
	term_z = 0;
      }
      if (s[i] == 't') {
	con_chan = 0;
	term_z = 1;
      }
      if (s[i] == 'm')
	make_userfile = 1;
      if (s[i] == 'v') {
	char x[256], *z = x;

	strcpy(x, egg_version);
	newsplit(&z);
	newsplit(&z);
	printf("%s\n", version);
	if (z[0])
	  printf("  (patches: %s)\n", z);
	exit(0);
      }
      if (s[i] == 'h') {
	printf("\n%s\n\n", version);
	printf(EGG_USAGE);
	printf("\n");
	exit(0);
      }
  } else
    strcpy(configfile, s);
}

void backup_userfile()
{
  char s[150];

  putlog(LOG_MISC, "*", USERF_BACKUP);
  strcpy(s, userfile);
  strcat(s, "~bak");
  copyfile(userfile, s);
}

/* timer info: */
static int lastmin = 99;
static time_t then;
static struct tm *nowtm;

/* rally BB, this is not QUITE as bad as it seems <G> */
/* ONCE A SECOND */
static void core_secondly()
{
  static int cnt = 0;
  int miltime;

  do_check_timers(&utimer);	/* secondly timers */
  cnt++;
  if (cnt >= 10) {		/* every 10 seconds */
    cnt = 0;
    check_expired_dcc();
    if (con_chan && !backgrd) {
      dprintf(DP_STDOUT, "\033[2J\033[1;1H");
      tell_verbose_status(DP_STDOUT);
      do_module_report(DP_STDOUT, 0, "server");
      do_module_report(DP_STDOUT, 0, "channels");
      tell_mem_status_dcc(DP_STDOUT);
    }
  }
  context;
  nowtm = localtime(&now);
  if (nowtm->tm_min != lastmin) {
    int i = 0;

    /* once a minute */
    lastmin = (lastmin + 1) % 60;
    call_hook(HOOK_MINUTELY);
    check_expired_ignores();
    autolink_cycle(NULL);	/* attempt autolinks */
    /* in case for some reason more than 1 min has passed: */
    while (nowtm->tm_min != lastmin) {
      /* timer drift, dammit */
      debug2("timer: drift (lastmin=%d, now=%d)", lastmin, nowtm->tm_min);
      context;
      i++;
      lastmin = (lastmin + 1) % 60;
      call_hook(HOOK_MINUTELY);
    }
    if (i > 1)
      putlog(LOG_MISC, "*", "(!) timer drift -- spun %d minutes", i);
    miltime = (nowtm->tm_hour * 100) + (nowtm->tm_min);
    context;
    if (((int) (nowtm->tm_min / 5) * 5) == (nowtm->tm_min)) {	/* 5 min */
      call_hook(HOOK_5MINUTELY);
      check_botnet_pings();
      context;
      if (quick_logs == 0) {
	flushlogs();
	check_logsize();
      }
      if (miltime == 0) {	/* at midnight */
	char s[128];
	int j;

	s[my_strcpy(s, ctime(&now)) - 1] = 0;
	putlog(LOG_MISC, "*", "--- %.11s%s", s, s + 20);
	backup_userfile();
	for (j = 0; j < max_logs; j++) {
	  if (logs[j].filename != NULL && logs[j].f != NULL) {
	    fclose(logs[j].f);
	    logs[j].f = NULL;
	  }
	}
      }
    }
    context;
    if (nowtm->tm_min == notify_users_at)
      call_hook(HOOK_HOURLY);
    context;			/* these no longer need checking since they are
				 * all check vs minutely settings and we only
				 * get this far on the minute */
    if (miltime == switch_logfiles_at) {
      call_hook(HOOK_DAILY);
      if (!keep_all_logs) {
	putlog(LOG_MISC, "*", MISC_LOGSWITCH);
	for (i = 0; i < max_logs; i++)
	  if (logs[i].filename) {
	    char s[1024];

	    if (logs[i].f) {
	      fclose(logs[i].f);
	      logs[i].f = NULL;
	    }
	    simple_sprintf(s, "%s.yesterday", logs[i].filename);
	    unlink(s);
	    movefile(logs[i].filename, s);
	  }
      }
    }
  }
}

static void core_minutely()
{
  context;
  check_tcl_time(nowtm);
  do_check_timers(&timer);
  context;
  if (quick_logs != 0) {
    flushlogs();
    check_logsize();
  }
}

static void core_hourly()
{
  context;
  write_userfile(-1);
}

static void event_rehash()
{
  context;
  check_tcl_event("rehash");
}

static void event_prerehash()
{
  context;
  check_tcl_event("prerehash");
}

static void event_save()
{
  context;
  check_tcl_event("save");
}

static void event_logfile()
{
  context;
  check_tcl_event("logfile");
}

void kill_tcl();
extern module_entry *module_list;
void restart_chons();

#ifdef STATIC
void check_static(char *, char *(*)());

#include "mod/static.h"
#endif
int init_mem(), init_dcc_max(), init_userent(), init_misc(), init_bots(),
 init_net(), init_modules(), init_tcl(), init_language(int);

int main(int argc, char **argv)
{
  int xx, i;
  char buf[520], s[520];
  FILE *f;
  struct sigaction sv;
  struct chanset_t *chan;

  /* initialise context list */
  for (i = 0; i < 16; i++) {
    context;
  }
#include "patch.h"
  /* version info! */
  sprintf(ver, "eggdrop v%s", egg_version);
  sprintf(version, "Eggdrop v%s  (c)1997 Robey Pointer (c)1999 Eggheads", egg_version);
  /* now add on the patchlevel (for Tcl) */
  sprintf(&egg_version[strlen(egg_version)], " %u", egg_numver);
  strcat(egg_version, egg_xtra);
  context;
#ifdef STOP_UAC
  {
    int nvpair[2];

    nvpair[0] = SSIN_UACPROC;
    nvpair[1] = UAC_NOPRINT;
    setsysinfo(SSI_NVPAIRS, (char *) nvpair, 1, NULL, 0);
  }
#endif
  /* set up error traps: */
  sv.sa_handler = got_bus;
  sigemptyset(&sv.sa_mask);
#ifdef SA_RESETHAND
  sv.sa_flags = SA_RESETHAND;
#else
  sv.sa_flags = 0;
#endif
  sigaction(SIGBUS, &sv, NULL);
  sv.sa_handler = got_segv;
  sigaction(SIGSEGV, &sv, NULL);
#ifdef SA_RESETHAND
  sv.sa_flags = 0;
#endif
  sv.sa_handler = got_fpe;
  sigaction(SIGFPE, &sv, NULL);
  sv.sa_handler = got_term;
  sigaction(SIGTERM, &sv, NULL);
  sv.sa_handler = got_hup;
  sigaction(SIGHUP, &sv, NULL);
  sv.sa_handler = got_quit;
  sigaction(SIGQUIT, &sv, NULL);
  sv.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sv, NULL);
  sv.sa_handler = got_ill;
  sigaction(SIGILL, &sv, NULL);
  sv.sa_handler = got_alarm;
  sigaction(SIGALRM, &sv, NULL);
  /* initialize variables and stuff */
  now = time(NULL);
  chanset = NULL;
  nowtm = localtime(&now);
  lastmin = nowtm->tm_min;
  srandom(now);
  init_mem();
  init_language(1);
  if (argc > 1)
    for (i = 1; i < argc; i++)
      do_arg(argv[i]);
  printf("\n%s\n", version);
  init_dcc_max();
  init_userent();
  init_misc();
  init_bots();
  init_net();
  init_modules();
  init_tcl();
  init_language(0);
#ifdef STATIC
  link_statics();
#endif
  context;
  strcpy(s, ctime(&now));
  s[strlen(s) - 1] = 0;
  strcpy(&s[11], &s[20]);
  putlog(LOG_ALL, "*", "--- Loading %s (%s)", ver, s);
  chanprog();
  context;
  if (encrypt_pass == 0) {
    printf(MOD_NOCRYPT);
    exit(1);
  }
  i = 0;
  for (chan = chanset; chan; chan = chan->next)
    i++;
  putlog(LOG_ALL, "*", "=== %s: %d channels, %d users.",
	 botnetnick, i, count_users(userlist));
  cache_miss = 0;
  cache_hit = 0;
  context;
  sprintf(pid_file, "pid.%s", botnetnick);
  context;
  /* check for pre-existing eggdrop! */
  f = fopen(pid_file, "r");
  if (f != NULL) {
    fgets(s, 10, f);
    xx = atoi(s);
    kill(xx, SIGCHLD);		/* meaningless kill to determine if pid
				 * is used */
    if (errno != ESRCH) {
      printf(EGG_RUNNING1, origbotname);
      printf(EGG_RUNNING2, pid_file);
      exit(1);
    }
  }
  context;
  /* move into background? */
  if (backgrd) {
    xx = fork();
    if (xx == -1)
      fatal("CANNOT FORK PROCESS.", 0);
    if (xx != 0) {
      FILE *fp;

      /* need to attempt to write pid now, not later */
      unlink(pid_file);
      fp = fopen(pid_file, "w");
      if (fp != NULL) {
	fprintf(fp, "%u\n", xx);
	if (fflush(fp)) {
	  /* kill bot incase a botchk is run from crond */
	  printf(EGG_NOWRITE, pid_file);
	  printf("  Try freeing some disk space\n");
	  fclose(fp);
	  unlink(pid_file);
	  exit(1);
	}
	fclose(fp);
      } else
	printf(EGG_NOWRITE, pid_file);
      printf("Launched into the background  (pid: %d)\n\n", xx);
#if HAVE_SETPGID
      setpgid(xx, xx);
#endif
      exit(0);
    }
  }
  use_stderr = 0;		/* stop writing to stderr now */
  xx = getpid();
  if ((xx != 0) && (!backgrd)) {
    FILE *fp;

    /* write pid to file */
    unlink(pid_file);
    fp = fopen(pid_file, "w");
    if (fp != NULL) {
      fprintf(fp, "%u\n", xx);
      if (fflush(fp)) {
	/* let the bot live since this doesn't appear to be a botchk */
	printf(EGG_NOWRITE, pid_file);
	fclose(fp);
	unlink(pid_file);
      }
      fclose(fp);
    } else
      printf(EGG_NOWRITE, pid_file);
  }
  if (backgrd) {
    /* ok, try to disassociate from controlling terminal */
    /* (finger cross) */
#if HAVE_SETPGID
    setpgid(0, 0);
#endif
    /* close out stdin/out/err */
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
    /* tcl wants those file handles kept open */
/*    close(0); close(1); close(2);  */
  }
  /* terminal emulating dcc chat */
  if ((!backgrd) && (term_z)) {
    int n = new_dcc(&DCC_CHAT, sizeof(struct chat_info));

    dcc[n].addr = iptolong(getmyip());
    dcc[n].sock = STDOUT;
    dcc[n].timeval = now;
    dcc[n].u.chat->con_flags = conmask;
    dcc[n].u.chat->strip_flags = STRIP_ALL;
    dcc[n].status = STAT_ECHO;
    strcpy(dcc[n].nick, "HQ");
    strcpy(dcc[n].host, "llama@console");
    dcc[n].user = get_user_by_handle(userlist, "HQ");
    /* make sure there's an innocuous HQ user if needed */
    if (!dcc[n].user) {
      adduser(userlist, "HQ", "none", "-", USER_PARTY);
      dcc[n].user = get_user_by_handle(userlist, "HQ");
    }
    setsock(STDOUT, 0);		/* entry in net table */
    dprintf(n, "\n### ENTERING DCC CHAT SIMULATION ###\n\n");
    dcc_chatter(n);
  }
  then = now;
  online_since = now;
  autolink_cycle(NULL);		/* hurry and connect to tandem bots */
  add_help_reference("cmds1.help");
  add_help_reference("cmds2.help");
  add_help_reference("core.help");
  add_hook(HOOK_SECONDLY, core_secondly);
  add_hook(HOOK_MINUTELY, core_minutely);
  add_hook(HOOK_HOURLY, core_hourly);
  add_hook(HOOK_REHASH, event_rehash);
  add_hook(HOOK_PRE_REHASH, event_prerehash);
  add_hook(HOOK_USERFILE, event_save);
  add_hook(HOOK_DAILY, event_logfile);

  debug0("main: entering loop");
  while (1) {
    int socket_cleanup = 0;

    context;
    /* process a single tcl event */
    Tcl_DoOneEvent(TCL_ALL_EVENTS | TCL_DONT_WAIT);
    /* lets move some of this here, reducing the numer of actual
     * calls to periodic_timers */
    now = time(NULL);
    random();			/* woop, lets really jumble things */
    if (now != then) {		/* once a second */
      call_hook(HOOK_SECONDLY);
      then = now;
    }
    context;
    /* only do this every so often */
    if (!socket_cleanup) {
      /* clean up sockets that were just left for dead */
      for (i = 0; i < dcc_total; i++) {
	if (dcc[i].type == &DCC_LOST) {
	  dcc[i].type = (struct dcc_table *) (dcc[i].sock);
	  lostdcc(i);
	  i--;
	}
      }
      /* check for server or dcc activity */
      dequeue_sockets();
      socket_cleanup = 5;
    } else
      socket_cleanup--;
    /* new net routines: help me mary! */
    xx = sockgets(buf, &i);
    if (xx >= 0) {		/* non-error */
      int idx;

      context;
      for (idx = 0; idx < dcc_total; idx++)
	if (dcc[idx].sock == xx) {
	  if (dcc[idx].type && dcc[idx].type->activity)
	    dcc[idx].type->activity(idx, buf, i);
	  else
	    putlog(LOG_MISC, "*",
		   "!!! untrapped dcc activity: type %s, sock %d",
		   dcc[idx].type->name, dcc[idx].sock);
	  break;
	}
    } else if (xx == -1) {	/* EOF from someone */
      int idx;

      if ((i == STDOUT) && !backgrd)
	fatal("END OF FILE ON TERMINAL", 0);
      context;
      for (idx = 0; idx < dcc_total; idx++)
	if (dcc[idx].sock == i) {
	  if (dcc[idx].type && dcc[idx].type->eof)
	    dcc[idx].type->eof(idx);
	  else {
	    putlog(LOG_MISC, "*",
		   "*** ATTENTION: DEAD SOCKET (%d) OF TYPE %s UNTRAPPED",
		   xx, dcc[idx].type ? dcc[idx].type->name : "*UNKNOWN*");
	    killsock(xx);
	    lostdcc(idx);
	  }
	  idx = dcc_total + 1;
	}
      if (idx == dcc_total) {
	putlog(LOG_MISC, "*",
	       "(@) EOF socket %d, not a dcc socket, not anything.",
	       xx);
	close(xx);
	killsock(xx);
      }
    } else if ((xx == -2) && (errno != EINTR)) {	/* select() error */
      context;
      putlog(LOG_MISC, "*", "* Socket error #%d; recovering.", errno);
      for (i = 0; i < dcc_total; i++) {
	if ((fcntl(dcc[i].sock, F_GETFD, 0) == -1) && (errno = EBADF)) {
	  if (dcc[i].type == &DCC_LOST)
	    dcc[i].type = (struct dcc_table *) (dcc[i].sock);
	  putlog(LOG_MISC, "*",
		 "DCC socket %d (type %d, name '%s') expired -- pfft",
		 dcc[i].sock, dcc[i].type, dcc[i].nick);
	  killsock(dcc[i].sock);
	  lostdcc(i);
	  i--;
	}
      }
    } else if (xx == (-3)) {
      call_hook(HOOK_IDLE);
      socket_cleanup = 0;	/* if we've been idle, cleanup & flush */
    }
    if (do_restart) {
      if (do_restart == -2)
	rehash();
      else {
	/* unload as many modules as possible */
	int f = 1;
	module_entry *p;
	Function x;
	char xx[256];

	while (f) {
	  f = 0;
	  for (p = module_list; p != NULL; p = p->next) {
	    dependancy *d = dependancy_list;
	    int ok = 1;

	    while (ok && d) {
	      if (d->needed == p)
		ok = 0;
	      d = d->next;
	    }
	    if (ok) {
	      strcpy(xx, p->name);
	      if (module_unload(xx, origbotname) == NULL) {
		f = 1;
		break;
	      }
	    }
	  }
	}
	p = module_list;
	if (p && p->next && p->next->next)
	  /* should be only 2 modules now -
	   * blowfish & eggdrop */
	  putlog(LOG_MISC, "*", MOD_STAGNANT);
	context;
	flushlogs();
	context;
	kill_tcl();
	init_tcl();
	init_language(0);
	x = p->funcs[MODCALL_START];
	x(0);
	rehash();
	restart_chons();
      }
      do_restart = 0;
    }
  }
}
