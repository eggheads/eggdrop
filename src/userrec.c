/*
 * userrec.c -- handles:
 *   add_q() del_q() str2flags() flags2str() str2chflags() chflags2str()
 *   a bunch of functions to find and change user records
 *   change and check user (and channel-specific) flags
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

#include <sys/stat.h>
#include "main.h"
#include "users.h"
#include "chan.h"
#include "modules.h"
#include "tandem.h"

extern struct dcc_t *dcc;
extern struct chanset_t *chanset;
extern int default_flags, default_uflags, quiet_save, dcc_total, share_greet,
           remove_pass;
extern char ver[], botnetnick[];
extern time_t now;

int noshare = 1;                   /* don't send out to sharebots       */
struct userrec *userlist = NULL;   /* user records are stored here      */
struct userrec *lastuser = NULL;   /* last accessed user record         */
maskrec *global_bans = NULL, *global_exempts = NULL, *global_invites = NULL;
struct igrec *global_ign = NULL;
int cache_hit = 0, cache_miss = 0; /* temporary cache accounting        */
int userfile_perm = 0600;          /* Userfile permissions
                                    * (default rw-------)               */
char userfile[121];                /* where the user records are stored */

void *_user_malloc(int size, const char *file, int line)
{
#ifdef DEBUG_MEM
  char x[1024];
  const char *p;

  p = strrchr(file, '/');
  simple_sprintf(x, "userrec.c:%s", p ? p + 1 : file);
  return n_malloc(size, x, line);
#else
  return nmalloc(size);
#endif
}

void *_user_realloc(void *ptr, int size, const char *file, int line)
{
#ifdef DEBUG_MEM
  char x[1024];
  const char *p;

  p = strrchr(file, '/');
  simple_sprintf(x, "userrec.c:%s", p ? p + 1 : file);
  return n_realloc(ptr, size, x, line);
#else
  return nrealloc(ptr, size);
#endif
}

static int expmem_mask(struct maskrec *m)
{
  int result = 0;

  for (; m; m = m->next) {
    result += sizeof(struct maskrec);
    result += strlen(m->mask) + 1;
    if (m->user)
      result += strlen(m->user) + 1;
    if (m->desc)
      result += strlen(m->desc) + 1;
  }

  return result;
}

/* Memory we should be using
 */
int expmem_users()
{
  int tot;
  struct userrec *u;
  struct chanuserrec *ch;
  struct chanset_t *chan;
  struct user_entry *ue;
  struct igrec *i;

  tot = 0;
  for (u = userlist; u; u = u->next) {
    for (ch = u->chanrec; ch; ch = ch->next) {
      tot += sizeof(struct chanuserrec);

      if (ch->info != NULL)
        tot += strlen(ch->info) + 1;
    }
    tot += sizeof(struct userrec);

    for (ue = u->entries; ue; ue = ue->next) {
      tot += sizeof(struct user_entry);

      if (ue->name) {
        tot += strlen(ue->name) + 1;
        tot += list_type_expmem(ue->u.list);
      } else
        tot += ue->type->expmem(ue);
    }
  }
  /* Account for each channel's masks */
  for (chan = chanset; chan; chan = chan->next) {

    /* Account for each channel's ban-list user */
    tot += expmem_mask(chan->bans);

    /* Account for each channel's exempt-list user */
    tot += expmem_mask(chan->exempts);

    /* Account for each channel's invite-list user */
    tot += expmem_mask(chan->invites);
  }

  tot += expmem_mask(global_bans);
  tot += expmem_mask(global_exempts);
  tot += expmem_mask(global_invites);

  for (i = global_ign; i; i = i->next) {
    tot += sizeof(struct igrec);

    tot += strlen(i->igmask) + 1;
    if (i->user)
      tot += strlen(i->user) + 1;
    if (i->msg)
      tot += strlen(i->msg) + 1;
  }
  return tot;
}

int count_users(struct userrec *bu)
{
  int tot = 0;
  struct userrec *u;

  for (u = bu; u; u = u->next)
    tot++;
  return tot;
}

struct userrec *check_dcclist_hand(char *handle)
{
  int i;

  for (i = 0; i < dcc_total; i++)
    if (!strcasecmp(dcc[i].nick, handle))
      return dcc[i].user;
  return NULL;
}

struct userrec *get_user_by_handle(struct userrec *bu, char *handle)
{
  struct userrec *u, *ret;

  if (!handle)
    return NULL;
  /* FIXME: This should be done outside of this function. */
  rmspace(handle);
  if (!handle[0] || (handle[0] == '*'))
    return NULL;
  if (bu == userlist) {
    if (lastuser && !strcasecmp(lastuser->handle, handle)) {
      cache_hit++;
      return lastuser;
    }
    ret = check_dcclist_hand(handle);
    if (ret) {
      cache_hit++;
      return ret;
    }
    ret = check_chanlist_hand(handle);
    if (ret) {
      cache_hit++;
      return ret;
    }
    cache_miss++;
  }
  for (u = bu; u; u = u->next)
    if (!strcasecmp(u->handle, handle)) {
      if (bu == userlist)
        lastuser = u;
      return u;
    }
  return NULL;
}

/* Fix capitalization, etc
 */
void correct_handle(char *handle)
{
  struct userrec *u;

  u = get_user_by_handle(userlist, handle);
  if (u == NULL || handle == u->handle)
    return;
  strcpy(handle, u->handle);
}

/* This will be useful in a lot of places, much more code re-use so we
 * endup with a smaller executable bot. <cybah>
 */
void clear_masks(maskrec *m)
{
  maskrec *temp = NULL;

  for (; m; m = temp) {
    temp = m->next;
    if (m->mask)
      nfree(m->mask);
    if (m->user)
      nfree(m->user);
    if (m->desc)
      nfree(m->desc);
    nfree(m);
  }
}

void clear_userlist(struct userrec *bu)
{
  struct userrec *u, *v;
  int i;

  for (u = bu; u; u = v) {
    v = u->next;
    freeuser(u);
  }
  if (userlist == bu) {
    struct chanset_t *cst;

    for (i = 0; i < dcc_total; i++)
      dcc[i].user = NULL;
    clear_chanlist();
    lastuser = NULL;

    while (global_ign)
      delignore(global_ign->igmask);

    clear_masks(global_bans);
    clear_masks(global_exempts);
    clear_masks(global_invites);
    global_exempts = global_invites = global_bans = NULL;

    for (cst = chanset; cst; cst = cst->next) {
      clear_masks(cst->bans);
      clear_masks(cst->exempts);
      clear_masks(cst->invites);

      cst->bans = cst->exempts = cst->invites = NULL;
    }
  }
  /* Remember to set your userlist to NULL after calling this */
}

/* Find CLOSEST host match
 * (if "*!*@*" and "*!*@*clemson.edu" both match, use the latter!)
 *
 * Checks the chanlist first, to possibly avoid needless search.
 */
struct userrec *get_user_by_host(char *host)
{
  struct userrec *u, *ret;
  struct list_type *q;
  int cnt, i;
  char host2[UHOSTLEN];

  if (host == NULL)
    return NULL;
  rmspace(host);
  if (!host[0])
    return NULL;
  ret = check_chanlist(host);
  cnt = 0;
  if (ret != NULL) {
    cache_hit++;
    return ret;
  }
  cache_miss++;
  strlcpy(host2, host, sizeof host2);
  for (u = userlist; u; u = u->next) {
    q = get_user(&USERENTRY_HOSTS, u);
    for (; q; q = q->next) {
      i = match_useraddr(q->extra, host);
      if (i > cnt) {
        ret = u;
        cnt = i;
      }
    }
  }
  if (ret != NULL) {
    lastuser = ret;
    set_chanlist(host2, ret);
  }
  return ret;
}

/* Description: checks the password given against the user's password.
 * Check against the password "-" to find out if a user has no password set.
 *
 * If encryption2 module is loaded and PASS2 is set PASS2 is compared; else
 * PASS.
 *
 * Returns: 1 if the password matches for that user; 0 otherwise. Or if we are
 * checking against the password "-": 1 if the user has no password set; 0
 * otherwise.
 */
int u_pass_match(struct userrec *u, char *pass)
{
  char *cmp = 0, *new, new2[32];
  int pass2 = 1;
  struct user_entry *e;

  if (!u || !pass)
    return 0;
  if (encrypt_pass2)
    cmp = get_user(&USERENTRY_PASS2, u);
  if (!cmp) { /* implicit && encrypt_pass, due to eggdrop has at least one
                 encryption module loaded */
    cmp = get_user(&USERENTRY_PASS, u);
    pass2 = 0;
  }
  if (pass[0] == '-') {
    if (!cmp)
      return 1;
    return 0;
  }
  /* If password is not set in userrecord, or password is not sent */
  if (!cmp || !pass[0])
    return 0;
  if (u->flags & USER_BOT) {
    if (!crypto_verify(cmp, pass)) /* verify successful */
      return 1;
    return 0;
  }
  if (strlen(pass) > PASSWORDMAX)
    pass[PASSWORDMAX] = 0;
  if (pass2) {
    new = verify_pass2(pass, cmp);
    if (new) { /* verify successful */
      if (new != cmp) /* reenrypted with new parameters,
                         no need to strcmp() */
        set_user(&USERENTRY_PASS2, u, new);
      return 1;
    }
  }
  else if (encrypt_pass) {
    encrypt_pass(pass, new2);
    if (!crypto_verify(cmp, new2)) { /* verify successful */
      if (encrypt_pass2) {
        new = encrypt_pass2(pass);
        if (new) {
          set_user(&USERENTRY_PASS2, u, new);
          if (remove_pass) { /* implicit e->u.extra != NULL */
            e = find_user_entry(&USERENTRY_PASS, u);
            explicit_bzero(e->u.extra, strlen(e->u.extra));
            nfree(e->u.extra);
            e->u.extra = NULL;
            egg_list_delete((struct list_type **) &(u->entries), (struct list_type *) e);
            nfree(e);
          }
        }
      }
      return 1;
    }
  }
  return 0;
}

int write_user(struct userrec *u, FILE *f, int idx)
{
  char s[181];
  long tv;
  struct chanuserrec *ch;
  struct chanset_t *cst;
  struct user_entry *ue;
  struct flag_record fr = { FR_GLOBAL, 0, 0, 0, 0, 0 };

  fr.global = u->flags;

  fr.udef_global = u->flags_udef;
  build_flags(s, &fr, NULL);
  if (fprintf(f, "%-10s - %-24s\n", u->handle, s) == EOF)
    return 0;
  for (ch = u->chanrec; ch; ch = ch->next) {
    cst = findchan_by_dname(ch->channel);
    if (cst && ((idx < 0) || channel_shared(cst))) {
      if (idx >= 0) {
        fr.match = (FR_CHAN | FR_BOT);
        get_user_flagrec(dcc[idx].user, &fr, ch->channel);
      } else
        fr.chan = BOT_AGGRESSIVE;
      if ((fr.chan & BOT_AGGRESSIVE) || (fr.bot & BOT_GLOBAL)) {
        fr.match = FR_CHAN;
        fr.chan = ch->flags;
        fr.udef_chan = ch->flags_udef;
        build_flags(s, &fr, NULL);
        tv = ch->laston;
        if (fprintf(f, "! %-20s %lu %-10s %s\n", ch->channel, tv, s,
            (((idx < 0) || share_greet) && ch->info) ? ch->info : "") == EOF)
          return 0;
      }
    }
  }
  for (ue = u->entries; ue; ue = ue->next) {
    if (ue->name) {
      struct list_type *lt;

      for (lt = ue->u.list; lt; lt = lt->next)
        if (fprintf(f, "--%s %s\n", ue->name, lt->extra) == EOF)
          return 0;
    } else if (!ue->type->write_userfile(f, u, ue))
      return 0;
  }
  return 1;
}

int write_ignores(FILE *f, int idx)
{
  struct igrec *i;
  char *mask;
  long expire, added;

  if (global_ign)
    if (fprintf(f, IGNORE_NAME " - -\n") == EOF)        /* Daemus */
      return 0;
  for (i = global_ign; i; i = i->next) {
    mask = str_escape(i->igmask, ':', '\\');
    expire = i->expire;
    added = i->added;
    if (!mask ||
        fprintf(f, "- %s:%s%lu:%s:%lu:%s\n", mask,
                (i->flags & IGREC_PERM) ? "+" : "", expire,
                i->user ? i->user : botnetnick, added,
                i->msg ? i->msg : "") == EOF) {
      if (mask)
        nfree(mask);
      return 0;
    }
    nfree(mask);
  }
  return 1;
}

int sort_compare(struct userrec *a, struct userrec *b)
{
  /* Order by flags, then alphabetically
   * first bots: +h / +a / +l / other bots
   * then users: +n / +m / +o / other users
   * return true if (a > b)
   */
  if (a->flags & b->flags & USER_BOT) {
    if (~bot_flags(a) & bot_flags(b) & BOT_HUB)
      return 1;
    if (bot_flags(a) & ~bot_flags(b) & BOT_HUB)
      return 0;
    if (~bot_flags(a) & bot_flags(b) & BOT_ALT)
      return 1;
    if (bot_flags(a) & ~bot_flags(b) & BOT_ALT)
      return 0;
    if (~bot_flags(a) & bot_flags(b) & BOT_LEAF)
      return 1;
    if (bot_flags(a) & ~bot_flags(b) & BOT_LEAF)
      return 0;
  } else {
    if (~a->flags & b->flags & USER_BOT)
      return 1;
    if (a->flags & ~b->flags & USER_BOT)
      return 0;
    if (~a->flags & b->flags & USER_OWNER)
      return 1;
    if (a->flags & ~b->flags & USER_OWNER)
      return 0;
    if (~a->flags & b->flags & USER_MASTER)
      return 1;
    if (a->flags & ~b->flags & USER_MASTER)
      return 0;
    if (~a->flags & b->flags & USER_OP)
      return 1;
    if (a->flags & ~b->flags & USER_OP)
      return 0;
    if (~a->flags & b->flags & USER_HALFOP)
      return 1;
    if (a->flags & ~b->flags & USER_HALFOP)
      return 0;
  }
  return (strcasecmp(a->handle, b->handle) > 0);
}

void sort_userlist()
{
  int again;
  struct userrec *last, *p, *c, *n;

  again = 1;
  last = NULL;
  while ((userlist != last) && (again)) {
    p = NULL;
    c = userlist;
    n = c->next;
    again = 0;
    while (n != last) {
      if (sort_compare(c, n)) {
        again = 1;
        c->next = n->next;
        n->next = c;
        if (p == NULL)
          userlist = n;
        else
          p->next = n;
      }
      p = c;
      c = n;
      n = n->next;
    }
    last = c;
  }
}

/* Rewrite the entire user file. Call USERFILE hook as well, probably
 * causing the channel file to be rewritten as well.
 */
void write_userfile(int idx)
{
  FILE *f;
  char new_userfile[(sizeof userfile) + 4]; /* 4 = strlen("~new") */
  char s1[81];
  time_t tt;
  struct userrec *u;
  int ok;

  if (userlist == NULL)
    return;                     /* No point in saving userfile */

  egg_snprintf(new_userfile, sizeof new_userfile, "%s~new", userfile);

  f = fopen(new_userfile, "w");
  chmod(new_userfile, userfile_perm);
  if (f == NULL) {
    putlog(LOG_MISC, "*", USERF_ERRWRITE);
    return;
  }
  if (!quiet_save)
    putlog(LOG_MISC, "*", USERF_WRITING);

  sort_userlist();
  tt = now;
  strlcpy(s1, ctime(&tt), sizeof s1);
  fprintf(f, "#4v: %s -- %s -- written %s", ver, botnetnick, s1);
  ok = 1;
  /* Add all users except the -tn user */
  for (u = userlist; u && ok; u = u->next)
    if (strcasecmp(u->handle, EGG_BG_HANDLE) && !write_user(u, f, idx))
      ok = 0;
  if (!ok || !write_ignores(f, -1) || fflush(f)) {
    putlog(LOG_MISC, "*", "%s (%s)", USERF_ERRWRITE, strerror(ferror(f)));
    fclose(f);
    return;
  }
  fclose(f);
  call_hook(HOOK_USERFILE);
  movefile(new_userfile, userfile);
}

void backup_userfile(void)
{
  char s[(sizeof userfile) + 4]; /* 4 = strlen("~bak") */

  if (quiet_save < 2)
    putlog(LOG_MISC, "*", USERF_BACKUP);
  egg_snprintf(s, sizeof s, "%s~bak", userfile);
  copyfile(userfile, s);
}

int change_handle(struct userrec *u, char *newh)
{
  int i;
  char s[HANDLEN + 1];

  if (!u)
    return 0;
  /* Don't allow the -tn handle to be changed */
  if (!strcasecmp(u->handle, EGG_BG_HANDLE))
    return 0;
  /* Nothing that will confuse the userfile */
  if (!newh[1] && strchr(BADHANDCHARS, newh[0]))
    return 0;
  check_tcl_nkch(u->handle, newh);
  /* Yes, even send bot nick changes now: */
  if (!noshare && !(u->flags & USER_UNSHARED))
    shareout(NULL, "h %s %s\n", u->handle, newh);
  strlcpy(s, u->handle, sizeof s);
  strlcpy(u->handle, newh, sizeof u->handle);
  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_CHAT || dcc[i].type == &DCC_CHAT_PASS) &&
        !strcasecmp(dcc[i].nick, s)) {
      strlcpy(dcc[i].nick, newh, sizeof dcc[i].nick);
      if (dcc[i].type == &DCC_CHAT && dcc[i].u.chat->channel >= 0) {
        chanout_but(-1, dcc[i].u.chat->channel,
                    "*** Handle change: %s -> %s\n", s, newh);
        if (dcc[i].u.chat->channel < GLOBAL_CHANS)
          botnet_send_nkch(i, s);
      }
    }
  return 1;
}

extern int noxtra;

struct userrec *adduser(struct userrec *bu, char *handle, char *host,
                        char *pass, int flags)
{
  struct userrec *u, *x;
  struct xtra_key *xk;
  int oldshare = noshare;
  long tv;

  noshare = 1;
  u = nmalloc(sizeof *u);

  /* u->next=bu; bu=u; */
  strlcpy(u->handle, handle, sizeof u->handle);
  u->next = NULL;
  u->chanrec = NULL;
  u->entries = NULL;
  if (flags != USER_DEFAULT) {  /* drummer */
    u->flags = flags;
    u->flags_udef = 0;
  } else {
    u->flags = default_flags;
    u->flags_udef = default_uflags;
  }
  set_user(&USERENTRY_PASS, u, pass);
  if (!noxtra) {
    int l;
    xk = nmalloc(sizeof *xk);
    xk->key = nmalloc(8);
    strcpy(xk->key, "created");
    tv = now;
    l = snprintf(NULL, 0, "%li", tv);
    xk->data = nmalloc(l + 1);
    sprintf(xk->data, "%li", tv);
    set_user(&USERENTRY_XTRA, u, xk);
  }
  /* Strip out commas -- they're illegal */
  if (host && host[0]) {
    char *p;

    p = strchr(host, ',');
    while (p != NULL) {
      *p = '?';
      p = strchr(host, ',');
    }
    set_user(&USERENTRY_HOSTS, u, host);
  } else
    set_user(&USERENTRY_HOSTS, u, "none");
  if (bu == userlist)
    clear_chanlist();
  noshare = oldshare;
  if ((!noshare) && (handle[0] != '*') && (!(flags & USER_UNSHARED)) &&
      (bu == userlist)) {
    struct flag_record fr = { FR_GLOBAL, 0, 0, 0, 0, 0 };
    char x[100];

    fr.global = u->flags;

    fr.udef_global = u->flags_udef;
    build_flags(x, &fr, 0);
    shareout(NULL, "n %s %s %s %s\n", handle, host && host[0] ? host : "none",
             pass, x);
  }
  if (bu == NULL)
    bu = u;
  else {
    if ((bu == userlist) && (lastuser != NULL))
      x = lastuser;
    else
      x = bu;
    while (x->next != NULL)
      x = x->next;
    x->next = u;
    if (bu == userlist)
      lastuser = u;
  }
  return bu;
}

void freeuser(struct userrec *u)
{
  struct user_entry *ue, *ut;
  struct chanuserrec *ch, *z;

  if (u == NULL)
    return;

  ch = u->chanrec;
  while (ch) {
    z = ch;
    ch = ch->next;
    if (z->info != NULL)
      nfree(z->info);
    nfree(z);
  }
  u->chanrec = NULL;
  for (ue = u->entries; ue; ue = ut) {
    ut = ue->next;
    if (ue->name) {
      struct list_type *lt, *ltt;

      for (lt = ue->u.list; lt; lt = ltt) {
        ltt = lt->next;
        nfree(lt->extra);
        nfree(lt);
      }
      nfree(ue->name);
      nfree(ue);
    } else
      ue->type->kill(ue);
  }
  nfree(u);
}

int deluser(char *handle)
{
  struct userrec *u = userlist, *prev = NULL;
  int fnd = 0;

  while ((u != NULL) && (!fnd)) {
    if (!strcasecmp(u->handle, handle))
      fnd = 1;
    else {
      prev = u;
      u = u->next;
    }
  }
  if (!fnd)
    return 0;
  if (prev == NULL)
    userlist = u->next;
  else
    prev->next = u->next;
  if (!noshare && (handle[0] != '*') && !(u->flags & USER_UNSHARED))
    shareout(NULL, "k %s\n", handle);
  for (fnd = 0; fnd < dcc_total; fnd++)
    if (dcc[fnd].user == u)
      dcc[fnd].user = 0;        /* Clear any dcc users for this entry,
                                 * null is safe-ish */
  clear_chanlist();
  freeuser(u);
  lastuser = NULL;
  return 1;
}

int delhost_by_handle(char *handle, char *host)
{
  struct userrec *u;
  struct list_type *q, *qnext, *qprev;
  struct user_entry *e = NULL;
  int i = 0;

  u = get_user_by_handle(userlist, handle);
  if (!u)
    return 0;
  q = get_user(&USERENTRY_HOSTS, u);
  qprev = q;
  if (q) {
    if (!rfc_casecmp(q->extra, host)) {
      e = find_user_entry(&USERENTRY_HOSTS, u);
      e->u.extra = q->next;
      nfree(q->extra);
      nfree(q);
      i++;
      qprev = NULL;
      q = e->u.extra;
    } else
      q = q->next;
    while (q) {
      qnext = q->next;
      if (!rfc_casecmp(q->extra, host)) {
        if (qprev)
          qprev->next = q->next;
        else if (e) {
          e->u.extra = q->next;
          qprev = NULL;
        }
        nfree(q->extra);
        nfree(q);
        i++;
      } else
        qprev = q;
      q = qnext;
    }
  }
  if (!qprev)
    set_user(&USERENTRY_HOSTS, u, "none");
  if (!noshare && i && !(u->flags & USER_UNSHARED))
    shareout(NULL, "-h %s %s\n", handle, host);
  clear_chanlist();
  return i;
}

void addhost_by_handle(char *handle, char *host)
{
  struct userrec *u = get_user_by_handle(userlist, handle);

  set_user(&USERENTRY_HOSTS, u, host);
  /* u will be cached, so really no overhead, even tho this looks dumb: */
  if ((!noshare) && !(u->flags & USER_UNSHARED)) {
    if (u->flags & USER_BOT)
      shareout(NULL, "+bh %s %s\n", handle, host);
    else
      shareout(NULL, "+h %s %s\n", handle, host);
  }
  clear_chanlist();
}

void touch_laston(struct userrec *u, char *where, time_t timeval)
{
  if (!u)
    return;

  if (timeval > 1) {
    struct laston_info *li = get_user(&USERENTRY_LASTON, u);

    if (!li)
      li = nmalloc(sizeof *li);

    else if (li->lastonplace)
      nfree(li->lastonplace);
    li->laston = timeval;
    if (where) {
      li->lastonplace = nmalloc(strlen(where) + 1);
      strcpy(li->lastonplace, where);
    } else
      li->lastonplace = NULL;
    set_user(&USERENTRY_LASTON, u, li);
  } else if (timeval == 1)
    set_user(&USERENTRY_LASTON, u, 0);
}

/*  Go through all channel records and try to find a matching
 *  nick. Will return the user's user record if that is known
 *  to the bot.  (Fabian)
 *
 *  Warning: This is unreliable by concept!
 */
struct userrec *get_user_by_nick(char *nick)
{
  struct chanset_t *chan;
  memberlist *m;

  for (chan = chanset; chan; chan = chan->next) {
    for (m = chan->channel.member; m && m->nick[0]; m = m->next) {
      if (!rfc_casecmp(nick, m->nick)) {
        char word[512];

        egg_snprintf(word, sizeof word, "%s!%s", m->nick, m->userhost);
        /* No need to check the return value ourself */
        return get_user_by_host(word);
      }
    }
  }
  /* Sorry, no matches */
  return NULL;
}

void user_del_chan(char *dname)
{
  struct chanuserrec *ch, *och;
  struct userrec *u;

  for (u = userlist; u; u = u->next) {
    ch = u->chanrec;
    och = NULL;
    while (ch) {
      if (!rfc_casecmp(dname, ch->channel)) {
        if (och)
          och->next = ch->next;
        else
          u->chanrec = ch->next;

        if (ch->info)
          nfree(ch->info);
        nfree(ch);
        break;
      }
      och = ch;
      ch = ch->next;
    }
  }
}

/* Check if the console flags specified in md are permissible according
 * to the given flagrec. If the FR_CHAN flag is not set in fr->match,
 * only global user flags will be considered.
 * Returns: md with all unallowed flags masked out.
 */
int check_conflags(struct flag_record *fr, int md)
{
  if (!glob_owner(*fr))
    md &= ~(LOG_RAW | LOG_SRVOUT | LOG_BOTNETIN | LOG_BOTNETOUT | LOG_BOTSHRIN | LOG_BOTSHROUT);
  if (!glob_master(*fr)) {
    md &= ~(LOG_FILES | LOG_LEV1 | LOG_LEV2 | LOG_LEV3 | LOG_LEV4 |
            LOG_LEV5 | LOG_LEV6 | LOG_LEV7 | LOG_LEV8 | LOG_DEBUG |
            LOG_WALL);
    if ((fr->match & FR_CHAN) && !chan_master(*fr))
      md &= ~(LOG_MISC | LOG_CMDS);
  }
  if (!glob_botmast(*fr))
    md &= ~(LOG_BOTS | LOG_BOTMSG);
  return md;
}
