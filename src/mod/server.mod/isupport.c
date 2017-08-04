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
#define ISET(dst, val) do { if ((dst)) { nfree((dst)); }; \
  (dst) = valuendup((val), strlen((val))); } while (0)
#define ISETLEN(dst, val, len) do { if ((dst)) { nfree((dst)); }; \
  (dst) = valuendup((val), (len)); } while (0)

struct isupport {
  size_t keylen;
  const char *key;
  const char *forced, *server, *def;
  int ignored;
};

/* Instantiate list type for isupport data */
CREATE_LIST_TYPE(isupportdata, isupportkeydata, struct isupport *);

static struct isupportdata *isupportdata;

static char *isupport_default = "CASEMAPPING=rfc1459 CHANNELLEN=80 NICKLEN=9 CHANTYPES=#& PREFIX=(ov)@+";

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

/*** Utility functions ***/

static char *valuendup(const char *value, size_t len) {
  char *str = nmalloc(len + 1);
  memcpy(str, value, len);
  str[len] = '\0';
  return str;
} 

/* length has to be validated for equality before calling this. */
static int keyupeq(const char *key, const char *upkey) {
  int i;
  for (i = 0; key[i] && upkey[i]; i++)
    if (toupper((unsigned char)key[i]) != upkey[i])
      return 0;
  return 1;
}

static char *keyupndup(const char *key, size_t len) {
  int i;
  char *up = nmalloc(len + 1);
  for (i = 0; i < len; i++)
    up[i] = toupper((unsigned char)key[i]);
  up[i] = '\0';
  return up;
}

/*** isupport key record managing functions ***/

static struct isupport *add_record(const char *key, size_t keylen) {
  struct isupport *data = nmalloc(sizeof *data);

  data->key = keyupndup(key, keylen);
  data->keylen = keylen;
  data->ignored = 0;
  data->forced = data->def = data->server = NULL;
  isupportdata_append(isupportdata, data);
  return data;
}

static struct isupport *find_record(const char *key, size_t keylen) {
  struct isupportkeydata *i;
  struct isupport *data;

  LIST_FOREACH_DATA(isupportdata, i, data) {
    if (keylen == data->keylen && keyupeq(key, data->key))
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

/*** isupport get ***/

const char *isupport_get_default_from(struct isupport *data) {
  return data->def;
}

const char *isupport_get_current_from(struct isupport *data) {
  if (data->ignored)
    return NULL;
  if (data->forced)
    return data->forced;
  if (data->server)
    return data->server;
  if (data->def)
    return data->def;
  return NULL;
}

const char *isupport_get_current(const char *key, size_t keylen) {
  struct isupport *data = find_record(key, keylen);
  return data ? isupport_get_current_from(data) : NULL;
}

const char *isupport_get_from(struct isupport *data, const char *type) {
  if (!strcmp(type, "current"))
    return isupport_get_current_from(data);
  else if (!strcmp(type, "forced"))
    return data->forced;
  else if (!strcmp(type, "default"))
    return data->def;
  else if (!strcmp(type, "ignored"))
    return (data->ignored ? "1" : NULL);
  else if (!strcmp(type, "server"))
    return data->server;
  putlog(LOG_MISC, "*", "Unknown ISUPPORT get type: %s", type);
  return NULL;
}

/*** isupport set ***/

static void isupport_set_ignored_into(struct isupport *data, int value)
{
  data->ignored = value;
}

/* value is ignored */
static void isupport_set_ignored(const char *key, size_t keylen,
    const char *value, size_t len)
{
  struct isupport *data = get_record(key, keylen);
  isupport_set_ignored_into(data, 1);
}

static void isupport_set_ignored_true(const char *key, size_t keylen)
{
  isupport_set_ignored(key, keylen, "", 0);
}

static void isupport_set_forced(const char *key, size_t keylen,
    const char *value, size_t len)
{
  struct isupport *data = get_record(key, keylen);
  ISETLEN(data->forced, value, len);
}

static void isupport_set_server(const char *key, size_t keylen,
    const char *value, size_t len)
{
  struct isupport *data = get_record(key, keylen);
  ISETLEN(data->server, value, len);
}

static void isupport_set_default(const char *key, size_t keylen,
    const char *value, size_t len)
{
  struct isupport *data = get_record(key, keylen);
  ISETLEN(data->def, value, len);
}

void isupport_set_into(struct isupport *data, const char *type, const char *value) {
  if (!strcmp(type, "forced"))
    ISET(data->forced, value);
  else if (!strcmp(type, "default"))
    ISET(data->def, value);
  else if (!strcmp(type, "server"))
    ISET(data->server, value);
  else
    putlog(LOG_MISC, "*", "Invalid ISUPPORT set type: %s", type);
}

/*** isupport unset ***/

static void isupport_unset_server(const char *key, size_t keylen) {
  struct isupport *data = find_record(key, keylen);

  if (!data) {
    putlog(LOG_MISC, "*", "isupport key %s not found, but tried to unset!", key);
    return;
  }
  ICLEAR(data->server);
}

void isupport_unset_into(struct isupport *data, const char *type) {
  if (!strcmp(type, "ignored"))
    data->ignored = 0;
  else if (!strcmp(type, "forced"))
    ICLEAR(data->forced);
  else if (!strcmp(type, "default"))
    ICLEAR(data->def);
  else if (!strcmp(type, "server"))
    ICLEAR(data->server);
  putlog(LOG_MISC, "*", "Unknown ISUPPORT unset type: %s", type);
}

/*** isupport clear ***/

static void isupport_clear_ignored(void) {
  struct isupportkeydata *i;
  struct isupport *data;
  LIST_FOREACH_DATA(isupportdata, i, data)
    data->ignored = 0;
}

static void isupport_clear_forced(void) {
  struct isupportkeydata *i;
  struct isupport *data;
  LIST_FOREACH_DATA(isupportdata, i, data)
    ICLEAR(data->forced);
}

static void isupport_clear_server(void) {
  struct isupportkeydata *i;
  struct isupport *data;
  LIST_FOREACH_DATA(isupportdata, i, data)
    ICLEAR(data->server);
}

static void isupport_clear_default(void) {
  struct isupportkeydata *i;
  struct isupport *data;
  LIST_FOREACH_DATA(isupportdata, i, data)
    ICLEAR(data->def);
}

/*** isupport parse/setstr ***/

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


/*** isupport handlers for strings ***/

void isupport_handle_ignored(const char *str)
{
  isupport_clear_ignored();
  isupport_parse(str, isupport_set_ignored, NULL);
}

void isupport_handle_forced(const char *str)
{
  isupport_clear_forced();
  isupport_parse(str, isupport_set_forced, isupport_set_ignored_true);
}

void isupport_handle_server(const char *str)
{
  /* intentionally no clear, we clear on disconnect, must handle multiple lines */
  isupport_parse(str, isupport_set_server, isupport_unset_server);
}

void isupport_handle_default(const char *str)
{
  isupport_clear_default();
  isupport_parse(str, isupport_set_default, NULL);
}

void isupport_handle(const char *str, const char *type)
{
  if (!strcmp(type, "default"))
    isupport_handle_default(str);
  else if (!strcmp(type, "forced"))
    isupport_handle_forced(str);
  else if (!strcmp(type, "ignored"))
    isupport_handle_ignored(str);
  else
    putlog(LOG_MISC, "*", "Invalid ISUPPORT handle type: %s", type);
}

/*** Module utility functions ***/

void isupport_reset(void)
{
  isupport_clear_server();
}

static void isupport_free(struct isupport *data) {
  if (data->key)
    nfree(data->key);
  if (data->forced)
    nfree(data->forced);
  if (data->def)
    nfree(data->def);
  if (data->server)
    nfree(data->server);
  nfree(data);
}

void isupport_init(void) {
  const char *def;;
  isupportdata = isupportdata_create(isupport_free);
  def = Tcl_GetVar(interp, "isupport-default", TCL_GLOBAL_ONLY);
  if (!def)
    def = isupport_default;
  isupport_handle_default(def);
}

void isupport_fini(void) {
  isupportdata_destroy(isupportdata);
}

void isupport_cleanup(void) {
  struct isupportkeydata *i, *next;
  struct isupport *data;
  LIST_FOREACH_SAFE(isupportdata, i, next) {
    data = LIST_ELEM_DATA(i);
    if (!data->forced && !data->def && !data->server && !data->ignored)
      isupportdata_delete(isupportdata, i);
  }
}

size_t isupport_expmem(void) {
  size_t bytes = sizeof *isupportdata;
  struct isupportkeydata *i;
  struct isupport *data;
  LIST_FOREACH_DATA(isupportdata, i, data) {
    bytes += sizeof *i;
    if (data->server)
      bytes += strlen(data->server) + 1;
    if (data->def)
      bytes += strlen(data->def) + 1;
    if (data->forced)
      bytes += strlen(data->forced) + 1;
  }
  return bytes;
}

/*** .status information ***/

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

static void isupport_report_type(int idx, const char *type)
{
  int forced;
  char buf[450];
  const char *value;
  size_t prefixlen, len;
  struct isupportkeydata *i;
  struct isupport *data;

  forced = !strcmp(type, "forced");
  len = prefixlen = sprintf(buf, "    isupport (%s):", type);
  LIST_FOREACH_DATA(isupportdata, i, data) {
    value = isupport_get_from(data, type);
    if (forced && isupport_get_from(data, "ignored"))
      value = NULL;
    else if (!value)
      continue;
    isupport_stringify(idx, buf, sizeof buf, &len, prefixlen, data->key, value);
  }
  if (len > prefixlen)
    dprintf(idx, "%s\n", buf);
}

void isupport_report(int idx, int details) {
  if (details) {
    isupport_report_type(idx, "forced");
    isupport_report_type(idx, "server");
    isupport_report_type(idx, "default");
  }
  if (server_online)
    isupport_report_type(idx, "current");
}
