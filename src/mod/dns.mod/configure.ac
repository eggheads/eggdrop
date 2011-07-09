dnl configure.ac: this file is processed by autoconf to produce ./configure.
dnl
dnl $Id: configure.ac,v 1.13 2011/07/09 15:07:48 thommey Exp $

AC_PREREQ(2.59)

sinclude(../eggmod.m4)

AC_INIT([Eggdrop DNS Module],[1.6.21],[bugs@eggheads.org])

AC_CONFIG_SRCDIR(coredns.c)
AC_CONFIG_AUX_DIR(../../../misc)

AC_COPYRIGHT([Copyright (C) 1999 - 2011 Eggheads Development Team])
AC_REVISION($Revision: 1.13 $)

dns_reslib_avail="false"

AC_HEADER_RESOLV

for lib in '' -lresolv -lbind -lsocket
do
  LIBS="$lib"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([
      #ifdef HAVE_SYS_TYPES_H
      #  include <sys/types.h>
      #endif
      #ifdef HAVE_NETINET_IN_H
      #  include <netinet/in.h>   /* inet_ functions / structs */
      #endif
      #ifdef HAVE_ARPA_NAMESER_H
      #  include <arpa/nameser.h> /* DNS HEADER struct */
      #endif
      #ifdef HAVE_NETDB_H
      #  include <netdb.h>
      #endif
      #include <resolv.h>
    ], [
      res_init();
      res_mkquery(0, NULL, 0, 0, NULL, 0, NULL, NULL, 0);
      ;
      return 0;
    ])
  ], [
    RESLIB="$lib"
    dns_reslib_avail="true"
    break
  ])
done

AC_MSG_CHECKING([for a working resolver library])
if test "$dns_reslib_avail" = "true"; then
  AC_MSG_RESULT([yes ($LIBS)])
else
  AC_MSG_RESULT(no)
  # Cygwin checks.
  case $host_os in
    *cygwin* )
      AC_MSG_CHECKING([for /usr/local/bind/lib/libbind.a])
      if test -r /usr/local/bind/lib/libbind.a; then
        AC_MSG_RESULT([yes])
        RESLIB="${RESLIB} -L/usr/local/bind/lib -lbind"
        RESINCLUDE="-I/usr/local/bind/include"
        dns_reslib_avail="true"
      else
        AC_MSG_RESULT([no])
      fi
    ;;
  esac
fi

if test "${dns_reslib_avail}" = "false"; then
  cat >&2 <<EOF
configure: warning:

  Your system provides no functional resolver library. The DNS
  module will therefore be disabled.

EOF
  EGG_REMOVE_MOD(dns)
fi

AC_SUBST(RESLIB)
AC_SUBST(RESINCLUDE)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
