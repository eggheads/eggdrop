dnl configure.ac: this file is processed by autoconf to produce ./configure.

AC_PREREQ(2.61)
AC_INIT([Eggdrop],[1.9.0],[bugs@eggheads.org])
AC_COPYRIGHT([Copyright (C) 1999 - 2020 Eggheads Development Team])
AC_LANG([C])
AC_REVISION([m4_esyscmd([misc/getcommit])])
AC_CONFIG_SRCDIR(src/eggdrop.h)
AC_CONFIG_AUX_DIR(misc)
AC_CONFIG_HEADER(config.h)
AC_PREFIX_DEFAULT([\${HOME}/eggdrop])
AC_PRESERVE_HELP_ORDER

dnl config.h stuff
AH_TOP([/*
 * Copyright (C) 1997 Robey Pointer
 * Copyright (C) 1999 - 2020 Eggheads Development Team
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

#ifndef _EGG_CONFIG_H
#define _EGG_CONFIG_H])
AH_BOTTOM([#endif /* !_EGG_CONFIG_H */])

# Print start message
EGG_MSG_CONFIGURE_START
EGG_SAVE_PARAMETERS


# Check for a working C compiler
AC_PROG_CC([gcc cc clang])
EGG_CHECK_CC
AC_PROG_CC_C99
EGG_CHECK_CC_C99

# These 3 need to be done before any AC_COMPILE_IFELSE()'s.
AC_AIX
AC_ISC_POSIX
AC_MINIX

# Check C compiler characteristics.
EGG_CHECK_ICC
EGG_CHECK_CCPIPE
EGG_CHECK_CCWALL


# Check for executable extension.
EGG_EXEEXT


# Checks for programs.
AC_PROG_INSTALL
AC_CHECK_PROGS(MAKE, make gmake)
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LN_S
EGG_PROG_HEAD_1
EGG_PROG_STRIP
EGG_PROG_AWK
EGG_PROG_BASENAME
AC_CHECK_PROG(UNAME,uname,uname)


# Check if we will strip the binary
# Do this *before* EGG_CHECK_OS
EGG_ENABLE_STRIP


# Checks for system libraries.
EGG_CHECK_LIBS


# Test the OS and set the module linking settings.
EGG_OS_VERSION
EGG_CHECK_MODULE_SUPPORT
EGG_CHECK_OS


# Checks for header files.
EGG_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h locale.h netdb.h netinet/in.h stdio.h stdarg.h stddef.h sys/file.h sys/param.h sys/rusage.h sys/select.h sys/socket.h sys/time.h unistd.h wchar.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN
AC_C_INLINE
AC_C_FLEXIBLE_ARRAY_MEMBER
AC_TYPE_INTPTR_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TIMEZONE
AC_C_VOLATILE
AX_TYPE_SOCKLEN_T


# Create stdint.h C99 compatibility header.
AX_CREATE_STDINT_H([eggint.h])

# Checks for functions and their arguments.
AC_CHECK_FUNCS([dprintf explicit_bzero explicit_memset getrandom getrusage inet_aton isascii memset_s random rand lrand48 snprintf strlcpy vsnprintf])
AC_FUNC_SELECT_ARGTYPES
EGG_FUNC_B64_NTOP
AC_FUNC_MMAP


# Tcl version to recommend if no Tcl is found, and the site where it can be
# found for download.
tclrecommendver="8.6.X"
tclrecommendsite="ftp://ftp.tcl.tk/pub/tcl/tcl8_6/"

# Tcl header filenames.
tclheadernames="tcl.h"

# We save the cache (if used) here to speed things up if we can't find Tcl.
AC_CACHE_SAVE

# Misc Tcl checks.
EGG_TCL_ARG_WITH
EGG_TCL_WITH_TCLLIB
EGG_TCL_WITH_TCLINC
EGG_TCL_TCLCONFIG
EGG_TCL_CHECK_VERSION
EGG_TCL_CHECK_NOTIFIER_INIT
EGG_TCL_LUSH


# Debug options
EGG_DEBUG_ENABLE
EGG_DEBUG_DEFAULTS
EGG_DEBUG_OPTIONS
EGG_DEBUG_CFLAGS
EGG_ENABLE_DEBUG_CONTEXT

# Let the user set the handlen with a configure option
EGG_ARG_HANDLEN


# Check for IPv6
EGG_IPV6_STATUS
EGG_IPV6_ENABLE
EGG_IPV6_COMPAT


# Check for TLS
EGG_TLS_ENABLE
EGG_TLS_WITHSSL
EGG_TLS_DETECT


# Substitute Makefile variables.
EGG_SUBST_EGGVERSION
EGG_SUBST_DEST
EGG_SUBST_MOD_UPDIR


EGG_CATCH_MAKEFILE_REBUILD

dnl If we are cross-compiling, make a note of it so we can skip some tests in the Makefile
AS_IF([test "$cross_compiling" = "yes"], [
	EGG_CROSS_COMPILING=yes
], [
	EGG_CROSS_COMPILING=no
])
AC_SUBST(EGG_CROSS_COMPILING)


# Create Makefiles
AC_CONFIG_FILES([Makefile doc/Makefile scripts/Makefile src/Makefile src/compat/Makefile src/md5/Makefile src/mod/Makefile])
AC_OUTPUT


# Display configure end message
AC_MSG_RESULT
EGG_MSG_SUMMARY
EGG_MSG_WEIRDOS
EGG_MSG_CONFIGURE_END
