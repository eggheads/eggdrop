
#ifndef NOTES_H
#define NOTES_H

#define NOTES_IGNKEY "NOTESIGNORE"

#ifdef MAKING_NOTES
static int get_note_ignores(struct userrec *, char ***);
static int add_note_ignore(struct userrec *, char *);
static int del_note_ignore(struct userrec *, char *);
static int match_note_ignore(struct userrec *, char *);
static void notes_read(char *, char *, char *, int);
static void notes_del(char *, char *, char *, int);
static void fwd_display(int, struct user_entry *);
#endif

#endif /* NOTES_H */
