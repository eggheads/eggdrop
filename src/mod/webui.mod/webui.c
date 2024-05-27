/*
 * webui.c -- part of webui.mod
 */
/*
 * Copyright (C) 2023 - 2024 Michael Ortmann MIT License
 * Copyright (C) 2024 Eggheads Development Team
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
 *
 */

#define MODULE_NAME "webui"

#include <errno.h>
#include <fcntl.h>
#include <resolv.h> /* base64 encode b64_ntop() and base64 decode b64_pton() */
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include "src/version.h"
#include "src/mod/module.h"

#define WS_GUID   "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_KEY    "Sec-WebSocket-Key:"
#define WS_KEYLEN 24 /* key is padded, so its always 24 bytes */
#define WS_LEN    28 /* length of Sec-WebSocket-Accept header field value
                      * base64(len(sha1))
                      * import math; (4 * math.ceil(20 / 3)) */

static Function *global = NULL;

/* 0x15 = TLS ContentType alert
 * 0x0a = TLS Alert       unexpected_message
 */
static uint8_t alert[] = {0x15, 0x03, 0x01, 0x00, 0x02, 0x02, 0x0a};

/* wget https://www.eggheads.org/favicon.ico
 * xxd -i favicon.ico
 */
static unsigned char favicon_ico[] = {
  0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x24, 0x00, 0x24, 0x00, 0xf7, 0x00,
  0x00, 0x2e, 0x2c, 0x2d, 0x23, 0x22, 0x24, 0x31, 0x31, 0x32, 0x3c, 0x3b,
  0x3c, 0x46, 0x41, 0x3f, 0x3a, 0x4a, 0x51, 0x3e, 0x5e, 0x71, 0x45, 0x44,
  0x45, 0x49, 0x46, 0x47, 0x48, 0x47, 0x48, 0x4b, 0x4a, 0x4b, 0x44, 0x44,
  0x49, 0x54, 0x53, 0x54, 0x54, 0x56, 0x58, 0x5c, 0x5b, 0x5c, 0x62, 0x60,
  0x5e, 0x78, 0x68, 0x5f, 0x48, 0x65, 0x76, 0x46, 0x68, 0x7e, 0x57, 0x68,
  0x76, 0x58, 0x69, 0x75, 0x63, 0x63, 0x65, 0x65, 0x66, 0x69, 0x6c, 0x6b,
  0x6c, 0x70, 0x6c, 0x6c, 0x6f, 0x6d, 0x70, 0x6e, 0x71, 0x74, 0x6e, 0x76,
  0x7e, 0x74, 0x73, 0x73, 0x74, 0x74, 0x78, 0x77, 0x78, 0x7b, 0x7b, 0x7b,
  0x7c, 0x81, 0x6d, 0x62, 0x80, 0x7e, 0x7e, 0x98, 0x7f, 0x70, 0x85, 0x80,
  0x7e, 0x1a, 0x5d, 0x82, 0x16, 0x6e, 0x9d, 0x22, 0x68, 0x8f, 0x3c, 0x67,
  0x86, 0x38, 0x75, 0x97, 0x1d, 0x75, 0xa7, 0x10, 0x74, 0xac, 0x1c, 0x77,
  0xa9, 0x09, 0x7e, 0xbc, 0x21, 0x76, 0xa9, 0x2b, 0x7f, 0xae, 0x24, 0x7b,
  0xaa, 0x33, 0x7b, 0xa5, 0x49, 0x6d, 0x84, 0x57, 0x73, 0x86, 0x5a, 0x76,
  0x87, 0x51, 0x79, 0x8e, 0x5b, 0x76, 0x8a, 0x4e, 0x7b, 0x95, 0x54, 0x7f,
  0x97, 0x69, 0x79, 0x85, 0x74, 0x7c, 0x83, 0x7c, 0x7e, 0x80, 0x62, 0x7f,
  0x91, 0x12, 0x80, 0xb9, 0x1f, 0x85, 0xb8, 0x34, 0x80, 0xa9, 0x22, 0x82,
  0xb5, 0x2c, 0x85, 0xb5, 0x3f, 0x87, 0xb1, 0x7c, 0x80, 0x83, 0x65, 0x82,
  0x98, 0x64, 0x88, 0x9c, 0x74, 0x88, 0x93, 0x47, 0x89, 0xad, 0x55, 0x89,
  0xab, 0x44, 0x8d, 0xb7, 0x4a, 0x8c, 0xb4, 0x4c, 0x91, 0xb4, 0x5a, 0x95,
  0xb9, 0x6a, 0x8d, 0xa0, 0x75, 0x92, 0xa2, 0x79, 0x93, 0xa3, 0x6a, 0x9c,
  0xb5, 0x04, 0x81, 0xc4, 0x01, 0x86, 0xcc, 0x0a, 0x82, 0xc3, 0x84, 0x83,
  0x84, 0x8a, 0x85, 0x85, 0x87, 0x88, 0x89, 0x8c, 0x8a, 0x8b, 0x80, 0x8e,
  0x97, 0x90, 0x8e, 0x90, 0x85, 0x92, 0x9b, 0x94, 0x93, 0x94, 0x96, 0x96,
  0x98, 0x95, 0x98, 0x9c, 0x9d, 0x9b, 0x9d, 0xa1, 0x90, 0x86, 0xa2, 0x9d,
  0x9e, 0xa2, 0x9f, 0xa1, 0x9b, 0xa0, 0xa6, 0x8d, 0xa4, 0xb6, 0x91, 0xa6,
  0xb3, 0xa2, 0xa2, 0xa3, 0xab, 0xa6, 0xa6, 0xa4, 0xa6, 0xaa, 0xa8, 0xa7,
  0xa8, 0xa6, 0xaa, 0xaf, 0xac, 0xab, 0xac, 0xb8, 0xb1, 0xaf, 0xb4, 0xb3,
  0xb4, 0xb9, 0xb7, 0xb7, 0xb5, 0xb7, 0xba, 0xb7, 0xb8, 0xb9, 0xbd, 0xbc,
  0xbd, 0xc0, 0xbf, 0xbf, 0xc2, 0xc0, 0xbf, 0xb2, 0xc1, 0xcb, 0xc5, 0xc2,
  0xc3, 0xca, 0xc5, 0xc5, 0xca, 0xc6, 0xc8, 0xcc, 0xcb, 0xcc, 0xd0, 0xcc,
  0xcd, 0xd5, 0xd0, 0xcf, 0xcf, 0xcf, 0xd0, 0xd1, 0xcf, 0xd0, 0xcd, 0xd1,
  0xd4, 0xcf, 0xd5, 0xd9, 0xd4, 0xd3, 0xd4, 0xdb, 0xd6, 0xd6, 0xda, 0xd8,
  0xd7, 0xdd, 0xdc, 0xdd, 0xd3, 0xda, 0xdd, 0xe1, 0xdd, 0xdd, 0xe2, 0xe0,
  0xdf, 0xde, 0xdf, 0xe0, 0xe0, 0xde, 0xe0, 0xdf, 0xe0, 0xe0, 0xe4, 0xe3,
  0xe4, 0xe9, 0xe7, 0xe7, 0xe7, 0xe7, 0xe8, 0xe6, 0xe8, 0xea, 0xed, 0xec,
  0xec, 0xf0, 0xee, 0xef, 0xf1, 0xf0, 0xef, 0xf4, 0xf3, 0xf3, 0xf8, 0xf7,
  0xf7, 0xf8, 0xf8, 0xf7, 0xf8, 0xf7, 0xf8, 0xfe, 0xfe, 0xfe, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x21, 0xf9, 0x04, 0x01, 0x00, 0x00, 0x93, 0x00, 0x2c, 0x00, 0x00,
  0x00, 0x00, 0x24, 0x00, 0x24, 0x00, 0x00, 0x08, 0xfe, 0x00, 0x27, 0x09,
  0x1c, 0x48, 0xb0, 0xa0, 0x40, 0x0e, 0x02, 0xa7, 0x18, 0x5c, 0xc8, 0xb0,
  0xa1, 0xc3, 0x87, 0x10, 0x09, 0xba, 0x59, 0x13, 0xb1, 0x62, 0xc1, 0x3a,
  0x82, 0x1c, 0x39, 0x5a, 0xf4, 0xa7, 0x8e, 0x45, 0x88, 0x6f, 0x08, 0x09,
  0x42, 0x24, 0xa9, 0xa4, 0x24, 0x48, 0x7d, 0x3e, 0x3a, 0xfc, 0x83, 0x88,
  0x91, 0xc9, 0x97, 0x80, 0x54, 0x2e, 0xb4, 0x53, 0x08, 0x11, 0xa4, 0x97,
  0x2f, 0xfd, 0xc8, 0x24, 0x38, 0xa7, 0x50, 0x46, 0x9c, 0x2f, 0x17, 0x51,
  0xdc, 0x39, 0xc9, 0x8e, 0xa0, 0x45, 0x87, 0xd2, 0x60, 0x21, 0xd3, 0xe7,
  0xa6, 0xa4, 0x47, 0x6f, 0xaa, 0xe8, 0xe8, 0x40, 0xf4, 0x0e, 0xa0, 0x3e,
  0x65, 0xd6, 0xc4, 0x79, 0xb3, 0xa6, 0xce, 0x4d, 0x41, 0x73, 0xfa, 0xd8,
  0xe9, 0x52, 0x95, 0xce, 0x97, 0x11, 0x17, 0x2c, 0x64, 0xe0, 0x30, 0xc5,
  0xce, 0x20, 0x2b, 0x0d, 0x12, 0x28, 0x58, 0x20, 0x20, 0x80, 0xcc, 0x2f,
  0x08, 0x04, 0x70, 0x58, 0x63, 0x27, 0xce, 0x14, 0x00, 0x07, 0x42, 0x0c,
  0x58, 0x03, 0x08, 0xd1, 0x21, 0x37, 0x04, 0x54, 0x76, 0x28, 0x73, 0x41,
  0x8b, 0xd3, 0x92, 0x75, 0x06, 0x38, 0x98, 0x82, 0x53, 0x61, 0xc5, 0x01,
  0x00, 0x12, 0x24, 0x78, 0x60, 0xa7, 0xe4, 0x9f, 0x46, 0x25, 0xad, 0x00,
  0xb0, 0x22, 0x29, 0x12, 0x1d, 0x47, 0x92, 0xb4, 0x1c, 0xa8, 0x28, 0x40,
  0x81, 0x1a, 0x0f, 0x0d, 0xfe, 0x48, 0x5a, 0xc4, 0x20, 0x4d, 0xc9, 0x2e,
  0x0b, 0x2e, 0x44, 0x6a, 0x54, 0x05, 0x90, 0xa4, 0x2e, 0x0e, 0x3e, 0xd2,
  0x59, 0x83, 0xa1, 0x4f, 0xc9, 0x3e, 0x8b, 0x4e, 0x7a, 0x48, 0x53, 0xc1,
  0x78, 0xa4, 0x93, 0x21, 0x64, 0xd2, 0xa1, 0x32, 0xe5, 0xd1, 0xcb, 0x36,
  0x17, 0x16, 0x9d, 0xc9, 0x70, 0x08, 0x12, 0xa4, 0x38, 0x03, 0xa4, 0xfe,
  0x03, 0x0a, 0x61, 0x05, 0x79, 0x52, 0x0b, 0x9d, 0x21, 0x91, 0xb1, 0xf0,
  0xa1, 0xc3, 0x80, 0x06, 0x32, 0xe7, 0x20, 0x72, 0xb4, 0xe6, 0x83, 0x85,
  0xc6, 0x87, 0x5e, 0x1e, 0xe2, 0x4a, 0x56, 0xba, 0x9f, 0x9b, 0x90, 0x2c,
  0x82, 0x1a, 0x50, 0x92, 0x38, 0x12, 0xc7, 0x4e, 0x73, 0xdc, 0xe1, 0x12,
  0x81, 0x2f, 0x25, 0x42, 0x54, 0x51, 0x7e, 0x30, 0xf8, 0x92, 0x1d, 0x0f,
  0xc6, 0x81, 0x07, 0x49, 0x12, 0x22, 0xf2, 0xa0, 0x40, 0x70, 0xe4, 0xc7,
  0xe0, 0x77, 0x1b, 0x0a, 0x74, 0x87, 0x23, 0x87, 0xf8, 0x71, 0x47, 0x1f,
  0x70, 0xf8, 0xf6, 0x88, 0x1e, 0x21, 0x0e, 0xf4, 0x06, 0x07, 0x03, 0x80,
  0x00, 0x01, 0x01, 0x54, 0xa4, 0x31, 0x54, 0x8b, 0x02, 0x35, 0x61, 0x40,
  0x0e, 0x5e, 0x88, 0x40, 0x01, 0x0a, 0x47, 0xe0, 0x38, 0x90, 0x0c, 0x45,
  0x88, 0x71, 0xc2, 0x03, 0x59, 0x8c, 0x71, 0xc3, 0x40, 0x76, 0x50, 0xa8,
  0x52, 0x11, 0xc8, 0x5d, 0x95, 0x83, 0x10, 0x68, 0xc4, 0x50, 0x81, 0x16,
  0x56, 0xe0, 0x40, 0x83, 0x13, 0x87, 0x38, 0x12, 0xc9, 0x22, 0x1f, 0xdd,
  0x20, 0x87, 0x49, 0x72, 0xa4, 0x40, 0x42, 0x01, 0x38, 0xe4, 0xa0, 0x41,
  0x0c, 0x2d, 0x40, 0x01, 0x84, 0x22, 0x25, 0x3d, 0xe2, 0x64, 0x44, 0x44,
  0x24, 0x77, 0xd2, 0x13, 0x51, 0x40, 0x61, 0x42, 0x07, 0x53, 0xe0, 0xc0,
  0x42, 0x14, 0x51, 0x00, 0x61, 0x48, 0x49, 0x8b, 0xcc, 0x61, 0xd1, 0x0d,
  0x7c, 0x78, 0xe9, 0x88, 0x12, 0x79, 0xc2, 0xc0, 0x05, 0x18, 0x61, 0x94,
  0x00, 0x68, 0x0f, 0x89, 0x12, 0xe2, 0xc4, 0x15, 0x11, 0xcd, 0xb1, 0x06,
  0x0e, 0x3e, 0x20, 0x91, 0xc4, 0x12, 0x3f, 0x44, 0xa1, 0x02, 0x05, 0x6c,
  0x74, 0x91, 0x46, 0x04, 0x3c, 0xe4, 0x19, 0x44, 0x12, 0x40, 0xd8, 0xd0,
  0x9f, 0x43, 0x44, 0x6d, 0xf4, 0xa1, 0x06, 0x16, 0x4c, 0x18, 0x01, 0xc4,
  0x0f, 0x2c, 0xbc, 0x50, 0x83, 0x15, 0x80, 0x80, 0xb1, 0x85, 0x0e, 0x35,
  0xa0, 0xf0, 0xc2, 0x0f, 0x30, 0x0c, 0x61, 0xc5, 0x17, 0x16, 0xa5, 0x41,
  0xc6, 0x1a, 0x66, 0xa4, 0x91, 0x85, 0x16, 0x53, 0x7c, 0xd1, 0x47, 0x1d,
  0x6b, 0x9c, 0x61, 0x85, 0x15, 0x5b, 0x68, 0x51, 0x06, 0x16, 0x69, 0xc8,
  0xb4, 0x46, 0x19, 0x77, 0x80, 0xe1, 0xd0, 0xab, 0x0b, 0x05, 0x04, 0x00,
  0x3b
};

static void webui_http_eof(int idx)
{
  debug2("webui: webui_http_eof() idx %i sock %li", idx, dcc[idx].sock);
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void webui_http_activity(int idx, char *buf, int len)
{
  struct rusage ru1, ru2;
  int r, i;
  char response[4096]; /* > sizeof webui.html
                        * TODO: dynamic size? else buffer overflow ;)
                        * we dont control webui.html size, that is user input!
                        */

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
  buf[len] = '\0'; /* TODO: is there no better way? we already know len */
  debug0("webui: http()");
  if (buf[5] == ' ') {
    debug0("webui: webui: GET /");
    #define PATH "text/webui.html"
    int fd;
    struct stat sb;
    char *body;
    if ((fd = open(PATH, O_RDONLY)) < 0) {
      putlog(LOG_MISC, "*", "WEBUI error: open(" PATH "): %s", strerror(errno));
      /* TODO: send 404 and/or lostdcc() killsock() */
      return;
    }
    if (fstat(fd, &sb) < 0) {
      putlog(LOG_MISC, "*", "WEBUI error: fstat(" PATH "): %s", strerror(errno));
      return;
    }

    if ((body = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE
#ifdef MAP_POPULATE
                        | MAP_POPULATE
#endif
                        , fd, 0)) == MAP_FAILED) {
      putlog(LOG_MISC, "*", "WEBUI error: mmap(" PATH "): %s\n", strerror(errno));
      return;
    }
    i = snprintf(response, sizeof response,
      "HTTP/1.1 200 \r\n" /* textual phrase is OPTIONAL */
      "Content-Length: %li\r\n"
      "Server: Eggdrop/%s+%s\r\n"
      "\r\n%.*s", sb.st_size, EGG_STRINGVER, EGG_PATCH, (int) sb.st_size, body);
    tputs(dcc[idx].sock, response, i);
    debug2("webui: tputs(): >>>%s<<< %i", response, i);
    if (munmap(body, sb.st_size) < 0) {
      putlog(LOG_MISC, "*", "WEBUI error: munmap(): %s", strerror(errno));
      return;
    }
  } else if (buf[5] == 'f') {
    debug0("webui: GET /favicon.ico");
    i = snprintf(response, sizeof response,
      "HTTP/1.1 200 \r\n" /* textual phrase is OPTIONAL */
      "Content-Length: %zu\r\n"
      "Content-Type: image/x-icon\r\n"
      "Server: Eggdrop/%s+%s\r\n" /* TODO: stealth_telnets */
      "\r\n",
      sizeof favicon_ico, EGG_STRINGVER, EGG_PATCH);
    memcpy(response + i, favicon_ico, sizeof favicon_ico);
    i += sizeof favicon_ico;

    tputs(dcc[idx].sock, response, i);
    debug1("webui: tputs(): %i", i);
  } else if (buf[5] == 'w') {
    debug0("webui: GET /w");
    buf = strstr(buf, WS_KEY);
    if (!buf) {
      putlog(LOG_MISC, "*", "WEBUI error: Sec-WebSocket-Key not found");
      return;
    }
    buf += sizeof WS_KEY;
    for(i = 0; i < WS_KEYLEN; i++)
      if (!buf[i]) {
        putlog(LOG_MISC, "*", "WEBUI error: Sec-WebSocket-Key too short");
        return;
      }
    debug0("webui: server requests websocket upgrade");

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
    SHA1_Update(&c, buf, KEYLEN);
    SHA1_Update(&c, WS_GUID, (sizeof WS_GUID) - 1);
    SHA1_Final(hash, &c);
#endif

    char out[WS_LEN + 1];
    /* TODO: remove assert / debug */
    if (b64_ntop(hash, sizeof hash, out, sizeof out) != WS_LEN) {
      putlog(LOG_MISC, "*", "WEBUI error: b64_ntop() != WS_LEN");
      return;
    }

    i = snprintf(response, sizeof response,
      "HTTP/1.1 101 Switching Protocols\r\n"
      "Upgrade: websocket\r\n"
      "Connection: Upgrade\r\n"
      "Sec-WebSocket-Accept: %s\r\n"
      "\r\n", out);
    tputs(dcc[idx].sock, response, i);
    debug2("webui: tputs(): >>>%s<<< %i", response, i);

    sock_list* socklist_i = &socklist[findsock(dcc[idx].sock)];
    socklist_i->flags |= SOCK_WS;


    socklist_i->flags &= ~ SOCK_BINARY; /* we need it for net.c sockgets(), is there better place to do this? */
    debug1("webui: unset flag SOCK_BINARY sock %li\n", dcc[idx].sock);
    strcpy(dcc[idx].host, "*"); /* important for later dcc_telnet_id wild_match, is there better place to do this? */
    /* .host becomes .nick in change_to_dcc_telnet_id() */
    debug4("webui: set flag SOCK_WS socklist %i idx %i sock %li status %lu\n", findsock(dcc[idx].sock), idx, dcc[idx].sock, dcc[idx].status);

    dcc[idx].status |= STAT_USRONLY; /* magick */
    for (i = 0; i < dcc_total; i++) /* quick hack, we need to link from idx, dont we? */
      if (!strcmp(dcc[i].nick, "(webui)")) {
        debug1("webui: found (webui) dcc %i\n", i);
        break;
      }

    /*
    for (int j = 0; j < dcc_total; j++) {
      debug4("dcc table %i %i %i %s", j, dcc[j].sock, dcc[j].ssl, dcc[j].host);
      debug2("             %s %s", dcc[j].nick, dcc[j].type->name);
    }
    */

    dcc[idx].u.other = NULL; /* fix ATTEMPTING TO FREE NON-MALLOC'D PTR: dccutil.c (561) */
    dcc_telnet_hostresolved2(idx, i);

    debug2("webui: CHANGEOVER -> idx %i sock %li\n", idx, dcc[idx].sock);
  } else /* TODO: send 404 or something ? */
    debug0("webui: 404");
  if ((dcc[idx].sock != -1) && (len == 511)) { /* sock == -1 if lostdcc() in dcc_telnet_hostresolved2() */
    /* read probable remaining bytes */
    SSL *ssl = socklist[findsock(dcc[idx].sock)].ssl;
    if (ssl)
      debug1("webui: SSL_read(): len %i", SSL_read(ssl, buf, 511));
    else
      debug1("webui: read(): len %li", read(dcc[idx].sock, buf, 511));
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

static void webui_dcc_telnet_hostresolved(int i)
{
    debug1("webui_dcc_telnet_hostresolved(%i)", i);
    changeover_dcc(i, &DCC_WEBUI_HTTP, 0);
    sockoptions(dcc[i].sock, EGG_OPTION_SET, SOCK_BINARY);
    dcc[i].u.other = NULL; /* important, else nfree() error in lostdcc on eof */
}

static void webui_frame(char **buf, unsigned int *len) {
  static uint8_t out[2048];

  /* no debug() or putlog() here or recursion */
  printf("webui: webui_frame() len %u\n", *len);
  //printf(">>>%s<<<", *buf);
  out[0] = 0x81; /* FIN + text frame */
  /* A server MUST NOT mask any frames that it sends to the client */
  if (*len < 0x7e) {
    out[1] = *len;
    /* TODO: we could offset buf and get rid of this memcpy() */
    memcpy(out + 2, *buf, *len);
    *buf = (char *) out;
    *len = *len + 2;
  } else {
    out[1] = 0x7e;
    uint16_t len2 = htons(*len);
    memcpy(out + 2, &len2, 2);
    /* TODO: we could offset buf and get rid of this memcpy() */
    memcpy(out + 4, *buf, *len);
    *buf = (char *) out;
    *len = *len + 4;
  }
  /* FIXME:len > 0xffff */
}

/* TODO: return error code ? */
static void webui_unframe(char *buf, int *len)
{
  int i;
  uint8_t *key, *payload;

  debug1("webui: webui_unframe(): len %i", *len);
  if (*len < 6) { /* TODO: better len check */

    /* TODO: return error code ? */
    putlog(LOG_MISC, "*", "WEBUI error: someone sent something other than WebSocket protocol");
    /*
    putlog(LOG_MISC, "*",
           "WEBUI error: %s sent something other than WebSocket protocol",
           iptostr(&dcc[idx].sockname.addr.sa));
    killsock(dcc[idx].sock);
    lostdcc(idx);
    */
    return;
  }
  if (buf[0] & 0x08) {
    putlog(LOG_MISC, "*", "WEBUI: fixme: sent connection close not handled yet");
    /*
    debug1("webui: webui_ws_activity(): %s sent connection close",
           iptostr(&dcc[idx].sockname.addr.sa));
    killsock(dcc[idx].sock);
    lostdcc(idx);
    */
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
  debug2("webui: webui_unframe(): payload >>>%.*s<<<", (int) *len, payload);

  memmove(buf, payload, *len);
  /* we switched back from binary sock to text sock for sockgets() needs this for dcc_telnet_id() */
  /* so now we have to add \r\n here :/ */
  strcpy(buf + *len, "\r\n");
  *len+= 2;
}

static char *webui_close(void)
{
  del_hook(HOOK_DCC_TELNET_HOSTRESOLVED, (Function) webui_dcc_telnet_hostresolved);
  del_hook(HOOK_WEBUI_FRAME, (Function) webui_frame);
  del_hook(HOOK_WEBUI_UNFRAME, (Function) webui_unframe);
  return NULL;
}

EXPORT_SCOPE char *webui_start();

static Function webui_table[] = {
  (Function) webui_start,
  (Function) webui_close,
  NULL,
  NULL,
};

char *webui_start(Function *global_funcs)
{
#ifdef TLS
  global = global_funcs;
  module_register(MODULE_NAME, webui_table, 0, 9);
  if (!module_depend(MODULE_NAME, "eggdrop", 109, 0)) {
    module_undepend(MODULE_NAME);
    return "This module requires Eggdrop 1.9.0 or later.";
  }
  add_hook(HOOK_DCC_TELNET_HOSTRESOLVED, (Function) webui_dcc_telnet_hostresolved);
  add_hook(HOOK_WEBUI_FRAME, (Function) webui_frame);
  add_hook(HOOK_WEBUI_UNFRAME, (Function) webui_unframe);
  return NULL;
#else
  return "Initialization failure: configured with --disable-tls or openssl not found";
#endif
}
