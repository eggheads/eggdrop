/*
 * dns.h
 *   stuff used by dns.c
 *
 * $Id: dns.h,v 1.2 2010/08/05 18:12:05 pseudo Exp $
 */
/*
 * Written by Fabian Knittel <fknittel@gmx.de>
 *
 * Copyright (C) 1999 - 2010 Eggheads Development Team
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

#ifndef _EGG_DNS_H
#define _EGG_DNS_H

typedef struct {
  char *name;
  int (*expmem) (void *);
  void (*event) (sockname_t *, char *, int, void *);
} devent_type;

typedef struct {
  char *proc;                   /* Tcl proc                       */
  char *paras;                  /* Additional parameters          */
  sockname_t sockname;
} devent_tclinfo_t;

typedef struct devent_str {
  struct devent_str *next;      /* Pointer to next dns_event      */
  devent_type *type;
  u_8bit_t lookup;              /* RES_IPBYHOST or RES_HOSTBYIP   */
  union {
    sockname_t *ip_addr;	/* Pointer to address placeholder */
    char *hostname;             /* Hostname                       */
  } res_data;
  void *other;                  /* Data specific to the event type */
} devent_t;

#endif /* _EGG_DNS_H */
