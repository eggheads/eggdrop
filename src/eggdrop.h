/*
 *   EGGDROP compile-time settings
 *
 *   IF YOU ALTER THIS FILE, YOU NEED TO RECOMPILE THE BOT.
 */
/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

#ifndef _H_EGGDROP
#define _H_EGGDROP

/* 
 * if you're *only* going to link to new version bots (1.3.0 or higher)
 * then you can safely define this 
 */

#undef NO_OLD_BOTNET

/*
 * define the maximum length a handle on the bot can be.
 * (standard is 9 characters long)
 * (DO NOT MAKE THIS VALUE LESS THAN 9 UNLESS YOU WANT TROUBLE!)
 * (beware that using lengths over 9 chars is 'non-standard' and if you
 * wish to link to other bots, they _must_ both have the same maximum
 * handle length)
 */

/* handy string lengths */

#define HANDLEN		9	/* valid values 9->NICKMAX */
#define BADHANDCHARS  "-,+*=:!.@#;$%&"
#define NICKMAX        9	/* valid values HANDLEN->32 */
#define UHOSTMAX     160        /* reasonable, i think? */
#define DIRMAX       256	/* paranoia */
#define MAX_LOG_LINE 767	/* for misc.c/putlog() <cybah> */

/* language stuff */

#define LANGDIR	"./language"	/* language file directory */
#define BASELANG "english"	/* language which always gets loaded before
				   all other languages. You don't want to
				   change this. */

/***********************************************************************/
/***** the 'configure' script should make this next part automatic *****/
/***********************************************************************/

#define NICKLEN         NICKMAX + 1
#define UHOSTLEN        UHOSTMAX + 1
#define DIRLEN          DIRMAX + 1
#define NOTENAMELEN     ((HANDLEN * 2) + 1)
#define BADNICKCHARS "-,+*=:!.@#;$%&"

/* have to use a weird way to make the compiler error out cos not all
 * compilers support #error or error */
#if !HAVE_VSPRINTF
#include "error_you_need_vsprintf_to_compile_eggdrop"
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef STATIC
#if (!defined(MODULES_OK) || !defined(HAVE_DLOPEN)) && !defined(HPUX_HACKS)
#include "you_can't_compile_with_module_support_on_this_system_try_make_static"
#endif
#endif

#if !defined(STDC_HEADERS)
#include "you_need_to_upgrade_your_compiler_to_a_standard_c_one_mate!"
#endif

#if (NICKMAX < 9) || (NICKMAX > 32)
#include "invalid NICKMAX value"
#endif

#if (HANDLEN < 9) || (HANDLEN > 32)
#include "invalid HANDLEN value"
#endif

#if HANDLEN > NICKMAX
#include "HANDLEN MUST BE <= NICKMAX"
#endif

/* almost every module needs some sort of time thingy, so... */
#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#if !HAVE_SRANDOM
#define srandom(x) srand(x)
#endif

#if !HAVE_RANDOM
#define random() (rand()/16)
#endif

#if !HAVE_SIGACTION		/* old "weird signals" */
#define sigaction sigvec
#ifndef sa_handler
#define sa_handler sv_handler
#define sa_mask sv_mask
#define sa_flags sv_flags
#endif
#endif

#if !HAVE_SIGEMPTYSET
/* and they probably won't have sigemptyset, dammit */
#define sigemptyset(x) ((*(int *)(x))=0)
#endif

/* handy aliases for memory tracking and core dumps */

#define nmalloc(x) n_malloc((x),__FILE__,__LINE__)
#define nrealloc(x,y) n_realloc((x),(y),__FILE__,__LINE__)
#define nfree(x) n_free((x),__FILE__,__LINE__)

#define context { cx_ptr=((cx_ptr + 1) & 15); \
                  strcpy(cx_file[cx_ptr],__FILE__); \
                  cx_line[cx_ptr]=__LINE__; \
                  cx_note[cx_ptr][0] = 0; }
/*      It's usefull to track variables too <cybah> */
#define contextnote(string) { cx_ptr=((cx_ptr + 1) & 15); \
                              strncpy(cx_file[cx_ptr],__FILE__,29); \
                              cx_file[cx_ptr][29] = 0; \
                              cx_line[cx_ptr]=__LINE__; \
                              strncpy(cx_note[cx_ptr],string,255); \
                              cx_note[cx_ptr][255] = 0; }
#define ASSERT(expr) { if (!(expr)) assert_failed (NULL, __FILE__, __LINE__); }

/* move these here, makes more sense to me :) */
extern int cx_line[16];
extern char cx_file[16][30];
extern char cx_note[16][256];
extern int cx_ptr;

#undef malloc
#undef free
#define malloc(x) dont_use_old_malloc(x)
#define free(x) dont_use_old_free(x)

/* IP type */
#if (SIZEOF_INT == 4)
typedef unsigned int IP;

#else
#if (SIZEOF_LONG == 4)
typedef unsigned long IP;

#else
#include "cant/find/32bit/type"
#endif
#endif

/* macro for simplifying patches */
#define PATCH(str) { \
  char *p=strchr(egg_version,'+'); \
  if (p==NULL) p=&egg_version[strlen(egg_version)]; \
  sprintf(p,"+%s",str); \
  egg_numver++; \
  sprintf(&egg_xtra[strlen(egg_xtra)]," %s",str); \
}

#define debug0(x) putlog(LOG_DEBUG,"*",x)
#define debug1(x,a1) putlog(LOG_DEBUG,"*",x,a1)
#define debug2(x,a1,a2) putlog(LOG_DEBUG,"*",x,a1,a2)
#define debug3(x,a1,a2,a3) putlog(LOG_DEBUG,"*",x,a1,a2,a3)
#define debug4(x,a1,a2,a3,a4) putlog(LOG_DEBUG,"*",x,a1,a2,a3,a4)

/***********************************************************************/

/* public structure for the listening port map */
struct portmap {
  int realport;
  int mappedto;
  struct portmap *next;
};

/* public structure of all the dcc connections */
struct dcc_table {
  char *name;
  int flags;
  void (*eof) (int);
  void (*activity) (int, char *, int);
  int *timeout_val;
  void (*timeout) ();
  void (*display) (int, char *);
  int (*expmem) (void *);
  void (*kill) (int, void *);
  void (*output) (int, char *, void *);
};

struct userrec;

struct dcc_t {
  long sock;			/* this should be a long to keep 64-bit machines sane */
  IP addr;
  unsigned int port;
  struct userrec *user;
  char nick[NICKLEN];
  char host[UHOSTLEN];
  struct dcc_table *type;
  time_t timeval;		/* use for any timing stuff 
				 * - this is used for timeout checking */
  unsigned long status;		/* A LOT of dcc types have status thingos, this
				 * makes it more avaliabe */
  union {
    struct chat_info *chat;
    struct file_info *file;
    struct edit_info *edit;
    struct xfer_info *xfer;
    struct bot_info *bot;
    struct relay_info *relay;
    struct script_info *script;
    int ident_sock;
    void *other;
  } u;				/* special use depending on type */
};

struct chat_info {
  char *away;			/* non-NULL if user is away */
  int msgs_per_sec;		/* used to stop flooding */
  int con_flags;		/* with console: what to show */
  int strip_flags;		/* what codes to strip (b,r,u,c,a,g,*) */
  char con_chan[81];		/* with console: what channel to view */
  int channel;			/* 0=party line, -1=off */
  struct msgq *buffer;		/* a buffer of outgoing lines (for .page cmd) */
  int max_line;			/* maximum lines at once */
  int line_count;		/* number of lines sent since last page */
  int current_lines;		/* number of lines total stored */
  char *su_nick;
};

struct file_info {
  struct chat_info *chat;
  char dir[161];
};

struct xfer_info {
  char filename[121];
  char dir[121];		/* used when uploads go to the current dir */
  unsigned long length;
  unsigned long acked;
  char buf[4];			/* you only need 5 bytes! */
  unsigned char sofar;		/* how much of the byte count received */
  char from[NICKLEN];		/* [GET] user who offered the file */
  FILE *f;			/* pointer to file being sent/received */
};

struct bot_info {
  char version[121];		/* channel/version info */
  char linker[NOTENAMELEN + 1];	/* who requested this link */
  int numver;
  int port;			/* base port */
};

struct relay_info {
  struct chat_info *chat;
  int sock;
  int port;
  int old_status;
};

struct script_info {
  struct dcc_table *type;
  union {
    struct chat_info *chat;
    struct file_info *file;
    void *other;
  } u;
  char command[121];
};

/* flags about dcc types */
#define DCT_CHAT      0x00000001	/* this dcc type receives botnet chatter */
#define DCT_MASTER    0x00000002	/* received master chatter */
#define DCT_SHOWWHO   0x00000004	/* show the user in .who */
#define DCT_REMOTEWHO 0x00000008	/* show in remote who */
#define DCT_VALIDIDX  0x00000010	/* valid idx for outputting to in tcl */
#define DCT_SIMUL     0x00000020	/* can be tcl_simul'd */
#define DCT_CANBOOT   0x00000040	/* can be booted */
#define DCT_GETNOTES  DCT_CHAT	/* can receive notes */
#define DCT_FILES     0x00000080	/* gratuitous hack ;) */
#define DCT_FORKTYPE  0x00000100	/* a forking type */
#define DCT_BOT       0x00000200	/* a bot connection of some sort... */
#define DCT_FILETRAN  0x00000400	/* a file transfer of some sort */
#define DCT_FILESEND  0x00000800	/* a sending file transfer, getting = !this */
#define DCT_LISTEN    0x00001000	/* a listening port of some sort */

/* for dcc chat & files: */
#define STAT_ECHO    1		/* echo commands back? */
#define STAT_DENY    2		/* bad username (ignore password & deny access) */
/*#define STAT_XFER    4       has 'x' flag on chat line */
#define STAT_CHAT    8		/* in file-system but may return */
#define STAT_TELNET  16		/* connected via telnet */
#define STAT_PARTY   32		/* only on party line via 'p' flag */
#define STAT_BOTONLY 64		/* telnet on bots-only connect */
#define STAT_USRONLY 128	/* telnet on users-only connect */
#define STAT_PAGE    256	/* page output to the user */

/* for stripping out mIRC codes */
#define STRIP_COLOR  1		/* remove mIRC color codes */
#define STRIP_BOLD   2		/* remove bold codes */
#define STRIP_REV    4		/* remove reverse video codes */
#define STRIP_UNDER  8		/* remove underline codes */
#define STRIP_ANSI   16		/* remove ALL ansi codes */
#define STRIP_BELLS  32		/* remote ctrl-g's */
#define STRIP_ALL    63		/* remove every damn thing! */

/* for dcc bot links: */
#define STAT_PINGED  0x01	/* waiting for ping to return */
#define STAT_SHARE   0x02	/* sharing user data with the bot */
#define STAT_CALLED  0x04	/* this bot called me */
#define STAT_OFFERED 0x08	/* offered her the user file */
#define STAT_SENDING 0x10	/* in the process of sending a user list */
#define STAT_GETTING 0x20	/* in the process of getting a user list */
#define STAT_WARNED  0x40	/* warned him about unleaflike behavior */
#define STAT_LEAF    0x80	/* this bot is a leaf only */
#define STAT_LINKING 0x100	/* the bot is currently going through the
				 * linking stage */
#define STAT_AGGRESSIVE 0x200	/* aggressively sharing with this bot */

/* chan & global */
#define FLOOD_PRIVMSG    0
#define FLOOD_NOTICE     1
#define FLOOD_CTCP       2
#define FLOOD_NICK       3
#define FLOOD_JOIN       4
#define FLOOD_KICK       5
#define FLOOD_DEOP       6
#define FLOOD_CHAN_MAX   7
#define FLOOD_GLOBAL_MAX 3

/* for local console: */
#define STDIN      0
#define STDOUT     1
#define STDERR     2

/* structure for internal logs */
typedef struct {
  char *filename;
  unsigned int mask;		/* what to send to this log */
  char *chname;			/* which channel */
  char szLast[MAX_LOG_LINE + 1];	/* for 'Last message repeated n times'
					 * stuff in misc.c/putlog() <cybah> */
  int Repeats;			/* number of times szLast has been repeated */
  unsigned int flags;		/* other flags <rtc> */
  FILE *f;			/* existing file */
} log_t;

/* logfile display flags */
#define LOG_MSGS   0x000001	/* m   msgs/notice/ctcps */
#define LOG_PUBLIC 0x000002	/* p   public msg/notice/ctcps */
#define LOG_JOIN   0x000004	/* j   channel joins/parts/etc */
#define LOG_MODES  0x000008	/* k   mode changes/kicks/bans */
#define LOG_CMDS   0x000010	/* c   user dcc or msg commands */
#define LOG_MISC   0x000020	/* o   other misc bot things */
#define LOG_BOTS   0x000040	/* b   bot notices */
#define LOG_RAW    0x000080	/* r   raw server stuff coming in */
#define LOG_FILES  0x000100	/* x   file transfer commands and stats */
#define LOG_LEV1   0x000200	/* 1   user log level */
#define LOG_LEV2   0x000400	/* 2   user log level */
#define LOG_LEV3   0x000800	/* 3   user log level */
#define LOG_LEV4   0x001000	/* 4   user log level */
#define LOG_LEV5   0x002000	/* 5   user log level */
#define LOG_LEV6   0x004000	/* 6   user log level */
#define LOG_LEV7   0x008000	/* 7   user log level */
#define LOG_LEV8   0x010000	/* 8   user log level */
#define LOG_SERV   0x020000	/* s   server information */
#define LOG_DEBUG  0x040000	/* d   debug */
#define LOG_WALL   0x080000	/* w   wallops */
#define LOG_SRVOUT 0x100000	/* v   server output */
#define LOG_BOTNET 0x200000	/* t   botnet traffic */
#define LOG_BOTSHARE 0x400000	/* h   share traffic */
#define LOG_ALL    0x7fffff	/* (dump to all logfiles) */
/* internal logfile flags */
#define LF_EXPIRING 0x000001	/* Logfile will be closed soon */

#define FILEDB_HIDE     1
#define FILEDB_UNHIDE   2
#define FILEDB_SHARE    3
#define FILEDB_UNSHARE  4

/* socket flags: */
#define SOCK_UNUSED     0x01	/* empty socket */
#define SOCK_BINARY     0x02	/* do not buffer input */
#define SOCK_LISTEN     0x04	/* listening port */
#define SOCK_CONNECT    0x08	/* connection attempt */
#define SOCK_NONSOCK    0x10	/* used for file i/o on debug */
#define SOCK_STRONGCONN 0x20	/* don't report success until sure */
#define SOCK_EOFD       0x40	/* it EOF'd recently during a write */
#define SOCK_PROXYWAIT	0x80	/* waiting for SOCKS traversal */

/* fake idx's for dprintf - these should be ridiculously large +ve nums */
#define DP_STDOUT       0x7FF1
#define DP_LOG          0x7FF2
#define DP_SERVER       0x7FF3
#define DP_HELP         0x7FF4
#define DP_STDERR       0x7FF5
#define DP_MODE         0x7FF6

#define NORMAL          0
#define QUICK           1

/* return codes for add_note */
#define NOTE_ERROR      0	/* error */
#define NOTE_OK         1	/* success */
#define NOTE_STORED     2	/* not online; stored */
#define NOTE_FULL       3	/* too many notes stored */
#define NOTE_TCL        4	/* tcl binding caught it */
#define NOTE_AWAY       5	/* away; stored */
#define NOTE_FWD        6	/* away; forwarded */
#define NOTE_REJECT     7	/* ignore mask matched */

#define STR_PROTECT     2
#define STR_DIR         1

#define HELP_DCC        1
#define HELP_TEXT       2
#define HELP_IRC        16

/* it's used in so many places, let's put it here */
typedef int (*Function) ();

/* this is used by the net module to keep track of sockets and what's
 * queued on them */
typedef struct {
  int sock;
  char flags;
  char *inbuf;
  char *outbuf;
  unsigned long outbuflen;	/* outbuf could be binary data */
} sock_list;

#endif
