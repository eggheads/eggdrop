/* 
 * This file is part of the eggdrop source code copyright (c) 1997 Robey
 * Pointer and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called COPYING
 * that was distributed with this code.
 */

#include <fcntl.h>
#include <sys/stat.h>
#define MAKING_FILESYS
#define MODULE_NAME "filesys"
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include <sys/file.h>
#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif
#include "../module.h"
#include "filesys.h"
#include "../../tandem.h"
#include "files.h"

static p_tcl_bind_list H_fil;
static Function *transfer_funcs = NULL;

/* fcntl.h sets this :/ */
#undef global			/* fcntl.h sets this :/ */
static Function *global = NULL;

/* root dcc directory */
static char dccdir[121];

/* directory to put incoming dcc's into */
static char dccin[121];

/* let all uploads go to the user's current directory? */
static int upload_to_cd = 0;

/* maximum allowable file size for dcc send (1M) */
static int dcc_maxsize = 1024;

/* maximum number of users can be in the file area at once */
static int dcc_users = 0;

/* where to put the filedb, if not in a hidden '.filedb' file in */
/* each directory */
static char filedb_path[121];

static int is_valid();

static void eof_dcc_files(int idx);
static void dcc_files(int idx, char *buf, int i);
static void disp_dcc_files(int idx, char *buf);
static int expmem_dcc_files(void *x);
static void kill_dcc_files(int idx, void *x);
static void out_dcc_files(int idx, char *buf, void *x);

static struct dcc_table DCC_FILES =
{
  "FILES",
  DCT_MASTER | DCT_VALIDIDX | DCT_SHOWWHO | DCT_SIMUL | DCT_CANBOOT | DCT_FILES,
  eof_dcc_files,
  dcc_files,
  0,
  0,
  disp_dcc_files,
  expmem_dcc_files,
  kill_dcc_files,
  out_dcc_files
};

static struct user_entry_type USERENTRY_DCCDIR =
{
  0,				/* always 0 ;) */
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  "DCCDIR"
};

#include "filedb.c"
#include "files.c"
#include "tclfiles.c"

/* check for tcl-bound file command, return 1 if found */
/* fil: proc-name <handle> <dcc-handle> <args...> */
static int check_tcl_fil(char *cmd, int idx, char *args)
{
  int x;
  char s[5];
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN, 0, 0, 0, 0, 0};

  context;
  get_user_flagrec(dcc[idx].user, &fr, dcc[idx].u.file->chat->con_chan);
  sprintf(s, "%ld", dcc[idx].sock);
  Tcl_SetVar(interp, "_fil1", dcc[idx].nick, 0);
  Tcl_SetVar(interp, "_fil2", s, 0);
  Tcl_SetVar(interp, "_fil3", args, 0);
  x = check_tcl_bind(H_fil, cmd, &fr, " $_fil1 $_fil2 $_fil3",
		     MATCH_PARTIAL | BIND_USE_ATTR | BIND_HAS_BUILTINS);
  if (x == BIND_AMBIGUOUS) {
    dprintf(idx, "Ambigious command.\n");
    return 0;
  }
  if (x == BIND_NOMATCH) {
    dprintf(idx, "What?  You need 'help'\n");
    return 0;
  }
  if (x == BIND_EXEC_BRK)
    return 1;
  if (x == BIND_EXEC_LOG)
    putlog(LOG_FILES, "*", "#%s# files: %s %s", dcc[idx].nick, cmd, args);
  return 0;
}

static void dcc_files_pass(int idx, char *buf, int x)
{
  struct userrec *u = get_user_by_handle(userlist, dcc[idx].nick);

  if (!x)
    return;
  if (u_pass_match(u, buf)) {
    if (too_many_filers()) {
      dprintf(idx, "Too many people are in the file system right now.\n");
      dprintf(idx, "Please try again later.\n");
      putlog(LOG_MISC, "*", "File area full: DCC chat [%s]%s", dcc[idx].nick,
	     dcc[idx].host);
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return;
    }
    dcc[idx].type = &DCC_FILES;
    if (dcc[idx].status & STAT_TELNET)
      dprintf(idx, "\377\374\001\n");	/* turn echo back on */
    putlog(LOG_FILES, "*", "File system: [%s]%s/%d", dcc[idx].nick,
	   dcc[idx].host, dcc[idx].port);
    if (!welcome_to_files(idx)) {
      putlog(LOG_FILES, "*", "File system broken.");
      killsock(dcc[idx].sock);
      lostdcc(idx);
    } else {
      struct userrec *u = get_user_by_handle(userlist, dcc[idx].nick);

      touch_laston(u, "filearea", now);
    }
    return;
  }
  dprintf(idx, "Negative on that, Houston.\n");
  putlog(LOG_MISC, "*", "Bad password: DCC chat [%s]%s", dcc[idx].nick,
	 dcc[idx].host);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

/* hash function for file area commands */
static int got_files_cmd(int idx, char *msg)
{
  char *code;

  context;
  strcpy(msg, check_tcl_filt(idx, msg));
  if (!msg[0])
    return 1;
  if (msg[0] == '.')
    msg++;
  code = newsplit(&msg);
  return check_tcl_fil(code, idx, msg);
}

static void dcc_files(int idx, char *buf, int i)
{
  context;
  if (buf[0] &&
      detect_dcc_flood(&dcc[idx].timeval, dcc[idx].u.file->chat, idx))
    return;
  dcc[idx].timeval = now;
  strcpy(buf, check_tcl_filt(idx, buf));
  if (!buf[0])
    return;
  touch_laston(dcc[idx].user, "filearea", now);
  if (buf[0] == ',') {
    for (i = 0; i < dcc_total; i++) {
      if ((dcc[i].type->flags & DCT_MASTER) && dcc[idx].user &&
	  (dcc[idx].user->flags & USER_MASTER) &&
	  ((dcc[i].type == &DCC_FILES) ||
	   (dcc[i].u.chat->channel >= 0)) &&
	  ((i != idx) || (dcc[idx].status & STAT_ECHO)))
	dprintf(i, "-%s- %s\n", dcc[idx].nick, &buf[1]);
    }
  } else if (got_files_cmd(idx, buf)) {
    dprintf(idx, "*** Ja mata!\n");
    flush_lines(idx, dcc[idx].u.file->chat);
    putlog(LOG_FILES, "*", "DCC user [%s]%s left file system", dcc[idx].nick,
	   dcc[idx].host);
    set_user(&USERENTRY_DCCDIR, dcc[idx].user, dcc[idx].u.file->dir);
    if (dcc[idx].status & STAT_CHAT) {
      struct chat_info *ci;

      dprintf(idx, "Returning you to command mode...\n");
      ci = dcc[idx].u.file->chat;
      nfree(dcc[idx].u.file);
      dcc[idx].u.chat = ci;
      dcc[idx].status &= (~STAT_CHAT);
      dcc[idx].type = &DCC_CHAT;
      if (dcc[idx].u.chat->channel >= 0) {
	chanout_but(-1, dcc[idx].u.chat->channel,
		    "*** %s has returned.\n", dcc[idx].nick);
	if (dcc[idx].u.chat->channel < 100000)
	  botnet_send_join_idx(idx, -1);
      }
    } else {
      dprintf(idx, "Dropping connection now.\n");
      putlog(LOG_FILES, "*", "Left files: [%s]%s/%d", dcc[idx].nick,
	     dcc[idx].host, dcc[idx].port);
      killsock(dcc[idx].sock);
      lostdcc(idx);
    }
  }
  if (dcc[idx].status & STAT_PAGE)
    flush_lines(idx, dcc[idx].u.file->chat);
}

static void tell_file_stats(int idx, char *hand)
{
  struct userrec *u;
  struct filesys_stats *fs;
  float fr = (-1.0), kr = (-1.0);

  u = get_user_by_handle(userlist, hand);
  if (u == NULL)
    return;
  if (!(fs = get_user(&USERENTRY_FSTAT, u))) {
    dprintf(idx, "No file statistics for %s.\n", hand);
  } else {
    dprintf(idx, "  uploads: %4u / %6luk\n", fs->uploads, fs->upload_ks);
    dprintf(idx, "downloads: %4u / %6luk\n", fs->dnloads, fs->dnload_ks);
    if (fs->uploads)
      fr = ((float) fs->dnloads / (float) fs->uploads);
    if (fs->upload_ks)
      kr = ((float) fs->dnload_ks / (float) fs->upload_ks);
    if (fr < 0.0)
      dprintf(idx, "(infinite file leech)\n");
    else
      dprintf(idx, "leech ratio (files): %6.2f\n", fr);
    if (kr < 0.0)
      dprintf(idx, "(infinite size leech)\n");
    else
      dprintf(idx, "leech ratio (size) : %6.2f\n", kr);
  }
}

static int cmd_files(struct userrec *u, int idx, char *par)
{
  int atr = u ? u->flags : 0;
  static struct chat_info *ci;

  context;
  if (dccdir[0] == 0)
    dprintf(idx, "There is no file transfer area.\n");
  else if (too_many_filers()) {
    dprintf(idx, "The maximum of %d people are in the file area right now.\n",
	    dcc_users);
    dprintf(idx, "Please try again later.\n");
  } else {
    if (!(atr & (USER_MASTER | USER_XFER)))
      dprintf(idx, "You don't have access to the file area.\n");
    else {
      putlog(LOG_CMDS, "*", "#%s# files", dcc[idx].nick);
      dprintf(idx, "Entering file system...\n");
      if (dcc[idx].u.chat->channel >= 0) {

	chanout_but(-1, dcc[idx].u.chat->channel,
		    "*** %s has left: file system\n",
		    dcc[idx].nick);
	if (dcc[idx].u.chat->channel < 100000)
	  botnet_send_part_idx(idx, "file system");
      }
      ci = dcc[idx].u.chat;
      dcc[idx].u.file = get_data_ptr(sizeof(struct file_info));

      dcc[idx].u.file->chat = ci;
      dcc[idx].type = &DCC_FILES;
      dcc[idx].status |= STAT_CHAT;
      if (!welcome_to_files(idx)) {
	struct chat_info *ci = dcc[idx].u.file->chat;

	nfree(dcc[idx].u.file);
	dcc[idx].u.chat = ci;
	dcc[idx].type = &DCC_CHAT;
	putlog(LOG_FILES, "*", "File system broken.");
	if (dcc[idx].u.chat->channel >= 0) {
	  chanout_but(-1, dcc[idx].u.chat->channel,
		      "*** %s has returned.\n",
		      dcc[idx].nick);
	  if (dcc[idx].u.chat->channel < 100000)
	    botnet_send_join_idx(idx, -1);
	}
      } else
	touch_laston(u, "filearea", now);
    }
  }
  return 0;
}

#define DCCSEND_OK     0
#define DCCSEND_FULL   1	/* dcc table is full */
#define DCCSEND_NOSOCK 2	/* can't open a listening socket */
#define DCCSEND_BADFN  3	/* no such file */

static int _dcc_send(int idx, char *filename, char *nick, char *dir)
{
  int x;
  char *nfn;

  context;
  if (strlen(nick) > HANDLEN)
    nick[HANDLEN] = 0;
  x = raw_dcc_send(filename, nick, dcc[idx].nick, dir);
  if (x == DCCSEND_FULL) {
    dprintf(idx, "Sorry, too many DCC connections.  (try again later)\n");
    putlog(LOG_FILES, "*", "DCC connections full: GET %s [%s]", filename,
	   dcc[idx].nick);
    return 0;
  }
  if (x == DCCSEND_NOSOCK) {
    if (reserved_port) {
      dprintf(idx, "My DCC SEND port is in use.  Try later.\n");
      putlog(LOG_FILES, "*", "DCC port in use (can't open): GET %s [%s]",
	     filename, dcc[idx].nick);
    } else {
      dprintf(idx, "Unable to listen at a socket.\n");
      putlog(LOG_FILES, "*", "DCC socket error: GET %s [%s]", filename,
	     dcc[idx].nick);
    }
    return 0;
  }
  if (x == DCCSEND_BADFN) {
    dprintf(idx, "File not found (???)\n");
    putlog(LOG_FILES, "*", "DCC file not found: GET %s [%s]", filename,
	   dcc[idx].nick);
    return 0;
  }
  nfn = strrchr(filename, '/');
  if (nfn == NULL)
    nfn = filename;
  else
    nfn++;
  if (strcasecmp(nick, dcc[idx].nick))
    dprintf(DP_HELP, "NOTICE %s :Here is a file from %s ...\n", nick, dcc[idx].nick);
  dprintf(idx, "Type '/DCC GET %s %s' to receive.\n", botname, nfn);
  dprintf(idx, "Sending: %s to %s\n", nfn, nick);
  return 1;
}

static int do_dcc_send(int idx, char *dir, char *nick)
{
  char s[161], s1[161], *fn;
  FILE *f;
  int x;

  context;
  /* nickname? */
  fn = newsplit(&nick);
  if (strlen(nick) > NICKMAX)
    nick[NICKMAX] = 0;
  if (dccdir[0] == 0) {
    dprintf(idx, "DCC file transfers not supported.\n");
    putlog(LOG_FILES, "*", "Refused dcc get %s from [%s]", fn, dcc[idx].nick);
    return 0;
  }
  if (strchr(fn, '/') != NULL) {
    dprintf(idx, "Filename cannot have '/' in it...\n");
    putlog(LOG_FILES, "*", "Refused dcc get %s from [%s]", fn, dcc[idx].nick);
    return 0;
  }
  if (dir[0])
    sprintf(s, "%s%s/%s", dccdir, dir, fn);
  else
    sprintf(s, "%s%s", dccdir, fn);
  f = fopen(s, "r");
  if (f == NULL) {
    dprintf(idx, "No such file.\n");
    putlog(LOG_FILES, "*", "Refused dcc get %s from [%s]", fn, dcc[idx].nick);
    return 0;
  }
  fclose(f);
  if (!nick[0])
    nick = dcc[idx].nick;
  /* already have too many transfers active for this user?  queue it */
  if (at_limit(nick)) {
    char xxx[1024];

    sprintf(xxx, "%d*%s%s", strlen(dccdir), dccdir, dir);
    queue_file(xxx, fn, dcc[idx].nick, nick);
    dprintf(idx, "Queued: %s to %s\n", fn, nick);
    return 1;
  }
  if (copy_to_tmp) {
    /* copy this file to /tmp */
    sprintf(s, "%s%s%s%s", dccdir, dir, dir[0] ? "/" : "", fn);
    sprintf(s1, "%s%s", tempdir, fn);
    if (copyfile(s, s1) != 0) {
      dprintf(idx, "Can't make temporary copy of file!\n");
      putlog(LOG_FILES | LOG_MISC, "*",
	     "Refused dcc get %s: copy to %s FAILED!",
	     fn, tempdir);
      return 0;
    }
  } else
    sprintf(s1, "%s%s%s%s", dccdir, dir, dir[0] ? "/" : "", fn);
  sprintf(s, "%s%s%s", dir, dir[0] ? "/" : "", fn);
  x = _dcc_send(idx, s1, nick, s);
  if (x != DCCSEND_OK)
    wipe_tmp_filename(s1, -1);
  return x;
}

static int builtin_fil STDVAR {
  int idx;
  Function F = (Function) cd;

  context;
  BADARGS(4, 4, " hand idx param");
  idx = findanyidx(atoi(argv[2]));
  if ((idx < 0) && (dcc[idx].type != &DCC_FILES)) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (F == CMD_LEAVE) {
    Tcl_AppendResult(irp, "break", NULL);
    return TCL_OK;
  }
  (F) (idx, argv[3]);
  Tcl_ResetResult(irp);
  return TCL_OK;
}

static void tout_dcc_files_pass(int i)
{
  context;
  dprintf(i, "Timeout.\n");
  putlog(LOG_MISC, "*", "Password timeout on dcc chat: [%s]%s", dcc[i].nick,
	 dcc[i].host);
  killsock(dcc[i].sock);
  lostdcc(i);
}

static void disp_dcc_files(int idx, char *buf)
{
  sprintf(buf, "file  flags: %c%c%c%c%c",
	  dcc[idx].status & STAT_CHAT ? 'C' : 'c',
	  dcc[idx].status & STAT_PARTY ? 'P' : 'p',
	  dcc[idx].status & STAT_TELNET ? 'T' : 't',
	  dcc[idx].status & STAT_ECHO ? 'E' : 'e',
	  dcc[idx].status & STAT_PAGE ? 'P' : 'p');
}

static void disp_dcc_files_pass(int idx, char *buf)
{
  sprintf(buf, "fpas  waited %lus", now - dcc[idx].timeval);
}

static void kill_dcc_files(int idx, void *x)
{
  register struct file_info *f = (struct file_info *) x;

  if (f->chat)
    DCC_CHAT.kill(idx, f->chat);
  nfree(x);
}

static void eof_dcc_files(int idx)
{
  dcc[idx].u.file->chat->con_flags = 0;
  putlog(LOG_MISC, "*", "Lost dcc connection to %s (%s/%d)", dcc[idx].nick,
	 dcc[idx].host, dcc[idx].port);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static int expmem_dcc_files(void *x)
{
  register struct file_info *p = (struct file_info *) x;
  int tot = sizeof(struct file_info);

  if (p->chat)
    tot += DCC_CHAT.expmem(p->chat);
  return tot;
}

static void out_dcc_files(int idx, char *buf, void *x)
{
  register struct file_info *p = (struct file_info *) x;

  if (p->chat)
    DCC_CHAT.output(idx, buf, p->chat);
  else
    tputs(dcc[idx].sock, buf, strlen(buf));
}

static cmd_t mydcc[] =
{
  {"files", "-", cmd_files, NULL},
  {0, 0, 0, 0}
};

static tcl_strings mystrings[] =
{
  {"files-path", dccdir, 120, STR_DIR | STR_PROTECT},
  {"incoming-path", dccin, 120, STR_DIR | STR_PROTECT},
  {"filedb-path", filedb_path, 120, STR_DIR | STR_PROTECT},
  {NULL, NULL, 0, 0}
};

static tcl_ints myints[] =
{
  {"max-filesize", &dcc_maxsize},
  {"max-file-users", &dcc_users},
  {"upload-to-pwd", &upload_to_cd},
  {NULL, NULL}
};

static struct dcc_table DCC_FILES_PASS =
{
  "FILES_PASS",
  0,
  eof_dcc_files,
  dcc_files_pass,
  0,
  tout_dcc_files_pass,
  disp_dcc_files_pass,
  expmem_dcc_files,
  kill_dcc_files,
  out_dcc_files
};

/* received a ctcp-dcc */
static void filesys_dcc_send(char *nick, char *from, struct userrec *u,
			     char *text)
{
  char *param, *ip, *prt, buf[512], s1[512], *msg = buf;
  FILE *f;
  int atr = u ? u->flags : 0, i, j;

  context;
  strcpy(buf, text);
  param = newsplit(&msg);
  if (!(atr & USER_XFER)) {
    putlog(LOG_FILES, "*",
	   "Refused DCC SEND %s (no access): %s!%s", param,
	   nick, from);
  } else if (!dccin[0] && !upload_to_cd) {
    dprintf(DP_HELP,
	    "NOTICE %s :DCC file transfers not supported.\n", nick);
    putlog(LOG_FILES, "*",
	   "Refused dcc send %s from %s!%s", param, nick, from);
  } else if (strchr(param, '/')) {
    dprintf(DP_HELP,
	    "NOTICE %s :Filename cannot have '/' in it...\n", nick);
    putlog(LOG_FILES, "*",
	   "Refused dcc send %s from %s!%s", param, nick, from);
  } else {
    ip = newsplit(&msg);
    prt = newsplit(&msg);
    if ((atoi(prt) < min_dcc_port) || (atoi(prt) > max_dcc_port)) {
      /* invalid port range, do clients even use over 5000?? */
      dprintf(DP_HELP, "NOTICE %s :%s (invalid port)\n", nick,
	      DCC_CONNECTFAILED1);
      putlog(LOG_FILES, "*", "Refused dcc send %s (%s): invalid port", param,
	     nick);
    } else if (atoi(msg) == 0) {
      dprintf(DP_HELP,
	      "NOTICE %s :Sorry, file size info must be included.\n",
	      nick);
      putlog(LOG_FILES, "*", "Refused dcc send %s (%s): no file size",
	     param, nick);
    } else if (atoi(msg) > (dcc_maxsize * 1024)) {
      dprintf(DP_HELP, "NOTICE %s :Sorry, file too large.\n", nick);
      putlog(LOG_FILES, "*",
	     "Refused dcc send %s (%s): file too large", param,
	     nick);
    } else {
      /* This looks like a good place for a sanity check. */
      if (!sanitycheck_dcc(nick, from, ip, prt))
	return;
      i = new_dcc(&DCC_FORK_SEND, sizeof(struct xfer_info));

      if (i < 0) {
	dprintf(DP_HELP, "NOTICE %s :Sorry, too many DCC connections.\n",
		nick);
	putlog(LOG_MISC, "*", "DCC connections full: SEND %s (%s!%s)",
	       param, nick, from);
      }
      dcc[i].addr = my_atoul(ip);
      dcc[i].port = atoi(prt);
      dcc[i].sock = -1;
      strcpy(dcc[i].nick, nick);
      strcpy(dcc[i].host, from);
      if (param[0] == '.')
	param[0] = '_';
      strncpy(dcc[i].u.xfer->filename, param, 120);
      dcc[i].u.xfer->filename[120] = 0;
      if (upload_to_cd) {
	char *p = get_user(&USERENTRY_DCCDIR, u);

	if (p)
	  sprintf(dcc[i].u.xfer->dir, "%s%s/", dccdir, p);
	else
	  sprintf(dcc[i].u.xfer->dir, "%s", dccdir);
      } else
	strcpy(dcc[i].u.xfer->dir, dccin);
      dcc[i].u.xfer->length = atoi(msg);
      sprintf(s1, "%s%s", dcc[i].u.xfer->dir, param);
      context;      
      f = fopen(s1, "r");
      if (f) {
	fclose(f);
	dprintf(DP_HELP, "NOTICE %s :That file already exists.\n", nick);
	lostdcc(i);
      } else {
	/* check for dcc-sends in process with the same filename */
	for (j = 0; j < dcc_total; j++)
	  if (j != i) {
	    if ((dcc[j].type->flags & (DCT_FILETRAN | DCT_FILESEND))
		== (DCT_FILETRAN | DCT_FILESEND)) {
	      if (!strcmp(param, dcc[j].u.xfer->filename)) {
		dprintf(DP_HELP,
		  "NOTICE %s :That file is already being sent.\n", nick);
		lostdcc(i);
		return;
	      }
	    }
	  }
	/* put uploads in /tmp first */
	sprintf(s1, "%s%s", tempdir, param);
	dcc[i].u.xfer->f = fopen(s1, "w");
	if (dcc[i].u.xfer->f == NULL) {
	  dprintf(DP_HELP,
		  "NOTICE %s :Can't create that file (temp dir error)\n",
		  nick);
	  lostdcc(i);
	} else {
	  dcc[i].timeval = now;
	  dcc[i].sock = getsock(SOCK_BINARY);
	  if (open_telnet_dcc(dcc[i].sock, ip, prt) < 0) {
	    dcc[i].type->eof(i);
	  }
	}
      }
    }
  }
}

/* this only handles CHAT requests, otherwise it's handled in filesys */
static int filesys_DCC_CHAT(char *nick, char *from, char *handle,
			    char *object, char *keyword, char *text)
{
  char *param, *ip, *prt, buf[512], *msg = buf;
  int i, sock;
  struct userrec *u = get_user_by_handle(userlist, handle);
  struct flag_record fr =
  {FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0};

  context;
  if (!strncasecmp(text, "SEND ", 5)) {
    filesys_dcc_send(nick, from, u, text + 5);
    return 1;
  }
  if (strncasecmp(text, "CHAT ", 5) || !u)
    return 0;
  strcpy(buf, text + 5);
  get_user_flagrec(u, &fr, 0);
  param = newsplit(&msg);
  if (dcc_total == max_dcc) {
    putlog(LOG_MISC, "*", DCC_TOOMANYDCCS2, "CHAT(file)", param, nick, from);
  } else if (glob_party(fr) || (!require_p && chan_op(fr)))
    return 0;			/* allow ctcp.so to pick up the chat */
  else if (!glob_xfer(fr)) {
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :.\n", nick, DCC_REFUSED3);
    putlog(LOG_MISC, "*", "%s: %s!%s", DCC_REFUSED, nick, from);
  } else if (u_pass_match(u, "-")) {
    if (!quiet_reject)
      dprintf(DP_HELP, "NOTICE %s :%s.\n", nick, DCC_REFUSED3);
    putlog(LOG_MISC, "*", "%s: %s!%s", DCC_REFUSED4, nick, from);
  } else if (!dccdir[0]) {
    putlog(LOG_MISC, "*", "%s: %s!%s", DCC_REFUSED5, nick, from);
  } else {
    ip = newsplit(&msg);
    prt = newsplit(&msg);
    sock = getsock(0);
    if (open_telnet_dcc(sock, ip, prt) < 0) {
      neterror(buf);
      if (!quiet_reject)
        dprintf(DP_HELP, "NOTICE %s :%s (%s)\n", nick,
	        DCC_CONNECTFAILED1, buf);
      putlog(LOG_MISC, "*", "%s: CHAT(file) (%s!%s)", DCC_CONNECTFAILED2,
	     nick, from);
      putlog(LOG_MISC, "*", "    (%s)", buf);
      killsock(sock);
    } else if ((atoi(prt) < min_dcc_port) || (atoi(prt) > max_dcc_port)) {
      /* invalid port range, do clients even use over 5000?? */
      if (!quiet_reject)
        dprintf(DP_HELP, "NOTICE %s :%s (invalid port)\n", nick,
	        DCC_CONNECTFAILED1);
      putlog(LOG_FILES, "*", "%s: %s!%s", DCC_REFUSED7, nick, from);

    } else {
      i = new_dcc(&DCC_FILES_PASS, sizeof(struct file_info));

      dcc[i].addr = my_atoul(ip);
      dcc[i].port = atoi(prt);
      dcc[i].sock = sock;
      strcpy(dcc[i].nick, u->handle);
      strcpy(dcc[i].host, from);
      dcc[i].status = STAT_ECHO;
      dcc[i].timeval = now;
      dcc[i].u.file->chat = get_data_ptr(sizeof(struct chat_info));
      bzero(dcc[i].u.file->chat, sizeof(struct chat_info));

      strcpy(dcc[i].u.file->chat->con_chan, "*");
      dcc[i].user = u;
      putlog(LOG_MISC, "*", "DCC connection: CHAT(file) (%s!%s)", nick, from);
      dprintf(i, "%s\n", DCC_ENTERPASS);
    }
  }
  return 1;
}

static cmd_t myctcp[] =
{
  {"DCC", "", filesys_DCC_CHAT, "files:DCC"},
  {0, 0, 0, 0}
};

static void init_server_ctcps(char *module)
{
  p_tcl_bind_list H_ctcp;

  if ((H_ctcp = find_bind_table("ctcp")))
    add_builtins(H_ctcp, myctcp);
}

static cmd_t myload[] =
{
  {"server", "", (Function) init_server_ctcps, "filesys:server"},
  {0, 0, 0, 0}
};

static int filesys_expmem()
{
  return 0;
}

static void filesys_report(int idx, int details)
{
  if (dccdir[0]) {
    dprintf(idx, "    DCC file path: %s", dccdir);
    if (upload_to_cd)
      dprintf(idx, "\n        incoming: (go to the current dir)\n");
    else if (dccin[0])
      dprintf(idx, "\n        incoming: %s\n", dccin);
    else
      dprintf(idx, "    (no uploads)\n");
    if (dcc_users)
      dprintf(idx, "        max users is %d\n", dcc_users);
    if ((upload_to_cd) || (dccin[0]))
      dprintf(idx, "    DCC max file size: %dk\n", dcc_maxsize);
  } else
    dprintf(idx, "  (Filesystem module loaded, but no active dcc path.)\n");
}

static char *filesys_close()
{
  int i;
  p_tcl_bind_list H_ctcp;

  context;
  putlog(LOG_MISC, "*", "Unloading filesystem, killing all filesystem connections..");
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_FILES) {
      dprintf(i, DCC_BOOTED1);
      dprintf(i,
	 "You have been booted from the filesystem, module unloaded.\n");
      killsock(dcc[i].sock);
      lostdcc(i);
    } else if (dcc[i].type == &DCC_FILES_PASS) {
      killsock(dcc[i].sock);
      lostdcc(i);
    }
  rem_tcl_commands(mytcls);
  rem_tcl_strings(mystrings);
  rem_tcl_ints(myints);
  rem_builtins(H_dcc, mydcc);
  rem_builtins(H_load, myload);
  rem_builtins(H_fil, myfiles);
  rem_help_reference("filesys.help");
  if ((H_ctcp = find_bind_table("ctcp")))
    rem_builtins(H_ctcp, myctcp);
  del_bind_table(H_fil);
  del_entry_type(&USERENTRY_DCCDIR);
  module_undepend(MODULE_NAME);
  return NULL;
}

char *filesys_start();

static Function filesys_table[] =
{
  /* 0 - 3 */
  (Function) filesys_start,
  (Function) filesys_close,
  (Function) filesys_expmem,
  (Function) filesys_report,
  /* 4 - 7 */
  (Function) remote_filereq,
  (Function) add_file,
  (Function) incr_file_gots,
  (Function) is_valid,
  /* 8 - 11 */
  (Function) & H_fil,
};

char *filesys_start(Function * global_funcs)
{
  global = global_funcs;

  context;
  dccdir[0] = 0;
  dccin[0] = 0;
  filedb_path[0] = 0;
  module_register(MODULE_NAME, filesys_table, 2, 0);
  if (!(transfer_funcs = module_depend(MODULE_NAME, "transfer", 2, 0)))
    return "You need the transfer module to user the file system.";
  if (!module_depend(MODULE_NAME, "eggdrop", 103, 15))
    return "You need at least eggdrop1.3.15 to run this module.";
  add_tcl_commands(mytcls);
  add_tcl_strings(mystrings);
  add_tcl_ints(myints);
  H_fil = add_bind_table("fil", 0, builtin_fil);
  add_builtins(H_dcc, mydcc);
  add_builtins(H_fil, myfiles);
  add_builtins(H_load, myload);
  add_help_reference("filesys.help");
  init_server_ctcps(0);
  my_memcpy(&USERENTRY_DCCDIR, &USERENTRY_INFO,
	    sizeof(struct user_entry_type) - sizeof(char *));

  USERENTRY_DCCDIR.got_share = 0;	/* we dont want it shared tho */
  add_entry_type(&USERENTRY_DCCDIR);
  DCC_FILES_PASS.timeout_val = &password_timeout;
  add_lang_section("filesys");
  return NULL;
}

static int is_valid()
{
  return dccdir[0];
}

/* 2 stupid backward compatability functions */
/* set upload/dnload stats for a user */
