/* 
 * notes.h -- part of notes.mod
 * 
 * $Id: notes.h,v 1.4 2000/01/08 21:23:16 per Exp $
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

#ifndef _EGG_MOD_NOTES_NOTES_H
#define _EGG_MOD_NOTES_NOTES_H

#define NOTES_IGNKEY "NOTESIGNORE"

#ifdef MAKING_NOTES
static int get_note_ignores(struct userrec *, char ***);
static int add_note_ignore(struct userrec *, char *);
static int del_note_ignore(struct userrec *, char *);
static int match_note_ignore(struct userrec *, char *);
static void notes_read(char *, char *, char *, int);
static void notes_del(char *, char *, char *, int);
static void fwd_display(int, struct user_entry *);

#endif				/* MAKING_NOTES */

#endif				/* _EGG_MOD_NOTES_H */
