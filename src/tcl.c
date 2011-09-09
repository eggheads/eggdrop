/*
 * tcl.c -- handles:
 *   the code for every command eggdrop adds to Tcl
 *   Tcl initialization
 *   getting and setting Tcl/eggdrop variables
 *
 * $Id: tcl.c,v 1.102 2011/09/09 21:37:53 thommey Exp $
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

#include <stdlib.h>             /* getenv()                             */
#include <locale.h>             /* setlocale()                          */
#include "main.h"

/* Used for read/write to internal strings */
typedef struct {
  char *str;                    /* Pointer to actual string in eggdrop       */
  int max;                      /* max length (negative: read-only var
                                 * when protect is on) (0: read-only ALWAYS) */
  int flags;                    /* 1 = directory                             */
} strinfo;

typedef struct {
  int *var;
  int ro;
} intinfo;


extern time_t online_since;

extern char origbotname[], botuser[], motdfile[], admin[], userfile[],
            firewall[], helpdir[], notify_new[], hostname[], myip[], moddir[],
            tempdir[], owner[], network[], botnetnick[], bannerfile[],
            egg_version[], natip[], configfile[], logfile_suffix[], log_ts[],
            textdir[], pid_file[];

extern int flood_telnet_thr, flood_telnet_time, shtime, share_greet,
           require_p, keep_all_logs, allow_new_telnets, stealth_telnets,
           use_telnet_banner, default_flags, conmask, switch_logfiles_at,
           connect_timeout, firewallport, notify_users_at, flood_thr, tands,
           ignore_time, reserved_port_min, reserved_port_max, die_on_sighup,
           die_on_sigterm, max_logs, max_logsize, dcc_total, raw_log,
           identtimeout, dcc_sanitycheck, dupwait_timeout, egg_numver,
           share_unlinks, protect_telnet, sort_users, strict_host,
           resolve_timeout, default_uflags, userfile_perm, cidr_support;

extern struct dcc_t *dcc;
extern tcl_timer_t *timer, *utimer;

Tcl_Interp *interp;

int protect_readonly = 0; /* Enable read-only protection? */
char whois_fields[1025] = "";

int dcc_flood_thr = 3;
int use_invites = 0;
int use_exempts = 0;
int force_expire = 0;
int remote_boots = 2;
int allow_dk_cmds = 1;
int must_be_owner = 1;
int quiet_reject = 1;
int copy_to_tmp = 1;
int max_socks = 100;
int quick_logs = 0;
int par_telnet_flood = 1;
int quiet_save = 0;
int strtot = 0;
int handlen = HANDLEN;
int utftot = 0;
int clientdata_stuff = 0;

/* Compatability for removed settings.*/
int strict_servernames = 0, enable_simul = 1, use_console_r = 0,
    debug_output = 0;

#ifdef REPLACE_NOTIFIER
tclevent_t *tclevents = NULL; /* Queue of Tcl snippets to execute */
#endif

/* Prototypes for Tcl */
Tcl_Interp *Tcl_CreateInterp();

int expmem_tcl()
{
  return strtot + utftot + clientdata_stuff;
}

static void botnet_change(char *new)
{
  if (egg_strcasecmp(botnetnick, new)) {
    /* Trying to change bot's nickname */
    if (tands > 0) {
      putlog(LOG_MISC, "*", "* Tried to change my botnet nick, but I'm still "
             "linked to a botnet.");
      putlog(LOG_MISC, "*", "* (Unlink and try again.)");
      return;
    } else {
      if (botnetnick[0])
        putlog(LOG_MISC, "*", "* IDENTITY CHANGE: %s -> %s", botnetnick, new);
      strcpy(botnetnick, new);
    }
  }
}


/*
 *     Vars, traces, misc
 */

int init_misc();

/* Used for read/write to integer couplets */
typedef struct {
  int *left;  /* left side of couplet */
  int *right; /* right side */
} coupletinfo;

/* FIXME: tcl_eggcouplet() should be redesigned so we can use
 * TCL_TRACE_WRITES | TCL_TRACE_READS as the bit mask instead
 * of 2 calls as is done in add_tcl_coups().
 */
/* Read/write integer couplets (int1:int2) */
static char *tcl_eggcouplet(ClientData cdata, Tcl_Interp *irp,
                            EGG_CONST char *name1,
                            EGG_CONST char *name2, int flags)
{
  char *s, s1[41];
  coupletinfo *cp = (coupletinfo *) cdata;

  if (flags & (TCL_TRACE_READS | TCL_TRACE_UNSETS)) {
    egg_snprintf(s1, sizeof s1, "%d:%d", *(cp->left), *(cp->right));
    Tcl_SetVar2(interp, name1, name2, s1, TCL_GLOBAL_ONLY);
    if (flags & TCL_TRACE_UNSETS)
      Tcl_TraceVar(interp, name1,
                   TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                   tcl_eggcouplet, cdata);
  } else {                        /* writes */
    s = (char *) Tcl_GetVar2(interp, name1, name2, 0);
    if (s != NULL) {
      int nr1, nr2;

      nr1 = nr2 = 0;

      if (strlen(s) > 40)
        s[40] = 0;

      sscanf(s, "%d%*c%d", &nr1, &nr2);
      *(cp->left) = nr1;
      *(cp->right) = nr2;
    }
  }
  return NULL;
}

/* Read or write normal integer.
 */
static char *tcl_eggint(ClientData cdata, Tcl_Interp *irp,
                        EGG_CONST char *name1,
                        EGG_CONST char *name2, int flags)
{
  char *s, s1[40];
  long l;
  intinfo *ii = (intinfo *) cdata;

  if (flags & (TCL_TRACE_READS | TCL_TRACE_UNSETS)) {
    /* Special cases */
    if ((int *) ii->var == &conmask)
      strcpy(s1, masktype(conmask));
    else if ((int *) ii->var == &default_flags) {
      struct flag_record fr = { FR_GLOBAL, 0, 0, 0, 0, 0 };
      fr.global = default_flags;

      fr.udef_global = default_uflags;
      build_flags(s1, &fr, 0);
    } else if ((int *) ii->var == &userfile_perm) {
      egg_snprintf(s1, sizeof s1, "0%o", userfile_perm);
    } else
      egg_snprintf(s1, sizeof s1, "%d", *(int *) ii->var);
    Tcl_SetVar2(interp, name1, name2, s1, TCL_GLOBAL_ONLY);
    if (flags & TCL_TRACE_UNSETS)
      Tcl_TraceVar(interp, name1,
                   TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                   tcl_eggint, cdata);
    return NULL;
  } else {                        /* Writes */
    s = (char *) Tcl_GetVar2(interp, name1, name2, TCL_GLOBAL_ONLY);
    if (s != NULL) {
      if ((int *) ii->var == &conmask) {
        if (s[0])
          conmask = logmodes(s);
        else
          conmask = LOG_MODES | LOG_MISC | LOG_CMDS;
      } else if ((int *) ii->var == &default_flags) {
        struct flag_record fr = { FR_GLOBAL, 0, 0, 0, 0, 0 };

        break_down_flags(s, &fr, 0);
        default_flags = sanity_check(fr.global);        /* drummer */

        default_uflags = fr.udef_global;
      } else if ((int *) ii->var == &userfile_perm) {
        int p = oatoi(s);

        if (p <= 0)
          return "invalid userfile permissions";
        userfile_perm = p;
      } else if ((ii->ro == 2) || ((ii->ro == 1) && protect_readonly))
        return "read-only variable";
      else {
        if (Tcl_ExprLong(interp, s, &l) == TCL_ERROR)
          return "variable must have integer value";
        if ((int *) ii->var == &max_socks) {
          if (l < threaddata()->MAXSOCKS)
            return "you can't DECREASE max-socks below current usage";
          max_socks = l;
        } else if ((int *) ii->var == &max_logs) {
          if (l < max_logs)
            return "you can't DECREASE max-logs";
          max_logs = l;
          init_misc();
        } else
          *(ii->var) = (int) l;
      }
    }
    return NULL;
  }
}

/* Read/write normal string variable
 */
static char *tcl_eggstr(ClientData cdata, Tcl_Interp *irp,
                        EGG_CONST char *name1,
                        EGG_CONST char *name2, int flags)
{
  char *s;
  strinfo *st = (strinfo *) cdata;

  if (flags & (TCL_TRACE_READS | TCL_TRACE_UNSETS)) {
    if ((st->str == firewall) && (firewall[0])) {
      char s1[127];

      egg_snprintf(s1, sizeof s1, "%s:%d", firewall, firewallport);
      Tcl_SetVar2(interp, name1, name2, s1, TCL_GLOBAL_ONLY);
    } else
      Tcl_SetVar2(interp, name1, name2, st->str, TCL_GLOBAL_ONLY);
    if (flags & TCL_TRACE_UNSETS) {
      Tcl_TraceVar(interp, name1, TCL_TRACE_READS | TCL_TRACE_WRITES |
                   TCL_TRACE_UNSETS, tcl_eggstr, cdata);
      if ((st->max <= 0) && (protect_readonly || (st->max == 0)))
        return "read-only variable";    /* it won't return the error... */
    }
    return NULL;
  } else {                        /* writes */
    if ((st->max <= 0) && (protect_readonly || (st->max == 0))) {
      Tcl_SetVar2(interp, name1, name2, st->str, TCL_GLOBAL_ONLY);
      return "read-only variable";
    }
#ifdef USE_TCL_BYTE_ARRAYS
#  undef malloc
#  undef free
    {
      Tcl_Obj *obj;
      unsigned char *bytes;
      int len;

      obj = Tcl_GetVar2Ex(interp, name1, name2, 0);
      if (!obj)
        return NULL;
      len = 0;
      bytes = Tcl_GetByteArrayFromObj(obj, &len);
      if (!bytes)
        return NULL;
      s = malloc(len + 1);
      egg_memcpy(s, bytes, len);
      s[len] = 0;
    }
#else
    s = (char *) Tcl_GetVar2(interp, name1, name2, 0);
#endif /* USE_TCL_BYTE_ARRAYS */
    if (s != NULL) {
      if (strlen(s) > abs(st->max))
        s[abs(st->max)] = 0;
      if (st->str == botnetnick)
        botnet_change(s);
      else if (st->str == logfile_suffix)
        logsuffix_change(s);
      else if (st->str == firewall) {
        splitc(firewall, s, ':');
        if (!firewall[0])
          strcpy(firewall, s);
        else
          firewallport = atoi(s);
      } else
        strcpy(st->str, s);
      if ((st->flags) && (s[0])) {
        if (st->str[strlen(st->str) - 1] != '/')
          strcat(st->str, "/");
      }
#ifdef USE_TCL_BYTE_ARRAYS
      free(s);
#endif /* USE_TCL_BYTE_ARRAYS */
    }
    return NULL;
  }
}

/* Add/remove tcl commands
 */

#ifdef USE_TCL_BYTE_ARRAYS
static int utf_converter(ClientData cdata, Tcl_Interp *myinterp, int objc,
                         Tcl_Obj *CONST objv[])
{
  char **strings, *byteptr;
  int i, len, retval, diff;
  void **callback_data;
  Function func;
  ClientData cd;

  objc += 5;
  strings = (char **) nmalloc(sizeof(char *) * objc);
  egg_memset(strings, 0, sizeof(char *) * objc);
  diff = utftot;
  utftot += sizeof(char *) * objc;
  objc -= 5;
  for (i = 0; i < objc; i++) {
    byteptr = (char *) Tcl_GetByteArrayFromObj(objv[i], &len);
    strings[i] = (char *) nmalloc(len + 1);
    utftot += len + 1;
    strncpy(strings[i], byteptr, len);
    strings[i][len] = 0;
  }
  callback_data = (void **) cdata;
  func = (Function) callback_data[0];
  cd = (ClientData) callback_data[1];
  diff -= utftot;
  retval = func(cd, myinterp, objc, strings);
  for (i = 0; i < objc; i++)
    nfree(strings[i]);
  nfree(strings);
  utftot += diff;
  return retval;
}

void cmd_delete_callback(ClientData cdata)
{
  nfree(cdata);
  clientdata_stuff -= sizeof(void *) * 2;
}
#endif /* USE_TCL_BYTE_ARRAYS */

#ifdef USE_TCL_BYTE_ARRAYS
void add_tcl_commands(tcl_cmds *table)
{
  void **cdata;

  while (table->name) {
    cdata = (void **) nmalloc(sizeof(void *) * 2);
    clientdata_stuff += sizeof(void *) * 2;
    cdata[0] = (void *)table->func;
    cdata[1] = NULL;
    Tcl_CreateObjCommand(interp, table->name, utf_converter, (ClientData) cdata,
                         cmd_delete_callback);
    table++;
  }
}

#else /* USE_TCL_BYTE_ARRAYS */

void add_tcl_commands(tcl_cmds *table)
{
  int i;

  for (i = 0; table[i].name; i++)
    Tcl_CreateCommand(interp, table[i].name, table[i].func, NULL, NULL);
}
#endif /* USE_TCL_BYTE_ARRAYS */

#ifdef USE_TCL_BYTE_ARRAYS
void add_cd_tcl_cmds(cd_tcl_cmd *table)
{
  void **cdata;

  while (table->name) {
    cdata = nmalloc(sizeof(void *) * 2);
    clientdata_stuff += sizeof(void *) * 2;
    cdata[0] = (void *)table->callback;
    cdata[1] = table->cdata;
    Tcl_CreateObjCommand(interp, table->name, utf_converter, (ClientData) cdata,
                         cmd_delete_callback);
    table++;
  }
}

#else /* USE_TCL_BYTE_ARRAYS */

void add_cd_tcl_cmds(cd_tcl_cmd *table)
{
  while (table->name) {
    Tcl_CreateCommand(interp, table->name, table->callback,
                      (ClientData) table->cdata, NULL);
    table++;
  }
}
#endif /* USE_TCL_BYTE_ARRAYS */

void rem_tcl_commands(tcl_cmds *table)
{
  int i;

  for (i = 0; table[i].name; i++)
    Tcl_DeleteCommand(interp, table[i].name);
}

void rem_cd_tcl_cmds(cd_tcl_cmd *table)
{
  while (table->name) {
    Tcl_DeleteCommand(interp, table->name);
    table++;
  }
}

#ifdef USE_TCL_OBJ
void add_tcl_objcommands(tcl_cmds *table)
{
  int i;

  for (i = 0; table[i].name; i++)
    Tcl_CreateObjCommand(interp, table[i].name, table[i].func, (ClientData) 0,
                         NULL);
}
#endif

/* Get the current tcl result string. */
const char *tcl_resultstring()
{
  const char *result;
#ifdef USE_TCL_OBJ
  result = Tcl_GetStringResult(interp);
#else
  result = interp->result;
#endif
  return result;
}

int tcl_resultempty() {
  const char *result;
  result = tcl_resultstring();
  return (result && result[0]) ? 0 : 1;
}

/* Get the current tcl result as int. replaces atoi(interp->result) */
int tcl_resultint()
{
  int result;
#ifdef USE_TCL_OBJ
  if (Tcl_GetIntFromObj(NULL, Tcl_GetObjResult(interp), &result) != TCL_OK)
#else
  if (Tcl_GetInt(NULL, interp->result, &result) != TCL_OK)
#endif
    result = 0;
  return result;
}

static tcl_strings def_tcl_strings[] = {
  {"botnet-nick",     botnetnick,     HANDLEN,                 0},
  {"userfile",        userfile,       120,           STR_PROTECT},
  {"motd",            motdfile,       120,           STR_PROTECT},
  {"admin",           admin,          120,                     0},
  {"help-path",       helpdir,        120, STR_DIR | STR_PROTECT},
  {"temp-path",       tempdir,        120, STR_DIR | STR_PROTECT},
  {"text-path",       textdir,        120, STR_DIR | STR_PROTECT},
#ifndef STATIC
  {"mod-path",        moddir,         120, STR_DIR | STR_PROTECT},
#endif
  {"notify-newusers", notify_new,     120,                     0},
  {"owner",           owner,          120,           STR_PROTECT},
  {"my-ip",           myip,           120,                     0},
  {"my-hostname",     hostname,       120,                     0},
  {"network",         network,        40,                      0},
  {"whois-fields",    whois_fields,   1024,                    0},
  {"nat-ip",          natip,          120,                     0},
  {"username",        botuser,        10,                      0},
  {"version",         egg_version,    0,                       0},
  {"firewall",        firewall,       120,                     0},
  {"config",          configfile,     0,                       0},
  {"telnet-banner",   bannerfile,     120,           STR_PROTECT},
  {"logfile-suffix",  logfile_suffix, 20,                      0},
  {"timestamp-format",log_ts,         32,                      0},
  {"pidfile",         pid_file,       120,           STR_PROTECT},
  {NULL,              NULL,           0,                       0}
};

static tcl_ints def_tcl_ints[] = {
  {"ignore-time",           &ignore_time,          0},
  {"handlen",               &handlen,              2},
  {"dcc-flood-thr",         &dcc_flood_thr,        0},
  {"hourly-updates",        &notify_users_at,      0},
  {"switch-logfiles-at",    &switch_logfiles_at,   0},
  {"connect-timeout",       &connect_timeout,      0},
  {"reserved-port",         &reserved_port_min,    0},
  {"require-p",             &require_p,            0},
  {"keep-all-logs",         &keep_all_logs,        0},
  {"open-telnets",          &allow_new_telnets,    0},
  {"stealth-telnets",       &stealth_telnets,      0},
  {"use-telnet-banner",     &use_telnet_banner,    0},
  {"uptime",                (int *) &online_since, 2},
  {"console",               &conmask,              0},
  {"default-flags",         &default_flags,        0},
  {"numversion",            &egg_numver,           2},
  {"die-on-sighup",         &die_on_sighup,        1},
  {"die-on-sigterm",        &die_on_sigterm,       1},
  {"remote-boots",          &remote_boots,         1},
  {"max-socks",             &max_socks,            0},
  {"max-logs",              &max_logs,             0},
  {"max-logsize",           &max_logsize,          0},
  {"quick-logs",            &quick_logs,           0},
  {"raw-log",               &raw_log,              1},
  {"protect-telnet",        &protect_telnet,       0},
  {"dcc-sanitycheck",       &dcc_sanitycheck,      0},
  {"sort-users",            &sort_users,           0},
  {"ident-timeout",         &identtimeout,         0},
  {"share-unlinks",         &share_unlinks,        0},
  {"log-time",              &shtime,               0},
  {"allow-dk-cmds",         &allow_dk_cmds,        0},
  {"resolve-timeout",       &resolve_timeout,      0},
  {"must-be-owner",         &must_be_owner,        1},
  {"paranoid-telnet-flood", &par_telnet_flood,     0},
  {"use-exempts",           &use_exempts,          0},
  {"use-invites",           &use_invites,          0},
  {"quiet-save",            &quiet_save,           0},
  {"force-expire",          &force_expire,         0},
  {"dupwait-timeout",       &dupwait_timeout,      0},
  {"strict-host",           &strict_host,          0},
  {"userfile-perm",         &userfile_perm,        0},
  {"copy-to-tmp",           &copy_to_tmp,          0},
  {"quiet-reject",          &quiet_reject,         0},
  {"cidr-support",          &cidr_support,         0},
  {"strict-servernames",    &strict_servernames,   0}, /* compat */
  {"enable-simul",          &enable_simul,         0}, /* compat */
  {"debug-output",          &debug_output,         0}, /* compat */
  {"use-console-r",         &use_console_r,        0}, /* compat */
  {NULL,                    NULL,                  0}
};

static tcl_coups def_tcl_coups[] = {
  {"telnet-flood",       &flood_telnet_thr,  &flood_telnet_time},
  {"reserved-portrange", &reserved_port_min, &reserved_port_max},
  {NULL,                 NULL,                             NULL}
};

/* Set up Tcl variables that will hook into eggdrop internal vars via
 * trace callbacks.
 */
static void init_traces()
{
  add_tcl_coups(def_tcl_coups);
  add_tcl_strings(def_tcl_strings);
  add_tcl_ints(def_tcl_ints);
}

/* tcleventcallback that's called after executing Tcl code,
 * used by do_tcl(). Just logs Tcl errors.
 */
void bgtclcallback(char *context, char *script, int code,
                   const char *result, int dofree) {
#ifdef USE_TCL_ENCODING
  Tcl_DString dstr;
#endif

  if (code == TCL_ERROR) {
#ifdef USE_TCL_ENCODING
    /* properly convert string to system encoding. */
    Tcl_DStringInit(&dstr);
    Tcl_UtfToExternalDString(NULL, result, -1, &dstr);
    result = Tcl_DStringValue(&dstr);
#endif
    putlog(LOG_MISC, "*", "Tcl error in script for '%s':", context);
    putlog(LOG_MISC, "*", "%s", result);
#ifdef USE_TCL_ENCODING
  Tcl_DStringFree(&dstr);
#endif
  }
  if (dofree) {
    nfree(context);
    nfree(script);
  }
}

/* Generic function (exported to modules) to execute Tcl code.
 * context is used to display Tcl errors (Error in '..')
 *
 * Schedules execution if defined(REPLACE_NOTIFIER) to bottom of mainloop,
 * where it's safe to recurse eventually (vwait/update).
 */
void do_tcl(char *context, char *script) {
  do_tcl_async(context, script, bgtclcallback);
}

/* Evaluates Tcl code.
 *
 * Should only be called from reentrant functions (can recurse),
 * use do_tcl/do_tcl_async instead.
 */
void do_tcl_sync(char *context, char *script, tcleventcallback cb, int dofree) {
  int x;

  x = Tcl_Eval(interp, script);
  cb(context, script, x, tcl_resultstring(), dofree);
}

extern tcl_cmds tcluser_cmds[], tcldcc_cmds[], tclmisc_cmds[],
       tclmisc_objcmds[], tcldns_cmds[];

#ifdef REPLACE_NOTIFIER
/* The tickle_*() functions replace the Tcl Notifier
 * The tickle_*() functions can be called by Tcl threads
 */
void tickle_SetTimer (TCL_CONST86 Tcl_Time *timePtr)
{
  struct threaddata *td = threaddata();
  /* we can block 1 second maximum, because we have SECONDLY events */
  if (!timePtr || timePtr->sec > 1 || (timePtr->sec == 1 && timePtr->usec > 0)) {
    td->blocktime.tv_sec = 1;
    td->blocktime.tv_usec = 0;
  } else {
    td->blocktime.tv_sec = timePtr->sec;
    td->blocktime.tv_usec = timePtr->usec;
  }
}

int tickle_WaitForEvent (TCL_CONST86 Tcl_Time *timePtr)
{
  struct threaddata *td = threaddata();

  tickle_SetTimer(timePtr);
  return (*td->mainloopfunc)(0);
}

void tickle_CreateFileHandler(int fd, int mask, Tcl_FileProc *proc, ClientData cd)
{
  alloctclsock(fd, mask, proc, cd);
}

void tickle_DeleteFileHandler(int fd)
{
  killtclsock(fd);
}

void tickle_FinalizeNotifier(ClientData cd)
{
  struct threaddata *td = threaddata();
  if (td->socklist)
    nfree(td->socklist);
}

ClientData tickle_InitNotifier()
{
  static int ismainthread = 1;
  init_threaddata(ismainthread);
  if (ismainthread)
    ismainthread = 0;
  return NULL;
}

int tclthreadmainloop(int zero)
{
  int i;
  i = sockread(NULL, NULL, threaddata()->socklist, threaddata()->MAXSOCKS, 1);
  return (i == -5);
}

struct threaddata *threaddata()
{
  static Tcl_ThreadDataKey tdkey;
  struct threaddata *td = Tcl_GetThreadData(&tdkey, sizeof(struct threaddata));
  return td;
}

/* Add a script snippet to the queue of Tcl events to execute.
 * See tclegg.h for the definition of the callback.
 */
void do_tcl_async(char *context, char *script, tcleventcallback callback) {
  tclevent_t *ev, *e = tclevents;

  ev = nmalloc(sizeof(tclevent_t));
  ev->script = nstrdup(script);
  ev->context = nstrdup(context);
  ev->callback = callback;
  ev->next = NULL;

  if (!e)
    tclevents = ev;
  else {
    while (e->next)
      e = e->next;
    e->next = ev;
  }
}

#else /* REPLACE_NOTIFIER */

int tclthreadmainloop() { return 0; }

struct threaddata *threaddata()
{
  static struct threaddata tsd;
  return &tsd;
}

/* Without the replacement notifier, this just executes the script. */
void do_tcl_async(char *context, char *script, tcleventcallback callback) {
  do_tcl_sync(context, script, callback, 0);
}

#endif /* REPLACE_NOTIFIER */

int init_threaddata(int mainthread)
{
  struct threaddata *td = threaddata();
/* Nested evaluation (vwait/update) of the event loop only
 * processes Tcl events (after/fileevent) for now. Using
 * eggdrops mainloop() requires caution regarding reentrance.
 * (check_tcl_* -> Tcl_Eval() -> mainloop() -> check_tcl_* etc.)
 */
/* td->mainloopfunc = mainthread ? mainloop : tclthreadmainloop; */
  td->mainloopfunc = tclthreadmainloop;
  td->socklist = NULL;
  td->mainthread = mainthread;
  td->blocktime.tv_sec = 1;
  td->blocktime.tv_usec = 0;
  td->MAXSOCKS = 0;
  increase_socks_max();
  return 0;
}

/* Not going through Tcl's crazy main() system (what on earth was he
 * smoking?!) so we gotta initialize the Tcl interpreter
 */
void init_tcl(int argc, char **argv)
{
#ifdef REPLACE_NOTIFIER
  Tcl_NotifierProcs notifierprocs;
#endif /* REPLACE_NOTIFIER */

#ifdef USE_TCL_ENCODING
  const char *encoding;
  int i;
  char *langEnv;
#endif /* USE_TCL_ENCODING */
#ifdef USE_TCL_PACKAGE
  int j;
  char pver[1024] = "";
#endif /* USE_TCL_PACKAGE */

#ifdef REPLACE_NOTIFIER
  egg_bzero(&notifierprocs, sizeof(notifierprocs));
  notifierprocs.initNotifierProc = tickle_InitNotifier;
  notifierprocs.createFileHandlerProc = tickle_CreateFileHandler;
  notifierprocs.deleteFileHandlerProc = tickle_DeleteFileHandler;
  notifierprocs.setTimerProc = tickle_SetTimer;
  notifierprocs.waitForEventProc = tickle_WaitForEvent;
  notifierprocs.finalizeNotifierProc = tickle_FinalizeNotifier;

  Tcl_SetNotifier(&notifierprocs);
#endif /* REPLACE_NOTIFIER */

/* This must be done *BEFORE* Tcl_SetSystemEncoding(),
 * or Tcl_SetSystemEncoding() will cause a segfault.
 */
#ifdef USE_TCL_FINDEXEC
  /* This is used for 'info nameofexecutable'.
   * The filename in argv[0] must exist in a directory listed in
   * the environment variable PATH for it to register anything.
   */
  Tcl_FindExecutable(argv[0]);
#endif /* USE_TCL_FINDEXEC */

  /* Initialize the interpreter */
  interp = Tcl_CreateInterp();

#ifdef DEBUG_MEM
  /* Initialize Tcl's memory debugging if we want it */
  Tcl_InitMemory(interp);
#endif

  /* Set Tcl variable tcl_interactive to 0 */
  Tcl_SetVar(interp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);

  /* Setup script library facility */
  Tcl_Init(interp);
  Tcl_SetServiceMode(TCL_SERVICE_ALL);

/* Code based on Tcl's TclpSetInitialEncodings() */
#ifdef USE_TCL_ENCODING
  /* Determine the current encoding from the LC_* or LANG environment
   * variables.
   */
  langEnv = getenv("LC_ALL");
  if (langEnv == NULL || langEnv[0] == '\0') {
    langEnv = getenv("LC_CTYPE");
  }
  if (langEnv == NULL || langEnv[0] == '\0') {
    langEnv = getenv("LANG");
  }
  if (langEnv == NULL || langEnv[0] == '\0') {
    langEnv = NULL;
  }

  encoding = NULL;
  if (langEnv != NULL) {
    for (i = 0; localeTable[i].lang != NULL; i++)
      if (strcmp(localeTable[i].lang, langEnv) == 0) {
        encoding = localeTable[i].encoding;
        break;
      }

    /* There was no mapping in the locale table.  If there is an
     * encoding subfield, we can try to guess from that.
     */
    if (encoding == NULL) {
      char *p;

      for (p = langEnv; *p != '\0'; p++) {
        if (*p == '.') {
          p++;
          break;
        }
      }
      if (*p != '\0') {
        Tcl_DString ds;

        Tcl_DStringInit(&ds);
        Tcl_DStringAppend(&ds, p, -1);

        encoding = Tcl_DStringValue(&ds);
        Tcl_UtfToLower(Tcl_DStringValue(&ds));
        if (Tcl_SetSystemEncoding(NULL, encoding) == TCL_OK) {
          Tcl_DStringFree(&ds);
          goto resetPath;
        }
        Tcl_DStringFree(&ds);
        encoding = NULL;
      }
    }
  }

  if (encoding == NULL) {
    encoding = "iso8859-1";
  }

  Tcl_SetSystemEncoding(NULL, encoding);

resetPath:

  /* Initialize the C library's locale subsystem. */
  setlocale(LC_CTYPE, "");

  /* In case the initial locale is not "C", ensure that the numeric
   * processing is done in "C" locale regardless. */
  setlocale(LC_NUMERIC, "C");

  /* Keep the iso8859-1 encoding preloaded.  The IO package uses it for
   * gets on a binary channel. */
  Tcl_GetEncoding(NULL, "iso8859-1");
#endif /* USE_TCL_ENCODING */

#ifdef USE_TCL_PACKAGE
  /* Add eggdrop to Tcl's package list */
  for (j = 0; j <= strlen(egg_version); j++) {
    if ((egg_version[j] == ' ') || (egg_version[j] == '+'))
      break;
    pver[strlen(pver)] = egg_version[j];
  }
  Tcl_PkgProvide(interp, "eggdrop", pver);
#endif /* USE_TCL_PACKAGE */

  /* Initialize binds and traces */
  init_bind();
  init_traces();

  /* Add new commands */
  add_tcl_commands(tcluser_cmds);
  add_tcl_commands(tcldcc_cmds);
  add_tcl_commands(tclmisc_cmds);
#ifdef USE_TCL_OBJ
  add_tcl_objcommands(tclmisc_objcmds);
#endif
  add_tcl_commands(tcldns_cmds);
}

void kill_tcl()
{
#ifdef REPLACE_NOTIFIER
  tclevent_t *ne, *e = tclevents;

  while (e) {
    ne = e->next;
    e->callback(e->context, e->script, -1, "", 1);
    nfree(e);
    e = ne;
  }
#endif
  rem_tcl_coups(def_tcl_coups);
  rem_tcl_strings(def_tcl_strings);
  rem_tcl_ints(def_tcl_ints);
  kill_bind();
  Tcl_DeleteInterp(interp);
}


/* Interpret tcl file fname.
 *
 * returns:   1 - if everything was okay
 */
int readtclprog(char *fname)
{
  int code;
  EGG_CONST char *result;
#ifdef USE_TCL_ENCODING
  Tcl_DString dstr;
#endif

  if (!file_readable(fname))
    return 0;

  code = Tcl_EvalFile(interp, fname);
  result = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);

#ifdef USE_TCL_ENCODING
  /* properly convert string to system encoding. */
  Tcl_DStringInit(&dstr);
  Tcl_UtfToExternalDString(NULL, result, -1, &dstr);
  result = Tcl_DStringValue(&dstr);
#endif

  if (code != TCL_OK) {
    putlog(LOG_MISC, "*", "Tcl error in file '%s':", fname);
    putlog(LOG_MISC, "*", "%s", result);
    code = 0; /* JJM: refactored to remove premature return */
  } else {
    /* Refresh internal variables */
    code = 1;
  }

#ifdef USE_TCL_ENCODING
  Tcl_DStringFree(&dstr);
#endif

  return code;
}

void add_tcl_strings(tcl_strings *list)
{
  int i;
  strinfo *st;
  int tmp;

  for (i = 0; list[i].name; i++) {
    st = nmalloc(sizeof *st);
    strtot += sizeof(strinfo);
    st->max = list[i].length - (list[i].flags & STR_DIR);
    if (list[i].flags & STR_PROTECT)
      st->max = -st->max;
    st->str = list[i].buf;
    st->flags = (list[i].flags & STR_DIR);
    tmp = protect_readonly;
    protect_readonly = 0;
    tcl_eggstr((ClientData) st, interp, list[i].name, NULL, TCL_TRACE_WRITES);
    protect_readonly = tmp;
    tcl_eggstr((ClientData) st, interp, list[i].name, NULL, TCL_TRACE_READS);
    Tcl_TraceVar(interp, list[i].name, TCL_TRACE_READS | TCL_TRACE_WRITES |
                 TCL_TRACE_UNSETS, tcl_eggstr, (ClientData) st);
  }
}

void rem_tcl_strings(tcl_strings *list)
{
  int i, f;
  strinfo *st;

  f = TCL_GLOBAL_ONLY | TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS;
  for (i = 0; list[i].name; i++) {
    st = (strinfo *) Tcl_VarTraceInfo(interp, list[i].name, f, tcl_eggstr,
                                      NULL);
    Tcl_UntraceVar(interp, list[i].name, f, tcl_eggstr, st);
    if (st != NULL) {
      strtot -= sizeof(strinfo);
      nfree(st);
    }
  }
}

void add_tcl_ints(tcl_ints *list)
{
  int i, tmp;
  intinfo *ii;

  for (i = 0; list[i].name; i++) {
    ii = nmalloc(sizeof *ii);
    strtot += sizeof(intinfo);
    ii->var = list[i].val;
    ii->ro = list[i].readonly;
    tmp = protect_readonly;
    protect_readonly = 0;
    tcl_eggint((ClientData) ii, interp, list[i].name, NULL, TCL_TRACE_WRITES);
    protect_readonly = tmp;
    tcl_eggint((ClientData) ii, interp, list[i].name, NULL, TCL_TRACE_READS);
    Tcl_TraceVar(interp, list[i].name,
                 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                 tcl_eggint, (ClientData) ii);
  }

}

void rem_tcl_ints(tcl_ints *list)
{
  int i, f;
  intinfo *ii;

  f = TCL_GLOBAL_ONLY | TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS;
  for (i = 0; list[i].name; i++) {
    ii = (intinfo *) Tcl_VarTraceInfo(interp, list[i].name, f, tcl_eggint,
                                      NULL);
    Tcl_UntraceVar(interp, list[i].name, f, tcl_eggint, (ClientData) ii);
    if (ii) {
      strtot -= sizeof(intinfo);
      nfree(ii);
    }
  }
}

/* Allocate couplet space for tracing couplets
 */
void add_tcl_coups(tcl_coups *list)
{
  coupletinfo *cp;
  int i;

  for (i = 0; list[i].name; i++) {
    cp = nmalloc(sizeof *cp);
    strtot += sizeof(coupletinfo);
    cp->left = list[i].lptr;
    cp->right = list[i].rptr;
    tcl_eggcouplet((ClientData) cp, interp, list[i].name, NULL,
                   TCL_TRACE_WRITES);
    tcl_eggcouplet((ClientData) cp, interp, list[i].name, NULL,
                   TCL_TRACE_READS);
    Tcl_TraceVar(interp, list[i].name,
                 TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                 tcl_eggcouplet, (ClientData) cp);
  }
}

void rem_tcl_coups(tcl_coups *list)
{
  int i, f;
  coupletinfo *cp;

  f = TCL_GLOBAL_ONLY | TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS;
  for (i = 0; list[i].name; i++) {
    cp = (coupletinfo *) Tcl_VarTraceInfo(interp, list[i].name, f,
                                          tcl_eggcouplet, NULL);
    strtot -= sizeof(coupletinfo);
    Tcl_UntraceVar(interp, list[i].name, f, tcl_eggcouplet, (ClientData) cp);
    nfree(cp);
  }
}

/* Check if the Tcl library supports threads
*/
int tcl_threaded()
{
#ifdef HAVE_TCL_GETCURRENTTHREAD
  if (Tcl_GetCurrentThread() != (Tcl_ThreadId)0)
    return 1;
#endif

  return 0;
}

/* Check if we need to fork before initializing Tcl
*/
int fork_before_tcl()
{
#ifndef REPLACE_NOTIFIER
  return tcl_threaded();
#endif
  return 0;
}
