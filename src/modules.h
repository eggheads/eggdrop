/*
 * modules.h - support for code modules in eggdrop
 * by Darrin Smith (beldin@light.iinet.net.au)
 */
/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

/*
 * module related structures
 */

#ifndef _MODULE_H_
#define _MODULE_H_

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
void *mod_malloc(int size, char *modname, char *filename, int line);
void mod_free(void *ptr, char *modname, char *filename, int line);
void add_hook(int hook_num, void *func);
void del_hook(int hook_num, void *func);
void *get_next_hook(int hook_num, void *func);
extern struct hook_entry {
  struct hook_entry *next;
  int (*func) ();
} *hook_list[REAL_HOOKS];

#define call_hook(x) { struct hook_entry *p; \
for (p = hook_list[x]; p; p = p->next) p->func(); }
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

#endif				/* _MODULE_H_ */
