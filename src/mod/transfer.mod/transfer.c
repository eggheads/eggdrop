/*
 * This file is part of the eggdrop source code copyright (c) 1997 Robey
 * Pointer and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called COPYING
 * that was distributed with this code.
 */

#define MAKING_TRANSFER
#define MOD_FILESYS
#define MODULE_NAME "transfer"

/* sigh sunos */
#include <sys/types.h>
#include <sys/stat.h>
#include "../module.h"
#include "../../tandem.h"

#include "../../users.h"
#include "transfer.h"
#include <netinet/in.h>
#include <arpa/inet.h>

static Function *global = NULL;

static int copy_to_tmp = 1;	/* copy files to /tmp before transmitting? */
static int wait_dcc_xfer = 300;	/* timeout time on DCC xfers */
static p_tcl_bind_list H_rcvd, H_sent;
static int dcc_limit = 3;	/* maximum number of simultaneous file
				 * downloads allowed */
static int dcc_block = 1024;
static void stats_add_dnload(struct userrec *, unsigned long);
static void stats_add_upload(struct userrec *, unsigned long);
static void wipe_tmp_filename(char *fn, int idx);
static int at_limit(char *nick);
static void dcc_get_pending(int idx, char *buf, int len);
static int quiet_reject;        /* Quietly reject dcc chat or sends from
                                 * users without access? */

typedef struct zarrf {
  char *dir;			/* starts with '*' -> absolute dir */
  char *file;			/* (otherwise -> dccdir) */
  char nick[NICKLEN];		/* who queued this file */
  char to[NICKLEN];		/* who will it be sent to */
  struct zarrf *next;
} fileq_t;


static fileq_t *fileq = NULL;

#undef MATCH
#define MATCH (match+sofar)

/* this function SHAMELESSLY :) pinched from match.c in the original
 * source, see that file for info about the author etc */

#define QUOTE '\\'
#define WILDS '*'
#define WILDQ '?'
#define NOMATCH 0
/*========================================================================*
 * EGGDROP:   wild_match_file(char *ma, char *na)                         *
 * IrcII:     NOT USED                                                    *
 *                                                                        *
 * Features:  Forward, case-sensitive, ?, *                               *
 * Best use:  File mask matching, as it is case-sensitive                 *
 *========================================================================*/
static int wild_match_file(register char *m, register char *n)
{
  char *ma = m, *lsm = 0, *lsn = 0;
  int match = 1;
  register unsigned int sofar = 0;

  /* take care of null strings (should never match) */
  if ((m == 0) || (n == 0) || (!*n))
    return NOMATCH;
  /* (!*m) test used to be here, too, but I got rid of it.  After all, If
   * (!*n) was false, there must be a character in the name (the second
   * string), so if the mask is empty it is a non-match.  Since the
   * algorithm handles this correctly without testing for it here and this
   * shouldn't be called with null masks anyway, it should be a bit faster
   * this way */
  while (*n) {
    /* Used to test for (!*m) here, but this scheme seems to work better */
    switch (*m) {
    case 0:
      do
	m--;			/* Search backwards      */
      while ((m > ma) && (*m == '?'));	/* For first non-? char  */
      if ((m > ma) ? ((*m == '*') && (m[-1] != QUOTE)) : (*m == '*'))
	return MATCH;		/* nonquoted * = match   */
      break;
    case WILDS:
      do
	m++;
      while (*m == WILDS);	/* Zap redundant wilds   */
      lsm = m;
      lsn = n;			/* Save * fallback spot  */
      match += sofar;
      sofar = 0;
      continue;			/* Save tally count      */
    case WILDQ:
      m++;
      n++;
      continue;			/* Match one char        */
    case QUOTE:
      m++;			/* Handle quoting        */
    }
    if (*m == *n) {		/* If matching           */
      m++;
      n++;
      sofar++;
      continue;			/* Tally the match       */
    }
    if (lsm) {			/* Try to fallback on *  */
      n = ++lsn;
      m = lsm;			/* Restore position      */
      /* Used to test for (!*n) here but it wasn't necessary so it's gone */
      sofar = 0;
      continue;			/* Next char, please     */
    }
    return NOMATCH;		/* No fallbacks=No match */
  }
  while (*m == WILDS)
    m++;			/* Zap leftover *s       */
  return (*m) ? NOMATCH : MATCH;	/* End of both = match   */
}

static int builtin_sentrcvd STDVAR {
  Function F = (Function) cd;

  BADARGS(4, 4, " hand nick path");
  if (!check_validity(argv[0], builtin_sentrcvd)) {
    Tcl_AppendResult(irp, "bad builtin command call!", NULL);
    return TCL_ERROR;
  }
  F(argv[1], argv[2], argv[3]);
  return TCL_OK;
}

static int expmem_fileq()
{
  fileq_t *q = fileq;
  int tot = 0;

  context;
  while (q != NULL) {
    tot += strlen(q->dir) + strlen(q->file) + 2 + sizeof(fileq_t);
    q = q->next;
  }
  return tot;
}

static void queue_file(char *dir, char *file, char *from, char *to)
{
  fileq_t *q = fileq;

  fileq = (fileq_t *) nmalloc(sizeof(fileq_t));
  fileq->next = q;
  fileq->dir = (char *) nmalloc(strlen(dir) + 1);
  fileq->file = (char *) nmalloc(strlen(file) + 1);
  strcpy(fileq->dir, dir);
  strcpy(fileq->file, file);
  strcpy(fileq->nick, from);
  strcpy(fileq->to, to);
}

static void deq_this(fileq_t * this)
{
  fileq_t *q = fileq, *last = NULL;

  while (q && (q != this)) {
    last = q;
    q = q->next;
  }
  if (!q)
    return;			/* bogus ptr */
  if (last)
    last->next = q->next;
  else
    fileq = q->next;
  nfree(q->dir);
  nfree(q->file);
  nfree(q);
}

/* remove all files queued to a certain user */
static void flush_fileq(char *to)
{
  fileq_t *q = fileq;
  int fnd = 1;

  while (fnd) {
    q = fileq;
    fnd = 0;
    while (q != NULL) {
      if (!strcasecmp(q->to, to)) {
	deq_this(q);
	q = NULL;
	fnd = 1;
      }
      if (q != NULL)
	q = q->next;
    }
  }
}

static void send_next_file(char *to)
{
  fileq_t *q = fileq, *this = NULL;
  char s[256], s1[256];
  int x;

  while (q != NULL) {
    if (!strcasecmp(q->to, to))
      this = q;
    q = q->next;
  }
  if (this == NULL)
    return;			/* none */
  /* copy this file to /tmp */
  if (this->dir[0] == '*')	/* absolute path */
    sprintf(s, "%s/%s", &this->dir[1], this->file);
  else {
    char *p = strchr(this->dir, '*');

    if (p == NULL) {		/* if it's messed up */
      send_next_file(to);
      return;
    }
    p++;
    sprintf(s, "%s%s%s", p, p[0] ? "/" : "", this->file);
    strcpy(this->dir, &(p[atoi(this->dir)]));
  }
  if (copy_to_tmp) {
    sprintf(s1, "%s%s", tempdir, this->file);
    if (copyfile(s, s1) != 0) {
      putlog(LOG_FILES | LOG_MISC, "*",
	     "Refused dcc get %s: copy to %s FAILED!",
	     this->file, tempdir);
      dprintf(DP_HELP,
	      "NOTICE %s :File system is broken; aborting queued files.\n",
	      this->to);
      strcpy(s, this->to);
      flush_fileq(s);
      return;
    }
  } else
    strcpy(s1, s);
  if (this->dir[0] == '*')
    sprintf(s, "%s/%s", &this->dir[1], this->file);
  else
    sprintf(s, "%s%s%s", this->dir, this->dir[0] ? "/" : "", this->file);
  x = raw_dcc_send(s1, this->to, this->nick, s);
  if (x == 1) {
    wipe_tmp_filename(s1, -1);
    putlog(LOG_FILES, "*", "DCC connections full: GET %s [%s]", s1, this->nick);
    dprintf(DP_HELP,
	    "NOTICE %s :DCC connections full; aborting queued files.\n",
	    this->to);
    strcpy(s, this->to);
    flush_fileq(s);
    return;
  }
  if (x == 2) {
    wipe_tmp_filename(s1, -1);
    putlog(LOG_FILES, "*", "DCC socket error: GET %s [%s]", s1, this->nick);
    dprintf(DP_HELP, "NOTICE %s :DCC socket error; aborting queued files.\n",
	    this->to);
    strcpy(s, this->to);
    flush_fileq(s);
    return;
  }
  if (strcasecmp(this->to, this->nick))
    dprintf(DP_HELP, "NOTICE %s :Here is a file from %s ...\n", this->to,
	    this->nick);
  deq_this(this);
}

static void check_tcl_sentrcvd(struct userrec *u, char *nick, char *path,
			       p_tcl_bind_list h)
{
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};
  char *hand = u ? u->handle : "*";

  context;
  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_sr1", hand, 0);
  Tcl_SetVar(interp, "_sr2", nick, 0);
  Tcl_SetVar(interp, "_sr3", path, 0);
  check_tcl_bind(h, hand, &fr, " $_sr1 $_sr2 $_sr3",
		 MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE);
  context;
}

static void eof_dcc_fork_send(int idx)
{
  char s1[121];
  char *s2;

  context;
  fclose(dcc[idx].u.xfer->f);
  if (!strcmp(dcc[idx].nick, "*users")) {
    int x, y = 0;

    for (x = 0; x < dcc_total; x++)
      if ((!strcasecmp(dcc[x].nick, dcc[idx].host)) &&
	  (dcc[x].type->flags & DCT_BOT))
	y = x;
    if (y != 0) {
      dcc[y].status &= ~STAT_GETTING;
      dcc[y].status &= ~STAT_SHARE;
    }
    putlog(LOG_BOTS, "*", USERF_FAILEDXFER);
    unlink(dcc[idx].u.xfer->filename);
  } else {
    neterror(s1);
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s (%s)\n", dcc[idx].nick,
	      DCC_CONNECTFAILED1, s1);
    putlog(LOG_MISC, "*", "%s: SEND %s (%s!%s)", DCC_CONNECTFAILED2,
	   dcc[idx].u.xfer->filename, dcc[idx].nick, dcc[idx].host);
    putlog(LOG_MISC, "*", "    (%s)", s1);
    s2 = nmalloc(strlen(tempdir) + strlen(dcc[idx].u.xfer->filename) + 1);
    sprintf(s2, "%s%s", tempdir, dcc[idx].u.xfer->filename);
    unlink(s2);
    nfree(s2);
  }
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void eof_dcc_send(int idx)
{
  int ok, j;
  char ofn[121], nfn[121], s[1024], *hand;
  struct userrec *u;

  context;
  if (dcc[idx].u.xfer->length == dcc[idx].status) {
    /* success */
    ok = 0;
    fclose(dcc[idx].u.xfer->f);
    if (!strcmp(dcc[idx].nick, "*users")) {
      module_entry *me = module_find("share", 0, 0);

      if (me && me->funcs) {
	Function f = me->funcs[SHARE_FINISH];

	(f) (idx);
      }
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return;
    }
    putlog(LOG_FILES, "*", "Completed dcc send %s from %s!%s",
	   dcc[idx].u.xfer->filename, dcc[idx].nick, dcc[idx].host);
    simple_sprintf(s, "%s!%s", dcc[idx].nick, dcc[idx].host);
    u = get_user_by_host(s);
    hand = u ? u->handle : "*";
    /* move the file from /tmp */
    /* slwstub - filenames to long segfault and kill the eggdrop
     * NOTE: This is NOT A PTF. Its a circumvention, a workaround
     * I'm not moving the file from the receiving area. You may
     * want to inspect it, shorten the name, give credit or NOT! */
    if (strlen(dcc[idx].u.xfer->filename) > MAX_FILENAME_LENGTH) {
      /* the filename is to long... blow it off */
      putlog(LOG_FILES, "*", "Filename %d length. Way To LONG.",
	     strlen(dcc[idx].u.xfer->filename));
      dprintf(DP_HELP, "NOTICE %s :Filename %d length Way To LONG!\n",
	      dcc[idx].nick, strlen(dcc[idx].u.xfer->filename));
      putlog(LOG_FILES, "*", "To Bad So Sad Your Dad!");
      dprintf(DP_HELP, "NOTICE %s :To Bad So Sad Your Dad!\n",
	      dcc[idx].nick);
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return;
    }
    /* slwstub - filenames to long segfault and kill the eggdrop */
    simple_sprintf(ofn, "%s%s", tempdir, dcc[idx].u.xfer->filename);
    simple_sprintf(nfn, "%s%s", dcc[idx].u.xfer->dir,
		   dcc[idx].u.xfer->filename);
    if (movefile(ofn, nfn))
      putlog(LOG_MISC | LOG_FILES, "*",
	     "FAILED move %s from %s ! File lost!",
	     dcc[idx].u.xfer->filename, tempdir);
    else {
      /* add to file database */
      module_entry *fs = module_find("filesys", 0, 0);

      if (fs != NULL) {
	Function f = fs->funcs[FILESYS_ADDFILE];

	f(dcc[idx].u.xfer->dir, dcc[idx].u.xfer->filename, hand);
      }
      stats_add_upload(u, dcc[idx].u.xfer->length);
      check_tcl_sentrcvd(u, dcc[idx].nick, nfn, H_rcvd);
    }
    for (j = 0; j < dcc_total; j++)
      if (!ok && (dcc[j].type->flags & (DCT_GETNOTES | DCT_FILES)) &&
	  !strcasecmp(dcc[j].nick, hand)) {
	ok = 1;
	dprintf(j, "Thanks for the file!\n");
      }
    if (!ok)
      dprintf(DP_HELP, "NOTICE %s :Thanks for the file!\n",
	      dcc[idx].nick);
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return;
  }
  /* failure :( */
  context;
  fclose(dcc[idx].u.xfer->f);
  if (!strcmp(dcc[idx].nick, "*users")) {
    int x, y = 0;

    for (x = 0; x < dcc_total; x++)
      if ((!strcasecmp(dcc[x].nick, dcc[idx].host)) &&
	  (dcc[x].type->flags & DCT_BOT))
	y = x;
    if (y) {
      putlog(LOG_BOTS, "*", "Lost userfile transfer to %s; aborting.",
	     dcc[y].nick);
      unlink(dcc[idx].u.xfer->filename);
      /* drop that bot */
      dprintf(y, "bye\n");
      simple_sprintf(s, "Disconnected %s (aborted userfile transfer)",
		     dcc[y].nick);
      botnet_send_unlinked(y, dcc[y].nick, s);
      chatout("*** %s\n", dcc[y].nick, s);
      if (y < idx) {
	int t = y;

	y = idx;
	idx = t;
      }
      killsock(dcc[y].sock);
      lostdcc(y);
    }
  } else {
    putlog(LOG_FILES, "*", "Lost dcc send %s from %s!%s (%lu/%lu)",
	   dcc[idx].u.xfer->filename, dcc[idx].nick, dcc[idx].host,
	   dcc[idx].status, dcc[idx].u.xfer->length);
    sprintf(s, "%s%s", tempdir, dcc[idx].u.xfer->filename);
    unlink(s);
    killsock(dcc[idx].sock);
    lostdcc(idx);
  }
}

static void dcc_get(int idx, char *buf, int len)
{
  char xnick[NICKLEN];
  unsigned char bbuf[4], *bf;
  unsigned long cmp, l;
  int w = len + dcc[idx].u.xfer->sofar, p = 0;

  context;
  dcc[idx].timeval = now;
  if (w < 4) {
    my_memcpy(&(dcc[idx].u.xfer->buf[dcc[idx].u.xfer->sofar]), buf, len);
    dcc[idx].u.xfer->sofar += len;
    return;
  } else if (w == 4) {
    my_memcpy(bbuf, dcc[idx].u.xfer->buf, dcc[idx].u.xfer->sofar);
    my_memcpy(&(bbuf[dcc[idx].u.xfer->sofar]), buf, len);
  } else {
    p = ((w - 1) & ~3) - dcc[idx].u.xfer->sofar;
    w = w - ((w - 1) & ~3);
    if (w < 4) {
      my_memcpy(dcc[idx].u.xfer->buf, &(buf[p]), w);
      return;
    }
    my_memcpy(bbuf, &(buf[p]), w);
  }				/* go back and read it again, it *does*
				 * make sense ;) */
  dcc[idx].u.xfer->sofar = 0;
  /* this is more compatable than ntohl for machines where an int
   * is more than 4 bytes: */
  cmp = ((unsigned int) (bbuf[0]) << 24) + ((unsigned int) (bbuf[1]) << 16) +
    ((unsigned int) (bbuf[2]) << 8) + bbuf[3];
  dcc[idx].u.xfer->acked = cmp;
  if ((cmp > dcc[idx].status) && (cmp <= dcc[idx].u.xfer->length)) {
    /* attempt to resume I guess */
    if (!strcmp(dcc[idx].nick, "*users")) {
      putlog(LOG_BOTS, "*", "!!! Trying to skip ahead on userfile transfer");
    } else {
      fseek(dcc[idx].u.xfer->f, cmp, SEEK_SET);
      dcc[idx].status = cmp;
      putlog(LOG_FILES, "*", "Resuming file transfer at %dk for %s to %s",
	     (int) (cmp / 1024), dcc[idx].u.xfer->filename,
	     dcc[idx].nick);
    }
  }
  if (cmp != dcc[idx].status)
    return;
  if (dcc[idx].status == dcc[idx].u.xfer->length) {
    context;
    /* successful send, we're done */
    killsock(dcc[idx].sock);
    fclose(dcc[idx].u.xfer->f);
    if (!strcmp(dcc[idx].nick, "*users")) {
      module_entry *me = module_find("share", 0, 0);
      int x, y = 0;

      for (x = 0; x < dcc_total; x++)
	if ((!strcasecmp(dcc[x].nick, dcc[idx].host)) &&
	    (dcc[x].type->flags & DCT_BOT))
	  y = x;
      if (y != 0)
	dcc[y].status &= ~STAT_SENDING;
      putlog(LOG_BOTS, "*", "Completed userfile transfer to %s.",
	     dcc[y].nick);
      unlink(dcc[idx].u.xfer->filename);
      /* any sharebot things that were queued: */
      if (me && me->funcs[SHARE_DUMP_RESYNC])
	((me->funcs)[SHARE_DUMP_RESYNC]) (y);
      xnick[0] = 0;
    } else {
      module_entry *fs = module_find("filesys", 0, 0);
      struct userrec *u = get_user_by_handle(userlist,
					     dcc[idx].u.xfer->from);
      check_tcl_sentrcvd(u, dcc[idx].nick,
			 dcc[idx].u.xfer->dir, H_sent);
      if (fs != NULL) {
	Function f = fs->funcs[FILESYS_INCRGOTS];

	f(dcc[idx].u.xfer->dir);
      }
      /* download is credited to the user who requested it
       * (not the user who actually received it) */
      stats_add_dnload(u, dcc[idx].u.xfer->length);
      putlog(LOG_FILES, "*", "Finished dcc send %s to %s",
	     dcc[idx].u.xfer->filename, dcc[idx].nick);
      wipe_tmp_filename(dcc[idx].u.xfer->filename, idx);
      strcpy((char *) xnick, dcc[idx].nick);
    }
    lostdcc(idx);
    /* any to dequeue? */
    if (!at_limit(xnick))
      send_next_file(xnick);
    return;
  }
  context;
  /* note: is this fseek necessary any more? */
/* fseek(dcc[idx].u.xfer->f,dcc[idx].status,0);   */
  l = dcc_block;
  if ((l == 0) || (dcc[idx].status + l > dcc[idx].u.xfer->length))
    l = dcc[idx].u.xfer->length - dcc[idx].status;
  bf = (unsigned char *) nmalloc(l + 1);
  fread(bf, l, 1, dcc[idx].u.xfer->f);
  tputs(dcc[idx].sock, bf, l);
  nfree(bf);
  dcc[idx].status += l;
}

static void eof_dcc_get(int idx)
{
  char xnick[NICKLEN], s[1024];

  context;
  fclose(dcc[idx].u.xfer->f);
  if (!strcmp(dcc[idx].nick, "*users")) {
    int x, y = 0;

    for (x = 0; x < dcc_total; x++)
      if ((!strcasecmp(dcc[x].nick, dcc[idx].host)) &&
	  (dcc[x].type->flags & DCT_BOT))
	y = x;
    putlog(LOG_BOTS, "*", "Lost userfile transfer; aborting.");
    /* unlink(dcc[idx].u.xfer->filename); *//* <- already unlinked */
    xnick[0] = 0;
    /* drop that bot */
    dprintf(-dcc[y].sock, "bye\n");
    simple_sprintf(s, "Disconnected %s (aborted userfile transfer)",
		   dcc[y].nick);
    botnet_send_unlinked(y, dcc[y].nick, s);
    chatout("*** %s\n", s);
    if (y < idx) {
      int t = y;

      y = idx;
      idx = t;
    }
    killsock(dcc[y].sock);
    lostdcc(y);
    return;
  } else {
    putlog(LOG_FILES, "*", "Lost dcc get %s from %s!%s",
	   dcc[idx].u.xfer->filename, dcc[idx].nick, dcc[idx].host);
    wipe_tmp_filename(dcc[idx].u.xfer->filename, idx);
    strcpy(xnick, dcc[idx].nick);
  }
  killsock(dcc[idx].sock);
  lostdcc(idx);
  /* send next queued file if there is one */
  if (!at_limit(xnick))
    send_next_file(xnick);
  context;
}

static void dcc_send(int idx, char *buf, int len)
{
  char s[512];
  unsigned long sent;

  context;
  fwrite(buf, len, 1, dcc[idx].u.xfer->f);
  dcc[idx].status += len;
  /* put in network byte order */
  sent = dcc[idx].status;
  s[0] = (sent / (1 << 24));
  s[1] = (sent % (1 << 24)) / (1 << 16);
  s[2] = (sent % (1 << 16)) / (1 << 8);
  s[3] = (sent % (1 << 8));
  tputs(dcc[idx].sock, s, 4);
  dcc[idx].timeval = now;
  if ((dcc[idx].status > dcc[idx].u.xfer->length) &&
      (dcc[idx].u.xfer->length > 0)) {
    dprintf(DP_HELP, "NOTICE %s :Bogus file length.\n", dcc[idx].nick);
    putlog(LOG_FILES, "*",
	   "File too long: dropping dcc send %s from %s!%s",
	   dcc[idx].u.xfer->filename, dcc[idx].nick, dcc[idx].host);
    fclose(dcc[idx].u.xfer->f);
    sprintf(s, "%s%s", tempdir, dcc[idx].u.xfer->filename);
    unlink(s);
    killsock(dcc[idx].sock);
    lostdcc(idx);
  }
}

static int tcl_getfileq STDVAR
{
  char s[512];
  fileq_t *q = fileq;

  BADARGS(2, 2, " handle");
  while (q != NULL) {
    if (!strcasecmp(q->nick, argv[1])) {
      if (q->dir[0] == '*')
	sprintf(s, "%s %s/%s", q->to, &q->dir[1], q->file);
      else
	sprintf(s, "%s /%s%s%s", q->to, q->dir, q->dir[0] ? "/" : "", q->file);
      Tcl_AppendElement(irp, s);
    }
    q = q->next;
  }
  return TCL_OK;
}

static int tcl_dccsend STDVAR
{
  char s[5], sys[512], *nfn;
  int i;
  FILE *f;

  BADARGS(3, 3, " filename ircnick");
  f = fopen(argv[1], "r");
  if (f == NULL) {
    /* file not found */
    Tcl_AppendResult(irp, "3", NULL);
    return TCL_OK;
  }
  fclose(f);
  nfn = strrchr(argv[1], '/');
  if (nfn == NULL)
    nfn = argv[1];
  else
    nfn++;
  if (at_limit(argv[2])) {
    /* queue that mother */
    if (nfn == argv[1])
      queue_file("*", nfn, "(script)", argv[2]);
    else {
      nfn--;
      *nfn = 0;
      nfn++;
      sprintf(sys, "*%s", argv[1]);
      queue_file(sys, nfn, "(script)", argv[2]);
    }
    Tcl_AppendResult(irp, "4", NULL);
    return TCL_OK;
  }
  if (copy_to_tmp) {
    sprintf(sys, "%s%s", tempdir, nfn);		/* new filename, in /tmp */
    copyfile(argv[1], sys);
  } else
    strcpy(sys, argv[1]);
  i = raw_dcc_send(sys, argv[2], "*", argv[1]);
  if (i > 0)
    wipe_tmp_filename(sys, -1);
  sprintf(s, "%d", i);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static tcl_cmds mytcls[] =
{
  {"dccsend", tcl_dccsend},
  {"getfileq", tcl_getfileq},
  {0, 0}
};

static void transfer_get_timeout(int i)
{
  char xx[1024];

  context;
  if (!strcmp(dcc[i].nick, "*users")) {
    int x, y = 0;

    for (x = 0; x < dcc_total; x++)
      if ((!strcasecmp(dcc[x].nick, dcc[i].host)) &&
	  (dcc[x].type->flags & DCT_BOT))
	y = x;
    if (y != 0) {
      dcc[y].status &= ~STAT_SENDING;
      dcc[y].status &= ~STAT_SHARE;
    }
    unlink(dcc[i].u.xfer->filename);
    putlog(LOG_BOTS, "*", "Timeout on userfile transfer.");
    dprintf(y, "bye\n");
    simple_sprintf(xx, "Disconnected %s (timed-out userfile transfer)",
		   dcc[y].nick);
    botnet_send_unlinked(y, dcc[y].nick, xx);
    chatout("*** %s\n", xx);
    if (y < i) {
      int t = y;

      y = i;
      i = t;
    }
    killsock(dcc[y].sock);
    lostdcc(y);
    xx[0] = 0;
  } else {
    char *p;

    strcpy(xx, dcc[i].u.xfer->filename);
    p = strrchr(xx, '/');
    dprintf(DP_HELP, "NOTICE %s :Timeout during transfer, aborting %s.\n",
	    dcc[i].nick, p ? p + 1 : xx);
    putlog(LOG_FILES, "*", "DCC timeout: GET %s (%s) at %lu/%lu", p ? p + 1 : xx,
	   dcc[i].nick, dcc[i].status, dcc[i].u.xfer->length);
    wipe_tmp_filename(dcc[i].u.xfer->filename, i);
    strcpy(xx, dcc[i].nick);
  }
  killsock(dcc[i].sock);
  lostdcc(i);
  if (!at_limit(xx))
    send_next_file(xx);
}

static void tout_dcc_send(int idx)
{
  if (!strcmp(dcc[idx].nick, "*users")) {
    int x, y = 0;

    for (x = 0; x < dcc_total; x++)
      if ((!strcasecmp(dcc[x].nick, dcc[idx].host)) &&
	  (dcc[x].type->flags & DCT_BOT))
	y = x;
    if (y != 0) {
      dcc[y].status &= ~STAT_GETTING;
      dcc[y].status &= ~STAT_SHARE;
    }
    unlink(dcc[idx].u.xfer->filename);
    putlog(LOG_BOTS, "*", "Timeout on userfile transfer.");
  } else {
    char xx[1024];

    dprintf(DP_HELP, "NOTICE %s :Timeout during transfer, aborting %s.\n",
	    dcc[idx].nick, dcc[idx].u.xfer->filename);
    putlog(LOG_FILES, "*", "DCC timeout: SEND %s (%s) at %lu/%lu",
	   dcc[idx].u.xfer->filename, dcc[idx].nick, dcc[idx].status,
	   dcc[idx].u.xfer->length);
    sprintf(xx, "%s%s", tempdir, dcc[idx].u.xfer->filename);
    unlink(xx);
  }
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void display_dcc_get(int idx, char *buf)
{
  if (dcc[idx].status == dcc[idx].u.xfer->length)
    sprintf(buf, "send  (%lu)/%lu\n    Filename: %s\n", dcc[idx].u.xfer->acked,
	    dcc[idx].u.xfer->length, dcc[idx].u.xfer->filename);
  else
    sprintf(buf, "send  %lu/%lu\n    Filename: %s\n", dcc[idx].status,
	    dcc[idx].u.xfer->length, dcc[idx].u.xfer->filename);
}

static void display_dcc_get_p(int idx, char *buf)
{
  sprintf(buf, "send  waited %lus    Filename: %s\n", now - dcc[idx].timeval,
	  dcc[idx].u.xfer->filename);
}

static void display_dcc_send(int idx, char *buf)
{
  sprintf(buf, "send  %lu/%lu\n    Filename: %s\n", dcc[idx].status,
	  dcc[idx].u.xfer->length, dcc[idx].u.xfer->filename);
}

static void display_dcc_fork_send(int idx, char *buf)
{
  sprintf(buf, "conn  send");
}

static int expmem_dcc_xfer(void *x)
{
  return sizeof(struct xfer_info);
}

static void kill_dcc_xfer(int idx, void *x)
{
  nfree(x);
}

static void out_dcc_xfer(int idx, char *buf, void *x)
{
}

static struct dcc_table DCC_SEND =
{
  "SEND",
  DCT_FILETRAN | DCT_FILESEND | DCT_VALIDIDX,
  eof_dcc_send,
  dcc_send,
  &wait_dcc_xfer,
  tout_dcc_send,
  display_dcc_send,
  expmem_dcc_xfer,
  kill_dcc_xfer,
  out_dcc_xfer
};

static void dcc_fork_send(int idx, char *x, int y);

static struct dcc_table DCC_FORK_SEND =
{
  "FORK_SEND",
  DCT_FILETRAN | DCT_FORKTYPE | DCT_FILESEND | DCT_VALIDIDX,
  eof_dcc_fork_send,
  dcc_fork_send,
  &wait_dcc_xfer,
  eof_dcc_fork_send,
  display_dcc_fork_send,
  expmem_dcc_xfer,
  kill_dcc_xfer,
  out_dcc_xfer
};

static void dcc_fork_send(int idx, char *x, int y)
{
  char s1[121];

  if (dcc[idx].type != &DCC_FORK_SEND)
    return;
  dcc[idx].type = &DCC_SEND;
  sprintf(s1, "%s!%s", dcc[idx].nick, dcc[idx].host);
  if (strcmp(dcc[idx].nick, "*users")) {
    putlog(LOG_MISC, "*", "DCC connection: SEND %s (%s)",
	   dcc[idx].type == &DCC_SEND ?
	   dcc[idx].u.xfer->filename : "", s1);
  }
}

static struct dcc_table DCC_GET =
{
  "GET",
  DCT_FILETRAN | DCT_VALIDIDX,
  eof_dcc_get,
  dcc_get,
  &wait_dcc_xfer,
  transfer_get_timeout,
  display_dcc_get,
  expmem_dcc_xfer,
  kill_dcc_xfer,
  out_dcc_xfer
};

static struct dcc_table DCC_GET_PENDING =
{
  "GET_PENDING",
  DCT_FILETRAN | DCT_VALIDIDX,
  eof_dcc_get,
  dcc_get_pending,
  &wait_dcc_xfer,
  transfer_get_timeout,
  display_dcc_get_p,
  expmem_dcc_xfer,
  kill_dcc_xfer,
  out_dcc_xfer
};

static void wipe_tmp_filename(char *fn, int idx)
{
  int i, ok = 1;

  if (!copy_to_tmp)
    return;
  for (i = 0; i < dcc_total; i++)
    if (i != idx)
      if ((dcc[i].type == &DCC_GET) || (dcc[i].type == &DCC_GET_PENDING))
	if (!strcmp(dcc[i].u.xfer->filename, fn))
	  ok = 0;
  if (ok)
    unlink(fn);
}

/* return true if this user has >= the maximum number of file xfers going */
static int at_limit(char *nick)
{
  int i, x = 0;

  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_GET) || (dcc[i].type == &DCC_GET_PENDING))
      if (!strcasecmp(dcc[i].nick, nick))
	x++;
  return (x >= dcc_limit);
}


static void dcc_get_pending(int idx, char *buf, int len)
{
  unsigned long ip;
  unsigned short port;
  int i;
  char *bf, s[UHOSTLEN];

  context;
  i = answer(dcc[idx].sock, s, &ip, &port, 1);
  killsock(dcc[idx].sock);
  dcc[idx].sock = i;
  dcc[idx].addr = ip;
  dcc[idx].port = (int) port;
  if (dcc[idx].sock == -1) {
    neterror(s);
    dprintf(DP_HELP, "NOTICE %s :Bad connection (%s)\n", dcc[idx].nick, s);
    putlog(LOG_FILES, "*", "DCC bad connection: GET %s (%s!%s)",
	   dcc[idx].u.xfer->filename, dcc[idx].nick, dcc[idx].host);
    lostdcc(idx);
    return;
  }
  /* file was already opened */
  if ((dcc_block == 0) || (dcc[idx].u.xfer->length < dcc_block))
    dcc[idx].status = dcc[idx].u.xfer->length;
  else
    dcc[idx].status = dcc_block;
  dcc[idx].type = &DCC_GET;
  bf = (char *) nmalloc(dcc[idx].status + 1);
  fread(bf, dcc[idx].status, 1, dcc[idx].u.xfer->f);
  tputs(dcc[idx].sock, bf, dcc[idx].status);
  nfree(bf);
  dcc[idx].timeval = now;
  /* leave f open until file transfer is complete */
}

static void show_queued_files(int idx)
{
  int i, cnt = 0, len;
  char spaces[] = "                                 ";
  fileq_t *q = fileq;

  while (q != NULL) {
    if (!strcasecmp(q->nick, dcc[idx].nick)) {
      if (!cnt) {
	spaces[HANDLEN - 9] = 0;
	dprintf(idx, "  Send to  %s  Filename\n", spaces);
	dprintf(idx, "  ---------%s  --------------------\n", spaces);
	spaces[HANDLEN - 9] = ' ';
      }
      cnt++;
      spaces[len = HANDLEN - strlen(q->to)] = 0;
      if (q->dir[0] == '*')
	dprintf(idx, "  %s%s  %s/%s\n", q->to, spaces, &q->dir[1],
		q->file);
      else
	dprintf(idx, "  %s%s  /%s%s%s\n", q->to, spaces, q->dir,
		q->dir[0] ? "/" : "", q->file);
      spaces[len] = ' ';
    }
    q = q->next;
  }
  for (i = 0; i < dcc_total; i++) {
    if (((dcc[i].type == &DCC_GET_PENDING) || (dcc[i].type == &DCC_GET)) &&
	((!strcasecmp(dcc[i].nick, dcc[idx].nick)) ||
	 (!strcasecmp(dcc[i].u.xfer->from, dcc[idx].nick)))) {
      char *nfn;

      if (!cnt) {
	spaces[HANDLEN - 9] = 0;
	dprintf(idx, "  Send to  %s  Filename\n", spaces);
	dprintf(idx, "  ---------%s  --------------------\n", spaces);
	spaces[HANDLEN - 9] = ' ';
      }
      nfn = strrchr(dcc[i].u.xfer->filename, '/');
      if (nfn == NULL)
	nfn = dcc[i].u.xfer->filename;
      else
	nfn++;
      cnt++;
      spaces[len = HANDLEN - strlen(dcc[i].nick)] = 0;
      if (dcc[i].type == &DCC_GET_PENDING)
	dprintf(idx, "  %s%s  %s  [WAITING]\n", dcc[i].nick, spaces,
		nfn);
      else
	dprintf(idx, "  %s%s  %s  (%.1f%% done)\n", dcc[i].nick, spaces,
		nfn, (100.0 * ((float) dcc[i].status /
			       (float) dcc[i].u.xfer->length)));
      spaces[len] = ' ';
    }
  }
  if (!cnt)
    dprintf(idx, "No files queued up.\n");
  else
    dprintf(idx, "Total: %d\n", cnt);
}

static void fileq_cancel(int idx, char *par)
{
  int fnd = 1, matches = 0, atot = 0, i;
  fileq_t *q;
  char s[256];

  while (fnd) {
    q = fileq;
    fnd = 0;
    while (q != NULL) {
      if (!strcasecmp(dcc[idx].nick, q->nick)) {
	if (q->dir[0] == '*')
	  sprintf(s, "%s/%s", &q->dir[1], q->file);
	else
	  sprintf(s, "/%s%s%s", q->dir, q->dir[0] ? "/" : "", q->file);
	if (wild_match_file(par, s)) {
	  dprintf(idx, "Cancelled: %s to %s\n", s, q->to);
	  fnd = 1;
	  deq_this(q);
	  q = NULL;
	  matches++;
	}
	if ((!fnd) && (wild_match_file(par, q->file))) {
	  dprintf(idx, "Cancelled: %s to %s\n", s, q->to);
	  fnd = 1;
	  deq_this(q);
	  q = NULL;
	  matches++;
	}
      }
      if (q != NULL)
	q = q->next;
    }
  }
  for (i = 0; i < dcc_total; i++) {
    if (((dcc[i].type == &DCC_GET_PENDING) || (dcc[i].type == &DCC_GET)) &&
	((!strcasecmp(dcc[i].nick, dcc[idx].nick)) ||
	 (!strcasecmp(dcc[i].u.xfer->from, dcc[idx].nick)))) {
      char *nfn = strrchr(dcc[i].u.xfer->filename, '/');

      if (nfn == NULL)
	nfn = dcc[i].u.xfer->filename;
      else
	nfn++;
      if (wild_match_file(par, nfn)) {
	dprintf(idx, "Cancelled: %s  (aborted dcc send)\n", nfn);
	if (strcasecmp(dcc[i].nick, dcc[idx].nick))
	  dprintf(DP_HELP, "NOTICE %s :Transfer of %s aborted by %s\n", dcc[i].nick,
		  nfn, dcc[idx].nick);
	if (dcc[i].type == &DCC_GET)
	  putlog(LOG_FILES, "*", "DCC cancel: GET %s (%s) at %lu/%lu", nfn,
		 dcc[i].nick, dcc[i].status, dcc[i].u.xfer->length);
	wipe_tmp_filename(dcc[i].u.xfer->filename, i);
	atot++;
	matches++;
	killsock(dcc[i].sock);
	lostdcc(i);
	i--;
      }
    }
  }
  if (!matches)
    dprintf(idx, "No matches.\n");
  else
    dprintf(idx, "Cancelled %d file%s.\n", matches, matches > 1 ? "s" : "");
  for (i = 0; i < atot; i++)
    if (!at_limit(dcc[idx].nick))
      send_next_file(dcc[idx].nick);
}

static int raw_dcc_send(char *filename, char *nick, char *from, char *dir)
{
  int zz, port, i;
  char *nfn;
  struct stat ss;
  FILE *f;

  context;
  port = reserved_port;
  zz = open_listen(&port);
  if (zz == (-1))
    return DCCSEND_NOSOCK;
  nfn = strrchr(filename, '/');
  if (nfn == NULL)
    nfn = filename;
  else
    nfn++;
  f = fopen(filename, "r");
  if (!f)
    return DCCSEND_BADFN;
  if ((i = new_dcc(&DCC_GET_PENDING, sizeof(struct xfer_info))) == -1)
     return DCCSEND_FULL;
  stat(filename, &ss);
  dcc[i].sock = zz;
  dcc[i].addr = (IP) (-559026163);
  dcc[i].port = port;
  strcpy(dcc[i].nick, nick);
  strcpy(dcc[i].host, "irc");
  strcpy(dcc[i].u.xfer->filename, filename);
  strcpy(dcc[i].u.xfer->from, from);
  strcpy(dcc[i].u.xfer->dir, dir);
  dcc[i].u.xfer->length = ss.st_size;
  dcc[i].timeval = now;
  dcc[i].u.xfer->f = f;
  if (nick[0] != '*') {
    dprintf(DP_HELP, "PRIVMSG %s :\001DCC SEND %s %lu %d %lu\001\n", nick, nfn,
	    iptolong(natip[0] ? (IP) inet_addr(natip) : getmyip()),
	    port, ss.st_size);
    putlog(LOG_FILES, "*", "Begin DCC send %s to %s", nfn, nick);
  }
  return DCCSEND_OK;
}

static tcl_ints myints[] =
{
  {"max-dloads", &dcc_limit},
  {"dcc-block", &dcc_block},
  {"copy-to-tmp", &copy_to_tmp},
  {"xfer-timeout", &wait_dcc_xfer},
  {0, 0}
};

static int fstat_unpack(struct userrec *u, struct user_entry *e)
{
  char *par, *arg;
  struct filesys_stats *fs;

  ASSERT (e != NULL);
  ASSERT (e->name != NULL);
  context;
  fs = user_malloc(sizeof(struct filesys_stats));
  bzero(fs, sizeof(struct filesys_stats));
  par = e->u.list->extra;
  arg = newsplit(&par);
  if (arg[0])
    fs->uploads = atoi(arg);
  arg = newsplit(&par);
  if (arg[0])
    fs->upload_ks = atoi(arg);
  arg = newsplit(&par);
  if (arg[0])
    fs->dnloads = atoi(arg);
  arg = newsplit(&par);
  if (arg[0])
    fs->dnload_ks = atoi(arg);

  list_type_kill(e->u.list);
  e->u.extra = fs;
  return 1;
}

static int fstat_pack(struct userrec *u, struct user_entry *e)
{
  register struct filesys_stats *fs;
  struct list_type *l = user_malloc(sizeof(struct list_type));

  ASSERT (e != NULL);
  ASSERT (e->name == NULL);
  ASSERT (e->u.extra != NULL);
  context;
  fs = e->u.extra;
  /* if you set it in the declaration, the ASSERT will be useless. ++rtc */

  l->extra = user_malloc(40);
  sprintf(l->extra, "%09u %09u %09u %09u",
          fs->uploads, fs->upload_ks, fs->dnloads, fs->dnload_ks);
  l->next = NULL;
  e->u.list = l;
  nfree(fs);
  return 1;
}

static int fstat_write_userfile(FILE * f, struct userrec *u,
				struct user_entry *e)
{
  register struct filesys_stats *fs;

  ASSERT (e != NULL);
  ASSERT (e->u.extra != NULL);
  context;
  fs = e->u.extra;
  if (fprintf(f, "--FSTAT %09u %09u %09u %09u\n",
	      fs->uploads, fs->upload_ks,
	      fs->dnloads, fs->dnload_ks) == EOF)
    return 0;
  return 1;
}

static int fstat_set(struct userrec *u, struct user_entry *e, void *buf)
{
  register struct filesys_stats *fs = buf;

  ASSERT (e != NULL);
  context;
  if (e->u.extra != fs) {
    if (e->u.extra)
      nfree (e->u.extra);
    e->u.extra = fs;
  } else if (!fs) /* e->u.extra == NULL && fs == NULL */
    return 1;

  if (!noshare && !(u->flags & (USER_BOT | USER_UNSHARED))) {
    if (fs)
      /* don't check here for something like
       *  ofs->uploads != fs->uploads || ofs->upload_ks != fs->upload_ks ||
       *  ofs->dnloads != fs->dnloads || ofs->dnload_ks != fs->dnload_ks
       * someone could do:
       *  e->u.extra->uploads = 12345;
       *  fs = user_malloc (sizeof (struct filesys_stats));
       *  memcpy (...e->u.extra...fs...);
       *  set_user (&USERENTRY_FSTAT, u, fs);
       * then we wouldn't detect here that something's changed...
       * --rtc
       */
      shareout (NULL, "ch fstat %09u %09u %09u %09u\n",
	        fs->uploads, fs->upload_ks, fs->dnloads, fs->dnload_ks);
    else
      shareout (NULL, "ch fstat r\n");
  }
  return 1;
}

static int fstat_tcl_get(Tcl_Interp * irp, struct userrec *u,
			 struct user_entry *e, int argc, char **argv)
{
  register struct filesys_stats *fs;
  char d[50];

  BADARGS(3, 4, " handle FSTAT ?u/d?");
  ASSERT (e != NULL)
  ASSERT (e->u.extra != NULL);
  fs = e->u.extra;
  if (argc == 3)
    simple_sprintf(d, "%u %u %u %u", fs->uploads, fs->upload_ks,
                   fs->dnloads, fs->dnload_ks);
  else
    switch (argv[3][0]) {
    case 'u':
      simple_sprintf(d, "%u %u", fs->uploads, fs->upload_ks);
      break;
    case 'd':
      simple_sprintf(d, "%u %u", fs->dnloads, fs->dnload_ks);
      break;
    }

  Tcl_AppendResult(irp, d, NULL);
  return TCL_OK;
}

static int fstat_kill(struct user_entry *e)
{
  ASSERT (e != NULL);
  context;
  if (e->u.extra)
    nfree(e->u.extra);
  nfree(e);
  return 1;
}

static int fstat_expmem(struct user_entry *e)
{
  return sizeof(struct filesys_stats);
}

static void fstat_display(int idx, struct user_entry *e)
{
  struct filesys_stats *fs;

  ASSERT (e != NULL);
  ASSERT (e->u.extra != NULL);

  fs = e->u.extra;
  dprintf(idx, "  FILES: %u download%s (%luk), %u upload%s (%luk)\n",
	  fs->dnloads, (fs->dnloads == 1) ? "" : "s", fs->dnload_ks,
	  fs->uploads, (fs->uploads == 1) ? "" : "s", fs->upload_ks);
}

static int fstat_gotshare(struct userrec *u, struct user_entry *e,
			  char *par, int idx);
static int fstat_dupuser(struct userrec *u, struct userrec *o,
			 struct user_entry *e);
static void stats_add_dnload(struct userrec *u, unsigned long bytes);
static void stats_add_upload(struct userrec *u, unsigned long bytes);
static int fstat_tcl_set(Tcl_Interp * irp, struct userrec *u,
			 struct user_entry *e, int argc, char **argv);

static struct user_entry_type USERENTRY_FSTAT =
{
  0,
  fstat_gotshare,
  fstat_dupuser,
  fstat_unpack,
  fstat_pack,
  fstat_write_userfile,
  fstat_kill,
  0,
  fstat_set,
  fstat_tcl_get,
  fstat_tcl_set,
  fstat_expmem,
  fstat_display,
  "FSTAT"
};

static int fstat_gotshare(struct userrec *u, struct user_entry *e,
			  char *par, int idx)
{
  char *p;
  struct filesys_stats *fs;

  ASSERT (e != NULL);
  noshare = 1;
  switch (par[0]) {
  case 'u':
  case 'd':
    /* no stats_add_up/dnload here, it's already been sent... --rtc */
    break;
  case 'r':
    set_user (&USERENTRY_FSTAT, u, NULL);
    break;
  default:
    if (!(fs = e->u.extra)) {
      fs = user_malloc(sizeof(struct filesys_stats));
      bzero(fs, sizeof(struct filesys_stats));
    }
    p = newsplit (&par);
    if (p[0])
      fs->uploads = atoi (p);
    p = newsplit (&par);
    if (p[0])
      fs->upload_ks = atoi (p);
    p = newsplit (&par);
    if (p[0])
      fs->dnloads = atoi (p);
    p = newsplit (&par);
    if (p[0])
      fs->dnload_ks = atoi (p);
    set_user(&USERENTRY_FSTAT, u, fs);
    break;
  }
  noshare = 0;
  return 1;
}

static int fstat_dupuser(struct userrec *u, struct userrec *o,
			 struct user_entry *e)
{
  struct filesys_stats *fs;

  context;
  if (e->u.extra) {
    fs = user_malloc(sizeof(struct filesys_stats));
    my_memcpy(fs, e->u.extra, sizeof(struct filesys_stats));

    return set_user(&USERENTRY_FSTAT, u, fs);
  }
  return 0;
}

static void stats_add_dnload(struct userrec *u, unsigned long bytes)
{
  struct user_entry *ue;
  register struct filesys_stats *fs;

  if (u) {
    if (!(ue = find_user_entry (&USERENTRY_FSTAT, u)) ||
        !(fs = ue->u.extra)) {
      fs = user_malloc(sizeof(struct filesys_stats));
      bzero(fs, sizeof(struct filesys_stats));
    }
    fs->dnloads++;
    fs->dnload_ks += ((bytes + 512) / 1024);
    set_user(&USERENTRY_FSTAT, u, fs);
    /* no shareout here, set_user already sends info... --rtc */
  }
}

static void stats_add_upload(struct userrec *u, unsigned long bytes)
{
  struct user_entry *ue;
  register struct filesys_stats *fs;

  if (u) {
    if (!(ue = find_user_entry (&USERENTRY_FSTAT, u)) ||
        !(fs = ue->u.extra)) {
      fs = user_malloc(sizeof(struct filesys_stats));
      bzero(fs, sizeof(struct filesys_stats));
    }
    fs->uploads++;
    fs->upload_ks += ((bytes + 512) / 1024);
    set_user(&USERENTRY_FSTAT, u, fs);
    /* no shareout here, set_user already sends info... --rtc */
  }
}

static int fstat_tcl_set(Tcl_Interp * irp, struct userrec *u,
			 struct user_entry *e, int argc, char **argv)
{
  register struct filesys_stats *fs;
  int f = 0, k = 0;

  ASSERT (e != NULL);
  BADARGS(4, 6, " handle FSTAT u/d ?files ?ks??");

  if (argc > 4)
    f = atoi(argv[4]);
  if (argc > 5)
    k = atoi(argv[5]);
  switch (argv[3][0]) {
  case 'u':
  case 'd':
    if (!(fs = e->u.extra)) {
      fs = user_malloc(sizeof(struct filesys_stats));
      bzero(fs, sizeof(struct filesys_stats));
    }
    switch (argv[3][0]) {
    case 'u':
      fs->uploads = f;
      fs->upload_ks = k;
      break;
    case 'd':
      fs->dnloads = f;
      fs->dnload_ks = k;
      break;
    }
    set_user (&USERENTRY_FSTAT, u, fs);
    break;
  case 'r':
    set_user (&USERENTRY_FSTAT, u, NULL);
    break;
  }
  return TCL_OK;
}

static char *transfer_close()
{
  int i;

  context;
  putlog(LOG_MISC, "*", "Unloading transfer module, killing all transfer connections..");
  for (i = dcc_total - 1; i >= 0; i--) {
    if (dcc[i].type == &DCC_GET || dcc[i].type == &DCC_GET_PENDING)
      eof_dcc_get(i);
    else if (dcc[i].type == &DCC_SEND)
      eof_dcc_send(i);
    else if (dcc[i].type == &DCC_FORK_SEND)
      eof_dcc_fork_send(i);
  }
  while (fileq)
    deq_this(fileq);
  del_entry_type(&USERENTRY_FSTAT);
  del_bind_table(H_rcvd);
  del_bind_table(H_sent);
  rem_tcl_commands(mytcls);
  rem_tcl_ints(myints);
  rem_help_reference("transfer.help");
  module_undepend(MODULE_NAME);
  return NULL;
}

static int transfer_expmem()
{
  return expmem_fileq();
}

static void transfer_report(int idx, int details)
{
  if (details) {
    dprintf(idx, "    DCC block is %d%s, max concurrent d/ls is %d\n", dcc_block,
	    (dcc_block == 0) ? " (turbo dcc)" : "", dcc_limit);
    dprintf(idx, "    Using %d bytes of memory\n", transfer_expmem());
  }
}

char *transfer_start();

static Function transfer_table[] =
{
  (Function) transfer_start,
  (Function) transfer_close,
  (Function) transfer_expmem,
  (Function) transfer_report,
  /* 4- 7 */
  (Function) & DCC_FORK_SEND,
  (Function) at_limit,
  (Function) & copy_to_tmp,
  (Function) fileq_cancel,
  /* 8 - 11 */
  (Function) queue_file,
  (Function) raw_dcc_send,
  (Function) show_queued_files,
  (Function) wild_match_file,
  /* 12 - 15 */
  (Function) wipe_tmp_filename,
  (Function) & DCC_GET,
  (Function) & H_rcvd,
  (Function) & H_sent,
  /* 16 - 19 */
  (Function) & USERENTRY_FSTAT,
  (Function) & quiet_reject,        /* int */
};

char *transfer_start(Function * global_funcs)
{
  global = global_funcs;

  fileq = NULL;
  context;
  module_register(MODULE_NAME, transfer_table, 2, 0);
  if (!module_depend(MODULE_NAME, "eggdrop", 104, 0))
    return "This module requires eggdrop1.4.0 or later";
  add_tcl_commands(mytcls);
  add_tcl_ints(myints);
  add_help_reference("transfer.help");
  H_rcvd = add_bind_table("rcvd", HT_STACKABLE, builtin_sentrcvd);
  H_sent = add_bind_table("sent", HT_STACKABLE, builtin_sentrcvd);
  USERENTRY_FSTAT.get = def_get;
  add_entry_type(&USERENTRY_FSTAT);
  return NULL;
}
