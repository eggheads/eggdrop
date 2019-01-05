/*
 * main.c -- handles:
 *   core event handling
 *   signal handling
 *   command line arguments
 *   context and assert debugging
 */
/*
 * Copyright (C) 1997 Robey Pointer
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
/*
 * The author (Robey Pointer) can be reached at:  robey@netcom.com
 * NOTE: Robey is no long working on this code, there is a discussion
 * list available at eggheads@eggheads.org.
 */

/* We need config.h for CYGWIN_HACKS, but windows.h must be included before
 * eggdrop headers, because the malloc/free/Context macros break the inclusion.
 * The SSL undefs are a workaround for bug #2182 in openssl with msys/mingw.
 */
#include <config.h>
#ifdef CYGWIN_HACKS
#  include <windows.h>
#  undef X509_NAME
#  undef X509_EXTENSIONS
#  undef X509_CERT_PAIR
#  undef PKCS7_ISSUER_AND_SERIAL
#  undef PKCS7_SIGNER_INFO
#  undef OCSP_REQUEST
#  undef OCSP_RESPONSE
#endif

#include "main.h"

#include <errno.h>
#include <fcntl.h>
#include <resolv.h>
#include <setjmp.h>
#include <signal.h>

#ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else
#    include <time.h>
#  endif
#endif

#ifdef STOP_UAC                         /* OSF/1 complains a lot */
#  include <sys/sysinfo.h>
#  define UAC_NOPRINT 0x00000001        /* Don't report unaligned fixups */
#endif

#include "version.h"
#include "chan.h"
#include "modules.h"
#include "tandem.h"
#include "bg.h"

#ifdef DEBUG                            /* For debug compile */
#  include <sys/resource.h>             /* setrlimit() */
#endif

#ifdef HAVE_GETRANDOM
#  include <sys/random.h>
#endif

#ifndef _POSIX_SOURCE
#  define _POSIX_SOURCE 1               /* Solaris needs this */
#endif

extern char origbotname[], botnetnick[]; 
extern char userfile[121];        /* 121 = sizeof userfile from users.c */
extern int dcc_total, conmask, cache_hit, cache_miss, max_logs, quick_logs,
           quiet_save;
extern struct dcc_t *dcc;
extern struct userrec *userlist;
extern struct chanset_t *chanset;
extern log_t *logs;
extern Tcl_Interp *interp;
extern tcl_timer_t *timer, *utimer;
extern sigjmp_buf alarmret;
time_t now;
static int argc;
static char **argv;

/*
 * Please use the PATCH macro instead of directly altering the version
 * string from now on (it makes it much easier to maintain patches).
 * Also please read the README file regarding your rights to distribute
 * modified versions of this bot.
 */

char egg_version[1024] = EGG_STRINGVER;
int egg_numver = EGG_NUMVER;
#ifdef EGG_PATCH
char egg_patch[] = EGG_PATCH;
#endif

char notify_new[121] = "";      /* Person to send a note to for new users */
int default_flags = 0;          /* Default user flags                     */
int default_uflags = 0;         /* Default user-definied flags            */

int backgrd = 1;    /* Run in the background?                        */
int con_chan = 0;   /* Foreground: constantly display channel stats? */
int term_z = -1;    /* Foreground: use the terminal as a partyline?  */
int use_stderr = 1; /* Send stuff to stderr instead of logfiles?     */

char configfile[121] = "eggdrop.conf";  /* Default config file name */
char pid_file[121];                     /* Name of the pid file     */
char helpdir[121] = "help/";            /* Directory of help files  */
char textdir[121] = "text/";            /* Directory for text files */

int keep_all_logs = 0;                  /* Never erase logfiles?    */
int switch_logfiles_at = 300;           /* When to switch logfiles  */

time_t online_since;    /* time that the bot was started */

int make_userfile = 0; /* Using bot in userfile-creation mode? */

int save_users_at = 0;   /* Minutes past the hour to save the userfile?     */
int notify_users_at = 0; /* Minutes past the hour to notify users of notes? */

char version[81];    /* Version info (long form)  */
char ver[41];        /* Version info (short form) */

volatile sig_atomic_t do_restart = 0; /* .restart has been called, restart ASAP */
int resolve_timeout = RES_TIMEOUT;    /* Hostname/address lookup timeout        */
char quit_msg[1024];                  /* Quit message                           */

/* Traffic stats */
unsigned long otraffic_irc = 0;
unsigned long otraffic_irc_today = 0;
unsigned long otraffic_bn = 0;
unsigned long otraffic_bn_today = 0;
unsigned long otraffic_dcc = 0;
unsigned long otraffic_dcc_today = 0;
unsigned long otraffic_filesys = 0;
unsigned long otraffic_filesys_today = 0;
unsigned long otraffic_trans = 0;
unsigned long otraffic_trans_today = 0;
unsigned long otraffic_unknown = 0;
unsigned long otraffic_unknown_today = 0;
unsigned long itraffic_irc = 0;
unsigned long itraffic_irc_today = 0;
unsigned long itraffic_bn = 0;
unsigned long itraffic_bn_today = 0;
unsigned long itraffic_dcc = 0;
unsigned long itraffic_dcc_today = 0;
unsigned long itraffic_trans = 0;
unsigned long itraffic_trans_today = 0;
unsigned long itraffic_unknown = 0;
unsigned long itraffic_unknown_today = 0;

#ifdef DEBUG_CONTEXT
/* Context storage for fatal crashes */
char cx_file[16][30];
char cx_note[16][256];
int cx_line[16];
int cx_ptr = 0;
#endif

#ifdef TLS
int ssl_cleanup();
#endif

void fatal(const char *s, int recoverable)
{
  int i;

  putlog(LOG_MISC, "*", "* %s", s);
  flushlogs();
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].sock >= 0)
      killsock(dcc[i].sock);
#ifdef TLS
  ssl_cleanup();
#endif
  unlink(pid_file);
  if (recoverable != 1) {
    bg_send_quit(BG_ABORT);
    exit(!recoverable);
  }
}

int expmem_chanprog();
int expmem_users();
int expmem_misc();
int expmem_dccutil();
int expmem_botnet();
int expmem_tcl();
int expmem_tclhash();
int expmem_net();
int expmem_modules(int);
int expmem_language();
int expmem_tcldcc();
int expmem_tclmisc();
int expmem_dns();
#ifdef TLS
int expmem_tls();
#endif

/* For mem.c : calculate memory we SHOULD be using
 */
int expected_memory(void)
{
  int tot;

  tot = expmem_chanprog() + expmem_users() + expmem_misc() + expmem_dccutil() +
        expmem_botnet() + expmem_tcl() + expmem_tclhash() + expmem_net() +
        expmem_modules(0) + expmem_language() + expmem_tcldcc() +
        expmem_tclmisc() + expmem_dns();
#ifdef TLS
  tot += expmem_tls();
#endif
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
      /* Only timeout 1 socket per cycle, too risky for more */
      return;
    }
}

#ifndef DEBUG_CONTEXT
#define write_debug() do {} while (0)
#else
static int nested_debug = 0;

static void write_debug()
{
  int x;
  char s[25];
  int y;

  if (nested_debug) {
    /* Yoicks, if we have this there's serious trouble!
     * All of these are pretty reliable, so we'll try these.
     *
     * NOTE: don't try and display context-notes in here, it's
     *       _not_ safe <cybah>
     */
    x = creat("DEBUG.DEBUG", 0644);
    if (x >= 0) {
      setsock(x, SOCK_NONSOCK);
      strlcpy(s, ctime(&now), sizeof s);
      dprintf(-x, "Debug (%s) written %s\n", ver, s);
      dprintf(-x, "Please report problem to bugs@eggheads.org\n");
      dprintf(-x, "after a visit to http://www.eggheads.org/bugzilla/\n");
#ifdef EGG_PATCH
      dprintf(-x, "Patch level: %s\n", egg_patch);
#else
      dprintf(-x, "Patch level: %s\n", "stable");
#endif
      dprintf(-x, "Context: ");
      cx_ptr = cx_ptr & 15;
      for (y = ((cx_ptr + 1) & 15); y != cx_ptr; y = ((y + 1) & 15))
        dprintf(-x, "%s/%d,\n         ", cx_file[y], cx_line[y]);
      dprintf(-x, "%s/%d\n\n", cx_file[y], cx_line[y]);
      killsock(x);
      close(x);
    }
    bg_send_quit(BG_ABORT);
    exit(1);                    /* Dont even try & tell people about, that may
                                 * have caused the fault last time. */
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
    strlcpy(s, ctime(&now), sizeof s);
    dprintf(-x, "Debug (%s) written %s\n", ver, s);
#ifdef EGG_PATCH
    dprintf(-x, "Patch level: %s\n", egg_patch);
#else
    dprintf(-x, "Patch level: %s\n", "stable");
#endif
#ifdef STATIC
    dprintf(-x, "STATICALLY LINKED\n");
#endif

    /* info library */
    dprintf(-x, "Tcl library: %s\n",
            ((interp) && (Tcl_Eval(interp, "info library") == TCL_OK)) ?
            tcl_resultstring() : "*unknown*");

    /* info tclversion/patchlevel */
    dprintf(-x, "Tcl version: %s (header version %s)\n",
            ((interp) && (Tcl_Eval(interp, "info patchlevel") == TCL_OK)) ?
            tcl_resultstring() : (Tcl_Eval(interp, "info tclversion") == TCL_OK) ?
            tcl_resultstring() : "*unknown*", TCL_PATCH_LEVEL);

    if (tcl_threaded())
      dprintf(-x, "Tcl is threaded\n");
#ifdef IPV6
    dprintf(-x, "Compiled with IPv6 support\n");
#else
    dprintf(-x, "Compiled without IPv6 support\n");
#endif

#ifdef TLS
    dprintf(-x, "Compiled with TLS support\n");
#else
    dprintf(-x, "Compiled without TLS support\n");
#endif

    if (!strcmp(EGG_AC_ARGS, "")) {
      dprintf(-x, "Configure flags: none\n");
    } else {
      dprintf(-x, "Configure flags: %s\n", EGG_AC_ARGS);
    }
#ifdef CCFLAGS
    dprintf(-x, "Compile flags: %s\n", CCFLAGS);
#endif

#ifdef LDFLAGS
    dprintf(-x, "Link flags: %s\n", LDFLAGS);
#endif

#ifdef STRIPFLAGS
    dprintf(-x, "Strip flags: %s\n", STRIPFLAGS);
#endif

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
#endif /* DEBUG_CONTEXT */

static void got_bus(int z)
{
  write_debug();
  fatal("BUS ERROR -- CRASHING!", 1);
#ifdef SA_RESETHAND
  kill(getpid(), SIGBUS);
#else
  bg_send_quit(BG_ABORT);
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
  bg_send_quit(BG_ABORT);
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
  /* Now we die by default on sigterm, but scripts have the chance to
   * catch the event themselves and cancel shutdown by returning 1
   */
  if (check_tcl_signal("sigterm"))
    return;
  kill_bot("ACK, I've been terminated!", "TERMINATE SIGNAL -- SIGNING OFF");
}

static void got_quit(int z)
{
  if (check_tcl_signal("sigquit"))
    return;
  putlog(LOG_MISC, "*", "Received QUIT signal: restarting...");
  do_restart = -1;
  return;
}

static void got_hup(int z)
{
  write_userfile(-1);
  if (check_tcl_signal("sighup"))
    return;
  putlog(LOG_MISC, "*", "Received HUP signal: rehashing...");
  do_restart = -2;
  return;
}

/* A call to resolver (gethostbyname, etc) timed out
 */
static void got_alarm(int z)
{
  siglongjmp(alarmret, 1);

  /* -Never reached- */
}

/* Got ILL signal -- log context and continue
 */
static void got_ill(int z)
{
  check_tcl_signal("sigill");
#ifdef DEBUG_CONTEXT
  putlog(LOG_MISC, "*", "* Context: %s/%d [%s]", cx_file[cx_ptr],
         cx_line[cx_ptr], (cx_note[cx_ptr][0]) ? cx_note[cx_ptr] : "");
#endif
}

#ifdef DEBUG_CONTEXT
/* Called from the Context macro.
 */
void eggContext(const char *file, int line, const char *module)
{
  char x[31], *p;

  p = strrchr(file, '/');
  if (!module) {
    strlcpy(x, p ? p + 1 : file, sizeof x);
  } else
    egg_snprintf(x, 31, "%s:%s", module, p ? p + 1 : file);
  cx_ptr = ((cx_ptr + 1) & 15);
  strcpy(cx_file[cx_ptr], x);
  cx_line[cx_ptr] = line;
  cx_note[cx_ptr][0] = 0;
}

/* Called from the ContextNote macro.
 */
void eggContextNote(const char *file, int line, const char *module,
                    const char *note)
{
  char x[31], *p;

  p = strrchr(file, '/');
  if (!module)
    strlcpy(x, p ? p + 1 : file, sizeof x);
  else
    egg_snprintf(x, 31, "%s:%s", module, p ? p + 1 : file);
  cx_ptr = ((cx_ptr + 1) & 15);
  strcpy(cx_file[cx_ptr], x);
  cx_line[cx_ptr] = line;
  strlcpy(cx_note[cx_ptr], note, sizeof cx_note[cx_ptr]);
}
#endif /* DEBUG_CONTEXT */

#ifdef DEBUG_ASSERT
/* Called from the Assert macro.
 */
void eggAssert(const char *file, int line, const char *module)
{
  write_debug();
  if (!module)
    putlog(LOG_MISC, "*", "* In file %s, line %u", file, line);
  else
    putlog(LOG_MISC, "*", "* In file %s:%s, line %u", module, file, line);
  fatal("ASSERT FAILED -- CRASHING!", 1);
}
#endif

static void show_ver() {
  char x[512], *z = x;

  strlcpy(x, egg_version, sizeof x);
  newsplit(&z);
  newsplit(&z);
  printf("%s\n", version);
  if (z[0]) {
    printf("  (patches: %s)\n", z);
  }
  if (!strcmp(EGG_AC_ARGS, "")) {
    printf("Configure flags: none\n");
  } else {
    printf("Configure flags: %s\n", EGG_AC_ARGS);
  }
  printf("Compiled with: ");
#ifdef IPV6
  printf("IPv6, ");
#endif
#ifdef TLS
  printf("TLS, ");
#endif
  printf("handlen=%d\n", HANDLEN);
  bg_send_quit(BG_ABORT);
}

/* Hard coded text because config file isn't loaded yet,
   meaning other languages can't be loaded yet.
   English (or an error) is the only possible option.
*/
static void show_help() {
  printf("\n%s\n\n", version);
  printf("Usage: %s [options] [config-file]\n\n"
         "Options:\n"
         "-n  Don't background; send all log entries to console.\n"
         "-nc Don't background; display channel stats every 10 seconds.\n"
         "-nt Don't background; use terminal to simulate DCC chat.\n"
         "-m  Create userfile.\n"
         "-h  Show this help and exit.\n"
         "-v  Show version info and exit.\n\n", argv[0]);
  bg_send_quit(BG_ABORT);
}

static void do_arg()
{
  int option = 0;
  unsigned char cliflags = 0;
  #define CLI_V        1 << 0
  #define CLI_M        1 << 1
  #define CLI_T        1 << 2
  #define CLI_C        1 << 3
  #define CLI_N        1 << 4
  #define CLI_H        1 << 5
  #define CLI_BAD_FLAG 1 << 6

  while ((option = getopt(argc, argv, "hnctmv")) != -1) {
    switch (option) {
      case 'n':
        cliflags |= CLI_N;
        backgrd = 0;
        break;
      case 'c':
        cliflags |= CLI_C;
        con_chan = 1;
        term_z = -1;
        break;
      case 't':
        cliflags |= CLI_T;
        con_chan = 0;
        term_z = 0;
        break;
      case 'm':
        cliflags |= CLI_M;
        make_userfile = 1;
        break;
      case 'v':
        cliflags |= CLI_V;
        break;
      case 'h':
        cliflags |= CLI_H;
        break;
      default:
        cliflags |= CLI_BAD_FLAG;
        break;
    }
  }
  if (cliflags & CLI_H) {
    show_help();
    exit(0);
  } else if (cliflags & CLI_BAD_FLAG) {
    show_help();
    exit(1);
  } else if (cliflags & CLI_V) {
    show_ver();
    exit(0);
  } else if (!(cliflags & CLI_N) && ((cliflags & CLI_C) || (cliflags & CLI_T))) {
    printf("\n%s\n", version);
    printf("ERROR: The -n flag is required when using the -c or -t flags. Exiting...\n\n");
    exit(1);
  } else if (argc > (optind + 1)) {
    printf("\n");
    printf("WARNING: More than one config file value detected\n");
    printf("         Using %s as config file\n", argv[optind]);
  }
  if (argc > optind) {
    strlcpy(configfile, argv[optind], sizeof configfile);
  }
}

void backup_userfile(void)
{
  char s[sizeof userfile + 4];

  if (quiet_save < 2)
    putlog(LOG_MISC, "*", USERF_BACKUP);
  egg_snprintf(s, sizeof s, "%s~bak", userfile);
  copyfile(userfile, s);
}

/* Timer info */
static time_t lastmin;
static time_t then;
static struct tm nowtm;

/* Called once a second.
 *
 * Note:  Try to not put any Context lines in here (guppy 21Mar2000).
 */
static void core_secondly()
{
  static int cnt = 0;
  int miltime;
  time_t nowmins;
  int i;

  do_check_timers(&utimer);     /* Secondly timers */
  cnt++;
  if (cnt >= 10) {              /* Every 10 seconds */
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
  nowmins = time(NULL) / 60;
  if (nowmins > lastmin) {
    memcpy(&nowtm, localtime(&now), sizeof(struct tm));
    i = 0;

    /* Once a minute */
    ++lastmin;
    call_hook(HOOK_MINUTELY);
    check_expired_ignores();
    autolink_cycle(NULL);       /* Attempt autolinks */
    /* In case for some reason more than 1 min has passed: */
    while (nowmins != lastmin) {
      /* Timer drift, dammit */
      debug2("timer: drift (lastmin=%d, nowmins=%d)", lastmin, nowmins);
      i++;
      ++lastmin;
      call_hook(HOOK_MINUTELY);
    }
    if (i > 1)
      putlog(LOG_MISC, "*", "(!) timer drift -- spun %d minutes", i);
    miltime = (nowtm.tm_hour * 100) + (nowtm.tm_min);
    if (((int) (nowtm.tm_min / 5) * 5) == (nowtm.tm_min)) {     /* 5 min */
      call_hook(HOOK_5MINUTELY);
      check_botnet_pings();
      if (!quick_logs) {
        flushlogs();
        check_logsize();
      }
      if (!miltime) {           /* At midnight */
        char s[25];
        int j;

        strlcpy(s, ctime(&now), sizeof s);
        if (quiet_save < 3)
          putlog(LOG_ALL, "*", "--- %.11s%s", s, s + 20);
        call_hook(HOOK_BACKUP);
        for (j = 0; j < max_logs; j++) {
          if (logs[j].filename != NULL && logs[j].f != NULL) {
            fclose(logs[j].f);
            logs[j].f = NULL;
          }
        }
      }
    }
    if (nowtm.tm_min == notify_users_at)
      call_hook(HOOK_HOURLY);
    /* These no longer need checking since they are all check vs minutely
     * settings and we only get this far on the minute.
     */
    if (miltime == switch_logfiles_at) {
      call_hook(HOOK_DAILY);
      if (!keep_all_logs) {
        if (quiet_save < 3)
          putlog(LOG_MISC, "*", MISC_LOGSWITCH);
        for (i = 0; i < max_logs; i++)
          if (logs[i].filename) {
            char s[1024];

            if (logs[i].f) {
              fclose(logs[i].f);
              logs[i].f = NULL;
            }
            egg_snprintf(s, sizeof s, "%s.yesterday", logs[i].filename);
            unlink(s);
            movefile(logs[i].filename, s);
          }
      }
    }
  }
}

static void core_minutely()
{
  check_tcl_time_and_cron(&nowtm);
  do_check_timers(&timer);
  if (quick_logs != 0) {
    flushlogs();
    check_logsize();
  }
}

static void core_hourly()
{
  write_userfile(-1);
}

static void event_rehash()
{
  check_tcl_event("rehash");
}

static void event_prerehash()
{
  check_tcl_event("prerehash");
}

static void event_save()
{
  check_tcl_event("save");
}

static void event_logfile()
{
  check_tcl_event("logfile");
}

static void event_resettraffic()
{
  otraffic_irc += otraffic_irc_today;
  itraffic_irc += itraffic_irc_today;
  otraffic_bn += otraffic_bn_today;
  itraffic_bn += itraffic_bn_today;
  otraffic_dcc += otraffic_dcc_today;
  itraffic_dcc += itraffic_dcc_today;
  otraffic_unknown += otraffic_unknown_today;
  itraffic_unknown += itraffic_unknown_today;
  otraffic_trans += otraffic_trans_today;
  itraffic_trans += itraffic_trans_today;
  otraffic_irc_today = otraffic_bn_today = 0;
  otraffic_dcc_today = otraffic_unknown_today = 0;
  itraffic_irc_today = itraffic_bn_today = 0;
  itraffic_dcc_today = itraffic_unknown_today = 0;
  itraffic_trans_today = otraffic_trans_today = 0;
}

static void event_loaded()
{
  check_tcl_event("loaded");
}

void kill_tcl();
extern module_entry *module_list;
void restart_chons();

#ifdef STATIC
void check_static(char *, char *(*)());

#include "mod/static.h"
#endif
int init_threaddata(int);
int init_mem();
int init_userent();
int init_misc();
int init_bots();
int init_modules();
int init_tcl(int, char **);
int init_language(int);
#ifdef TLS
int ssl_init();
#endif

static void garbage_collect(void)
{
  static uint8_t run_cnt = 0;

  if (run_cnt == 3)
    garbage_collect_tclhash();
  else
    run_cnt++;
}

int mainloop(int toplevel)
{
  static int socket_cleanup = 0;
  int xx, i, eggbusy = 1, tclbusy = 0;
  char buf[520];

  /* Lets move some of this here, reducing the number of actual
   * calls to periodic_timers
   */
  now = time(NULL);

  /* If we want to restart, we have to unwind to the toplevel.
   * Tcl will Panic if we kill the interp with Tcl_Eval in progress.
   * This is done by returning -1 in tickle_WaitForEvent.
   */
  if (do_restart && do_restart != -2 && !toplevel)
    return -1;

  /* Once a second */
  if (now != then) {
    call_hook(HOOK_SECONDLY);
    then = now;
  }

  /* Only do this every so often. */
  if (!socket_cleanup) {
    socket_cleanup = 5;

    /* Remove dead dcc entries. */
    dcc_remove_lost();

    /* Check for server or dcc activity. */
    dequeue_sockets();
  } else
    socket_cleanup--;

  /* Free unused structures. */
  garbage_collect();

  xx = sockgets(buf, &i);
  if (xx >= 0) {              /* Non-error */
    int idx;

    for (idx = 0; idx < dcc_total; idx++)
      if (dcc[idx].sock == xx) {
        if (dcc[idx].type && dcc[idx].type->activity) {
          /* Traffic stats */
          if (dcc[idx].type->name) {
            if (!strncmp(dcc[idx].type->name, "BOT", 3))
              itraffic_bn_today += strlen(buf) + 1;
            else if (!strcmp(dcc[idx].type->name, "SERVER"))
              itraffic_irc_today += strlen(buf) + 1;
            else if (!strncmp(dcc[idx].type->name, "CHAT", 4))
              itraffic_dcc_today += strlen(buf) + 1;
            else if (!strncmp(dcc[idx].type->name, "FILES", 5))
              itraffic_dcc_today += strlen(buf) + 1;
            else if (!strcmp(dcc[idx].type->name, "SEND"))
              itraffic_trans_today += strlen(buf) + 1;
            else if (!strcmp(dcc[idx].type->name, "FORK_SEND"))
              itraffic_trans_today += strlen(buf) + 1;
            else if (!strncmp(dcc[idx].type->name, "GET", 3))
              itraffic_trans_today += strlen(buf) + 1;
            else
              itraffic_unknown_today += strlen(buf) + 1;
          }
          dcc[idx].type->activity(idx, buf, i);
        } else
          putlog(LOG_MISC, "*",
                 "!!! untrapped dcc activity: type %s, sock %d",
                 dcc[idx].type->name, dcc[idx].sock);
        break;
      }
  } else if (xx == -1) {        /* EOF from someone */
    int idx;

    if (i == STDOUT && !backgrd)
      fatal("END OF FILE ON TERMINAL", 0);
    for (idx = 0; idx < dcc_total; idx++)
      if (dcc[idx].sock == i) {
        if (dcc[idx].type && dcc[idx].type->eof)
          dcc[idx].type->eof(idx);
        else {
          putlog(LOG_MISC, "*",
                 "*** ATTENTION: DEAD SOCKET (%d) OF TYPE %s UNTRAPPED",
                 i, dcc[idx].type ? dcc[idx].type->name : "*UNKNOWN*");
          killsock(i);
          lostdcc(idx);
        }
        idx = dcc_total + 1;
      }
    if (idx == dcc_total) {
      putlog(LOG_MISC, "*",
             "(@) EOF socket %d, not a dcc socket, not anything.", i);
      close(i);
      killsock(i);
    }
  } else if (xx == -2 && errno != EINTR) {      /* select() error */
    putlog(LOG_MISC, "*", "* Socket error #%d; recovering.", errno);
    for (i = 0; i < dcc_total; i++) {
      if ((fcntl(dcc[i].sock, F_GETFD, 0) == -1) && (errno == EBADF)) {
        putlog(LOG_MISC, "*",
               "DCC socket %d (type %d, name '%s') expired -- pfft",
               dcc[i].sock, dcc[i].type, dcc[i].nick);
        killsock(dcc[i].sock);
        lostdcc(i);
        i--;
      }
    }
  } else if (xx == -3) {
    call_hook(HOOK_IDLE);
    socket_cleanup = 0;       /* If we've been idle, cleanup & flush */
    eggbusy = 0;
  } else if (xx == -5) {
    eggbusy = 0;
    tclbusy = 1;
  }

  if (do_restart) {
    if (do_restart == -2)
      rehash();
    else if (!toplevel)
      return -1; /* Unwind to toplevel before restarting */
    else {
      /* Unload as many modules as possible */
      int f = 1;
      module_entry *p;
      Function startfunc;
      char name[256];

      /* oops, I guess we should call this event before tcl is restarted */
      check_tcl_event("prerestart");

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
            strlcpy(name, p->name, sizeof name);
            if (module_unload(name, botnetnick) == NULL) {
              f = 1;
              break;
            }
          }
        }
      }

      /* Make sure we don't have any modules left hanging around other than
       * "eggdrop" and the two that are supposed to be.
       */
      for (f = 0, p = module_list; p; p = p->next) {
        if (strcmp(p->name, "eggdrop") && strcmp(p->name, "encryption") &&
            strcmp(p->name, "uptime")) {
          f++;
        }
      }
      if (f != 0) {
        putlog(LOG_MISC, "*", MOD_STAGNANT);
      }

      flushlogs();
      kill_tcl();
      init_tcl(argc, argv);
      init_language(0);

      /* this resets our modules which we didn't unload (encryption and uptime) */
      for (p = module_list; p; p = p->next) {
        if (p->funcs) {
          startfunc = p->funcs[MODCALL_START];
          startfunc(NULL);
        }
      }

      rehash();
#ifdef TLS
      ssl_cleanup();
      ssl_init();
#endif
      restart_chons();
      call_hook(HOOK_LOADED);
    }
    eggbusy = 1;
    do_restart = 0;
  }

  if (!eggbusy) {
/* Process all pending tcl events */
#  ifdef REPLACE_NOTIFIER
    if (Tcl_ServiceAll())
      tclbusy = 1;
#  else
    while (Tcl_DoOneEvent(TCL_DONT_WAIT | TCL_ALL_EVENTS))
      tclbusy = 1;
#  endif /* REPLACE_NOTIFIER */
  }

  return (eggbusy || tclbusy);
}

static void init_random(void) {
  unsigned int seed;
#ifdef HAVE_GETRANDOM
  if (getrandom(&seed, sizeof(seed), 0) != sizeof(seed)) {
    if (errno != ENOSYS) {
      fatal("ERROR: getrandom()\n", 0);
    } else {
      /* getrandom() is available in header but syscall is not!
       * This can happen with glibc>=2.25 and linux<3.17
       */
#endif
      struct timeval tp;
      gettimeofday(&tp, NULL);
      seed = (tp.tv_sec * tp.tv_usec) ^ getpid();
#ifdef HAVE_GETRANDOM
    }
  }
#endif
  srandom(seed);
}

int main(int arg_c, char **arg_v)
{
  int i, xx;
  char s[25];
  FILE *f;
  struct sigaction sv;
  struct chanset_t *chan;
#ifdef DEBUG
  struct rlimit cdlim;
#endif
#ifdef STOP_UAC
  int nvpair[2];
#endif

/* Make sure it can write core, if you make debug. Else it's pretty
 * useless (dw)
 *
 * Only allow unlimited size core files when compiled with DEBUG defined.
 * This is not a good idea for normal builds -- in these cases, use the
 * default system resource limits instead.
 */
#ifdef DEBUG
  cdlim.rlim_cur = RLIM_INFINITY;
  cdlim.rlim_max = RLIM_INFINITY;
  setrlimit(RLIMIT_CORE, &cdlim);
#endif

#ifdef DEBUG_CONTEXT
  /* Initialise context list */
  for (i = 0; i < 16; i++)
    Context;
#endif

  argc = arg_c;
  argv = arg_v;

  /* Version info! */
#ifdef EGG_PATCH
  egg_snprintf(&egg_version[strlen(egg_version)], sizeof egg_version, 
               "+%s", egg_patch);
#endif
  egg_snprintf(ver, sizeof ver, "eggdrop v%s", egg_version);
  egg_snprintf(version, sizeof version,
               "Eggdrop v%s (C) 1997 Robey Pointer (C) 2010-2019 Eggheads",
               egg_version);
  /* Now add on the patchlevel (for Tcl) */
  sprintf(&egg_version[strlen(egg_version)], " %u", egg_numver);

/* For OSF/1 */
#ifdef STOP_UAC
  /* Don't print "unaligned access fixup" warning to the user */
  nvpair[0] = SSIN_UACPROC;
  nvpair[1] = UAC_NOPRINT;
  setsysinfo(SSI_NVPAIRS, (char *) nvpair, 1, NULL, 0);
#endif

  /* Set up error traps: */
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

  /* Initialize variables and stuff */
  now = time(NULL);
  chanset = NULL;
  lastmin = now / 60;
  init_random();
  init_mem();
  if (argc > 1)
    do_arg();
  init_language(1);

  printf("\n%s\n", version);

#ifndef CYGWIN_HACKS
  /* Don't allow eggdrop to run as root
   * This check isn't useful under cygwin and has been
   * reported to cause trouble in some situations.
   */
  if (((int) getuid() == 0) || ((int) geteuid() == 0))
    fatal("ERROR: Eggdrop will not run as root!", 0);
#endif

#ifndef REPLACE_NOTIFIER
  init_threaddata(1);
#endif
  init_userent();
  init_misc();
  init_bots();
  init_modules();
  if (backgrd)
    bg_prepare_split();
  init_tcl(argc, argv);
  init_language(0);
#ifdef STATIC
  link_statics();
#endif
  strlcpy(s, ctime(&now), sizeof s);
  memmove(&s[11], &s[20], strlen(&s[20])+1);
  putlog(LOG_ALL, "*", "--- Loading %s (%s)", ver, s);
  chanprog();
  if (!encrypt_pass) {
    printf("%s", MOD_NOCRYPT);
    bg_send_quit(BG_ABORT);
    exit(1);
  }
  i = 0;
  for (chan = chanset; chan; chan = chan->next)
    i++;
  putlog(LOG_MISC, "*", "=== %s: %d channels, %d users.",
         botnetnick, i, count_users(userlist));
#ifdef TLS
  ssl_init();
#endif
  cache_miss = 0;
  cache_hit = 0;
  if (!pid_file[0])
    egg_snprintf(pid_file, sizeof pid_file, "pid.%s", botnetnick);

  /* Check for pre-existing eggdrop! */
  f = fopen(pid_file, "r");
  if (f != NULL) {
    if (fgets(s, 10, f) != NULL) {
      xx = atoi(s);
      i = kill(xx, SIGCHLD);      /* Meaningless kill to determine if pid
                                   * is used */
      if (i == 0 || errno != ESRCH) {
        printf(EGG_RUNNING1, botnetnick);
        printf(EGG_RUNNING2, pid_file);
        bg_send_quit(BG_ABORT);
        exit(1);
      }
    } else {
      printf("Error checking for existing Eggdrop process.\n");
    }
    fclose(f);
  }

  /* Move into background? */
  if (backgrd) {
    bg_do_split();
  } else {                        /* !backgrd */
    xx = getpid();
    if (xx != 0) {
      FILE *fp;

      /* Write pid to file */
      unlink(pid_file);
      fp = fopen(pid_file, "w");
      if (fp != NULL) {
        fprintf(fp, "%u\n", xx);
        if (fflush(fp)) {
          /* Let the bot live since this doesn't appear to be a botchk */
          printf(EGG_NOWRITE, pid_file);
          fclose(fp);
          unlink(pid_file);
        } else
          fclose(fp);
      } else
        printf(EGG_NOWRITE, pid_file);
    }
  }

  use_stderr = 0;               /* Stop writing to stderr now */
  if (backgrd) {
    /* Ok, try to disassociate from controlling terminal (finger cross) */
#ifdef HAVE_SETPGID
    setpgid(0, 0);
#endif
    /* Tcl wants the stdin, stdout and stderr file handles kept open. */
    if (freopen("/dev/null", "r", stdin) == NULL) {
      putlog(LOG_MISC, "*", "Error renaming stdin file handle: %s", strerror(errno));
    }
    if (freopen("/dev/null", "w", stdout) == NULL) {
      putlog(LOG_MISC, "*", "Error renaming stdout file handle: %s", strerror(errno));
    }
    if (freopen("/dev/null", "w", stderr) == NULL) {
      putlog(LOG_MISC, "*", "Error renaming stderr file handle: %s", strerror(errno));
    }
#ifdef CYGWIN_HACKS
    FreeConsole();
#endif
  }

  /* Terminal emulating dcc chat */
  if (!backgrd && term_z >= 0) {
    /* reuse term_z as glob var to pass it's index in the dcc table around */
    term_z = new_dcc(&DCC_CHAT, sizeof(struct chat_info));

    /* new_dcc returns -1 on error */
    if (term_z < 0)
      fatal("ERROR: Failed to initialize foreground chat.", 0);

    getvhost(&dcc[term_z].sockname, AF_INET);
    dcc[term_z].sock = STDOUT;
    dcc[term_z].timeval = now;
    dcc[term_z].u.chat->con_flags = conmask | EGG_BG_CONMASK;
    dcc[term_z].u.chat->strip_flags = STRIP_ALL;
    dcc[term_z].status = STAT_ECHO;
    strcpy(dcc[term_z].nick, EGG_BG_HANDLE);
    strcpy(dcc[term_z].host, "llama@console");
    add_hq_user();
    setsock(STDOUT, 0);          /* Entry in net table */
    dprintf(term_z, "\n### ENTERING DCC CHAT SIMULATION ###\n");
    dprintf(term_z, "You can use the .su command to log into your Eggdrop account.\n\n");
    dcc_chatter(term_z);
  }

  then = now;
  online_since = now;
  autolink_cycle(NULL);         /* Hurry and connect to tandem bots */
  add_help_reference("cmds1.help");
  add_help_reference("cmds2.help");
  add_help_reference("core.help");
  add_hook(HOOK_SECONDLY, (Function) core_secondly);
  add_hook(HOOK_MINUTELY, (Function) core_minutely);
  add_hook(HOOK_HOURLY, (Function) core_hourly);
  add_hook(HOOK_REHASH, (Function) event_rehash);
  add_hook(HOOK_PRE_REHASH, (Function) event_prerehash);
  add_hook(HOOK_USERFILE, (Function) event_save);
  add_hook(HOOK_BACKUP, (Function) backup_userfile);
  add_hook(HOOK_DAILY, (Function) event_logfile);
  add_hook(HOOK_DAILY, (Function) event_resettraffic);
  add_hook(HOOK_LOADED, (Function) event_loaded);

  call_hook(HOOK_LOADED);

  debug0("main: entering loop");
  while (1) {
    mainloop(1);
  }
}
