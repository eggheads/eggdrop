/*
 * userent.c -- handles:
 *   user-entry handling, new style more versatile.
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2021 Eggheads Development Team
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


int share_greet = 0; /* Share greeting info                      */
int remove_pass = 0; /* create and keep encryption mod passwords */
struct user_entry_type *entry_type_list;


void init_userent()
{
  entry_type_list = 0;
  add_entry_type(&USERENTRY_COMMENT);
  add_entry_type(&USERENTRY_XTRA);
  add_entry_type(&USERENTRY_INFO);
  add_entry_type(&USERENTRY_LASTON);
  add_entry_type(&USERENTRY_BOTADDR);
  add_entry_type(&USERENTRY_PASS);
  add_entry_type(&USERENTRY_PASS2);
  add_entry_type(&USERENTRY_HOSTS);
  add_entry_type(&USERENTRY_BOTFL);
#ifdef TLS
  add_entry_type(&USERENTRY_FPRINT);
#endif
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

    e->u.string = user_realloc(e->u.string, l + 1);

    strlcpy(e->u.string, string, l + 1);

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

int def_tcl_append(Tcl_Interp * interp, struct userrec *u,
                   struct user_entry *e)
{
  Tcl_AppendElement(interp, e->u.string);
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
  "COMMENT",
  def_tcl_append
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
  "INFO",
  def_tcl_append
};

int pass2_set(struct userrec *u, struct user_entry *e, void *new)
{
  if (e->u.extra) {
    explicit_bzero(e->u.extra, strlen(e->u.extra));
    nfree(e->u.extra);
  }
  if (new) { /* set PASS2 */
    e->u.extra = user_malloc(strlen(new) + 1);
    strcpy(e->u.extra, new);
  }
  else /* remove PASS2 */
    e->u.extra = NULL;
  return 0;
}

static int def_tcl_null(Tcl_Interp * irp, struct userrec *u,
                        struct user_entry *e, int argc, char **argv)
{
  Tcl_AppendResult(irp, "Please use PASS instead.", NULL);
  return TCL_ERROR;
}

struct user_entry_type USERENTRY_PASS2 = {
  0,
  0,
  0,
  def_unpack,
  def_pack,
  def_write_userfile,
  def_kill,
  def_get,
  pass2_set,
  def_tcl_null,
  def_tcl_null,
  def_expmem,
  0,
  "PASS2",
  def_tcl_append
};

int pass_set(struct userrec *u, struct user_entry *e, void *buf)
{
  char *pass = buf;
  unsigned char *p;
  char new[PASSWORDLEN];
  char *new2 = 0;

  /* encrypt_pass means encryption module is loaded
   * encrypt_pass2 means encryption2 module is loaded
   */
  if (encrypt_pass && e->u.extra) {
    explicit_bzero(e->u.extra, strlen(e->u.extra));
    nfree(e->u.extra);
  }
  if (!pass || !pass[0] || (pass[0] == '-')) {
    /* empty string or '-' means remove passwords */
    if (encrypt_pass) /* remove PASS */
      e->u.extra = NULL;
    if (encrypt_pass2) /* remove PASS2 */
      set_user(&USERENTRY_PASS2, u, NULL);
  }
  else {
    /* sanitize password */
    if (strlen(pass) > PASSWORDMAX)
      pass[PASSWORDMAX] = 0;
    p = (unsigned char *) pass;
    while (*p) {
      if ((*p <= 32) || (*p == 127))
        *p = '?';
      p++;
    }
    /* load new with new PASS
     * load new2 with new PASS2
     */
    if (u->flags & USER_BOT) {
      /* set PASS and PASS2 cleartext password */
      strlcpy(new, pass, sizeof new);
      if (encrypt_pass2)
        new2 = new;
    }
    else if (pass[0] == '+') {
      /* '+' means pass is already encrypted, set PASS = pass */
      strlcpy(new, pass, sizeof new);
      /* due to module api encrypted pass2 cannot be available here
       * caller must do set_user(&USERENTRY_PASS2, u, password);
       * probably only share.c:dup_userlist()
       */
    }
    else {
      /* encrypt password into new and/or new2 depending on the encryption
       * modules loaded and the value of remove-pass
       */
      if (encrypt_pass && (!encrypt_pass2 || !remove_pass))
        encrypt_pass(pass, new);
      if (encrypt_pass2)
        new2 = encrypt_pass2(pass);
    }
    /* set PASS to new and PASS2 to new2 depending on the encryption modules
     * loaded and the value of remove-pass
     */
    if (encrypt_pass && (!encrypt_pass2 || !remove_pass)) {
      /* set PASS */
      e->u.extra = user_malloc(strlen(new) + 1);
      strcpy(e->u.extra, new);
    }
    if (new2) { /* implicit encrypt_pass2 && */
      /* set PASS2 */
      set_user(&USERENTRY_PASS2, u, new2);
      if (encrypt_pass && remove_pass && e->u.extra)
        e->u.extra = NULL; /* remove PASS, e->u.extra already freed */
    }
    explicit_bzero(new, sizeof new);
    if (new2 && new2 != new)
      explicit_bzero(new2, strlen(new2));
  }
  if (!noshare && !(u->flags & (USER_BOT | USER_UNSHARED)))
    shareout(NULL, "c PASS %s %s\n", u->handle, pass ? pass : "");
  return 1;
}

static int pass_tcl_get(Tcl_Interp * interp, struct userrec *u,
                        struct user_entry *e, int argc, char **argv)
{
  char *pass = 0;

  if (encrypt_pass2)
    pass = get_user(&USERENTRY_PASS2, u);
  if (!pass)
    pass = e->u.string;
  Tcl_AppendResult(interp, pass, NULL);

  return TCL_OK;
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
  pass_tcl_get,
  pass_tcl_set,
  def_expmem,
  0,
  "PASS",
  def_tcl_append
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

    e->u.extra = buf;
  }
  /* donut share laston info */
  return 1;
}

static int laston_tcl_get(Tcl_Interp * irp, struct userrec *u,
                          struct user_entry *e, int argc, char **argv)
{
  struct laston_info *li = (struct laston_info *) e->u.extra;
  char number[22];
  struct chanuserrec *cr;

  BADARGS(3, 4, " handle LASTON ?channel?");

  if (argc == 4) {
    for (cr = u->chanrec; cr; cr = cr->next) {
      if (!rfc_casecmp(cr->channel, argv[3])) {
        Tcl_AppendResult(irp, int_to_base10(cr->laston), NULL);
        break;
      }
    }
    if (!cr)
      Tcl_AppendResult(irp, "0", NULL);
  } else {
    snprintf(number, sizeof number, "%" PRId64 " ", (int64_t) li->laston);
    Tcl_AppendResult(irp, number, li->lastonplace, NULL);
  }
  return TCL_OK;
}

static int laston_tcl_append(Tcl_Interp *irp, struct userrec *u,
                             struct user_entry *e)
{
  Tcl_DString ds;
  struct chanuserrec *cr;

  Tcl_DStringInit(&ds);
  for (cr = u->chanrec; cr; cr = cr->next) {
    Tcl_DStringAppendElement(&ds, cr->channel);
    Tcl_DStringAppendElement(&ds, int_to_base10(cr->laston));
  }
  Tcl_AppendElement(irp, Tcl_DStringValue(&ds));
  Tcl_DStringFree(&ds);
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
  "LASTON",
  laston_tcl_append
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
#ifdef TLS
    if (*q == '+')
      bi->ssl |= TLS_BOT;
#endif
    bi->telnet_port = atoi(q);
    if ((q = strchr(q, '/')))
#ifdef TLS
    {
      if (q[1] == '+')
        bi->ssl |= TLS_RELAY;
      bi->relay_port = atoi(q + 1);
    }
#else
      bi->relay_port = atoi(q + 1);
#endif
  }
#ifdef IPV6
  for (p = bi->address; *p; p++)
    if (*p == ';')
      *p = ':';
#endif
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
  char work[1024], *p, *q = work;
  struct bot_addr *bi;
  int l;

  bi = (struct bot_addr *) e->u.extra;
  for (p = bi->address; *p; p++)
    if (*p == ':')
      *q++ = ';';
    else
      *q++ = *p;
#ifdef TLS
  l = simple_sprintf(q, ":%s%u/%s%u", (bi->ssl & TLS_BOT) ? "+" : "",
                     bi->telnet_port, (bi->ssl & TLS_RELAY) ? "+" : "",
                     bi->relay_port);
#else
  l = simple_sprintf(q, ":%u/%u", bi->telnet_port, bi->relay_port);
#endif
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
  int ret = 1;
  char *p, *q, *addr;
  struct bot_addr *bi = (struct bot_addr *) e->u.extra;

  p = bi->address;
  addr = user_malloc(strlen(bi->address) + 1);
  for (q = addr; *p; p++)
    if (*p == ':')
      *q++ = ';';
    else
      *q++ = *p;
  *q = 0;
#ifdef TLS
  if (fprintf(f, "--%s %s:%s%u/%s%u\n", e->type->name, addr,
      (bi->ssl & TLS_BOT) ? "+" : "", bi->telnet_port, (bi->ssl & TLS_RELAY) ?
      "+" : "", bi->relay_port) == EOF)
#else
  if (fprintf(f, "--%s %s:%u/%u\n", e->type->name, addr,
      bi->telnet_port, bi->relay_port) == EOF)
#endif
    ret = 0;
  nfree(addr);
  return ret;
}

static int botaddr_set(struct userrec *u, struct user_entry *e, void *buf)
{
  struct bot_addr *bi = (struct bot_addr *) e->u.extra;

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
#ifdef TLS
    shareout(NULL, "c BOTADDR %s %s %s%d %s%d\n", u->handle, bi->address,
             (bi->ssl & TLS_BOT) ? "+" : "", bi->telnet_port,
             (bi->ssl & TLS_RELAY) ? "+" : "", bi->relay_port);
#else
    shareout(NULL, "c BOTADDR %s %s %d %d\n", u->handle,
             bi->address, bi->telnet_port, bi->relay_port);
#endif
  }
  return 1;
}

static int botaddr_tcl_dstring(Tcl_DString *ds, struct user_entry *e)
{
  struct bot_addr *bi = (struct bot_addr *)e->u.extra;

  Tcl_DStringAppendElement(ds, bi->address);
  /* This is safe because there are no special characters in the port */
  Tcl_DStringAppend(ds, " ", 1);
#ifdef TLS
  if (bi->ssl & TLS_BOT)
    Tcl_DStringAppend(ds, "+", 1);
#endif
  Tcl_DStringAppend(ds, int_to_base10(bi->telnet_port), -1);
  Tcl_DStringAppend(ds, " ", 1);
#ifdef TLS
  if (bi->ssl & TLS_RELAY)
    Tcl_DStringAppend(ds, "+", 1);
#endif
  Tcl_DStringAppend(ds, int_to_base10(bi->relay_port), -1);
  return TCL_OK;
}

static int botaddr_tcl_get(Tcl_Interp * interp, struct userrec *u,
                           struct user_entry *e, int argc, char **argv)
{
  Tcl_DString ds;
  Tcl_DStringInit(&ds);
  botaddr_tcl_dstring(&ds, e);

  Tcl_AppendResult(interp, Tcl_DStringValue(&ds), NULL);
  Tcl_DStringFree(&ds);
  return TCL_OK;
}

static int botaddr_tcl_append(Tcl_Interp * interp, struct userrec *u,
                           struct user_entry *e)
{
  Tcl_DString ds;
  Tcl_DStringInit(&ds);
  botaddr_tcl_dstring(&ds, e);

  Tcl_AppendElement(interp, Tcl_DStringValue(&ds));
  Tcl_DStringFree(&ds);
  return TCL_OK;
}

static int botaddr_tcl_set(Tcl_Interp * irp, struct userrec *u,
                           struct user_entry *e, int argc, char **argv)
{
  struct bot_addr *bi = (struct bot_addr *) e->u.extra;

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
    if (argc > 4) {
#ifdef TLS
      /* If no user port set, use bot port for both entries */
      if (*argv[4] == '+') {
        bi->ssl |= TLS_BOT;
        if (argc == 5) {
          bi->ssl |= TLS_RELAY;
        } 
      } else {
        bi->ssl &= ~TLS_BOT;
        if (argc == 5) {
          bi->ssl &= ~TLS_RELAY;
        }
      }
#endif
      bi->telnet_port = atoi(argv[4]);
      if (argc == 5) {
        bi->relay_port = atoi(argv[4]);
      }
    }
    if (argc > 5) {
#ifdef TLS
      if (*argv[5] == '+') {
        bi->ssl |= TLS_RELAY;
      } else {
        bi->ssl &= ~TLS_RELAY;
      }
#endif
      bi->relay_port = atoi(argv[5]);
    }
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
  struct bot_addr *bi = (struct bot_addr *) e->u.extra;

  return strlen(bi->address) + 1 + sizeof(struct bot_addr);
}

static void botaddr_display(int idx, struct user_entry *e)
{
  struct bot_addr *bi = (struct bot_addr *) e->u.extra;

  dprintf(idx, "  ADDRESS: %.70s\n", bi->address);
#ifdef TLS
  dprintf(idx, "     users: %s%d, bots: %s%d\n", (bi->ssl & TLS_RELAY) ? "+" : "",
          bi->relay_port, (bi->ssl & TLS_BOT) ? "+" : "", bi->telnet_port);
#else
  dprintf(idx, "     users: %d, bots: %d\n", bi->relay_port, bi->telnet_port);
#endif
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
#ifdef TLS
  if (*arg == '+')
    bi->ssl |= TLS_BOT;
  if (*buf == '+')
    bi->ssl |= TLS_RELAY;
#endif
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
#ifdef TLS
      bi2->ssl = bi->ssl;
#endif
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
  "BOTADDR",
  botaddr_tcl_append
};

int xtra_set(struct userrec *u, struct user_entry *e, void *buf)
{
  struct xtra_key *curr, *old = NULL, *new = buf;

  for (curr = e->u.extra; curr; curr = curr->next) {
    if (curr->key && !strcasecmp(curr->key, new->key)) {
      old = curr;
      break;
    }
  }
  if (!old && (!new->data || !new->data[0])) {
    /* Delete non-existent entry -- doh ++rtc */
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
    if (old == e->u.extra)
      e->u.extra = NULL;
    nfree(old);
    old = NULL;
  }
  /* don't do anything when old == new */
  if (old != new) {
    if (new->data && new->data[0])
      list_insert((&e->u.extra), new)  /* do not add a ';' here */
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
  egg_bzero(xk, sizeof(struct xtra_key));
  if (l > 500)
    l = 500;
  xk->key = user_malloc(l + 1);
  strlcpy(xk->key, argv[3], l + 1);

  if (argc == 5) {
    int k = strlen(argv[4]);

    if (k > 500 - l)
      k = 500 - l;
    xk->data = user_malloc(k + 1);
    strlcpy(xk->data, argv[4], k + 1);
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
      if (!strcasecmp(list[j], xk->key))
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
  strlcpy(xk->key, arg, l + 1);

  if (buf[0]) {
    int k = strlen(buf);

    if (k > 500 - l)
      k = 500 - l;
    xk->data = user_malloc(k + 1);
    strlcpy(xk->data, buf, k + 1);
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

static int xtra_tcl_dstring(Tcl_DString *ds, int sublist, struct user_entry *e)
{
  struct xtra_key *x;

  for (x = e->u.extra; x; x = x->next) {
    if (sublist)
      Tcl_DStringStartSublist(ds);
    Tcl_DStringAppendElement(ds, x->key);
    Tcl_DStringAppendElement(ds, x->data);
    if (sublist)
      Tcl_DStringEndSublist(ds);
  }
  return TCL_OK;
}

static int xtra_tcl_get(Tcl_Interp * irp, struct userrec *u,
                        struct user_entry *e, int argc, char **argv)
{
  struct xtra_key *x;

  BADARGS(3, 4, " handle XTRA ?key?");

  if (argc == 3) {
    Tcl_DString ds;
    Tcl_DStringInit(&ds);
    xtra_tcl_dstring(&ds, 1, e);
    Tcl_AppendResult(irp, Tcl_DStringValue(&ds), NULL);
    Tcl_DStringFree(&ds);
    return TCL_OK;
  }
  for (x = e->u.extra; x; x = x->next)
    if (!strcasecmp(argv[3], x->key)) {
      Tcl_AppendResult(irp, x->data, NULL);
      return TCL_OK;
    }
  return TCL_OK;
}

static int xtra_tcl_append(Tcl_Interp *irp, struct userrec *u,
                           struct user_entry *e)
{
  Tcl_DString ds;
  Tcl_DStringInit(&ds);
  xtra_tcl_dstring(&ds, 0, e);
  Tcl_AppendElement(irp, Tcl_DStringValue(&ds));
  Tcl_DStringFree(&ds);
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
  "XTRA",
  xtra_tcl_append
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
      strncat(s, q->extra, (sizeof s - strlen(s) -1));
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
  if (!buf || !strcasecmp(buf, "none")) {
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

static int hosts_tcl_append(Tcl_Interp *irp, struct userrec *u,
                            struct user_entry *e)
{
  Tcl_DString ds;
  struct list_type *x;

  Tcl_DStringInit(&ds);
  for (x = e->u.list; x; x = x->next)
    Tcl_DStringAppendElement(&ds, x->extra);
  Tcl_AppendElement(irp, Tcl_DStringValue(&ds));
  Tcl_DStringFree(&ds);
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
  "HOSTS",
  hosts_tcl_append
};

#ifdef TLS
int fprint_unpack(struct userrec *u, struct user_entry *e)
{
  char *tmp;

  tmp = ssl_fpconv(e->u.list->extra, NULL);
  nfree(e->u.list->extra);
  e->u.list->extra = NULL;
  list_type_kill(e->u.list);
  e->u.string = tmp;
  return 1;
}

int fprint_set(struct userrec *u, struct user_entry *e, void *buf)
{
  char *fp = buf;

  if (!fp || !fp[0] || (fp[0] == '-')) {
    if (e->u.string) {
      nfree(e->u.string);
      e->u.string = NULL;
    }
  } else {
    fp = ssl_fpconv(buf, e->u.string);
    if (fp)
      e->u.string = fp;
    else
      return 0;
  }
  if (!noshare && !(u->flags & (USER_BOT | USER_UNSHARED)))
    shareout(NULL, "c FPRINT %s %s\n", u->handle, e->u.string ? e->u.string : "");
  return 1;
}

static int fprint_tcl_set(Tcl_Interp * irp, struct userrec *u,
                        struct user_entry *e, int argc, char **argv)
{
  BADARGS(3, 4, " handle FPRINT ?new-fingerprint?");

  fprint_set(u, e, argc == 3 ? NULL : argv[3]);
  return TCL_OK;
}

struct user_entry_type USERENTRY_FPRINT = {
  0,
  def_gotshare,
  def_dupuser,
  fprint_unpack,
  def_pack,
  def_write_userfile,
  def_kill,
  def_get,
  fprint_set,
  def_tcl_get,
  fprint_tcl_set,
  def_expmem,
  0,
  "FPRINT",
  def_tcl_append
};
#endif /* TLS */

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
    egg_list_delete((struct list_type **) &(u->entries), (struct list_type *) e);
    nfree(e);
  }
  return r;
}
