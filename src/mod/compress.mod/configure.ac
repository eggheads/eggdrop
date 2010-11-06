dnl configure.ac: this file is processed by autoconf to produce ./configure.
dnl
dnl $Id: configure.ac,v 1.2 2010/07/27 13:05:43 pseudo Exp $

AC_PREREQ(2.59)

sinclude(../eggmod.m4)

AC_INIT([Eggdrop Compress Module],[1.8.0],[bugs@eggheads.org])

AC_CONFIG_SRCDIR(compress.c)
AC_CONFIG_AUX_DIR(../../../misc)

AC_COPYRIGHT([Copyright (C) 1999 - 2010 Eggheads Development Team])
AC_REVISION($Revision: 1.2 $)

# Check for gzopen() in -lz.
AC_CHECK_LIB(z, gzopen, [ZLIB="-lz"], [])
AC_CHECK_HEADER(zlib.h)

# Disable the module if either the header file or the library are missing.
if test "x${ZLIB}" = "x"; then
  cat >&2 <<EOF
configure: warning:

  Your system does not provide a working zlib compression library. The
  compress module will therefore be disabled.

EOF
  EGG_REMOVE_MOD(compress)
else
  if test "${ac_cv_header_zlib_h}" != "yes"; then
    cat >&2 <<EOF
configure: warning:

  Your system does not provide the necessary zlib header files. The
  compress module will therefore be disabled.

EOF
    EGG_REMOVE_MOD(compress)
  fi
fi

AC_SUBST(ZLIB)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
