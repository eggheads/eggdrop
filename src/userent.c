/*
 * userent.c -- handles:
 * user-entry handling, new stylem more versatile.
 */
/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */
#include "main.h"
#include "users.h"

extern int noshare;
extern struct userrec *userlist;
extern struct dcc_t *dcc;
extern Tcl_Interp *interp;
extern char whois_fields[];

int share_greet = 0;		/* share greeting info */

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

  ASSERT (e != NULL);
  ASSERT (e->name != NULL);
  context;
  tmp = e->u.list->extra;
  e->u.list->extra = NULL;
  list_type_kill(e->u.list);
  e->u.string = tmp;
  return 1;
}

int def_pack(struct userrec *u, struct user_entry *e)
{
  char *tmp;

  ASSERT (e != NULL);
  ASSERT (e->name == NULL);
  tmp = e->u.string;
  e->u.list = user_malloc(sizeof(struct list_type));
  e->u.list->next = NULL;
  e->u.list->extra = tmp;
  return 1;
}

int def_kill(struct user_entry *e)
{
  context;
  nfree(e->u.string);
  nfree(e);
  return 1;
}

int def_write_userfile(FILE * f, struct userrec *u, struct user_entry *e)
{
  context;
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
  contextnote("drummer's bug?");
  if (buf)
    if (!((char *) buf)[0])
      buf = 0;
  if (e->u.string)
    nfree(e->u.string);
  context;
  if (buf) {
    unsigned char *p = (unsigned char *) buf, *q = (unsigned char *) buf;
    while (*p && (q - (unsigned char *) buf < 160)) {
      if (*p < 32)
	p++;
      else
	*q++ = *p++;
    }
    *q = 0;
    e->u.string = user_malloc(((q - (unsigned char *) buf) + 1));
    strncpy(e->u.string, (char *) buf, (q - (unsigned char *) buf));
    (e->u.string)[(q - (unsigned char *) buf)] = 0;
  } else
    e->u.string = NULL;
  if (!noshare && (!u || !(u->flags & (USER_BOT | USER_UNSHARED)))) {
    if ((e->type == &USERENTRY_INFO) && !share_greet)
      return 1;
    shareout(NULL, "c %s %s %s\n", e->type->name, u->handle, buf ? buf : "");
  }
  context;
  return 1;
}

int def_gotshare(struct userrec *u, struct user_entry *e,
		 char *data, int idx)
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
  context;
  return strlen(e->u.string) + 1;
}

void def_display(int idx, struct user_entry *e)
{
  context;
  dprintf(idx, "  %s: %s\n", e->type->name, e->u.string);
}

int def_dupuser(struct userrec *new, struct userrec *old,
		struct user_entry *e)
{
  return set_user(e->type, new, e->u.string);
}

static void comment_display(int idx, struct user_entry *e)
{
  context;
  if (dcc[idx].user && (dcc[idx].user->flags & USER_MASTER))
    dprintf(idx, "  COMMENT: %.70s\n", e->u.string);
}

struct user_entry_type USERENTRY_COMMENT =
{
  0,				/* always 0 ;) */
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

struct user_entry_type USERENTRY_INFO =
{
  0,				/* always 0 ;) */
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

    if (strlen(pass) > 15)
      pass[15] = 0;
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
  pass_set(u, e, argv[3]);
  return TCL_OK;
}

struct user_entry_type USERENTRY_PASS =
{
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

  ASSERT (e != NULL);
  ASSERT (e->name != NULL);
  context;
  par = e->u.list->extra;
  ASSERT (par != NULL);
  arg = newsplit (&par);
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
  struct laston_info *li;
  int l;

  ASSERT (e != NULL);
  ASSERT (e->u.extra != NULL);
  ASSERT (e->name == NULL);
  li = (struct laston_info *) e->u.extra;
  l = sprintf(work, "%lu %s", li->laston, li->lastonplace);
  e->u.list = user_malloc(sizeof(struct list_type));
  e->u.list->next = NULL;
  e->u.list->extra = user_malloc(l + 1);
  strcpy(e->u.list->extra, work);
  nfree(li->lastonplace);
  nfree(li);
  return 1;
}

static int laston_write_userfile(FILE * f,
				 struct userrec *u,
				 struct user_entry *e)
{
  struct laston_info *li = (struct laston_info *) e->u.extra;

  context;
  if (fprintf(f, "--LASTON %lu %s\n", li->laston,
	      li->lastonplace ? li->lastonplace : "") == EOF)
    return 0;
  return 1;
}

static int laston_kill(struct user_entry *e)
{
  context;
  if (((struct laston_info *) (e->u.extra))->lastonplace)
    nfree(((struct laston_info *) (e->u.extra))->lastonplace);
  nfree(e->u.extra);
  nfree(e);
  return 1;
}

static int laston_set(struct userrec *u, struct user_entry *e, void *buf)
{
  struct laston_info *li = (struct laston_info *) e->u.extra;

  context;
  if (li != buf) {
    if (li) {
      ASSERT (li->lastonplace != NULL);
      nfree(li->lastonplace);
      nfree(li);
    }
    contextnote("(sharebug) occurred in laston_set");

    e->u.extra = buf;
  }
  /* donut share laston unfo */
  return 1;
}

static int laston_tcl_get(Tcl_Interp * irp, struct userrec *u,
			  struct user_entry *e, int argc, char **argv)
{
  struct laston_info *li = (struct laston_info *) e->u.extra;
  char number[20];
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
    sprintf(number, "%lu ", li->laston);
    Tcl_AppendResult(irp, number, li->lastonplace, NULL);
  }
  return TCL_OK;
}

static int laston_tcl_set(Tcl_Interp * irp, struct userrec *u,
			  struct user_entry *e, int argc, char **argv)
{
  struct laston_info *li;
  struct chanuserrec *cr;

  BADARGS(5, 6, " handle LASTON time ?place?");
  if (argc == 5) {
    li = user_malloc(sizeof(struct laston_info));

    li->lastonplace = user_malloc(strlen(argv[4]) + 1);
    li->laston = atoi(argv[3]);
    strcpy(li->lastonplace, argv[4]);
    set_user(&USERENTRY_LASTON, u, li);
  } else {
    for (cr = u->chanrec; cr; cr = cr->next)
      if (!rfc_casecmp(cr->channel, argv[4]))
	cr->laston = atoi(argv[3]);
  }
  return TCL_OK;
}

static int laston_expmem(struct user_entry *e)
{
  context;
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

struct user_entry_type USERENTRY_LASTON =
{
  0,				/* always 0 ;) */
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

  ASSERT (e != NULL);
  ASSERT (e->name != NULL);
  context;
  bzero (bi, sizeof(struct bot_addr));

  if (!(q = strchr ((p = e->u.list->extra), ':'))) {
    bi->address = user_malloc (strlen (p) + 1);
    strcpy (bi->address, p);
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

  ASSERT (e != NULL);
  ASSERT (e->name == NULL);
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
  context;
  nfree(((struct bot_addr *) (e->u.extra))->address);
  nfree(e->u.extra);
  nfree(e);
  context;
  return 1;
}

static int botaddr_write_userfile(FILE * f, struct userrec *u, struct user_entry *e)
{
  register struct bot_addr *bi = (struct bot_addr *) e->u.extra;

  context;
  if (fprintf(f, "--%s %s:%u/%u\n", e->type->name, bi->address,
	      bi->telnet_port, bi->relay_port) == EOF)
    return 0;
  return 1;
}

static int botaddr_set(struct userrec *u, struct user_entry *e, void *buf)
{
  register struct bot_addr *bi = (struct bot_addr *) e->u.extra;

  context;
  if (!bi && !buf)
    return 1;
  if (bi != buf) {
    if (bi) {
      ASSERT (bi->address != NULL);
      nfree (bi->address);
      nfree (bi);
    }
    contextnote("(sharebug) occurred in botaddr_set");
    e->u.extra = buf;
  }
  if (!noshare && !(u && (u->flags & USER_UNSHARED))) {
    bi = (struct bot_addr *)e->u.extra;
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

  context;
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
    /* silently ignore for users */
    if (!bi) {
      bi = user_malloc(sizeof(struct bot_addr));
      bzero (bi, sizeof (struct bot_addr));
    } else {
      ASSERT (bi->address != NULL);
      nfree(bi->address);
    }
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

  context;
  return strlen(bi->address) + 1 + sizeof(struct bot_addr);
}

static void botaddr_display(int idx, struct user_entry *e)
{
  register struct bot_addr *bi = (struct bot_addr *) e->u.extra;

  context;
  dprintf(idx, "  ADDRESS: %.70s\n", bi->address);
  dprintf(idx, "     telnet: %d, relay: %d\n", bi->telnet_port, bi->relay_port);
}

static int botaddr_gotshare(struct userrec *u, struct user_entry *e,
			    char *buf, int idx)
{
  struct bot_addr *bi = user_malloc(sizeof(struct bot_addr));
  char *arg;

  bzero (bi, sizeof(struct bot_addr));
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
    putlog(LOG_CMDS, "*", "%s: change botaddr %s", dcc[idx].nick,
	   u->handle);
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

struct user_entry_type USERENTRY_BOTADDR =
{
  0,				/* always 0 ;) */
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

  ASSERT (new != NULL);
  context;
  for (curr = e->u.extra; curr; curr = curr->next) {
    if (curr->key && !strcasecmp(curr->key, new->key)) {
      old = curr;
      break;
    }
  }
  if (!old && (!new->data || !new->data[0])) {
    /* delete non-existant entry -- doh ++rtc */
    nfree(new->key);
    if (new->data)
      nfree(new->data);
    nfree(new);
    return TCL_OK;
  }

  /* we will possibly free new below, so let's send the information
   * to the botnet now */
  if (!noshare && !(u->flags & (USER_BOT | USER_UNSHARED)))
    shareout(NULL, "c XTRA %s %s %s\n", u->handle, new->key,
	     new->data ? new->data : "");
  if ((old && old != new) || !new->data || !new->data[0]) {
    list_delete((struct list_type **) (&e->u.extra),
		(struct list_type *) old);
    nfree(old->key);
    nfree(old->data);
    nfree(old);
  }
  if (old != new) {
    if (new->data && new->data[0])
      list_insert((&e->u.extra), new) /* do not add a ';' here */
    else {
      if (new->data)
        nfree(new->data);
      nfree(new->key);
      nfree(new);
    }
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
  bzero (xk, sizeof (struct xtra_key));
  if (l > 500)
    l = 500;
  xk->key = user_malloc(l + 1);
  strncpy(xk->key, argv[3], l);
  xk->key[l] = 0;

  if (argc == 4) {
    int k = strlen(argv[4]);
    if (k > 500 - l)
      k = 500 - l;
    xk->data = user_malloc(k + 1);
    strncpy(xk->data, argv[4], k);
    xk->data[k] = 0;
  }
  xtra_set(u, e, xk);
  return TCL_OK;
}

int xtra_unpack(struct userrec *u, struct user_entry *e)
{
  struct list_type *curr, *head;
  struct xtra_key *t;
  char *key, *data;

  ASSERT (e != NULL);
  ASSERT (e->name != NULL);
  context;

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

  ASSERT (e != NULL);
  ASSERT (e->name == NULL);
  context;
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
  struct xtra_key *xk;
  char **list;

  context;
  code = Tcl_SplitList(interp, whois_fields, &lc, &list);
  if (code == TCL_ERROR)
    return;
  /* scan thru xtra field, searching for matches */
  context;
  for (xk = e->u.extra; xk; xk = xk->next) {
    /* ok, it's a valid xtra field entry */
    context;
    for (j = 0; j < lc; j++) {
      if (strcasecmp(list[j], xk->key) == 0)
	dprintf(idx, "  %s: %s\n", xk->key, xk->data);
    }
  }
  n_free(list, "", 0);
  context;
}

static int xtra_gotshare(struct userrec *u, struct user_entry *e,
			 char *buf, int idx)
{
  char *arg;
  struct xtra_key *xk;
  int l;

  arg = newsplit (&buf);
  if (!arg[0])
    return 1;

  xk = user_malloc (sizeof(struct xtra_key));
  bzero (xk, sizeof(struct xtra_key));
  l = strlen(arg);
  if (l > 500)
    l = 500;
  xk->key = user_malloc(l + 1);
  strncpy(xk->key, arg, l);
  xk->key[l] = 0;

  if (buf[0]) {
    int k = strlen(buf);

    if (k > 500 - l)
      k = 500 - l;
    xk->data = user_malloc(k + 1);
    strncpy(xk->data, buf, k);
    xk->data[k] = 0;
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

static int xtra_write_userfile(FILE * f, struct userrec *u, struct user_entry *e)
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

  context;
  for (x = e->u.extra; x; x = y) {
    y = x->next;
    nfree(x->key);
    nfree(x->data);
    nfree(x);
  }
  nfree(e);
  context;
  return 1;
}

static int xtra_tcl_get(Tcl_Interp * irp, struct userrec *u,
			struct user_entry *e, int argc, char **argv)
{
  struct xtra_key *x;

  BADARGS(3, 4, " handle XTRA ?key?");
  if (argc == 4) {
    for (x = e->u.extra; x; x = x->next)
      if (!strcasecmp(argv[3], x->key)) {
	Tcl_AppendResult(irp, x->data, NULL);
	return TCL_OK;
      }
    return TCL_OK;
  }
  for (x = e->u.extra; x; x = x->next) {
    char *p, *list[2];

    list[0] = x->key;
    list[1] = x->data;
    p = Tcl_Merge(2, list);
    Tcl_AppendElement(irp, p);
    n_free(p, "", 0);
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

struct user_entry_type USERENTRY_XTRA =
{
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

static int hosts_write_userfile(FILE * f, struct userrec *u, struct user_entry *e)
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
      simple_sprintf(s, "         %s", q->extra);
    else {
      if (strlen(s) + strlen(q->extra) + 2 > 65) {
	dprintf(idx, "%s\n", s);
	simple_sprintf(s, "         %s", q->extra);
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
  context;
  if (!buf || !strcasecmp(buf, "none")) {
    contextnote("SEGV with sharing bug track");
    /* when the bot crashes, it's in this part, not in the 'else' part */
    contextnote(e ? "e is valid" : "e is NULL!");
    ASSERT (e != NULL); /* e cannot be null ++rtc */
    contextnote((e->u.list) ? "(sharebug) e->u.list is valid" : "(sharebug) e->u.list is NULL!");
    list_type_kill(e->u.list);
    contextnote("(sharebug[c1]) occurred in hosts_set - added 99/03/26");
    e->u.list = NULL;
    contextnote("(sharebug[c2]) occurred in hosts_set - added 99/03/26");
  } else {
    char *host = buf, *p = strchr(host, ',');
    struct list_type **t;

    contextnote("SEGV with sharing bug track");
    /* can't have ,'s in hostmasks */
    while (p) {
      *p = '?';
      p = strchr(host, ',');
    }
    /* fred1: check for redundant hostmasks with
     * controversial "superpenis" algorithm ;) */
    /* I'm surprised Raistlin hasn't gotten involved in this controversy */
    t = &(e->u.list);
    while (*t) {
      if (wild_match(host, (*t)->extra)) {
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
  contextnote("please visit http://www.eggheads.org/bugs.html now.");
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
  else /* {
     while (e->u.list && strcasecmp(e->u.list->extra, "none"))
       delhost_by_handle(u->handle, e->u.list->extra);
  } */
    addhost_by_handle(u->handle, "none"); /* drummer */
  return TCL_OK;
}

static int hosts_gotshare(struct userrec *u, struct user_entry *e,
			  char *buf, int idx)
{
  /* doh, try to be too clever and it bites your butt */
  return 0;
}

struct user_entry_type USERENTRY_HOSTS =
{
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

int list_append(struct list_type **h, struct list_type *i)
{
  for (; *h; h = &((*h)->next));
  *h = i;
  return 1;
}

int list_delete(struct list_type **h, struct list_type *i)
{
  for (; *h; h = &((*h)->next))
    if (*h == i) {
      *h = i->next;
      return 1;
    }
  return 0;
}

int list_contains(struct list_type *h, struct list_type *i)
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
  return list_delete((struct list_type **) &entry_type_list,
		     (struct list_type *) type);
}

struct user_entry_type *find_entry_type(char *name)
{
  struct user_entry_type *p;

  for (p = entry_type_list; p; p = p->next) {
    if (!strcasecmp(name, p->name))
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
	((*e)->name && !strcasecmp((*e)->name, et->name))) {
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
    list_delete((struct list_type **) &(u->entries), (struct list_type *) e);
    nfree(e);
  }
  return r;
}
