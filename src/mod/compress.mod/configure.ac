dnl configure.ac: this file is processed by autoconf to produce ./configure.

AC_PREREQ(2.58)

sinclude(../eggmod.m4)

AC_INIT([Eggdrop Compress Module],[1.9.1],[bugs@eggheads.org])

AC_CONFIG_SRCDIR(compress.c)
AC_CONFIG_AUX_DIR(../../../misc)

AC_COPYRIGHT([Copyright (C) 1999 - 2021 Eggheads Development Team])
AC_REVISION([m4_esyscmd([../../../misc/getcommit])])

AC_PROG_CC([gcc cc clang])

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
