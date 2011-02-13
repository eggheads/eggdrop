/*
 * users.c -- handles:
 *   testing and enforcing ignores
 *   adding and removing ignores
 *   listing ignores
 *   auto-linking bots
 *   sending and receiving a userfile from a bot
 *   listing users ('.whois' and '.match')
 *   reading the user file
 *
 * dprintf'ized, 9nov1995
 *
 * $Id: users.c,v 1.60 2011/02/13 14:19:33 simple Exp $
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
#include "chan.h"
#include "modules.h"
#include "tandem.h"
char natip[121] = "";

#include <netinet/in.h>
#include <arpa/inet.h>

extern struct dcc_t *dcc;
extern struct userrec *userlist, *lastuser;
extern struct chanset_t *chanset;
extern int dcc_total, noshare;
extern char botnetnick[];
extern Tcl_Interp *interp;
extern time_t now;

char userfile[121] = "";        /* where the user records are stored */
int ignore_time = 10;           /* how many minutes will ignores last? */

/* is this nick!user@host being ignored? */
int match_ignore(char *uhost)
{
  struct igrec *ir;

  for (ir = global_ign; ir; ir = ir->next)
    if (match_useraddr(ir->igmask, uhost))
      return 1;
  return 0;
}

int equals_ignore(char *uhost)
{
  struct igrec *u = global_ign;

  for (; u; u = u->next)
    if (!rfc_casecmp(u->igmask, uhost)) {
      if (u->flags & IGREC_PERM)
        return 2;
      else
        return 1;
    }
  return 0;
}

int delignore(char *ign)
{
  int i, j;
  struct igrec **u;
  struct igrec *t;
  char temp[256];

  i = 0;
  if (!strchr(ign, '!') && (j = atoi(ign))) {
    for (u = &global_ign, j--; *u && j; u = &((*u)->next), j--);
    if (*u) {
      strncpyz(temp, (*u)->igmask, sizeof temp);
      i = 1;
    }
  } else {
    /* find the matching host, if there is one */
    for (u = &global_ign; *u && !i; u = &((*u)->next))
      if (!rfc_casecmp(ign, (*u)->igmask)) {
        strncpyz(temp, ign, sizeof temp);
        i = 1;
        break;
      }
  }
  if (i) {
    if (!noshare) {
      char *mask = str_escape(temp, ':', '\\');

      if (mask) {
        shareout(NULL, "-i %s\n", mask);
        nfree(mask);
      }
    }
    nfree((*u)->igmask);
    if ((*u)->msg)
      nfree((*u)->msg);
    if ((*u)->user)
      nfree((*u)->user);
    t = *u;
    *u = (*u)->next;
    nfree(t);
  }
  return i;
}

void addignore(char *ign, char *from, char *mnote, time_t expire_time)
{
  struct igrec *p = NULL, *l;

  for (l = global_ign; l; l = l->next)
    if (!rfc_casecmp(l->igmask, ign)) {
      p = l;
      break;
    }

  if (p == NULL) {
    p = user_malloc(sizeof(struct igrec));
    p->next = global_ign;
    global_ign = p;
  } else {
    nfree(p->igmask);
    nfree(p->user);
    nfree(p->msg);
  }

  p->expire = expire_time;
  p->added = now;
  p->flags = expire_time ? 0 : IGREC_PERM;
  p->igmask = user_malloc(strlen(ign) + 1);
  strcpy(p->igmask, ign);
  p->user = user_malloc(strlen(from) + 1);
  strcpy(p->user, from);
  p->msg = user_malloc(strlen(mnote) + 1);
  strcpy(p->msg, mnote);
  if (!noshare) {
    char *mask = str_escape(ign, ':', '\\');

    if (mask) {
      shareout(NULL, "+i %s %li %c %s %s\n", mask, expire_time - now,
               (p->flags & IGREC_PERM) ? 'p' : '-', from, mnote);
      nfree(mask);
    }
  }
}

/* take host entry from ignore list and display it ignore-style */
void display_ignore(int idx, int number, struct igrec *ignore)
{
  char dates[81], s[41];

  if (ignore->added) {
    daysago(now, ignore->added, s);
    sprintf(dates, "Started %s", s);
  } else
    dates[0] = 0;
  if (ignore->flags & IGREC_PERM)
    strcpy(s, "(perm)");
  else {
    char s1[41];

    days(ignore->expire, now, s1);
    sprintf(s, "(expires %s)", s1);
  }
  if (number >= 0)
    dprintf(idx, "  [%3d] %s %s\n", number, ignore->igmask, s);
  else
    dprintf(idx, "IGNORE: %s %s\n", ignore->igmask, s);
  if (ignore->msg && ignore->msg[0])
    dprintf(idx, "        %s: %s\n", ignore->user, ignore->msg);
  else
    dprintf(idx, "        %s %s\n", MODES_PLACEDBY, ignore->user);
  if (dates[0])
    dprintf(idx, "        %s\n", dates);
}

/* list the ignores and how long they've been active */
void tell_ignores(int idx, char *match)
{
  struct igrec *u = global_ign;
  int k = 1;

  if (u == NULL) {
    dprintf(idx, "No ignores.\n");
    return;
  }
  dprintf(idx, "%s:\n", IGN_CURRENT);
  for (; u; u = u->next) {
    if (match[0]) {
      if (cmp_usermasks(match, u->igmask) ||
          wild_match(match, u->msg) || wild_match(match, u->user))
        display_ignore(idx, k, u);
      k++;
    } else
      display_ignore(idx, k++, u);
  }
}

/* check for expired timed-ignores */
void check_expired_ignores()
{
  struct igrec **u = &global_ign;

  if (!*u)
    return;

  while (*u) {
    if (!((*u)->flags & IGREC_PERM) && (now >= (*u)->expire)) {
      putlog(LOG_MISC, "*", "%s %s (%s)", IGN_NOLONGER, (*u)->igmask,
             MISC_EXPIRED);
      delignore((*u)->igmask);
    } else
      u = &((*u)->next);
  }
}

/*        Channel mask loaded from user file. This function is
 *      add(ban|invite|exempt)_fully merged into one. <cybah>
 */
static void addmask_fully(struct chanset_t *chan, maskrec ** m,
                          maskrec ** global, char *mask, char *from, char *note,
                          time_t expire_time, int flags, time_t added,
                          time_t last)
{
  maskrec *p = user_malloc(sizeof(maskrec));
  maskrec **u = (chan) ? m : global;

  p->next = *u;
  *u = p;
  p->expire = expire_time;
  p->added = added;
  p->lastactive = last;
  p->flags = flags;
  p->mask = user_malloc(strlen(mask) + 1);
  strcpy(p->mask, mask);
  p->user = user_malloc(strlen(from) + 1);
  strcpy(p->user, from);
  p->desc = user_malloc(strlen(note) + 1);
  strcpy(p->desc, note);
}

static void restore_chanban(struct chanset_t *chan, char *host)
{
  char *expi, *add, *last, *user, *desc;
  int flags = 0;

  expi = strchr_unescape(host, ':', '\\');
  if (expi) {
    if (*expi == '+') {
      flags |= MASKREC_PERM;
      expi++;
    }
    add = strchr(expi, ':');
    if (add) {
      if (add[-1] == '*') {
        flags |= MASKREC_STICKY;
        add[-1] = 0;
      } else
        *add = 0;
      add++;
      if (*add == '+') {
        last = strchr(add, ':');
        if (last) {
          *last = 0;
          last++;
          user = strchr(last, ':');
          if (user) {
            *user = 0;
            user++;
            desc = strchr(user, ':');
            if (desc) {
              *desc = 0;
              desc++;
              addmask_fully(chan, &chan->bans, &global_bans, host, user,
                            desc, atoi(expi), flags, atoi(add), atoi(last));
              return;
            }
          }
        }
      } else {
        desc = strchr(add, ':');
        if (desc) {
          *desc = 0;
          desc++;
          addmask_fully(chan, &chan->bans, &global_bans, host, add, desc,
                        atoi(expi), flags, now, 0);
          return;
        }
      }
    }
  }
  putlog(LOG_MISC, "*", "*** Malformed banline for %s.",
         chan ? chan->dname : "global_bans");
}

static void restore_chanexempt(struct chanset_t *chan, char *host)
{
  char *expi, *add, *last, *user, *desc;
  int flags = 0;

  expi = strchr_unescape(host, ':', '\\');
  if (expi) {
    if (*expi == '+') {
      flags |= MASKREC_PERM;
      expi++;
    }
    add = strchr(expi, ':');
    if (add) {
      if (add[-1] == '*') {
        flags |= MASKREC_STICKY;
        add[-1] = 0;
      } else
        *add = 0;
      add++;
      if (*add == '+') {
        last = strchr(add, ':');
        if (last) {
          *last = 0;
          last++;
          user = strchr(last, ':');
          if (user) {
            *user = 0;
            user++;
            desc = strchr(user, ':');
            if (desc) {
              *desc = 0;
              desc++;
              addmask_fully(chan, &chan->exempts, &global_exempts, host, user,
                            desc, atoi(expi), flags, atoi(add), atoi(last));
              return;
            }
          }
        }
      } else {
        desc = strchr(add, ':');
        if (desc) {
          *desc = 0;
          desc++;
          addmask_fully(chan, &chan->exempts, &global_exempts, host, add,
                        desc, atoi(expi), flags, now, 0);
          return;
        }
      }
    }
  }
  putlog(LOG_MISC, "*", "*** Malformed exemptline for %s.",
         chan ? chan->dname : "global_exempts");
}

static void restore_chaninvite(struct chanset_t *chan, char *host)
{
  char *expi, *add, *last, *user, *desc;
  int flags = 0;

  expi = strchr_unescape(host, ':', '\\');
  if (expi) {
    if (*expi == '+') {
      flags |= MASKREC_PERM;
      expi++;
    }
    add = strchr(expi, ':');
    if (add) {
      if (add[-1] == '*') {
        flags |= MASKREC_STICKY;
        add[-1] = 0;
      } else
        *add = 0;
      add++;
      if (*add == '+') {
        last = strchr(add, ':');
        if (last) {
          *last = 0;
          last++;
          user = strchr(last, ':');
          if (user) {
            *user = 0;
            user++;
            desc = strchr(user, ':');
            if (desc) {
              *desc = 0;
              desc++;
              addmask_fully(chan, &chan->invites, &global_invites, host, user,
                            desc, atoi(expi), flags, atoi(add), atoi(last));
              return;
            }
          }
        }
      } else {
        desc = strchr(add, ':');
        if (desc) {
          *desc = 0;
          desc++;
          addmask_fully(chan, &chan->invites, &global_invites, host, add,
                        desc, atoi(expi), flags, now, 0);
          return;
        }
      }
    }
  }
  putlog(LOG_MISC, "*", "*** Malformed inviteline for %s.",
         chan ? chan->dname : "global_invites");
}

static void restore_ignore(char *host)
{
  char *expi, *user, *added, *desc;
  int flags = 0;
  struct igrec *p;

  expi = strchr_unescape(host, ':', '\\');
  if (expi) {
    if (*expi == '+') {
      flags |= IGREC_PERM;
      expi++;
    }
    user = strchr(expi, ':');
    if (user) {
      *user = 0;
      user++;
      added = strchr(user, ':');
      if (added) {
        *added = 0;
        added++;
        desc = strchr(added, ':');
        if (desc) {
          *desc = 0;
          desc++;
        } else
          desc = NULL;
      } else {
        added = "0";
        desc = NULL;
      }
      p = user_malloc(sizeof(struct igrec));

      p->next = global_ign;
      global_ign = p;
      p->expire = atoi(expi);
      p->added = atoi(added);
      p->flags = flags;
      p->igmask = user_malloc(strlen(host) + 1);
      strcpy(p->igmask, host);
      p->user = user_malloc(strlen(user) + 1);
      strcpy(p->user, user);
      if (desc) {
        p->msg = user_malloc(strlen(desc) + 1);
        strcpy(p->msg, desc);
      } else
        p->msg = NULL;
      return;
    }
  }
  putlog(LOG_MISC, "*", "*** Malformed ignore line.");
}

void tell_user(int idx, struct userrec *u, int master)
{
  char s[81], s1[81], format[81];
  int n = 0;
  time_t now2;
  struct chanuserrec *ch;
  struct user_entry *ue;
  struct laston_info *li;
  struct flag_record fr = { FR_GLOBAL, 0, 0, 0, 0, 0 };

  fr.global = u->flags;

  fr.udef_global = u->flags_udef;
  build_flags(s, &fr, NULL);
  if (module_find("notes", 0, 0)) {
    Tcl_SetVar(interp, "_user", u->handle, 0);
    if (Tcl_VarEval(interp, "notes ", "$_user", NULL) == TCL_OK)
      n = tcl_resultint();
  }
  li = get_user(&USERENTRY_LASTON, u);
  if (!li || !li->laston)
    strcpy(s1, "never");
  else {
    now2 = now - li->laston;
    if (now2 >= 86400)
      egg_strftime(s1, 11, "%Y-%m-%d", localtime(&li->laston));
    else
      egg_strftime(s1, 6, "%H:%M", localtime(&li->laston));
  }
  egg_snprintf(format, sizeof format, "%%-%us %%-5s%%5d %%-15s %%s (%%s)\n",
               HANDLEN);
  dprintf(idx, format, u->handle,
          get_user(&USERENTRY_PASS, u) ? "yes" : "no", n, s, s1,
          (li && li->lastonplace) ? li->lastonplace : "nowhere");
  /* channel flags? */
  for (ch = u->chanrec; ch; ch = ch->next) {
    fr.match = FR_CHAN | FR_GLOBAL;
    get_user_flagrec(dcc[idx].user, &fr, ch->channel);
    if (glob_op(fr) || chan_op(fr)) {
      if (ch->laston == 0L)
        strcpy(s1, "never");
      else {
        now2 = now - (ch->laston);
        if (now2 >= 86400)
          egg_strftime(s1, 11, "%Y-%m-%d", localtime(&ch->laston));
        else
          egg_strftime(s1, 6, "%H:%M", localtime(&ch->laston));
      }
      fr.match = FR_CHAN;
      fr.chan = ch->flags;
      fr.udef_chan = ch->flags_udef;
      build_flags(s, &fr, NULL);
      egg_snprintf(format, sizeof format, "%%%us  %%-18s %%-15s %%s\n",
                   HANDLEN - 9);
      dprintf(idx, format, " ", ch->channel, s, s1);
      if (ch->info != NULL)
        dprintf(idx, "    INFO: %s\n", ch->info);
    }
  }
  /* user-defined extra fields */
  for (ue = u->entries; ue; ue = ue->next)
    if (!ue->name && ue->type->display)
      ue->type->display(idx, ue);
}

/* show user by ident */
void tell_user_ident(int idx, char *id, int master)
{
  char format[81];
  struct userrec *u;

  u = get_user_by_handle(userlist, id);
  if (u == NULL)
    u = get_user_by_host(id);
  if (u == NULL) {
    dprintf(idx, "%s.\n", USERF_NOMATCH);
    return;
  }
  egg_snprintf(format, sizeof format,
               "%%-%us PASS NOTES FLAGS           LAST\n", HANDLEN);
  dprintf(idx, format, "HANDLE");
  tell_user(idx, u, master);
}

/* match string:
 * wildcard to match nickname or hostmasks
 * +attr to find all with attr */
void tell_users_match(int idx, char *mtch, int start, int limit,
                      int master, char *chname)
{
  char format[81];
  struct userrec *u;
  int fnd = 0, cnt, nomns = 0, flags = 0;
  struct list_type *q;
  struct flag_record user, pls, mns;

  dprintf(idx, "*** %s '%s':\n", MISC_MATCHING, mtch);
  cnt = 0;
  egg_snprintf(format, sizeof format,
               "%%-%us PASS NOTES FLAGS           LAST\n", HANDLEN);
  dprintf(idx, format, "HANDLE");
  if (start > 1)
    dprintf(idx, "(%s %d)\n", MISC_SKIPPING, start - 1);
  if (strchr("+-&|", *mtch)) {
    user.match = pls.match = FR_GLOBAL | FR_BOT | FR_CHAN;
    break_down_flags(mtch, &pls, &mns);
    mns.match = pls.match ^ (FR_AND | FR_OR);
    if (!mns.global && !mns.udef_global && !mns.chan && !mns.udef_chan &&
        !mns.bot) {
      nomns = 1;
      if (!pls.global && !pls.udef_global && !pls.chan && !pls.udef_chan &&
          !pls.bot) {
        /* happy now BB you weenie :P */
        dprintf(idx, "Unknown flag specified for matching!!\n");
        return;
      }
    }
    if (!chname || !chname[0])
      chname = dcc[idx].u.chat->con_chan;
    flags = 1;
  }

  for (u = userlist; u; u = u->next) {
    if (flags) {
      get_user_flagrec(u, &user, chname);
      if (flagrec_eq(&pls, &user)) {
        if (nomns || !flagrec_eq(&mns, &user)) {
          cnt++;
          if ((cnt <= limit) && (cnt >= start)) {
            tell_user(idx, u, master);
          }
          if (cnt == limit + 1) {
            dprintf(idx, MISC_TRUNCATED, limit);
          }
        }
      }
    } else if (wild_match(mtch, u->handle)) {
      cnt++;
      if ((cnt <= limit) && (cnt >= start)) {
        tell_user(idx, u, master);
      }
      if (cnt == limit + 1) {
        dprintf(idx, MISC_TRUNCATED, limit);
      }
    } else {
      fnd = 0;
      for (q = get_user(&USERENTRY_HOSTS, u); q; q = q->next) {
        if (wild_match(mtch, q->extra) && !fnd) {
          cnt++;
          fnd = 1;
          if ((cnt <= limit) && (cnt >= start)) {
            tell_user(idx, u, master);
          }
          if (cnt == limit + 1) {
            dprintf(idx, MISC_TRUNCATED, limit);
          }
        }
      }
    }
  }

  dprintf(idx, MISC_FOUNDMATCH, cnt, cnt == 1 ? "" : MISC_MATCH_PLURAL);
}

/*
 * tagged lines in the user file:
 * * OLD:
 * #  (comment)
 * ;  (comment)
 * -  hostmask(s)
 * +  email
 * *  dcc directory
 * =  comment
 * :  info line
 * .  xtra (Tcl)
 * !  channel-specific
 * !! global laston
 * :: channel-specific bans
 * NEW:
 * *ban global bans
 * *ignore global ignores
 * ::#chan channel bans
 * - entries in each
 * <handle> begin user entry
 * --KEY INFO - info on each
 * NEWER:
 * % exemptmask(s)
 * @ Invitemask(s)
 * *exempt global exempts
 * *Invite global Invites
 * && channel-specific exempts
 * &&#chan channel exempts
 * $$ channel-specific Invites
 * $$#chan channel Invites
 */

int noxtra = 0;
int readuserfile(char *file, struct userrec **ret)
{
  char *p, buf[512], lasthand[512], *attr, *pass, *code, s1[512], *s;
  FILE *f;
  struct userrec *bu, *u = NULL;
  struct chanset_t *cst = NULL;
  int i;
  char ignored[LOGLINEMAX]; /* putlog() will truncate anything larger anyway */
  struct flag_record fr;
  struct chanuserrec *cr;

  bu = (*ret);
  ignored[0] = 0;
  if (bu == userlist) {
    clear_chanlist();
    lastuser = NULL;
    global_bans = NULL;
    global_ign = NULL;
    global_exempts = NULL;
    global_invites = NULL;
  }
  lasthand[0] = 0;
  f = fopen(file, "r");
  if (f == NULL)
    return 0;
  noshare = noxtra = 1;
  /* read opening comment */
  s = buf;
  fgets(s, 180, f);
  if (s[1] < '4') {
    fatal(USERF_OLDFMT, 0);
  }
  if (s[1] > '4')
    fatal(USERF_INVALID, 0);
  while (!feof(f)) {
    s = buf;
    fgets(s, 511, f);
    if (!feof(f)) {
      if (s[0] != '#' && s[0] != ';' && s[0]) {
        code = newsplit(&s);
        rmspace(s);
        if (!strcmp(code, "-")) {
          if (!lasthand[0])
            continue;           /* Skip this entry.     */
          if (u) {              /* only break it down if there a real users */
            p = strchr(s, ',');
            while (p != NULL) {
              splitc(s1, s, ',');
              rmspace(s1);
              if (s1[0])
                set_user(&USERENTRY_HOSTS, u, s1);
              p = strchr(s, ',');
            }
          }
          /* channel bans are never stacked with , */
          if (s[0]) {
            if (lasthand[0] && strchr(CHANMETA, lasthand[0]) != NULL)
              restore_chanban(cst, s);
            else if (lasthand[0] == '*') {
              if (lasthand[1] == 'i')
                restore_ignore(s);
              else
                restore_chanban(NULL, s);
            } else if (lasthand[0])
              set_user(&USERENTRY_HOSTS, u, s);
          }
        } else if (!strcmp(code, "%")) {  /* exemptmasks */
          if (!lasthand[0])
            continue;           /* Skip this entry.     */
          if (s[0]) {
            if (lasthand[0] == '#' || lasthand[0] == '+')
              restore_chanexempt(cst, s);
            else if (lasthand[0] == '*')
              if (lasthand[1] == 'e')
                restore_chanexempt(NULL, s);
          }
        } else if (!strcmp(code, "@")) {  /* Invitemasks */
          if (!lasthand[0])
            continue;           /* Skip this entry.     */
          if (s[0]) {
            if (lasthand[0] == '#' || lasthand[0] == '+')
              restore_chaninvite(cst, s);
            else if (lasthand[0] == '*')
              if (lasthand[1] == 'I')
                restore_chaninvite(NULL, s);
          }
        } else if (!strcmp(code, "!")) {
          /* ! #chan laston flags [info] */
          char *chname, *st, *fl;

          if (u) {
            chname = newsplit(&s);
            st = newsplit(&s);
            fl = newsplit(&s);
            rmspace(s);
            fr.match = FR_CHAN;
            break_down_flags(fl, &fr, 0);
            if (findchan_by_dname(chname)) {
              for (cr = u->chanrec; cr; cr = cr->next)
                if (!rfc_casecmp(cr->channel, chname))
                  break;
              if (!cr) {
                cr = (struct chanuserrec *)
                  user_malloc(sizeof(struct chanuserrec));

                cr->next = u->chanrec;
                u->chanrec = cr;
                strncpyz(cr->channel, chname, 80);
                cr->laston = atoi(st);
                cr->flags = fr.chan;
                cr->flags_udef = fr.udef_chan;
                if (s[0]) {
                  cr->info = (char *) user_malloc(strlen(s) + 1);
                  strcpy(cr->info, s);
                } else
                  cr->info = NULL;
              }
            }
          }
        } else if (!strncmp(code, "::", 2)) {
          /* channel-specific bans */
          strcpy(lasthand, &code[2]);
          u = NULL;
          if (!findchan_by_dname(lasthand)) {
            strcpy(s1, lasthand);
            strcat(s1, " ");
            if (strstr(ignored, s1) == NULL) {
              strncat(ignored, lasthand,
                      sizeof(ignored) - 1 - strlen(ignored));
              strncat(ignored, " ",
                      sizeof(ignored) - 1 - strlen(ignored));
            }
            lasthand[0] = 0;
          } else {
            /* Remove all bans for this channel to avoid dupes */
            /* NOTE only remove bans for when getting a userfile
             * from another bot & that channel is shared */
            cst = findchan_by_dname(lasthand);
            if ((*ret == userlist) || channel_shared(cst)) {
              clear_masks(cst->bans);
              cst->bans = NULL;
            } else {
              /* otherwise ignore any bans for this channel */
              cst = NULL;
              lasthand[0] = 0;
            }
          }
        } else if (!strncmp(code, "&&", 2)) {
          /* channel-specific exempts */
          strcpy(lasthand, &code[2]);
          u = NULL;
          if (!findchan_by_dname(lasthand)) {
            strcpy(s1, lasthand);
            strcat(s1, " ");
            if (strstr(ignored, s1) == NULL) {
              strncat(ignored, lasthand,
                      sizeof(ignored) - 1 - strlen(ignored));
              strncat(ignored, " ",
                      sizeof(ignored) - 1 - strlen(ignored));
            }
            lasthand[0] = 0;
          } else {
            /* Remove all exempts for this channel to avoid dupes */
            /* NOTE only remove exempts for when getting a userfile
             * from another bot & that channel is shared */
            cst = findchan_by_dname(lasthand);
            if ((*ret == userlist) || channel_shared(cst)) {
              clear_masks(cst->exempts);
              cst->exempts = NULL;
            } else {
              /* otherwise ignore any exempts for this channel */
              cst = NULL;
              lasthand[0] = 0;
            }
          }
        } else if (!strncmp(code, "$$", 2)) {
          /* channel-specific invites */
          strcpy(lasthand, &code[2]);
          u = NULL;
          if (!findchan_by_dname(lasthand)) {
            strcpy(s1, lasthand);
            strcat(s1, " ");
            if (strstr(ignored, s1) == NULL) {
              strncat(ignored, lasthand,
                      sizeof(ignored) - 1 - strlen(ignored));
              strncat(ignored, " ",
                      sizeof(ignored) - 1 - strlen(ignored));
            }
            lasthand[0] = 0;
          } else {
            /* Remove all invites for this channel to avoid dupes */
            /* NOTE only remove invites for when getting a userfile
             * from another bot & that channel is shared */
            cst = findchan_by_dname(lasthand);
            if ((*ret == userlist) || channel_shared(cst)) {
              clear_masks(cst->invites);
              cst->invites = NULL;
            } else {
              /* otherwise ignore any invites for this channel */
              cst = NULL;
              lasthand[0] = 0;
            }
          }
        } else if (!strncmp(code, "--", 2)) {
          if (u) {
            /* new format storage */
            struct user_entry *ue;
            int ok = 0;

            for (ue = u->entries; ue && !ok; ue = ue->next)
              if (ue->name && !egg_strcasecmp(code + 2, ue->name)) {
                struct list_type *list;

                list = user_malloc(sizeof(struct list_type));

                list->next = NULL;
                list->extra = user_malloc(strlen(s) + 1);
                strcpy(list->extra, s);
                egg_list_append((&ue->u.list), list);
                ok = 1;
              }
            if (!ok) {
              ue = user_malloc(sizeof(struct user_entry));

              ue->name = user_malloc(strlen(code + 1));
              ue->type = NULL;
              strcpy(ue->name, code + 2);
              ue->u.list = user_malloc(sizeof(struct list_type));

              ue->u.list->next = NULL;
              ue->u.list->extra = user_malloc(strlen(s) + 1);
              strcpy(ue->u.list->extra, s);
              list_insert((&u->entries), ue);
            }
          }
        } else if (!rfc_casecmp(code, BAN_NAME)) {
          strcpy(lasthand, code);
          u = NULL;
        } else if (!rfc_casecmp(code, IGNORE_NAME)) {
          strcpy(lasthand, code);
          u = NULL;
        } else if (!rfc_casecmp(code, EXEMPT_NAME)) {
          strcpy(lasthand, code);
          u = NULL;
        } else if (!rfc_casecmp(code, INVITE_NAME)) {
          strcpy(lasthand, code);
          u = NULL;
        } else if (code[0] == '*') {
          lasthand[0] = 0;
          u = NULL;
        } else {
          pass = newsplit(&s);
          attr = newsplit(&s);
          rmspace(s);
          if (!attr[0] || !pass[0]) {
            putlog(LOG_MISC, "*", "* %s '%s'!", USERF_CORRUPT, code);
            lasthand[0] = 0;
          } else {
            u = get_user_by_handle(bu, code);
            if (u && !(u->flags & USER_UNSHARED)) {
              putlog(LOG_MISC, "*", "* %s '%s'!", USERF_DUPE, code);
              lasthand[0] = 0;
              u = NULL;
            } else if (u) {
              lasthand[0] = 0;
              u = NULL;
            } else {
              fr.match = FR_GLOBAL;
              break_down_flags(attr, &fr, 0);
              strcpy(lasthand, code);
              cst = NULL;
              if (strlen(code) > HANDLEN)
                code[HANDLEN] = 0;
              if (strlen(pass) > 20) {
                putlog(LOG_MISC, "*", "* %s '%s'", USERF_BROKEPASS, code);
                strcpy(pass, "-");
              }
              bu = adduser(bu, code, 0, pass,
                           sanity_check(fr.global &USER_VALID));

              u = get_user_by_handle(bu, code);
              for (i = 0; i < dcc_total; i++)
                if (!egg_strcasecmp(code, dcc[i].nick))
                  dcc[i].user = u;
              u->flags_udef = fr.udef_global;
              /* if s starts with '/' it's got file info */
            }
          }
        }
      }
    }
  }
  fclose(f);
  (*ret) = bu;
  if (ignored[0]) {
    putlog(LOG_MISC, "*", "%s %s", USERF_IGNBANS, ignored);
  }
  putlog(LOG_MISC, "*", "Userfile loaded, unpacking...");
  for (u = bu; u; u = u->next) {
    struct user_entry *e;

    if (!(u->flags & USER_BOT) && !egg_strcasecmp(u->handle, botnetnick)) {
      putlog(LOG_MISC, "*", "(!) I have a user record, but without +b");
      /* u->flags |= USER_BOT; */
    }

    for (e = u->entries; e; e = e->next)
      if (e->name) {
        struct user_entry_type *uet = find_entry_type(e->name);

        if (uet) {
          e->type = uet;
          uet->unpack(u, e);
          nfree(e->name);
          e->name = NULL;
        }
      }
  }
  noshare = noxtra = 0;
  /* process the user data *now* */
  return 1;
}

/* New methodology - cycle through list 3 times
 * 1st time scan for +sh bots and link if none connected
 * 2nd time scan for +h bots
 * 3rd time scan for +a/+h bots */
void autolink_cycle(char *start)
{
  struct userrec *u = userlist, *autc = NULL;
  static int cycle = 0;
  int got_hub = 0, got_alt = 0, got_shared = 0, linked, ready = 0, i, bfl;

  /* don't start a new cycle if some links are still pending */
  if (!start) {
    for (i = 0; i < dcc_total; i++) {
      if (dcc[i].type == &DCC_BOT_NEW)
        return;
      if (dcc[i].type == &DCC_FORK_BOT)
        return;
      if ((dcc[i].type == &DCC_DNSWAIT) &&
          (dcc[i].u.dns && (dcc[i].u.dns->type == &DCC_FORK_BOT)))
        return;
    }
  }
  if (!start) {
    ready = 1;
    cycle = 0;
  }                             /* new run through the user list */
  while (u && !autc) {
    while (u && !autc) {
      if (u->flags & USER_BOT && strcmp(u->handle, botnetnick)) {              /* ignore our own user record */
        bfl = bot_flags(u);
        if (bfl & (BOT_HUB | BOT_ALT)) {
          linked = 0;
          for (i = 0; i < dcc_total; i++) {
            if (dcc[i].user == u) {
              if (dcc[i].type == &DCC_BOT)
                linked = 1;
              if (dcc[i].type == &DCC_BOT_NEW)
                linked = 1;
              if (dcc[i].type == &DCC_FORK_BOT)
                linked = 1;
            }
          }
          if ((bfl & BOT_HUB) && (bfl & BOT_SHARE)) {
            if (linked)
              got_shared = 1;
            else if (!cycle && ready && !autc)
              autc = u;
          } else if ((bfl & BOT_HUB) && cycle > 0) {
            if (linked)
              got_hub = 1;
            else if ((cycle == 1) && ready && !autc)
              autc = u;
          } else if ((bfl & BOT_ALT) && (cycle == 2)) {
            if (linked)
              got_alt = 1;
            else if (!in_chain(u->handle) && ready && !autc)
              autc = u;
          }
          /* did we make it where we're supposed to start?  yay! */
          if (!ready)
            if (!egg_strcasecmp(u->handle, start)) {
              ready = 1;
              autc = NULL;
              /* if starting point is a +h bot, must be in 2nd cycle */
              if ((bfl & BOT_HUB) && !(bfl & BOT_SHARE)) {
                cycle = 1;
              }
              /* if starting point is a +a bot, must be in 3rd cycle */
              if (bfl & BOT_ALT) {
                cycle = 2;
              }
            }
        }
        if (!cycle && (bfl & BOT_REJECT) && in_chain(u->handle)) {
          /* get rid of nasty reject bot */
          int i;

          i = nextbot(u->handle);
          if ((i >= 0) && !egg_strcasecmp(dcc[i].nick, u->handle)) {
            char *p = MISC_REJECTED;

            /* we're directly connected to the offending bot?! (shudder!) */
            putlog(LOG_BOTS, "*", "%s %s", BOT_REJECTING, dcc[i].nick);
            chatout("*** %s bot %s\n", p, dcc[i].nick);
            botnet_send_unlinked(i, dcc[i].nick, p);
            dprintf(i, "bye %s\n", BOT_REJECTING);
            killsock(dcc[i].sock);
            lostdcc(i);
          } else if ((i < 0) && egg_strcasecmp(botnetnick, u->handle)) {
            /* The bot is not connected, but listed in our tandem list! */
            putlog(LOG_BOTS, "*", "(!) BUG: rejecting not connected bot %s!",
                   u->handle);
            rembot(u->handle);
          }
        }
      }
      u = u->next;
    }
    if (!autc) {
      if (!cycle && !got_shared) {
        cycle++;
        u = userlist;
      } else if ((cycle == 1) && !(got_shared || got_hub)) {
        cycle++;
        u = userlist;
      }
    }
  }
  if (got_shared && !cycle)
    autc = NULL;
  else if ((got_shared || got_hub) && (cycle == 1))
    autc = NULL;
  else if ((got_shared || got_hub || got_alt) && (cycle == 2))
    autc = NULL;
  if (autc)
    botlink("", -3, autc->handle);      /* try autoconnect */
}
