/*
 * tclmisc.c -- handles:
 * Tcl stubs for file system commands
 * Tcl stubs for everything else
 * 
 * dprintf'ized, 1aug1996
 */
/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

#include "main.h"
#include <sys/stat.h>
#include "modules.h"
#include "tandem.h"
#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif

extern tcl_timer_t *timer, *utimer;
extern struct dcc_t *dcc;
extern char origbotname[];
extern struct userrec *userlist;
extern time_t now;
extern module_entry *module_list;
extern char botnetnick[];

/***********************************************************************/

static int tcl_putlog STDVAR
{
  char logtext[501];

  context;
  BADARGS(2, 2, " text");
  strncpy(logtext, argv[1], 500);
  logtext[500] = 0;
  putlog(LOG_MISC, "*", "%s", logtext);
  return TCL_OK;
}

static int tcl_putcmdlog STDVAR
{
  char logtext[501];

  context;
  BADARGS(2, 2, " text");
  strncpy(logtext, argv[1], 500);
  logtext[500] = 0;
  putlog(LOG_CMDS, "*", "%s", logtext);
  return TCL_OK;
}

static int tcl_putxferlog STDVAR
{
  char logtext[501];

  context;
  BADARGS(2, 2, " text");
  strncpy(logtext, argv[1], 500);
  logtext[500] = 0;
  putlog(LOG_FILES, "*", "%s", logtext);
  return TCL_OK;
}

static int tcl_putloglev STDVAR
{
  int lev = 0;
  char logtext[501];

  context;
  BADARGS(4, 4, " level channel text");
  lev = logmodes(argv[1]);
  if (lev == 0) {
    Tcl_AppendResult(irp, "No valid log-level given", NULL);
    return TCL_ERROR;
  }
  strncpy(logtext, argv[3], 500);
  logtext[500] = 0;
  putlog(lev, argv[2], "%s", logtext);
  return TCL_OK;
}

static int tcl_timer STDVAR
{
  unsigned long x;
  char s[41];

  context;
  BADARGS(3, 3, " minutes command");
  if (atoi(argv[1]) < 0) {
    Tcl_AppendResult(irp, "time value must be positive", NULL);
    return TCL_ERROR;
  }
  if (argv[2][0] != '#') {
    x = add_timer(&timer, atoi(argv[1]), argv[2], 0L);
    sprintf(s, "timer%lu", x);
    Tcl_AppendResult(irp, s, NULL);
  }
  return TCL_OK;
}

static int tcl_utimer STDVAR
{
  unsigned long x;
  char s[41];

  context;
  BADARGS(3, 3, " seconds command");
  if (atoi(argv[1]) < 0) {
    Tcl_AppendResult(irp, "time value must be positive", NULL);
    return TCL_ERROR;
  }
  if (argv[2][0] != '#') {
    x = add_timer(&utimer, atoi(argv[1]), argv[2], 0L);
    sprintf(s, "timer%lu", x);
    Tcl_AppendResult(irp, s, NULL);
  }
  return TCL_OK;
}

static int tcl_killtimer STDVAR
{
  context;
  BADARGS(2, 2, " timerID");
  if (strncmp(argv[1], "timer", 5) != 0) {
    Tcl_AppendResult(irp, "argument is not a timerID", NULL);
    return TCL_ERROR;
  }
  if (remove_timer(&timer, atol(&argv[1][5])))
     return TCL_OK;
  Tcl_AppendResult(irp, "invalid timerID", NULL);
  return TCL_ERROR;
}

static int tcl_killutimer STDVAR
{
  context;
  BADARGS(2, 2, " timerID");
  if (strncmp(argv[1], "timer", 5) != 0) {
    Tcl_AppendResult(irp, "argument is not a timerID", NULL);
    return TCL_ERROR;
  }
  if (remove_timer(&utimer, atol(&argv[1][5])))
     return TCL_OK;

  Tcl_AppendResult(irp, "invalid timerID", NULL);
  return TCL_ERROR;
}

static int tcl_duration STDVAR
{
  char s[256];
  time_t sec;
  BADARGS(2, 2, " seconds");

  if (atol(argv[1]) <= 0) {
    Tcl_AppendResult(irp, "0 seconds", NULL);
    return TCL_OK;
  }
  sec = atoi(argv[1]);
  s[0] = 0;

  if (sec >= 31536000) {
    sprintf(s, "%d year", (int) (sec / 31536000));
    if ((int) (sec / 31536000) > 1)
      strcat(s, "s");
    strcat(s, " ");
    sec -= (((int) (sec / 31536000)) * 31536000);
  }
  if (sec >= 604800) {
    sprintf(&s[strlen(s)], "%d week", (int) (sec / 604800));
    if ((int) (sec / 604800) > 1)
      strcat(s, "s");
    strcat(s, " ");
    sec -= (((int) (sec / 604800)) * 604800);
  }
  if (sec >= 86400) {
    sprintf(&s[strlen(s)], "%d day", (int) (sec / 86400));
    if ((int) (sec / 86400) > 1)
      strcat(s, "s");
    strcat(s, " ");
    sec -= (((int) (sec / 86400)) * 86400);
  }
  if (sec >= 3600) {
    sprintf(&s[strlen(s)], "%d hour", (int) (sec / 3600));
    if ((int) (sec / 3600) > 1)
      strcat(s, "s");
    strcat(s, " ");
    sec -= (((int) (sec / 3600)) * 3600);
  }
  if (sec >= 60) {
    sprintf(&s[strlen(s)], "%d minute", (int) (sec / 60));
    if ((int) (sec / 60) > 1)
      strcat(s, "s");
    strcat(s, " ");
    sec -= (((int) (sec / 60)) * 60);
  }
  if (sec > 0) {
    sprintf(&s[strlen(s)], "%d second", (int) (sec / 1));
    if ((int) (sec / 1) > 1)
      strcat(s, "s");
  }
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_unixtime STDVAR
{
  char s[20];

  context;
  BADARGS(1, 1, "");
  sprintf(s, "%lu", (unsigned long) now);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_time STDVAR
{
  char s[81];

  context;
  BADARGS(1, 1, "");
  strcpy(s, ctime(&now));
  strcpy(s, &s[11]);
  s[5] = 0;
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_date STDVAR
{
  char s[81];

  context;
  BADARGS(1, 1, "");
  strcpy(s, ctime(&now));
  s[10] = s[24] = 0;
  strcpy(s, &s[8]);
  strcpy(&s[8], &s[20]);
  strcpy(&s[2], &s[3]);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_timers STDVAR
{
  context;
  BADARGS(1, 1, "");
  list_timers(irp, timer);
  return TCL_OK;
}

static int tcl_utimers STDVAR
{
  context;
  BADARGS(1, 1, "");
  list_timers(irp, utimer);
  return TCL_OK;
}

static int tcl_ctime STDVAR
{
  time_t tt;
  char s[81];

  context;
  BADARGS(2, 2, " unixtime");
  tt = (time_t) atol(argv[1]);
  strcpy(s, ctime(&tt));
  s[strlen(s) - 1] = 0;
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_myip STDVAR
{
  char s[21];

  context;
  BADARGS(1, 1, "");
  sprintf(s, "%lu", iptolong(getmyip()));
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_rand STDVAR
{
  unsigned long x;
  char s[41];

  context;
  BADARGS(2, 2, " limit");
  if (atol(argv[1]) <= 0) {
    Tcl_AppendResult(irp, "random limit must be greater than zero", NULL);
    return TCL_ERROR;
  }
  x = random() % (atol(argv[1]));

  sprintf(s, "%lu", x);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_sendnote STDVAR {
  char s[5], from[NOTENAMELEN + 1], to[NOTENAMELEN + 1], msg[451];

  context;
  BADARGS(4, 4, " from to message");
  strncpy(from, argv[1], NOTENAMELEN);
  from[NOTENAMELEN] = 0;
  strncpy(to, argv[2], NOTENAMELEN);
  to[NOTENAMELEN] = 0;
  strncpy(msg, argv[3], 450);
  msg[450] = 0;
  sprintf(s, "%d", add_note(to, from, msg, -1, 0));
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_dumpfile STDVAR
{
  char nick[NICKLEN];
  struct flag_record fr = {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  context;
  BADARGS(3, 3, " nickname filename");
  strncpy(nick, argv[1], NICKLEN - 1);
  nick[NICKLEN - 1] = 0;
  get_user_flagrec(get_user_by_nick(nick), &fr, NULL);
  showhelp(argv[1], argv[2], &fr, HELP_TEXT);
  return TCL_OK;
}

static int tcl_dccdumpfile STDVAR
{
  int idx, i;
  struct flag_record fr = {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

  context;
  BADARGS(3, 3, " idx filename");
  i = atoi(argv[1]);
  idx = findidx(i);
  if (idx < 0) {
    Tcl_AppendResult(irp, "illegal idx", NULL);
    return TCL_ERROR;
  }
  get_user_flagrec(get_user_by_handle(userlist, dcc[idx].nick), &fr, NULL);
  tellhelp(idx, argv[2], &fr, HELP_TEXT);
  return TCL_OK;
}

static int tcl_backup STDVAR
{
  context;
  BADARGS(1, 1, "");
  backup_userfile();
  return TCL_OK;
}

static int tcl_die STDVAR
{
  char s[1024];
  char g[1024];

  context;
  BADARGS(1, 2, " ?reason?");
  if (argc == 2) {
    simple_sprintf(s, "BOT SHUTDOWN (%s)", argv[1]);
    simple_sprintf(g, "%s", argv[1]);
  } else {
    simple_sprintf(s, "BOT SHUTDOWN (authorized by a canadian)");
    simple_sprintf(g, "EXIT");
  }
  chatout("*** %s\n", s);
  botnet_send_chat(-1, botnetnick, s);
  botnet_send_bye();
  write_userfile(-1);
  fatal(g, 0);
  /* should never return, but, to keep gcc happy: */
  return TCL_OK;
}

static int tcl_strftime STDVAR
{
  char buf[512];
  struct tm *tm1;
  time_t t;

  context;
  BADARGS(2, 3, " format ?time?");
  if (argc == 3)
    t = atol(argv[2]);
  else
    t = now;
    tm1 = localtime(&t);
  if (strftime(buf, sizeof(buf) - 1, argv[1], tm1)) {
    Tcl_AppendResult(irp, buf, NULL);
    return TCL_OK;
  }
  Tcl_AppendResult(irp, " error with strftime", NULL);
  return TCL_ERROR;
}

static int tcl_loadmodule STDVAR
{
  const char *p;

  context;
  BADARGS(2, 2, " module-name");
  p = module_load(argv[1]);
  if (p && strcmp(p, MOD_ALREADYLOAD) && !strcmp(argv[0], "loadmodule"))
    putlog(LOG_MISC, "*", "%s %s: %s", MOD_CANTLOADMOD, argv[1], p);
  Tcl_AppendResult(irp, p, NULL);
  return TCL_OK;
}

static int tcl_unloadmodule STDVAR
{
  context;
  BADARGS(2, 2, " module-name");
  Tcl_AppendResult(irp, module_unload(argv[1], origbotname), NULL);
  return TCL_OK;
}

static int tcl_unames STDVAR
{
  char *unix_n, *vers_n;
#ifdef HAVE_UNAME
  struct utsname un;
  if (uname(&un) < 0) {
#endif
    unix_n = "*unkown*";
    vers_n = "";
#ifdef HAVE_UNAME
  } else {
    unix_n = un.sysname;
    vers_n = un.release;
  }
#endif
  Tcl_AppendResult(irp, unix_n, " ", vers_n, NULL);
  return TCL_OK;
}

static int tcl_modules STDVAR
{
  module_entry *current;
  dependancy *dep;
  char *list[100], *list2[2], *p;
  char s[40], s2[40];
  int i;

  context;
  BADARGS(1, 1, "");
  for (current = module_list; current; current = current->next) {
    list[0] = current->name;
    simple_sprintf(s, "%d.%d", current->major, current->minor);
    list[1] = s;
    i = 2;
    for (dep = dependancy_list; dep && (i < 100); dep = dep->next) {
      if (dep->needing == current) {
	list2[0] = dep->needed->name;
	simple_sprintf(s2, "%d.%d", dep->major, dep->minor);
	list2[1] = s2;
	list[i] = Tcl_Merge(2, list2);
	i++;
      }
    }
    p = Tcl_Merge(i, list);
    Tcl_AppendElement(irp, p);
    n_free(p, "", 0);
    while (i > 2) {
      i--;
      n_free(list[i], "", 0);
    }
  }
  return TCL_OK;
}

static int tcl_loadhelp STDVAR
{
  context;
  BADARGS(2, 2, " helpfile-name");
  add_help_reference(argv[1]);
  return TCL_OK;
}

static int tcl_unloadhelp STDVAR
{
  context;
  BADARGS(2, 2, " helpfile-name");
  rem_help_reference(argv[1]);
  return TCL_OK;
}

static int tcl_reloadhelp STDVAR
{
  context;
  BADARGS(1, 1, "");
  reload_help_data();
  return TCL_OK;
}

tcl_cmds tclmisc_cmds[] =
{
  {"putlog", tcl_putlog},
  {"putcmdlog", tcl_putcmdlog},
  {"putxferlog", tcl_putxferlog},
  {"putloglev", tcl_putloglev},
  {"timer", tcl_timer},
  {"utimer", tcl_utimer},
  {"killtimer", tcl_killtimer},
  {"killutimer", tcl_killutimer},
  {"unixtime", tcl_unixtime},
  {"time", tcl_time},
  {"date", tcl_date},
  {"timers", tcl_timers},
  {"utimers", tcl_utimers},
  {"ctime", tcl_ctime},
  {"myip", tcl_myip},
  {"rand", tcl_rand},
  {"sendnote", tcl_sendnote},
  {"dumpfile", tcl_dumpfile},
  {"dccdumpfile", tcl_dccdumpfile},
  {"backup", tcl_backup},
  {"exit", tcl_die},
  {"die", tcl_die},
  {"strftime", tcl_strftime},
  {"unames", tcl_unames},
  {"unloadmodule", tcl_unloadmodule},
  {"loadmodule", tcl_loadmodule},
  {"checkmodule", tcl_loadmodule},
  {"modules", tcl_modules},
  {"loadhelp", tcl_loadhelp},
  {"unloadhelp", tcl_unloadhelp},
  {"reloadhelp", tcl_reloadhelp},
  {"duration", tcl_duration},
  {0, 0}
};
