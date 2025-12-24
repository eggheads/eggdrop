/*
 * webui.c -- part of webui.mod
 */
/*
 * Copyright (C) 2023 - 2025 Michael Ortmann MIT License
 * Copyright (C) 2025 Eggheads Development Team
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

#include "src/mod/module.h"

#ifdef TLS
#define MODULE_NAME "webui"

#include <errno.h>
#include <fcntl.h>
#include <resolv.h> /* base64 encode b64_ntop() and base64 decode b64_pton() */
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include "src/version.h"

#define WS_GUID   "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_KEY    "Sec-WebSocket-Key:"
#define WS_KEYLEN 24 /* key is padded, so its always 24 bytes */
#define WS_LEN    28 /* length of Sec-WebSocket-Accept header field value
                      * base64(len(sha1))
                      * import math; (4 * math.ceil(20 / 3)) */
#define WS_ECHO_ON  0x01
#define WS_ECHO_OFF 0x02

static Function *global = NULL;

/* 0x15 = TLS ContentType alert
 * 0x0a = TLS Alert       unexpected_message
 */
static const uint8_t alert[] = {0x15, 0x03, 0x01, 0x00, 0x02, 0x02, 0x0a};

static void webui_http_eof(int idx)
{
  debug2("webui: webui_http_eof() idx %i sock %li", idx, dcc[idx].sock);
  dcc[idx].u.webui_listen_idx = 0;
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void put_404(int idx) {
  int i;
  char *response;

  debug1("webui: put_404() idx %i", idx);
  i = snprintf(NULL, 0,
    "HTTP/1.1 404 \r\n" /* textual phrase is OPTIONAL */
    "Content-Length: 13\r\n"
    "Content-Type: text/plain\r\n"
    "Server: %s\r\n"
    "\r\n"
    "404 Not Found",
    stealth_telnets ? "nginx/1.28.1" : "Eggdrop/" EGG_STRINGVER "+" EGG_PATCH);
  response = nmalloc(i + 1);
  sprintf(response,
    "HTTP/1.1 404 \r\n" /* textual phrase is OPTIONAL */
    "Content-Length: 13\r\n"
    "Content-Type: text/plain\r\n"
    "Server: %s\r\n"
    "\r\n"
    "404 Not Found",
    stealth_telnets ? "nginx/1.28.1" : "Eggdrop/" EGG_STRINGVER "+" EGG_PATCH);
  tputs(dcc[idx].sock, response, i);
  nfree(response);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

/* cache file data by modification time */
struct file_cache_struct {
  char filename[27];
  char content_type[25];
  struct timespec st_mtim;
  char *data;
} file_cache[3] = {
  {"webui/apple-touch-icon.png", "image/png",                { .tv_sec = -1, .tv_nsec = -1 }, NULL},
  {"webui/favicon.ico",          "image/x-icon",             { .tv_sec = -1, .tv_nsec = -1 }, NULL},
  {"webui/index.html",           "text/html; charset=utf-8", { .tv_sec = -1, .tv_nsec = -1 }, NULL}
};

static void put_file(int idx, int file_cache_index) {
  struct stat sb;
  struct file_cache_struct *f = &file_cache[file_cache_index];
  int fd, i;
  char *response;

  if (stat(f->filename, &sb) < 0) {
    putlog(LOG_MISC, "*", "WEBUI error: fstat(%s): %s", f->filename, strerror(errno));
    return;
  }
  if ((f->st_mtim.tv_sec != sb.st_mtim.tv_sec) ||
      (f->st_mtim.tv_nsec != sb.st_mtim.tv_nsec)) {
    if ((fd = open(f->filename, O_RDONLY)) < 0) {
      putlog(LOG_MISC, "*", "WEBUI error: open(%s): %s", f->filename, strerror(errno));
      put_404(idx);
      return;
    }
    f->data = nrealloc(f->data, sb.st_size); /* TODO: nfree() on module unloading */
    if (read(fd, f->data, sb.st_size) < 0) {
      putlog(LOG_MISC, "*", "WEBUI error: read(%s): %s", f->filename, strerror(errno));
      if ((fd = close(fd)) < 0)
        putlog(LOG_MISC, "*", "WEBUI error: close(%s): %s", f->filename, strerror(errno));
      put_404(idx);
      return;
    }
    if ((fd = close(fd)) < 0) {
      putlog(LOG_MISC, "*", "WEBUI error: close(%s): %s", f->filename, strerror(errno));
      put_404(idx);
      return;
    }
    f->st_mtim.tv_sec = sb.st_mtim.tv_sec;
    f->st_mtim.tv_nsec = sb.st_mtim.tv_nsec;
  }
  i = snprintf(NULL, 0,
    "HTTP/1.1 200 \r\n" /* textual phrase is OPTIONAL */
    "Content-Length: %jd\r\n"
    "Content-Type: %s\r\n" /* at least firefox 144 needs this */
    "Server: %s\r\n"
    "\r\n",
    (intmax_t) sb.st_size, f->content_type,
    stealth_telnets ? "nginx/1.28.1" : "Eggdrop/" EGG_STRINGVER "+" EGG_PATCH);
  response = nmalloc(i + sb.st_size);
  sprintf(response,
    "HTTP/1.1 200 \r\n" /* textual phrase is OPTIONAL */
    "Content-Length: %jd\r\n"
    "Content-Type: %s\r\n" /* at least firefox 144 needs this */
    "Server: %s\r\n"
    "\r\n",
    (intmax_t) sb.st_size, f->content_type,
    stealth_telnets ? "nginx/1.28.1" : "Eggdrop/" EGG_STRINGVER "+" EGG_PATCH);
  memcpy(response + i, f->data, sb.st_size);
  tputs(dcc[idx].sock, response, i + sb.st_size);
  nfree(response);
}

static void webui_http_activity(int idx, char *buf, int len)
{
  struct rusage ru1, ru2;
  int r, i, listen_idx;
  char *response;

  if (len < 6) { /* TODO: better len check */
    putlog(LOG_MISC, "*",
           "WEBUI error: %s sent something other than http GET request",
           iptostr(&dcc[idx].sockname.addr.sa));
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return;
  }
  if (buf[0] == 0x16) { /* 0x16 = TLS handshake */
      putlog(LOG_MISC, "*",
             "WEBUI error: %s requested TLS handshake for non-ssl port",
             iptostr(&dcc[idx].sockname.addr.sa));
      tputs(dcc[idx].sock, (char *) alert, sizeof alert);
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return;
  }
  r = getrusage(RUSAGE_SELF, &ru1);
  debug2("webui: webui_http_activity(): idx %i len %i", idx, len);
  buf[len] = '\0';
  if (buf[5] == ' ') {
    debug1("webui: GET / idx %i", idx);
    put_file(idx, 2);
  } else if (buf[5] == 'f') {
    put_file(idx, 1);
  } else if (buf[5] == 'w') {
    debug1("webui: GET /w idx %i", idx);
    buf = strstr(buf, WS_KEY);
    if (!buf) {
      putlog(LOG_MISC, "*", "WEBUI error: Sec-WebSocket-Key not found ip %s", iptostr(&dcc[idx].sockname.addr.sa));
      return;
    }
    buf += sizeof WS_KEY;
    for(i = 0; i < WS_KEYLEN; i++)
      if (!buf[i]) {
        putlog(LOG_MISC, "*", "WEBUI error: Sec-WebSocket-Key too short ip %s", iptostr(&dcc[idx].sockname.addr.sa));
        return;
      }
    unsigned char hash[SHA_DIGEST_LENGTH];
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha1();
    unsigned int md_len;
    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, buf, WS_KEYLEN);
    EVP_DigestUpdate(mdctx, WS_GUID, (sizeof WS_GUID) - 1);
    EVP_DigestFinal_ex(mdctx, hash, &md_len);
    EVP_MD_CTX_free(mdctx);
#else
    SHA_CTX c;
    SHA1_Init(&c);
    SHA1_Update(&c, buf, WS_KEYLEN);
    SHA1_Update(&c, WS_GUID, (sizeof WS_GUID) - 1);
    SHA1_Final(hash, &c);
#endif

    char out[WS_LEN + 1];
    /* TODO: remove assert / debug */
    if (b64_ntop(hash, sizeof hash, out, sizeof out) != WS_LEN) {
      putlog(LOG_MISC, "*", "WEBUI error: b64_ntop() != WS_LEN");
      return;
    }

    i = snprintf(NULL, 0,
      "HTTP/1.1 101 Switching Protocols\r\n"
      "Upgrade: websocket\r\n"
      "Connection: Upgrade\r\n"
      "Sec-WebSocket-Accept: %s\r\n"
      "\r\n", out);
    response = nmalloc(i + 1);
    sprintf(response,
      "HTTP/1.1 101 Switching Protocols\r\n"
      "Upgrade: websocket\r\n"
      "Connection: Upgrade\r\n"
      "Sec-WebSocket-Accept: %s\r\n"
      "\r\n", out);
    tputs(dcc[idx].sock, response, i);
    nfree(response);

    sock_list* socklist_i = &socklist[findsock(dcc[idx].sock)];
    socklist_i->flags |= SOCK_WS;


    socklist_i->flags &= ~ SOCK_BINARY; /* we need it for net.c sockgets(), is there better place to do this? */
    debug2("webui: unset flag SOCK_BINARY idx %i sock %li", idx, dcc[idx].sock);
    //strcpy(dcc[idx].host, "*"); /* important for later dcc_telnet_id wild_match, is there better place to do this? */
    /* .host becomes .nick in change_to_dcc_telnet_id() */
    debug4("webui: set flag SOCK_WS socklist %i idx %i sock %li status %lu", findsock(dcc[idx].sock), idx, dcc[idx].sock, dcc[idx].status);

    dcc[idx].status |= STAT_USRONLY; /* magick */

    listen_idx = dcc[idx].u.webui_listen_idx;
    dcc[idx].u.webui_listen_idx = 0;
    dcc_telnet_hostresolved2(idx, listen_idx);
  } else if (buf[5] == 'a') {
    put_file(idx, 0);
  } else
    put_404(idx);
  if ((dcc[idx].sock != -1) && (len == 511)) { /* sock == -1 if lostdcc() in dcc_telnet_hostresolved2() */
    /* read probable remaining bytes */
    SSL *ssl = socklist[findsock(dcc[idx].sock)].ssl;
    if (ssl)
      debug2("webui: SSL_read(): idx %i len %i", idx, SSL_read(ssl, buf, 511));
    else
      debug2("webui: read(): idx %i len %li", idx, read(dcc[idx].sock, buf, 511));
  }
  if (!r && !getrusage(RUSAGE_SELF, &ru2))
    debug2("webui: webui_http_activity(): user %.3fms sys %.3fms",
           (double) (ru2.ru_utime.tv_usec - ru1.ru_utime.tv_usec) / 1000 +
           (double) (ru2.ru_utime.tv_sec  - ru1.ru_utime.tv_sec ) * 1000,
           (double) (ru2.ru_stime.tv_usec - ru1.ru_stime.tv_usec) / 1000 +
           (double) (ru2.ru_stime.tv_sec  - ru1.ru_stime.tv_sec ) * 1000);
}

static void webui_http_display(int idx, char *buf)
{
  if (!dcc[idx].ssl)
    strcpy(buf, "webui http");
  else
    strcpy(buf, "webui https");
}

static struct dcc_table DCC_WEBUI_HTTP = {
  "WEBUI_HTTP",
  0,
  webui_http_eof,
  webui_http_activity,
  NULL,
  NULL,
  webui_http_display,
  NULL,
  NULL,
  NULL,
  NULL
};

static void webui_dcc_telnet_hostresolved(int idx, int listen_idx)
{
    debug2("webui_dcc_telnet_hostresolved() idx %i listen_idx %i", idx, listen_idx);
    changeover_dcc(idx, &DCC_WEBUI_HTTP, 0);
    dcc[idx].u.webui_listen_idx = listen_idx;
    sockoptions(dcc[idx].sock, EGG_OPTION_SET, SOCK_BINARY);
    sockoptions(dcc[idx].sock, EGG_OPTION_UNSET, SOCK_BUFFER);
    // dcc[i].u.other = NULL; /* important, else nfree() error in lostdcc on eof */
}

static size_t escape_html(char *dst, size_t dst_size, char *src, size_t src_size) {
  int i;
  char *d = dst;

  for (i = 0; i < src_size; i++) {
    switch ((unsigned char) src[i]) {
      case '"':
        if (dst_size < 6) {
          debug0("webui: escape_html(): destination string too long, PLEASE REPORT THIS BUG");
          return d - dst;
        }
        *d++ = '&';
        *d++ = 'q';
        *d++ = 'u';
        *d++ = 'o';
        *d++ = 't';
        *d++ = ';';
        dst_size -= 6;
        break;
      case '&':
        if (dst_size < 5) {
          debug0("webui: escape_html(): destination string too long, PLEASE REPORT THIS BUG");
          return d - dst;
        }
        *d++ = '&';
        *d++ = 'a';
        *d++ = 'm';
        *d++ = 'p';
        *d++ = ';';
        dst_size -= 5;
        break;
      case '\'':
        if (dst_size < 6) {
          debug0("webui: escape_html(): destination string too long, PLEASE REPORT THIS BUG");
          return d - dst;
        }
        *d++ = '&';
        *d++ = 'a';
        *d++ = 'p';
        *d++ = 'o';
        *d++ = 's';
        *d++ = ';';
        dst_size -= 6;
        break;
      case '<':
        if (dst_size < 4) {
          debug0("webui: escape_html(): destination string too long, PLEASE REPORT THIS BUG");
          return d - dst;
        }
        *d++ = '&';
        *d++ = 'l';
        *d++ = 't';
        *d++ = ';';
        dst_size -= 4;
        break;
      case '>':
        if (dst_size < 4) {
          debug0("webui: escape_html(): destination string too long, PLEASE REPORT THIS BUG");
          return d - dst;
        }
        *d++ = '&';
        *d++ = 'g';
        *d++ = 't';
        *d++ = ';';
        dst_size -= 4;
        break;
      case ESC:
        if ((i + 3) < src_size) {
          if (     ((unsigned char) src[i + 1] == '[') &&
                   ((unsigned char) src[i + 2] == '0') &&
                   ((unsigned char) src[i + 3] == 'm')) {
            if (dst_size < 4) {
              debug0("webui: escape_html(): destination string too long, PLEASE REPORT THIS BUG");
              return d - dst;
            }
            *d++ = '<';
            *d++ = '/';
            *d++ = 'b';
            *d++ = '>';
            dst_size -= 4;
          } else if (((unsigned char) src[i + 1] == '[') &&
                   ((unsigned char) src[i + 2] == '1') &&
                   ((unsigned char) src[i + 3] == 'm')) {
            if (dst_size < 3) {
              debug0("webui: escape_html(): destination string too long, PLEASE REPORT THIS BUG");
              return d - dst;
            }
            *d++ = '<';
            *d++ = 'b';
            *d++ = '>';
            dst_size -= 3;
          } else
            debug3("webui: escape_html(): unknown escape sequence found, skipping, %02x %02x %02x, PLEASE REPORT THIS BUG",
                   (unsigned char) src[i + 1], (unsigned char) src[i + 2], (unsigned char) src[i + 3]);
          i += 3;
        } else
          debug0("webui: escape_html(): unknown SHORT escape sequence found, skipping, PLEASE REPORT THIS BUG");
        break;
      case TLN_IAC:
        if ((i + 2) < src_size) {
          if (dst_size < 1) {
            debug0("webui: escape_html(): destination string too long, PLEASE REPORT THIS BUG");
            return d - dst;
          }
          if (       ((unsigned char) src[i + 1] == TLN_WILL) &&
                     ((unsigned char) src[i + 2] == TLN_ECHO)) {
            *d++ = WS_ECHO_OFF;
            dst_size--;
          } else if (((unsigned char) src[i + 1] == TLN_WONT) &&
                     ((unsigned char) src[i + 2] == TLN_ECHO)) {
            *d++ = WS_ECHO_ON;
            dst_size--;
          } else
            debug2("webui: escape_html(): unknown telnet command found, skipping, %02x %02x, PLEASE REPORT THIS BUG",
                   (unsigned char) src[i + 1], (unsigned char) src[i + 2]);
          i += 2;
        } else
          debug0("webui: escape_html(): unknown SHORT telnet command found, skipping, PLEASE REPORT THIS BUG");
        break;
      default:
        if (dst_size < 1) {
          debug0("webui: escape_html(): destination string too long, PLEASE REPORT THIS BUG");
          return d - dst;
        }
        *d++ = src[i];
        dst_size--;
    }
  }
  return d - dst;
}

static size_t webui_frame(char **dst, char *src, size_t len) {
  static char buf[LOGLINELEN];
  uint16_t len2;

  /* escape/replace html code chars
   * write to buf + offset 4 to leave room for webui frame header
   */
  len = escape_html(buf + 4, (sizeof buf) - 4, src, len);
  /* we must not use putlog() or debug() here or we get recursion */
  /* A server MUST NOT mask any frames that it sends to the client */
  /* we use text, not binary, so escape_html() must output valid html */
  if (len < 0x7e) {
    buf[2] =0x81; /* FIN + text frame */
    buf[3] = len;
    *dst = buf + 2;
    len += 2;
  } else {
    buf[0] =0x81; /* FIN + text frame */
    buf[1] = 0x7e;
    len2 = htons(len);
    memcpy(buf + 2, &len2, 2);
    *dst = buf;
    len += 4;
  }
  /* we dont need to implement len > 0xffff,
   * because eggdrop wont send that much data at once,
   * we also limit by sizeof buf */
  return len;
}

/* TODO: return error code ? */
static void webui_unframe(int sock, char *buf, int *len)
{
  int i;
  uint8_t *key, *payload;
  uint16_t status_code;

  if (*len < 6) { /* TODO: better len check */

    /* TODO: return error code ? */
    putlog(LOG_MISC, "*", "WEBUI error: bogus WebSocket frame from sock %i", sock);
    killsock(sock);
    lostdcc(findanyidx(sock));
    return;
  }
  /* xor decrypt
   */
  key = (uint8_t *) buf;
  if (key[1] < 0xfe) {
    key += 2;
    *len -= 6;
  } else if (key[1] == 0xfe) {
    key += 4;
    *len -= 8;
  } else {
    key += 10;
    *len -= 14;
  }
  payload = key + 4;
  for (i = 0; i < *len; i++)
    payload[i] = payload[i] ^ key[i % 4];

  if (buf[0] & 0x08) {
    status_code = ntohs(*((uint16_t *) payload));
    putlog(LOG_MISC, "*", "WEBUI: connection closed by peer with status code %i%s sock %i",
           status_code, status_code == 1001 ? " (Going Away)" : "", sock);
    killsock(sock);
    lostdcc(findanyidx(sock));
    return;
  }

  memmove(buf, payload, *len);
  /* we switched back from binary sock to text sock for sockgets() needs this for dcc_telnet_id() */
  /* so now we have to add \r\n here :/ */
  strcpy(buf + *len, "\r\n");
  *len+= 2;
}

static char *webui_close(void)
{
  int idx;

  del_hook(HOOK_DCC_TELNET_HOSTRESOLVED, (Function) webui_dcc_telnet_hostresolved);
  del_hook(HOOK_WEBUI_FRAME, (Function) webui_frame);
  del_hook(HOOK_WEBUI_UNFRAME, (Function) webui_unframe);
  for (idx = 0; idx < dcc_total; idx++) {
    if (!strcmp(dcc[idx].nick, "(webui)") ||
        !strcmp(dcc[idx].type->name, "WEBUI_HTTP") ||
        (socklist[findsock(dcc[idx].sock)].flags & SOCK_WS)) {
      debug2("webui: webui_close(): closing sock idx %i, %li", idx, dcc[idx].sock);
      killsock(dcc[idx].sock);
      lostdcc(idx);
    }
  }
  return NULL;
}

EXPORT_SCOPE char *webui_start();

static Function webui_table[] = {
  (Function) webui_start,
  (Function) webui_close,
  NULL,
  NULL,
};

#endif
char *webui_start(Function *global_funcs)
{
#ifdef TLS
  global = global_funcs;
  module_register(MODULE_NAME, webui_table, 0, 10);
  if (!module_depend(MODULE_NAME, "eggdrop", 110, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.10.0 or later.";
  }
  add_hook(HOOK_DCC_TELNET_HOSTRESOLVED, (Function) webui_dcc_telnet_hostresolved);
  add_hook(HOOK_WEBUI_FRAME, (Function) webui_frame);
  add_hook(HOOK_WEBUI_UNFRAME, (Function) webui_unframe);
  return NULL;
#else
  return "Initialization failure: configured with --disable-tls or openssl not found";
#endif
}
