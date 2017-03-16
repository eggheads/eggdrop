/*
 * pbkdf2.c -- part of pbkdf2.mod
 *   PBKDF2 password hashing
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2017 Eggheads Development Team
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

/* Also in mbedtls/config.h! */
#define MODULE_NAME "encryption2"

#include "src/mod/module.h"

#define PBKDF2_HASH     "SHA256"
#define PBKDF2_CYCLES   512000

/* Global functions exported to other modules (egg_malloc, egg_free, ..) */
#undef global
#define global pbkdf2_global

Function *pbkdf2_global = NULL;

EXPORT_SCOPE char *pbkdf2_start(Function *global_funcs);
static char *pbkdf2_close(void);
static char *pbkdf2_encrypt_pass(const char *pass);
static int pbkdf2_verify_pass(const char *pass, const char *hash);


static Function exported_functions[] = {
  (Function) pbkdf2_start,
  (Function) pbkdf2_close,
  (Function) NULL, /* expmem */
  (Function) NULL, /* report */
  (Function) pbkdf2_encrypt_pass,
  (Function) pbkdf2_verify_pass
};

char *pbkdf2_start(Function *global_funcs)
{
  if (global_funcs) {
    pbkdf2_global = global_funcs;
    if (!module_rename("pbkdf2", MODULE_NAME))
      return "Already loaded.";
    module_register(MODULE_NAME, exported_functions, 0, 1);
    if (!module_depend(MODULE_NAME, "eggdrop", 108, 0)) {
      module_undepend(MODULE_NAME);
      return "This module requires Eggdrop 1.8.0 or later.";
    }
    add_hook(HOOK_ENCRYPT_PASS2, (Function) pbkdf2_encrypt_pass);
    add_hook(HOOK_VERIFY_PASS2, (Function) pbkdf2_verify_pass);
  }
  return NULL;
}

static char *pbkdf2_close(void)
{
  return "You cannot unload the PBKDF2 module.";
}

static char *pbkdf2_encrypt_pass(const char *pass)
{
  return 0;
}

/* Return values:
 * 0 - no match
 * 1 - match
 * 2 - match, but re-hash password (more cycles, new algorithm, ...)
 */
static int pbkdf2_verify_pass(const char *pass, const char *hash) {
  return 0;
}
