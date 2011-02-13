/*
 * dns.h -- part of dns.mod
 *   dns module header file
 *
 * Written by Fabian Knittel <fknittel@gmx.de>
 *
 * $Id: dns.h,v 1.20 2011/02/13 14:19:33 simple Exp $
 */
/*
 * Copyright (C) 1999 - 2011 Eggheads Development Team
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

/*
 * Borrowed from mtr  --  a network diagnostic tool
 * Copyright (C) 1997,1998  Matt Kimball <mkimball@xmission.com>
 * Released under GPL, as above.
 *
 * Non-blocking DNS portion --
 * Copyright (C) 1998  Simon Kirby <sim@neato.org>
 * Released under GPL, as above.
 */

#ifndef _EGG_MOD_DNS_DNS_H
#define _EGG_MOD_DNS_DNS_H

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
  IP ip;
  u_16bit_t id;
  u_8bit_t state;
  u_8bit_t sends;
};

enum resolve_states {
  STATE_FINISHED,
  STATE_FAILED,
  STATE_PTRREQ,
  STATE_AREQ
};

#define IS_PTR(x) (x->state == STATE_PTRREQ)
#define IS_A(x)   (x->state == STATE_AREQ)

#ifdef DEBUG_DNS
#  define ddebug0                debug0
#  define ddebug1                debug1
#  define ddebug2                debug2
#  define ddebug3                debug3
#  define ddebug4                debug4
#else /* !DEBUG_DNS */
#  define ddebug0(x)
#  define ddebug1(x, x1)
#  define ddebug2(x, x1, x2)
#  define ddebug3(x, x1, x2, x3)
#  define ddebug4(x, x1, x2, x3, x4)
#endif /* !DEBUG_DNS */

#endif /* _EGG_MOD_DNS_DNS_H */
