/* these *were* something, but changed */
#define HOOK_GET_FLAGREC         0
#define HOOK_BUILD_FLAGREC       1
#define HOOK_SET_FLAGREC         2
#define HOOK_READ_USERFILE       3
#define HOOK_REHASH              4
#define HOOK_MINUTELY            5
#define HOOK_DAILY               6
#define HOOK_HOURLY              7
#define HOOK_USERFILE            8
#define HOOK_SECONDLY            9
#define HOOK_PRE_REHASH          10
#define HOOK_IDLE                11
#define HOOK_5MINUTELY           12
#define REAL_HOOKS               13
#define HOOK_SHAREOUT            105
#define HOOK_SHAREIN             106
#define HOOK_ENCRYPT_PASS        107
#define HOOK_QSERV               108
#define HOOK_ADD_MODE            109
#define HOOK_MATCH_NOTEREJ	 110

/* these are FIXED once they are in a release they STAY
 * well, unless im feeling grumpy ;) */
#define MODCALL_START  0
#define MODCALL_CLOSE  1
#define MODCALL_EXPMEM 2
#define MODCALL_REPORT 3
/* filesys */
#define FILESYS_REMOTE_REQ 4
#define FILESYS_ADDFILE    5
#define FILESYS_INCRGOTS   6
#define FILESYS_ISVALID	   7
/* share */
#define SHARE_FINISH       4
#define SHARE_DUMP_RESYNC  5
/* channels */
#define CHANNEL_CLEAR     15
/* server */
#define SERVER_BOTNAME       4
#define SERVER_BOTUSERHOST   5
/* irc */
#define IRC_RECHECK_CHANNEL 15
/* notes */
#define NOTES_CMD_NOTE       4
 
#ifdef HPUX_HACKS
#include <dl.h>
#endif

typedef struct _module_entry {
  char *name;			/* name of the module (without .so) */
  int major;			/* major version number MUST match */
  int minor;			/* minor version number MUST be >= */
#ifndef STATIC
#ifdef HPUX_HACKS
  shl_t hand;
#else
  void *hand;			/* module handle */
#endif
#endif
  struct _module_entry *next;
  Function *funcs;
#ifdef EBUG_MEM
  int mem_work;
#endif
} module_entry;
