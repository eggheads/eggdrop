/* 
 * modules.h
 *   support for modules in eggdrop
 * 
 * by Darrin Smith (beldin@light.iinet.net.au)
 * 
 * $Id: modules.h,v 1.7 2000/07/01 06:28:03 guppy Exp $
 */
/* 
 * Copyright (C) 1997  Robey Pointer
 * Copyright (C) 1999, 2000  Eggheads
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

/* 
 * module related structures
 */

#include "mod/modvals.h"

#ifndef MAKING_NUMMODS
/* modules specific functions */
/* functions called by eggdrop */
void do_module_report(int, int, char *);

int module_register(char *name, Function * funcs,
		    int major, int minor);
const char *module_load(char *module_name);
char *module_unload(char *module_name, char *nick);
module_entry *module_find(char *name, int, int);
Function *module_depend(char *, char *, int major, int minor);
int module_undepend(char *);
void *mod_malloc(int, char *, char *, int);
void *mod_realloc(void *, int, char *, char *, int);
void mod_free(void *ptr, char *modname, char *filename, int line);
void add_hook(int hook_num, Function func);
void del_hook(int hook_num, Function func);
void *get_next_hook(int hook_num, void *func);
extern struct hook_entry {
  struct hook_entry *next;
  int (*func) ();
} *hook_list[REAL_HOOKS];

#define call_hook(x) do { struct hook_entry *p, *pn; \
for (p = hook_list[x]; p; p = pn) { pn = p->next; p->func(); } } while (0)
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

#endif				/* _EGG_MODULE_H */
