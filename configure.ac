dnl configure.ac: this file is processed by autoconf to produce ./configure.
dnl
dnl $Id: configure.ac,v 1.5 2004/04/09 04:18:07 wcc Exp $
dnl

AC_PREREQ(2.59)
AC_INIT([Eggdrop],[1.6.16],[bugs@eggheads.org])
AC_COPYRIGHT([Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Eggheads Development Team])
AC_REVISION($Revision: 1.5 $)
AC_CONFIG_SRCDIR(src/eggdrop.h)
AC_CONFIG_AUX_DIR(misc)
AC_CONFIG_HEADER(config.h)
AC_PREFIX_DEFAULT([\${HOME}/eggdrop])

EGG_MSG_CONFIGURE_START
EGG_SAVE_PARAMETERS

# Setup build environment
AC_PROG_CC
EGG_CHECK_CC

# Crazy machines
AC_AIX
AC_ISC_POSIX
AC_MINIX

# Speedup compile
EGG_CHECK_CCPIPE

# Checks for programs
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LN_S
EGG_PROG_HEAD_1
EGG_PROG_STRIP
EGG_PROG_AWK
EGG_PROG_BASENAME
AC_CHECK_PROG(UNAME,uname,uname)

# Test the os and set the module linking settings
EGG_CHECK_OS

# Checks for system libraries
EGG_CHECK_LIBS

# Checks for header files
EGG_HEADER_STDC
AC_HEADER_DIRENT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h locale.h netdb.h netinet/in.h stddef.h sys/file.h sys/param.h sys/rusage.h sys/select.h sys/socket.h sys/time.h unistd.h wchar.h])
AC_HEADER_TIME

# Checks for typedefs, structures, and compiler characteristics
AC_C_CONST
AC_C_BIGENDIAN
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_C_VOLATILE

AC_CHECK_SIZEOF(long, 0)
AC_CHECK_SIZEOF(int, 0)

# Checks for functions
AC_CHECK_FUNCS(clock dlopen dprintf fsync getdtablesize getrusage inet_aton isascii memcpy memset random rename setpgid sigaction sigemptyset snprintf srandom strcasecmp strncasecmp uname vsnprintf)
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_VPRINTF
EGG_FUNC_VPRINTF
AC_FUNC_STRFTIME

# Make sure we are using the correct result from a broken libsafe sscanf
EGG_CHECK_LIBSAFE_SSCANF

# Checks for executable extension
EGG_EXEEXT

# Begin Tcl checks
# Latest tested Tcl version to recommend if Tcl isn't found
tclrecommendver="8.3.4"

# Site recommended to download Tcl from
tclrecommendsite="ftp://tcl.activestate.com/pub/tcl/tcl8_3/"

# Tcl library filename prefixes (also used for Tcl header dir on FreeBSD)
tcllibnames="tcl tcl8.5 tcl85 tcl8.4 tcl84 tcl8.3 tcl83 tcl8.2 \
        tcl82 tcl8.1 tcl81 tcl8.0 tcl80 tcl7.6 tcl76 tcl7.5 tcl75 \
        tcl7.4 tcl74 tcl7.3 tcl73 tcl7.2 tcl72 tcl7.1 tcl71 tcl7.0 tcl70"

# Tcl library filename suffixes
tcllibextensions=".so .so.1 .so.1.0 .so.1.2 .a .sl .dll .dylib"

# Tcl library search paths
tcllibpaths="/usr/local/lib /usr/local/pkgs/tcl/lib \
	/usr/lib /lib /usr/i486-linuxaout/lib \
	/beos/system/lib /sys/lib /usr/local/lib/tcl8.3 /usr/local/lib/tcl8.4 \
        /usr/lib/tcl8.3 /usr/lib/tcl8.4 $HOME/lib $HOME/tcl/lib $HOME"

# Tcl header filenames
tclheadernames="tcl.h"

# Tcl header search paths
tclheaderpaths="/usr/local/include /usr/local/pkgs/tcl/include \
	/usr/include /beos/system/include /beos/devel/include \
	/sys/include /usr/local/include/tcl8.3 /usr/local/include/tcl8.4 \
        /usr/include/tcl8.3 /usr/include/tcl8.4 $HOME/include \
        $HOME/tcl/include $HOME"

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
# End Tcl checks

EGG_FUNC_DLOPEN

# Substitute Makefile variables
EGG_SUBST_EGGVERSION
EGG_SUBST_DEST
EGG_SUBST_MOD_UPDIR

EGG_CATCH_MAKEFILE_REBUILD
AC_CONFIG_FILES([Makefile doc/Makefile scripts/Makefile src/Makefile src/compat/Makefile src/md5/Makefile src/mod/Makefile])
AC_OUTPUT

EGG_MSG_CONFIGURE_END
