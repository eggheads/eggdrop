/*
 * twitch.h -- part of twitch.mod
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

typedef struct userstate {
  char channel[CHANNELLEN + 1];
  char handle[HANDLEN + 1];
  int badge_info;
  char badges[8191];
  char color[8];
  char display_name[8191];
  char emote_sets[8191];
  int mod;
} userstate_t;

typedef struct twitchchan {
  struct twitchchan *next;
  char dname[CHANNELLEN + 1]; /* display name (!foo) - THIS IS ALWAYS SET */
  char name[CHANNELLEN + 1];  /* actual name (!BARfoo) - THIS IS SET WHEN THE BOT
                               * ACTUALLY JOINS THE CHANNEL */
  userstate_t userstate;
  char mods[8191];
  char vips[8191];
  unsigned int emote_only:1;
  unsigned int subs_only:1;
  unsigned int r9k:1;
  int followers_only;
  int slow;
} twitchchan_t;
