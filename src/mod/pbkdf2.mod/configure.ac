dnl configure.ac: this file is processed by autoconf to produce ./configure.

AC_PREREQ(2.59)

sinclude(../eggmod.m4)
sinclude(ax_check_openssl.m4)

AC_INIT([Eggdrop PBKDF2 Module],[1.9.0],[bugs@eggheads.org])

AC_CONFIG_SRCDIR(pbkdf2.c)

AC_COPYRIGHT([Copyright (C) 1999 - 2020 Eggheads Development Team])
AC_REVISION([m4_esyscmd([../../../misc/getcommit])])

AX_CHECK_OPENSSL([
  AC_MSG_CHECKING([whether openssl supports pbkdf2 and sha256])
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <openssl/evp.h>], [PKCS5_PBKDF2_HMAC(NULL, 0, NULL, 0, 0, EVP_sha256(), 0, NULL)])],
    [
      AC_MSG_RESULT([yes])
      have_openssl="yes"
    ], [
      AC_MSG_RESULT([no])
      have_openssl="no"
    ])
  ], [
    have_openssl="no"
  ])

if test "$have_openssl" = "yes"; then
  LDFLAGS="$LDFLAGS $OPENSSL_LDFLAGS"
  LIBS="$OPENSSL_LIBS $LIBS"
  CPPFLAGS="$OPENSSL_INCLUDES $CPPFLAGS -DPBKDF2_OPENSSL=1"
else
  cat >&2 <<EOF
configure: warning:

  Your system does not provide a working openssl library or the necessary openssl
  functions EVP_sha256() and PKCS5_PBKDF2_HMAC(). The pbkdf2 module will therefore
  be disabled.

EOF
  EGG_REMOVE_MOD(pbkdf2)
fi

AC_SUBST([LDFLAGS])
AC_SUBST([LIBS])
AC_SUBST([CPPFLAGS])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
