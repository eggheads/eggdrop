/*
 * misc_file.h
 *   prototypes for misc_file.c
 */
/*
 * Copyright (C) 2000 - 2019 Eggheads Development Team
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

#ifndef _EGG_MISC_FILE_H
#define _EGG_MISC_FILE_H

int copyfile(char *, char *);
int copyfilef(char *, FILE *);
int fcopyfile(FILE *, char *);
int movefile(char *, char *);
int file_readable(char *);

#endif /* _EGG_MISC_FILE_H */
