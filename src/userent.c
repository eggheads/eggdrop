/*
 * userent.c -- handles:
 *   user-entry handling, new style more versatile.
 *
 * $Id: userent.c,v 1.41 2011/02/13 14:19:33 simple Exp $
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
#include "users.h"

extern int noshare;
extern struct userrec *userlist;
extern struct dcc_t *dcc;
extern Tcl_Interp *interp;
extern char whois_fields[];


int share_greet = 0;            /* Share greeting info                  */
static struct user_entry_type *entry_type_list;


void init_userent()
{
  entry_type_list = 0;
  add_entry_type(&USERENTRY_COMMENT);
  add_entry_type(&USERENTRY_XTRA);
  add_entry_type(&USERENTRY_INFO);
  add_entry_type(&USERENTRY_LASTON);
  add_entry_type(&USERENTRY_BOTADDR);
  add_entry_type(&USERENTRY_PASS);
  add_entry_type(&USERENTRY_HOSTS);
  add_entry_type(&USERENTRY_BOTFL);
}

void list_type_kill(struct list_type *t)
{
  struct list_type *u;

  while (t) {
    u = t->next;
    if (t->extra)
      nfree(t->extra);
    nfree(t);
    t = u;
  }
}

int list_type_expmem(struct list_type *t)
{
  int tot = 0;

  for (; t; t = t->next)
    tot += sizeof(struct list_type) + strlen(t->extra) + 1;

  return tot;
}

int def_unpack(struct userrec *u, struct user_entry *e)
{
  char *tmp;

  tmp = e->u.list->extra;
  e->u.list->extra = NULL;
  list_type_kill(e->u.list);
  e->u.string = tmp;
  return 1;
}

int def_pack(struct userrec *u, struct user_entry *e)
{
  char *tmp;

  tmp = e->u.string;
  e->u.list = user_malloc(sizeof(struct list_type));
  e->u.list->next = NULL;
  e->u.list->extra = tmp;
  return 1;
}

int def_kill(struct user_entry *e)
{
  nfree(e->u.string);
  nfree(e);
  return 1;
}

int def_write_userfile(FILE *f, struct userrec *u, struct user_entry *e)
{
  if (fprintf(f, "--%s %s\n", e->type->name, e->u.string) == EOF)
    return 0;
  return 1;
}

void *def_get(struct userrec *u, struct user_entry *e)
{
  return e->u.string;
}

int def_set(struct userrec *u, struct user_entry *e, void *buf)
{
  char *string = (char *) buf;

  if (string && !string[0])
    string = NULL;
  if (!string && !e->u.string)
    return 1;
  if (string) {
    int l = strlen(string);
    char *i;

    if (l > 160)
      l = 160;


    e->u.string = user_realloc(e->u.string, l + 1);

    strncpyz(e->u.string, string, l + 1);

    for (i = e->u.string; *i; i++)
      /* Allow bold, inverse, underline, color text here...
       * But never add cr or lf!! --rtc
       */
      if ((unsigned int) *i < 32 && !strchr("\002\003\026\037", *i))
        *i = '?';
  } else {
    nfree(e->u.string);
    e->u.string = NULL;
  }
  if (!noshare && !(u->flags & (USER_BOT | USER_UNSHARED))) {
    if (e->type != &USERENTRY_INFO || share_greet)
      shareout(NULL, "c %s %s %s\n", e->type->name, u->handle,
               e->u.string ? e->u.string : "");
  }
  return 1;
}

int def_gotshare(struct userrec *u, struct user_entry *e, char *data, int idx)
{
  putlog(LOG_CMDS, "*", "%s: change %s %s", dcc[idx].nick, e->type->name,
         u->handle);
  return e->type->set(u, e, data);
}

int def_tcl_get(Tcl_Interp * interp, struct userrec *u,
                struct user_entry *e, int argc, char **argv)
{
  Tcl_AppendResult(interp, e->u.string, NULL);
  return TCL_OK;
}

int def_tcl_set(Tcl_Interp * irp, struct userrec *u,
                struct user_entry *e, int argc, char **argv)
{
  BADARGS(4, 4, " handle type setting");

  e->type->set(u, e, argv[3]);
  return TCL_OK;
}

int def_expmem(struct user_entry *e)
{
  return strlen(e->u.string) + 1;
}

void def_display(int idx, struct user_entry *e)
{
  dprintf(idx, "  %s: %s\n", e->type->name, e->u.string);
}

int def_dupuser(struct userrec *new, struct userrec *old, struct user_entry *e)
{
  return set_user(e->type, new, e->u.string);
}

static void comment_display(int idx, struct user_entry *e)
{
  if (dcc[idx].user && (dcc[idx].user->flags & USER_MASTER))
    dprintf(idx, "  COMMENT: %.70s\n", e->u.string);
}

struct user_entry_type USERENTRY_COMMENT = {
  0,                            /* always 0 ;) */
  def_gotshare,
  def_dupuser,
  def_unpack,
  def_pack,
  def_write_userfile,
  def_kill,
  def_get,
  def_set,
  def_tcl_get,
  def_tcl_set,
  def_expmem,
  comment_display,
  "COMMENT"
};

struct user_entry_type USERENTRY_INFO = {
  0,                            /* always 0 ;) */
  def_gotshare,
  def_dupuser,
  def_unpack,
  def_pack,
  def_write_userfile,
  def_kill,
  def_get,
  def_set,
  def_tcl_get,
  def_tcl_set,
  def_expmem,
  def_display,
  "INFO"
};

int pass_set(struct userrec *u, struct user_entry *e, void *buf)
{
  char new[32];
  register char *pass = buf;

  if (e->u.extra)
    nfree(e->u.extra);
  if (!pass || !pass[0] || (pass[0] == '-'))
    e->u.extra = NULL;
  else {
    unsigned char *p = (unsigned char *) pass;

    if (strlen(pass) > 30)
      pass[30] = 0;
    while (*p) {
      if ((*p <= 32) || (*p == 127))
        *p = '?';
      p++;
    }
    if ((u->flags & USER_BOT) || (pass[0] == '+'))
      strcpy(new, pass);
    else
      encrypt_pass(pass, new);
    e->u.extra = user_malloc(strlen(new) + 1);
    strcpy(e->u.extra, new);
  }
  if (!noshare && !(u->flags & (USER_BOT | USER_UNSHARED)))
    shareout(NULL, "c PASS %s %s\n", u->handle, pass ? pass : "");
  return 1;
}

static int pass_tcl_set(Tcl_Interp * irp, struct userrec *u,
                        struct user_entry *e, int argc, char **argv)
{
  BADARGS(3, 4, " handle PASS ?newpass?");

  pass_set(u, e, argc == 3 ? NULL : argv[3]);
  return TCL_OK;
}

struct user_entry_type USERENTRY_PASS = {
  0,
  def_gotshare,
  0,
  def_unpack,
  def_pack,
  def_write_userfile,
  def_kill,
  def_get,
  pass_set,
  def_tcl_get,
  pass_tcl_set,
  def_expmem,
  0,
  "PASS"
};

static int laston_unpack(struct userrec *u, struct user_entry *e)
{
  char *par, *arg;
  struct laston_info *li;

  par = e->u.list->extra;
  arg = newsplit(&par);
  if (!par[0])
    par = "???";
  li = user_malloc(sizeof(struct laston_info));
  li->lastonplace = user_malloc(strlen(par) + 1);
  li->laston = atoi(arg);
  strcpy(li->lastonplace, par);
  list_type_kill(e->u.list);
  e->u.extra = li;
  return 1;
}

static int laston_pack(struct userrec *u, struct user_entry *e)
{
  char work[1024];
  long tv;
  struct laston_info *li;
  int l;

  li = (struct laston_info *) e->u.extra;
  tv = li->laston;
  l = sprintf(work, "%lu %s", tv, li->lastonplace);
  e->u.list = user_malloc(sizeof(struct list_type));
  e->u.list->next = NULL;
  e->u.list->extra = user_malloc(l + 1);
  strcpy(e->u.list->extra, work);
  nfree(li->lastonplace);
  nfree(li);
  return 1;
}

static int laston_write_userfile(FILE *f, struct userrec *u,
                                 struct user_entry *e)
{
  long tv;
  struct laston_info *li = (struct laston_info *) e->u.extra;

  tv = li->laston;
  if (fprintf(f, "--LASTON %lu %s\n", tv,
              li->lastonplace ? li->lastonplace : "") == EOF)
    return 0;
  return 1;
}

static int laston_kill(struct user_entry *e)
{
  if (((struct laston_info *) (e->u.extra))->lastonplace)
    nfree(((struct laston_info *) (e->u.extra))->lastonplace);
  nfree(e->u.extra);
  nfree(e);
  return 1;
}

static int laston_set(struct userrec *u, struct user_entry *e, void *buf)
{
  struct laston_info *li = (struct laston_info *) e->u.extra;

  if (li != buf) {
    if (li) {
      nfree(li->lastonplace);
      nfree(li);
    }

    li = e->u.extra = buf;
  }
  /* donut share laston info */
  return 1;
}

static int laston_tcl_get(Tcl_Interp * irp, struct userrec *u,
                          struct user_entry *e, int argc, char **argv)
{
  struct laston_info *li = (struct laston_info *) e->u.extra;
  char number[20];
  long tv;
  struct chanuserrec *cr;

  BADARGS(3, 4, " handle LASTON ?channel?");

  if (argc == 4) {
    for (cr = u->chanrec; cr; cr = cr->next)
      if (!rfc_casecmp(cr->channel, argv[3])) {
        Tcl_AppendResult(irp, int_to_base10(cr->laston), NULL);
        break;
      }
    if (!cr)
      Tcl_AppendResult(irp, "0", NULL);
  } else {
    tv = li->laston;
    sprintf(number, "%lu ", tv);
    Tcl_AppendResult(irp, number, li->lastonplace, NULL);
  }
  return TCL_OK;
}

static int laston_tcl_set(Tcl_Interp * irp, struct userrec *u,
                          struct user_entry *e, int argc, char **argv)
{
  struct laston_info *li;
  struct chanuserrec *cr;

  BADARGS(4, 5, " handle LASTON time ?place?");

  if ((argc == 5) && argv[4][0] && strchr(CHANMETA, argv[4][0])) {
    /* Search for matching channel */
    for (cr = u->chanrec; cr; cr = cr->next)
      if (!rfc_casecmp(cr->channel, argv[4])) {
        cr->laston = atoi(argv[3]);
        break;
      }
  }
  /* Save globally */
  li = user_malloc(sizeof(struct laston_info));

  if (argc == 5) {
    li->lastonplace = user_malloc(strlen(argv[4]) + 1);
    strcpy(li->lastonplace, argv[4]);
  } else {
    li->lastonplace = user_malloc(1);
    li->lastonplace[0] = 0;
  }
  li->laston = atoi(argv[3]);
  set_user(&USERENTRY_LASTON, u, li);
  return TCL_OK;
}

static int laston_expmem(struct user_entry *e)
{
  return sizeof(struct laston_info) +
    strlen(((struct laston_info *) (e->u.extra))->lastonplace) + 1;
}

static int laston_dupuser(struct userrec *new, struct userrec *old,
                          struct user_entry *e)
{
  struct laston_info *li = e->u.extra, *li2;

  if (li) {
    li2 = user_malloc(sizeof(struct laston_info));

    li2->laston = li->laston;
    li2->lastonplace = user_malloc(strlen(li->lastonplace) + 1);
    strcpy(li2->lastonplace, li->lastonplace);
    return set_user(&USERENTRY_LASTON, new, li2);
  }
  return 0;
}

struct user_entry_type USERENTRY_LASTON = {
  0,                            /* always 0 ;) */
  0,
  laston_dupuser,
  laston_unpack,
  laston_pack,
  laston_write_userfile,
  laston_kill,
  def_get,
  laston_set,
  laston_tcl_get,
  laston_tcl_set,
  laston_expmem,
  0,
  "LASTON"
};

static int botaddr_unpack(struct userrec *u, struct user_entry *e)
{
  char *p, *q;
  struct bot_addr *bi = user_malloc(sizeof(struct bot_addr));

  egg_bzero(bi, sizeof(struct bot_addr));

  if (!(q = strchr((p = e->u.list->extra), ':'))) {
    bi->address = user_malloc(strlen(p) + 1);
    strcpy(bi->address, p);
  } else {
    bi->address = user_malloc((q - p) + 1);
    strncpy(bi->address, p, q - p);
    bi->address[q - p] = 0;
    q++;
    bi->telnet_port = atoi(q);
    if ((q = strchr(q, '/')))
      bi->relay_port = atoi(q + 1);
  }
  if (!bi->telnet_port)
    bi->telnet_port = 3333;
  if (!bi->relay_port)
    bi->relay_port = bi->telnet_port;
  list_type_kill(e->u.list);
  e->u.extra = bi;
  return 1;
}

static int botaddr_pack(struct userrec *u, struct user_entry *e)
{
  char work[1024];
  struct bot_addr *bi;
  int l;

  bi = (struct bot_addr *) e->u.extra;
  l = simple_sprintf(work, "%s:%u/%u", bi->address, bi->telnet_port,
                     bi->relay_port);
  e->u.list = user_malloc(sizeof(struct list_type));
  e->u.list->next = NULL;
  e->u.list->extra = user_malloc(l + 1);
  strcpy(e->u.list->extra, work);
  nfree(bi->address);
  nfree(bi);
  return 1;
}

static int botaddr_kill(struct user_entry *e)
{
  nfree(((struct bot_addr *) (e->u.extra))->address);
  nfree(e->u.extra);
  nfree(e);
  return 1;
}

static int botaddr_write_userfile(FILE *f, struct userrec *u,
                                  struct user_entry *e)
{
  register struct bot_addr *bi = (struct bot_addr *) e->u.extra;

  if (fprintf(f, "--%s %s:%u/%u\n", e->type->name, bi->address,
              bi->telnet_port, bi->relay_port) == EOF)
    return 0;
  return 1;
}

static int botaddr_set(struct userrec *u, struct user_entry *e, void *buf)
{
  register struct bot_addr *bi = (struct bot_addr *) e->u.extra;

  if (!bi && !buf)
    return 1;
  if (bi != buf) {
    if (bi) {
      nfree(bi->address);
      nfree(bi);
    }
    bi = e->u.extra = buf;
  }
  if (bi && !noshare && !(u->flags & USER_UNSHARED)) {
    shareout(NULL, "c BOTADDR %s %s %d %d\n", u->handle,
             bi->address, bi->telnet_port, bi->relay_port);
  }
  return 1;
}

static int botaddr_tcl_get(Tcl_Interp * interp, struct userrec *u,
                           struct user_entry *e, int argc, char **argv)
{
  register struct bot_addr *bi = (struct bot_addr *) e->u.extra;
  char number[20];

  sprintf(number, " %d", bi->telnet_port);
  Tcl_AppendResult(interp, bi->address, number, NULL);
  sprintf(number, " %d", bi->relay_port);
  Tcl_AppendResult(interp, number, NULL);
  return TCL_OK;
}

static int botaddr_tcl_set(Tcl_Interp * irp, struct userrec *u,
                           struct user_entry *e, int argc, char **argv)
{
  register struct bot_addr *bi = (struct bot_addr *) e->u.extra;

  BADARGS(4, 6, " handle type address ?telnetport ?relayport??");

  if (u->flags & USER_BOT) {
    /* Silently ignore for users */
    if (!bi) {
      bi = user_malloc(sizeof(struct bot_addr));
      egg_bzero(bi, sizeof(struct bot_addr));
    } else
      nfree(bi->address);
    bi->address = user_malloc(strlen(argv[3]) + 1);
    strcpy(bi->address, argv[3]);
    if (argc > 4)
      bi->telnet_port = atoi(argv[4]);
    if (argc > 5)
      bi->relay_port = atoi(argv[5]);
    if (!bi->telnet_port)
      bi->telnet_port = 3333;
    if (!bi->relay_port)
      bi->relay_port = bi->telnet_port;
    botaddr_set(u, e, bi);
  }
  return TCL_OK;
}

static int botaddr_expmem(struct user_entry *e)
{
  register struct bot_addr *bi = (struct bot_addr *) e->u.extra;

  return strlen(bi->address) + 1 + sizeof(struct bot_addr);
}

static void botaddr_display(int idx, struct user_entry *e)
{
  register struct bot_addr *bi = (struct bot_addr *) e->u.extra;

  dprintf(idx, "  ADDRESS: %.70s\n", bi->address);
  dprintf(idx, "     users: %d, bots: %d\n", bi->relay_port, bi->telnet_port);
}

static int botaddr_gotshare(struct userrec *u, struct user_entry *e,
                            char *buf, int idx)
{
  struct bot_addr *bi = user_malloc(sizeof(struct bot_addr));
  char *arg;

  egg_bzero(bi, sizeof(struct bot_addr));
  arg = newsplit(&buf);
  bi->address = user_malloc(strlen(arg) + 1);
  strcpy(bi->address, arg);
  arg = newsplit(&buf);
  bi->telnet_port = atoi(arg);
  bi->relay_port = atoi(buf);
  if (!bi->telnet_port)
    bi->telnet_port = 3333;
  if (!bi->relay_port)
    bi->relay_port = bi->telnet_port;
  if (!(dcc[idx].status & STAT_GETTING))
    putlog(LOG_CMDS, "*", "%s: change botaddr %s", dcc[idx].nick, u->handle);
  return botaddr_set(u, e, bi);
}

static int botaddr_dupuser(struct userrec *new, struct userrec *old,
                           struct user_entry *e)
{
  if (old->flags & USER_BOT) {
    struct bot_addr *bi = e->u.extra, *bi2;

    if (bi) {
      bi2 = user_malloc(sizeof(struct bot_addr));

      bi2->telnet_port = bi->telnet_port;
      bi2->relay_port = bi->relay_port;
      bi2->address = user_malloc(strlen(bi->address) + 1);
      strcpy(bi2->address, bi->address);
      return set_user(&USERENTRY_BOTADDR, new, bi2);
    }
  }
  return 0;
}

struct user_entry_type USERENTRY_BOTADDR = {
  0,                            /* always 0 ;) */
  botaddr_gotshare,
  botaddr_dupuser,
  botaddr_unpack,
  botaddr_pack,
  botaddr_write_userfile,
  botaddr_kill,
  def_get,
  botaddr_set,
  botaddr_tcl_get,
  botaddr_tcl_set,
  botaddr_expmem,
  botaddr_display,
  "BOTADDR"
};

int xtra_set(struct userrec *u, struct user_entry *e, void *buf)
{
  struct xtra_key *curr, *old = NULL, *new = buf;

  for (curr = e->u.extra; curr; curr = curr->next) {
    if (curr->key && !egg_strcasecmp(curr->key, new->key)) {
      old = curr;
      break;
    }
  }
  if (!old && (!new->data || !new->data[0])) {
    /* Delete non-existant entry -- doh ++rtc */
    nfree(new->key);
    if (new->data)
      nfree(new->data);
    nfree(new);
    return TCL_OK;
  }

  /* We will possibly free new below, so let's send the information
   * to the botnet now
   */
  if (!noshare && !(u->flags & (USER_BOT | USER_UNSHARED)))
    shareout(NULL, "c XTRA %s %s %s\n", u->handle, new->key,
             new->data ? new->data : "");
  if ((old && old != new) || !new->data || !new->data[0]) {
    egg_list_delete(&e->u.list, (struct list_type *) old);
    nfree(old->key);
    nfree(old->data);
    nfree(old);
  }
  if (old != new && new->data) {
    if (new->data[0])
      list_insert((&e->u.extra), new)  /* do not add a ';' here */
  } else {
    if (new->data)
      nfree(new->data);
    nfree(new->key);
    nfree(new);
  }
  return TCL_OK;
}

static int xtra_tcl_set(Tcl_Interp * irp, struct userrec *u,
                        struct user_entry *e, int argc, char **argv)
{
  struct xtra_key *xk;
  int l;

  BADARGS(4, 5, " handle type key ?value?");

  xk = user_malloc(sizeof(struct xtra_key));
  l = strlen(argv[3]);
  egg_bzero(xk, sizeof(struct xtra_key));
  if (l > 500)
    l = 500;
  xk->key = user_malloc(l + 1);
  strncpyz(xk->key, argv[3], l + 1);

  if (argc == 5) {
    int k = strlen(argv[4]);

    if (k > 500 - l)
      k = 500 - l;
    xk->data = user_malloc(k + 1);
    strncpyz(xk->data, argv[4], k + 1);
  }
  xtra_set(u, e, xk);
  return TCL_OK;
}

int xtra_unpack(struct userrec *u, struct user_entry *e)
{
  struct list_type *curr, *head;
  struct xtra_key *t;
  char *key, *data;

  head = curr = e->u.list;
  e->u.extra = NULL;
  while (curr) {
    t = user_malloc(sizeof(struct xtra_key));

    data = curr->extra;
    key = newsplit(&data);
    if (data[0]) {
      t->key = user_malloc(strlen(key) + 1);
      strcpy(t->key, key);
      t->data = user_malloc(strlen(data) + 1);
      strcpy(t->data, data);
      list_insert((&e->u.extra), t);
    }
    curr = curr->next;
  }
  list_type_kill(head);
  return 1;
}

static int xtra_pack(struct userrec *u, struct user_entry *e)
{
  struct list_type *t;
  struct xtra_key *curr, *next;

  curr = e->u.extra;
  e->u.list = NULL;
  while (curr) {
    t = user_malloc(sizeof(struct list_type));
    t->extra = user_malloc(strlen(curr->key) + strlen(curr->data) + 4);
    sprintf(t->extra, "%s %s", curr->key, curr->data);
    list_insert((&e->u.list), t);
    next = curr->next;
    nfree(curr->key);
    nfree(curr->data);
    nfree(curr);
    curr = next;
  }
  return 1;
}

static void xtra_display(int idx, struct user_entry *e)
{
  int code, lc, j;
  EGG_CONST char **list;
  struct xtra_key *xk;

  code = Tcl_SplitList(interp, whois_fields, &lc, &list);
  if (code == TCL_ERROR)
    return;
  /* Scan thru xtra field, searching for matches */
  for (xk = e->u.extra; xk; xk = xk->next) {
    /* Ok, it's a valid xtra field entry */
    for (j = 0; j < lc; j++) {
      if (!egg_strcasecmp(list[j], xk->key))
        dprintf(idx, "  %s: %s\n", xk->key, xk->data);
    }
  }
  Tcl_Free((char *) list);
}

static int xtra_gotshare(struct userrec *u, struct user_entry *e,
                         char *buf, int idx)
{
  char *arg;
  struct xtra_key *xk;
  int l;

  arg = newsplit(&buf);
  if (!arg[0])
    return 1;

  xk = user_malloc(sizeof(struct xtra_key));
  egg_bzero(xk, sizeof(struct xtra_key));
  l = strlen(arg);
  if (l > 500)
    l = 500;
  xk->key = user_malloc(l + 1);
  strncpyz(xk->key, arg, l + 1);

  if (buf[0]) {
    int k = strlen(buf);

    if (k > 500 - l)
      k = 500 - l;
    xk->data = user_malloc(k + 1);
    strncpyz(xk->data, buf, k + 1);
  }
  xtra_set(u, e, xk);
  return 1;
}

static int xtra_dupuser(struct userrec *new, struct userrec *old,
                        struct user_entry *e)
{
  struct xtra_key *x1, *x2;

  for (x1 = e->u.extra; x1; x1 = x1->next) {
    x2 = user_malloc(sizeof(struct xtra_key));

    x2->key = user_malloc(strlen(x1->key) + 1);
    strcpy(x2->key, x1->key);
    x2->data = user_malloc(strlen(x1->data) + 1);
    strcpy(x2->data, x1->data);
    set_user(&USERENTRY_XTRA, new, x2);
  }
  return 1;
}

static int xtra_write_userfile(FILE *f, struct userrec *u,
                               struct user_entry *e)
{
  struct xtra_key *x;

  for (x = e->u.extra; x; x = x->next)
    if (fprintf(f, "--XTRA %s %s\n", x->key, x->data) == EOF)
      return 0;
  return 1;
}

int xtra_kill(struct user_entry *e)
{
  struct xtra_key *x, *y;

  for (x = e->u.extra; x; x = y) {
    y = x->next;
    nfree(x->key);
    nfree(x->data);
    nfree(x);
  }
  nfree(e);
  return 1;
}

static int xtra_tcl_get(Tcl_Interp * irp, struct userrec *u,
                        struct user_entry *e, int argc, char **argv)
{
  struct xtra_key *x;

  BADARGS(3, 4, " handle XTRA ?key?");

  if (argc == 4) {
    for (x = e->u.extra; x; x = x->next)
      if (!egg_strcasecmp(argv[3], x->key)) {
        Tcl_AppendResult(irp, x->data, NULL);
        return TCL_OK;
      }
    return TCL_OK;
  }
  for (x = e->u.extra; x; x = x->next) {
    char *p;
    EGG_CONST char *list[2];

    list[0] = x->key;
    list[1] = x->data;
    p = Tcl_Merge(2, list);
    Tcl_AppendElement(irp, p);
    Tcl_Free((char *) p);
  }
  return TCL_OK;
}

static int xtra_expmem(struct user_entry *e)
{
  struct xtra_key *x;
  int tot = 0;

  for (x = e->u.extra; x; x = x->next) {
    tot += sizeof(struct xtra_key);

    tot += strlen(x->key) + 1;
    tot += strlen(x->data) + 1;
  }
  return tot;
}

struct user_entry_type USERENTRY_XTRA = {
  0,
  xtra_gotshare,
  xtra_dupuser,
  xtra_unpack,
  xtra_pack,
  xtra_write_userfile,
  xtra_kill,
  def_get,
  xtra_set,
  xtra_tcl_get,
  xtra_tcl_set,
  xtra_expmem,
  xtra_display,
  "XTRA"
};

static int hosts_dupuser(struct userrec *new, struct userrec *old,
                         struct user_entry *e)
{
  struct list_type *h;

  for (h = e->u.extra; h; h = h->next)
    set_user(&USERENTRY_HOSTS, new, h->extra);
  return 1;
}

static int hosts_null(struct userrec *u, struct user_entry *e)
{
  return 1;
}

static int hosts_write_userfile(FILE *f, struct userrec *u,
                                struct user_entry *e)
{
  struct list_type *h;

  for (h = e->u.extra; h; h = h->next)
    if (fprintf(f, "--HOSTS %s\n", h->extra) == EOF)
      return 0;
  return 1;
}

static int hosts_kill(struct user_entry *e)
{
  list_type_kill(e->u.list);
  nfree(e);
  return 1;
}

static int hosts_expmem(struct user_entry *e)
{
  return list_type_expmem(e->u.list);
}

static void hosts_display(int idx, struct user_entry *e)
{
  char s[1024];
  struct list_type *q;

  s[0] = 0;
  strcpy(s, "  HOSTS: ");
  for (q = e->u.list; q; q = q->next) {
    if (s[0] && !s[9])
      strcat(s, q->extra);
    else if (!s[0])
      sprintf(s, "         %s", q->extra);
    else {
      if (strlen(s) + strlen(q->extra) + 2 > 65) {
        dprintf(idx, "%s\n", s);
        sprintf(s, "         %s", q->extra);
      } else {
        strcat(s, ", ");
        strcat(s, q->extra);
      }
    }
  }
  if (s[0])
    dprintf(idx, "%s\n", s);
}

static int hosts_set(struct userrec *u, struct user_entry *e, void *buf)
{
  if (!buf || !egg_strcasecmp(buf, "none")) {
    /* When the bot crashes, it's in this part, not in the 'else' part */
    list_type_kill(e->u.list);
    e->u.list = NULL;
  } else {
    char *host = buf, *p = strchr(host, ',');
    struct list_type **t;

    /* Can't have ,'s in hostmasks */
    while (p) {
      *p = '?';
      p = strchr(host, ',');
    }
    /* fred1: check for redundant hostmasks with
     * controversial "superpenis" algorithm ;) */
    /* I'm surprised Raistlin hasn't gotten involved in this controversy */
    t = &(e->u.list);
    while (*t) {
      if (cmp_usermasks(host, (*t)->extra)) {
        struct list_type *u;

        u = *t;
        *t = (*t)->next;
        if (u->extra)
          nfree(u->extra);
        nfree(u);
      } else
        t = &((*t)->next);
    }
    *t = user_malloc(sizeof(struct list_type));

    (*t)->next = NULL;
    (*t)->extra = user_malloc(strlen(host) + 1);
    strcpy((*t)->extra, host);
  }
  return 1;
}

static int hosts_tcl_get(Tcl_Interp * irp, struct userrec *u,
                         struct user_entry *e, int argc, char **argv)
{
  struct list_type *x;

  BADARGS(3, 3, " handle HOSTS");

  for (x = e->u.list; x; x = x->next)
    Tcl_AppendElement(irp, x->extra);
  return TCL_OK;
}

static int hosts_tcl_set(Tcl_Interp * irp, struct userrec *u,
                         struct user_entry *e, int argc, char **argv)
{
  BADARGS(3, 4, " handle HOSTS ?host?");

  if (argc == 4)
    addhost_by_handle(u->handle, argv[3]);
  else
    addhost_by_handle(u->handle, "none");       /* drummer */
  return TCL_OK;
}

static int hosts_gotshare(struct userrec *u, struct user_entry *e,
                          char *buf, int idx)
{
  /* doh, try to be too clever and it bites your butt */
  return 0;
}

struct user_entry_type USERENTRY_HOSTS = {
  0,
  hosts_gotshare,
  hosts_dupuser,
  hosts_null,
  hosts_null,
  hosts_write_userfile,
  hosts_kill,
  def_get,
  hosts_set,
  hosts_tcl_get,
  hosts_tcl_set,
  hosts_expmem,
  hosts_display,
  "HOSTS"
};

int egg_list_append(struct list_type **h, struct list_type *i)
{
  for (; *h; h = &((*h)->next));
  *h = i;
  return 1;
}

int egg_list_delete(struct list_type **h, struct list_type *i)
{
  for (; *h; h = &((*h)->next))
    if (*h == i) {
      *h = i->next;
      return 1;
    }
  return 0;
}

int egg_list_contains(struct list_type *h, struct list_type *i)
{
  for (; h; h = h->next)
    if (h == i) {
      return 1;
    }
  return 0;
}

int add_entry_type(struct user_entry_type *type)
{
  struct userrec *u;

  list_insert(&entry_type_list, type);
  for (u = userlist; u; u = u->next) {
    struct user_entry *e = find_user_entry(type, u);

    if (e && e->name) {
      e->type = type;
      e->type->unpack(u, e);
      nfree(e->name);
      e->name = NULL;
    }
  }
  return 1;
}

int del_entry_type(struct user_entry_type *type)
{
  struct userrec *u;

  for (u = userlist; u; u = u->next) {
    struct user_entry *e = find_user_entry(type, u);

    if (e && !e->name) {
      e->type->pack(u, e);
      e->name = user_malloc(strlen(e->type->name) + 1);
      strcpy(e->name, e->type->name);
      e->type = NULL;
    }
  }
  return egg_list_delete((struct list_type **) &entry_type_list,
                     (struct list_type *) type);
}

struct user_entry_type *find_entry_type(char *name)
{
  struct user_entry_type *p;

  for (p = entry_type_list; p; p = p->next) {
    if (!egg_strcasecmp(name, p->name))
      return p;
  }
  return NULL;
}

struct user_entry *find_user_entry(struct user_entry_type *et,
                                   struct userrec *u)
{
  struct user_entry **e, *t;

  for (e = &(u->entries); *e; e = &((*e)->next)) {
    if (((*e)->type == et) ||
        ((*e)->name && !egg_strcasecmp((*e)->name, et->name))) {
      t = *e;
      *e = t->next;
      t->next = u->entries;
      u->entries = t;
      return t;
    }
  }
  return NULL;
}

void *get_user(struct user_entry_type *et, struct userrec *u)
{
  struct user_entry *e;

  if (u && (e = find_user_entry(et, u)))
    return et->get(u, e);
  return 0;
}

int set_user(struct user_entry_type *et, struct userrec *u, void *d)
{
  struct user_entry *e;
  int r;

  if (!u || !et)
    return 0;

  if (!(e = find_user_entry(et, u))) {
    e = user_malloc(sizeof(struct user_entry));

    e->type = et;
    e->name = NULL;
    e->u.list = NULL;
    list_insert((&(u->entries)), e);
  }
  r = et->set(u, e, d);
  if (!e->u.list) {
    egg_list_delete((struct list_type **) &(u->entries), (struct list_type *) e);
    nfree(e);
  }
  return r;
}
