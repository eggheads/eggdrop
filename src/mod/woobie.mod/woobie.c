/* 
 * woobie.c -- part of woobie.mod
 *   nonsensical command to exemplify module programming
 * 
 * By ButchBub - 15 July 1997
 * 
 * $Id: woobie.c,v 1.6 1999/12/15 02:33:00 guppy Exp $
 */
/* 
 * Copyright (C) 1997  Robey Pointer
 * Copyright (C) 1999  Eggheads
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
#include "../module.h"
#include <stdlib.h>

#undef global
static Function *global = NULL;

static int woobie_expmem()
{
  int size = 0;

  Context;
  return size;
}

static int cmd_woobie(struct userrec *u, int idx, char *par)
{
  Context;
  putlog(LOG_CMDS, "*", "#%s# woobie", dcc[idx].nick);
  dprintf(idx, "WOOBIE!\n");
  return 0;
}

/* a report on the module status */
static void woobie_report(int idx, int details)
{
  int size = 0;

  Context;
  if (details)
    dprintf(idx, "    0 woobies using %d bytes\n", size);
}

static cmd_t mydcc[] =
{
  {"woobie", "", cmd_woobie, NULL},
  {0, 0, 0, 0}
};

static char *woobie_close()
{
  Context;
  rem_builtins(H_dcc, mydcc);
  module_undepend(MODULE_NAME);
  return NULL;
}

char *woobie_start();

static Function woobie_table[] =
{
  (Function) woobie_start,
  (Function) woobie_close,
  (Function) woobie_expmem,
  (Function) woobie_report,
};

char *woobie_start(Function * global_funcs)
{
  global = global_funcs;
  Context;
  module_register(MODULE_NAME, woobie_table, 2, 0);
  if (!module_depend(MODULE_NAME, "eggdrop", 104, 0))
    return "This module requires eggdrop1.4.0 or later";
  add_builtins(H_dcc, mydcc);
  return NULL;
}
