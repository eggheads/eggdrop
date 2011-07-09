/*
 * modules.c -- handles:
 *   support for modules in eggdrop
 *
 * by Darrin Smith (beldin@light.iinet.net.au)
 *
 * $Id: modules.c,v 1.110 2011/07/09 15:07:48 thommey Exp $
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

#include <ctype.h>
#include "main.h"
#include "modules.h"
#include "tandem.h"
#include "md5/md5.h"
#include "users.h"

#ifndef STATIC
#  ifdef MOD_USE_SHL
#    include <dl.h>
#  endif
#  ifdef MOD_USE_DYLD
#    include <mach-o/dyld.h>
#    define DYLDFLAGS NSLINKMODULE_OPTION_BINDNOW|NSLINKMODULE_OPTION_PRIVATE|NSLINKMODULE_OPTION_RETURN_ON_ERROR
#  endif
#  ifdef MOD_USE_RLD
#    ifdef HAVE_MACH_O_RLD_H
#      include <mach-o/rld.h>
#    else
#      ifdef HAVE_RLD_H
#        indluce <rld.h>
#      endif
#    endif
#  endif
#  ifdef MOD_USE_LOADER
#    include <loader.h>
#  endif

#  ifdef MOD_USE_DL
#    ifdef DLOPEN_1
char *dlerror();
void *dlopen(const char *, int);
int dlclose(void *);
void *dlsym(void *, char *);
#      define DLFLAGS 1
#    else /* DLOPEN_1 */
#      include <dlfcn.h>

#      ifndef RTLD_GLOBAL
#        define RTLD_GLOBAL 0
#      endif
#      ifndef RTLD_NOW
#        define RTLD_NOW 1
#      endif
#      ifdef RTLD_LAZY
#        define DLFLAGS RTLD_LAZY|RTLD_GLOBAL
#      else
#        define DLFLAGS RTLD_NOW|RTLD_GLOBAL
#      endif
#    endif /* DLOPEN_1 */
#  endif /* MOD_USE_DL */
#endif /* !STATIC */



extern struct dcc_t *dcc;
extern struct userrec *userlist, *lastuser;
extern struct chanset_t *chanset;

extern char tempdir[], botnetnick[], botname[], natip[], hostname[],
            origbotname[], botuser[], admin[], userfile[], ver[], notify_new[],
            helpdir[], version[], quit_msg[], log_ts[];

extern int parties, noshare, dcc_total, egg_numver, userfile_perm, do_restart,
           ignore_time, must_be_owner, raw_log, max_dcc, make_userfile,
           default_flags, require_p, share_greet, use_invites, use_exempts,
           password_timeout, force_expire, protect_readonly, reserved_port_min,
           reserved_port_max, copy_to_tmp, quiet_reject;

extern party_t *party;
extern time_t now, online_since;
extern tand_t *tandbot;
extern Tcl_Interp *interp;
extern sock_list *socklist;

int cmd_die();
int xtra_kill();
int xtra_unpack();
static int module_rename(char *name, char *newname);

#ifndef STATIC
char moddir[121] = "modules/";
#endif

#ifdef STATIC
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
#endif /* STATIC */


/* The null functions */
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


/* The REAL hooks. When these are called, a return of 0 indicates unhandled;
 * 1 indicates handled. */
struct hook_entry *hook_list[REAL_HOOKS];

static void null_share(int idx, char *x)
{
  if ((x[0] == 'u') && (x[1] == 'n')) {
    putlog(LOG_BOTS, "*", "User file rejected by %s: %s", dcc[idx].nick, x + 3);
    dcc[idx].status &= ~STAT_OFFERED;
    if (!(dcc[idx].status & STAT_GETTING)) {
      dcc[idx].status &= ~STAT_SHARE;
    }
  } else if ((x[0] != 'v') && (x[0] != 'e')) {
    dprintf(idx, "s un Not sharing userfile.\n");
  }
}

void (*encrypt_pass) (char *, char *) = 0;
char *(*encrypt_string) (char *, char *) = 0;
char *(*decrypt_string) (char *, char *) = 0;
void (*shareout) () = null_func;
void (*sharein) (int, char *) = null_share;
void (*qserver) (int, char *, int) = (void (*)(int, char *, int)) null_func;
void (*add_mode) () = null_func;
int (*match_noterej) (struct userrec *, char *) =
    (int (*)(struct userrec *, char *)) false_func;
int (*rfc_casecmp) (const char *, const char *) = _rfc_casecmp;
int (*rfc_ncasecmp) (const char *, const char *, int) = _rfc_ncasecmp;
int (*rfc_toupper) (int) = _rfc_toupper;
int (*rfc_tolower) (int) = _rfc_tolower;
void (*dns_hostbyip) (IP) = block_dns_hostbyip;
void (*dns_ipbyhost) (char *) = block_dns_ipbyhost;

module_entry *module_list;
dependancy *dependancy_list = NULL;

/* The horrible global lookup table for functions
 * BUT it makes the whole thing *much* more portable than letting each
 * OS screw up the symbols their own special way :/
 */
Function global_table[] = {
  /* 0 - 3 */
  (Function) mod_malloc,
  (Function) mod_free,
#ifdef DEBUG_CONTEXT
  (Function) eggContext,
#else
  (Function) 0,
#endif
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
  (Function) egg_list_delete,
  (Function) egg_list_append,
  (Function) egg_list_contains,
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
  (Function) check_tcl_event,
  (Function) egg_memcpy,
  (Function) my_atoul,
  (Function) my_strcpy,
  /* 92 - 95 */
  (Function) & dcc,               /* struct dcc_t *                      */
  (Function) & chanset,           /* struct chanset_t *                  */
  (Function) & userlist,          /* struct userrec *                    */
  (Function) & lastuser,          /* struct userrec *                    */
  /* 96 - 99 */
  (Function) & global_bans,       /* struct banrec *                     */
  (Function) & global_ign,        /* struct igrec *                      */
  (Function) & password_timeout,  /* int                                 */
  (Function) & share_greet,       /* int                                 */
  /* 100 - 103 */
  (Function) & max_dcc,           /* int                                 */
  (Function) & require_p,         /* int                                 */
  (Function) & ignore_time,       /* int                                 */
  (Function) 0,                   /* was use_console_r <Wcc[02/02/03]>   */
  /* 104 - 107 */
  (Function) & reserved_port_min,
  (Function) & reserved_port_max,
  (Function) & raw_log,           /* int                                 */
  (Function) & noshare,           /* int                                 */
  /* 108 - 111 */
  (Function) 0,                   /* gban_total -- UNUSED! (Eule)        */
  (Function) & make_userfile,     /* int                                 */
  (Function) & default_flags,     /* int                                 */
  (Function) & dcc_total,         /* int                                 */
  /* 112 - 115 */
  (Function) tempdir,             /* char *                              */
  (Function) natip,               /* char *                              */
  (Function) hostname,            /* char *                              */
  (Function) origbotname,         /* char *                              */
  /* 116 - 119 */
  (Function) botuser,             /* char *                              */
  (Function) admin,               /* char *                              */
  (Function) userfile,            /* char *                              */
  (Function) ver,                 /* char *                              */
  /* 120 - 123 */
  (Function) notify_new,          /* char *                              */
  (Function) helpdir,             /* char *                              */
  (Function) version,             /* char *                              */
  (Function) botnetnick,          /* char *                              */
  /* 124 - 127 */
  (Function) & DCC_CHAT_PASS,     /* struct dcc_table *                  */
  (Function) & DCC_BOT,           /* struct dcc_table *                  */
  (Function) & DCC_LOST,          /* struct dcc_table *                  */
  (Function) & DCC_CHAT,          /* struct dcc_table *                  */
  /* 128 - 131 */
  (Function) & interp,            /* Tcl_Interp *                        */
  (Function) & now,               /* time_t                              */
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
  (Function) maskaddr,
  (Function) show_motd,
  /* 156 - 159 */
  (Function) tellhelp,
  (Function) showhelp,
  (Function) add_help_reference,
  (Function) rem_help_reference,
  /* 160 - 163 */
  (Function) touch_laston,
  (Function) & add_mode,          /* Function *                          */
  (Function) rmspace,
  (Function) in_chain,
  /* 164 - 167 */
  (Function) add_note,
  (Function) del_lang_section,
  (Function) detect_dcc_flood,
  (Function) flush_lines,
  /* 168 - 171 */
  (Function) expected_memory,
  (Function) tell_mem_status,
  (Function) & do_restart,        /* int                                 */
  (Function) check_tcl_filt,
  /* 172 - 175 */
  (Function) add_hook,
  (Function) del_hook,
  (Function) & H_dcc,             /* p_tcl_bind_list *                   */
  (Function) & H_filt,            /* p_tcl_bind_list *                   */
  /* 176 - 179 */
  (Function) & H_chon,            /* p_tcl_bind_list *                   */
  (Function) & H_chof,            /* p_tcl_bind_list *                   */
  (Function) & H_load,            /* p_tcl_bind_list *                   */
  (Function) & H_unld,            /* p_tcl_bind_list *                   */
  /* 180 - 183 */
  (Function) & H_chat,            /* p_tcl_bind_list *                   */
  (Function) & H_act,             /* p_tcl_bind_list *                   */
  (Function) & H_bcst,            /* p_tcl_bind_list *                   */
  (Function) & H_bot,             /* p_tcl_bind_list *                   */
  /* 184 - 187 */
  (Function) & H_link,            /* p_tcl_bind_list *                   */
  (Function) & H_disc,            /* p_tcl_bind_list *                   */
  (Function) & H_away,            /* p_tcl_bind_list *                   */
  (Function) & H_nkch,            /* p_tcl_bind_list *                   */
  /* 188 - 191 */
  (Function) & USERENTRY_BOTADDR, /* struct user_entry_type *            */
  (Function) & USERENTRY_BOTFL,   /* struct user_entry_type *            */
  (Function) & USERENTRY_HOSTS,   /* struct user_entry_type *            */
  (Function) & USERENTRY_PASS,    /* struct user_entry_type *            */
  /* 192 - 195 */
  (Function) & USERENTRY_XTRA,    /* struct user_entry_type *            */
  (Function) user_del_chan,
  (Function) & USERENTRY_INFO,    /* struct user_entry_type *            */
  (Function) & USERENTRY_COMMENT, /* struct user_entry_type *            */
  /* 196 - 199 */
  (Function) & USERENTRY_LASTON,  /* struct user_entry_type *            */
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
  (Function) & online_since,      /* time_t *                            */
  (Function) cmd_loadlanguage,
  (Function) check_dcc_attrs,
  /* 208 - 211 */
  (Function) check_dcc_chanattrs,
  (Function) add_tcl_coups,
  (Function) rem_tcl_coups,
  (Function) botname,
  /* 212 - 215 */
  (Function) 0,                   /* remove_gunk() -- UNUSED! (drummer)  */
  (Function) check_tcl_chjn,
  (Function) sanitycheck_dcc,
  (Function) isowner,
  /* 216 - 219 */
  (Function) 0,                   /* min_dcc_port -- UNUSED! (guppy)     */
  (Function) 0,                   /* max_dcc_port -- UNUSED! (guppy)     */
  (Function) & rfc_casecmp,       /* Function *                          */
  (Function) & rfc_ncasecmp,      /* Function *                          */
  /* 220 - 223 */
  (Function) & global_exempts,    /* struct exemptrec *                  */
  (Function) & global_invites,    /* struct inviterec *                  */
  (Function) 0,                   /* ginvite_total -- UNUSED! (Eule)     */
  (Function) 0,                   /* gexempt_total -- UNUSED! (Eule)     */
  /* 224 - 227 */
  (Function) & H_event,           /* p_tcl_bind_list *                   */
  (Function) & use_exempts,       /* int                                 */
  (Function) & use_invites,       /* int                                 */
  (Function) & force_expire,      /* int                                 */
  /* 228 - 231 */
  (Function) add_lang_section,
  (Function) _user_realloc,
  (Function) mod_realloc,
  (Function) xtra_set,
  /* 232 - 235 */
#ifdef DEBUG_CONTEXT
  (Function) eggContextNote,
#else
  (Function) 0,
#endif
#ifdef DEBUG_ASSERT
  (Function) eggAssert,
#else
  (Function) 0,
#endif
  (Function) allocsock,
  (Function) call_hostbyip,
  /* 236 - 239 */
  (Function) call_ipbyhost,
  (Function) iptostr,
  (Function) & DCC_DNSWAIT,       /* struct dcc_table *                  */
  (Function) hostsanitycheck_dcc,
  /* 240 - 243 */
  (Function) dcc_dnsipbyhost,
  (Function) dcc_dnshostbyip,
  (Function) changeover_dcc,
  (Function) make_rand_str,
  /* 244 - 247 */
  (Function) & protect_readonly,  /* int                                 */
  (Function) findchan_by_dname,
  (Function) removedcc,
  (Function) & userfile_perm,     /* int                                 */
  /* 248 - 251 */
  (Function) sock_has_data,
  (Function) bots_in_subtree,
  (Function) users_in_subtree,
  (Function) egg_inet_aton,
  /* 252 - 255 */
  (Function) egg_snprintf,
  (Function) egg_vsnprintf,
  (Function) egg_memset,
  (Function) egg_strcasecmp,
  /* 256 - 259 */
  (Function) egg_strncasecmp,
  (Function) is_file,
  (Function) & must_be_owner,     /* int                                 */
  (Function) & tandbot,           /* tand_t *                            */
  /* 260 - 263 */
  (Function) & party,             /* party_t *                           */
  (Function) open_address_listen,
  (Function) str_escape,
  (Function) strchr_unescape,
  /* 264 - 267 */
  (Function) str_unescape,
  (Function) egg_strcatn,
  (Function) clear_chanlist_member,
  (Function) fixfrom,
  /* 268 - 271 */
  (Function) & socklist,          /* sock_list *                         */
  (Function) sockoptions,
  (Function) flush_inbuf,
  (Function) kill_bot,
  /* 272 - 275 */
  (Function) quit_msg,            /* char *                              */
  (Function) module_load,
  (Function) module_unload,
  (Function) & parties,           /* int                                 */
  /* 276 - 279 */
  (Function) tell_bottree,
  (Function) MD5_Init,
  (Function) MD5_Update,
  (Function) MD5_Final,
  /* 280 - 283 */
  (Function) _wild_match_per,
  (Function) killtransfer,
  (Function) write_ignores,
  (Function) & copy_to_tmp,       /* int                                 */
  /* 284 - 287 */
  (Function) & quiet_reject,      /* int                                 */
  (Function) file_readable,
  (Function) 0,                   /* IPv6 leftovers: 286                 */
  (Function) 0,                   /* IPv6 leftovers: 287                 */
  /* 288 - 291 */
  (Function) 0,                   /* IPv6 leftovers: 288                 */
  (Function) strip_mirc_codes,
  (Function) check_ansi,
  (Function) oatoi,
  /* 292 - 295 */
  (Function) str_isdigit,
  (Function) remove_crlf,
  (Function) addr_match,
  (Function) mask_match,
  /* 296 - 299 */
  (Function) check_conflags,
  (Function) increase_socks_max,
  (Function) log_ts,
  (Function) mod_strdup
};

void init_modules(void)
{
  int i;

  module_list = nmalloc(sizeof(module_entry));
  module_list->name = nmalloc(8);
  strcpy(module_list->name, "eggdrop");
  module_list->major = (egg_numver) / 10000;
  module_list->minor = (egg_numver / 100) % 100;
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
  int c = 0, i;
  module_entry *p;
  dependancy *d;
  struct hook_entry *q;
  Function *f;
#ifdef STATIC
  struct static_list *s;

  for (s = static_modules; s; s = s->next)
    c += sizeof(struct static_list) + strlen(s->name) + 1;
#endif

  for (i = 0; i < REAL_HOOKS; i++)
    for (q = hook_list[i]; q; q = q->next)
      c += sizeof(struct hook_entry);

  for (d = dependancy_list; d; d = d->next)
    c += sizeof(dependancy);

  for (p = module_list; p; p = p->next) {
    c += sizeof(module_entry);
    c += strlen(p->name) + 1;
    f = p->funcs;
    if (f && f[MODCALL_EXPMEM] && !y)
      c += (int) (f[MODCALL_EXPMEM] ());
  }
  return c;
}

int module_register(char *name, Function *funcs, int major, int minor)
{
  module_entry *p;

  for (p = module_list; p && p->name; p = p->next) {
    if (!egg_strcasecmp(name, p->name)) {
      p->major = major;
      p->minor = minor;
      p->funcs = funcs;
      return 1;
    }
  }

  return 0;
}

const char *module_load(char *name)
{
  module_entry *p;
  char *e;
  Function f;
#ifdef STATIC
  struct static_list *sl;
#endif

#ifndef STATIC
  char workbuf[1024];
#  ifdef MOD_USE_SHL
  shl_t hand;
#  endif
#  ifdef MOD_USE_DYLD
  NSObjectFileImage file;
  NSObjectFileImageReturnCode ret;
  NSModule hand;
  NSSymbol sym;
#  endif
#  ifdef MOD_USE_RLD
  long ret;
#  endif
#  ifdef MOD_USE_LOADER
  ldr_module_t hand;
#  endif
#  ifdef MOD_USE_DL
  void *hand;
#  endif
#endif /* !STATIC */

  if (module_find(name, 0, 0) != NULL)
    return MOD_ALREADYLOAD;

#ifndef STATIC
  if (moddir[0] != '/') {
    if (getcwd(workbuf, 1024) == NULL)
      return MOD_BADCWD;
    sprintf(&(workbuf[strlen(workbuf)]), "/%s%s." EGG_MOD_EXT, moddir, name);
  } else {
    sprintf(workbuf, "%s%s." EGG_MOD_EXT, moddir, name);
  }

#  ifdef MOD_USE_SHL
  hand = shl_load(workbuf, BIND_IMMEDIATE, 0L);
  if (!hand)
    return "Can't load module.";
  sprintf(workbuf, "%s_start", name);
  if (shl_findsym(&hand, workbuf, (short) TYPE_PROCEDURE, (void *) &f))
    f = NULL;
  if (f == NULL) {
    /* Some OS's require a _ to be prepended to the symbol name (Darwin, etc). */
    sprintf(workbuf, "_%s_start", name);
    if (shl_findsym(&hand, workbuf, (short) TYPE_PROCEDURE, (void *) &f))
      f = NULL;
  }
  if (f == NULL) {
    shl_unload(hand);
    return MOD_NOSTARTDEF;
  }
#  endif /* MOD_USE_SHL */

#  ifdef MOD_USE_DYLD
  ret = NSCreateObjectFileImageFromFile(workbuf, &file);
  if (ret != NSObjectFileImageSuccess)
    return "Can't load module.";
  hand = NSLinkModule(file, workbuf, DYLDFLAGS);
  sprintf(workbuf, "_%s_start", name);
  sym = NSLookupSymbolInModule(hand, workbuf);
  if (sym)
    f = (Function) NSAddressOfSymbol(sym);
  else
    f = NULL;
  if (f == NULL) {
    NSUnLinkModule(hand, NSUNLINKMODULE_OPTION_NONE);
    return MOD_NOSTARTDEF;
  }
#  endif /* MOD_USE_DYLD */

#  ifdef MOD_USE_RLD
  ret = rld_load(NULL, (struct mach_header **) 0, workbuf, (const char *) 0);
  if (!ret)
    return "Can't load module.";
  sprintf(workbuf, "_%s_start", name);
  ret = rld_lookup(NULL, workbuf, &f)
  if (!ret || f == NULL)
    return MOD_NOSTARTDEF;
  /* There isn't a reliable way to unload at this point... just keep it loaded. */
#  endif /* MOD_USE_DYLD */

#  ifdef MOD_USE_LOADER
  hand = load(workbuf, LDR_NOFLAGS);
  if (hand == LDR_NULL_MODULE)
    return "Can't load module.";
  sprintf(workbuf, "%s_start", name);
  f = (Function) ldr_lookup_package(hand, workbuf);
  if (f == NULL) {
    sprintf(workbuf, "_%s_start", name);
    f = (Function) ldr_lookup_package(hand, workbuf);
  }
  if (f == NULL) {
    unload(hand);
    return MOD_NOSTARTDEF;
  }
#  endif /* MOD_USE_LOADER */

#  ifdef MOD_USE_DL
  hand = dlopen(workbuf, DLFLAGS);
  if (!hand)
    return dlerror();
  sprintf(workbuf, "%s_start", name);
  f = (Function) dlsym(hand, workbuf);
  if (f == NULL) {
    sprintf(workbuf, "_%s_start", name);
    f = (Function) dlsym(hand, workbuf);
  }
  if (f == NULL) {
    dlclose(hand);
    return MOD_NOSTARTDEF;
  }
#  endif /* MOD_USE_DL */
#endif /* !STATIC */

#ifdef STATIC
  for (sl = static_modules; sl && egg_strcasecmp(sl->name, name); sl = sl->next);
  if (!sl)
    return "Unknown module.";
  f = (Function) sl->func;
#endif /* STATIC */

  p = nmalloc(sizeof(module_entry));
  if (p == NULL)
    return "Malloc error";
  p->name = nmalloc(strlen(name) + 1);
  strcpy(p->name, name);
  p->major = 0;
  p->minor = 0;
#ifndef STATIC
  p->hand = hand;
#endif
  p->funcs = 0;
  p->next = module_list;
  module_list = p;
  e = (((char *(*)()) f) (global_table));
  if (e) {
    module_list = module_list->next;
    nfree(p->name);
    nfree(p);
    return e;
  }
  check_tcl_load(name);

  if (exist_lang_section(name))
    putlog(LOG_MISC, "*", MOD_LOADED_WITH_LANG, name);
  else
    putlog(LOG_MISC, "*", MOD_LOADED, name);

  return NULL;
}

char *module_unload(char *name, char *user)
{
  module_entry *p = module_list, *o = NULL;
  char *e;
  Function *f;

  while (p) {
    if ((p->name != NULL) && !strcmp(name, p->name)) {
      dependancy *d;

      for (d = dependancy_list; d; d = d->next)
        if (d->needed == p)
          return MOD_NEEDED;

      f = p->funcs;
      if (f && !f[MODCALL_CLOSE])
        return MOD_NOCLOSEDEF;
      if (f) {
        check_tcl_unld(name);
        e = (((char *(*)()) f[MODCALL_CLOSE]) (user));
        if (e != NULL)
          return e;
#ifndef STATIC
#  ifdef MOD_USE_SHL
        shl_unload(p->hand);
#  endif
#  ifdef MOD_USE_DYLD
        NSUnLinkModule(p->hand, NSUNLINKMODULE_OPTION_NONE);
#  endif
#  ifdef MOD_USE_LOADER
        unload(p->hand);
#  endif
#  ifdef MOD_USE_DL
        dlclose(p->hand);
#  endif
#endif /* !STATIC */
      }
      nfree(p->name);
      if (o == NULL)
        module_list = p->next;
      else
        o->next = p->next;
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
  module_entry *p;

  for (p = module_list; p && p->name; p = p->next) {
    if ((major == p->major || !major) && minor <= p->minor &&
        !egg_strcasecmp(name, p->name))
      return p;
  }
  return NULL;
}

static int module_rename(char *name, char *newname)
{
  module_entry *p;

  for (p = module_list; p; p = p->next)
    if (!egg_strcasecmp(newname, p->name))
      return 0;

  for (p = module_list; p && p->name; p = p->next) {
    if (!egg_strcasecmp(name, p->name)) {
      nfree(p->name);
      p->name = nmalloc(strlen(newname) + 1);
      strcpy(p->name, newname);
      return 1;
    }
  }
  return 0;
}

Function *module_depend(char *name1, char *name2, int major, int minor)
{
  module_entry *p = module_find(name2, major, minor);
  module_entry *o = module_find(name1, 0, 0);
  dependancy *d;

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
  return p->funcs ? p->funcs : (Function *) 1;
}

int module_undepend(char *name1)
{
  int ok = 0;
  module_entry *p = module_find(name1, 0, 0);
  dependancy *d = dependancy_list, *o = NULL;

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
  return ok;
}

char *mod_strdup(const char *s, const char *modname, const char *filename, int line)
{
#ifdef DEBUG_MEM
  char x[100], *p;

  p = strrchr(filename, '/');
  egg_snprintf(x, sizeof x, "%s:%s", modname, p ? p + 1 : filename);
  x[19] = 0;
  return n_strdup(s, x, line);
#else
  return nstrdup(s);
#endif
}

void *mod_malloc(int size, const char *modname, const char *filename, int line)
{
#ifdef DEBUG_MEM
  char x[100], *p;

  p = strrchr(filename, '/');
  egg_snprintf(x, sizeof x, "%s:%s", modname, p ? p + 1 : filename);
  x[19] = 0;
  return n_malloc(size, x, line);
#else
  return nmalloc(size);
#endif
}

void *mod_realloc(void *ptr, int size, const char *modname,
                  const char *filename, int line)
{
#ifdef DEBUG_MEM
  char x[100], *p;

  p = strrchr(filename, '/');
  egg_snprintf(x, sizeof x, "%s:%s", modname, p ? p + 1 : filename);
  x[19] = 0;
  return n_realloc(ptr, size, x, line);
#else
  return nrealloc(ptr, size);
#endif
}

void mod_free(void *ptr, const char *modname, const char *filename, int line)
{
  char x[100], *p;

  p = strrchr(filename, '/');
  egg_snprintf(x, sizeof x, "%s:%s", modname, p ? p + 1 : filename);
  x[19] = 0;
  n_free(ptr, x, line);
}

/* Hooks, various tables of functions to call on certain events
 */
void add_hook(int hook_num, Function func)
{
  if (hook_num < REAL_HOOKS) {
    struct hook_entry *p;

    for (p = hook_list[hook_num]; p; p = p->next)
      if (p->func == func)
        return;                 /* Don't add it if it's already there */
    p = nmalloc(sizeof(struct hook_entry));

    p->next = hook_list[hook_num];
    hook_list[hook_num] = p;
    p->func = func;
  } else
    switch (hook_num) {
    case HOOK_ENCRYPT_PASS:
      encrypt_pass = (void (*)(char *, char *)) func;
      break;
    case HOOK_ENCRYPT_STRING:
      encrypt_string = (char *(*)(char *, char *)) func;
      break;
    case HOOK_DECRYPT_STRING:
      decrypt_string = (char *(*)(char *, char *)) func;
      break;
    case HOOK_SHAREOUT:
      shareout = (void (*)()) func;
      break;
    case HOOK_SHAREIN:
      sharein = (void (*)(int, char *)) func;
      break;
    case HOOK_QSERV:
      if (qserver == (void (*)(int, char *, int)) null_func)
        qserver = (void (*)(int, char *, int)) func;
      break;
    case HOOK_ADD_MODE:
      if (add_mode == (void (*)()) null_func)
        add_mode = (void (*)()) func;
      break;
      /* special hook <drummer> */
    case HOOK_RFC_CASECMP:
      if (func == NULL) {
        rfc_casecmp = egg_strcasecmp;
        rfc_ncasecmp =
          (int (*)(const char *, const char *, int)) egg_strncasecmp;
        rfc_tolower = tolower;
        rfc_toupper = toupper;
      } else {
        rfc_casecmp = _rfc_casecmp;
        rfc_ncasecmp = _rfc_ncasecmp;
        rfc_tolower = _rfc_tolower;
        rfc_toupper = _rfc_toupper;
      }
      break;
    case HOOK_MATCH_NOTEREJ:
      if (match_noterej == false_func)
        match_noterej = (int (*)(struct userrec *, char *)) func;
      break;
    case HOOK_DNS_HOSTBYIP:
      if (dns_hostbyip == block_dns_hostbyip)
        dns_hostbyip = (void (*)(IP)) func;
      break;
    case HOOK_DNS_IPBYHOST:
      if (dns_ipbyhost == block_dns_ipbyhost)
        dns_ipbyhost = (void (*)(char *)) func;
      break;
    }
}

void del_hook(int hook_num, Function func)
{
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
      if (encrypt_pass == (void (*)(char *, char *)) func)
        encrypt_pass = (void (*)(char *, char *)) null_func;
      break;
    case HOOK_ENCRYPT_STRING:
      if (encrypt_string == (char *(*)(char *, char *)) func)
        encrypt_string = (char *(*)(char *, char *)) null_func;
      break;
    case HOOK_DECRYPT_STRING:
      if (decrypt_string == (char *(*)(char *, char *)) func)
        decrypt_string = (char *(*)(char *, char *)) null_func;
      break;
    case HOOK_SHAREOUT:
      if (shareout == (void (*)()) func)
        shareout = null_func;
      break;
    case HOOK_SHAREIN:
      if (sharein == (void (*)(int, char *)) func)
        sharein = null_share;
      break;
    case HOOK_QSERV:
      if (qserver == (void (*)(int, char *, int)) func)
        qserver = null_func;
      break;
    case HOOK_ADD_MODE:
      if (add_mode == (void (*)()) func)
        add_mode = null_func;
      break;
    case HOOK_MATCH_NOTEREJ:
      if (match_noterej == (int (*)(struct userrec *, char *)) func)
        match_noterej = false_func;
      break;
    case HOOK_DNS_HOSTBYIP:
      if (dns_hostbyip == (void (*)(IP)) func)
        dns_hostbyip = block_dns_hostbyip;
      break;
    case HOOK_DNS_IPBYHOST:
      if (dns_ipbyhost == (void (*)(char *)) func)
        dns_ipbyhost = block_dns_ipbyhost;
      break;
    }
}

int call_hook_cccc(int hooknum, char *a, char *b, char *c, char *d)
{
  struct hook_entry *p, *pn;
  int f = 0;

  if (hooknum >= REAL_HOOKS)
    return 0;
  p = hook_list[hooknum];
  for (p = hook_list[hooknum]; p && !f; p = pn) {
    pn = p->next;
    f = p->func(a, b, c, d);
  }
  return f;
}

void do_module_report(int idx, int details, char *which)
{
  module_entry *p = module_list;

  if (p && !which)
    dprintf(idx, "Loaded module information:\n");
  for (; p; p = p->next) {
    if (!which || !egg_strcasecmp(which, p->name)) {
      dependancy *d;

      if (details)
        dprintf(idx, "  Module: %s, v %d.%d\n", p->name ? p->name : "CORE",
                p->major, p->minor);
      if (details > 1) {
        for (d = dependancy_list; d; d = d->next)
          if (d->needing == p)
            dprintf(idx, "    requires: %s, v %d.%d\n", d->needed->name,
                    d->major, d->minor);
      }
      if (p->funcs) {
        Function f = p->funcs[MODCALL_REPORT];

        if (f != NULL)
          f(idx, details);
      }
      if (which)
        return;
    }
  }
  if (which)
    dprintf(idx, "No such module.\n");
}
