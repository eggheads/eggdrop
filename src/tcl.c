/*
 * tcl.c -- handles:
 *   the code for every command eggdrop adds to Tcl
 *   Tcl initialization
 *   getting and setting Tcl/eggdrop variables
 *
 * $Id: tcl.c,v 1.49 2002/11/02 00:23:21 wcc Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999, 2000, 2001, 2002 Eggheads Development Team
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

#include <stdlib.h>		/* getenv()				*/
#include <locale.h>		/* setlocale()				*/
#include "main.h"

/* Used for read/write to internal strings */
typedef struct {
  char *str;			/* Pointer to actual string in eggdrop	     */
  int max;			/* max length (negative: read-only var
				   when protect is on) (0: read-only ALWAYS) */
  int flags;			/* 1 = directory			     */
} strinfo;

typedef struct {
  int *var;
  int ro;
} intinfo;


extern time_t	online_since;
extern int	backgrd, flood_telnet_thr, flood_telnet_time;
extern int	shtime, share_greet, require_p, keep_all_logs;
extern int	allow_new_telnets, stealth_telnets, use_telnet_banner;
extern int	default_flags, conmask, switch_logfiles_at, connect_timeout;
extern int	firewallport, notify_users_at, flood_thr, ignore_time;
extern int	reserved_port_min, reserved_port_max;
extern char	origbotname[], botuser[], motdfile[], admin[], userfile[],
		firewall[], helpdir[], notify_new[], hostname[], myip[],
		moddir[], tempdir[], owner[], network[], botnetnick[],
		bannerfile[], egg_version[], natip[], configfile[],
		logfile_suffix[], textdir[], pid_file[];
extern int	die_on_sighup, die_on_sigterm, max_logs, max_logsize,
		enable_simul, dcc_total, debug_output, identtimeout,
		protect_telnet, dupwait_timeout, egg_numver, share_unlinks,
		dcc_sanitycheck, sort_users, tands, resolve_timeout,
		default_uflags, strict_host, userfile_perm;
extern struct dcc_t	*dcc;
extern tcl_timer_t	*timer, *utimer;

int	    protect_readonly = 0;	/* turn on/off readonly protection */
char	    whois_fields[1025] = "";	/* fields to display in a .whois */
Tcl_Interp *interp;			/* eggdrop always uses the same
					   interpreter */
int	    dcc_flood_thr = 3;
int	    use_invites = 0;		/* Jason/drummer */
int	    use_exempts = 0;		/* Jason/drummer */
int	    force_expire = 0;		/* Rufus */
int	    remote_boots = 2;
int	    allow_dk_cmds = 1;
int	    must_be_owner = 1;
int	    max_dcc = 20;		/* needs at least 4 or 5 just to
					   get started. 20 should be enough   */
int	    quick_logs = 0;		/* quick write logs? (flush them
					   every min instead of every 5	      */
int	    par_telnet_flood = 1;       /* trigger telnet flood for +f
					   ppl? - dw			      */
int	    quiet_save = 0;             /* quiet-save patch by Lucas	      */
int	    strtot = 0;
int 	    handlen = HANDLEN;
int	    utftot = 0;
int	    clientdata_stuff = 0;


/* Prototypes for tcl */
Tcl_Interp *Tcl_CreateInterp();

int expmem_tcl()
{
  return strtot + utftot + clientdata_stuff;
}

int findidx(int z)
{
  int j;

  for (j = 0; j < dcc_total; j++)
    if ((dcc[j].sock == z) && (dcc[j].type->flags & DCT_VALIDIDX))
      return j;
  return -1;
}

static void botnet_change(char *new)
{
  if (egg_strcasecmp(botnetnick, new)) {
    /* Trying to change bot's nickname */
    if (tands > 0) {
      putlog(LOG_MISC, "*", "* Tried to change my botnet nick, but I'm still linked to a botnet.");
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

int init_dcc_max(), init_misc();

/* Used for read/write to integer couplets */
typedef struct {
  int *left;			/* left side of couplet */
  int *right;			/* right side */
} coupletinfo;

/* Read/write integer couplets (int1:int2) */
#if ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4))
static char *tcl_eggcouplet(ClientData cdata, Tcl_Interp *irp, char *name1,
                            CONST char *name2, int flags)
#else
static char *tcl_eggcouplet(ClientData cdata, Tcl_Interp *irp, char *name1,
			    char *name2, int flags)
#endif
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
  } else {			/* writes */
    s = (char *) Tcl_GetVar2(interp, name1, name2, TCL_GLOBAL_ONLY);
    if (s != NULL) {
      int nr1, nr2;

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
#if ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4))
static char *tcl_eggint(ClientData cdata, Tcl_Interp *irp, char *name1,
			CONST char *name2, int flags)
#else
static char *tcl_eggint(ClientData cdata, Tcl_Interp *irp, char *name1,
                        char *name2, int flags)
#endif
{
  char *s, s1[40];
  long l;
  intinfo *ii = (intinfo *) cdata;

  if (flags & (TCL_TRACE_READS | TCL_TRACE_UNSETS)) {
    /* Special cases */
    if ((int *) ii->var == &conmask)
      strcpy(s1, masktype(conmask));
    else if ((int *) ii->var == &default_flags) {
      struct flag_record fr = {FR_GLOBAL, 0, 0, 0, 0, 0};
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
  } else {			/* Writes */
    s = (char *) Tcl_GetVar2(interp, name1, name2, TCL_GLOBAL_ONLY);
    if (s != NULL) {
      if ((int *) ii->var == &conmask) {
	if (s[0])
	  conmask = logmodes(s);
	else
	  conmask = LOG_MODES | LOG_MISC | LOG_CMDS;
      } else if ((int *) ii->var == &default_flags) {
	struct flag_record fr = {FR_GLOBAL, 0, 0, 0, 0, 0};

	break_down_flags(s, &fr, 0);
	default_flags = sanity_check(fr.global); /* drummer */
	default_uflags = fr.udef_global;
      } else if ((int *) ii->var == &userfile_perm) {
	int p = oatoi(s);

	if (p <= 0)
	  return "invalid userfile permissions";
	userfile_perm = p;
      } else if ((ii->ro == 2) || ((ii->ro == 1) && protect_readonly)) {
	return "read-only variable";
      } else {
	if (Tcl_ExprLong(interp, s, &l) == TCL_ERROR)
	  return interp->result;
	if ((int *) ii->var == &max_dcc) {
	  if (l < max_dcc)
	    return "you can't DECREASE max-dcc";
	  max_dcc = l;
	  init_dcc_max();
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
#if ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4))
static char *tcl_eggstr(ClientData cdata, Tcl_Interp *irp, char *name1,
                        CONST char *name2, int flags)
#else
static char *tcl_eggstr(ClientData cdata, Tcl_Interp *irp, char *name1,
			char *name2, int flags)
#endif
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
	return "read-only variable"; /* it won't return the error... */
    }
    return NULL;
  } else {			/* writes */
    if ((st->max <= 0) && (protect_readonly || (st->max == 0))) {
      Tcl_SetVar2(interp, name1, name2, st->str, TCL_GLOBAL_ONLY);
      return "read-only variable";
    }
    s = (char *) Tcl_GetVar2(interp, name1, name2, TCL_GLOBAL_ONLY);
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
    }
    return NULL;
  }
}

/* Add/remove tcl commands
 */

#if ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 1)) || (TCL_MAJOR_VERSION > 8)

static int utf_converter(ClientData cdata, Tcl_Interp *myinterp, int objc,
			 Tcl_Obj *CONST objv[])
{
  char **strings, *byteptr;
  int i, len, retval, diff;
  void **callback_data;
  Function func;
  ClientData cd;

  objc += 5;
  strings = (char **)nmalloc(sizeof(char *) * objc);
  egg_memset(strings, 0, sizeof(char *) * objc);
  diff = utftot;
  utftot += sizeof(char *) * objc;
  objc -= 5;
  for (i = 0; i < objc; i++) {
    byteptr = (char *)Tcl_GetByteArrayFromObj(objv[i], &len);
    strings[i] = (char *)nmalloc(len+1);
    utftot += len+1;
    strncpy(strings[i], byteptr, len);
    strings[i][len] = 0;
  }
  callback_data = (void **)cdata;
  func = (Function) callback_data[0];
  cd = (ClientData) callback_data[1];
  diff -= utftot;
  retval = func(cd, myinterp, objc, strings);
  for (i = 0; i < objc; i++) nfree(strings[i]);
  nfree(strings);
  utftot += diff;
  return(retval);
}

void cmd_delete_callback(ClientData cdata)
{
  nfree(cdata);
  clientdata_stuff -= sizeof(void *) * 2;
}

void add_tcl_commands(tcl_cmds *table)
{
  void **cdata;

  while (table->name) {
    cdata = (void **)nmalloc(sizeof(void *) * 2);
    clientdata_stuff += sizeof(void *) * 2;
    cdata[0] = table->func;
    cdata[1] = NULL;
    Tcl_CreateObjCommand(interp, table->name, utf_converter, (ClientData) cdata,
			 cmd_delete_callback);
    table++;
  }
}

void add_cd_tcl_cmds(cd_tcl_cmd *table)
{
  void **cdata;

  while (table->name) {
    cdata = (void **)nmalloc(sizeof(void *) * 2);
    clientdata_stuff += sizeof(void *) * 2;
    cdata[0] = table->callback;
    cdata[1] = table->cdata;
    Tcl_CreateObjCommand(interp, table->name, utf_converter, (ClientData) cdata, 
			 cmd_delete_callback);
    table++;
  }
}

#else

void add_tcl_commands(tcl_cmds *table)
{
  int i;

  for (i = 0; table[i].name; i++)
    Tcl_CreateCommand(interp, table[i].name, table[i].func, NULL, NULL);
}

void add_cd_tcl_cmds(cd_tcl_cmd *table)
{
  while (table->name) {
    Tcl_CreateCommand(interp, table->name, table->callback, 
		      (ClientData) table->cdata, NULL);
    table++;
  }
}

#endif

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

void add_tcl_objcommands(tcl_cmds *table)
{
#if (TCL_MAJOR_VERSION >= 8)
  int i;

  for (i = 0; table[i].name; i++)
    Tcl_CreateObjCommand(interp, table[i].name, table[i].func, (ClientData) 0, NULL);
#endif
}

/* Strings */
static tcl_strings def_tcl_strings[] =
{
  {"botnet-nick",	botnetnick,	HANDLEN,	0},
  {"userfile",		userfile,	120,		STR_PROTECT},
  {"motd",		motdfile,	120,		STR_PROTECT},
  {"admin",		admin,		120,		0},
  {"help-path",		helpdir,	120,		STR_DIR | STR_PROTECT},
  {"temp-path",		tempdir,	120,		STR_DIR | STR_PROTECT},
  {"text-path",		textdir,	120,		STR_DIR | STR_PROTECT},
#ifndef STATIC
  {"mod-path",		moddir,		120,		STR_DIR | STR_PROTECT},
#endif
  {"notify-newusers",	notify_new,	120,		0},
  {"owner",		owner,		120,		STR_PROTECT},
  {"my-hostname",	hostname,	120,		0},
  {"my-ip",		myip,		120,		0},
  {"network",		network,	40,		0},
  {"whois-fields",	whois_fields,	1024,		0},
  {"nat-ip",		natip,		120,		0},
  {"username",		botuser,	10,		0},
  {"version",		egg_version,	0,		0},
  {"firewall",		firewall,	120,		0},
/* confvar patch by aaronwl */
  {"config",		configfile,	0,		0},
  {"telnet-banner",	bannerfile,	120,		STR_PROTECT},
  {"logfile-suffix",	logfile_suffix,	20,		0},
  {"pidfile",		pid_file,       120,		STR_PROTECT},
  {NULL,		NULL,		0,		0}
};

/* Ints */
static tcl_ints def_tcl_ints[] =
{
  {"ignore-time",		&ignore_time,		0},
  {"handlen",			&handlen,		2},
  {"dcc-flood-thr",		&dcc_flood_thr,		0},
  {"hourly-updates",		&notify_users_at,	0},
  {"switch-logfiles-at",	&switch_logfiles_at,	0},
  {"connect-timeout",		&connect_timeout,	0},
  {"reserved-port",		&reserved_port_min,		0},
  /* booleans (really just ints) */
  {"require-p",			&require_p,		0},
  {"keep-all-logs",		&keep_all_logs,		0},
  {"open-telnets",		&allow_new_telnets,	0},
  {"stealth-telnets",		&stealth_telnets,	0},
  {"use-telnet-banner",		&use_telnet_banner,	0},
  {"uptime",			(int *) &online_since,	2},
  {"console",			&conmask,		0},
  {"default-flags",		&default_flags,		0},
  /* moved from eggdrop.h */
  {"numversion",		&egg_numver,		2},
  {"die-on-sighup",		&die_on_sighup,		1},
  {"die-on-sigterm",		&die_on_sigterm,	1},
  {"remote-boots",		&remote_boots,		1},
  {"max-dcc",			&max_dcc,		0},
  {"max-logs",			&max_logs,		0},
  {"max-logsize",		&max_logsize,		0},
  {"quick-logs",		&quick_logs,		0},
  {"enable-simul",		&enable_simul,		1},
  {"debug-output",		&debug_output,		1},
  {"protect-telnet",		&protect_telnet,	0},
  {"dcc-sanitycheck",		&dcc_sanitycheck,	0},
  {"sort-users",		&sort_users,		0},
  {"ident-timeout",		&identtimeout,		0},
  {"share-unlinks",		&share_unlinks,		0},
  {"log-time",			&shtime,		0},
  {"allow-dk-cmds",		&allow_dk_cmds,		0},
  {"resolve-timeout",		&resolve_timeout,	0},
  {"must-be-owner",		&must_be_owner,		1},
  {"paranoid-telnet-flood",	&par_telnet_flood,	0},
  {"use-exempts",		&use_exempts,		0},			/* Jason/drummer */
  {"use-invites",		&use_invites,		0},			/* Jason/drummer */
  {"quiet-save",		&quiet_save,		0},			/* Lucas */
  {"force-expire",		&force_expire,		0},			/* Rufus */
  {"dupwait-timeout",		&dupwait_timeout,	0},
  {"strict-host",		&strict_host,		0}, 			/* drummer */
  {"userfile-perm",		&userfile_perm,		0},
  {NULL,			NULL,			0}	/* arthur2 */
};

static tcl_coups def_tcl_coups[] =
{
  {"telnet-flood",	&flood_telnet_thr,	&flood_telnet_time},
  {"reserved-portrange", &reserved_port_min, &reserved_port_max},
  {NULL,		NULL,			NULL}
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

void kill_tcl()
{
  rem_tcl_coups(def_tcl_coups);
  rem_tcl_strings(def_tcl_strings);
  rem_tcl_ints(def_tcl_ints);
  kill_bind();
  Tcl_DeleteInterp(interp);
}

extern tcl_cmds tcluser_cmds[], tcldcc_cmds[], tclmisc_cmds[], tclmisc_objcmds[], tcldns_cmds[];

/* Not going through Tcl's crazy main() system (what on earth was he
 * smoking?!) so we gotta initialize the Tcl interpreter
 */
void init_tcl(int argc, char **argv)
{
#if (TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 1) || (TCL_MAJOR_VERSION >= 9)
  const char *encoding;
  int i;
  char *langEnv;
#endif
#ifndef HAVE_PRE7_5_TCL
  int j;
  char pver[1024] = "";
#endif

/* This must be done *BEFORE* Tcl_SetSystemEncoding(),
 * or Tcl_SetSystemEncoding() will cause a segfault.
 */
#ifndef HAVE_PRE7_5_TCL
  /* This is used for 'info nameofexecutable'.
   * The filename in argv[0] must exist in a directory listed in
   * the environment variable PATH for it to register anything.
   */
  Tcl_FindExecutable(argv[0]);
#endif

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

/* Code based on Tcl's TclpSetInitialEncodings() */
#if (TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 1) || (TCL_MAJOR_VERSION >= 9)
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
#endif

#ifndef HAVE_PRE7_5_TCL
  /* Add eggdrop to Tcl's package list */
  for (j = 0; j <= strlen(egg_version); j++) {
    if ((egg_version[j] == ' ') || (egg_version[j] == '+'))
      break;
    pver[strlen(pver)] = egg_version[j];
  }
  Tcl_PkgProvide(interp, "eggdrop", pver);
#endif

  /* Initialize binds and traces */
  init_bind();
  init_traces();

  /* Add new commands */
  add_tcl_commands(tcluser_cmds);
  add_tcl_commands(tcldcc_cmds);
  add_tcl_commands(tclmisc_cmds);
  add_tcl_objcommands(tclmisc_objcmds);
  add_tcl_commands(tcldns_cmds);
}

void do_tcl(char *whatzit, char *script)
{
  int code;

  code = Tcl_Eval(interp, script);
  if (code != TCL_OK) {
    putlog(LOG_MISC, "*", "Tcl error in script for '%s':", whatzit);
    putlog(LOG_MISC, "*", "%s", interp->result);
  }
}

/* Interpret tcl file fname.
 *
 * returns:   1 - if everything was okay
 */
int readtclprog(char *fname)
{
  FILE	*f;

  /* Check whether file is readable. */
  if ((f = fopen(fname, "r")) == NULL)
    return 0;
  fclose(f);

  if (Tcl_EvalFile(interp, fname) != TCL_OK) {
    putlog(LOG_MISC, "*", "Tcl error in file '%s':", fname);
    putlog(LOG_MISC, "*", "%s",
	   Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY));
    return 0;
  }

  /* Refresh internal variables */
  return 1;
}

void add_tcl_strings(tcl_strings *list)
{
  int i;
  strinfo *st;
  int tmp;

  for (i = 0; list[i].name; i++) {
    st = (strinfo *) nmalloc(sizeof(strinfo));
    strtot += sizeof(strinfo);
    st->max = list[i].length - (list[i].flags & STR_DIR);
    if (list[i].flags & STR_PROTECT)
      st->max = -st->max;
    st->str = list[i].buf;
    st->flags = (list[i].flags & STR_DIR);
    tmp = protect_readonly;
    protect_readonly = 0;
    tcl_eggstr((ClientData) st, interp, list[i].name, NULL, TCL_TRACE_WRITES);
    tcl_eggstr((ClientData) st, interp, list[i].name, NULL, TCL_TRACE_READS);
    Tcl_TraceVar(interp, list[i].name, TCL_TRACE_READS | TCL_TRACE_WRITES |
		 TCL_TRACE_UNSETS, tcl_eggstr, (ClientData) st);
  }
}

void rem_tcl_strings(tcl_strings *list)
{
  int i;
  strinfo *st;

  for (i = 0; list[i].name; i++) {
    st = (strinfo *) Tcl_VarTraceInfo(interp, list[i].name,
				      TCL_TRACE_READS |
				      TCL_TRACE_WRITES |
				      TCL_TRACE_UNSETS,
				      tcl_eggstr, NULL);
    Tcl_UntraceVar(interp, list[i].name,
		   TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		   tcl_eggstr, st);
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
    ii = nmalloc(sizeof(intinfo));
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
  int i;
  intinfo *ii;

  for (i = 0; list[i].name; i++) {
    ii = (intinfo *) Tcl_VarTraceInfo(interp, list[i].name,
				      TCL_TRACE_READS |
				      TCL_TRACE_WRITES |
				      TCL_TRACE_UNSETS,
				      tcl_eggint, NULL);
    Tcl_UntraceVar(interp, list[i].name,
		   TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		   tcl_eggint, (ClientData) ii);
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
    cp = (coupletinfo *) nmalloc(sizeof(coupletinfo));
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

void rem_tcl_coups(tcl_coups * list)
{
  coupletinfo *cp;
  int i;

  for (i = 0; list[i].name; i++) {
    cp = (coupletinfo *) Tcl_VarTraceInfo(interp, list[i].name,
					  TCL_TRACE_READS |
					  TCL_TRACE_WRITES |
					  TCL_TRACE_UNSETS,
					  tcl_eggcouplet, NULL);
    strtot -= sizeof(coupletinfo);
    Tcl_UntraceVar(interp, list[i].name,
		   TCL_TRACE_READS | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
		   tcl_eggcouplet, (ClientData) cp);
    nfree(cp);
  }
}
