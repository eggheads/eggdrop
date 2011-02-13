/*
 * tclmisc.c -- handles:
 *   Tcl stubs for everything else
 *
 * $Id: tclmisc.c,v 1.71 2011/02/13 14:19:33 simple Exp $
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
#include "modules.h"
#include "tandem.h"
#include "md5/md5.h"

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

#include <sys/stat.h>

#ifdef HAVE_UNAME
#  include <sys/utsname.h>
#endif

extern p_tcl_bind_list bind_table_list;
extern tcl_timer_t *timer, *utimer;
extern struct dcc_t *dcc;
extern char botnetnick[], quit_msg[];
extern struct userrec *userlist;
extern time_t now;
extern module_entry *module_list;
extern int max_logs;
extern log_t *logs;
extern Tcl_Interp *interp;

int expmem_tclmisc()
{
  int i, tot = 0;

  for (i = 0; i < max_logs; i++) {
    if (logs[i].filename != NULL) {
      tot += strlen(logs[i].filename) + 1;
      tot += strlen(logs[i].chname) + 1;
    }
  }

  return tot;
}

/*
 *      Logging
 */

/* logfile [<modes> <channel> <filename>] */
static int tcl_logfile STDVAR
{
  int i;
  char s[151];

  BADARGS(1, 4, " ?logModes channel logFile?");

  if (argc == 1) {
    /* They just want a list of the logfiles and modes */
    for (i = 0; i < max_logs; i++)
      if (logs[i].filename != NULL) {
        snprintf(s, sizeof s, "%s %s %s", masktype(logs[i].mask),
                 logs[i].chname, logs[i].filename);
        Tcl_AppendElement(interp, s);
      }
      return TCL_OK;
  }

  BADARGS(4, 4, " ?logModes channel logFile?");

  if (*argv[1] && !*argv[2]) {
    Tcl_AppendResult(interp,
                     "log modes set, but no channel specified", NULL);
    return TCL_ERROR;
  }
  if (*argv[2] && !strchr(CHANMETA, *argv[2]) && strcmp(argv[2], "*")) {
    Tcl_AppendResult(interp, "invalid channel prefix", NULL);
    return TCL_ERROR;
  }
  if (*argv[2] && strchr(argv[2], ' ')) {
    Tcl_AppendResult(interp, "channel names cannot contain spaces", NULL);
    return TCL_ERROR;
  }

  for (i = 0; i < max_logs; i++)
    if ((logs[i].filename != NULL) && (!strcmp(logs[i].filename, argv[3]))) {
      logs[i].flags &= ~LF_EXPIRING;
      logs[i].mask = logmodes(argv[1]);
      nfree(logs[i].chname);
      logs[i].chname = NULL;
      if (!logs[i].mask) {
        /* ending logfile */
        nfree(logs[i].filename);
        logs[i].filename = NULL;
        if (logs[i].f != NULL) {
          fclose(logs[i].f);
          logs[i].f = NULL;
        }
        logs[i].flags = 0;
      } else {
        logs[i].chname = nmalloc(strlen(argv[2]) + 1);
        strcpy(logs[i].chname, argv[2]);
      }
      Tcl_AppendResult(interp, argv[3], NULL);
      return TCL_OK;
    }
  /* Do not add logfiles without any flags to log ++rtc */
  if (!logmodes(argv[1])) {
    Tcl_AppendResult(interp, "can't remove \"", argv[3],
                     "\" from list: no such logfile", NULL);
    return TCL_ERROR;
  }
  for (i = 0; i < max_logs; i++)
    if (logs[i].filename == NULL) {
      logs[i].flags = 0;
      logs[i].mask = logmodes(argv[1]);
      logs[i].filename = nmalloc(strlen(argv[3]) + 1);
      strcpy(logs[i].filename, argv[3]);
      logs[i].chname = nmalloc(strlen(argv[2]) + 1);
      strcpy(logs[i].chname, argv[2]);
      Tcl_AppendResult(interp, argv[3], NULL);
      return TCL_OK;
    }
  Tcl_AppendResult(interp, "reached max # of logfiles", NULL);
  return TCL_ERROR;
}

static int tcl_putlog STDVAR
{
  char logtext[501];

  BADARGS(2, 2, " text");

  strncpyz(logtext, argv[1], sizeof logtext);
  putlog(LOG_MISC, "*", "%s", logtext);
  return TCL_OK;
}

static int tcl_putcmdlog STDVAR
{
  char logtext[501];

  BADARGS(2, 2, " text");

  strncpyz(logtext, argv[1], sizeof logtext);
  putlog(LOG_CMDS, "*", "%s", logtext);
  return TCL_OK;
}

static int tcl_putxferlog STDVAR
{
  char logtext[501];

  BADARGS(2, 2, " text");

  strncpyz(logtext, argv[1], sizeof logtext);
  putlog(LOG_FILES, "*", "%s", logtext);
  return TCL_OK;
}

static int tcl_putloglev STDVAR
{
  int lev = 0;
  char logtext[501];

  BADARGS(4, 4, " level(s) channel text");

  lev = logmodes(argv[1]);
  if (!lev) {
    Tcl_AppendResult(irp, "No valid log-level given", NULL);
    return TCL_ERROR;
  }
  strncpyz(logtext, argv[3], sizeof logtext);

  putlog(lev, argv[2], "%s", logtext);
  return TCL_OK;
}

static int tcl_binds STDVAR
{
  int matching = 0;
  char *g, flg[100], hits[11];
  EGG_CONST char *list[5];
  tcl_bind_list_t *tl, *tl_kind;
  tcl_bind_mask_t *tm;
  tcl_cmd_t *tc;

  BADARGS(1, 2, " ?type/mask?");

  if (argv[1])
    tl_kind = find_bind_table(argv[1]);
  else
    tl_kind = NULL;
  if (!tl_kind && argv[1])
    matching = 1;
  for (tl = tl_kind ? tl_kind : bind_table_list; tl;
       tl = tl_kind ? 0 : tl->next) {
    if (tl->flags & HT_DELETED)
      continue;
    for (tm = tl->first; tm; tm = tm->next) {
      if (tm->flags & TBM_DELETED)
        continue;
      for (tc = tm->first; tc; tc = tc->next) {
        if (tc->attributes & TC_DELETED)
          continue;
        if (matching &&
            !wild_match_per(argv[1], tl->name) &&
            !wild_match_per(argv[1], tm->mask) &&
            !wild_match_per(argv[1], tc->func_name))
          continue;
        build_flags(flg, &(tc->flags), NULL);
        egg_snprintf(hits, sizeof hits, "%i", (int) tc->hits);
        list[0] = tl->name;
        list[1] = flg;
        list[2] = tm->mask;
        list[3] = hits;
        list[4] = tc->func_name;
        g = Tcl_Merge(5, list);
        Tcl_AppendElement(irp, g);
        Tcl_Free((char *) g);
      }
    }
  }
  return TCL_OK;
}

static int tcl_timer STDVAR
{
  unsigned long x;
  char s[16];

  BADARGS(3, 3, " minutes command");

  if (atoi(argv[1]) < 0) {
    Tcl_AppendResult(irp, "time value must be positive", NULL);
    return TCL_ERROR;
  }
  if (argv[2][0] != '#') {
    x = add_timer(&timer, atoi(argv[1]), argv[2], 0L);
    egg_snprintf(s, sizeof s, "timer%lu", x);
    Tcl_AppendResult(irp, s, NULL);
  }
  return TCL_OK;
}

static int tcl_utimer STDVAR
{
  unsigned long x;
  char s[16];

  BADARGS(3, 3, " seconds command");

  if (atoi(argv[1]) < 0) {
    Tcl_AppendResult(irp, "time value must be positive", NULL);
    return TCL_ERROR;
  }
  if (argv[2][0] != '#') {
    x = add_timer(&utimer, atoi(argv[1]), argv[2], 0L);
    egg_snprintf(s, sizeof s, "timer%lu", x);
    Tcl_AppendResult(irp, s, NULL);
  }
  return TCL_OK;
}

static int tcl_killtimer STDVAR
{
  BADARGS(2, 2, " timerID");

  if (strncmp(argv[1], "timer", 5)) {
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
  BADARGS(2, 2, " timerID");

  if (strncmp(argv[1], "timer", 5)) {
    Tcl_AppendResult(irp, "argument is not a timerID", NULL);
    return TCL_ERROR;
  }
  if (remove_timer(&utimer, atol(&argv[1][5])))
    return TCL_OK;
  Tcl_AppendResult(irp, "invalid timerID", NULL);
  return TCL_ERROR;
}

static int tcl_timers STDVAR
{
  BADARGS(1, 1, "");

  list_timers(irp, timer);
  return TCL_OK;
}

static int tcl_utimers STDVAR
{
  BADARGS(1, 1, "");

  list_timers(irp, utimer);
  return TCL_OK;
}

static int tcl_duration STDVAR
{
  char s[70];
  unsigned long sec, tmp;

  BADARGS(2, 2, " seconds");

  if (atol(argv[1]) <= 0) {
    Tcl_AppendResult(irp, "0 seconds", NULL);
    return TCL_OK;
  }
  sec = atol(argv[1]);

  s[0] = 0;
  if (sec >= 31536000) {
    tmp = (sec / 31536000);
    sprintf(s, "%lu year%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 31536000);
  }
  if (sec >= 604800) {
    tmp = (sec / 604800);
    sprintf(&s[strlen(s)], "%lu week%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 604800);
  }
  if (sec >= 86400) {
    tmp = (sec / 86400);
    sprintf(&s[strlen(s)], "%lu day%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 86400);
  }
  if (sec >= 3600) {
    tmp = (sec / 3600);
    sprintf(&s[strlen(s)], "%lu hour%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 3600);
  }
  if (sec >= 60) {
    tmp = (sec / 60);
    sprintf(&s[strlen(s)], "%lu minute%s ", tmp, (tmp == 1) ? "" : "s");
    sec -= (tmp * 60);
  }
  if (sec > 0) {
    tmp = (sec);
    sprintf(&s[strlen(s)], "%lu second%s", tmp, (tmp == 1) ? "" : "s");
  }
  if (strlen(s) > 0 && s[strlen(s) - 1] == ' ')
    s[strlen(s) - 1] = 0;
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_unixtime STDVAR
{
  char s[11];
  time_t now2 = time(NULL);

  BADARGS(1, 1, "");

  egg_snprintf(s, sizeof s, "%li", (long) now2);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_ctime STDVAR
{
  time_t tt;
  char s[25];

  BADARGS(2, 2, " unixtime");

  tt = (time_t) atol(argv[1]);
  strncpyz(s, ctime(&tt), sizeof s);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_strftime STDVAR
{
  char buf[512];
  struct tm *tm1;
  time_t t;

  BADARGS(2, 3, " format ?time?");

  if (argc == 3)
    t = atol(argv[2]);
  else
    t = now;
  tm1 = localtime(&t);
  if (egg_strftime(buf, sizeof(buf) - 1, argv[1], tm1)) {
    Tcl_AppendResult(irp, buf, NULL);
    return TCL_OK;
  }
  Tcl_AppendResult(irp, " error with strftime", NULL);
  return TCL_ERROR;
}

static int tcl_myip STDVAR
{
  char s[16];

  BADARGS(1, 1, "");

  egg_snprintf(s, sizeof s, "%lu", iptolong(getmyip()));
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_rand STDVAR
{
  long i;
  unsigned long x;
  char s[11];

  BADARGS(2, 2, " limit");

  i = atol(argv[1]);

  if (i <= 0) {
    Tcl_AppendResult(irp, "random limit must be greater than zero", NULL);
    return TCL_ERROR;
  } else if (i > RANDOM_MAX) {
    Tcl_AppendResult(irp, "random limit must be less than ", RANDOM_MAX, NULL);
    return TCL_ERROR;
  }

  x = randint(i);

  egg_snprintf(s, sizeof s, "%lu", x);

  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_sendnote STDVAR
{
  char s[5], from[NOTENAMELEN + 1], to[NOTENAMELEN + 1], msg[451];

  BADARGS(4, 4, " from to message");

  strncpyz(from, argv[1], sizeof from);
  strncpyz(to, argv[2], sizeof to);
  strncpyz(msg, argv[3], sizeof msg);
  egg_snprintf(s, sizeof s, "%d", add_note(to, from, msg, -1, 0));
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_dumpfile STDVAR
{
  char nick[NICKLEN];
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  BADARGS(3, 3, " nickname filename");

  strncpyz(nick, argv[1], sizeof nick);
  get_user_flagrec(get_user_by_nick(nick), &fr, NULL);
  showhelp(argv[1], argv[2], &fr, HELP_TEXT);
  return TCL_OK;
}

static int tcl_dccdumpfile STDVAR
{
  int idx, i;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };

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
  BADARGS(1, 1, "");

  call_hook(HOOK_BACKUP);
  return TCL_OK;
}

static int tcl_die STDVAR
{
  char s[1024];

  BADARGS(1, 2, " ?reason?");

  if (argc == 2) {
    egg_snprintf(s, sizeof s, "BOT SHUTDOWN (%s)", argv[1]);
    strncpyz(quit_msg, argv[1], 1024);
  } else {
    strncpyz(s, "BOT SHUTDOWN (No reason)", sizeof s);
    quit_msg[0] = 0;
  }
  kill_bot(s, quit_msg[0] ? quit_msg : "EXIT");
  return TCL_OK;
}

static int tcl_loadmodule STDVAR
{
  const char *p;

  BADARGS(2, 2, " module-name");

  p = module_load(argv[1]);
  if (p && strcmp(p, MOD_ALREADYLOAD) && !strcmp(argv[0], "loadmodule"))
    putlog(LOG_MISC, "*", "%s %s: %s", MOD_CANTLOADMOD, argv[1], p);
  Tcl_AppendResult(irp, p, NULL);
  return TCL_OK;
}

static int tcl_unloadmodule STDVAR
{
  BADARGS(2, 2, " module-name");

  Tcl_AppendResult(irp, module_unload(argv[1], botnetnick), NULL);
  return TCL_OK;
}

static int tcl_unames STDVAR
{
  char *unix_n, *vers_n;
#ifdef HAVE_UNAME
  struct utsname un;

  if (uname(&un) < 0) {
#endif
    unix_n = "*unknown*";
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
  int i;
  char *p, s[24], s2[24];
  EGG_CONST char *list[100], *list2[2];
  dependancy *dep;
  module_entry *current;

  BADARGS(1, 1, "");

  for (current = module_list; current; current = current->next) {
    list[0] = current->name;
    egg_snprintf(s, sizeof s, "%d.%d", current->major, current->minor);
    list[1] = s;
    i = 2;
    for (dep = dependancy_list; dep && (i < 100); dep = dep->next) {
      if (dep->needing == current) {
        list2[0] = dep->needed->name;
        egg_snprintf(s2, sizeof s2, "%d.%d", dep->major, dep->minor);
        list2[1] = s2;
        list[i] = Tcl_Merge(2, list2);
        i++;
      }
    }
    p = Tcl_Merge(i, list);
    Tcl_AppendElement(irp, p);
    Tcl_Free((char *) p);
    while (i > 2) {
      i--;
      Tcl_Free((char *) list[i]);
    }
  }
  return TCL_OK;
}

static int tcl_loadhelp STDVAR
{
  BADARGS(2, 2, " helpfile-name");

  add_help_reference(argv[1]);
  return TCL_OK;
}

static int tcl_unloadhelp STDVAR
{
  BADARGS(2, 2, " helpfile-name");

  rem_help_reference(argv[1]);
  return TCL_OK;
}

static int tcl_reloadhelp STDVAR
{
  BADARGS(1, 1, "");

  reload_help_data();
  return TCL_OK;
}

static int tcl_callevent STDVAR
{
  BADARGS(2, 2, " event");

  check_tcl_event(argv[1]);
  return TCL_OK;
}

static int tcl_stripcodes STDVAR
{
  int flags = 0;
  char *p;

  BADARGS(3, 3, " strip-flags string");

  for (p = argv[1]; *p; p++)
    switch (*p) {
    case 'a':
      flags |= STRIP_ANSI;
      break;
    case 'b':
      flags |= STRIP_BOLD;
      break;
    case 'c':
      flags |= STRIP_COLOR;
      break;
    case 'g':
      flags |= STRIP_BELLS;
      break;
    case 'r':
      flags |= STRIP_REV;
      break;
    case 'u':
      flags |= STRIP_UNDER;
      break;
    default:
      Tcl_AppendResult(irp, "Invalid strip-flags: ", argv[1], NULL);
      return TCL_ERROR;
    }

  strip_mirc_codes(flags, argv[2]);
  Tcl_AppendResult(irp, argv[2], NULL);
  return TCL_OK;
}

#ifdef USE_TCL_OBJ
static int tcl_md5(cd, irp, objc, objv)
ClientData cd;
Tcl_Interp *irp;
int objc;
Tcl_Obj *CONST objv[];
{
#else
static int tcl_md5 STDVAR
{
#endif /* USE_TCL_OBJ */
  MD5_CTX md5context;
  char digest_string[33], *string;
  unsigned char digest[16];
  int i, len;

#ifdef USE_TCL_OBJ
  if (objc != 2) {
    Tcl_WrongNumArgs(irp, 1, objv, "string");
    return TCL_ERROR;
  }
#  ifdef USE_TCL_BYTE_ARRAYS
  string = (char *)Tcl_GetByteArrayFromObj(objv[1], &len);
#  else
  string = Tcl_GetStringFromObj(objv[1], &len);
#  endif /* USE_TCL_BYTE_ARRAYS */
#else /* USE_TCL_OBJ */
  BADARGS(2, 2, " string");

  string = argv[1];
  len = strlen(argv[1]);
#endif /* USE_TCL_OBJ */

  MD5_Init(&md5context);
  MD5_Update(&md5context, (unsigned char *) string, len);
  MD5_Final(digest, &md5context);
  for (i = 0; i < 16; i++)
    sprintf(digest_string + (i * 2), "%.2x", digest[i]);
  Tcl_AppendResult(irp, digest_string, NULL);
  return TCL_OK;
}

static int tcl_matchaddr STDVAR
{
  BADARGS(3, 3, " mask address");

  if (match_addr(argv[1], argv[2]))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_matchcidr STDVAR
{
  BADARGS(4, 4, " block address prefix");

  if (cidr_match(argv[1], argv[2], atoi(argv[3])))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int tcl_matchstr STDVAR
{
  BADARGS(3, 3, " pattern string");

  if (wild_match(argv[1], argv[2]))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

tcl_cmds tclmisc_objcmds[] = {
#ifdef USE_TCL_OBJ
  {"md5", tcl_md5},
#endif /* USE_TCL_OBJ */
  {NULL,     NULL}
};

tcl_cmds tclmisc_cmds[] = {
  {"logfile",           tcl_logfile},
  {"putlog",             tcl_putlog},
  {"putcmdlog",       tcl_putcmdlog},
  {"putxferlog",     tcl_putxferlog},
  {"putloglev",       tcl_putloglev},
  {"timer",               tcl_timer},
  {"utimer",             tcl_utimer},
  {"killtimer",       tcl_killtimer},
  {"killutimer",     tcl_killutimer},
  {"timers",             tcl_timers},
  {"utimers",           tcl_utimers},
  {"unixtime",         tcl_unixtime},
  {"strftime",         tcl_strftime},
  {"ctime",               tcl_ctime},
  {"myip",                 tcl_myip},
  {"rand",                 tcl_rand},
  {"sendnote",         tcl_sendnote},
  {"dumpfile",         tcl_dumpfile},
  {"dccdumpfile",   tcl_dccdumpfile},
  {"backup",             tcl_backup},
  {"exit",                  tcl_die},
  {"die",                   tcl_die},
  {"unames",             tcl_unames},
  {"unloadmodule", tcl_unloadmodule},
  {"loadmodule",     tcl_loadmodule},
  {"checkmodule",    tcl_loadmodule},
  {"modules",           tcl_modules},
  {"loadhelp",         tcl_loadhelp},
  {"unloadhelp",     tcl_unloadhelp},
  {"reloadhelp",     tcl_reloadhelp},
  {"duration",         tcl_duration},
#ifndef USE_TCL_OBJ
  {"md5",                   tcl_md5},
#endif /* USE_TCL_OBJ */
  {"binds",               tcl_binds},
  {"callevent",       tcl_callevent},
  {"stripcodes",     tcl_stripcodes},
  {"matchaddr",       tcl_matchaddr},
  {"matchcidr",       tcl_matchcidr},
  {"matchstr",         tcl_matchstr},
  {NULL,                       NULL}
};
