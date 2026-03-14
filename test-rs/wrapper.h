/*
 * Wrapper header for bindgen.
 * Includes eggdrop's main header chain plus module-specific headers.
 * Compiled with -Dstatic=extern so all file-scope declarations become
 * extern, matching the globalized symbols in libeggdrop.a.
 */

/* MAKING_MODS is defined via -D in build.rs */

#include "src/main.h"
#include "src/modules.h"
#include "src/mod/modvals.h"

/* Module headers that define key structs/variables */
#include "src/mod/server.mod/server.h"
