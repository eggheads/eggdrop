/*
 * transfer.c -- part of transfer.mod
 *
 * $Id: transfer.c,v 1.77 2011/02/13 14:19:34 simple Exp $
 *
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
 *
 * Small code snippets related to REGET/RESEND support were taken from
 * BitchX, copyright by panasync.
 */

#define MODULE_NAME "transfer"
#define MAKING_TRANSFER

/* sigh sunos */
#include <sys/types.h>
#include <sys/stat.h>
#include "src/mod/module.h"
#include "src/tandem.h"

#include "src/users.h"
#include "transfer.h"

#include <netinet/in.h>
#include <arpa/inet.h>


static Function *global = NULL;

static p_tcl_bind_list H_rcvd, H_sent, H_lost, H_tout;

static int wait_dcc_xfer = 300; /* Timeout time on DCC xfers */
static int dcc_limit = 3;       /* Max simultaneous downloads allowed */
static int dcc_block = 0;       /* Size of one dcc block */
static fileq_t *fileq = NULL;

static struct dcc_table DCC_SEND, DCC_GET, DCC_GET_PENDING;

#include "transferfstat.c"
#include "transferqueue.c"
#include "tcltransfer.c"

/* Forward, case-sensitive file matching supporting the '*' and '?'
 * wildcard characters. This is basically a direct copy of wild_match_per(),
 * but without support for '%', or '~'.
 */
static int wild_match_file(register char *m, register char *n)
{
  char *ma = m, *lsm = 0, *lsn = 0;
  int match = 1;
  register unsigned int sofar = 0;

  /* null strings should never match */
  if ((m == 0) || (n == 0) || (!*n))
    return 0;

  while (*n) {
    switch (*m) {
    case 0:
      do
        m--;
      while ((m > ma) && (*m == '?'));
      if ((m > ma) ? ((*m == '*') && (m[-1] != FILEQUOTE)) : (*m == '*'))
        return FILEMATCH;
      break;
    case FILEWILDS:
      do
        m++;
      while (*m == FILEWILDS);
      lsm = m;
      lsn = n;
      match += sofar;
      sofar = 0;
      continue;
    case FILEWILDQ:
      m++;
      n++;
      continue;
    case FILEQUOTE:
      m++;
    }
    if (*m == *n) {
      m++;
      n++;
      sofar++;
      continue;
    }
    if (lsm) {
      n = ++lsn;
      m = lsm;
      sofar = 0;
      continue;
    }
    return 0;
  }
  while (*m == FILEWILDS)
    m++;
  return (*m) ? 0 : FILEMATCH;
}

/* Remove a temp file from /tmp if not being used elsewhere. */
static void wipe_tmp_filename(char *fn, int idx)
{
  int i, ok = 1;

  if (!copy_to_tmp)
    return;

  for (i = 0; i < dcc_total; i++) {
    if ((i != idx) &&
        (dcc[i].type == &DCC_GET || dcc[i].type == &DCC_GET_PENDING) &&
        (!strcmp(dcc[i].u.xfer->filename, fn))) {
      ok = 0;
      break;
    }
  }

  if (ok)
    unlink(fn);
}

/* Return true if this user has >= the maximum number of file xfers allowed.
 */
static int at_limit(char *nick)
{
  int i, x = 0;

  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_GET || dcc[i].type == &DCC_GET_PENDING) &&
        !egg_strcasecmp(dcc[i].nick, nick))
      x++;

  return (x >= dcc_limit);
}

/* Replaces all spaces with underscores (' ' -> '_').  The returned buffer
 * needs to be freed after use.
 */
static char *replace_spaces(char *fn)
{
  register char *ret, *p;

  p = ret = nmalloc(strlen(fn) + 1);
  strcpy(ret, fn);
  while ((p = strchr(p, ' ')) != NULL)
    *p = '_';
  return ret;
}


static int builtin_sentrcvd STDVAR
{
  Function F = (Function) cd;

  BADARGS(4, 4, " hand nick path");

  CHECKVALIDITY(builtin_sentrcvd);
  F(argv[1], argv[2], argv[3]);
  return TCL_OK;
}

static int builtin_toutlost STDVAR
{
  Function F = (Function) cd;

  BADARGS(6, 6, " hand nick path acked length");

  CHECKVALIDITY(builtin_toutlost);
  F(argv[1], argv[2], argv[3], argv[4], argv[5]);
  return TCL_OK;
}

static void check_tcl_sentrcvd(struct userrec *u, char *nick, char *path,
                               p_tcl_bind_list h)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };
  char *hand = u ? u->handle : "*";

  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_sr1", hand, 0);
  Tcl_SetVar(interp, "_sr2", nick, 0);
  Tcl_SetVar(interp, "_sr3", path, 0);
  check_tcl_bind(h, hand, &fr, " $_sr1 $_sr2 $_sr3",
                 MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE);
}

static void check_tcl_toutlost(struct userrec *u, char *nick, char *path,
                               unsigned long acked, unsigned long length,
                               p_tcl_bind_list h)
{
  struct flag_record fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };
  char *hand = u ? u->handle : "*";
  char s[15];

  get_user_flagrec(u, &fr, NULL);
  Tcl_SetVar(interp, "_sr1", hand, 0);
  Tcl_SetVar(interp, "_sr2", nick, 0);
  Tcl_SetVar(interp, "_sr3", path, 0);
  egg_snprintf(s, sizeof s, "%lu", acked);
  Tcl_SetVar(interp, "_sr4", s, 0);
  egg_snprintf(s, sizeof s, "%lu", length);
  Tcl_SetVar(interp, "_sr5", s, 0);
  check_tcl_bind(h, hand, &fr, " $_sr1 $_sr2 $_sr3 $_sr4 $_sr5",
                 MATCH_MASK | BIND_USE_ATTR | BIND_STACKABLE);
}

/*
 *    DCC routines
 */

/* Instead of reading all data intended to go into the DCC block
 * in one go, we read it in PMAX_SIZE chunks, feed it to tputs and
 * continue until we get to know that the network buffer only
 * buffers the data instead of sending it.
 *
 * In that case, we delay further sending until we receive the
 * dcc outdone event.
 *
 * Note: To optimize buffer sizes, we default to PMAX_SIZE, but
 *       allocate a smaller buffer for smaller pending_data sizes.
 */
#define PMAX_SIZE 4096
static unsigned long pump_file_to_sock(FILE *file, long sock,
                                       register unsigned long pending_data)
{
  register unsigned long actual_size;
  const unsigned long buf_len = pending_data >= PMAX_SIZE ?
                      PMAX_SIZE : pending_data;
  char *bf = nmalloc(buf_len);

  if (bf) {
    do {
      actual_size = pending_data >= buf_len ? buf_len : pending_data;
      fread(bf, actual_size, 1, file);
      tputs(sock, bf, actual_size);
      pending_data -= actual_size;
    } while (!sock_has_data(SOCK_DATA_OUTGOING, sock) && pending_data != 0);
    nfree(bf);
  }
  return pending_data;
}

static void eof_dcc_fork_send(int idx)
{
  char s1[121];
  char *s2;

  fclose(dcc[idx].u.xfer->f);
  if (!strcmp(dcc[idx].nick, "*users")) {
    int x, y = 0;

    for (x = 0; x < dcc_total; x++)
      if ((!egg_strcasecmp(dcc[x].nick, dcc[idx].host)) &&
          (dcc[x].type->flags & DCT_BOT)) {
        y = x;
        break;
      }
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
           dcc[idx].u.xfer->origname, dcc[idx].nick, dcc[idx].host);
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
  char *ofn, *nfn, s[1024], *hand;
  struct userrec *u;

  fclose(dcc[idx].u.xfer->f);

  if (dcc[idx].u.xfer->length == dcc[idx].status) {
    int l;

    /* Success */
    ok = 0;
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
    putlog(LOG_FILES, "*", TRANSFER_COMPLETED_DCC, dcc[idx].u.xfer->origname,
           dcc[idx].nick, dcc[idx].host);
    egg_snprintf(s, sizeof s, "%s!%s", dcc[idx].nick, dcc[idx].host);
    u = get_user_by_host(s);
    hand = u ? u->handle : "*";

    l = strlen(dcc[idx].u.xfer->filename);
    if (l > NAME_MAX) {         /* Filename is too long. */
      putlog(LOG_FILES, "*", TRANSFER_FILENAME_TOOLONG, l);
      dprintf(DP_HELP, TRANSFER_NOTICE_FNTOOLONG, dcc[idx].nick, l);
      putlog(LOG_FILES, "*", TRANSFER_TOO_BAD);
      dprintf(DP_HELP, TRANSFER_NOTICE_TOOBAD, dcc[idx].nick);
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return;
    }
    /* Move the file from /tmp */
    ofn = nmalloc(strlen(tempdir) + strlen(dcc[idx].u.xfer->filename) + 1);
    nfn = nmalloc(strlen(dcc[idx].u.xfer->dir)
                  + strlen(dcc[idx].u.xfer->origname) + 1);
    sprintf(ofn, "%s%s", tempdir, dcc[idx].u.xfer->filename);
    sprintf(nfn, "%s%s", dcc[idx].u.xfer->dir, dcc[idx].u.xfer->origname);
    if (movefile(ofn, nfn))
      putlog(LOG_MISC | LOG_FILES, "*", TRANSFER_FAILED_MOVE, nfn, ofn);
    else {
      /* Add to file database */
      module_entry *fs = module_find("filesys", 0, 0);

      if (fs != NULL) {
        Function f = fs->funcs[FILESYS_ADDFILE];

        f(dcc[idx].u.xfer->dir, dcc[idx].u.xfer->origname, hand);
      }
      stats_add_upload(u, dcc[idx].u.xfer->length);
      check_tcl_sentrcvd(u, dcc[idx].nick, nfn, H_rcvd);
    }
    nfree(ofn);
    nfree(nfn);
    for (j = 0; j < dcc_total; j++)
      if (!ok && (dcc[j].type->flags & (DCT_GETNOTES | DCT_FILES)) &&
          !egg_strcasecmp(dcc[j].nick, hand)) {
        ok = 1;
        dprintf(j, TRANSFER_THANKS);
      }
    if (!ok)
      dprintf(DP_HELP, TRANSFER_NOTICE_THANKS, dcc[idx].nick);
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return;
  }
  /* Failure :( */
  if (!strcmp(dcc[idx].nick, "*users")) {
    int x, y = 0;

    for (x = 0; x < dcc_total; x++)
      if ((!egg_strcasecmp(dcc[x].nick, dcc[idx].host)) &&
          (dcc[x].type->flags & DCT_BOT))
        y = x;
    if (y) {
      putlog(LOG_BOTS, "*", TRANSFER_USERFILE_LOST, dcc[y].nick);
      unlink(dcc[idx].u.xfer->filename);
      /* Drop that bot */
      dprintf(y, "bye\n");
      egg_snprintf(s, sizeof s, TRANSFER_USERFILE_DISCON, dcc[y].nick);
      botnet_send_unlinked(y, dcc[y].nick, s);
      chatout("*** %s\n", dcc[y].nick, s);
      if (y != idx) {
        killsock(dcc[y].sock);
        lostdcc(y);
      }
      killsock(dcc[idx].sock);
      lostdcc(idx);
    }
  } else {
    putlog(LOG_FILES, "*", TRANSFER_LOST_DCCSEND, dcc[idx].u.xfer->origname,
           dcc[idx].nick, dcc[idx].host, dcc[idx].status,
           dcc[idx].u.xfer->length);
    ofn = nmalloc(strlen(tempdir) + strlen(dcc[idx].u.xfer->filename) + 1);
    sprintf(ofn, "%s%s", tempdir, dcc[idx].u.xfer->filename);
    unlink(ofn);
    nfree(ofn);
    killsock(dcc[idx].sock);
    lostdcc(idx);
  }
}

/* Determine byte order. Used for resend DCC startup packets.
 */
static inline u_8bit_t byte_order_test(void)
{
  u_16bit_t test = TRANSFER_REGET_PACKETID;

  if (*((u_8bit_t *) & test) == ((TRANSFER_REGET_PACKETID & 0xff00) >> 8))
    return 0;
  if (*((u_8bit_t *) & test) == (TRANSFER_REGET_PACKETID & 0x00ff))
    return 1;
  return 0;
}

/* Parse and handle resend DCC startup packets.
 */
inline static void handle_resend_packet(int idx, transfer_reget *reget_data)
{
  if (byte_order_test() != reget_data->byte_order) {
    /* The sender's byte order does not match our's so we need to switch the
     * bytes first, before we can make use of them.
     */
    reget_data->packet_id = ((reget_data->packet_id & 0x00ff) << 8) |
                            ((reget_data->packet_id & 0xff00) >> 8);
    reget_data->byte_offset = ((reget_data->byte_offset & 0xff000000) >> 24) |
                              ((reget_data->byte_offset & 0x00ff0000) >> 8) |
                              ((reget_data->byte_offset & 0x0000ff00) << 8) |
                              ((reget_data->byte_offset & 0x000000ff) << 24);
  }
  if (reget_data->packet_id != TRANSFER_REGET_PACKETID)
    putlog(LOG_FILES, "*", TRANSFER_REGET_PACKET, dcc[idx].nick,
           dcc[idx].u.xfer->origname);
  else
    dcc[idx].u.xfer->offset = reget_data->byte_offset;
  dcc[idx].u.xfer->type = XFER_RESEND;
}

/* Handles DCC packets the client sends us. As soon as the last sent dcc
 * block is fully acknowledged we send the next block.
 *
 * Note: The first received packet during reget is a special 8 bit packet
 *       containing special information.
 */
static void dcc_get(int idx, char *buf, int len)
{
  char xnick[NICKLEN];
  unsigned char bbuf[4];
  unsigned long cmp, l;
  int w = len + dcc[idx].u.xfer->sofar, p = 0;

  dcc[idx].timeval = now;       /* Mark as active               */

  /* Add bytes to our buffer if we don't have a complete response yet.
   * This is either a 4 bit ack or the 8 bit reget packet.
   */
  if (w < 4 || (w < 8 && dcc[idx].u.xfer->type == XFER_RESEND_PEND)) {
    my_memcpy(&(dcc[idx].u.xfer->buf[dcc[idx].u.xfer->sofar]), buf, len);
    dcc[idx].u.xfer->sofar += len;
    return;
    /* Waiting for the 8 bit reget packet? */
  } else if (dcc[idx].u.xfer->type == XFER_RESEND_PEND) {
    /* The 8 bit packet is complete now. Parse it. */
    if (w == 8) {
      transfer_reget reget_data;

      my_memcpy(&reget_data, dcc[idx].u.xfer->buf, dcc[idx].u.xfer->sofar);
      my_memcpy(&reget_data + dcc[idx].u.xfer->sofar, buf, len);
      handle_resend_packet(idx, &reget_data);
      cmp = dcc[idx].u.xfer->offset;
    } else
      return;
    /* Fall through! */
    /* No, only want 4 bit ack responses. */
  } else {
    /* Complete packet? */
    if (w == 4) {
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
    }
    /* This is more compatible than ntohl for machines where an int
     * is more than 4 bytes:
     */
    cmp = ((unsigned int) (bbuf[0]) << 24) +
          ((unsigned int) (bbuf[1]) << 16) +
          ((unsigned int) (bbuf[2]) << 8) + bbuf[3];
    dcc[idx].u.xfer->acked = cmp;
  }

  dcc[idx].u.xfer->sofar = 0;
  if (cmp > dcc[idx].u.xfer->length && cmp > dcc[idx].status) {
    /* Attempt to resume, but file is not as long as requested... */
    putlog(LOG_FILES, "*", TRANSFER_BEHIND_FILEEND, dcc[idx].u.xfer->origname,
           dcc[idx].nick);
  } else if (cmp > dcc[idx].status) {
    /* Attempt to resume */
    if (!strcmp(dcc[idx].nick, "*users"))
      putlog(LOG_BOTS, "*", TRANSFER_TRY_SKIP_AHEAD);
    else {
      fseek(dcc[idx].u.xfer->f, cmp, SEEK_SET);
      dcc[idx].status = cmp;
      putlog(LOG_FILES, "*", TRANSFER_RESUME_FILE, (int) (cmp / 1024),
             dcc[idx].u.xfer->origname, dcc[idx].nick);
    }
  } else {
    if (dcc[idx].u.xfer->ack_type == XFER_ACK_UNKNOWN) {
      if (cmp < dcc[idx].u.xfer->offset)
        /* If we don't start at the top of the file, some clients only tell
         * us the really received bytes (e.g. bitchx). This seems to be the
         * case here.
         */
        dcc[idx].u.xfer->ack_type = XFER_ACK_WITHOUT_OFFSET;
      else
        dcc[idx].u.xfer->ack_type = XFER_ACK_WITH_OFFSET;
    }
    if (dcc[idx].u.xfer->ack_type == XFER_ACK_WITHOUT_OFFSET)
      cmp += dcc[idx].u.xfer->offset;
  }

  if (cmp != dcc[idx].status)
    return;
  if (dcc[idx].status == dcc[idx].u.xfer->length) {
    /* Successful send, we are done */
    killsock(dcc[idx].sock);
    fclose(dcc[idx].u.xfer->f);
    if (!strcmp(dcc[idx].nick, "*users")) {
      module_entry *me = module_find("share", 0, 0);
      int x, y = 0;

      for (x = 0; x < dcc_total; x++)
        if (!egg_strcasecmp(dcc[x].nick, dcc[idx].host) &&
            (dcc[x].type->flags & DCT_BOT))
          y = x;
      if (y != 0)
        dcc[y].status &= ~STAT_SENDING;
      putlog(LOG_BOTS, "*", TRANSFER_COMPLETED_USERFILE, dcc[y].nick);
      unlink(dcc[idx].u.xfer->filename);
      /* Any sharebot things that were queued: */
      if (me && me->funcs[SHARE_DUMP_RESYNC])
        ((me->funcs)[SHARE_DUMP_RESYNC]) (y);
      xnick[0] = 0;
    } else {
      module_entry *fs = module_find("filesys", 0, 0);
      struct userrec *u = get_user_by_handle(userlist, dcc[idx].u.xfer->from);

      check_tcl_sentrcvd(u, dcc[idx].nick, dcc[idx].u.xfer->dir, H_sent);
      if (fs != NULL) {
        Function f = fs->funcs[FILESYS_INCRGOTS];

        f(dcc[idx].u.xfer->dir);
      }
      /* Download is credited to the user who requested it
       * (not the user who actually received it)
       */
      stats_add_dnload(u, dcc[idx].u.xfer->length);
      putlog(LOG_FILES, "*", TRANSFER_FINISHED_DCCSEND,
             dcc[idx].u.xfer->origname, dcc[idx].nick);
      wipe_tmp_filename(dcc[idx].u.xfer->filename, idx);
      strcpy((char *) xnick, dcc[idx].nick);
    }
    lostdcc(idx);
    /* Any to dequeue? */
    if (!at_limit(xnick))
      send_next_file(xnick);
    return;
  }
  /* Note:  No fseek() needed here, because the file position is kept from
   *        the last run.
   */
  l = dcc_block;
  if (l == 0 || dcc[idx].status + l > dcc[idx].u.xfer->length)
    l = dcc[idx].u.xfer->length - dcc[idx].status;
  dcc[idx].u.xfer->block_pending = pump_file_to_sock(dcc[idx].u.xfer->f,
                                                     dcc[idx].sock, l);
  dcc[idx].status += l;
}

static void eof_dcc_get(int idx)
{
  char xnick[NICKLEN], s[1024];

  fclose(dcc[idx].u.xfer->f);
  if (!strcmp(dcc[idx].nick, "*users")) {
    int x, y = 0;

    for (x = 0; x < dcc_total; x++)
      if (!egg_strcasecmp(dcc[x].nick, dcc[idx].host) &&
          (dcc[x].type->flags & DCT_BOT))
        y = x;
    putlog(LOG_BOTS, "*", TRANSFER_ABORT_USERFILE);
    /* Note: no need to unlink the xfer file, as it's already unlinked. */
    xnick[0] = 0;
    /* Drop that bot */
    dprintf(-dcc[y].sock, "bye\n");
    egg_snprintf(s, sizeof s, TRANSFER_USERFILE_DISCON, dcc[y].nick);
    botnet_send_unlinked(y, dcc[y].nick, s);
    chatout("*** %s\n", s);
    if (y != idx) {
      killsock(dcc[y].sock);
      lostdcc(y);
    }
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return;
  } else {
    struct userrec *u;

    /* Call `lost' DCC trigger now.
     */
    egg_snprintf(s, sizeof s, "%s!%s", dcc[idx].nick, dcc[idx].host);
    u = get_user_by_host(s);
    check_tcl_toutlost(u, dcc[idx].nick, dcc[idx].u.xfer->dir,
                       dcc[idx].u.xfer->acked, dcc[idx].u.xfer->length, H_lost);

    putlog(LOG_FILES, "*", TRANSFER_LOST_DCCGET,
           dcc[idx].u.xfer->origname, dcc[idx].nick, dcc[idx].host);
    wipe_tmp_filename(dcc[idx].u.xfer->filename, idx);
    strcpy(xnick, dcc[idx].nick);
  }
  killsock(dcc[idx].sock);
  lostdcc(idx);
  /* Send next queued file if there is one */
  if (!at_limit(xnick))
    send_next_file(xnick);
}

static void dcc_send(int idx, char *buf, int len)
{
  char s[512], *b;
  unsigned long sent;

  fwrite(buf, len, 1, dcc[idx].u.xfer->f);
  dcc[idx].status += len;
  /* Put in network byte order */
  sent = dcc[idx].status;
  s[0] = (sent / (1 << 24));
  s[1] = (sent % (1 << 24)) / (1 << 16);
  s[2] = (sent % (1 << 16)) / (1 << 8);
  s[3] = (sent % (1 << 8));
  tputs(dcc[idx].sock, s, 4);
  dcc[idx].timeval = now;
  if (dcc[idx].status > dcc[idx].u.xfer->length && dcc[idx].u.xfer->length > 0) {
    dprintf(DP_HELP, TRANSFER_BOGUS_FILE_LENGTH, dcc[idx].nick);
    putlog(LOG_FILES, "*", TRANSFER_FILE_TOO_LONG, dcc[idx].u.xfer->origname,
           dcc[idx].nick, dcc[idx].host);
    fclose(dcc[idx].u.xfer->f);
    b = nmalloc(strlen(tempdir) + strlen(dcc[idx].u.xfer->filename) + 1);
    sprintf(b, "%s%s", tempdir, dcc[idx].u.xfer->filename);
    unlink(b);
    nfree(b);
    killsock(dcc[idx].sock);
    lostdcc(idx);
  }
}

static void transfer_get_timeout(int i)
{
  char xx[1024];

  fclose(dcc[i].u.xfer->f);
  if (strcmp(dcc[i].nick, "*users") == 0) {
    int x, y = 0;

    for (x = 0; x < dcc_total; x++)
      if ((!egg_strcasecmp(dcc[x].nick, dcc[i].host)) &&
          (dcc[x].type->flags & DCT_BOT))
        y = x;
    if (y != 0) {
      dcc[y].status &= ~STAT_SENDING;
      dcc[y].status &= ~STAT_SHARE;
    }
    unlink(dcc[i].u.xfer->filename);
    putlog(LOG_BOTS, "*", TRANSFER_USERFILE_TIMEOUT);
    dprintf(y, "bye\n");
    egg_snprintf(xx, sizeof xx, TRANSFER_DICONNECT_TIMEOUT, dcc[y].nick);
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
    struct userrec *u;

    p = strrchr(dcc[i].u.xfer->origname, '/');
    dprintf(DP_HELP, TRANSFER_NOTICE_TIMEOUT, dcc[i].nick,
            p ? p + 1 : dcc[i].u.xfer->origname);

    /* Call DCC `timeout' trigger now.
     */
    egg_snprintf(xx, sizeof xx, "%s!%s", dcc[i].nick, dcc[i].host);
    u = get_user_by_host(xx);
    check_tcl_toutlost(u, dcc[i].nick, dcc[i].u.xfer->dir,
                       dcc[i].u.xfer->acked, dcc[i].u.xfer->length, H_tout);

    putlog(LOG_FILES, "*", TRANSFER_DCC_GET_TIMEOUT,
           p ? p + 1 : dcc[i].u.xfer->origname, dcc[i].nick, dcc[i].status,
           dcc[i].u.xfer->length);
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

  fclose(dcc[idx].u.xfer->f);

  if (!strcmp(dcc[idx].nick, "*users")) {
    int x, y = 0;

    for (x = 0; x < dcc_total; x++)
      if (!egg_strcasecmp(dcc[x].nick, dcc[idx].host) &&
          (dcc[x].type->flags & DCT_BOT))
        y = x;

    if (y != 0) {
      dcc[y].status &= ~STAT_GETTING;
      dcc[y].status &= ~STAT_SHARE;
    }

    unlink(dcc[idx].u.xfer->filename);

    putlog(LOG_BOTS, "*", TRANSFER_USERFILE_TIMEOUT);
  } else {
    char *buf;

    buf = nmalloc(strlen(tempdir) + strlen(dcc[idx].u.xfer->filename) + 1);
    sprintf(buf, "%s%s", tempdir, dcc[idx].u.xfer->filename);
    unlink(buf);
    nfree(buf);

    dprintf(DP_HELP, TRANSFER_NOTICE_TIMEOUT, dcc[idx].nick,
            dcc[idx].u.xfer->origname);
    putlog(LOG_FILES, "*", TRANSFER_DCC_SEND_TIMEOUT, dcc[idx].u.xfer->origname,
           dcc[idx].nick, dcc[idx].status, dcc[idx].u.xfer->length);
  }

  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void display_dcc_get(int idx, char *buf)
{
  if (dcc[idx].status == dcc[idx].u.xfer->length)
    sprintf(buf, TRANSFER_SEND, dcc[idx].u.xfer->acked,
            dcc[idx].u.xfer->length, dcc[idx].u.xfer->origname);
  else
    sprintf(buf, TRANSFER_SEND, dcc[idx].status,
            dcc[idx].u.xfer->length, dcc[idx].u.xfer->origname);
}

static void display_dcc_get_p(int idx, char *buf)
{
  sprintf(buf, TRANSFER_SEND_WAITED, now - dcc[idx].timeval,
          dcc[idx].u.xfer->origname);
}

static void display_dcc_send(int idx, char *buf)
{
  sprintf(buf, TRANSFER_SEND, dcc[idx].status,
          dcc[idx].u.xfer->length, dcc[idx].u.xfer->origname);
}

static void display_dcc_fork_send(int idx, char *buf)
{
  sprintf(buf, TRANSFER_CONN_SEND);
}

static int expmem_dcc_xfer(void *x)
{
  register struct xfer_info *p = (struct xfer_info *) x;
  int tot;

  tot = sizeof(struct xfer_info);
  if (p->filename)
    tot += strlen(p->filename) + 1;
  /* We need to check if origname points to filename before
   * accounting for the memory.
   */
  if (p->origname && p->filename != p->origname)
    tot += strlen(p->origname) + 1;
  return tot;
}

static void kill_dcc_xfer(int idx, void *x)
{
  register struct xfer_info *p = (struct xfer_info *) x;

  if (p->filename)
    nfree(p->filename);
  /* We need to check if origname points to filename before
   * attempting to free the memory.
   */
  if (p->origname && p->origname != p->filename)
    nfree(p->origname);
  nfree(x);
}

static void out_dcc_xfer(int idx, char *buf, void *x)
{
}

static void outdone_dcc_xfer(int idx)
{
  if (dcc[idx].u.xfer->block_pending)
    dcc[idx].u.xfer->block_pending =
      pump_file_to_sock(dcc[idx].u.xfer->f, dcc[idx].sock,
                        dcc[idx].u.xfer->block_pending);
}

/* Send TO the bot - Wcc */
static struct dcc_table DCC_SEND = {
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

/* Send TO the bot from outside of the transfer module - Wcc */
static struct dcc_table DCC_FORK_SEND = {
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

/* Send FROM the bot, don't know why this isn't called DCC_SEND - Wcc */
static struct dcc_table DCC_GET = {
  "GET",
  DCT_FILETRAN | DCT_VALIDIDX,
  eof_dcc_get,
  dcc_get,
  &wait_dcc_xfer,
  transfer_get_timeout,
  display_dcc_get,
  expmem_dcc_xfer,
  kill_dcc_xfer,
  out_dcc_xfer,
  outdone_dcc_xfer
};

/* Send FROM the bot - Wcc */
static struct dcc_table DCC_GET_PENDING = {
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

static void dcc_fork_send(int idx, char *x, int y)
{
  char s1[121];

  if (dcc[idx].type != &DCC_FORK_SEND)
    return;

  dcc[idx].type = &DCC_SEND;
  dcc[idx].u.xfer->start_time = now;

  if (strcmp(dcc[idx].nick, "*users")) {
    egg_snprintf(s1, sizeof s1, "%s!%s", dcc[idx].nick, dcc[idx].host);
    putlog(LOG_MISC, "*", TRANSFER_DCC_CONN, dcc[idx].u.xfer->origname, s1);
  }
}

static void dcc_get_pending(int idx, char *buf, int len)
{
  unsigned long ip;
  unsigned short port;
  int i;
  char s[UHOSTLEN];

  i = answer(dcc[idx].sock, s, &ip, &port, 1);
  killsock(dcc[idx].sock);
  dcc[idx].sock = i;
  dcc[idx].addr = ip;
  dcc[idx].port = (int) port;
  if (dcc[idx].sock == -1) {
    neterror(s);
    dprintf(DP_HELP, TRANSFER_NOTICE_BAD_CONN, dcc[idx].nick, s);
    putlog(LOG_FILES, "*", TRANSFER_LOG_BAD_CONN, dcc[idx].u.xfer->origname,
           dcc[idx].nick, dcc[idx].host);
    fclose(dcc[idx].u.xfer->f);
    lostdcc(idx);
    return;
  }

  dcc[idx].type = &DCC_GET;
  dcc[idx].u.xfer->ack_type = XFER_ACK_UNKNOWN;

  /*
   * Note: The file was already opened and dcc[idx].u.xfer->f may be
   *       used immediately. Leave it opened until the file transfer
   *       is complete.
   */

  if (dcc[idx].u.xfer->type == XFER_RESUME_PEND) { /* Are we resuming? */
    int l;

    if (dcc_block == 0 || dcc[idx].u.xfer->length < dcc_block) {
      l = dcc[idx].u.xfer->length - dcc[idx].u.xfer->offset;
      dcc[idx].status = dcc[idx].u.xfer->length;
    } else {
      l = dcc_block;
      dcc[idx].status = dcc[idx].u.xfer->offset + dcc_block;
    }

    /* Seek forward ... */
    fseek(dcc[idx].u.xfer->f, dcc[idx].u.xfer->offset, SEEK_SET);
    dcc[idx].u.xfer->block_pending = pump_file_to_sock(dcc[idx].u.xfer->f,
                                                       dcc[idx].sock, l);
    dcc[idx].u.xfer->type = XFER_RESUME;
  } else {
    dcc[idx].u.xfer->offset = 0;

    /* If we're resending the data, wait for the client's response first,
     * before sending anything ourself.
     */
    if (dcc[idx].u.xfer->type != XFER_RESEND_PEND) {
      if (dcc_block == 0 || dcc[idx].u.xfer->length < dcc_block)
        dcc[idx].status = dcc[idx].u.xfer->length;
      else
        dcc[idx].status = dcc_block;
      dcc[idx].u.xfer->block_pending = pump_file_to_sock(dcc[idx].u.xfer->f,
                                                         dcc[idx].sock,
                                                         dcc[idx].status);
    } else
      dcc[idx].status = 0;
  }

  dcc[idx].timeval = dcc[idx].u.xfer->start_time = now;
}

/* Starts a new DCC SEND or DCC RESEND connection to `nick', transferring
 * `filename' from `dir'.
 *
 * Use raw_dcc_resend() and raw_dcc_send() instead of this function.
 *
 */
static int raw_dcc_resend_send(char *filename, char *nick, char *from,
                               char *dir, int resend)
{
  int zz, port, i;
  char *nfn, *buf = NULL;
  long dccfilesize;
  FILE *f, *dccfile;

  zz = -1;
  dccfile = fopen(filename, "r");
  if (!dccfile)
    return DCCSEND_BADFN;
  fseek(dccfile, 0, SEEK_END);
  dccfilesize = ftell(dccfile);
  fclose(dccfile);

  if (dccfilesize == 0)
    return DCCSEND_FEMPTY;

  if (reserved_port_min > 0 && reserved_port_min < reserved_port_max) {
    for (port = reserved_port_min; port <= reserved_port_max; port++)
      if ((zz = open_listen(&port)) != -1)
        break;
  } else {
    port = reserved_port_min;
    zz = open_listen(&port);
  }

  if (zz == -1)
    return DCCSEND_NOSOCK;

  nfn = strrchr(dir, '/');
  if (nfn == NULL)
    nfn = dir;
  else
    nfn++;

  f = fopen(filename, "r");

  if (!f)
    return DCCSEND_BADFN;

  if ((i = new_dcc(&DCC_GET_PENDING, sizeof(struct xfer_info))) == -1)
    return DCCSEND_FULL;

  dcc[i].sock = zz;
  dcc[i].addr = (IP) (-559026163);
  dcc[i].port = port;
  strcpy(dcc[i].nick, nick);
  strcpy(dcc[i].host, "irc");
  dcc[i].u.xfer->filename = get_data_ptr(strlen(filename) + 1);
  strcpy(dcc[i].u.xfer->filename, filename);
  if (strchr(nfn, ' '))
    nfn = buf = replace_spaces(nfn);
  dcc[i].u.xfer->origname = get_data_ptr(strlen(nfn) + 1);
  strcpy(dcc[i].u.xfer->origname, nfn);
  strncpyz(dcc[i].u.xfer->from, from, NICKLEN);
  strncpyz(dcc[i].u.xfer->dir, dir, DIRLEN);
  dcc[i].u.xfer->length = dccfilesize;
  dcc[i].timeval = now;
  dcc[i].u.xfer->f = f;
  dcc[i].u.xfer->type = resend ? XFER_RESEND_PEND : XFER_SEND;

  if (nick[0] != '*') {
    dprintf(DP_SERVER, "PRIVMSG %s :\001DCC %sSEND %s %lu %d %lu\001\n", nick,
            resend ? "RE" : "", nfn, iptolong(natip[0] ?
            (IP) inet_addr(natip) : getmyip()), port, dccfilesize);
    putlog(LOG_FILES, "*", TRANSFER_BEGIN_DCC, resend ? TRANSFER_RE : "", nfn,
           nick);
  }

  if (buf)
    nfree(buf);

  return DCCSEND_OK;
}

/* Starts a DCC RESEND connection. */
static int raw_dcc_resend(char *filename, char *nick, char *from, char *dir)
{
  return raw_dcc_resend_send(filename, nick, from, dir, 1);
}

/* Starts a DCC_SEND connection. */
static int raw_dcc_send(char *filename, char *nick, char *from, char *dir)
{
  return raw_dcc_resend_send(filename, nick, from, dir, 0);
}

/* This handles DCC RESUME requests.
 */
static int ctcp_DCC_RESUME(char *nick, char *from, char *handle,
                           char *object, char *keyword, char *text)
{
  char *action, *fn, buf[512], *msg = buf;
  int i, port;
  unsigned long offset;

  strcpy(msg, text);
  action = newsplit(&msg);

  if (egg_strcasecmp(action, "RESUME"))
    return 0;

  fn = newsplit(&msg);
  port = atoi(newsplit(&msg));
  offset = my_atoul(newsplit(&msg));

  for (i = 0; i < dcc_total; i++) /* Search for existing SEND */
    if ((dcc[i].type == &DCC_GET_PENDING) &&
        (!rfc_casecmp(dcc[i].nick, nick)) && (dcc[i].port == port))
      break;
  if (i == dcc_total) /* No matching transfer found? */
    return 0;

  if (dcc[i].u.xfer->length <= offset) {
    char *p = strrchr(dcc[i].u.xfer->origname, '/');

    dprintf(DP_HELP, TRANSFER_DCC_IGNORED, nick,
            p ? p + 1 : dcc[i].u.xfer->origname);
    killsock(dcc[i].sock);
    killtransfer(i);
    lostdcc(i);
    return 0;
  }

  dcc[i].u.xfer->type = XFER_RESUME_PEND;
  dcc[i].u.xfer->offset = offset;
  dprintf(DP_SERVER, "PRIVMSG %s :\001DCC ACCEPT %s %d %u\001\n", nick, fn, port,
          offset);
  return 1; /* Now we wait for the client to connect. */
}

static tcl_ints myints[] = {
  {"max-dloads",       &dcc_limit},
  {"dcc-block",        &dcc_block},
  {"xfer-timeout", &wait_dcc_xfer},
  {NULL,                     NULL}
};

static cmd_t transfer_ctcps[] = {
  {"DCC", "",  ctcp_DCC_RESUME, "transfer:DCC"},
  {NULL, NULL, NULL,                      NULL}
};

static cmd_t transfer_load[] = {
  {"server", "", server_transfer_setup, NULL},
  {NULL,     "", NULL,                  NULL}
};

/* Add our CTCP bindings if the server module is loaded. */
static int server_transfer_setup(char *mod)
{
  p_tcl_bind_list H_ctcp;

  if ((H_ctcp = find_bind_table("ctcp")))
    add_builtins(H_ctcp, transfer_ctcps);
  return 1;
}

static char *transfer_close()
{
  int i;
  p_tcl_bind_list H_ctcp;

  putlog(LOG_MISC, "*", TRANSFER_UNLOADING);
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
  del_bind_table(H_lost);
  del_bind_table(H_tout);
  rem_builtins(H_load, transfer_load);
  if ((H_ctcp = find_bind_table("ctcp"))) /* Try to remove our CTCP bindings */
    rem_builtins(H_ctcp, transfer_ctcps);
  rem_tcl_commands(mytcls);
  rem_tcl_ints(myints);
  rem_help_reference("transfer.help");
  del_lang_section("transfer");
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
    int size = transfer_expmem();

    dprintf(idx, TRANSFER_STAT_1, dcc_block,
            (dcc_block == 0) ? " (turbo dcc)" : "");
    dprintf(idx, TRANSFER_STAT_2, dcc_limit);
    dprintf(idx, "    Using %d byte%s of memory\n", size,
            (size != 1) ? "s" : "");
  }
}

EXPORT_SCOPE char *transfer_start();

static Function transfer_table[] = {
  (Function) transfer_start,
  (Function) transfer_close,
  (Function) transfer_expmem,
  (Function) transfer_report,
  /* 4- 7 */
  (Function) & DCC_FORK_SEND,   /* struct dcc_table             */
  (Function) at_limit,
  (Function) NULL,              /* Was copy_to_tmp <Wcc[01/20/03]>. */
  (Function) fileq_cancel,
  /* 8 - 11 */
  (Function) queue_file,
  (Function) raw_dcc_send,
  (Function) show_queued_files,
  (Function) wild_match_file,
  /* 12 - 15 */
  (Function) wipe_tmp_filename,
  (Function) & DCC_GET,         /* struct dcc_table             */
  (Function) & H_rcvd,          /* p_tcl_bind_list              */
  (Function) & H_sent,          /* p_tcl_bind_list              */
  /* 16 - 19 */
  (Function) & USERENTRY_FSTAT, /* struct user_entry_type       */
  (Function) NULL,              /* Was quiet_reject <Wcc[01/20/03]>. */
  (Function) raw_dcc_resend,
  (Function) & H_lost,          /* p_tcl_bind_list              */
  /* 20 - 23 */
  (Function) & H_tout,          /* p_tcl_bind_list              */
  (Function) & DCC_SEND,        /* struct dcc_table             */
  (Function) & DCC_GET_PENDING, /* struct dcc_table             */
};

char *transfer_start(Function *global_funcs)
{
  global = global_funcs;

  fileq = NULL;
  module_register(MODULE_NAME, transfer_table, 2, 3);
  if (!module_depend(MODULE_NAME, "eggdrop", 106, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.6.0 or later.";
  }

  add_tcl_commands(mytcls);
  add_tcl_ints(myints);
  add_builtins(H_load, transfer_load);
  server_transfer_setup(NULL);
  add_help_reference("transfer.help");
  H_rcvd = add_bind_table("rcvd", HT_STACKABLE, builtin_sentrcvd);
  H_sent = add_bind_table("sent", HT_STACKABLE, builtin_sentrcvd);
  H_lost = add_bind_table("lost", HT_STACKABLE, builtin_toutlost);
  H_tout = add_bind_table("tout", HT_STACKABLE, builtin_toutlost);

  USERENTRY_FSTAT.get = def_get;
  add_entry_type(&USERENTRY_FSTAT);
  add_lang_section("transfer");
  return NULL;
}
