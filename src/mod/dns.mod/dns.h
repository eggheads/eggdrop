/*
 * dns.h - dns module header file
 *
 * Copyright (C) 1999  Eggheads
 * Written by Fabian Knittel <fknittel@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Borrowed from mtr  --  a network diagnostic tool
 * Copyright (C) 1997,1998  Matt Kimball <mkimball@xmission.com>
 * Released under GPL, as above.
 *
 * Non-blocking DNS portion --
 * Copyright (C) 1998  Simon Kirby <sim@neato.org>
 * Released under GPL, as above.
 */

#ifndef MOD_DNS_H
#define MOD_DNS_H

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
typedef unsigned int ip_t;

struct resolve {
    struct resolve *next;
    struct resolve *previous;
    struct resolve *nextid;
    struct resolve *previousid;
    struct resolve *nextip;
    struct resolve *previousip;
    struct resolve *nexthost;
    struct resolve *previoushost;
    time_t expiretime;
    char *hostn;
    ip_t ip;
    word id;
    byte state;
    byte sends;
};

enum {
    STATE_FINISHED,
    STATE_FAILED,
    STATE_PTRREQ,
    STATE_AREQ,
};

#define IS_PTR(x) (x->state == STATE_PTRREQ)
#define IS_A(x)   (x->state == STATE_AREQ)

#endif /* MOD_DNS_H */

