/* 
 * server.h -- part of server.mod
 * 
 * $Id: server.h,v 1.4 1999/12/15 02:33:00 guppy Exp $
 */
/* 
 * Copyright (C) 1997  Robey Pointer
 * Copyright (C) 1999  Eggheads
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
#ifndef _EGG_MOD_SERVER_SERVER_H
#define _EGG_MOD_SERVER_SERVER_H

#define check_tcl_ctcp(a,b,c,d,e,f) check_tcl_ctcpr(a,b,c,d,e,f,H_ctcp)
#define check_tcl_ctcr(a,b,c,d,e,f) check_tcl_ctcpr(a,b,c,d,e,f,H_ctcr)
#ifndef MAKING_SERVER
/* 4 - 7 */

#define botuserhost ((char *)(server_funcs[5]))
#define quiet_reject (*(int *)(server_funcs[6]))
#define serv (*(int *)(server_funcs[7]))
/* 8 - 11 */
#define flud_thr (*(int*)(server_funcs[8]))
#define flud_time (*(int*)(server_funcs[9]))
#define flud_ctcp_thr (*(int*)(server_funcs[10]))
#define flud_ctcp_time (*(int*)(server_funcs[11]))
/* 12 - 15 */
#define match_my_nick ((int(*)(char *))server_funcs[12])
#define check_tcl_flud ((int (*)(char *,char *,struct userrec *,char *,char *))server_funcs[13])
#define fixfrom ((void (*)(char *))server_funcs[14])
#define answer_ctcp (*(int *)(server_funcs[15]))
/* 16 - 19 */
#define trigger_on_ignore (*(int *)(server_funcs[16]))
#define check_tcl_ctcpr ((int(*)(char*,char*,struct userrec*,char*,char*,char*,p_tcl_bind_list))server_funcs[17])
#define detect_avalanche ((int(*)(char *))server_funcs[18])
#define nuke_server ((void(*)(char *))server_funcs[19])
/* 20 - 22 */
#define newserver ((char *)(server_funcs[20]))
#define newserverport (*(int *)(server_funcs[21]))
#define newserverpass ((char *)(server_funcs[22]))
/* 23 - 26 */
#define cycle_time (*(int *)(server_funcs[23]))
#define default_port (*(int *)(server_funcs[24]))
#define server_online (*(int *)(server_funcs[25]))
#define min_servs (*(int *)(server_funcs[26]))
/* 27 - 30 */
#define H_raw (*(p_tcl_bind_list *)(server_funcs[27]))
#define H_wall (*(p_tcl_bind_list *)(server_funcs[28]))
#define H_msg (*(p_tcl_bind_list *)(server_funcs[29]))
#define H_msgm (*(p_tcl_bind_list *)(server_funcs[30]))
/* 31 - 34 */
#define H_notc (*(p_tcl_bind_list *)(server_funcs[31]))
#define H_flud (*(p_tcl_bind_list *)(server_funcs[32]))
#define H_ctcp (*(p_tcl_bind_list *)(server_funcs[33]))
#define H_ctcr (*(p_tcl_bind_list *)(server_funcs[34]))
/* 35 - 36 */
#define ctcp_reply ((char *)(server_funcs[35]))
#define get_altbotnick ((char *(*)(void))(server_funcs[36]))
#endif				/* MAKING_SERVER */

#endif				/* _EGG_MOD_SERVER_SERVER_H */
