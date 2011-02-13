/*
 * woobie.c -- part of woobie.mod
 *   nonsensical command to exemplify module programming
 *
 * Originally written by ButchBub         15 July     1997
 * Comments by Fabian Knittel             29 December 1999
 *
 * $Id: woobie.c,v 1.29 2011/02/13 14:19:34 simple Exp $
 */
/*
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

#define MODULE_NAME "woobie"
#define MAKING_WOOBIE

#include "src/mod/module.h"
#include <stdlib.h>

#undef global
/* Pointer to the eggdrop core function table. Gets initialized in
 * woobie_start().
 */
static Function *global = NULL;

/* Calculate the memory we keep allocated.
 */
static int woobie_expmem()
{
  int size = 0;

  Context;
  return size;
}

static int cmd_woobie(struct userrec *u, int idx, char *par)
{
  /* Define a context.
   *
   * If the bot crashes after the context, it will be  the last mentioned
   * in the resulting DEBUG file. This helps you debugging.
   */
  Context;

  /* Log the command as soon as you're sure all parameters are valid. */
  putlog(LOG_CMDS, "*", "#%s# woobie", dcc[idx].nick);

  dprintf(idx, "WOOBIE!\n");
  return 0;
}

/* A report on the module status.
 *
 * details is either 0 or 1:
 *    0 - `.status'
 *    1 - `.status all'  or  `.module woobie'
 */
static void woobie_report(int idx, int details)
{
  if (details) {
    int size = woobie_expmem();

    dprintf(idx, "    Using %d byte%s of memory\n", size,
            (size != 1) ? "s" : "");
  }
}

/* Note: The tcl-name is automatically created if you set it to NULL. In
 *       the example below it would be just "*dcc:woobie". If you specify
 *       "woobie:woobie" it would be "*dcc:woobie:woobie" instead.
 *               ^----- command name   ^--- table name
 *        ^------------ module name
 *
 *       This is only useful for stackable binding tables (and H_dcc isn't
 *       stackable).
 */
static cmd_t mydcc[] = {
  /* command  flags  function     tcl-name */
  {"woobie",  "",    cmd_woobie,  NULL},
  {NULL,      NULL,  NULL,        NULL}  /* Mark end. */
};

static char *woobie_close()
{
  Context;
  rem_builtins(H_dcc, mydcc);
  module_undepend(MODULE_NAME);
  return NULL;
}

/* Define the prototype here, to avoid warning messages in the
 * woobie_table.
 */
EXPORT_SCOPE char *woobie_start();

/* This function table is exported and may be used by other modules and
 * the core.
 *
 * The first four have to be defined (you may define them as NULL), as
 * they are checked by eggdrop core.
 */
static Function woobie_table[] = {
  (Function) woobie_start,
  (Function) woobie_close,
  (Function) woobie_expmem,
  (Function) woobie_report,
};

char *woobie_start(Function *global_funcs)
{
  /* Assign the core function table. After this point you use all normal
   * functions defined in src/mod/modules.h
   */
  global = global_funcs;

  Context;
  /* Register the module. */
  module_register(MODULE_NAME, woobie_table, 2, 0);
  /*                                            ^--- minor module version
   *                                         ^------ major module version
   *                           ^-------------------- module function table
   *              ^--------------------------------- module name
   */

  if (!module_depend(MODULE_NAME, "eggdrop", 106, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.6.0 or later.";
  }

  /* Add command table to bind list H_dcc, responsible for dcc commands.
   * Currently we only add one command, `woobie'.
   */
  add_builtins(H_dcc, mydcc);
  return NULL;
}
