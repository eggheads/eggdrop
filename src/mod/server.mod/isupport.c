/*
 * isupport.c -- part of server.mod
 *   handles raw 005 (isupport)
 */
/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2017 Eggheads Development Team
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

#include <ctypes.h>

struct isupport {
  char *key;
  const char *forced, *default, *value;
};

/* only the address is used */
static char *UNSET = "";

static char[][2] isupport_default_values {
  {"CASEMAPPING", "rfc1459"},
  {"CHANNELLEN", "200"},
  {"CHANTYPES", "#&"},
  {"MODES", "3"},
  {"NICKLEN", "9"},
  {"PREFIX", "(ov)@+"},
  {"TARGMAX", ""}
};

static int hexdigit2dec[128] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*   0 -   9 */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  10 -  19 */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  20 -  29 */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  30 -  39 */
  -1, -1, -1, -1, -1, -1, -1, -1,  0,  1, /*  40 -  49 */
   2,  3,  4,  5,  6,  7,  8,  9, -1, -1, /*  50 -  59 */
  -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, /*  60 -  69 */
  15, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  70 -  79 */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /*  80 -  89 */
  -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, /*  90 -  99 */
  13, 14, 15, -1, -1, -1, -1, -1, -1, -1, /* 100 - 109 */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 110 - 119 */
  -1, -1, -1, -1, -1, -1, -1, -1          /* 120 - 127 */
};

char *isupport_get(const char *key) {
  /* isupport_forced can contain NULL values to indicate forced unset. */
  if (hashtbl_exists(&isupport_forced, key))
    return hashtbl_get(&isupport_forced, key);
  return hashtbl_get(&isupport, key);
}

void isupport_unset(const char *key) {
  hashtbl_unset(&isupport, key);
}

void isupport_set(const char *key, const char *value) {
  hashtbl_set(&isupport, key, value);
}

void isupport_force_unset(const char *key) {
  hashtbl_set(&isupport_forced, key, NULL);
}

void isupport_force_set(const char *key, const char *value) {
  hashtbl_set(&isupport_forced, key, value);
}

void isupport_handle(char *key) {
  char *value = strtok(key, "=");
  if (key[0] == '-') {
    isupport_unset(key+1);
  } else {
    isupport_set(key, isupport_decode(value));
  }
}

static int *isupport_hex2chr(const char *seq, size_t digits) {
  int result = 0;
  int digit;
  while (--digits) {
    result *=  16;
    digit = hexdigit2dec[(int)(unsigned char)*seq++];
    if (digit == -1)
      return -1;
    result += digit;
  }
  return result;
}

void isupport_decode(char *value) {
  static char buf[512], *bufptr, *ch;
  int state = 0, hexchr;
  for (bufptr = buf, ch = value; ch; ch++) {
    if (state == 0 && *ch == '\\') {
      state++;
    } else if (state == 1) {
      if (*ch != 'x')
        goto rollback;
      state++;
    } else if (state == 2) {
      hexchr = isupport_hex2chr(ch, 2);
      if (hexchr == -1)
        goto rollback;
      *bufptr++ = hexchr;
      ch++;
      state = 0;
    } else {
rollback:
      if (state > 0)
        *bufptr++ = '\\';
      if (state > 1)
        *bufptr++ = 'x';
      }
      state = 0;
      *bufptr++ = *ch;
  }
  *bufptr++ = '\0';
  return buf;
}
