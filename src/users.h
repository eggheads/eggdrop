/* structures and definitions used by users.c & userrec.c */
/*
 * This file is part of the eggdrop source code
 * copyright (c) 1997 Robey Pointer
 * and is distributed according to the GNU general public license.
 * For full details, read the top of 'main.c' or the file called
 * COPYING that was distributed with this code.
 */

#ifndef _H_USERS
#define _H_USERS

/* list functions :) , next *must* be the 1st item in the struct */
struct list_type {
  struct list_type *next;
  char *extra;
};

#define list_insert(a,b) { b->next = *a; *a = b; }
int list_append(struct list_type **, struct list_type *);
int list_delete(struct list_type **, struct list_type *);
int list_contains(struct list_type *, struct list_type *);

/* new userfile format stuff */
struct userrec;
struct user_entry;
struct user_entry_type {
  struct user_entry_type *next;
  int (*got_share) (struct userrec *, struct user_entry *, char *, int);
  int (*dup_user) (struct userrec *, struct userrec *,
		   struct user_entry *);
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
extern struct user_entry_type USERENTRY_EMAIL, USERENTRY_COMMENT, USERENTRY_LASTON,
 USERENTRY_XTRA, USERENTRY_INFO, USERENTRY_BOTADDR, USERENTRY_HOSTS,
 USERENTRY_PASS, USERENTRY_BOTFL, USERENTRY_URL;

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

void *_user_malloc(int, char *, int);
void *_user_realloc(void *, int, char *, int);

#ifndef MAKING_MODS
#define user_malloc(x) _user_malloc(x,__FILE__,__LINE__)
#define user_realloc(x,y) _user_realloc(x,y,__FILE__,__LINE__)
#endif
int add_entry_type(struct user_entry_type *);
int del_entry_type(struct user_entry_type *);
struct user_entry_type *find_entry_type(char *);
struct user_entry *find_user_entry(struct user_entry_type *, struct userrec *);
void *get_user(struct user_entry_type *, struct userrec *);
int set_user(struct user_entry_type *, struct userrec *, void *);

#define bot_flags(u) ((long)get_user(&USERENTRY_BOTFL,u))
#define is_bot(u) (u && (u->flags & USER_BOT))
#define is_master(u) (u && (u->flags & USER_MASTER))
#define is_owner(u) (u && (u->flags & USER_OWNER))

/* fake users used to store ignores and bans */
#define IGNORE_NAME "*ignore"
#define BAN_NAME    "*ban"
#define EXEMPT_NAME "*exempt"
#define INVITE_NAME "*Invite"

/* channel-specific info */
struct chanuserrec {
  struct chanuserrec *next;
  char channel[81];
  time_t laston;
  unsigned long flags;
  unsigned long flags_udef;
  char *info;
};

/* new-style userlist */
struct userrec {
  struct userrec *next;
  char handle[HANDLEN + 1];
  unsigned long flags;
  unsigned long flags_udef;
  struct chanuserrec *chanrec;
  struct user_entry *entries;
};

/* let's get neat, a struct for each */
struct banrec {
  struct banrec *next;
  char *banmask;
  time_t expire;
  time_t added;
  time_t lastactive;
  char *user;
  char *desc;
  int flags;
};
extern struct banrec *global_bans;

#define BANREC_STICKY 1
#define BANREC_PERM   2

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

struct exemptrec {
   struct exemptrec * next;
   char * exemptmask;
   time_t expire;
   time_t added;
   time_t lastactive;
   char * user;
   char * desc;  
   int flags;
};
extern struct exemptrec * global_exempts;
   
#define EXEMPTREC_STICKY 1
#define EXEMPTREC_PERM   2

struct inviterec {
   struct inviterec * next;
   char * invitemask;
   time_t expire;
   time_t added;
   time_t lastactive;
   char * user;
   char * desc;
   int flags;
};
extern struct inviterec * global_invites;
   
#define INVITEREC_STICKY 1
#define INVITEREC_PERM   2

/* flags are in eggdrop.h */

struct userrec *adduser();
struct userrec *get_user_by_handle(struct userrec *, char *);
struct userrec *get_user_by_host(char *);
struct userrec *get_user_by_nick(char *);
struct userrec *check_chanlist();
struct userrec *check_chanlist_hand();

/* all the default userentry stuff, for code re-use */
int def_unpack(struct userrec *u, struct user_entry *e);
int def_pack(struct userrec *u, struct user_entry *e);
int def_kill(struct user_entry *e);
int def_write_userfile(FILE * f, struct userrec *u, struct user_entry *e);
void *def_get(struct userrec *u, struct user_entry *e);
int def_set(struct userrec *u, struct user_entry *e, void *buf);
int def_gotshare(struct userrec *u, struct user_entry *e,
		 char *data, int idx);
int def_tcl_get(Tcl_Interp * interp, struct userrec *u,
		struct user_entry *e, int argc, char **argv);
int def_tcl_set(Tcl_Interp * irp, struct userrec *u,
		struct user_entry *e, int argc, char **argv);
int def_expmem(struct user_entry *e);
void def_display(int idx, struct user_entry *e);
int def_dupuser(struct userrec *new, struct userrec *old,
		struct user_entry *e);

#endif
