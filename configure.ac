dnl configure.ac: this file is processed by autoconf to produce ./configure.
dnl
dnl $Id: configure.ac,v 1.14 2004/07/25 22:15:12 wcc Exp $

AC_PREREQ(2.59)
AC_INIT([Eggdrop],[1.6.17],[bugs@eggheads.org])
AC_COPYRIGHT([Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Eggheads Development Team])
AC_REVISION($Revision: 1.14 $)
AC_CONFIG_SRCDIR(src/eggdrop.h)
AC_CONFIG_AUX_DIR(misc)
AC_CONFIG_HEADER(config.h)
AC_PREFIX_DEFAULT([\${HOME}/eggdrop])

dnl config.h stuff
AH_TOP([#ifndef _EGG_CONFIG_H
#define _EGG_CONFIG_H])
AH_BOTTOM([#endif /* !_EGG_CONFIG_H */])

# Print start message
EGG_MSG_CONFIGURE_START
EGG_SAVE_PARAMETERS


# Check for a working C compiler
AC_PROG_CC
EGG_CHECK_CC

# These 3 need to be done before any AC_COMPILE_IFELSE()'s.
AC_AIX
AC_ISC_POSIX
AC_MINIX

# Check C compiler characteristics.
EGG_CHECK_CCWALL
EGG_CHECK_CCPIPE


# Checks for programs.
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LN_S
EGG_PROG_HEAD_1
EGG_PROG_STRIP
EGG_PROG_AWK
EGG_PROG_BASENAME
AC_CHECK_PROG(UNAME,uname,uname)


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
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h locale.h netdb.h netinet/in.h std_args.h stdarg.h stddef.h sys/file.h sys/param.h sys/rusage.h sys/select.h sys/socket.h sys/time.h unistd.h wchar.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TIMEZONE
AC_C_VOLATILE
EGG_CHECK_SOCKLEN_T


# Check the sizes of various types.
AC_CHECK_SIZEOF(long, 0)
AC_CHECK_SIZEOF(int, 0)
AC_CHECK_SIZEOF(short, 0)


# Checks for functions and their arguments.
AC_CHECK_FUNCS([clock dprintf fsync getdtablesize getrusage inet_aton isascii mbrlen memcpy memset random rename setpgid sigaction sigemptyset snprintf srandom strcasecmp strncasecmp uname vsnprintf])
AC_FUNC_SELECT_ARGTYPES
EGG_FUNC_VPRINTF
AC_FUNC_STRFTIME
AC_FUNC_MMAP


# Make sure we are using the correct result from a broken libsafe sscanf().
EGG_CHECK_LIBSAFE_SSCANF


# Check for executable extension.
EGG_EXEEXT


# Tcl version to recomend if no Tcl is found, and the site where it can be
# found for download.
tclrecommendver="8.4.6"
tclrecommendsite="ftp://tcl.activestate.com/pub/tcl/tcl8_4/"


# Tcl library filename prefixes, suffixes, and search paths.
tcllibnames="tcl8.4 tcl84 tcl8.3 tcl83 tcl8.2 tcl82 tcl8.1 tcl81 \
             tcl8.0 tcl80 tcl tcl7.6 tcl76 tcl7.5 tcl75 tcl7.4 tcl74 tcl7.3 \
             tcl73 tcl7.2 tcl72 tcl7.1 tcl71 tcl7.0 tcl70 tcl8.5 tcl85"
tcllibextensions=".so .so.1 .so.1.0 .so.1.2 .dll .dylib .a .sl"
tcllibpaths="$HOME/lib $HOME/tcl/lib /usr/local/lib /usr/lib /lib \
             /usr/local/lib/tcl8.4 /usr/local/lib/tcl8.3 \
             /usr/lib/tcl8.4 /usr/lib/tcl8.3 \
             /usr/local/lib/tcl8.5 /usr/lib/tcl8.5 \
             /usr/local/pkgs/tcl/lib /sys/lib \
             /usr/i486-linuxaout/lib /beos/system/lib $HOME"



# Tcl header filenames and search paths.
tclheadernames="tcl.h"
tclheaderpaths="$HOME/include $HOME/tcl/include /usr/local/include /usr/include \
                /usr/local/include/tcl8.3 /usr/local/include/tcl8.4 \
                /usr/include/tcl8.3 /usr/include/tcl8.4 \
                /usr/local/include/tcl8.5 /usr/include/tcl8.5 \
                /usr/local/pkgs/tcl/include /sys/include \
                /beos/system/include /beos/devel/include $HOME"


# Misc Tcl checks. We save the cache (if used) here to speed things up if we
# can't find Tcl.
AC_CACHE_SAVE
EGG_TCL_ARG_WITH
EGG_TCL_ENV
EGG_TCL_WITH_TCLLIB
EGG_TCL_WITH_TCLINC
EGG_TCL_FIND_LIBRARY
EGG_TCL_FIND_HEADER
EGG_TCL_CHECK_LIBRARY
EGG_TCL_CHECK_HEADER
EGG_TCL_DETECT_CHANGE
EGG_TCL_CHECK_VERSION
EGG_TCL_CHECK_PRE70
EGG_TCL_TESTLIBS
EGG_TCL_CHECK_FREE
EGG_TCL_ENABLE_THREADS
EGG_TCL_CHECK_THREADS
EGG_TCL_LIB_REQS
EGG_TCL_LUSH


# Substitute Makefile variables.
EGG_SUBST_EGGVERSION
EGG_SUBST_DEST
EGG_SUBST_MOD_UPDIR


EGG_CATCH_MAKEFILE_REBUILD


# Create Makefiles
AC_CONFIG_FILES([Makefile doc/Makefile scripts/Makefile src/Makefile src/compat/Makefile src/md5/Makefile src/mod/Makefile])
AC_OUTPUT


# Display configure end message
AC_MSG_RESULT
EGG_MSG_WEIRDOS
EGG_MSG_CONFIGURE_END
