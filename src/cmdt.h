/* stuff for builtin commands */
/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */
#ifndef _H_CMDT
#define _H_CMDT

#define CMD_LEAVE    (Function)(-1)
typedef struct {
  char *name;
  char *flags;
  Function func;
  char *funcname;
} cmd_t;

typedef struct {
  char *name;
  Function func;
} botcmd_t;

#endif
