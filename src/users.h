/*
 * users.h
 *   structures and definitions used by users.c and userrec.c
 *
 * $Id: users.h,v 1.21 2011/02/13 14:19:33 simple Exp $
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

#ifndef _EGG_USERS_H
#define _EGG_USERS_H

/* List functions :) , next *must* be the 1st item in the struct */
struct list_type {
  struct list_type *next;
  char *extra;
};

#define list_insert(a,b) {                                              \
        (b)->next = *(a);                                               \
        *(a) = (b);                                                     \
}
int egg_list_append(struct list_type **, struct list_type *);
int egg_list_delete(struct list_type **, struct list_type *);
int egg_list_contains(struct list_type *, struct list_type *);


/* New userfile format stuff
 */
struct userrec;
struct user_entry;
struct user_entry_type {
  struct user_entry_type *next;
  int (*got_share) (struct userrec *, struct user_entry *, char *, int);
  int (*dup_user) (struct userrec *, struct userrec *, struct user_entry *);
  int (*unpack) (struct userrec *, struct user_entry *);
  int (*pack) (struct userrec *, struct user_entry *);
  int (*write_userfile) (FILE *, struct userrec *, struct user_entry *);
  int (*kill) (struct user_entry *);
  void *(*get) (struct userrec *, struct user_entry *);
  int (*set) (struct userrec *, struct user_entry *, void *);
  int (*tcl_get) (Tcl_Interp *, struct userrec *, struct user_entry *,
                  int, char **);
  int (*tcl_set) (Tcl_Interp *, struct userrec *, struct user_entry *,
                  int, char **);
  int (*expmem) (struct user_entry *);
  void (*display) (int idx, struct user_entry *);
  char *name;
};


#ifndef MAKING_MODS
extern struct user_entry_type USERENTRY_COMMENT, USERENTRY_LASTON,
  USERENTRY_XTRA, USERENTRY_INFO, USERENTRY_BOTADDR, USERENTRY_HOSTS,
  USERENTRY_PASS, USERENTRY_BOTFL;
#endif


struct laston_info {
  time_t laston;
  char *lastonplace;
};

struct bot_addr {
  int telnet_port;
  int relay_port;
  char *address;
};

struct user_entry {
  struct user_entry *next;
  struct user_entry_type *type;
  union {
    char *string;
    void *extra;
    struct list_type *list;
    unsigned long ulong;
  } u;
  char *name;
};

struct xtra_key {
  struct xtra_key *next;
  char *key;
  char *data;
};

struct filesys_stats {
  int uploads;
  int upload_ks;
  int dnloads;
  int dnload_ks;
};

void *_user_malloc(int size, const char *file, int line);
void *_user_realloc(void *ptr, int size, const char *file, int line);

#ifndef MAKING_MODS
#  define user_malloc(x)     _user_malloc(x, __FILE__, __LINE__)
#  define user_realloc(x, y) _user_realloc(x, y, __FILE__, __LINE__)
#endif

int add_entry_type(struct user_entry_type *);
int del_entry_type(struct user_entry_type *);
struct user_entry_type *find_entry_type(char *);
struct user_entry *find_user_entry(struct user_entry_type *, struct userrec *);
void *get_user(struct user_entry_type *, struct userrec *);
int set_user(struct user_entry_type *, struct userrec *, void *);

#define bot_flags(u) ((long)get_user(&USERENTRY_BOTFL, (u)))
#define is_bot(u)    ((u) && ((u)->flags & USER_BOT))
#define is_owner(u)  ((u) && ((u)->flags & USER_OWNER))

/* Fake users used to store ignores and bans
 */
#define IGNORE_NAME "*ignore"
#define BAN_NAME    "*ban"
#define EXEMPT_NAME "*exempt"
#define INVITE_NAME "*Invite"

/* Channel-specific info
 */
struct chanuserrec {
  struct chanuserrec *next;
  char channel[81];
  time_t laston;
  unsigned long flags;
  unsigned long flags_udef;
  char *info;
};

/* New-style userlist
 */
struct userrec {
  struct userrec *next;
  char handle[HANDLEN + 1];
  unsigned long flags;
  unsigned long flags_udef;
  struct chanuserrec *chanrec;
  struct user_entry *entries;
};

struct igrec {
  struct igrec *next;
  char *igmask;
  time_t expire;
  char *user;
  time_t added;
  char *msg;
  int flags;
};
extern struct igrec *global_ign;

#define IGREC_PERM   2

/*
 * Note: Flags are in eggdrop.h
 */

struct userrec *adduser();
struct userrec *get_user_by_handle(struct userrec *, char *);
struct userrec *get_user_by_host(char *);
struct userrec *get_user_by_nick(char *);
struct userrec *check_chanlist();
struct userrec *check_chanlist_hand();

/* All the default userentry stuff, for code re-use
 */
int def_unpack(struct userrec *u, struct user_entry *e);
int def_pack(struct userrec *u, struct user_entry *e);
int def_kill(struct user_entry *e);
int def_write_userfile(FILE *f, struct userrec *u, struct user_entry *e);
void *def_get(struct userrec *u, struct user_entry *e);
int def_set(struct userrec *u, struct user_entry *e, void *buf);
int def_gotshare(struct userrec *u, struct user_entry *e, char *data, int idx);
int def_tcl_get(Tcl_Interp *interp, struct userrec *u,
                struct user_entry *e, int argc, char **argv);
int def_tcl_set(Tcl_Interp *irp, struct userrec *u,
                struct user_entry *e, int argc, char **argv);
int def_expmem(struct user_entry *e);
void def_display(int idx, struct user_entry *e);
int def_dupuser(struct userrec *new, struct userrec *old, struct user_entry *e);

#endif /* _EGG_USERS_H */
