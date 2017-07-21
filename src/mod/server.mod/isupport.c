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

#include <ctype.h>

#define ICLEAR(dst) do { if ((dst)) { nfree((dst)); (dst) = NULL; } } while (0)
#define ISET(dst, val, len) do { if ((dst)) { nfree((dst)); }; \
  (dst) = valuendup((val), ((len) < 0 ? strlen((val)) : (len))); } while (0);

struct isupport {
  size_t keylen;
  const char *key;
  const char *forced, *value, *def;
  int forced_unset;
};

/* Instantiate list type for isupport data */
CREATE_LIST_TYPE(isupportdata, isupportkeydata, struct isupport *);

static struct isupportdata *isupportdata;

static char *isupport_default = "CASEMAPPING=rfc1459 CHANNELLEN=200 CHANTYPES=#& MODES=3 NICKLEN=9 PREFIX=(ov)@+ TARGMAX";

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

static char *valuendup(const char *value, size_t len) {
  char *str = nmalloc(len + 1);
  memcpy(str, value, len);
  str[len] = '\0';
  return str;
} 

static int keyupeq(const char *key, const char *upkey) {
  int i;
  for (i = 0; key[i] && upkey[i]; i++)
    if (toupper((unsigned char)key[i]) != upkey[i])
      return 0;
  return (key[i] == upkey[i]);
}

static char *keyupndup(const char *key, size_t len) {
  int i;
  char *up = nmalloc(len + 1);
  for (i = 0; i < len; i++)
    up[i] = toupper((unsigned char)key[i]);
  up[i] = '\0';
  return up;
}

static struct isupport *add_record(const char *key, size_t keylen) {
  struct isupport *data = nmalloc(sizeof *data);

  data->key = keyupndup(key, keylen);
  data->keylen = keylen;
  data->forced_unset = 0;
  data->forced = data->def = data->value = NULL;
  isupportdata_append(isupportdata, data);
  return data;
}

static struct isupport *find_record(const char *key, size_t keylen) {
  struct isupportkeydata *i;
  struct isupport *data;

  LIST_FOREACH_DATA(isupportdata, i, data) {
    if (keylen == data->keylen && keyupeq(data->key, key))
      return data;
  }
  return NULL;
}

static struct isupport *get_record(const char *key, size_t keylen) {
  struct isupport *data = find_record(key, keylen);

  if (!data)
    data = add_record(key, keylen);

  return data;
}

const char *isupport_get_from(struct isupport *data) {
  if (data->forced_unset)
    return NULL;
  if (data->forced)
    return data->forced;
  if (data->value)
    return data->value;
  if (data->def)
    return data->def;
  return NULL;
}

/* Get the value of an isupport key, following overrides and defs. */
const char *isupport_get(const char *key, size_t keylen) {
  struct isupport *data = find_record(key, keylen);

  if (!data)
    return NULL;

  return isupport_get_from(data);
}

/* Clear the isupport setting from the server. */
static void isupport_unset(const char *key, size_t keylen) {
  struct isupport *data = find_record(key, keylen);

  if (!data) {
    putlog(LOG_MISC, "*", "isupport key %s not found, but tried to unset!", key);
    return;
  }

  ICLEAR(data->value);
}

/* Set the isupport setting from the server. */
static void isupport_set(const char *key, size_t keylen,
    const char *value, size_t len)
{
  struct isupport *data = get_record(key, keylen);

  ISET(data->value, value, len);
}

static void isupport_clear(void) {
  struct isupportkeydata *i;
  struct isupport *data;

  LIST_FOREACH_DATA(isupportdata, i, data) {
    ICLEAR(data->value);
  }
}

static void isupport_unset_forced(const char *key, size_t keylen) {
  struct isupport *data = get_record(key, keylen);

  ICLEAR(data->forced);
  data->forced_unset = 1;
}

static void isupport_set_forced(const char *key, size_t keylen,
    const char *value, size_t len)
{
  struct isupport *data = get_record(key, keylen);

  ISET(data->forced, value, len);
  data->forced_unset = 0;
}

static void isupport_clear_forced(void) {
  struct isupportkeydata *i;
  struct isupport *data;

  LIST_FOREACH_DATA(isupportdata, i, data) {
    ICLEAR(data->forced);
    data->forced_unset = 0;
  }
}

static void isupport_set_default(const char *key, size_t keylen,
    const char *value, size_t len)
{
  struct isupport *data = get_record(key, keylen);

  ISET(data->def, value, len);
}

static void isupport_clear_default(void) {
  struct isupportkeydata *i;
  struct isupport *data;

  LIST_FOREACH_DATA(isupportdata, i, data) {
    ICLEAR(data->def);
  }
}

static int isupport_hex2chr(const char *seq, size_t digits) {
  int result = 0;
  int digit;
  while (--digits) {
    result *=  16;
    digit = hexdigit2dec[(int)(unsigned char)*seq++];
    if (digit == -1)
      return -1;
    result += digit;
  }
  /* disallow \x00 */
  return result ? result : -1;
}

static const char *isupport_encode(const char *value) {
  static char buf[512];
  int i, j;

  for (i = j = 0; i < strlen(value) && j < sizeof buf - strlen("\\xHH") - 1; i++) {
    if ((unsigned char)value[i] < 33 || (unsigned char)value[i] > 126)
      j += sprintf(buf + j, "\\x%02hhx", (unsigned char)value[i]);
    else
      buf[j++] = value[i];
  }
  buf[j] = '\0';
  return buf;
}

static size_t isupport_decode(char *buf, size_t bufsize,
    const char *value, size_t len)
{
  const char *ch;
  char *bufptr = buf;
  int state = 0, hexchr;

  for (ch = value; (ch - value) < len && (bufptr - buf) < bufsize - 1; ch++) {
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
  *bufptr = '\0';
  return (size_t)(bufptr - buf);
}

static void isupport_parse(const char *str,
    void (*set_cb)(const char *key, size_t keylen, const char *v, size_t vlen),
    void (*unset_cb)(const char *key, size_t keylen))
{
  static char buf[512];
  size_t keylen, valuelen;
  const char *key, *value;

  while (1) {
    while (*str == ' ')
      str++;
    /* :are supported by this server */
    if (!*str || *str == ':')
      break;
    key = str;
    keylen = strcspn(key, "= ");
    if (key[0] == '-') {
      if (key[keylen] == '=') {
        putlog(LOG_MISC, "*", "isupport key %.*s is unset but has a value!", keylen, key);
      }
      unset_cb(key, keylen);
      str = key + keylen;
    } else {
      value = key + keylen + (key[keylen] == '=');
      valuelen = strcspn(value, " ");
      valuelen = isupport_decode(buf, sizeof buf, value, valuelen);
      set_cb(key, keylen, buf, valuelen);
      str = value + valuelen;
    }
  }
}

void isupport_handle(const char *str)
{
  isupport_parse(str, isupport_set, isupport_unset);
}

void isupport_reset(void)
{
  isupport_clear();
}

void isupport_handle_default(const char *str)
{
  isupport_clear_default();
  isupport_parse(str, isupport_set_default, NULL);
}

void isupport_handle_force(const char *str)
{
  isupport_clear_forced();
  isupport_parse(str, isupport_set_forced, isupport_unset_forced);
}

static void isupport_free(struct isupport *data) {
  if (data->key)
    nfree(data->key);
  if (data->forced)
    nfree(data->forced);
  if (data->def)
    nfree(data->def);
  if (data->value)
    nfree(data->value);
  nfree(data);
}

static int tcl_isupport STDVAR
{
  BADARGS(1, 2, " subcommand ?args?");
/*
  if (match_my_nick(argv[1]))
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
    */
  return TCL_OK;
}

void isupport_init(void) {
  isupportdata = isupportdata_create(isupport_free);
  isupport_handle_default(isupport_default);
}

void isupport_fini(void) {
  isupportdata_destroy(isupportdata);
}

void isupport_cleanup(void) {
  struct isupportkeydata *i, *next;
  struct isupport *data;
  LIST_FOREACH_SAFE(isupportdata, i, next) {
    data = LIST_ELEM_DATA(i);
    if (!data->forced && !data->def && !data->value && !data->forced_unset)
      isupportdata_delete(isupportdata, i);
  }
}

size_t isupport_expmem(void) {
  size_t bytes = sizeof *isupportdata;
  struct isupportkeydata *i;
  struct isupport *data;
  LIST_FOREACH_DATA(isupportdata, i, data) {
    bytes += sizeof *i;
    if (data->value)
      bytes += strlen(data->value) + 1;
    if (data->def)
      bytes += strlen(data->def) + 1;
    if (data->forced)
      bytes += strlen(data->forced) + 1;
  }
  return bytes;
}

#define FITS_INTO(len, key, value) ((strlen((key)) + ((value) ? strlen((value)) : 0) + 2) < (len))

static void isupport_stringify(int idx, char *buf, size_t bufsize, size_t *len,
    size_t prefixlen, const char *key, const char *value)
{
  if (value)
    value = isupport_encode(value);
  if (!FITS_INTO(bufsize - *len, key, value)) {
    dprintf(idx, "%s\n", buf);
    *len = prefixlen;
    if (!FITS_INTO(bufsize - *len, key, value)) {
      dprintf(idx, "    isupport info too long to display.\n");
      return;
    }
  }
  if (!value)
    *len += sprintf(buf + *len, " -%s", key);
  else if (!value[0])
    *len += sprintf(buf + *len, " %s", key);
  else
    *len += sprintf(buf + *len, " %s=%s", key, value);
}

#define ISUPPORT_REPORT(name, elem, forced) do {                               \
  char buf[450] = "    isupport (" name "):";                                  \
  size_t prefixlen = strlen(buf), len = prefixlen;                             \
  struct isupportkeydata *i;                                                   \
  struct isupport *data;                                                       \
  LIST_FOREACH_DATA(isupportdata, i, data) {                                   \
    if ((forced && data->forced_unset) || data->elem)                          \
      isupport_stringify(idx, buf, sizeof buf, &len, prefixlen, data->key,     \
          (forced && data->forced_unset ? NULL : data->elem));                 \
  }                                                                            \
  if (len > prefixlen)                                                         \
    dprintf(idx, "%s\n", buf);                                                 \
} while (0)

#define ISUPPORT_REPORT_COMBINED do {                                          \
  char buf[450] = "    isupport (current):";                                   \
  size_t prefixlen = strlen(buf), len = prefixlen;                             \
  struct isupportkeydata *i;                                                   \
  struct isupport *data;                                                       \
  LIST_FOREACH_DATA(isupportdata, i, data) {                                   \
    if (isupport_get_from(data))                                               \
      isupport_stringify(idx, buf, sizeof buf, &len, prefixlen, data->key,     \
          isupport_get_from(data));                                            \
  }                                                                            \
  if (len > prefixlen)                                                         \
    dprintf(idx, "%s\n", buf);                                                 \
} while (0)

void isupport_report(int idx, int details) {
  if (details) {
    ISUPPORT_REPORT("forced", forced, 1);
    ISUPPORT_REPORT("server", value, 0);
    ISUPPORT_REPORT("default", def, 0);
  }
  ISUPPORT_REPORT_COMBINED;
}
