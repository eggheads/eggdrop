/*
 * misc.c -- handles:
 *   split() maskhost() dumplots() daysago() days() daysdur()
 *   logging things
 *   queueing output for the bot (msg and help)
 *   resync buffers for sharebots
 *   help system
 *   motd display and %var substitution
 *
 * $Id: misc.c,v 1.88 2011/07/09 15:07:48 thommey Exp $
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

#include "main.h"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include "chan.h"
#include "tandem.h"
#include "modules.h"

#ifdef HAVE_UNAME
#  include <sys/utsname.h>
#endif

#include "stat.h"

extern struct dcc_t *dcc;
extern struct chanset_t *chanset;

extern char helpdir[], version[], origbotname[], botname[], admin[], network[],
            motdfile[], ver[], botnetnick[], bannerfile[], logfile_suffix[],
            textdir[];
extern int  backgrd, con_chan, term_z, use_stderr, dcc_total, keep_all_logs,
            quick_logs, strict_host;

extern time_t now;
extern Tcl_Interp *interp;

char log_ts[32] = "[%H:%M:%S]"; /* Timestamp format for logfile entries */

int shtime = 1;                 /* Display the time with console output */
log_t *logs = 0;                /* Logfiles */
int max_logs = 5;               /* Current maximum log files */
int max_logsize = 0;            /* Maximum logfile size, 0 for no limit */
int raw_log = 0;                /* Disply output to server to LOG_SERVEROUT */

int conmask = LOG_MODES | LOG_CMDS | LOG_MISC; /* Console mask */

struct help_list_t {
  struct help_list_t *next;
  char *name;
  int type;
};

static struct help_ref {
  char *name;
  struct help_list_t *first;
  struct help_ref *next;
} *help_list = NULL;


/* Expected memory usage
 */
int expmem_misc()
{
  struct help_ref *current;
  struct help_list_t *item;
  int tot = 0;

  for (current = help_list; current; current = current->next) {
    tot += sizeof(struct help_ref) + strlen(current->name) + 1;

    for (item = current->first; item; item = item->next)
      tot += sizeof(struct help_list_t) + strlen(item->name) + 1;
  }
  return tot + (max_logs * sizeof(log_t));
}

void init_misc()
{
  static int last = 0;

  if (max_logs < 1)
    max_logs = 1;
  if (logs)
    logs = nrealloc(logs, max_logs * sizeof(log_t));
  else
    logs = nmalloc(max_logs * sizeof(log_t));
  for (; last < max_logs; last++) {
    logs[last].filename = logs[last].chname = NULL;
    logs[last].mask = 0;
    logs[last].f = NULL;
    /* Added by cybah  */
    logs[last].szlast[0] = 0;
    logs[last].repeats = 0;
    /* Added by rtc  */
    logs[last].flags = 0;
  }
}


/*
 *    Misc functions
 */

/* low-level stuff for other modules
 */
int is_file(const char *s)
{
  struct stat ss;
  int i = stat(s, &ss);

  if (i < 0)
    return 0;
  if ((ss.st_mode & S_IFREG) || (ss.st_mode & S_IFLNK))
    return 1;
  return 0;
}

/*  This implementation wont overrun dst - 'max' is the max bytes that dst
 *  can be, including the null terminator. So if 'dst' is a 128 byte buffer,
 *  pass 128 as 'max'. The function will _always_ null-terminate 'dst'.
 *
 *  Returns: The number of characters appended to 'dst'.
 *
 *  Usage example:
 *
 *    char buf[128];
 *    size_t bufsize = sizeof(buf);
 *
 *    buf[0] = 0, bufsize--;
 *
 *    while (blah && bufsize) {
 *      bufsize -= egg_strcatn(buf, <some-long-string>, sizeof(buf));
 *    }
 *
 *  <Cybah>
 */
int egg_strcatn(char *dst, const char *src, size_t max)
{
  size_t tmpmax = 0;

  /* find end of 'dst' */
  while (*dst && max > 0) {
    dst++;
    max--;
  }

  /*    Store 'max', so we can use it to workout how many characters were
   *  written later on.
   */
  tmpmax = max;

  /* copy upto, but not including the null terminator */
  while (*src && max > 1) {
    *dst++ = *src++;
    max--;
  }

  /* null-terminate the buffer */
  *dst = 0;

  /*    Don't include the terminating null in our count, as it will cumulate
   *  in loops - causing a headache for the caller.
   */
  return tmpmax - max;
}

int my_strcpy(register char *a, register char *b)
{
  register char *c = b;

  while (*b)
    *a++ = *b++;
  *a = *b;
  return b - c;
}

/* Split first word off of rest and put it in first
 */
void splitc(char *first, char *rest, char divider)
{
  char *p = strchr(rest, divider);

  if (p == NULL) {
    if (first != rest && first)
      first[0] = 0;
    return;
  }
  *p = 0;
  if (first != NULL)
    strcpy(first, rest);
  if (first != rest)
    /*    In most circumstances, strcpy with src and dst being the same buffer
     *  can produce undefined results. We're safe here, as the src is
     *  guaranteed to be at least 2 bytes higher in memory than dest. <Cybah>
     */
    strcpy(rest, p + 1);
}

/*    As above, but lets you specify the 'max' number of bytes (EXCLUDING the
 * terminating null).
 *
 * Example of use:
 *
 * char buf[HANDLEN + 1];
 *
 * splitcn(buf, input, "@", HANDLEN);
 *
 * <Cybah>
 */
void splitcn(char *first, char *rest, char divider, size_t max)
{
  char *p = strchr(rest, divider);

  if (p == NULL) {
    if (first != rest && first)
      first[0] = 0;
    return;
  }
  *p = 0;
  if (first != NULL)
    strncpyz(first, rest, max);
  if (first != rest)
    /*    In most circumstances, strcpy with src and dst being the same buffer
     *  can produce undefined results. We're safe here, as the src is
     *  guaranteed to be at least 2 bytes higher in memory than dest. <Cybah>
     */
    strcpy(rest, p + 1);
}

char *splitnick(char **blah)
{
  char *p = strchr(*blah, '!'), *q = *blah;

  if (p) {
    *p = 0;
    *blah = p + 1;
    return q;
  }
  return "";
}

void remove_crlf(char **line)
{
  char *p;

  p = strchr(*line, '\n');
  if (p != NULL)
    *p = 0;
  p = strchr(*line, '\r');
  if (p != NULL)
    *p = 0;
}

char *newsplit(char **rest)
{
  register char *o, *r;

  if (!rest)
    return "";
  o = *rest;
  while (*o == ' ')
    o++;
  r = o;
  while (*o && (*o != ' '))
    o++;
  if (*o)
    *o++ = 0;
  *rest = o;
  return r;
}

/* maskhost(), modified to support custom mask types, as defined
 * by mIRC.
 * Does not require a proper hostmask in 's'. Accepts any strings,
 * including empty ones and attempts to provide meaningful results.
 *
 * Strings containing no '@' character will be parsed as if the
 * whole string is a host.
 * Strings containing no '!' character will be interpreted as if
 * there is no nick.
 * '!' as a nick/user separator must precede any '@' characters.
 * Otherwise it will be considered a part of the host.
 * Supported types are listed in tcl-commands.doc in the maskhost
 * command section. Type 3 resembles the older maskhost() most closely.
 *
 * Specific examples (with type=3):
 *
 * "nick!user@is.the.lamest.bg"  -> *!*user@*.the.lamest.bg (ccTLD)
 * "nick!user@is.the.lamest.com" -> *!*user@*.lamest.com (gTLD)
 * "lamest.example"	         -> *!*@lamest.example
 * "whatever@lamest.example"     -> *!*whatever@lamest.example
 * "com.example@user!nick"       -> *!*com.example@user!nick
 * "!"			         -> *!*@!
 * "@"			         -> *!*@*
 * ""				 -> *!*@*
 * "abc!user@2001:db8:618:5c0:263:15:dead:babe"
 * -> *!*user@2001:db8:618:5c0:263:15:dead:*
 * "abc!user@0:0:0:0:0:ffff:1.2.3.4"
 * -> *!*user@0:0:0:0:0:ffff:1.2.3.*
 */
void maskaddr(const char *s, char *nw, int type)
{
  int d = type % 5, num = 1;
  register char *p, *u = 0, *h = 0, *ss;

  /* Look for user and host.. */
  ss = (char *)s;
  u = strchr(s, '!');
  if (u)
    h = strchr(u, '@');
  if (!h){
    h = strchr(s, '@');
    u = 0;
  }

  /* Print nick if required and available */
  if (!u || (type % 10) < 5)
    *nw++ = '*';
  else {
    strncpy(nw, s, u - s);
    nw += u - s;
  }
  *nw++ = '!';

  /* Write user if required and available */
  u = (u ? u + 1 : ss);
  if (!h || (d == 2) || (d == 4))
    *nw++ = '*';
  else {
    if (d) {
      *nw++ = '*';
      if (strchr("~+-^=", *u))
        u++; /* trim leading crap */
    }
    strncpy(nw, u, h - u);
    nw += h - u;
  }
  *nw++ = '@';

  /* The rest is for the host */
  h = (h ? h + 1 : ss);
  for (p = h; *p; p++) /* hostname? */
    if ((*p > '9' || *p < '0') && *p != '.') {
      num = 0;
      break;
    }
  p = strrchr(h, ':'); /* IPv6? */
  /* Mask out after the last colon/dot */
  if (p && d > 2) {
    if ((u = strrchr(p, '.')))
      p = u;
    strncpy(nw, h, ++p - h);
    nw += p - h;
    strcpy(nw, "*");
  } else if (!p && !num && type >= 10) {
      /* we have a hostname and type
       requires us to replace numbers */
    num = 0;
    for (p = h; *p; p++) {
      if (*p < '0' || *p > '9') {
        *nw++ = *p;
        num = 0;
      } else {
        if (type < 20)
          *nw++ = '?';
        else if (!num) {
          *nw++ = '*';
          num = 1; /* place only one '*'
                      per numeric sequence */
        }
      }
    }
    *nw = 0;
  } else if (d > 2 && (p = strrchr(h, '.'))) {
    if (num) { /* IPv4 */
      strncpy(nw, h, p - h);
      nw += p - h;
      strcpy(nw, ".*");
      return;
    }
    for (u = h, d = 0; (u = strchr(++u, '.')); d++);
    if (d < 2) { /* types < 2 don't mask the host */
      strcpy(nw, h);
      return;
    }
    u = strchr(h, '.');
    if (d > 3 || (d == 3 && strlen(p) > 3))
      u = strchr(++u, '.'); /* ccTLD or not? Look above. */
    sprintf(nw, "*%s", u);
  } else if (!*h)
      /* take care if the mask is empty or contains only '@' */
      strcpy(nw, "*");
    else
      strcpy(nw, h);
}

/* Dump a potentially super-long string of text.
 */
void dumplots(int idx, const char *prefix, const char *data)
{
  const char *p = data, *q, *n;
  const int max_data_len = 500 - strlen(prefix);

  if (!*data) {
    dprintf(idx, "%s\n", prefix);
    return;
  }
  while (strlen(p) > max_data_len) {
    q = p + max_data_len;
    /* Search for embedded linefeed first */
    n = strchr(p, '\n');
    if (n && n < q) {
      /* Great! dump that first line then start over */
      dprintf(idx, "%s%.*s\n", prefix, n - p, p);
      p = n + 1;
    } else {
      /* Search backwards for the last space */
      while (*q != ' ' && q != p)
        q--;
      if (q == p)
        q = p + max_data_len;
      dprintf(idx, "%s%.*s\n", prefix, q - p, p);
      p = q;
      if (*q == ' ')
        p++;
    }
  }
  /* Last trailing bit: split by linefeeds if possible */
  n = strchr(p, '\n');
  while (n) {
    dprintf(idx, "%s%.*s\n", prefix, n - p, p);
    p = n + 1;
    n = strchr(p, '\n');
  }
  if (*p)
    dprintf(idx, "%s%s\n", prefix, p);  /* Last trailing bit */
}

/* Convert an interval (in seconds) to one of:
 * "19 days ago", "1 day ago", "18:12"
 */
void daysago(time_t now, time_t then, char *out)
{
  if (now - then > 86400) {
    int days = (now - then) / 86400;

    sprintf(out, "%d day%s ago", days, (days == 1) ? "" : "s");
    return;
  }
  egg_strftime(out, 6, "%H:%M", localtime(&then));
}

/* Convert an interval (in seconds) to one of:
 * "in 19 days", "in 1 day", "at 18:12"
 */
void days(time_t now, time_t then, char *out)
{
  if (now - then > 86400) {
    int days = (now - then) / 86400;

    sprintf(out, "in %d day%s", days, (days == 1) ? "" : "s");
    return;
  }
  egg_strftime(out, 9, "at %H:%M", localtime(&now));
}

/* Convert an interval (in seconds) to one of:
 * "for 19 days", "for 1 day", "for 09:10"
 */
void daysdur(time_t now, time_t then, char *out)
{
  char s[81];
  int hrs, mins;

  if (now - then > 86400) {
    int days = (now - then) / 86400;

    sprintf(out, "for %d day%s", days, (days == 1) ? "" : "s");
    return;
  }
  strcpy(out, "for ");
  now -= then;
  hrs = (int) (now / 3600);
  mins = (int) ((now - (hrs * 3600)) / 60);
  sprintf(s, "%02d:%02d", hrs, mins);
  strcat(out, s);
}


/*
 *    Logging functions
 */

/* Log something
 * putlog(level,channel_name,format,...);
 */
void putlog EGG_VARARGS_DEF(int, arg1)
{
  static int inhere = 0;
  int i, type, tsl = 0;
  char *format, *chname, s[LOGLINELEN], s1[256], *out, ct[81], *s2, stamp[34];
  va_list va;
  time_t now2 = time(NULL);
  struct tm *t = localtime(&now2);

  type = EGG_VARARGS_START(int, arg1, va);
  chname = va_arg(va, char *);
  format = va_arg(va, char *);

  /* Create the timestamp */
  t = localtime(&now2);
  if (shtime) {
    egg_strftime(stamp, sizeof(stamp) - 2, log_ts, t);
    strcat(stamp, " ");
    tsl = strlen(stamp);
  }
  else
    *stamp = '\0';

  /* Format log entry at offset 'tsl,' then i can prepend the timestamp */
  out = s + tsl;
  /* No need to check if out should be null-terminated here,
   * just do it! <cybah>
   */
  egg_vsnprintf(out, LOGLINEMAX - tsl, format, va);
  out[LOGLINEMAX - tsl] = 0;
  if (keep_all_logs) {
    if (!logfile_suffix[0])
      egg_strftime(ct, 12, ".%d%b%Y", t);
    else {
      egg_strftime(ct, 80, logfile_suffix, t);
      ct[80] = 0;
      s2 = ct;
      /* replace spaces by underscores */
      while (s2[0]) {
        if (s2[0] == ' ')
          s2[0] = '_';
        s2++;
      }
    }
  }
  /* Make sure the bind list is initialized and we're not looping here */
  if (!inhere && H_log) {
    inhere = 1;
    check_tcl_log(type, chname, out);
    inhere = 0;
  }
  /* Place the timestamp in the string to be printed */
  if (out[0] && shtime) {
    strncpy(s, stamp, tsl);
    out = s;
  }
  strcat(out, "\n");
  if (!use_stderr) {
    for (i = 0; i < max_logs; i++) {
      if ((logs[i].filename != NULL) && (logs[i].mask & type) &&
          ((chname[0] == '*') || (logs[i].chname[0] == '*') ||
           (!rfc_casecmp(chname, logs[i].chname)))) {
        if (logs[i].f == NULL) {
          /* Open this logfile */
          if (keep_all_logs) {
            egg_snprintf(s1, 256, "%s%s", logs[i].filename, ct);
            logs[i].f = fopen(s1, "a+");
          } else
            logs[i].f = fopen(logs[i].filename, "a+");
        }
        if (logs[i].f != NULL) {
          /* Check if this is the same as the last line added to
           * the log. <cybah>
           */
          if (!egg_strcasecmp(out + tsl, logs[i].szlast))
            /* It is a repeat, so increment repeats */
            logs[i].repeats++;
          else {
            /* Not a repeat, check if there were any repeat
             * lines previously...
             */
            if (logs[i].repeats > 0) {
              /* Yep.. so display 'last message repeated x times'
               * then reset repeats. We want the current time here,
               * so put that in the file first.
               */
              fprintf(logs[i].f, stamp);
              fprintf(logs[i].f, MISC_LOGREPEAT, logs[i].repeats);
              logs[i].repeats = 0;
              /* No need to reset logs[i].szlast here
               * because we update it later on...
               */
            }
            fputs(out, logs[i].f);
            strncpyz(logs[i].szlast, out + tsl, LOGLINEMAX);
          }
        }
      }
    }
  }
  for (i = 0; i < dcc_total; i++) {
    if ((dcc[i].type == &DCC_CHAT) && (dcc[i].u.chat->con_flags & type)) {
      if ((chname[0] == '*') || (dcc[i].u.chat->con_chan[0] == '*') ||
          !rfc_casecmp(chname, dcc[i].u.chat->con_chan)) {
        dprintf(i, "%s", out);
      }
    }
  }
  if (!backgrd && !con_chan && !term_z)
    dprintf(DP_STDOUT, "%s", out);
  else if ((type & LOG_MISC) && use_stderr) {
    if (shtime)
      out += tsl;
    dprintf(DP_STDERR, "%s", s);
  }
  va_end(va);
}

/* Called as soon as the logfile suffix changes. All logs are closed
 * and the new suffix is stored in `logfile_suffix'.
 */
void logsuffix_change(char *s)
{
  int i;
  char *s2 = logfile_suffix;

  /* If the suffix didn't really change, ignore. It's probably a rehash. */
  if (s && s2 && !strcmp(s, s2))
    return;

  debug0("Logfile suffix changed. Closing all open logs.");
  strcpy(logfile_suffix, s);
  while (s2[0]) {
    if (s2[0] == ' ')
      s2[0] = '_';
    s2++;
  }
  for (i = 0; i < max_logs; i++) {
    if (logs[i].f) {
      fflush(logs[i].f);
      fclose(logs[i].f);
      logs[i].f = NULL;
    }
  }
}

void check_logsize()
{
  struct stat ss;
  int i;

/* int x=1; */
  char buf[1024];               /* Should be plenty */

  if (!keep_all_logs && max_logsize > 0) {
    for (i = 0; i < max_logs; i++) {
      if (logs[i].filename) {
        if (stat(logs[i].filename, &ss) != 0) {
          break;
        }
        if ((ss.st_size >> 10) > max_logsize) {
          if (logs[i].f) {
            /* write to the log before closing it huh.. */
            putlog(LOG_MISC, "*", MISC_CLOGS, logs[i].filename, ss.st_size);
            fflush(logs[i].f);
            fclose(logs[i].f);
            logs[i].f = NULL;
          }

          egg_snprintf(buf, sizeof buf, "%s.yesterday", logs[i].filename);
          buf[1023] = 0;
          unlink(buf);
          movefile(logs[i].filename, buf);
        }
      }
    }
  }
}

/* Flush the logfiles to disk
 */
void flushlogs()
{
  int i;

  /* Logs may not be initialised yet. */
  if (!logs)
    return;

  /* Now also checks to see if there's a repeat message and
   * displays the 'last message repeated...' stuff too <cybah>
   */
  for (i = 0; i < max_logs; i++) {
    if (logs[i].f != NULL) {
      if ((logs[i].repeats > 0) && quick_logs) {
        /* Repeat.. if quicklogs used then display 'last message
         * repeated x times' and reset repeats.
         */
        char stamp[33];

        egg_strftime(stamp, sizeof(stamp) - 1, log_ts, localtime(&now));
        fprintf(logs[i].f, "%s ", stamp);
        fprintf(logs[i].f, MISC_LOGREPEAT, logs[i].repeats);
        /* Reset repeats */
        logs[i].repeats = 0;
      }
      fflush(logs[i].f);
    }
  }
}


/*
 *     String substitution functions
 */

static int cols = 0;
static int colsofar = 0;
static int blind = 0;
static int subwidth = 70;
static char *colstr = NULL;


/* Add string to colstr
 */
static void subst_addcol(char *s, char *newcol)
{
  char *p, *q;
  int i, colwidth;

  if ((newcol[0]) && (newcol[0] != '\377'))
    colsofar++;
  colstr = nrealloc(colstr, strlen(colstr) + strlen(newcol) +
                    (colstr[0] ? 2 : 1));
  if ((newcol[0]) && (newcol[0] != '\377')) {
    if (colstr[0])
      strcat(colstr, "\377");
    strcat(colstr, newcol);
  }
  if ((colsofar == cols) || ((newcol[0] == '\377') && (colstr[0]))) {
    colsofar = 0;
    strcpy(s, "     ");
    colwidth = (subwidth - 5) / cols;
    q = colstr;
    p = strchr(colstr, '\377');
    while (p != NULL) {
      *p = 0;
      strcat(s, q);
      for (i = strlen(q); i < colwidth; i++)
        strcat(s, " ");
      q = p + 1;
      p = strchr(q, '\377');
    }
    strcat(s, q);
    nfree(colstr);
    colstr = nmalloc(1);
    colstr[0] = 0;
  }
}

/* Substitute %x codes in help files
 *
 * %B = bot nickname
 * %V = version
 * %C = list of channels i monitor
 * %E = eggdrop banner
 * %A = admin line
 * %n = network name
 * %T = current time ("14:15")
 * %N = user's nickname
 * %U = display system name if possible
 * %{+xy}     require flags to read this section
 * %{-}       turn of required flag matching only
 * %{center}  center this line
 * %{cols=N}  start of columnated section (indented)
 * %{help=TOPIC} start a section for a particular command
 * %{end}     end of section
 */
#define HELP_BUF_LEN 256
#define HELP_BOLD  1
#define HELP_REV   2
#define HELP_UNDER 4
#define HELP_FLASH 8

void help_subst(char *s, char *nick, struct flag_record *flags,
                int isdcc, char *topic)
{
  char xx[HELP_BUF_LEN + 1], sub[161], *current, *q, chr, *writeidx,
       *readidx, *towrite;
  struct chanset_t *chan;
  int i, j, center = 0;
  static int help_flags;

#ifdef HAVE_UNAME
  struct utsname uname_info;
#endif

  if (s == NULL) {
    /* Used to reset substitutions */
    blind = 0;
    cols = 0;
    subwidth = 70;
    if (colstr != NULL) {
      nfree(colstr);
      colstr = NULL;
    }
    help_flags = isdcc;
    return;
  }
  strncpyz(xx, s, sizeof xx);
  readidx = xx;
  writeidx = s;
  current = strchr(readidx, '%');
  while (current) {
    /* Are we about to copy a chuck to the end of the buffer?
     * if so return
     */
    if ((writeidx + (current - readidx)) >= (s + HELP_BUF_LEN)) {
      strncpy(writeidx, readidx, (s + HELP_BUF_LEN) - writeidx);
      s[HELP_BUF_LEN] = 0;
      return;
    }
    chr = *(current + 1);
    *current = 0;
    if (!blind)
      writeidx += my_strcpy(writeidx, readidx);
    towrite = NULL;
    switch (chr) {
    case 'b':
      if (glob_hilite(*flags)) {
        if (help_flags & HELP_IRC) {
          towrite = "\002";
        } else if (help_flags & HELP_BOLD) {
          help_flags &= ~HELP_BOLD;
          towrite = "\033[0m";
        } else {
          help_flags |= HELP_BOLD;
          towrite = "\033[1m";
        }
      }
      break;
    case 'v':
      if (glob_hilite(*flags)) {
        if (help_flags & HELP_IRC) {
          towrite = "\026";
        } else if (help_flags & HELP_REV) {
          help_flags &= ~HELP_REV;
          towrite = "\033[0m";
        } else {
          help_flags |= HELP_REV;
          towrite = "\033[7m";
        }
      }
      break;
    case '_':
      if (glob_hilite(*flags)) {
        if (help_flags & HELP_IRC) {
          towrite = "\037";
        } else if (help_flags & HELP_UNDER) {
          help_flags &= ~HELP_UNDER;
          towrite = "\033[0m";
        } else {
          help_flags |= HELP_UNDER;
          towrite = "\033[4m";
        }
      }
      break;
    case 'f':
      if (glob_hilite(*flags)) {
        if (help_flags & HELP_FLASH) {
          if (help_flags & HELP_IRC)
            towrite = "\002\037";
          else
            towrite = "\033[0m";
          help_flags &= ~HELP_FLASH;
        } else {
          help_flags |= HELP_FLASH;
          if (help_flags & HELP_IRC)
            towrite = "\037\002";
          else
            towrite = "\033[5m";
        }
      }
      break;
    case 'U':
#ifdef HAVE_UNAME
      if (uname(&uname_info) >= 0) {
        egg_snprintf(sub, sizeof sub, "%s %s", uname_info.sysname,
                     uname_info.release);
        towrite = sub;
      } else
#endif
        towrite = "*UNKNOWN*";
      break;
    case 'B':
      towrite = (isdcc ? botnetnick : botname);
      break;
    case 'V':
      towrite = ver;
      break;
    case 'E':
      towrite = version;
      break;
    case 'A':
      towrite = admin;
      break;
    case 'n':
      towrite = network;
      break;
    case 'T':
      egg_strftime(sub, 6, "%H:%M", localtime(&now));
      towrite = sub;
      break;
    case 'N':
      towrite = strchr(nick, ':');
      if (towrite)
        towrite++;
      else
        towrite = nick;
      break;
    case 'C':
      if (!blind)
        for (chan = chanset; chan; chan = chan->next) {
          if ((strlen(chan->dname) + writeidx + 2) >= (s + HELP_BUF_LEN)) {
            strncpy(writeidx, chan->dname, (s + HELP_BUF_LEN) - writeidx);
            s[HELP_BUF_LEN] = 0;
            return;
          }
          writeidx += my_strcpy(writeidx, chan->dname);
          if (chan->next) {
            *writeidx++ = ',';
            *writeidx++ = ' ';
          }
        }
      break;
    case '{':
      q = current;
      current++;
      while ((*current != '}') && (*current))
        current++;
      if (*current) {
        *current = 0;
        current--;
        q += 2;
        /* Now q is the string and p is where the rest of the fcn expects */
        if (!strncmp(q, "help=", 5)) {
          if (topic && egg_strcasecmp(q + 5, topic))
            blind |= 2;
          else
            blind &= ~2;
        } else if (!(blind & 2)) {
          if (q[0] == '+') {
            struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

            break_down_flags(q + 1, &fr, NULL);

            /* We used to check flagrec_ok(), but we can use flagrec_eq()
             * instead because lower flags are automatically added now.
             */
            if (!flagrec_eq(&fr, flags))
              blind |= 1;
            else
              blind &= ~1;
          } else if (q[0] == '-')
            blind &= ~1;
          else if (!egg_strcasecmp(q, "end")) {
            blind &= ~1;
            subwidth = 70;
            if (cols) {
              sub[0] = 0;
              subst_addcol(sub, "\377");
              nfree(colstr);
              colstr = NULL;
              cols = 0;
              towrite = sub;
            }
          } else if (!egg_strcasecmp(q, "center"))
            center = 1;
          else if (!strncmp(q, "cols=", 5)) {
            char *r;

            cols = atoi(q + 5);
            colsofar = 0;
            colstr = nmalloc(1);
            colstr[0] = 0;
            r = strchr(q + 5, '/');
            if (r != NULL)
              subwidth = atoi(r + 1);
          }
        }
      } else
        current = q;            /* no } so ignore */
      break;
    default:
      if (!blind) {
        *writeidx++ = chr;
        if (writeidx >= (s + HELP_BUF_LEN)) {
          *writeidx = 0;
          return;
        }
      }
    }
    if (towrite && !blind) {
      if ((writeidx + strlen(towrite)) >= (s + HELP_BUF_LEN)) {
        strncpy(writeidx, towrite, (s + HELP_BUF_LEN) - writeidx);
        s[HELP_BUF_LEN] = 0;
        return;
      }
      writeidx += my_strcpy(writeidx, towrite);
    }
    if (chr) {
      readidx = current + 2;
      current = strchr(readidx, '%');
    } else {
      readidx = current + 1;
      current = NULL;
    }
  }
  if (!blind) {
    i = strlen(readidx);
    if (i && ((writeidx + i) >= (s + HELP_BUF_LEN))) {
      strncpy(writeidx, readidx, (s + HELP_BUF_LEN) - writeidx);
      s[HELP_BUF_LEN] = 0;
      return;
    }
    strcpy(writeidx, readidx);
  } else
    *writeidx = 0;
  if (center) {
    strcpy(xx, s);
    i = 35 - (strlen(xx) / 2);
    if (i > 0) {
      s[0] = 0;
      for (j = 0; j < i; j++)
        s[j] = ' ';
      strcpy(s + i, xx);
    }
  }
  if (cols) {
    strcpy(xx, s);
    s[0] = 0;
    subst_addcol(s, xx);
  }
}

static void scan_help_file(struct help_ref *current, char *filename, int type)
{
  FILE *f;
  char s[HELP_BUF_LEN + 1], *p, *q;
  struct help_list_t *list;

  if (is_file(filename) && (f = fopen(filename, "r"))) {
    while (!feof(f)) {
      fgets(s, HELP_BUF_LEN, f);
      if (!feof(f)) {
        p = s;
        while ((q = strstr(p, "%{help="))) {
          q += 7;
          if ((p = strchr(q, '}'))) {
            *p = 0;
            list = nmalloc(sizeof *list);

            list->name = nmalloc(p - q + 1);
            strcpy(list->name, q);
            list->next = current->first;
            list->type = type;
            current->first = list;
            p++;
          } else
            p = "";
        }
      }
    }
    fclose(f);
  }
}

void add_help_reference(char *file)
{
  char s[1024];
  struct help_ref *current;

  for (current = help_list; current; current = current->next)
    if (!strcmp(current->name, file))
      return;                   /* Already exists, can't re-add :P */
  current = nmalloc(sizeof *current);

  current->name = nmalloc(strlen(file) + 1);
  strcpy(current->name, file);
  current->next = help_list;
  current->first = NULL;
  help_list = current;
  egg_snprintf(s, sizeof s, "%smsg/%s", helpdir, file);
  scan_help_file(current, s, 0);
  egg_snprintf(s, sizeof s, "%s%s", helpdir, file);
  scan_help_file(current, s, 1);
  egg_snprintf(s, sizeof s, "%sset/%s", helpdir, file);
  scan_help_file(current, s, 2);
}

void rem_help_reference(char *file)
{
  struct help_ref *current, *last = NULL;
  struct help_list_t *item;

  for (current = help_list; current; last = current, current = current->next)
    if (!strcmp(current->name, file)) {
      while ((item = current->first)) {
        current->first = item->next;
        nfree(item->name);
        nfree(item);
      }
      nfree(current->name);
      if (last)
        last->next = current->next;
      else
        help_list = current->next;
      nfree(current);
      return;
    }
}

void reload_help_data(void)
{
  struct help_ref *current = help_list, *next;
  struct help_list_t *item;

  help_list = NULL;
  while (current) {
    while ((item = current->first)) {
      current->first = item->next;
      nfree(item->name);
      nfree(item);
    }
    add_help_reference(current->name);
    nfree(current->name);
    next = current->next;
    nfree(current);
    current = next;
  }
}

void debug_help(int idx)
{
  struct help_ref *current;
  struct help_list_t *item;

  for (current = help_list; current; current = current->next) {
    dprintf(idx, "HELP FILE(S): %s\n", current->name);
    for (item = current->first; item; item = item->next) {
      dprintf(idx, "   %s (%s)\n", item->name, (item->type == 0) ? "msg/" :
              (item->type == 1) ? "" : "set/");
    }
  }
}

FILE *resolve_help(int dcc, char *file)
{

  char s[1024];
  FILE *f;
  struct help_ref *current;
  struct help_list_t *item;

  /* Somewhere here goes the eventual substituation */
  if (!(dcc & HELP_TEXT)) {
    for (current = help_list; current; current = current->next)
      for (item = current->first; item; item = item->next)
        if (!strcmp(item->name, file)) {
          if (!item->type && !dcc) {
            egg_snprintf(s, sizeof s, "%smsg/%s", helpdir, current->name);
            if ((f = fopen(s, "r")))
              return f;
          } else if (dcc && item->type) {
            if (item->type == 1)
              egg_snprintf(s, sizeof s, "%s%s", helpdir, current->name);
            else
              egg_snprintf(s, sizeof s, "%sset/%s", helpdir, current->name);
            if ((f = fopen(s, "r")))
              return f;
          }
        }
    /* No match was found, so we better return NULL */
    return NULL;
  }
  /* Since we're not dealing with help files, we should just prepend the filename with textdir */
  simple_sprintf(s, "%s%s", textdir, file);
  if (is_file(s))
    return fopen(s, "r");
  else
    return NULL;
}

void showhelp(char *who, char *file, struct flag_record *flags, int fl)
{
  int lines = 0;
  char s[HELP_BUF_LEN + 1];
  FILE *f = resolve_help(fl, file);

  if (f) {
    help_subst(NULL, NULL, 0, HELP_IRC, NULL);  /* Clear flags */
    while (!feof(f)) {
      fgets(s, HELP_BUF_LEN, f);
      if (!feof(f)) {
        if (s[strlen(s) - 1] == '\n')
          s[strlen(s) - 1] = 0;
        if (!s[0])
          strcpy(s, " ");
        help_subst(s, who, flags, 0, file);
        if ((s[0]) && (strlen(s) > 1)) {
          dprintf(DP_HELP, "NOTICE %s :%s\n", who, s);
          lines++;
        }
      }
    }
    fclose(f);
  }
  if (!lines && !(fl & HELP_TEXT))
    dprintf(DP_HELP, "NOTICE %s :%s\n", who, IRC_NOHELP2);
}

static int display_tellhelp(int idx, char *file, FILE *f,
                            struct flag_record *flags)
{
  char s[HELP_BUF_LEN + 1];
  int lines = 0;

  if (f) {
    help_subst(NULL, NULL, 0,
               (dcc[idx].status & STAT_TELNET) ? 0 : HELP_IRC, NULL);
    while (!feof(f)) {
      fgets(s, HELP_BUF_LEN, f);
      if (!feof(f)) {
        if (s[strlen(s) - 1] == '\n')
          s[strlen(s) - 1] = 0;
        if (!s[0])
          strcpy(s, " ");
        help_subst(s, dcc[idx].nick, flags, 1, file);
        if (s[0]) {
          dprintf(idx, "%s\n", s);
          lines++;
        }
      }
    }
    fclose(f);
  }
  return lines;
}

void tellhelp(int idx, char *file, struct flag_record *flags, int fl)
{
  int lines = 0;
  FILE *f = resolve_help(HELP_DCC | fl, file);

  if (f)
    lines = display_tellhelp(idx, file, f, flags);
  if (!lines && !(fl & HELP_TEXT))
    dprintf(idx, "%s\n", IRC_NOHELP2);
}

/* Same as tellallhelp, just using wild_match instead of strcmp
 */
void tellwildhelp(int idx, char *match, struct flag_record *flags)
{
  struct help_ref *current;
  struct help_list_t *item;
  FILE *f;
  char s[1024];

  s[0] = '\0';
  for (current = help_list; current; current = current->next)
    for (item = current->first; item; item = item->next)
      if (wild_match(match, item->name) && item->type) {
        if (item->type == 1)
          egg_snprintf(s, sizeof s, "%s%s", helpdir, current->name);
        else
          egg_snprintf(s, sizeof s, "%sset/%s", helpdir, current->name);
        if ((f = fopen(s, "r")))
          display_tellhelp(idx, item->name, f, flags);
      }
  if (!s[0])
    dprintf(idx, "%s\n", IRC_NOHELP2);
}

/* Same as tellwildhelp, just using strcmp instead of wild_match
 */
void tellallhelp(int idx, char *match, struct flag_record *flags)
{
  struct help_ref *current;
  struct help_list_t *item;
  FILE *f;
  char s[1024];

  s[0] = '\0';
  for (current = help_list; current; current = current->next)
    for (item = current->first; item; item = item->next)
      if (!strcmp(match, item->name) && item->type) {

        if (item->type == 1)
          egg_snprintf(s, sizeof s, "%s%s", helpdir, current->name);
        else
          egg_snprintf(s, sizeof s, "%sset/%s", helpdir, current->name);
        if ((f = fopen(s, "r")))
          display_tellhelp(idx, item->name, f, flags);
      }
  if (!s[0])
    dprintf(idx, "%s\n", IRC_NOHELP2);
}

/* Substitute vars in a lang text to dcc chatter
 */
void sub_lang(int idx, char *text)
{
  char s[1024];
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);
  help_subst(NULL, NULL, 0,
             (dcc[idx].status & STAT_TELNET) ? 0 : HELP_IRC, NULL);
  strncpyz(s, text, sizeof s);
  if (s[strlen(s) - 1] == '\n')
    s[strlen(s) - 1] = 0;
  if (!s[0])
    strcpy(s, " ");
  help_subst(s, dcc[idx].nick, &fr, 1, botnetnick);
  if (s[0])
    dprintf(idx, "%s\n", s);
}

/* This will return a pointer to the first character after the @ in the
 * string given it.  Possibly it's time to think about a regexp library
 * for eggdrop...
 */
char *extracthostname(char *hostmask)
{
  char *p = strrchr(hostmask, '@');

  return p ? p + 1 : "";
}

/* Show motd to dcc chatter
 */
void show_motd(int idx)
{
  FILE *vv;
  char s[1024];
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  if (!is_file(motdfile))
    return;

  vv = fopen(motdfile, "r");
  if (!vv)
    return;

  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);
  dprintf(idx, "\n");
  /* reset the help_subst variables to their defaults */
  help_subst(NULL, NULL, 0,
             (dcc[idx].status & STAT_TELNET) ? 0 : HELP_IRC, NULL);
  while (!feof(vv)) {
    fgets(s, 120, vv);
    if (!feof(vv)) {
      if (s[strlen(s) - 1] == '\n')
        s[strlen(s) - 1] = 0;
      if (!s[0])
        strcpy(s, " ");
      help_subst(s, dcc[idx].nick, &fr, 1, botnetnick);
      if (s[0])
        dprintf(idx, "%s\n", s);
    }
  }
  fclose(vv);
  dprintf(idx, "\n");
}

/* Show banner to telnet user
 */
void show_banner(int idx)
{
  FILE *vv;
  char s[1024];
  struct flag_record fr = { FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0 };

  if (!is_file(bannerfile))
    return;

  vv = fopen(bannerfile, "r");
  if (!vv)
    return;

  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.chat->con_chan);
  /* reset the help_subst variables to their defaults */
  help_subst(NULL, NULL, 0, 0, NULL);
  while (!feof(vv)) {
    fgets(s, 120, vv);
    if (!feof(vv)) {
      if (!s[0])
        strcpy(s, " \n");
      help_subst(s, dcc[idx].nick, &fr, 0, botnetnick);
      dprintf(idx, "%s", s);
    }
  }
  fclose(vv);
}

/* Create a string with random letters and digits
 */
void make_rand_str(char *s, int len)
{
  int j;

  for (j = 0; j < len; j++) {
    if (!randint(3))
      s[j] = '0' + randint(10);
    else
      s[j] = 'a' + randint(26);
  }
  s[len] = 0;
}

/* Convert an octal string into a decimal integer value.  If the string
 * is empty or contains non-octal characters, -1 is returned.
 */
int oatoi(const char *octal)
{
  register int i;

  if (!*octal)
    return -1;
  for (i = 0; ((*octal >= '0') && (*octal <= '7')); octal++)
    i = (i * 8) + (*octal - '0');
  if (*octal)
    return -1;
  return i;
}

/* Return an allocated buffer which contains a copy of the string
 * 'str', with all 'div' characters escaped by 'mask'. 'mask'
 * characters are escaped too.
 *
 * Remember to free the returned memory block.
 */
char *str_escape(const char *str, const char div, const char mask)
{
  const int len = strlen(str);
  int buflen = (2 * len), blen = 0;
  char *buf = nmalloc(buflen + 1), *b = buf;
  const char *s;

  if (!buf)
    return NULL;
  for (s = str; *s; s++) {
    /* Resize buffer. */
    if ((buflen - blen) <= 3) {
      buflen = (buflen * 2);
      buf = nrealloc(buf, buflen + 1);
      if (!buf)
        return NULL;
      b = buf + blen;
    }

    if (*s == div || *s == mask) {
      sprintf(b, "%c%02x", mask, *s);
      b += 3;
      blen += 3;
    } else {
      *(b++) = *s;
      blen++;
    }
  }
  *b = 0;
  return buf;
}

/* Search for a certain character 'div' in the string 'str', while
 * ignoring escaped characters prefixed with 'mask'.
 *
 * The string
 *
 *   "\\3a\\5c i am funny \\3a):further text\\5c):oink"
 *
 * as str, '\\' as mask and ':' as div would change the str buffer
 * to
 *
 *   ":\\ i am funny :)"
 *
 * and return a pointer to "further text\\5c):oink".
 *
 * NOTE: If you look carefully, you'll notice that strchr_unescape()
 *       behaves differently than strchr().
 */
char *strchr_unescape(char *str, const char div, register const char esc_char)
{
  char buf[3];
  register char *s, *p;

  buf[2] = 0;
  for (s = p = str; *s; s++, p++) {
    if (*s == esc_char) {       /* Found escape character.              */
      /* Convert code to character. */
      buf[0] = s[1], buf[1] = s[2];
      *p = (unsigned char) strtol(buf, NULL, 16);
      s += 2;
    } else if (*s == div) {
      *p = *s = 0;
      return (s + 1);           /* Found searched for character.        */
    } else
      *p = *s;
  }
  *p = 0;
  return NULL;
}

/* Is every character in a string a digit? */
int str_isdigit(const char *str)
{
  if (!*str)
    return 0;

  for (; *str; ++str) {
    if (!egg_isdigit(*str))
      return 0;
  }
  return 1;
}

/* As strchr_unescape(), but converts the complete string, without
 * searching for a specific delimiter character.
 */
void str_unescape(char *str, register const char esc_char)
{
  (void) strchr_unescape(str, 0, esc_char);
}

char *str_dup(const char *s)
{
  char *d = nmalloc(strlen(s)+1);
  strcpy(d, s);
  return d;
}

/* Kills the bot. s1 is the reason shown to other bots,
 * s2 the reason shown on the partyline. (Sup 25Jul2001)
 */
void kill_bot(char *s1, char *s2)
{
  call_hook(HOOK_DIE);
  chatout("*** %s\n", s1);
  botnet_send_chat(-1, botnetnick, s1);
  botnet_send_bye();
  write_userfile(-1);
  fatal(s2, 0);
}
