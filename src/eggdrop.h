/*
 * eggdrop.h
 *   Eggdrop compile-time settings
 *
 *   IF YOU ALTER THIS FILE, YOU NEED TO RECOMPILE THE BOT.
 *
 * $Id: eggdrop.h,v 1.81 2011/07/09 15:07:48 thommey Exp $
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

#ifndef _EGG_EGGDROP_H
#define _EGG_EGGDROP_H

/*
 * If you're *only* going to link to new version bots (1.3.0 or higher)
 * then you can safely define this.
 */
#undef NO_OLD_BOTNET

/* DEBUG_CONTEXT is now set in config.h by configure */

/*
 * HANDLEN note:
 *       HANDLEN defines the maximum length a handle on the bot can be.
 *       Standard (and minimum) is 9 characters long.
 *
 *       Beware that using lengths over 9 chars is 'non-standard' and if
 *       you wish to link to other bots, they _must_ both have the same
 *       maximum handle length.
 *
 * NICKMAX note:
 *       You should leave this at 32 characters and modify nick-len in the
 *       configuration file instead.
 */
#define HANDLEN 9   /* valid values 9->NICKMAX  */
#define NICKMAX 32  /* valid values HANDLEN->32 */


/* Handy string lengths */
#define UHOSTMAX    291 + NICKMAX /* 32 (ident) + 3 (\0, !, @) + NICKMAX */
#define DIRMAX      512           /* paranoia                            */
#define LOGLINEMAX  767           /* for misc.c/putlog() <cybah>         */

/* Invalid characters */
#define BADNICKCHARS "-,+*=:!.@#;$%&"
#define BADHANDCHARS "-,+*=:!.@#;$%&"


/* Language stuff */
#define LANGDIR  "./language" /* language file directory                   */
#define BASELANG "english"    /* language which always gets loaded before
                                 all other languages. You do not want to
                                 change this.                              */


/* The 'configure' script should make this next part automatic, so you
 * shouldn't need to adjust anything below.
 */
#define NICKLEN      NICKMAX + 1
#define UHOSTLEN     UHOSTMAX + 1
#define DIRLEN       DIRMAX + 1
#define LOGLINELEN   LOGLINEMAX + 1
#define NOTENAMELEN  ((HANDLEN * 2) + 1)


/* We have to generate compiler errors in a weird way since not all compilers
 * support the #error preprocessor directive. */
#ifndef STDC_HEADERS
#  include "Error: Your system must have standard ANSI C headers."
#endif

#ifndef HAVE_VPRINTF
#  include "Error: You need vsprintf to compile eggdrop."
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

/* This allows us to make things a lot less messy in modules.c. */
#ifndef STATIC
#  if !defined(MODULES_OK) || (!defined(MOD_USE_DL) && !defined(MOD_USE_SHL) && !defined(MOD_USE_DYLD) && !defined(MOD_USE_RLD) && !defined(MOD_USE_LOADER))
#    include "Error: You can't compile with module support on this system (try make static)."
#  else
#    ifdef MOD_USE_DL
#      ifndef HAVE_DLOPEN
#        include "Error: We have detected that dlopen() should be used to load modules on this OS; but it was not found. Please use 'make static'."
#      endif
#      undef MOD_USE_SHL
#      undef MOD_USE_DYLD
#      undef MOD_USE_RLD
#      undef MOD_USE_LOADER
#    endif
#    ifdef MOD_USE_SHL
#      ifndef HAVE_SHL_LOAD
#        include "Error: We have detected that shl_load() should be used to load modules on this OS; but it was not found. Please use 'make static'."
#      endif
#      undef MOD_USE_DL
#      undef MOD_USE_DYLD
#      undef MOD_USE_RLD
#      undef MOD_USE_LOADER
#    endif
#    ifdef MOD_USE_DYLD
#      ifndef HAVE_NSLINKMODULE
#        include "Error: We have detected that NSLinkModule() should be used to load modules on this OS; but it was not found. Please use 'make static'."
#      endif
#      undef MOD_USE_DL
#      undef MOD_USE_SHL
#      undef MOD_USE_RLD
#      undef MOD_USE_LOADER
#    endif
#    ifdef MOD_USE_RLD
#      ifndef HAVE_RLD_LOAD
#        include "Error: We have detected that rld_load() should be used to load modules on this OS; but it was not found. Please use 'make static'."
#      endif
#      undef MOD_USE_DL
#      undef MOD_USE_SHL
#      undef MOD_USE_DYLD
#      undef MOD_USE_LOADER
#    endif
#    ifdef MOD_USE_LOADER
#      ifndef HAVE_LOAD
#        include "Error: We have detected that load() should be used to load modules on this OS; but it was not found. Please use 'make static'."
#      endif
#      undef MOD_USE_DL
#      undef MOD_USE_SHL
#      undef MOD_USE_DYLD
#      undef MOD_USE_RLD
#    endif
#  endif
#endif

#ifdef EGG_HANDLEN
#  undef HANDLEN
#  define HANDLEN EGG_HANDLEN
#endif

#if (NICKMAX < 9) || (NICKMAX > 32)
#  include "Error: Invalid NICKMAX value."
#endif

#if (HANDLEN < 9) || (HANDLEN > 32)
#  include "Error: Invalid HANDLEN value."
#endif

#if HANDLEN > NICKMAX
#  include "Error: HANDLEN must be <= NICKMAX."
#endif

#ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif

/* NAME_MAX is what POSIX defines, but BSD calls it MAXNAMLEN.
 * Use 255 if we can't find anything else.
 */
#ifndef NAME_MAX
#  ifdef MAXNAMLEN
#    define NAME_MAX    MAXNAMLEN
#  else
#    define NAME_MAX    255
#  endif
#endif

/* Almost every module needs some sort of time thingy, so... */
#ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else
#    include <time.h>
#  endif
#endif


/* Yikes...who would have thought finding a usable random() would be so much
 * trouble?
 * Note: random(), rand(), and lrand48() are *not* thread safe.
 *
 * QNX doesn't include random() and srandom() in libc.so, only in libc.a
 * So we can only use these functions in static builds on QNX.
 */
#if defined QNX_HACKS && defined MAKING_MODS
#  undef HAVE_RANDOM
#  undef HAVE_SRANDOM
#endif

#ifdef HAVE_RANDOM
  /* On systems with random(), RANDOM_MAX may or may not be defined.
   *
   * If RANDOM_MAX isn't defined, we use 0x7FFFFFFF (2^31-1), or 2147483647
   * since this follows the 4.3BSD and POSIX.1-2001 standards. This of course
   * assumes random() uses a 32 bit long int type per the standards.
   */
#  ifndef RANDOM_MAX
#    define RANDOM_MAX 0x7FFFFFFF  /* random() -- 2^31-1 */
#  endif
#else                              /* !HAVE_RANDOM */
   /* This shouldn't exist in this case, but just to be safe... */
#  ifdef RANDOM_MAX
#    undef RANDOM_MAX
#  endif
  /* If we don't have random() it's safe to assume we also don't have
   * srandom(), and we need both.
   */
#  ifdef HAVE_RAND
#    define random() rand()
#    define srandom(x) srand(x)
    /* Depending on the system int size, RAND_MAX can be either 0x7FFFFFFF
     * (2^31-1), or 2147483647 for a 32 bit int, or 0x7FFF (2^15-1), or
     * 32767 for a 16 bit int. The standards only state that RAND_MAX must
     * be _at least_ 32767 but some systems with 16 bit int define it as
     * 32767. See: SVr4, 4.3BSD, C89, C99, POSIX.1-2001.
     */
#    define RANDOM_MAX RAND_MAX    /* rand() -- 2^31-1 or 2^15-1 */
#  else                            /* !HAVE_RAND */
#    ifdef  HAVE_LRAND48
#      define random() lrand48()
#      define srandom(x) srand48(x)
      /* For lrand48() we define RANDOM_MAX as 0x7FFFFFFF (2^31-1), or
       * 2147483647 since this is what the SVr4 and POSIX.1-2001 standards
       * call for. Note: SVID 3 declares these functions as obsolete and
       * states rand() should be used instead.
       */
#      define RANDOM_MAX 0x7FFFFFFF /* lrand48() -- 2^31-1 */
#    else                          /* !HAVE_LRAND48 */
#      include "Error: Must define one of HAVE_RANDOM, HAVE_RAND, or HAVE_LRAND48"
#    endif                         /* HAVE_LRAND48 */
#  endif                           /* HAVE_RAND */
#endif                             /* HAVE_RANDOM */


/* Use high-order bits for getting the random integer. With a modern
 * random() implmentation, modulo would probably be sufficient, but on
 * systems lacking random(), it may just be a macro for an older rand()
 * function.
 */
#define randint(n) (unsigned long) (random() / (RANDOM_MAX + 1.0) * n)


#ifndef HAVE_SIGACTION /* old "weird signals" */
#  define sigaction sigvec
#  ifndef sa_handler
#    define sa_handler sv_handler
#    define sa_mask sv_mask
#    define sa_flags sv_flags
#  endif
#endif

#ifndef HAVE_SIGEMPTYSET
#  define sigemptyset(x) ((*(int *)(x))=0)
#endif

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif

/*
 *    Handy aliases for memory tracking and core dumps
 */

#define nstrdup(x)    n_strdup((x),__FILE__,__LINE__)
#define nmalloc(x)    n_malloc((x),__FILE__,__LINE__)
#define nrealloc(x,y) n_realloc((x),(y),__FILE__,__LINE__)
#define nfree(x)      n_free((x),__FILE__,__LINE__)

#ifdef DEBUG_CONTEXT
#  define Context           eggContext(__FILE__, __LINE__, NULL)
#  define ContextNote(note) eggContextNote(__FILE__, __LINE__, NULL, note)
#else
#  define Context           do {} while (0)
#  define ContextNote(note) do {} while (0)
#endif

#ifdef DEBUG_ASSERT
#  define Assert(expr) do {                                             \
          if (!(expr))                                                  \
            eggAssert(__FILE__, __LINE__, NULL);                        \
} while (0)
#else
#  define Assert(expr) do {} while (0)
#endif

#ifndef COMPILING_MEM
#  undef malloc
#  define malloc(x) dont_use_old_malloc(x)
#  undef free
#  define free(x)   dont_use_old_free(x)
#endif /* !COMPILING_MEM */

/* 32 bit type */
#if (SIZEOF_INT == 4)
typedef unsigned int u_32bit_t;
#else
#  if (SIZEOF_LONG == 4)
typedef unsigned long u_32bit_t;
#  else
#    include "Error: Can't find 32bit type."
#  endif
#endif

typedef unsigned short int u_16bit_t;
typedef unsigned char u_8bit_t;

/* IP type */
typedef u_32bit_t IP;

/* Debug logging macros */
#define debug0(x)             putlog(LOG_DEBUG,"*",x)
#define debug1(x,a1)          putlog(LOG_DEBUG,"*",x,a1)
#define debug2(x,a1,a2)       putlog(LOG_DEBUG,"*",x,a1,a2)
#define debug3(x,a1,a2,a3)    putlog(LOG_DEBUG,"*",x,a1,a2,a3)
#define debug4(x,a1,a2,a3,a4) putlog(LOG_DEBUG,"*",x,a1,a2,a3,a4)

/* These apparently are unsafe without recasting. */
#define egg_isdigit(x)  isdigit((int)  (unsigned char) (x))
#define egg_isxdigit(x) isxdigit((int) (unsigned char) (x))
#define egg_isascii(x)  isascii((int)  (unsigned char) (x))
#define egg_isspace(x)  isspace((int)  (unsigned char) (x))
#define egg_islower(x)  islower((int)  (unsigned char) (x))

/***********************************************************************/

/* It's used in so many places, let's put it here */
typedef intptr_t (*Function) ();
typedef int (*IntFunc) ();

/* Public structure for the listening port map */
struct portmap {
  int realport;
  int mappedto;
  struct portmap *next;
};

/* Public structure of all the dcc connections */
struct dcc_table {
  char *name;
  int flags;
  void (*eof) (int);
  void (*activity) (int, char *, int);
  int *timeout_val;
  void (*timeout) (int);
  void (*display) (int, char *);
  int (*expmem) (void *);
  void (*kill) (int, void *);
  void (*output) (int, char *, void *);
  void (*outdone) (int);
};

struct userrec;

struct dcc_t {
  long sock;                    /* This should be a long to keep 64-bit machines sane. */
  IP addr;                      /* IP address in host network byte order. */
  unsigned int port;
  struct userrec *user;
  char nick[NICKLEN];
  char host[UHOSTLEN];
  struct dcc_table *type;
  time_t timeval;               /* This is used for timeout checking. */
  unsigned long status;         /* A LOT of dcc types have status things; makes it more availabe. */
  union {
    struct chat_info *chat;
    struct file_info *file;
    struct edit_info *edit;
    struct xfer_info *xfer;
    struct bot_info *bot;
    struct relay_info *relay;
    struct script_info *script;
    struct dns_info *dns;
    struct dupwait_info *dupwait;
    int ident_sock;
    void *other;
  } u;                          /* Special use depending on type        */
};

struct chat_info {
  char *away;                   /* non-NULL if user is away             */
  int msgs_per_sec;             /* used to stop flooding                */
  int con_flags;                /* with console: what to show           */
  int strip_flags;              /* what codes to strip (b,r,u,c,a,g,*)  */
  char con_chan[81];            /* with console: what channel to view   */
  int channel;                  /* 0=party line, -1=off                 */
  struct msgq *buffer;          /* a buffer of outgoing lines
                                 * (for .page cmd)                      */
  int max_line;                 /* maximum lines at once                */
  int line_count;               /* number of lines sent since last page */
  int current_lines;            /* number of lines total stored         */
  char *su_nick;
};

struct file_info {
  struct chat_info *chat;
  char dir[161];
};

struct xfer_info {
  char *filename;
  char *origname;
  char dir[DIRLEN];             /* used when uploads go to the current dir */
  unsigned long length;
  unsigned long acked;
  char buf[4];                  /* you only need 5 bytes!                  */
  unsigned char sofar;          /* how much of the byte count received     */
  char from[NICKLEN];           /* [GET] user who offered the file         */
  FILE *f;                      /* pointer to file being sent/received     */
  unsigned int type;            /* xfer connection type, see enum below    */
  unsigned short ack_type;      /* type of ack                             */
  unsigned long offset;         /* offset from beginning of file, during
                                 * resend.                                 */
  unsigned long block_pending;  /* bytes of this DCC block which weren't
                                 * sent yet.                               */
  time_t start_time;            /* Time when a xfer was started.           */
};

enum {                          /* transfer connection handling a ...   */
  XFER_SEND,                    /*  ... normal file-send to s.o.        */
  XFER_RESEND,                  /*  ... file-resend to s.o.             */
  XFER_RESEND_PEND,             /*  ... (as above) and waiting for info */
  XFER_RESUME,                  /*  ... file-send-resume to s.o.        */
  XFER_RESUME_PEND,             /*  ... (as above) and waiting for conn */
  XFER_GET                      /*  ... file-get from s.o.              */
};

enum {
  XFER_ACK_UNKNOWN,             /* We don't know how blocks are acked.  */
  XFER_ACK_WITH_OFFSET,         /* Skipped data is also counted as
                                 * received.                            */
  XFER_ACK_WITHOUT_OFFSET       /* Skipped data is NOT counted in ack.  */
};

struct bot_info {
  char version[121];            /* channel/version info                 */
  char linker[NOTENAMELEN + 1]; /* who requested this link              */
  int numver;
  int port;                     /* base port                            */
  int uff_flags;                /* user file feature flags              */
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

struct dns_info {
  void (*dns_success) (int);    /* is called if the dns request succeeds   */
  void (*dns_failure) (int);    /* is called if it fails                   */
  char *host;                   /* hostname                                */
  char *cbuf;                   /* temporary buffer. Memory will be free'd
                                 * as soon as dns_info is free'd           */
  char *cptr;                   /* temporary pointer                       */
  IP ip;                        /* IP address                              */
  int ibuf;                     /* temporary buffer for one integer        */
  char dns_type;                /* lookup type, e.g. RES_HOSTBYIP          */
  struct dcc_table *type;       /* type of the dcc table we are making the
                                 * lookup for                              */
};

/* Flags for dns_type. */
#define RES_HOSTBYIP  1         /* hostname to IP address               */
#define RES_IPBYHOST  2         /* IP address to hostname               */

struct dupwait_info {
  int atr;                      /* the bots attributes                  */
  struct chat_info *chat;       /* holds current chat data              */
};

/* Flags for dcc types. */
#define DCT_CHAT      0x00000001        /* this dcc type receives botnet
                                         * chatter                          */
#define DCT_MASTER    0x00000002        /* received master chatter          */
#define DCT_SHOWWHO   0x00000004        /* show the user in .who            */
#define DCT_REMOTEWHO 0x00000008        /* show in remote who               */
#define DCT_VALIDIDX  0x00000010        /* valid idx for outputting to
                                         * in tcl                           */
#define DCT_SIMUL     0x00000020        /* can be tcl_simul'd               */
#define DCT_CANBOOT   0x00000040        /* can be booted                    */
#define DCT_GETNOTES  DCT_CHAT          /* can receive notes                */
#define DCT_FILES     0x00000080        /* gratuitous hack ;)               */
#define DCT_FORKTYPE  0x00000100        /* a forking type                   */
#define DCT_BOT       0x00000200        /* a bot connection of some sort... */
#define DCT_FILETRAN  0x00000400        /* a file transfer of some sort     */
#define DCT_FILESEND  0x00000800        /* a sending file transfer,
                                         * getting = !this                  */
#define DCT_LISTEN    0x00001000        /* a listening port of some sort    */

/* For dcc chat & files. */
#define STAT_ECHO    0x00001    /* echo commands back?                  */
#define STAT_DENY    0x00002    /* bad username (ignore password & deny
                                 * access)                              */
#define STAT_CHAT    0x00004    /* in file-system but may return        */
#define STAT_TELNET  0x00008    /* connected via telnet                 */
#define STAT_PARTY   0x00010    /* only on party line via 'p' flag      */
#define STAT_BOTONLY 0x00020    /* telnet on bots-only connect          */
#define STAT_USRONLY 0x00040    /* telnet on users-only connect         */
#define STAT_PAGE    0x00080    /* page output to the user              */

/* For stripping out mIRC codes. */
#define STRIP_COLOR  0x00001    /* remove mIRC color codes              */
#define STRIP_BOLD   0x00002    /* remove bold codes                    */
#define STRIP_REV    0x00004    /* remove reverse video codes           */
#define STRIP_UNDER  0x00008    /* remove underline codes               */
#define STRIP_ANSI   0x00010    /* remove ALL ANSI codes                */
#define STRIP_BELLS  0x00020    /* remove ctrl-g's                      */
#define STRIP_ALL    0x0007F    /* remove every damn thing!             */

/* For dcc bot links. */
#define STAT_PINGED  0x00001    /* waiting for ping to return            */
#define STAT_SHARE   0x00002    /* sharing user data with the bot        */
#define STAT_CALLED  0x00004    /* this bot called me                    */
#define STAT_OFFERED 0x00008    /* offered her the user file             */
#define STAT_SENDING 0x00010    /* in the process of sending a user list */
#define STAT_GETTING 0x00020    /* in the process of getting a user list */
#define STAT_WARNED  0x00040    /* warned him about unleaflike behavior  */
#define STAT_LEAF    0x00080    /* this bot is a leaf only               */
#define STAT_LINKING 0x00100    /* the bot is currently going through
                                 * the linking stage                     */
#define STAT_AGGRESSIVE 0x00200 /* aggressively sharing with this bot    */

/* Flags for listening sockets */
#define LSTN_PUBLIC  0x000001   /* No access restrictions               */

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

/* For local console: */
#define STDIN  0
#define STDOUT 1
#define STDERR 2

/* Structure for internal logs */
typedef struct {
  char *filename;
  unsigned int mask;            /* what to send to this log                 */
  char *chname;                 /* which channel                            */
  char szlast[LOGLINELEN];      /* for 'Last message repeated n times'
                                 * stuff in misc.c/putlog() <cybah>         */
  int repeats;                  /* number of times szLast has been repeated */
  unsigned int flags;           /* other flags <rtc>                        */
  FILE *f;                      /* existing file                            */
} log_t;

/* Logfile display flags
 */
#define LOG_MSGS     0x000001   /* m   private msgs/notices/ctcps       */
#define LOG_PUBLIC   0x000002   /* p   public  msgs/notices/ctcps       */
#define LOG_JOIN     0x000004   /* j   channel joins/parts/etc          */
#define LOG_MODES    0x000008   /* k   mode changes/kicks/bans          */
#define LOG_CMDS     0x000010   /* c   user dcc or msg commands         */
#define LOG_MISC     0x000020   /* o   other misc bot things            */
#define LOG_BOTS     0x000040   /* b   bot notices                      */
#define LOG_RAW      0x000080   /* r   raw server stuff coming in       */
#define LOG_FILES    0x000100   /* x   file transfer commands and stats */
#define LOG_LEV1     0x000200   /* 1   user log level                   */
#define LOG_LEV2     0x000400   /* 2   user log level                   */
#define LOG_LEV3     0x000800   /* 3   user log level                   */
#define LOG_LEV4     0x001000   /* 4   user log level                   */
#define LOG_LEV5     0x002000   /* 5   user log level                   */
#define LOG_LEV6     0x004000   /* 6   user log level                   */
#define LOG_LEV7     0x008000   /* 7   user log level                   */
#define LOG_LEV8     0x010000   /* 8   user log level                   */
#define LOG_SERV     0x020000   /* s   server information               */
#define LOG_DEBUG    0x040000   /* d   debug                            */
#define LOG_WALL     0x080000   /* w   wallops                          */
#define LOG_SRVOUT   0x100000   /* v   server output                    */
#define LOG_BOTNET   0x200000   /* t   botnet traffic                   */
#define LOG_BOTSHARE 0x400000   /* h   share traffic                    */
#define LOG_ALL      0x7fffff   /* (dump to all logfiles)               */

/* Internal logfile flags
 */
#define LF_EXPIRING 0x000001    /* Logfile will be closed soon          */

#define FILEDB_HIDE     1
#define FILEDB_UNHIDE   2
#define FILEDB_SHARE    3
#define FILEDB_UNSHARE  4

/* Socket flags:
 */
#define SOCK_UNUSED     0x0001  /* empty socket                         */
#define SOCK_BINARY     0x0002  /* do not buffer input                  */
#define SOCK_LISTEN     0x0004  /* listening port                       */
#define SOCK_CONNECT    0x0008  /* connection attempt                   */
#define SOCK_NONSOCK    0x0010  /* used for file i/o on debug           */
#define SOCK_STRONGCONN 0x0020  /* don't report success until sure      */
#define SOCK_EOFD       0x0040  /* it EOF'd recently during a write     */
#define SOCK_PROXYWAIT  0x0080  /* waiting for SOCKS traversal          */
#define SOCK_PASS       0x0100  /* passed on; only notify in case
                                 * of traffic                           */
#define SOCK_VIRTUAL    0x0200  /* not-connected socket (dont read it!) */
#define SOCK_BUFFER     0x0400  /* buffer data; don't notify dcc funcs  */
#define SOCK_TCL        0x0800  /* tcl socket, don't do anything on it  */

/* Flags to sock_has_data
 */
enum {
  SOCK_DATA_OUTGOING,           /* Data in out-queue?                   */
  SOCK_DATA_INCOMING            /* Data in in-queue?                    */
};

/* Fake idx's for dprintf - these should be ridiculously large +ve nums
 */
#define DP_STDOUT       0x7FF1
#define DP_LOG          0x7FF2
#define DP_SERVER       0x7FF3
#define DP_HELP         0x7FF4
#define DP_STDERR       0x7FF5
#define DP_MODE         0x7FF6
#define DP_MODE_NEXT    0x7FF7
#define DP_SERVER_NEXT  0x7FF8
#define DP_HELP_NEXT    0x7FF9

#define NORMAL          0
#define QUICK           1

/* Return codes for add_note */
#define NOTE_ERROR      0       /* error                        */
#define NOTE_OK         1       /* success                      */
#define NOTE_STORED     2       /* not online; stored           */
#define NOTE_FULL       3       /* too many notes stored        */
#define NOTE_TCL        4       /* tcl binding caught it        */
#define NOTE_AWAY       5       /* away; stored                 */
#define NOTE_FWD        6       /* away; forwarded              */
#define NOTE_REJECT     7       /* ignore mask matched          */

#define STR_PROTECT     2
#define STR_DIR         1

#define HELP_DCC        1
#define HELP_TEXT       2
#define HELP_IRC        16

/* These are used by the net module to keep track of sockets and what's
 * queued on them
 */
struct sock_handler {
  char *inbuf;
  char *outbuf;
  unsigned long outbuflen;      /* Outbuf could be binary data  */
  unsigned long inbuflen;       /* Inbuf could be binary data   */
};

struct tclsock_handler {
  int mask;                     /* desired events               */
  Tcl_FileProc *proc;
  ClientData cd;
};

typedef struct sock_list {
  int sock;
  short flags;
  union {
    struct sock_handler sock;
    struct tclsock_handler tclsock;
  } handler;
} sock_list;

enum {
  EGG_OPTION_SET = 1,           /* Set option(s).               */
  EGG_OPTION_UNSET = 2          /* Unset option(s).             */
};

/* Telnet codes.  See "TELNET Protocol Specification" (RFC 854) and
 * "TELNET Echo Option" (RFC 875) for details. */

/* Telnet commands */
#define TLN_AYT         246     /* Are You There        */
#define TLN_WILL        251     /* Will                 */
#define TLN_WILL_C      "\373"
#define TLN_WONT        252     /* Won't                */
#define TLN_WONT_C      "\374"
#define TLN_DO          253     /* Do                   */
#define TLN_DO_C        "\375"
#define TLN_DONT        254     /* Don't                */
#define TLN_DONT_C      "\376"
#define TLN_IAC         255     /* Interpret As Command */
#define TLN_IAC_C       "\377"

/* Telnet options */
#define TLN_ECHO        1       /* Echo (RFC 875)        */
#define TLN_ECHO_C      "\001"
#define TLN_STATUS      5       /* STATUS (RFC 859)      */
#define TLN_STATUS_C    "\005"

#endif /* _EGG_EGGDROP_H */
