/*
 * modules.c - support for code modules in eggdrop
 * by Darrin Smith (beldin@light.iinet.net.au)
 */
/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

#include "main.h"
#include "modules.h"
#include "tandem.h"
#include <ctype.h>
#ifndef STATIC
#ifdef HPUX_HACKS
#include <dl.h>
#else
#ifdef OSF1_HACKS
#include <loader.h>
#else
#ifdef DLOPEN_1
char *dlerror();
void *dlopen(const char *, int);
int dlclose(void *);
void *dlsym(void *, char *);

#define DLFLAGS 1
#else
#include <dlfcn.h>
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif
#ifndef RTLD_NOW
#define RTLD_NOW 1
#endif
#ifdef RTLD_LAZY
#define DLFLAGS RTLD_LAZY|RTLD_GLOBAL
#else
#define DLFLAGS RTLD_NOW|RTLD_GLOBAL
#endif
#endif				/* DLOPEN_1 */
#endif				/* OSF1_HACKS */
#endif				/* HPUX_HACKS */

#endif				/* STATIC */
extern struct dcc_t *dcc;

#include "users.h"

/* from other areas */
extern Tcl_Interp *interp;
extern struct userrec *userlist, *lastuser;
extern char tempdir[], botnetnick[], botname[], natip[], hostname[];
extern char origbotname[], botuser[], admin[], userfile[], ver[], notify_new[];
extern char helpdir[], version[];
extern int reserved_port, noshare, dcc_total, egg_numver, use_silence;
extern int use_console_r, ignore_time, debug_output, gban_total, make_userfile;
extern int gexempt_total, ginvite_total;
extern int default_flags, require_p, max_dcc, share_greet, password_timeout;
extern int min_dcc_port, max_dcc_port;	/* dw */
extern int use_invites, use_exempts; /* Jason/drummer */
extern int force_expire; /* Rufus */
extern int do_restart;
extern time_t now, online_since;
extern struct chanset_t *chanset;
extern int protect_readonly;
int cmd_die(), xtra_kill(), xtra_unpack();
static int module_rename(char *name, char *newname);

#ifndef STATIC
/* directory to look for modules */
char moddir[121] = "modules/";

#else
struct static_list {
  struct static_list *next;
  char *name;
  char *(*func) ();
} *static_modules = NULL;

void check_static(char *name, char *(*func) ())
{
  struct static_list *p = nmalloc(sizeof(struct static_list));

  p->name = nmalloc(strlen(name) + 1);
  strcpy(p->name, name);
  p->func = func;
  p->next = static_modules;
  static_modules = p;
}

#endif

/* the null functions */
void null_func()
{
}
char *charp_func()
{
  return NULL;
}
int minus_func()
{
  return -1;
}
int false_func()
{
  return 0;
}

/* various hooks & things */
/* the REAL hooks, when these are called, a return of 0 indicates unhandled
 * 1 is handled */
struct hook_entry *hook_list[REAL_HOOKS];

static void null_share(int idx, char *x)
{
  if ((x[0] == 'u') && (x[1] == 'n')) {
    putlog(LOG_BOTS, "*", "User file rejected by %s: %s",
	   dcc[idx].nick, x + 3);
    dcc[idx].status &= ~STAT_OFFERED;
    if (!(dcc[idx].status & STAT_GETTING)) {
      dcc[idx].status &= ~STAT_SHARE;
    }
  } else if ((x[0] != 'v') && (x[0] != 'e'))
    dprintf(idx, "s un Not sharing userfile.\n");
}

/* these are obscure ones that I hope to neaten eventually :/ */
void (*encrypt_pass) (char *, char *) = 0;
void (*shareout) () = null_func;
void (*sharein) (int, char *) = null_share;
void (*qserver) (int, char *, int) = null_func;
void (*add_mode) () = null_func;
int (*match_noterej) (struct userrec*, char *) = false_func;
int (*rfc_casecmp) (const char *, const char *) = _rfc_casecmp;
int (*rfc_ncasecmp) (const char *, const char *, int) = _rfc_ncasecmp;
int (*rfc_toupper) (int) = _rfc_toupper;
int (*rfc_tolower) (int) = _rfc_tolower;

module_entry *module_list;
dependancy *dependancy_list = NULL;

void mod_context(char *module, char *file, int line)
{
  char x[100];

  sprintf(x, "%s:%s", module, file);
  x[30] = 0;
  cx_ptr = ((cx_ptr + 1) & 15);
  strcpy(cx_file[cx_ptr], x);
  cx_line[cx_ptr] = line;
}

void mod_contextnote(char *module, char *file, int line, char *note)
{
  cx_ptr=((cx_ptr + 1) & 15);
#ifdef HAVE_SNPRINTF
  snprintf(cx_file[cx_ptr], 30, "%s:%s", module, file);
#else
  sprintf(cx_file[cx_ptr], "%s:%s", module, file);
#endif
  cx_line[cx_ptr] = line;
  strncpy(cx_note[cx_ptr], note, 255);
  cx_note[cx_ptr][255] = 0;
}

/* the horrible global lookup table for functions
 * BUT it makes the whole thing *much* more portable than letting each
 * OS screw up the symbols their own special way :/
 */

Function global_table[] =
{
  /* 0 - 3 */
  (Function) mod_malloc,
  (Function) mod_free,
  (Function) mod_context,
  (Function) module_rename,
  /* 4 - 7 */
  (Function) module_register,
  (Function) module_find,
  (Function) module_depend,
  (Function) module_undepend,
  /* 8 - 11 */
  (Function) add_bind_table,
  (Function) del_bind_table,
  (Function) find_bind_table,
  (Function) check_tcl_bind,
  /* 12 - 15 */
  (Function) add_builtins,
  (Function) rem_builtins,
  (Function) add_tcl_commands,
  (Function) rem_tcl_commands,
  /* 16 - 19 */
  (Function) add_tcl_ints,
  (Function) rem_tcl_ints,
  (Function) add_tcl_strings,
  (Function) rem_tcl_strings,
  /* 20 - 23 */
  (Function) base64_to_int,
  (Function) int_to_base64,
  (Function) int_to_base10,
  (Function) simple_sprintf,
  /* 24 - 27 */
  (Function) botnet_send_zapf,
  (Function) botnet_send_zapf_broad,
  (Function) botnet_send_unlinked,
  (Function) botnet_send_bye,
  /* 28 - 31 */
  (Function) botnet_send_chat,
  (Function) botnet_send_filereject,
  (Function) botnet_send_filesend,
  (Function) botnet_send_filereq,
  /* 32 - 35 */
  (Function) botnet_send_join_idx,
  (Function) botnet_send_part_idx,
  (Function) updatebot,
  (Function) nextbot,
  /* 36 - 39 */
  (Function) zapfbot,
  (Function) n_free,
  (Function) u_pass_match,
  (Function) _user_malloc,
  /* 40 - 43 */
  (Function) get_user,
  (Function) set_user,
  (Function) add_entry_type,
  (Function) del_entry_type,
  /* 44 - 47 */
  (Function) get_user_flagrec,
  (Function) set_user_flagrec,
  (Function) get_user_by_host,
  (Function) get_user_by_handle,
  /* 48 - 51 */
  (Function) find_entry_type,
  (Function) find_user_entry,
  (Function) adduser,
  (Function) deluser,
  /* 52 - 55 */
  (Function) addhost_by_handle,
  (Function) delhost_by_handle,
  (Function) readuserfile,
  (Function) write_userfile,
  /* 56 - 59 */
  (Function) geticon,
  (Function) clear_chanlist,
  (Function) reaffirm_owners,
  (Function) change_handle,
  /* 60 - 63 */
  (Function) write_user,
  (Function) clear_userlist,
  (Function) count_users,
  (Function) sanity_check,
  /* 64 - 67 */
  (Function) break_down_flags,
  (Function) build_flags,
  (Function) flagrec_eq,
  (Function) flagrec_ok,
  /* 68 - 71 */
  (Function) & shareout,
  (Function) dprintf,
  (Function) chatout,
  (Function) chanout_but,
  /* 72 - 75 */
  (Function) check_validity,
  (Function) list_delete,
  (Function) list_append,
  (Function) list_contains,
  /* 76 - 79 */
  (Function) answer,
  (Function) getmyip,
  (Function) neterror,
  (Function) tputs,
  /* 80 - 83 */
  (Function) new_dcc,
  (Function) lostdcc,
  (Function) getsock,
  (Function) killsock,
  /* 84 - 87 */
  (Function) open_listen,
  (Function) open_telnet_dcc,
  (Function) _get_data_ptr,
  (Function) open_telnet,
  /* 88 - 91 */
#ifdef HAVE_BZERO
  (Function) null_func,
#else
  (Function) bzero,
#endif
  (Function) my_memcpy,
  (Function) my_atoul,
  (Function) my_strcpy,
  /* 92 - 95 */
  (Function) & dcc,		/* struct dcc_t * */
  (Function) & chanset,		/* struct chanset_t * */
  (Function) & userlist,	/* struct userrec * */
  (Function) & lastuser,	/* struct userrec * */
  /* 96 - 99 */
  (Function) & global_bans,	/* struct banrec * */
  (Function) & global_ign,	/* struct igrec * */
  (Function) & password_timeout,	/* int */
  (Function) & share_greet,	/* int */
  /* 100 - 103 */
  (Function) & max_dcc,		/* int */
  (Function) & require_p,	/* int */
  (Function) & use_silence,	/* int */
  (Function) & use_console_r,	/* int */
  /* 104 - 107 */
  (Function) & ignore_time,	/* int */
  (Function) & reserved_port,	/* int */
  (Function) & debug_output,	/* int */
  (Function) & noshare,		/* int */
  /* 108 - 111 */
  (Function) & gban_total,	/* int */
  (Function) & make_userfile,	/* int */
  (Function) & default_flags,	/* int */
  (Function) & dcc_total,	/* int */
  /* 112 - 115 */
  (Function) tempdir,		/* char * */
  (Function) natip,		/* char * */
  (Function) hostname,		/* char * */
  (Function) origbotname,	/* char * */
  /* 116 - 119 */
  (Function) botuser,		/* char * */
  (Function) admin,		/* char * */
  (Function) userfile,		/* char * */
  (Function) ver,		/* char * */
     /* 120 - 123 */
  (Function) notify_new,	/* char * */
  (Function) helpdir,		/* char * */
  (Function) version,		/* char * */
  (Function) botnetnick,	/* char * */
  /* 124 - 127 */
  (Function) & DCC_CHAT_PASS,	/* struct dcc_table * */
  (Function) & DCC_BOT,		/* struct dcc_table * */
  (Function) & DCC_LOST,	/* struct dcc_table * */
  (Function) & DCC_CHAT,	/* struct dcc_table * */
  /* 128 - 131 */
  (Function) & interp,		/* Tcl_Interp * */
  (Function) & now,		/* time_t */
  (Function) findanyidx,
  (Function) findchan,
  /* 132 - 135 */
  (Function) cmd_die,
  (Function) days,
  (Function) daysago,
  (Function) daysdur,
  /* 136 - 139 */
  (Function) ismember,
  (Function) newsplit,
  (Function) splitnick,
  (Function) splitc,
  /* 140 - 143 */
  (Function) addignore,
  (Function) match_ignore,
  (Function) delignore,
  (Function) fatal,
  /* 144 - 147 */
  (Function) xtra_kill,
  (Function) xtra_unpack,
  (Function) movefile,
  (Function) copyfile,
  /* 148 - 151 */
  (Function) do_tcl,
  (Function) readtclprog,
  (Function) get_language,
  (Function) def_get,
  /* 152 - 155 */
  (Function) makepass,
  (Function) _wild_match,
  (Function) maskhost,
  (Function) show_motd,
  /* 156 - 159 */
  (Function) tellhelp,
  (Function) showhelp,
  (Function) add_help_reference,
  (Function) rem_help_reference,
  /* 160 - 163 */
  (Function) touch_laston,
  (Function) & add_mode,
  (Function) rmspace,
  (Function) in_chain,
  /* 164 - 167 */
  (Function) add_note,
  (Function) NULL,
  (Function) detect_dcc_flood,
  (Function) flush_lines,
  /* 168 - 171 */
  (Function) expected_memory,
  (Function) tell_mem_status,
  (Function) & do_restart,
  (Function) check_tcl_filt,
  /* 172 - 175 */
  (Function) add_hook,
  (Function) del_hook,
  (Function) & H_dcc,
  (Function) & H_filt,
  /* 176 - 179 */
  (Function) & H_chon,
  (Function) & H_chof,
  (Function) & H_load,
  (Function) & H_unld,
  /* 180 - 183 */
  (Function) & H_chat,
  (Function) & H_act,
  (Function) & H_bcst,
  (Function) & H_bot,
  /* 184 - 187 */
  (Function) & H_link,
  (Function) & H_disc,
  (Function) & H_away,
  (Function) & H_nkch,
  /* 188 - 191 */
  (Function) & USERENTRY_BOTADDR,
  (Function) & USERENTRY_BOTFL,
  (Function) & USERENTRY_HOSTS,
  (Function) & USERENTRY_PASS,
  /* 192 - 195 */
  (Function) & USERENTRY_XTRA,
  (Function) 0,
  (Function) & USERENTRY_INFO,
  (Function) & USERENTRY_COMMENT,
  /* 196 - 199 */
  (Function) & USERENTRY_LASTON,
  (Function) putlog,
  (Function) botnet_send_chan,
  (Function) list_type_kill,
  /* 200 - 203 */
  (Function) logmodes,
  (Function) masktype,
  (Function) stripmodes,
  (Function) stripmasktype,
  /* 204 - 207 */
  (Function) sub_lang,
  (Function) & online_since,
  (Function) cmd_loadlanguage,
  (Function) check_dcc_attrs,
  /* 208 - 211 */
  (Function) check_dcc_chanattrs,
  (Function) add_tcl_coups,
  (Function) rem_tcl_coups,
  (Function) botname,
  /* 212 - 215 */
  (Function) remove_gunk,
  (Function) check_tcl_chjn,
  (Function) sanitycheck_dcc,
  (Function) isowner,		/* Daemus */
  /* 216 - 219 */
  (Function) & min_dcc_port,	/* dw */
  (Function) & max_dcc_port,
  (Function) & rfc_casecmp,	/* Function * */
  (Function) & rfc_ncasecmp,	/* Function * */
  /* 220 - 223 */
  (Function) & global_exempts,	/* struct exemptrec * */
  (Function) & global_invites,	/* struct inviterec * */
  (Function) & gexempt_total,	/* int */
  (Function) & ginvite_total,	/* int */
  /* 224 - 227 */
  (Function) & H_event,
  (Function) & use_exempts,	/* int - drummer/Jason */
  (Function) & use_invites,	/* int - drummer/Jason */
  (Function) & force_expire,	/* int - Rufus */
  /* 228 - 231 */
  (Function) add_lang_section,
  (Function) _user_realloc,
  (Function) mod_realloc,
  (Function) xtra_set,
  /* 232 - 235 */
  (Function) mod_contextnote,
  (Function) assert_failed,
  (Function) & protect_readonly, /* int */
};

void init_modules(void)
{
  int i;

  context;
  module_list = nmalloc(sizeof(module_entry));
  module_list->name = nmalloc(8);
  strcpy(module_list->name, "eggdrop");
  module_list->major = (egg_numver) / 10000;
  module_list->minor = ((egg_numver) / 100) % 100;
#ifndef STATIC
  module_list->hand = NULL;
#endif
  module_list->next = NULL;
  module_list->funcs = NULL;
  for (i = 0; i < REAL_HOOKS; i++)
    hook_list[i] = NULL;
}

int expmem_modules(int y)
{
  int c = 0;
  int i;
  module_entry *p = module_list;
  dependancy *d = dependancy_list;

#ifdef STATIC
  struct static_list *s;

#endif
  Function *f;

  context;
#ifdef STATIC
  for (s = static_modules; s; s = s->next)
    c += sizeof(struct static_list) + strlen(s->name) + 1;

#endif
  for (i = 0; i < REAL_HOOKS; i++) {
    struct hook_entry *q = hook_list[i];

    while (q) {
      c += sizeof(struct hook_entry);

      q = q->next;
    }
  }
  while (d) {
    c += sizeof(dependancy);
    d = d->next;
  }
  while (p) {
    c += sizeof(module_entry);
    c += strlen(p->name) + 1;
    f = p->funcs;
    if (f && f[MODCALL_EXPMEM] && !y)
      c += (int) (f[MODCALL_EXPMEM] ());
    p = p->next;
  }
  return c;
}

int module_register(char *name, Function * funcs,
		    int major, int minor)
{
  module_entry *p = module_list;

  context;
  while (p) {
    if (p->name && !strcasecmp(name, p->name)) {
      p->major = major;
      p->minor = minor;
      p->funcs = funcs;
      return 1;
    }
    p = p->next;
  }
  return 0;
}

const char *module_load(char *name)
{
  module_entry *p;
  char *e;
  Function f;

#ifndef STATIC
  char workbuf[1024];

#ifdef HPUX_HACKS
  shl_t hand;

#else
#ifdef OSF1_HACKS
  ldr_module_t hand;

#else
  void *hand;

#endif
#endif
#else
  struct static_list *sl;

#endif

  context;
  if (module_find(name, 0, 0) != NULL)
    return MOD_ALREADYLOAD;
#ifndef STATIC
  context;
  if (moddir[0] != '/') {
    if (getcwd(workbuf, 1024) == NULL)
      return MOD_BADCWD;
    sprintf(&(workbuf[strlen(workbuf)]), "/%s%s.so", moddir, name);
  } else
    sprintf(workbuf, "%s%s.so", moddir, name);
#ifdef HPUX_HACKS
  hand = shl_load(workbuf, BIND_IMMEDIATE, 0L);
  context;
  if (!hand)
    return "Can't load module.";
#else
#ifdef OSF1_HACKS
#ifndef HAVE_OLD_TCL
  hand = (Tcl_PackageInitProc *) load(workbuf, LDR_NOFLAGS);
  if (hand == LDR_NULL_MODULE)
    return "Can't load module.";
#endif
#else
  context;
  hand = dlopen(workbuf, DLFLAGS);
  if (!hand)
    return dlerror();
#endif
#endif

  sprintf(workbuf, "%s_start", name);
#ifdef HPUX_HACKS
  context;
  if (shl_findsym(&hand, workbuf, (short) TYPE_PROCEDURE, (void *) &f))
    f = NULL;
#else
#ifdef OSF1_HACKS
  f = ldr_lookup_package(hand, workbuf);
#else
  f = dlsym(hand, workbuf);
#endif
#endif
  if (f == NULL) {		/* some OS's need the _ */
    sprintf(workbuf, "_%s_start", name);
#ifdef HPUX_HACKS
    if (shl_findsym(&hand, workbuf, (short) TYPE_PROCEDURE, (void *) &f))
      f = NULL;
#else
#ifdef OSF1_HACKS
    f = ldr_lookup_package(hand, workbuf);
#else
    f = dlsym(hand, workbuf);
#endif
#endif
    if (f == NULL) {
#ifdef HPUX_HACKS
      shl_unload(hand);
#else
#ifdef OSF1_HACKS
#else
      dlclose(hand);
#endif
#endif
      return MOD_NOSTARTDEF;
    }
  }
#else
  for (sl = static_modules; sl && strcasecmp(sl->name, name); sl = sl->next);
  context;
  if (!sl)
    return "Unkown module.";
  f = (Function) sl->func;
#endif
  p = nmalloc(sizeof(module_entry));
  if (p == NULL)
    return "Malloc error";
  p->name = nmalloc(strlen(name) + 1);
  strcpy(p->name, name);
  context;
  p->major = 0;
  p->minor = 0;
#ifndef STATIC
  p->hand = hand;
#endif
  p->funcs = 0;
  p->next = module_list;
  module_list = p;
  e = (((char *(*)()) f) (global_table));
  context;
  if (e) {
    module_list = module_list->next;
    nfree(p->name);
    nfree(p);
    return e;
  }
  check_tcl_load(name);
  putlog(LOG_MISC, "*", "%s %s", MOD_LOADED, name);
  context;
  return NULL;
}

char *module_unload(char *name, char *user)
{
  module_entry *p = module_list, *o = NULL;
  char *e;
  Function *f;

  context;
  while (p) {
    if ((p->name != NULL) && (!strcmp(name, p->name))) {
      dependancy *d = dependancy_list;

      while (d != NULL) {
	if (d->needed == p) {
	  return MOD_NEEDED;
	}
	d = d->next;
      }
      f = p->funcs;
      if (f && !f[MODCALL_CLOSE])
	return MOD_NOCLOSEDEF;
      if (f) {
	check_tcl_unld(name);
	e = (((char *(*)()) f[MODCALL_CLOSE]) (user));
	if (e != NULL)
	  return e;
#ifndef STATIC
#ifdef HPUX_HACKS
	shl_unload(p->hand);
#else
#ifdef OSF1_HACKS
#else
	dlclose(p->hand);
#endif
#endif
#endif				/* STATIC */
      }
      nfree(p->name);
      if (o == NULL) {
	module_list = p->next;
      } else {
	o->next = p->next;
      }
      nfree(p);
      putlog(LOG_MISC, "*", "%s %s", MOD_UNLOADED, name);
      return NULL;
    }
    o = p;
    p = p->next;
  }
  return MOD_NOSUCH;
}

module_entry *module_find(char *name, int major, int minor)
{
  module_entry *p = module_list;

  while (p) {
    if (p->name && !strcasecmp(name, p->name) &&
	((major == p->major) || (major == 0)) &&
	(minor <= p->minor))
      return p;
    p = p->next;
  }
  return NULL;
}

static int module_rename(char *name, char *newname)
{
  module_entry *p = module_list;

  while (p) {
    if (!strcasecmp(newname, p->name))
      return 0;
    p = p->next;
  }
  p = module_list;
  while (p) {
    if (p->name && !strcasecmp(name, p->name)) {
      nfree(p->name);
      p->name = nmalloc(strlen(newname) + 1);
      strcpy(p->name, newname);
      return 1;
    }
    p = p->next;
  }
  return 0;
}

Function *module_depend(char *name1, char *name2, int major, int minor)
{
  module_entry *p = module_find(name2, major, minor);
  module_entry *o = module_find(name1, 0, 0);
  dependancy *d;

  context;
  if (!p) {
    if (module_load(name2))
      return 0;
    p = module_find(name2, major, minor);
  }
  if (!p || !o)
    return 0;
  d = nmalloc(sizeof(dependancy));

  d->needed = p;
  d->needing = o;
  d->next = dependancy_list;
  d->major = major;
  d->minor = minor;
  dependancy_list = d;
  context;
  return p->funcs ? p->funcs : (Function *) 1;
}

int module_undepend(char *name1)
{
  int ok = 0;
  module_entry *p = module_find(name1, 0, 0);
  dependancy *d = dependancy_list, *o = NULL;

  context;
  if (p == NULL)
    return 0;
  while (d != NULL) {
    if (d->needing == p) {
      if (o == NULL) {
	dependancy_list = d->next;
      } else {
	o->next = d->next;
      }
      nfree(d);
      if (o == NULL)
	d = dependancy_list;
      else
	d = o->next;
      ok++;
    } else {
      o = d;
      d = d->next;
    }
  }
  context;
  return ok;
}

void *mod_malloc(int size, char *modname, char *filename, int line)
{
  char x[100];

  sprintf(x, "%s:%s", modname, filename);
  x[19] = 0;
  return n_malloc(size, x, line);
}

void *mod_realloc(void *ptr, int size, char *modname, char *filename, int line)
{
  char x[100];

  sprintf(x, "%s:%s", modname, filename);
  x[19] = 0;
  return n_realloc(ptr, size, x, line);
}

void mod_free(void *ptr, char *modname, char *filename, int line)
{
  char x[100];

  sprintf(x, "%s:%s", modname, filename);
  x[19] = 0;
  n_free(ptr, x, line);
}

/* hooks, various tables of functions to call on ceratin events */
void add_hook(int hook_num, void *func)
{
  context;
  if (hook_num < REAL_HOOKS) {
    struct hook_entry *p;

    for (p = hook_list[hook_num]; p; p = p->next)
      if (p->func == func)
	return;			/* dont add it if it's already there */
    p = nmalloc(sizeof(struct hook_entry));

    p->next = hook_list[hook_num];
    hook_list[hook_num] = p;
    p->func = func;
  } else
    switch (hook_num) {
    case HOOK_ENCRYPT_PASS:
      encrypt_pass = func;
      break;
    case HOOK_SHAREOUT:
      shareout = func;
      break;
    case HOOK_SHAREIN:
      sharein = func;
      break;
    case HOOK_QSERV:
      if (qserver == null_func)
	qserver = func;
      break;
    case HOOK_ADD_MODE:
      if (add_mode == null_func)
	add_mode = func;
      break;
    /* special hook <drummer> */
    case HOOK_RFC_CASECMP:
      if (func == 0) {
	rfc_casecmp = (void *) strcasecmp;
	rfc_ncasecmp = (void *) strncasecmp;
	rfc_tolower = (void *) tolower;
	rfc_toupper = (void *) toupper;
      } else {
	rfc_casecmp = _rfc_casecmp;
	rfc_ncasecmp = _rfc_ncasecmp;
	rfc_tolower = _rfc_tolower;
	rfc_toupper = _rfc_toupper;
      }
      break;
    case HOOK_MATCH_NOTEREJ:
      if (match_noterej == false_func)
	match_noterej = func;
      break;
    }
}

void del_hook(int hook_num, void *func)
{
  context;
  if (hook_num < REAL_HOOKS) {
    struct hook_entry *p = hook_list[hook_num], *o = NULL;

    while (p) {
      if (p->func == func) {
	if (o == NULL)
	  hook_list[hook_num] = p->next;
	else
	  o->next = p->next;
	nfree(p);
	break;
      }
      o = p;
      p = p->next;
    }
  } else
    switch (hook_num) {
    case HOOK_ENCRYPT_PASS:
      if (encrypt_pass == func)
	encrypt_pass = null_func;
      break;
    case HOOK_SHAREOUT:
      if (shareout == func)
	shareout = null_func;
      break;
    case HOOK_SHAREIN:
      if (sharein == func)
	sharein = null_share;
      break;
    case HOOK_QSERV:
      if (qserver == func)
	qserver = null_func;
      break;
    case HOOK_ADD_MODE:
      if (add_mode == func)
	add_mode = null_func;
      break;
    case HOOK_MATCH_NOTEREJ:
      if (match_noterej == func)
	match_noterej = false_func;
      break;
    }
}

int call_hook_cccc(int hooknum, char *a, char *b, char *c, char *d)
{
  struct hook_entry *p;
  int f = 0;

  if (hooknum >= REAL_HOOKS)
    return 0;
  p = hook_list[hooknum];
  context;
  while ((p != NULL) && !f) {
    f = p->func(a, b, c, d);
    p = p->next;
  }
  return f;
}

void do_module_report(int idx, int details, char *which)
{
  module_entry *p = module_list;

  if (p && !which && details)
    dprintf(idx, "MODULES LOADED:\n");
  while (p) {
    if (!which || !strcasecmp(which, p->name)) {
      dependancy *d = dependancy_list;

      if (details)
	dprintf(idx, "Module: %s, v %d.%d\n", p->name ? p->name : "CORE",
		p->major, p->minor);
      if (details > 1) {
	while (d != NULL) {
	  if (d->needing == p)
	    dprintf(idx, "    requires: %s, v %d.%d\n", d->needed->name,
		    d->major, d->minor);
	  d = d->next;
	}
      }
      if (p->funcs) {
	Function f = p->funcs[MODCALL_REPORT];

	if (f != NULL)
	  f(idx, details);
      }
      if (which)
	return;
    }
    p = p->next;
  }
  if (which)
    dprintf(idx, "No such module.\n");
}
