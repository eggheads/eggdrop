/*
 * ctcp.h -- part of ctcp.mod
 *   all the defines for ctcp.c
 *
 * $Id: ctcp.h,v 1.14 2011/02/13 14:19:33 simple Exp $
 */
/*
 * Copyright (C) 1997 Robey Pointer
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

#ifndef _EGG_MOD_CTCP_CTCP_H
#define _EGG_MOD_CTCP_CTCP_H

#define CLIENTINFO "SED VERSION CLIENTINFO USERINFO ERRMSG FINGER TIME ACTION DCC UTC PING ECHO  :Use CLIENTINFO <COMMAND> to get more specific information"
#define CLIENTINFO_SED "SED contains simple_encrypted_data"
#define CLIENTINFO_VERSION "VERSION shows client type, version and environment"
#define CLIENTINFO_CLIENTINFO "CLIENTINFO gives information about available CTCP commands"
#define CLIENTINFO_USERINFO "USERINFO returns user settable information"
#define CLIENTINFO_ERRMSG "ERRMSG returns error messages"
#define CLIENTINFO_FINGER "FINGER shows real name, login name and idle time of user"
#define CLIENTINFO_TIME "TIME tells you the time on the user's host"
#define CLIENTINFO_ACTION "ACTION contains action descriptions for atmosphere"
#define CLIENTINFO_DCC "DCC requests a direct_client_connection"
#define CLIENTINFO_UTC "UTC substitutes the local timezone"
#define CLIENTINFO_PING "PING returns the arguments it receives"
#define CLIENTINFO_ECHO "ECHO returns the arguments it receives"

#endif /* _EGG_MOD_CTCP_CTCP_H */
