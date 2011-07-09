/*
 * modules.h
 *   support for modules in eggdrop
 *
 * by Darrin Smith (beldin@light.iinet.net.au)
 *
 * $Id: modules.h,v 1.23 2011/07/09 15:07:48 thommey Exp $
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

#ifndef _EGG_MODULE_H
#define _EGG_MODULE_H

/* Module related structures
 */
#include "mod/modvals.h"

#ifndef MAKING_NUMMODS

/* Modules specific functions and functions called by Eggdrop */
void do_module_report(int, int, char *);
int module_register(char *, Function *, int, int);
const char *module_load(char *);
char *module_unload(char *, char *);
module_entry *module_find(char *, int, int);
Function *module_depend(char *, char *, int, int);
int module_undepend(char *);
char *mod_strdup(const char *, const char *, const char *, int);
void *mod_malloc(int, const char *, const char *, int);
void *mod_realloc(void *, int, const char *, const char *, int);
void mod_free(void *, const char *, const char *, int);
void add_hook(int, Function);
void del_hook(int, Function);
void *get_next_hook(int, void *);

extern struct hook_entry {
  struct hook_entry *next;
  Function func;
} *hook_list[REAL_HOOKS];

#define call_hook(x) do {                                       \
        register struct hook_entry *p, *pn;                     \
                                                                \
        for (p = hook_list[x]; p; p = pn) {                     \
                pn = p->next;                                   \
                p->func();                                      \
        }                                                       \
} while (0)

int call_hook_cccc(int, char *, char *, char *, char *);

#endif

typedef struct _dependancy {
  struct _module_entry *needed;
  struct _module_entry *needing;
  struct _dependancy *next;
  int major;
  int minor;
} dependancy;
extern dependancy *dependancy_list;

#endif /* _EGG_MODULE_H */
