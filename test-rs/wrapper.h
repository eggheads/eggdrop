/*
 * Wrapper header for bindgen.
 * Includes eggdrop's main header chain plus module-specific headers.
 * Symbols not declared here are discovered via nm on libeggdrop.a
 * and declared in the generated globals.h (see build.rs).
 */

/* MAKING_MODS is defined via -D in build.rs */

#include "src/main.h"
#include "src/modules.h"
#include "src/mod/modvals.h"

/* Module headers that define key structs/variables */
#include "src/mod/server.mod/server.h"
#include "src/mod/irc.mod/irc.h"
#include "src/mod/channels.mod/channels.h"
