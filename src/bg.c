/* 
 * bg.c -- handles:
 *   moving the process to the background, i.e. forking, while keeping threads
 *   happy.
 * 
 * $Id: bg.c,v 1.1 2000/09/27 19:48:54 fabian Exp $
 */
/* 
 * Copyright (C) 1997  Robey Pointer
 * Copyright (C) 1999, 2000  Eggheads
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
#include <signal.h>
#include "bg.h"

#ifdef HAVE_TCL_THREADS
#  define SUPPORT_THREADS
#endif

extern char	pid_file[];

#ifdef SUPPORT_THREADS

/* When threads are started during eggdrop's init phase, we can't simply
 * fork() later on, because that only copies the VM space over and
 * doesn't actually duplicate the threads.
 *
 * To work around this, we fork() very early and let the parent process
 * wait in an event loop. As soon as the init phase is completed and we
 * would normally move to the background, the child process simply
 * messages it's parent that it may now quit. This allows us to control
 * the terminal long enough to, e.g. properly feed error messages to
 * cron scripts and let the user abort the loading process by hitting
 * CTRL+C.
 *
 *
 * [ Parent process                  ] [ Child process                   ]
 *
 *   main()
 *     ...
 *     bg_prepare_split()
 *       fork()                              - created -
 *       waiting in event loop.            continuing execution in main()
 *                                         ...
 *                                         completed init.
 *                                         bg_do_detach()
 *                                           message parent new PID file-
 *                                            name.
 *       receives new PID filename
 *         message.
 *                                           message parent to quit.
 *       receives quit message.            continues to main loop.
 *       writes PID to file.               ...
 *       exits.
 */

/* Messages sent from the newly forked process to the original process,
 * connected to the terminal.
 */
#  define BG_MSG_QUIT		'\100'	/* Quit original process. Write
				 	   PID file, etc. i.e. detach.	 */
#  define BG_MSG_ABORT		'\101'	/* Quit original process.	 */
#  define BG_MSG_TRANSFERPF	'\102'	/* Sending pid_file.		 */

typedef enum {
	BG_NONE = 0,			/* No forking has taken place
					   yet.				 */
	BG_SPLIT,			/* I'm the newly forked process. */
	BG_PARENT			/* I'm the original process.	 */
} bg_state_t;

typedef struct {
	int		com[2];		/* Pair of fd's used to
					   communicate between parent
					   and split process.		 */
	bg_state_t	state;		/* Current state, see above
					   enum descriptions.		 */
	pid_t		child_pid;	/* PID of split process.	 */
} bg_t;

static bg_t	bg = { { 0 }, 0 };

#endif /* SUPPORT_THREADS */


/* Do everything we normally do after we have split off a new
 * process to the background. This includes writing a PID file
 * and informing the user of the split.
 */
static void bg_do_detach(pid_t p)
{
    FILE *fp;

    /* Need to attempt to write pid now, not later. */
    unlink(pid_file);
    fp = fopen(pid_file, "w");
    if (fp != NULL) {
      fprintf(fp, "%u\n", p);
      if (fflush(fp)) {
	/* Kill bot incase a botchk is run from crond. */
	printf(EGG_NOWRITE, pid_file);
	printf("  Try freeing some disk space\n");
	fclose(fp);
	unlink(pid_file);
	exit(1);
      }
    fclose(fp);
    } else
      printf(EGG_NOWRITE, pid_file);
    printf("Launched into the background  (pid: %d)\n\n", p);
#if HAVE_SETPGID
    setpgid(p, p);
#endif
    exit(0);
}

void bg_prepare_split(void)
{
#ifdef SUPPORT_THREADS
  /* Create a pipe between parent and split process, fork to create a
   * parent and a split process and wait for messages on the pipe.
   */
  pid_t	p;
  char	com_buf;

  if (pipe(bg.com) < 0)
    fatal("CANNOT OPEN PIPE.", 0);

  p = fork();
  if (p == -1)
    fatal("CANNOT FORK PROCESS.", 0);
  if (p == 0) {
    bg.state = BG_SPLIT;
    return;
  } else {
    bg.child_pid = p;
    bg.state = BG_PARENT;
  }

  while (read(bg.com[0], &com_buf, 1) > 0) {
    switch (com_buf) {
    case BG_MSG_QUIT:
      bg_do_detach(p);
      break;
    case BG_MSG_ABORT:
      exit(1);
      break;
    case BG_MSG_TRANSFERPF:
      /* Now transferring file from split process.
       */
      /* First message is the number of bytes to be received. */
      if (read(bg.com[0], &com_buf, 1) <= 0)
	goto error;
      if (com_buf >= 40)
	com_buf = 40 - 1;
      /* Second message contains data. */
      if (read(bg.com[0], pid_file, (size_t) com_buf) <= 0)
	goto error;
      pid_file[(int) com_buf] = 0;
      break;
    }
  }

error:
  /* We only reach this point in case of an error.
   */
  fatal("COMMUNICATION THROUGH PIPE BROKE.", 0);
#endif
}

#ifdef SUPPORT_THREADS
/* Transfer contents of pid_file to parent process. This is necessary,
 * as the pid_file[] buffer has changed in this fork by now, but the
 * parent needs an up-to-date version.
 */
static void bg_send_pidfile(void)
{
    char com_buf = BG_MSG_TRANSFERPF;

    /* Send type message. */
    if (write(bg.com[1], &com_buf, 1) < 0)
      goto error;
    /* Send length of data. */
    com_buf = strlen(pid_file);
    if (write(bg.com[1], &com_buf, 1) < 0)
      goto error;
    /* Send data. */
    if (write(bg.com[1], pid_file, (size_t) com_buf) < 0)
      goto error;
    return;
error:
    fatal("COMMUNICATION THROUGH PIPE BROKE.", 0);
}
#endif

void bg_send_quit(bg_quit_t q)
{
#ifdef SUPPORT_THREADS
  if (bg.state == BG_PARENT) {
    kill(bg.child_pid, SIGKILL);
  } else if (bg.state == BG_SPLIT) {
    char com_buf;

    if (q == BG_QUIT) {
      bg_send_pidfile();
      com_buf = BG_MSG_QUIT;
    } else
      com_buf = BG_MSG_ABORT;
    /* Send message. */
    if (write(bg.com[1], &com_buf, 1) < 0)
      fatal("COMMUNICATION THROUGH PIPE BROKE.", 0);
  }
#endif
}

void bg_do_split(void)
{
#ifdef SUPPORT_THREADS
  /* Tell our parent process to go away now, as we don't need it
   * anymore.
   */
  bg_send_quit(BG_QUIT);
#else
  /* Split off a new process.
   */
  int	xx = fork();

  if (xx == -1)
    fatal("CANNOT FORK PROCESS.", 0);
  if (xx != 0)
    bg_do_detach(xx);
#endif
}
