/*
 * tclhash.c -- handles:
 *   bind and unbind
 *   checking and triggering the various in-bot bindings
 *   listing current bindings
 *   adding/removing new binding tables
 *   (non-Tcl) procedure lookups for msg/dcc/file commands
 *   (Tcl) binding internal procedures to msg/dcc/file commands
 *
 * $Id: tclhash.c,v 1.72 2011/07/09 15:07:48 thommey Exp $
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
#include "chan.h"
#include "users.h"

extern Tcl_Interp *interp;
extern struct dcc_t *dcc;
extern struct userrec *userlist;
extern int dcc_total;
extern time_t now;

p_tcl_bind_list bind_table_list;
p_tcl_bind_list H_chat, H_act, H_bcst, H_chon, H_chof, H_load, H_unld, H_link,
                H_disc, H_dcc, H_chjn, H_chpt, H_bot, H_time, H_nkch, H_away,
                H_note, H_filt, H_event, H_cron, H_log = NULL;

static int builtin_2char();
static int builtin_3char();
static int builtin_5int();
static int builtin_cron();
static int builtin_char();
static int builtin_chpt();
static int builtin_chjn();
static int builtin_idxchar();
static int builtin_charidx();
static int builtin_chat();
static int builtin_dcc();
static int builtin_log();


/* Allocate and initialise a chunk of memory.
 */
static inline void *n_malloc_null(int size, const char *file, int line)
{
#ifdef DEBUG_MEM
#  define nmalloc_null(size) n_malloc_null(size, __FILE__, __LINE__)
  void *ptr = n_malloc(size, file, line);
#else
#  define nmalloc_null(size) n_malloc_null(size, NULL, 0)
  void *ptr = nmalloc(size);
#endif

  egg_memset(ptr, 0, size);
  return ptr;
}


/* Delete trigger/command.
 */
static inline void tcl_cmd_delete(tcl_cmd_t *tc)
{
  nfree(tc->func_name);
  nfree(tc);
}

/* Delete bind and its elements.
 */
static inline void tcl_bind_mask_delete(tcl_bind_mask_t *tm)
{
  tcl_cmd_t *tc, *tc_next;

  for (tc = tm->first; tc; tc = tc_next) {
    tc_next = tc->next;
    tcl_cmd_delete(tc);
  }
  nfree(tm->mask);
  nfree(tm);
}

/* Delete bind list and its elements.
 */
static inline void tcl_bind_list_delete(tcl_bind_list_t *tl)
{
  tcl_bind_mask_t *tm, *tm_next;

  for (tm = tl->first; tm; tm = tm_next) {
    tm_next = tm->next;
    tcl_bind_mask_delete(tm);
  }
  nfree(tl);
}

inline void garbage_collect_tclhash(void)
{
  tcl_bind_list_t *tl, *tl_next, *tl_prev;
  tcl_bind_mask_t *tm, *tm_next, *tm_prev;
  tcl_cmd_t *tc, *tc_next, *tc_prev;

  for (tl = bind_table_list, tl_prev = NULL; tl; tl = tl_next) {
    tl_next = tl->next;

    if (tl->flags & HT_DELETED) {
      if (tl_prev)
        tl_prev->next = tl->next;
      else
        bind_table_list = tl->next;
      tcl_bind_list_delete(tl);
    } else {
      for (tm = tl->first, tm_prev = NULL; tm; tm = tm_next) {
        tm_next = tm->next;

        if (!(tm->flags & TBM_DELETED)) {
          for (tc = tm->first, tc_prev = NULL; tc; tc = tc_next) {
            tc_next = tc->next;

            if (tc->attributes & TC_DELETED) {
              if (tc_prev)
                tc_prev->next = tc->next;
              else
                tm->first = tc->next;
              tcl_cmd_delete(tc);
            } else
              tc_prev = tc;
          }
        }

        /* Delete the bind when it's marked as deleted or when it's empty. */
        if ((tm->flags & TBM_DELETED) || tm->first == NULL) {
          if (tm_prev)
            tm_prev->next = tm->next;
          else
            tl->first = tm_next;
          tcl_bind_mask_delete(tm);
        } else
          tm_prev = tm;
      }
      tl_prev = tl;
    }
  }
}

static inline int tcl_cmd_expmem(tcl_cmd_t *tc)
{
  int tot;

  tot = sizeof(*tc);
  if (tc->func_name)
    tot += strlen(tc->func_name) + 1;
  return tot;
}

static inline int tcl_bind_mask_expmem(tcl_bind_mask_t *tm)
{
  int tot = 0;
  tcl_cmd_t *tc;

  for (tc = tm->first; tc; tc = tc->next)
    tot += tcl_cmd_expmem(tc);
  if (tm->mask)
    tot += strlen(tm->mask) + 1;
  tot += sizeof(*tm);
  return tot;
}

static inline int tcl_bind_list_expmem(tcl_bind_list_t *tl)
{
  int tot = 0;
  tcl_bind_mask_t *tm;

  for (tm = tl->first; tm; tm = tm->next)
    tot += tcl_bind_mask_expmem(tm);
  tot += sizeof(*tl);
  return tot;
}

int expmem_tclhash(void)
{
  int tot = 0;
  tcl_bind_list_t *tl;

  for (tl = bind_table_list; tl; tl = tl->next)
    tot += tcl_bind_list_expmem(tl);
  return tot;
}


extern cmd_t C_dcc[];
static int tcl_bind();

static cd_tcl_cmd cd_cmd_table[] = {
  {"bind",   tcl_bind, (void *) 0},
  {"unbind", tcl_bind, (void *) 1},
  {0}
};

void init_bind(void)
{
  bind_table_list = NULL;
  Context;
  add_cd_tcl_cmds(cd_cmd_table);
  H_unld = add_bind_table("unld", HT_STACKABLE, builtin_char);
  H_time = add_bind_table("time", HT_STACKABLE, builtin_5int);
  H_cron = add_bind_table("cron", HT_STACKABLE, builtin_cron);
  H_note = add_bind_table("note", 0, builtin_3char);
  H_nkch = add_bind_table("nkch", HT_STACKABLE, builtin_2char);
  H_load = add_bind_table("load", HT_STACKABLE, builtin_char);
  H_link = add_bind_table("link", HT_STACKABLE, builtin_2char);
  H_filt = add_bind_table("filt", HT_STACKABLE, builtin_idxchar);
  H_disc = add_bind_table("disc", HT_STACKABLE, builtin_char);
  H_dcc = add_bind_table("dcc", 0, builtin_dcc);
  H_chpt = add_bind_table("chpt", HT_STACKABLE, builtin_chpt);
  H_chon = add_bind_table("chon", HT_STACKABLE, builtin_charidx);
  H_chof = add_bind_table("chof", HT_STACKABLE, builtin_charidx);
  H_chjn = add_bind_table("chjn", HT_STACKABLE, builtin_chjn);
  H_chat = add_bind_table("chat", HT_STACKABLE, builtin_chat);
  H_bot = add_bind_table("bot", 0, builtin_3char);
  H_bcst = add_bind_table("bcst", HT_STACKABLE, builtin_chat);
  H_away = add_bind_table("away", HT_STACKABLE, builtin_chat);
  H_act = add_bind_table("act", HT_STACKABLE, builtin_chat);
  H_event = add_bind_table("evnt", HT_STACKABLE, builtin_char);
  H_log = add_bind_table("log", HT_STACKABLE, builtin_log);
  add_builtins(H_dcc, C_dcc);
  Context;
}

void kill_bind(void)
{
  tcl_bind_list_t *tl, *tl_next;

  rem_builtins(H_dcc, C_dcc);
  for (tl = bind_table_list; tl; tl = tl_next) {
    tl_next = tl->next;

    if (!(tl->flags |= HT_DELETED))
      putlog(LOG_DEBUG, "*", "De-Allocated bind table %s", tl->name);
    tcl_bind_list_delete(tl);
  }
  H_log = NULL;
  bind_table_list = NULL;
}

tcl_bind_list_t *add_bind_table(const char *nme, int flg, IntFunc func)
{
  tcl_bind_list_t *tl, *tl_prev;
  int v;

  /* Do not allow coders to use bind table names longer than
   * 4 characters. */
  Assert(strlen(nme) <= 4);

  for (tl = bind_table_list, tl_prev = NULL; tl; tl_prev = tl, tl = tl->next) {
    if (tl->flags & HT_DELETED)
      continue;
    v = egg_strcasecmp(tl->name, nme);
    if (!v)
      return tl;                /* Duplicate, just return old value.    */
    if (v > 0)
      break;                    /* New. Insert at start of list.        */
  }

  tl = nmalloc_null(sizeof *tl);
  strcpy(tl->name, nme);
  tl->flags = flg;
  tl->func = func;

  if (tl_prev) {
    tl->next = tl_prev->next;
    tl_prev->next = tl;
  } else {
    tl->next = bind_table_list;
    bind_table_list = tl;
  }

  putlog(LOG_DEBUG, "*", "Allocated bind table %s (flags %d)", nme, flg);
  return tl;
}

void del_bind_table(tcl_bind_list_t *tl_which)
{
  tcl_bind_list_t *tl;

  for (tl = bind_table_list; tl; tl = tl->next) {
    if (tl->flags & HT_DELETED)
      continue;
    if (tl == tl_which) {
      tl->flags |= HT_DELETED;
      putlog(LOG_DEBUG, "*", "De-Allocated bind table %s", tl->name);
      return;
    }
  }
  putlog(LOG_DEBUG, "*", "??? Tried to delete not listed bind table ???");
}

tcl_bind_list_t *find_bind_table(const char *nme)
{
  tcl_bind_list_t *tl;
  int v;

  for (tl = bind_table_list; tl; tl = tl->next) {
    if (tl->flags & HT_DELETED)
      continue;
    v = egg_strcasecmp(tl->name, nme);
    if (!v)
      return tl;
    if (v > 0)
      return NULL;
  }
  return NULL;
}

static void dump_bind_tables(Tcl_Interp *irp)
{
  tcl_bind_list_t *tl;
  u_8bit_t i;

  for (tl = bind_table_list, i = 0; tl; tl = tl->next) {
    if (tl->flags & HT_DELETED)
      continue;
    if (i)
      Tcl_AppendResult(irp, ", ", NULL);
    else
      i = 1;
    Tcl_AppendResult(irp, tl->name, NULL);
  }
}

static int unbind_bind_entry(tcl_bind_list_t *tl, const char *flags,
                             const char *cmd, const char *proc)
{
  tcl_bind_mask_t *tm;

  /* Search for matching bind in bind list. */
  for (tm = tl->first; tm; tm = tm->next) {
    if (tm->flags & TBM_DELETED)
      continue;
    if (!strcmp(cmd, tm->mask))
      break;                    /* Found it! fall out! */
  }

  if (tm) {
    tcl_cmd_t *tc;

    /* Search for matching proc in bind. */
    for (tc = tm->first; tc; tc = tc->next) {
      if (tc->attributes & TC_DELETED)
        continue;
      if (!egg_strcasecmp(tc->func_name, proc)) {
        /* Erase proc regardless of flags. */
        tc->attributes |= TC_DELETED;
        return 1;               /* Match.       */
      }
    }
  }
  return 0;                     /* No match.    */
}

/* Add command (remove old one if necessary)
 */
static int bind_bind_entry(tcl_bind_list_t *tl, const char *flags,
                           const char *cmd, const char *proc)
{
  tcl_cmd_t *tc;
  tcl_bind_mask_t *tm;

  /* Search for matching bind in bind list. */
  for (tm = tl->first; tm; tm = tm->next) {
    if (tm->flags & TBM_DELETED)
      continue;
    if (!strcmp(cmd, tm->mask))
      break;                    /* Found it! fall out! */
  }

  /* Create bind if it doesn't exist yet. */
  if (!tm) {
    tm = nmalloc_null(sizeof *tm);
    tm->mask = nmalloc(strlen(cmd) + 1);
    strcpy(tm->mask, cmd);

    /* Link into linked list of binds. */
    tm->next = tl->first;
    tl->first = tm;
  }

  /* Proc already defined? If so, replace. */
  for (tc = tm->first; tc; tc = tc->next) {
    if (tc->attributes & TC_DELETED)
      continue;
    if (!egg_strcasecmp(tc->func_name, proc)) {
      tc->flags.match = FR_GLOBAL | FR_CHAN;
      break_down_flags(flags, &(tc->flags), NULL);
      return 1;
    }
  }

  /* If this bind list is not stackable, remove the
   * old entry from this bind. */
  if (!(tl->flags & HT_STACKABLE)) {
    for (tc = tm->first; tc; tc = tc->next) {
      if (tc->attributes & TC_DELETED)
        continue;
      /* NOTE: We assume there's only one not-yet-deleted entry. */
      tc->attributes |= TC_DELETED;
      break;
    }
  }

  tc = nmalloc_null(sizeof *tc);
  tc->flags.match = FR_GLOBAL | FR_CHAN;
  break_down_flags(flags, &(tc->flags), NULL);
  tc->func_name = nmalloc(strlen(proc) + 1);
  strcpy(tc->func_name, proc);

  /* Link into linked list of the bind's command list. */
  tc->next = tm->first;
  tm->first = tc;

  return 1;
}

static int tcl_getbinds(tcl_bind_list_t *tl_kind, const char *name)
{
  tcl_bind_mask_t *tm;

  for (tm = tl_kind->first; tm; tm = tm->next) {
    if (tm->flags & TBM_DELETED)
      continue;
    if (!egg_strcasecmp(tm->mask, name)) {
      tcl_cmd_t *tc;

      for (tc = tm->first; tc; tc = tc->next) {
        if (tc->attributes & TC_DELETED)
          continue;
        Tcl_AppendElement(interp, tc->func_name);
      }
      break;
    }
  }
  return TCL_OK;
}

static int tcl_bind STDVAR
{
  tcl_bind_list_t *tl;

  /* cd defines what tcl_bind is supposed do: 0 = bind, 1 = unbind. */
  if ((long int) cd == 1)
    BADARGS(5, 5, " type flags cmd/mask procname");

  else
    BADARGS(4, 5, " type flags cmd/mask ?procname?");

  tl = find_bind_table(argv[1]);
  if (!tl) {
    Tcl_AppendResult(irp, "bad type, should be one of: ", NULL);
    dump_bind_tables(irp);
    return TCL_ERROR;
  }
  if ((long int) cd == 1) {
    if (!unbind_bind_entry(tl, argv[2], argv[3], argv[4])) {
      /* Don't error if trying to re-unbind a builtin */
      if (argv[4][0] != '*' || argv[4][4] != ':' ||
          strcmp(argv[3], &argv[4][5]) || strncmp(argv[1], &argv[4][1], 3)) {
        Tcl_AppendResult(irp, "no such binding", NULL);
        return TCL_ERROR;
      }
    }
  } else {
    if (argc == 4)
      return tcl_getbinds(tl, argv[3]);
    bind_bind_entry(tl, argv[2], argv[3], argv[4]);
  }
  Tcl_AppendResult(irp, argv[3], NULL);
  return TCL_OK;
}

int check_validity(char *nme, IntFunc func)
{
  char *p;
  tcl_bind_list_t *tl;

  if (*nme != '*')
    return 0;
  p = strchr(nme + 1, ':');
  if (p == NULL)
    return 0;
  *p = 0;
  tl = find_bind_table(nme + 1);
  *p = ':';
  if (!tl)
    return 0;
  if (tl->func != func)
    return 0;
  return 1;
}

static int builtin_3char STDVAR
{
  Function F = (Function) cd;

  BADARGS(4, 4, " from to args");

  CHECKVALIDITY(builtin_3char);
  F(argv[1], argv[2], argv[3]);
  return TCL_OK;
}

static int builtin_2char STDVAR
{
  Function F = (Function) cd;

  BADARGS(3, 3, " nick msg");

  CHECKVALIDITY(builtin_2char);
  F(argv[1], argv[2]);
  return TCL_OK;
}

static int builtin_5int STDVAR
{
  Function F = (Function) cd;

  BADARGS(6, 6, " min hrs dom mon year");

  CHECKVALIDITY(builtin_5int);
  F(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
  return TCL_OK;
}

static int builtin_cron STDVAR
{
  Function F = (Function) cd;

  BADARGS(6, 6, " min hrs dom mon weekday");

  CHECKVALIDITY(builtin_cron);
  F(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
  return TCL_OK;
}

static int builtin_char STDVAR
{
  Function F = (Function) cd;

  BADARGS(2, 2, " handle");

  CHECKVALIDITY(builtin_char);
  F(argv[1]);
  return TCL_OK;
}

static int builtin_chpt STDVAR
{
  Function F = (Function) cd;

  BADARGS(3, 3, " bot nick sock");

  CHECKVALIDITY(builtin_chpt);
  F(argv[1], argv[2], atoi(argv[3]));
  return TCL_OK;
}

static int builtin_chjn STDVAR
{
  Function F = (Function) cd;

  BADARGS(6, 6, " bot nick chan# flag&sock host");

  CHECKVALIDITY(builtin_chjn);
  F(argv[1], argv[2], atoi(argv[3]), argv[4][0],
    argv[4][0] ? atoi(argv[4] + 1) : 0, argv[5]);
  return TCL_OK;
}

static int builtin_idxchar STDVAR
{
  Function F = (Function) cd;
  int idx;
  char *r;

  BADARGS(3, 3, " idx args");

  CHECKVALIDITY(builtin_idxchar);
  idx = findidx(atoi(argv[1]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  r = (((char *(*)()) F) (idx, argv[2]));

  Tcl_ResetResult(irp);
  Tcl_AppendResult(irp, r, NULL);
  return TCL_OK;
}

static int builtin_charidx STDVAR
{
  Function F = (Function) cd;
  int idx;

  BADARGS(3, 3, " handle idx");

  CHECKVALIDITY(builtin_charidx);
  idx = findanyidx(atoi(argv[2]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(irp, int_to_base10(F(argv[1], idx)), NULL);

  return TCL_OK;
}

static int builtin_chat STDVAR
{
  Function F = (Function) cd;
  int ch;

  BADARGS(4, 4, " handle idx text");

  CHECKVALIDITY(builtin_chat);
  ch = atoi(argv[2]);
  F(argv[1], ch, argv[3]);
  return TCL_OK;
}

static int builtin_dcc STDVAR
{
  int idx;
  Function F = (Function) cd;

  BADARGS(4, 4, " hand idx param");

  CHECKVALIDITY(builtin_dcc);
  idx = findidx(atoi(argv[2]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }

  /* FIXME: This is an ugly hack. It is not documented as a
   *        'feature' because it will eventually go away.
   */
  if (F == CMD_LEAVE) {
    Tcl_AppendResult(irp, "break", NULL);
    return TCL_OK;
  }

  /* Check if it's a password change, if so, don't show the password. We
   * don't need pretty formats here, as it's only for debugging purposes.
   */
  debug4("tcl: builtin dcc call: %s %s %s %s", argv[0], argv[1], argv[2],
         (!strcmp(argv[0] + 5, "newpass") || !strcmp(argv[0] + 5, "chpass")) ?
         "[something]" : argv[3]);
  F(dcc[idx].user, idx, argv[3]);
  Tcl_ResetResult(irp);
  Tcl_AppendResult(irp, "0", NULL);
  return TCL_OK;
}

static int builtin_log STDVAR
{
  Function F = (Function) cd;

  BADARGS(3, 3, " lvl chan msg");

  CHECKVALIDITY(builtin_log);
  F(argv[1], argv[2], argv[3]);
  return TCL_OK;
}

/* Trigger (execute) a Tcl proc
 *
 * Note: This is INLINE code for check_tcl_bind().
 */
static inline int trigger_bind(const char *proc, const char *param,
                               char *mask)
{
  int x;
#ifdef DEBUG_CONTEXT
  const char *msg = "Tcl proc: %s, param: %s";
  char *buf;

  /* We now try to debug the Tcl_VarEval() call below by remembering both
   * the called proc name and it's parameters. This should render us a bit
   * less helpless when we see context dumps.
   */
  Context;
  buf = nmalloc(strlen(msg) + (proc ? strlen(proc) : 6)
                + (param ? strlen(param) : 6) + 1);
  sprintf(buf, msg, proc ? proc : "<null>", param ? param : "<null>");
  ContextNote(buf);
  nfree(buf);
#endif /* DEBUG_CONTEXT */

  /* Set the lastbind variable before evaluating the proc so that the name
   * of the command that triggered the bind will be available to the proc.
   * This feature is used by scripts such as userinfo.tcl
   */
  Tcl_SetVar(interp, "lastbind", (char *) mask, TCL_GLOBAL_ONLY);

  x = Tcl_VarEval(interp, proc, param, NULL);
  Context;

  if (x == TCL_ERROR) {
    /* FIXME: we really should be able to log longer errors */
    putlog(LOG_MISC, "*", "Tcl error [%s]: %.*s", proc, 400, tcl_resultstring());

    return BIND_EXECUTED;
  }

  /* FIXME: This is an ugly hack. It is not documented as a
   *        'feature' because it will eventually go away.
   */
  if (!strcmp(tcl_resultstring(), "break"))
    return BIND_QUIT;

  return (tcl_resultint() > 0) ? BIND_EXEC_LOG : BIND_EXECUTED;
}


/* Find out whether this bind matches the mask or provides the
 * requested attributes, depending on the specified requirements.
 *
 * Note: This is INLINE code for check_tcl_bind().
 */
static inline int check_bind_match(const char *match, char *mask,
                                   int match_type)
{
  switch (match_type & 0x07) {
  case MATCH_PARTIAL:
    return (!egg_strncasecmp(match, mask, strlen(match)));
    break;
  case MATCH_EXACT:
    return (!egg_strcasecmp(match, mask));
    break;
  case MATCH_CASE:
    return (!strcmp(match, mask));
    break;
  case MATCH_MASK:
    return (wild_match_per(mask, match));
    break;
  case MATCH_MODE:
    return (wild_match_partial_case(mask, match));
    break;
  case MATCH_CRON:
    return (cron_match(mask, match));
    break;
  default:
    /* Do nothing */
    break;
  }
  return 0;
}


/* Check if the provided flags suffice for this command/trigger.
 *
 * Note: This is INLINE code for check_tcl_bind().
 */
static inline int check_bind_flags(struct flag_record *flags,
                                   struct flag_record *atr, int match_type)
{
  if (match_type & BIND_USE_ATTR) {
    if (match_type & BIND_HAS_BUILTINS)
      return (flagrec_ok(flags, atr));
    else
      return (flagrec_eq(flags, atr));
  } else
    return 1;
  return 0;
}


/* Check for and process Tcl binds */
int check_tcl_bind(tcl_bind_list_t *tl, const char *match,
                   struct flag_record *atr, const char *param, int match_type)
{
  int x, result = 0, cnt = 0, finish = 0;
  char *proc = NULL, *mask = NULL;
  tcl_bind_mask_t *tm, *tm_last = NULL, *tm_p = NULL;
  tcl_cmd_t *tc, *htc = NULL;

  for (tm = tl->first; tm && !finish; tm_last = tm, tm = tm->next) {

    if (tm->flags & TBM_DELETED)
      continue;                 /* This bind mask was deleted */

    if (!check_bind_match(match, tm->mask, match_type))
      continue;                 /* This bind does not match. */

    for (tc = tm->first; tc; tc = tc->next) {

      /* Search for valid entry. */
      if (!(tc->attributes & TC_DELETED)) {

        /* Check if the provided flags suffice for this command. */
        if (check_bind_flags(&tc->flags, atr, match_type)) {
          cnt++;
          tm_p = tm_last;

          /* Not stackable */
          if (!(match_type & BIND_STACKABLE)) {

            /* Remember information about this bind. */
            proc = tc->func_name;
            mask = tm->mask;
            htc = tc;

            /* Either this is a non-partial match, which means we
             * only want to execute _one_ bind ...
             */
            if ((match_type & 0x07) != MATCH_PARTIAL ||
              /* ... or this happens to be an exact match. */
              !egg_strcasecmp(match, tm->mask)) {
              cnt = 1;
              finish = 1;
            }

            /* We found a match so break out of the inner loop. */
            break;
          }

          /*
           * Stackable; could be multiple commands/triggers.
           * Note: This code assumes BIND_ALTER_ARGS, BIND_WANTRET, and
           *       BIND_STACKRET will only be used for stackable binds.
           */

          /* We will only return if BIND_ALTER_ARGS or BIND_WANTRET was
           * specified because we want to trigger all binds in a stack.
           */

          tc->hits++;

          x = trigger_bind(tc->func_name, param, tm->mask);

          if ((tl->flags & HT_DELETED) || (tm->flags & TBM_DELETED)) {
            /* This bind or bind type was deleted within trigger_bind() */
            return x;
          } else if (match_type & BIND_ALTER_ARGS) {
            if (tcl_resultempty())
              return x;
          } else if ((match_type & BIND_STACKRET) && x == BIND_EXEC_LOG) {
            /* If we have multiple commands/triggers, and if any of the
             * commands return 1, we store the result so we can return it
             * after processing all stacked binds.
             */
            if (!result)
              result = x;
            continue;
          } else if ((match_type & BIND_WANTRET) && x == BIND_EXEC_LOG) {
            /* Return immediately if any commands return 1 */
            return x;
          }
        }
      }
    }
  }

  if (!cnt)
    return BIND_NOMATCH;

  /* Do this before updating the preferred entries information,
   * since we don't want to change the order of stacked binds
   */
  if (result)           /* BIND_STACKRET */
    return result;

  if ((match_type & 0x07) == MATCH_MASK || (match_type & 0x07) == MATCH_CASE)
    return BIND_EXECUTED;

  /* Hit counter */
  if (htc)
    htc->hits++;

  /* Now that we have found at least one bind, we can update the
   * preferred entries information.
   */
  if (tm_p && tm_p->next) {
    tm = tm_p->next;            /* Move mask to front of bind's mask list. */
    tm_p->next = tm->next;      /* Unlink mask from list. */
    tm->next = tl->first;       /* Readd mask to front of list. */
    tl->first = tm;
  }

  if (cnt > 1)
    return BIND_AMBIGUOUS;

  return trigger_bind(proc, param, mask);
}


/* Check for tcl-bound dcc command, return 1 if found
 * dcc: proc-name <handle> <sock> <args...>
 */
int check_tcl_dcc(const char *cmd, int idx, const char *args)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };
  int x;
  char s[11];

  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);
  egg_snprintf(s, sizeof s, "%ld", dcc[idx].sock);
  Tcl_SetVar(interp, "_dcc1", (char *) dcc[idx].nick, 0);
  Tcl_SetVar(interp, "_dcc2", (char *) s, 0);
  Tcl_SetVar(interp, "_dcc3", (char *) args, 0);
  x = check_tcl_bind(H_dcc, cmd, &fr, " $_dcc1 $_dcc2 $_dcc3",
                     MATCH_PARTIAL | BIND_USE_ATTR | BIND_HAS_BUILTINS);
  if (x == BIND_AMBIGUOUS) {
    dprintf(idx, MISC_AMBIGUOUS);
    return 0;
  }
  if (x == BIND_NOMATCH) {
    dprintf(idx, MISC_NOSUCHCMD);
    return 0;
  }

  /* We return 1 to leave the partyline */
  if (x == BIND_QUIT)           /* CMD_LEAVE, 'quit' */
    return 1;

  if (x == BIND_EXEC_LOG)
    putlog(LOG_CMDS, "*", "#%s# %s %s", dcc[idx].nick, cmd, args);
  return 0;
}

void check_tcl_bot(const char *nick, const char *code, const char *param)
{
  Tcl_SetVar(interp, "_bot1", (char *) nick, 0);
  Tcl_SetVar(interp, "_bot2", (char *) code, 0);
  Tcl_SetVar(interp, "_bot3", (char *) param, 0);
  check_tcl_bind(H_bot, code, 0, " $_bot1 $_bot2 $_bot3", MATCH_EXACT);
}

void check_tcl_chonof(char *hand, int sock, tcl_bind_list_t *tl)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };
  char s[11];
  struct userrec *u;

  u = get_user_by_handle(userlist, hand);
  touch_laston(u, "partyline", now);
  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_chonof1", (char *) hand, 0);
  egg_snprintf(s, sizeof s, "%d", sock);
  Tcl_SetVar(interp, "_chonof2", (char *) s, 0);
  check_tcl_bind(tl, hand, &fr, " $_chonof1 $_chonof2", MATCH_MASK |
                 BIND_USE_ATTR | BIND_STACKABLE | BIND_WANTRET);
}

void check_tcl_chatactbcst(const char *from, int chan, const char *text,
                           tcl_bind_list_t *tl)
{
  char s[11];

  egg_snprintf(s, sizeof s, "%d", chan);
  Tcl_SetVar(interp, "_cab1", (char *) from, 0);
  Tcl_SetVar(interp, "_cab2", (char *) s, 0);
  Tcl_SetVar(interp, "_cab3", (char *) text, 0);
  check_tcl_bind(tl, text, 0, " $_cab1 $_cab2 $_cab3",
                 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_nkch(const char *ohand, const char *nhand)
{
  Tcl_SetVar(interp, "_nkch1", (char *) ohand, 0);
  Tcl_SetVar(interp, "_nkch2", (char *) nhand, 0);
  check_tcl_bind(H_nkch, ohand, 0, " $_nkch1 $_nkch2",
                 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_link(const char *bot, const char *via)
{
  Tcl_SetVar(interp, "_link1", (char *) bot, 0);
  Tcl_SetVar(interp, "_link2", (char *) via, 0);
  check_tcl_bind(H_link, bot, 0, " $_link1 $_link2",
                 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_disc(const char *bot)
{
  Tcl_SetVar(interp, "_disc1", (char *) bot, 0);
  check_tcl_bind(H_disc, bot, 0, " $_disc1", MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_loadunld(const char *mod, tcl_bind_list_t *tl)
{
  Tcl_SetVar(interp, "_lu1", (char *) mod, 0);
  check_tcl_bind(tl, mod, 0, " $_lu1", MATCH_MASK | BIND_STACKABLE);
}

const char *check_tcl_filt(int idx, const char *text)
{
  char s[11];
  int x;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  egg_snprintf(s, sizeof s, "%ld", dcc[idx].sock);
  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);
  Tcl_SetVar(interp, "_filt1", (char *) s, 0);
  Tcl_SetVar(interp, "_filt2", (char *) text, 0);
  x = check_tcl_bind(H_filt, text, &fr, " $_filt1 $_filt2",
                     MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE |
                     BIND_WANTRET | BIND_ALTER_ARGS);
  if (x == BIND_EXECUTED || x == BIND_EXEC_LOG) {
    if (tcl_resultempty())
      return "";
    else
      return tcl_resultstring();
  } else
    return text;
}

int check_tcl_note(const char *from, const char *to, const char *text)
{
  int x;

  Tcl_SetVar(interp, "_note1", (char *) from, 0);
  Tcl_SetVar(interp, "_note2", (char *) to, 0);
  Tcl_SetVar(interp, "_note3", (char *) text, 0);

  x = check_tcl_bind(H_note, to, 0, " $_note1 $_note2 $_note3",
                     MATCH_MASK | BIND_STACKABLE | BIND_WANTRET);

  return (x == BIND_EXEC_LOG);
}

void check_tcl_listen(const char *cmd, int idx)
{
  char s[11];
  int x;

  egg_snprintf(s, sizeof s, "%d", idx);
  Tcl_SetVar(interp, "_n", (char *) s, 0);
  x = Tcl_VarEval(interp, cmd, " $_n", NULL);
  if (x == TCL_ERROR)
    putlog(LOG_MISC, "*", "error on listen: %s", tcl_resultstring());
}

void check_tcl_chjn(const char *bot, const char *nick, int chan,
                    const char type, int sock, const char *host)
{
  struct flag_record fr = { FR_GLOBAL, 0, 0, 0, 0, 0 };
  char s[11], t[2], u[11];

  t[0] = type;
  t[1] = 0;
  switch (type) {
  case '*':
    fr.global = USER_OWNER;

    break;
  case '+':
    fr.global = USER_MASTER;

    break;
  case '@':
    fr.global = USER_OP;

    break;
  case '^':
    fr.global = USER_HALFOP;

    break;
  case '%':
    fr.global = USER_BOTMAST;
  }
  egg_snprintf(s, sizeof s, "%d", chan);
  egg_snprintf(u, sizeof u, "%d", sock);
  Tcl_SetVar(interp, "_chjn1", (char *) bot, 0);
  Tcl_SetVar(interp, "_chjn2", (char *) nick, 0);
  Tcl_SetVar(interp, "_chjn3", (char *) s, 0);
  Tcl_SetVar(interp, "_chjn4", (char *) t, 0);
  Tcl_SetVar(interp, "_chjn5", (char *) u, 0);
  Tcl_SetVar(interp, "_chjn6", (char *) host, 0);
  check_tcl_bind(H_chjn, s, &fr,
                 " $_chjn1 $_chjn2 $_chjn3 $_chjn4 $_chjn5 $_chjn6",
                 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_chpt(const char *bot, const char *hand, int sock, int chan)
{
  char u[11], v[11];

  egg_snprintf(u, sizeof u, "%d", sock);
  egg_snprintf(v, sizeof v, "%d", chan);
  Tcl_SetVar(interp, "_chpt1", (char *) bot, 0);
  Tcl_SetVar(interp, "_chpt2", (char *) hand, 0);
  Tcl_SetVar(interp, "_chpt3", (char *) u, 0);
  Tcl_SetVar(interp, "_chpt4", (char *) v, 0);
  check_tcl_bind(H_chpt, v, 0, " $_chpt1 $_chpt2 $_chpt3 $_chpt4",
                 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_away(const char *bot, int idx, const char *msg)
{
  char u[11];

  egg_snprintf(u, sizeof u, "%d", idx);
  Tcl_SetVar(interp, "_away1", (char *) bot, 0);
  Tcl_SetVar(interp, "_away2", (char *) u, 0);
  Tcl_SetVar(interp, "_away3", msg ? (char *) msg : "", 0);
  check_tcl_bind(H_away, bot, 0, " $_away1 $_away2 $_away3",
                 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_time(struct tm *tm)
{
  char y[18];

  egg_snprintf(y, sizeof y, "%02d", tm->tm_min);
  Tcl_SetVar(interp, "_time1", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%02d", tm->tm_hour);
  Tcl_SetVar(interp, "_time2", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%02d", tm->tm_mday);
  Tcl_SetVar(interp, "_time3", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%02d", tm->tm_mon);
  Tcl_SetVar(interp, "_time4", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%04d", tm->tm_year + 1900);
  Tcl_SetVar(interp, "_time5", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%02d %02d %02d %02d %04d", tm->tm_min, tm->tm_hour,
               tm->tm_mday, tm->tm_mon, tm->tm_year + 1900);
  check_tcl_bind(H_time, y, 0,
                 " $_time1 $_time2 $_time3 $_time4 $_time5",
                 MATCH_MASK | BIND_STACKABLE);
}

void check_tcl_cron(struct tm *tm)
{
  char y[15];

  egg_snprintf(y, sizeof y, "%02d", tm->tm_min);
  Tcl_SetVar(interp, "_cron1", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%02d", tm->tm_hour);
  Tcl_SetVar(interp, "_cron2", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%02d", tm->tm_mday);
  Tcl_SetVar(interp, "_cron3", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%02d", tm->tm_mon + 1);
  Tcl_SetVar(interp, "_cron4", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%02d", tm->tm_wday);
  Tcl_SetVar(interp, "_cron5", (char *) y, 0);
  egg_snprintf(y, sizeof y, "%02d %02d %02d %02d %02d", tm->tm_min, tm->tm_hour,
               tm->tm_mday, tm->tm_mon + 1, tm->tm_wday);
  check_tcl_bind(H_cron, y, 0,
                 " $_cron1 $_cron2 $_cron3 $_cron4 $_cron5",
                 MATCH_CRON | BIND_STACKABLE);
}

void check_tcl_event(const char *event)
{
  Tcl_SetVar(interp, "_event1", (char *) event, 0);
  check_tcl_bind(H_event, event, 0, " $_event1", MATCH_EXACT | BIND_STACKABLE);
}

void check_tcl_log(int lv, char *chan, char *msg)
{
  char mask[512];

  snprintf(mask, sizeof mask, "%s %s", chan, msg);
  Tcl_SetVar(interp, "_log1", masktype(lv), 0);
  Tcl_SetVar(interp, "_log2", chan, 0);
  Tcl_SetVar(interp, "_log3", msg, 0);
  check_tcl_bind(H_log, mask, 0, " $_log1 $_log2 $_log3",
                 MATCH_MASK | BIND_STACKABLE);
}

void tell_binds(int idx, char *par)
{
  tcl_bind_list_t *tl, *tl_kind;
  tcl_bind_mask_t *tm;
  int fnd = 0, showall = 0, patmatc = 0;
  tcl_cmd_t *tc;
  char *name, *proc, *s, flg[100];

  if (par[0])
    name = newsplit(&par);
  else
    name = NULL;
  if (par[0])
    s = newsplit(&par);
  else
    s = NULL;

  if (name)
    tl_kind = find_bind_table(name);
  else
    tl_kind = NULL;

  if ((name && name[0] && !egg_strcasecmp(name, "all")) ||
      (s && s[0] && !egg_strcasecmp(s, "all")))
    showall = 1;
  if (tl_kind == NULL && name && name[0] && egg_strcasecmp(name, "all"))
    patmatc = 1;

  dprintf(idx, MISC_CMDBINDS);
  dprintf(idx, "  TYPE FLAGS    COMMAND              HITS BINDING (TCL)\n");

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
        proc = tc->func_name;
        build_flags(flg, &(tc->flags), NULL);
        if (showall || proc[0] != '*') {
          int ok = 0;

          if (patmatc == 1) {
            if (wild_match_per(name, tl->name) ||
                wild_match_per(name, tm->mask) ||
                wild_match_per(name, tc->func_name))
              ok = 1;
          } else
            ok = 1;

          if (ok) {
            dprintf(idx, "  %-4s %-8s %-20s %4d %s\n", tl->name, flg, tm->mask,
                    tc->hits, tc->func_name);
            fnd = 1;
          }
        }
      }
    }
  }
  if (!fnd) {
    if (patmatc)
      dprintf(idx, "No command bindings found that match %s\n", name);
    else if (tl_kind)
      dprintf(idx, "No command bindings for type: %s.\n", name);
    else
      dprintf(idx, "No command bindings exist.\n");
  }
}

/* Bring the default msg/dcc/fil commands into the Tcl interpreter */
void add_builtins(tcl_bind_list_t *tl, cmd_t *cc)
{
  int k, i;
  char p[1024], *l;
  cd_tcl_cmd table[2];

  table[0].name = p;
  table[0].callback = tl->func;
  table[1].name = NULL;
  for (i = 0; cc[i].name; i++) {
    egg_snprintf(p, sizeof p, "*%s:%s", tl->name,
                 cc[i].funcname ? cc[i].funcname : cc[i].name);
    l = nmalloc(Tcl_ScanElement(p, &k) + 1);
    Tcl_ConvertElement(p, l, k | TCL_DONT_USE_BRACES);
    table[0].cdata = (void *) cc[i].func;
    add_cd_tcl_cmds(table);
    bind_bind_entry(tl, cc[i].flags, cc[i].name, l);
    nfree(l);
  }
}

/* Remove the default msg/dcc/fil commands from the Tcl interpreter */
void rem_builtins(tcl_bind_list_t *table, cmd_t *cc)
{
  int k, i;
  char p[1024], *l;

  for (i = 0; cc[i].name; i++) {
    egg_snprintf(p, sizeof p, "*%s:%s", table->name,
                 cc[i].funcname ? cc[i].funcname : cc[i].name);
    l = nmalloc(Tcl_ScanElement(p, &k) + 1);
    Tcl_ConvertElement(p, l, k | TCL_DONT_USE_BRACES);
    Tcl_DeleteCommand(interp, p);
    unbind_bind_entry(table, cc[i].flags, cc[i].name, l);
    nfree(l);
  }
}
