dnl aclocal.m4: macros autoconf uses when building configure from configure.ac
dnl
dnl Copyright (C) 1999 - 2019 Eggheads Development Team
dnl
dnl This program is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU General Public License
dnl as published by the Free Software Foundation; either version 2
dnl of the License, or (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
dnl

dnl Load tcl macros
builtin(include,m4/tcl.m4)

dnl Load gnu autoconf archive macros
builtin(include,m4/ax_create_stdint_h.m4)
builtin(include,m4/ax_lib_socket_nsl.m4)
builtin(include,m4/ax_type_socklen_t.m4)


dnl
dnl Message macros.
dnl


dnl EGG_MSG_CONFIGURE_START()
dnl
AC_DEFUN([EGG_MSG_CONFIGURE_START],
[
  AC_MSG_RESULT
  AC_MSG_RESULT([This is Eggdrop's GNU configure script.])
  AC_MSG_RESULT([It's going to run a bunch of tests to hopefully make your compile])
  AC_MSG_RESULT([work without much twiddling.])
  AC_MSG_RESULT
])


dnl EGG_MSG_CONFIGURE_END()
dnl
AC_DEFUN([EGG_MSG_CONFIGURE_END],
[
  AC_MSG_RESULT([Type 'make config' to configure the modules, or type 'make iconfig'])
  AC_MSG_RESULT([to interactively choose which modules to compile.])
  AC_MSG_RESULT
])

dnl EGG_MSG_SUMMARY()
dnl
dnl Print summary with OS, TLS and TCL info.
AC_DEFUN([EGG_MSG_SUMMARY],
[
  AC_MSG_RESULT([Operating System: $egg_cv_var_system_type $egg_cv_var_system_release])
  AC_MSG_RESULT([IPv6 Support: $ipv6_enabled])
  ADD=""
  if test "x$TCL_THREADS" = "x1"; then
    ADD=" (threaded)"
  fi
  AC_MSG_RESULT([Tcl version: $TCL_PATCHLEVEL$ADD])
  ADD=""
  if test "x$tls_enabled" = "xyes"; then
    EGG_FIND_SSL_VERSION
    if test "x$tls_version" != "x"; then
      ADD=" ($tls_version)"
    fi
  fi
  AC_MSG_RESULT([SSL/TLS Support: $tls_enabled$ADD])
  AC_MSG_RESULT
])

dnl EGG_FIND_SSL_VERSION()
dnl
dnl Tries to find the SSL version used
AC_DEFUN([EGG_FIND_SSL_VERSION],
[
  tmpout="tmpfile"
  if test ! -e "tmp.c" && test ! -e "$tmpout"; then
    cat >tmp.c <<EOF
#include <openssl/opensslv.h>
#include <stdio.h>
int main(void) {
 printf("%s\n", OPENSSL_VERSION_TEXT);
 return 0;
}
EOF
    $CC $SSL_INCLUDES tmp.c -o "$tmpout" >/dev/null 2>&1
    if test -x "./$tmpout"; then
      tls_version=$("./$tmpout")
      tls_versionf=
    fi
    rm -f "$tmpout" tmp.c >/dev/null 2>&1
  fi
])

dnl EGG_MSG_WEIRDOS()
dnl
dnl Print some messages at the end of configure to give extra information to
dnl users of 'weird' operating systems.
dnl
AC_DEFUN([EGG_MSG_WEIRDOS],
[
  if test "$UNKNOWN_OS" = yes; then
    AC_MSG_RESULT([WARNING:])
    AC_MSG_RESULT
    AC_MSG_RESULT([  Unknown Operating System: $egg_cv_var_system_type $egg_cv_var_system_release])
    AC_MSG_RESULT
    AC_MSG_RESULT([  Module support has been disabled for this build.])
    AC_MSG_RESULT
    AC_MSG_RESULT([  Please let us know what type of system this is by e-mailing])
    AC_MSG_RESULT([  bugs@eggheads.org. The output of uname -a, and some other basic])
    AC_MSG_RESULT([  information about the OS should be included.])
    AC_MSG_RESULT
  else
    if test "$WEIRD_OS" = yes; then
      AC_MSG_RESULT([WARNING:])
      AC_MSG_RESULT
      AC_MSG_RESULT([  The operating system you are using has not yet had a great])
      AC_MSG_RESULT([  deal of testing with Eggdrop. For this reason, this compile])
      AC_MSG_RESULT([  will default to "make static".])
      AC_MSG_RESULT
      AC_MSG_RESULT([  To enable module support, type "make eggdrop" instead of just])
      AC_MSG_RESULT([  "make" after you run "make config" (or "make iconfig").])
      AC_MSG_RESULT
      AC_MSG_RESULT([  As we have not done a sufficient amount of testing on this])
      AC_MSG_RESULT([  OS, your feedback is greatly appreciated. Please let us know])
      AC_MSG_RESULT([  at bugs@eggheads.org if there are any problems compiling with])
      AC_MSG_RESULT([  module support, or if you got it to work :)])
      AC_MSG_RESULT
    fi
    AC_MSG_RESULT([If you experience any problems compiling Eggdrop, please read the])
    AC_MSG_RESULT([compile guide, found in doc/COMPILE-GUIDE.])
    AC_MSG_RESULT
  fi
])


dnl EGG_APPEND_VAR()
dnl
dnl Append a non-empty string to a variable
dnl
dnl $1 = variable
dnl $2 = string 
dnl
AC_DEFUN([EGG_APPEND_VAR],
[
  if test "x$2" != x; then
    if test "x$$1" = x; then
      $1="$2"
    else
      $1="$$1 $2"
    fi
  fi
])   


dnl
dnl Compiler checks.
dnl


dnl EGG_CHECK_CC()
dnl
dnl Check for a working C compiler.
dnl
AC_DEFUN([EGG_CHECK_CC],
[
  if test "x$cross_compiling" = x; then
    cat << 'EOF' >&2
configure: error:

  This system does not appear to have a working C compiler.
  A working C compiler is required to compile Eggdrop.

EOF
    exit 1
  fi
])


dnl EGG_HEADER_STDC()
dnl
AC_DEFUN([EGG_HEADER_STDC],
[
  if test "$ac_cv_header_stdc" = no; then
    cat << 'EOF' >&2
configure: error:

  Your system must support ANSI C Header files.
  These are required for the language support. Sorry.

EOF
    exit 1
  fi
])


dnl EGG_CHECK_ICC()
dnl
dnl Check for Intel's C compiler. It attempts to emulate gcc but doesn't
dnl accept all the standard gcc options.
dnl
dnl
AC_DEFUN([EGG_CHECK_ICC],[
  AC_CACHE_CHECK([for icc], egg_cv_var_cc_icc, [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#if !(defined(__ICC) || defined(__ECC) || defined(__INTEL_COMPILER))
  "Toto, I've a feeling we're not in Kansas anymore."
#endif
    ]])], [
      egg_cv_var_cc_icc="yes"
    ], [
      egg_cv_var_cc_icc="no"
    ])
  ])

  if test "$egg_cv_var_cc_icc" = yes; then
    ICC="yes"
  else
    ICC="no"
  fi
])


dnl EGG_CHECK_CCPIPE()
dnl
dnl This macro checks whether or not the compiler supports the `-pipe' flag,
dnl which speeds up the compilation.
dnl
AC_DEFUN([EGG_CHECK_CCPIPE],
[
  if test "$GCC" = yes && test "$ICC" = no; then
    AC_CACHE_CHECK([whether the compiler understands -pipe], egg_cv_var_ccpipe, [
      ac_old_CC="$CC"
      CC="$CC -pipe"
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[ return(0); ]])], [
        egg_cv_var_ccpipe="yes"
      ], [
        egg_cv_var_ccpipe="no"
      ])
      CC="$ac_old_CC"
    ])

    if test "$egg_cv_var_ccpipe" = yes; then
      EGG_APPEND_VAR(CFLAGS, -pipe)
    fi
  fi
])


dnl EGG_CHECK_CCWALL()
dnl
dnl See if the compiler supports -Wall.
dnl
AC_DEFUN([EGG_CHECK_CCWALL],
[
  if test "$GCC" = yes && test "$ICC" = no; then
    AC_CACHE_CHECK([whether the compiler understands -Wall], egg_cv_var_ccwall, [
      ac_old_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS -Wall"
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[ return(0); ]])], [
        egg_cv_var_ccwall="yes"
      ], [
        egg_cv_var_ccwall="no"
      ])
      CFLAGS="$ac_old_CFLAGS"
    ])

    if test "$egg_cv_var_ccwall" = yes; then
      EGG_APPEND_VAR(CFLAGS, -Wall)
    fi
  fi
])


dnl
dnl Checks for types and functions.
dnl


dnl EGG_FUNC_VPRINTF()
dnl
AC_DEFUN([EGG_FUNC_VPRINTF],
[
  AC_FUNC_VPRINTF
  if test "$ac_cv_func_vprintf" = no; then
    cat << 'EOF' >&2
configure: error:

  Your system does not have the vprintf/vsprintf/sprintf libraries.
  These are required to compile almost anything. Sorry.

EOF
    exit 1
  fi
])


dnl
dnl Checks for programs.
dnl


dnl EGG_PROG_HEAD_1()
dnl
dnl This macro checks for the proper 'head -1' command variant to use.
dnl
AC_DEFUN([EGG_PROG_HEAD_1],
[
  cat << 'EOF' > conftest.head
a
b
c
EOF

  for ac_prog in 'head -n 1' 'head -1' 'sed 1q'; do
    AC_MSG_CHECKING([whether $ac_prog works])
    AC_CACHE_VAL([ac_cv_prog_HEAD_1], [
      if test -n "$HEAD_1"; then
        # Let the user override the test.
        ac_cv_prog_HEAD_1="$HEAD_1"
      else
        if test `cat conftest.head | $ac_prog` = a; then
          ac_cv_prog_HEAD_1="$ac_prog"
        fi
      fi
    ])
    if test -n "$ac_cv_prog_HEAD_1"; then
      AC_MSG_RESULT([yes])
      break
    else
      AC_MSG_RESULT([no])
    fi
  done

  if test "x$ac_cv_prog_HEAD_1" = x; then
    cat << 'EOF' >&2
configure: error:

  This system seems to lack a working 'head -1' or 'head -n 1' command.
  A working 'head -1' (or equivalent) command is required to compile Eggdrop.

EOF
    exit 1
  fi

  rm -f conftest.head
  HEAD_1="$ac_cv_prog_HEAD_1"
  AC_SUBST(HEAD_1)
])


dnl EGG_PROG_STRIP()
dnl
AC_DEFUN([EGG_PROG_STRIP],
[
  AC_CHECK_PROG(STRIP, strip, strip)
  if test "x$STRIP" = x; then
    STRIP=touch
  fi
])


dnl EGG_PROG_AWK()
dnl
AC_DEFUN([EGG_PROG_AWK],
[
  AC_PROG_AWK
  if test "x$AWK" = x; then
    cat << 'EOF' >&2
configure: error:

  This system seems to lack a working 'awk' command.
  A working 'awk' command is required to compile Eggdrop.

EOF
    exit 1
  fi
])


dnl EGG_PROG_BASENAME()
dnl
AC_DEFUN([EGG_PROG_BASENAME],
[
  AC_CHECK_PROG(BASENAME, basename, basename)
  if test "x$BASENAME" = x; then
    cat << 'EOF' >&2
configure: error:

  This system seems to lack a working 'basename' command.
  A working 'basename' command is required to compile Eggdrop.

EOF
    exit 1
  fi
])


dnl EGG_ENABLE_STRIP()
dnl
AC_DEFUN([EGG_ENABLE_STRIP],
[
  AC_ARG_ENABLE([strip],
                [  --enable-strip          enable stripping of binaries],
                [enable_strip="$enableval"],
                [enable_strip="no"])

  if test "$enable_strip" = yes; then
    cat << 'EOF' >&2

configure: WARNING:

  Stripping the executable, while saving some disk space, will make bug
  reports nearly worthless. If Eggdrop crashes and you wish to report
  a bug, you will need to recompile with stripping disabled.

EOF
  else
    STRIP="touch"
  fi
])


dnl
dnl Checks for operating system and module support.
dnl


dnl EGG_OS_VERSION()
dnl
AC_DEFUN([EGG_OS_VERSION],
[
  dnl FIXME: Eventually replace these with the results of AC_CANONICAL_* below
  AC_CACHE_CHECK([system type], egg_cv_var_system_type, [egg_cv_var_system_type=`$UNAME -s`])
  AC_CACHE_CHECK([system release], egg_cv_var_system_release, [egg_cv_var_system_release=`$UNAME -r`])

  AC_CANONICAL_BUILD
  AC_CANONICAL_HOST
  AC_CANONICAL_TARGET
])


dnl EGG_CYGWIN_BINMODE
dnl
dnl Check for binmode.o on Cygwin.
dnl
AC_DEFUN([EGG_CYGWIN_BINMODE],
[
  if test "$EGG_CYGWIN" = yes; then
    AC_MSG_CHECKING([for /usr/lib/binmode.o])
    if test -r /usr/lib/binmode.o; then
      AC_MSG_RESULT([yes])
      EGG_APPEND_VAR(LIBS, /usr/lib/binmode.o)
    else
      AC_MSG_RESULT([no])
      AC_MSG_WARN([Make sure the directory Eggdrop is installed into is mounted in binary mode.])
    fi
  fi
])


dnl EGG_DARWIN_BUNDLE
dnl
dnl Locate bundle1.o on Darwin. Test systems seem to have it in /usr/lib,
dnl however the official docs say /lib.
dnl
AC_DEFUN([EGG_DARWIN_BUNDLE],
[
  BUNDLE=""
  for bundlepath in "/lib" "/usr/lib" "/usr/local/lib"; do
    AC_MSG_CHECKING([for bundle1.o in ${bundlepath}])
    if test -r "${bundlepath}/bundle1.o"; then
      AC_MSG_RESULT([yes])
      BUNDLE="${bundlepath}/bundle1.o"
      break
    else
      AC_MSG_RESULT([no])
    fi
  done

  if test "x$BUNDLE" = x; then
    cat << 'EOF' >&2
configure: WARNING:

  bundle1.o cannot be located. A module build might not compile correctly.

EOF
  fi
])


dnl EGG_CHECK_MODULE_SUPPORT()
dnl
dnl Checks for module support. This should be run after EGG_OS_VERSION.
dnl
AC_DEFUN([EGG_CHECK_MODULE_SUPPORT],
[
  MODULES_OK="yes"
  MOD_EXT="so"
  DEFAULT_MAKE="debug"
  LOAD_METHOD="dl"
  WEIRD_OS="yes"
  UNKNOWN_OS="no"
  MODULE_XLIBS=""

  AC_MSG_CHECKING([module loading capabilities])
  AC_MSG_RESULT
  AC_CHECK_HEADERS([dl.h dlfcn.h loader.h rld.h mach-o/dyld.h mach-o/rld.h])
  AC_CHECK_FUNCS([dlopen load NSLinkModule shl_load rld_load])

  # Note to other maintainers:
  # Bourne shell has no concept of "fall through"
  case "$egg_cv_var_system_type" in
    CYGWI*)
      WEIRD_OS="no"
      MOD_EXT="dll"
    ;;
    HP-UX)
      LOAD_METHOD="shl"
    ;;
    dell)
      # do nothing
    ;;
    IRIX)
      # do nothing
    ;;
    Ultrix)
      # No dlopen() or similar on Ultrix. We can't use modules.
      MODULES_OK="no"
    ;;
    BeOS)
      # We don't yet support BeOS's dynamic linking interface.
      MODULES_OK="no"
    ;;
    Linux)
      WEIRD_OS="no"
    ;;
    Lynx)
      # do nothing
    ;;
    QNX)
      # do nothing
      # QNX (recent versions at least) support dlopen().
    ;;
    OSF1)
      case `echo "$egg_cv_var_system_release" | cut -d . -f 1` in
        1.*) LOAD_METHOD="loader" ;;
      esac
    ;;
    SunOS)
      if test `echo "$egg_cv_var_system_release" | cut -d . -f 1` = 5; then
        # We've had quite a bit of testing on Solaris.
        WEIRD_OS="no"
      else
        # SunOS 4
        AC_DEFINE(DLOPEN_1, 1, [Define if running on SunOS 4.0.])
      fi
    ;;
    FreeBSD|OpenBSD|NetBSD|DragonFly)
      WEIRD_OS="no"
    ;;
    Darwin)
      # We should support Mac OS X (at least 10.1 and later) now.
      # Use rld on < 10.1.
      if test "$ac_cv_func_NSLinkModule" = no; then
        LOAD_METHOD="rld"
      fi
      LOAD_METHOD="dyld"
      EGG_DARWIN_BUNDLE
      EGG_APPEND_VAR(MODULE_XLIBS, $BUNDLE)
    ;;
    Haiku)
      WEIRD_OS="no"
    ;;
    Minix)
      WEIRD_OS="no"
    ;;
    *)
      if test -r /mach; then
        # At this point, we're guessing this is NeXT Step. We support rld, so
        # modules will probably work on NeXT now, but we have absolutely no way
        # to test this. I've never even seen a NeXT box, let alone do I know of
        # one I can test this on.
        LOAD_METHOD="rld"
      else
        # QNX apparently supports dlopen()... Fallthrough.
        if test -r /cmds; then
          UNKNOWN_OS="yes"
          MODULES_OK="no"
        fi
      fi
    ;;
  esac

  if test "$MODULES_OK" = yes; then
    AC_DEFINE(MODULES_OK, 1, [Define if modules will work on your system.])
    case $LOAD_METHOD in
      dl)
        AC_DEFINE(MOD_USE_DL, 1, [Define if modules should be loaded using the dl*() functions.])
      ;;
      shl)
        AC_DEFINE(MOD_USE_SHL, 1, [Define if modules should be loaded using the shl_*() functions.])
      ;;
      dyld)
        AC_DEFINE(MOD_USE_DYLD, 1, [Define if modules should be loaded using the NS*() functions.])
      ;;
      loader)
        AC_DEFINE(MOD_USE_LOADER, 1, [Define if modules should be loaded using the ldr*() and *load() functions.])
      ;;
      rld)
        AC_DEFINE(MOD_USE_RLD, 1, [Define if modules should be loaded using the rld_*() functions.])
      ;;
    esac
  else
    DEFAULT_MAKE="static"
  fi

  if test "$WEIRD_OS" = yes; then
    # Default to "make static" for 'weird' operating systems. Will print a
    # note at the end of configure explaining. This way, Eggdrop should compile
    # "out of the box" on most every operating system we know of, and they can
    # do a "make eggdrop" if they want to use(/try to use) module support. - Wcc
    DEFAULT_MAKE="static"
  fi

  AC_SUBST(DEFAULT_MAKE)
  AC_SUBST(MOD_EXT)
  AC_SUBST(MODULE_XLIBS)
  AC_DEFINE_UNQUOTED(EGG_MOD_EXT, "$MOD_EXT", [Defines the extension of Eggdrop modules.])
])


dnl EGG_CHECK_OS()
dnl
dnl Various operating system tests.
dnl
AC_DEFUN([EGG_CHECK_OS],
[
  MOD_CC="$CC"
  MOD_LD="$CC"
  MOD_STRIP="$STRIP"
  SHLIB_CC="$CC"
  SHLIB_LD="$CC"
  SHLIB_STRIP="$STRIP"
  LINUX="no"
  IRIX="no"
  SUNOS="no"
  HPUX="no"
  EGG_CYGWIN="no"

  case "$egg_cv_var_system_type" in
    CYGWI*)
      SHLIB_LD="$CC -shared"
      MOD_CC="$CC"
      MOD_LD="$CC"
      EGG_CYGWIN="yes"
      EGG_CYGWIN_BINMODE
      AC_DEFINE(CYGWIN_HACKS, 1, [Define if running under Cygwin.])
    ;;
    HP-UX)
      HPUX="yes"
      if test "$CC" = cc; then
        # HP-UX ANSI C Compiler.
        MOD_LD="$CC +z"
        SHLIB_CC="$CC +z"
      else
        # GCC
        MOD_LD="$CC -fPIC -shared"
        SHLIB_CC="$CC -fPIC"
      fi
      SHLIB_LD="ld -b"
    ;;
    dell)
      SHLIB_STRIP="touch"
      MOD_LD="$CC -lelf -lucb"
    ;;
    IRIX)
      SHLIB_LD="ld -n32 -shared -rdata_shared"
      IRIX="yes"
      SHLIB_STRIP="touch"
    ;;
    Ultrix)
      SHLIB_STRIP="touch"
      DEFAULT_MAKE="static"
      SHELL="/bin/sh5"
    ;;
    SINIX*)
      SHLIB_STRIP="touch"
      SHLIB_CC="cc -G"
    ;;
    BeOS)
      # do nothing
    ;;
    Linux)
      LINUX="yes"
      MOD_LD="$CC"
      SHLIB_CC="$CC -fPIC"
      SHLIB_LD="$CC -shared -nostartfiles"
    ;;
    Lynx)
      # do nothing
    ;;
    QNX)
      SHLIB_LD="ld -shared"
      AC_DEFINE(QNX_HACKS, 1, [Define if running under QNX.])
    ;;
    OSF1)
      case `echo "$egg_cv_var_system_release" | cut -d . -f 1` in
        V*)
          # Digital OSF uses an ancient version of gawk
          if test "$AWK" = gawk; then
            AWK="awk"
          fi
          SHLIB_LD="ld -shared -expect_unresolved \"'*'\""
          SHLIB_STRIP="touch"
        ;;
        1.0|1.1|1.2)
          SHLIB_LD="ld -R -export $@:"
        ;;
        1.*)
          SHLIB_CC="$CC -fpic"
          SHLIB_LD="ld -shared"
        ;;
      esac
      AC_DEFINE(BROKEN_SNPRINTF, 1, [Define to use Eggdrop's snprintf functions regardless of HAVE_SNPRINTF.])
      AC_DEFINE(STOP_UAC, 1, [Define if running on OSF/1 platform.])
    ;;
    SunOS)
      if test `echo "$egg_cv_var_system_release" | cut -d . -f 1` = 5; then
        # Solaris
        if test -n "$GCC"; then
          SHLIB_CC="$CC -fPIC"
          SHLIB_LD="$CC -shared"
        else
          SHLIB_CC="$CC -KPIC"
          SHLIB_LD="$CC -G -z text"
        fi
      else
        # SunOS 4
        SUNOS="yes"
        SHLIB_LD="ld"
        SHLIB_CC="$CC -PIC"
      fi
    ;;
    FreeBSD|OpenBSD|NetBSD)
      SHLIB_CC="$CC -fPIC"
      SHLIB_LD="$CC -shared"
      case "$egg_cv_var_system_type" in
        *NetBSD)
          AC_DEFINE(NETBSD_HACKS, 1, [Define if running under NetBSD.])
        ;;
      esac
    ;;
    DragonFly)
      SHLIB_CC="$CC -fPIC"
    ;;
    Darwin)
      # Mac OS X
      SHLIB_CC="$CC -fPIC"
      SHLIB_LD="ld -bundle -undefined error"
      AC_DEFINE(BIND_8_COMPAT, 1, [Define if running on Mac OS X with dns.mod.])
    ;;
    *)
      if test -r /mach; then
        # At this point, we're guessing this is NeXT Step.
        AC_DEFINE(BORGCUBES, 1, [Define if running on NeXT Step.])
      else
        if test -r /cmds; then
          # Probably QNX.
          SHLIB_LD="ld -shared"
          SHLIB_STRIP="touch"
        fi
      fi
    ;;
  esac

  AC_SUBST(MOD_LD)
  AC_SUBST(MOD_CC)
  AC_SUBST(MOD_STRIP)
  AC_SUBST(SHLIB_LD)
  AC_SUBST(SHLIB_CC)
  AC_SUBST(SHLIB_STRIP)
])


dnl
dnl Library tests.
dnl


dnl EGG_CHECK_LIBS()
dnl
AC_DEFUN([EGG_CHECK_LIBS],
[
  # FIXME: this needs to be fixed so that it works on IRIX
  if test "$IRIX" = yes; then
    AC_MSG_WARN([Skipping library tests because they CONFUSE IRIX.])
  else
    AX_LIB_SOCKET_NSL
    AC_SEARCH_LIBS([dlopen], [dl])
    AC_CHECK_LIB(m, tan, EGG_MATH_LIB="-lm")

    # This is needed for Tcl libraries compiled with thread support
    AC_CHECK_LIB(pthread, pthread_mutex_init, [
      ac_cv_lib_pthread_pthread_mutex_init="yes"
      ac_cv_lib_pthread="-lpthread"
    ], [
      AC_CHECK_LIB(pthread, __pthread_mutex_init, [
        ac_cv_lib_pthread_pthread_mutex_init="yes"
        ac_cv_lib_pthread="-lpthread"
      ], [
        AC_CHECK_LIB(pthreads, pthread_mutex_init, [
          ac_cv_lib_pthread_pthread_mutex_init="yes"
          ac_cv_lib_pthread="-lpthreads"
        ], [
          AC_CHECK_FUNC(pthread_mutex_init, [
            ac_cv_lib_pthread_pthread_mutex_init="yes"
            ac_cv_lib_pthread=""
          ], [
            ac_cv_lib_pthread_pthread_mutex_init="no"
          ]
        )]
      )]
    )])

    if test "$SUNOS" = yes; then
      # For suns without yp
      AC_CHECK_LIB(dl, main)
    else
      if test "$HPUX" = yes; then
        AC_CHECK_LIB(dld, shl_load)
      fi
    fi
  fi
])


dnl EGG_ARG_HANDLEN()
dnl 
AC_DEFUN([EGG_ARG_HANDLEN], [
  AC_ARG_WITH(handlen, [  --with-handlen=VALUE    set the maximum length a handle on the bot can be], [
    if test -n $withval && test $withval -ge 9 && test $withval -le 32;
    then
      AC_DEFINE_UNQUOTED(EGG_HANDLEN, $withval, [
        Define the maximum length of handles on the bot.
      ])
    else
      AC_MSG_WARN([Invalid handlen given (must be a number between 9 and 32), defaulting to 9.])
    fi
  ])
])

dnl
dnl Misc checks.
dnl


dnl EGG_EXEEXT()
dnl
dnl Test for executable suffix and define Eggdrop's executable name accordingly.
dnl
AC_DEFUN([EGG_EXEEXT], [
  EGGEXEC="eggdrop"
  AC_EXEEXT
  if test "x$EXEEXT" != x; then
    EGGEXEC="eggdrop${EXEEXT}"
  fi
  AC_SUBST(EGGEXEC)
])


dnl
dnl Tcl checks.
dnl


dnl EGG_TCL_ARG_WITH()
dnl
AC_DEFUN([EGG_TCL_ARG_WITH],
[
  AC_ARG_WITH(tcllib, [  --with-tcllib=PATH      full path to Tcl library (e.g. /usr/lib/libtcl8.5.so)], [tcllibname="$withval"])
  AC_ARG_WITH(tclinc, [  --with-tclinc=PATH      full path to Tcl header (e.g. /usr/include/tcl.h)],  [tclincname="$withval"])

  WARN=0
  # Make sure either both or neither $tcllibname and $tclincname are set
  if test "x$tcllibname" != x; then
    if test "x$tclincname" = x; then
      WARN=1
      tcllibname=""
      TCLLIB=""
      TCLINC=""
    fi
  else
    if test "x$tclincname" != x; then
      WARN=1
      tclincname=""
      TCLLIB=""
      TCLINC=""
    fi
  fi

  if test "$WARN" = 1; then
    cat << 'EOF' >&2
configure: WARNING:

  You must specify both --with-tcllib and --with-tclinc for either to work.

  configure will now attempt to autodetect both the Tcl library and header.

EOF
  fi
])


dnl EGG_TCL_WITH_TCLLIB()
dnl
AC_DEFUN([EGG_TCL_WITH_TCLLIB],
[
  # Look for Tcl library: if $tcllibname is set, check there first
  if test "x$tcllibname" != x; then
    if test -f "$tcllibname" && test -r "$tcllibname"; then
      TCLLIB=`echo $tcllibname | sed 's%/[[^/]][[^/]]*$%%'`
      TCLLIBFN=`$BASENAME $tcllibname | cut -c4-`
      TCLLIBEXT=`echo $TCLLIBFN | $AWK '{j=split([$]1, i, "."); suffix=""; while (i[[j]] ~ /^[[0-9]]+$/) { suffix = "." i[[j--]] suffix; }; print "." i[[j]] suffix }'`
      TCLLIBFNS=`$BASENAME $tcllibname $TCLLIBEXT | cut -c4-`

      # Set default make as static for unshared Tcl library
      if test TCLLIBEXT = ".a"; then
        if test "$DEFAULT_MAKE" != static; then
          cat << 'EOF' >&2
configure: WARNING:

  Your Tcl library is not a shared lib.
  configure will now set default make type to static.

EOF
          DEFAULT_MAKE="static"
          AC_SUBST(DEFAULT_MAKE)
        fi
      fi

    else
      cat << EOF >&2
configure: WARNING:

  The file '$tcllibname' given to option --with-tcllib is not valid.
  Specify the full path including the file name (e.g. /usr/lib/libtcl8.5.so)

  configure will now attempt to autodetect both the Tcl library and header.

EOF
      tcllibname=""
      tclincname=""
      TCLLIB=""
      TCLLIBFN=""
      TCLINC=""
      TCLINCFN=""
    fi
  fi
])


dnl EGG_TCL_WITH_TCLINC()
dnl
AC_DEFUN([EGG_TCL_WITH_TCLINC],
[
  # Look for Tcl header: if $tclincname is set, check there first
  if test "x$tclincname" != x; then
    if test -f "$tclincname" && test -r "$tclincname"; then
      TCLINC=`echo $tclincname | sed 's%/[[^/]][[^/]]*$%%'`
      TCLINCFN=`$BASENAME $tclincname`
    else
      cat << EOF >&2
configure: WARNING:

  The file '$tclincname' given to option --with-tclinc is not valid.
  Specify the full path including the file name (e.g. /usr/include/tcl.h)

  configure will now attempt to autodetect both the Tcl library and header.

EOF
      tcllibname=""
      tclincname=""
      TCLLIB=""
      TCLLIBFN=""
      TCLINC=""
      TCLINCFN=""
    fi
  fi
])


dnl EGG_TCL_TCLCONFIG()
dnl
AC_DEFUN([EGG_TCL_TCLCONFIG],
[
  if test "x$TCLLIBFN" = x; then
    AC_MSG_NOTICE([Autoconfiguring Tcl with tclConfig.sh])
    egg_tcl_changed="yes"
    TEA_INIT("3.10")
    TEA_PATH_TCLCONFIG
    TEA_LOAD_TCLCONFIG
    TEA_TCL_LINK_LIBS
    # Overwrite TCL_LIBS again, which TCL_LOAD_TCLCONFIG unfortunately overwrites from tclConfig.sh
    # Also, use the Tcl linker idea to be compatible with their ldflags
    if test -r ${TCL_BIN_DIR}/tclConfig.sh; then
      . ${TCL_BIN_DIR}/tclConfig.sh
      # OpenBSD uses -pthread, but tclConfig.sh provides that flag in EXTRA_CFLAGS
      if test "$(echo $TCL_EXTRA_CFLAGS | grep -- -pthread)"; then
        TCL_PTHREAD_LDFLAG="-pthread"
      else
        TCL_PTHREAD_LDFLAG=""
      fi
      AC_SUBST(SHLIB_LD, $TCL_SHLIB_LD)
      AC_MSG_CHECKING([for Tcl linker])
      AC_MSG_RESULT([$SHLIB_LD])
    else
      TCL_LIBS="${EGG_MATH_LIB}"
    fi
    TCL_PATCHLEVEL="${TCL_MAJOR_VERSION}.${TCL_MINOR_VERSION}${TCL_PATCH_LEVEL}"
    TCL_LIB_SPEC="${TCL_PTHREAD_LDFLAG} ${TCL_LIB_SPEC} ${TCL_LIBS}"
  else
    egg_tcl_changed="yes"
    if test -r ${TCLLIB}/tclConfig.sh; then
      . ${TCLLIB}/tclConfig.sh
      # OpenBSD uses -pthread, but tclConfig.sh provides that flag in EXTRA_CFLAGS
      if test "$(echo $TCL_EXTRA_CFLAGS | grep -- -pthread)"; then
        TCL_PTHREAD_LDFLAG="-pthread"
      else
        TCL_PTHREAD_LDFLAG=""
      fi
      AC_SUBST(SHLIB_LD, $TCL_SHLIB_LD)
      AC_MSG_CHECKING([for Tcl linker])
      AC_MSG_RESULT([$SHLIB_LD])
      TCL_PATCHLEVEL="${TCL_MAJOR_VERSION}.${TCL_MINOR_VERSION}${TCL_PATCH_LEVEL}"
      TCL_LIB_SPEC="${TCL_PTHREAD_LDFLAG} ${TCL_LIB_SPEC} ${TCL_LIBS}"
    else
      TCL_LIB_SPEC="-L$TCLLIB -l$TCLLIBFNS ${EGG_MATH_LIB}"
      if test "x$ac_cv_lib_pthread" != x; then
        TCL_LIB_SPEC="$TCL_LIB_SPEC $ac_cv_lib_pthread"
      fi
      TCL_INCLUDE_SPEC=""
      TCL_VERSION=`grep TCL_VERSION $TCLINC/$TCLINCFN | $HEAD_1 | $AWK '{gsub(/\"/, "", [$]3); print [$]3}'`
      TCL_PATCHLEVEL=`grep TCL_PATCH_LEVEL $TCLINC/$TCLINCFN | $HEAD_1 | $AWK '{gsub(/\"/, "", [$]3); print [$]3}'`
      TCL_MAJOR_VERSION=`echo $TCL_VERSION | cut -d. -f1`
      TCL_MINOR_VERSION=`echo $TCL_VERSION | cut -d. -f2`
      if test $TCL_MAJOR_VERSION -gt 8 || test $TCL_MAJOR_VERSION -eq 8 -a $TCL_MINOR_VERSION -ge 6; then
        TCL_LIB_SPEC="$TCL_LIB_SPEC -lz"
      fi
    fi
  fi

  if test -z "$ac_cv_lib_dlopen"; then
    TCL_LIB_SPEC=$(echo $TCL_LIB_SPEC | sed -- 's/-ldl//g')
  fi

  AC_MSG_CHECKING([for Tcl version])
  AC_MSG_RESULT([$TCL_PATCHLEVEL])
  AC_MSG_CHECKING([for Tcl library flags])
  AC_MSG_RESULT([$TCL_LIB_SPEC])
  AC_MSG_CHECKING([for Tcl header flags])
  AC_MSG_RESULT([$TCL_INCLUDE_SPEC])

  AC_SUBST(TCL_LIB_SPEC)
  AC_SUBST(TCL_INCLUDE_SPEC)
])

dnl EGG_TCL_CHECK_VERSION()
dnl
AC_DEFUN([EGG_TCL_CHECK_VERSION],
[

  if test "x$TCL_MAJOR_VERSION" = x || test "x$TCL_MINOR_VERSION" = x || test $TCL_MAJOR_VERSION -lt 8 || test $TCL_MAJOR_VERSION -eq 8 -a $TCL_MINOR_VERSION -lt 3; then
    cat << EOF >&2
configure: error:

  Your Tcl version is much too old for Eggdrop to use. You should
  download and compile a more recent version. The most reliable
  current version is $tclrecommendver and can be downloaded from
  ${tclrecommendsite}. We require at least Tcl 8.3.

  See doc/COMPILE-GUIDE's 'Tcl Detection and Installation' section
  for more information.

EOF
    exit 1
  fi

])


dnl EGG_CACHE_UNSET(CACHE-ID)
dnl
dnl Unsets a certain cache item. Typically called before using the AC_CACHE_*()
dnl macros.
dnl
AC_DEFUN([EGG_CACHE_UNSET], [unset $1])


dnl EGG_TCL_CHECK_NOTIFIER_INIT
dnl
AC_DEFUN([EGG_TCL_CHECK_NOTIFIER_INIT],
[
  if test $TCL_MAJOR_VERSION -gt 8 || test $TCL_MAJOR_VERSION -eq 8 -a $TCL_MINOR_VERSION -ge 4; then
    AC_DEFINE(HAVE_TCL_NOTIFIER_INIT, 1, [Define for Tcl that has the Tcl_NotifierProcs struct member initNotifierProc (8.4 and later).])
  fi
])


dnl EGG_SUBST_EGGVERSION()
dnl
AC_DEFUN([EGG_SUBST_EGGVERSION],
[

  EGGVERSION=`grep '^ *# *define  *EGG_STRINGVER ' $srcdir/src/version.h | $AWK '{gsub(/(\")/, "", $NF); print $NF}'`
  egg_version_num=`echo $EGGVERSION | $AWK 'BEGIN {FS = "."} {printf("%d%02d%02d", [$]1, [$]2, [$]3)}'`
  AC_SUBST(EGGVERSION)
  AC_DEFINE_UNQUOTED(EGG_VERSION, $egg_version_num, [Defines the current Eggdrop version.])
])


dnl EGG_SUBST_DEST()
AC_DEFUN([EGG_SUBST_DEST],
[
  if test "x$DEST" = x; then
    DEST=\${prefix}
  fi
  AC_SUBST(DEST)
])


dnl EGG_SUBST_MOD_UPDIR()
dnl
dnl Since module's Makefiles aren't generated by configure, some paths in
dnl src/mod/Makefile.in take care of them. For correct path "calculation", we
dnl need to keep absolute paths in mind (which don't need a "../" prepended).
dnl
AC_DEFUN([EGG_SUBST_MOD_UPDIR], [
  case "$srcdir" in
    [[\\/]]* | ?:[[\\/]]*)
      MOD_UPDIR=""
    ;;
    *)
      MOD_UPDIR="../"
    ;;
  esac
  AC_SUBST(MOD_UPDIR)
])


dnl EGG_REPLACE_IF_CHANGED(FILE-NAME, CONTENTS-CMDS, INIT-CMDS)
dnl
dnl Replace FILE-NAME if the newly created contents differs from the existing
dnl file contents. Otherwise, leave the file alone. This avoids needless
dnl recompiles.
dnl
m4_define(EGG_REPLACE_IF_CHANGED,
[
  AC_CONFIG_COMMANDS([replace-if-changed], [[
    egg_replace_file="$1"
    $2
    if test -f "$egg_replace_file" && cmp -s conftest.out $egg_replace_file; then
      echo "$1 is unchanged"
    else
      echo "creating $1"
      mv conftest.out $egg_replace_file
    fi
    rm -f conftest.out
  ]],
  [[$3]])
])


dnl EGG_TCL_LUSH()
dnl
AC_DEFUN([EGG_TCL_LUSH],
[
  if test "x$TCLINCFN" != x; then
    egg_tclinc="\\\"$TCLINC/$TCLINCFN\\\""
  else
    egg_tclinc="<tcl.h>"
  fi
  EGG_REPLACE_IF_CHANGED(lush.h, [
cat > conftest.out << EOF

/* Ignore me but do not erase me. I am a kludge. */

#include ${egg_tclinc}

EOF
  ],[
    egg_tclinc="${egg_tclinc}"
  ])

])


dnl EGG_DEBUG_ENABLE()
dnl
AC_DEFUN([EGG_DEBUG_ENABLE],
[
  AC_ARG_ENABLE(debug,         [  --enable-debug          enable generic debug code (default for 'make debug')], [enable_debug="$enableval"], [enable_debug="auto"])
  AC_ARG_ENABLE(debug,         [  --disable-debug         disable generic debug code], [enable_debug="$enableval"], [enable_debug="auto"])
  AC_ARG_ENABLE(debug-assert,  [  --enable-debug-assert   enable assert debug code (default for 'make debug')], [enable_debug_assert="$enableval"], [enable_debug_assert="auto"])
  AC_ARG_ENABLE(debug-assert,  [  --disable-debug-assert  disable assert debug code], [enable_debug_assert="$enableval"], [enable_debug_assert="auto"])
  AC_ARG_ENABLE(debug-mem,     [  --enable-debug-mem      enable memory debug code (default for 'make debug')], [enable_debug_mem="$enableval"], [enable_debug_mem="auto"])
  AC_ARG_ENABLE(debug-mem,     [  --disable-debug-mem     disable memory debug code], [enable_debug_mem="$enableval"], [enable_debug_mem="auto"])
  AC_ARG_ENABLE(debug-dns,     [  --enable-debug-dns      enable dns.mod debug messages (default for 'make debug')], [enable_debug_dns="$enableval"], [enable_debug_dns="auto"])
  AC_ARG_ENABLE(debug-dns,     [  --disable-debug-dns     disable dns.mod debug messages], [enable_debug_dns="$enableval"], [enable_debug_dns="auto"])
  AC_ARG_ENABLE(debug-context, [  --enable-debug-context  enable context debug code (default)], [enable_debug_context="$enableval"], [enable_debug_context="auto"])
  AC_ARG_ENABLE(debug-context, [  --disable-debug-context disable context debug code], [enable_debug_context="$enableval"], [enable_debug_context="auto"])
])


dnl EGG_DEBUG_DEFAULTS()
dnl
AC_DEFUN([EGG_DEBUG_DEFAULTS],
[
  # Defaults:

  # make: 'eggdrop' or 'static'
  default_std_debug="no"
  default_std_debug_assert="no"
  default_std_debug_mem="no"
  default_std_debug_context="yes"
  default_std_debug_dns="no"

  # make: 'debug' or 'sdebug'
  default_deb_debug="yes"
  default_deb_debug_assert="yes"
  default_deb_debug_mem="yes"
  default_deb_debug_context="yes"
  default_deb_debug_dns="yes"

  if test "$DEFAULT_MAKE" = eggdrop || test "$DEFAULT_MAKE" = static; then
    default_debug="$default_std_debug" 
    default_debug_assert="$default_std_debug_assert" 
    default_debug_mem="$default_std_debug_mem" 
    default_debug_context="$default_std_debug_context"
    default_debug_dns="$default_std_debug_dns"
  else
    default_debug="$default_deb_debug"
    default_debug_assert="$default_deb_debug_assert"
    default_debug_mem="$default_deb_debug_mem"
    default_debug_context="$default_deb_debug_context"
    default_debug_dns="$default_deb_debug_dns"
  fi

  debug_options="debug debug_assert debug_mem debug_dns"

  debug_cflags_debug="-g3 -DDEBUG"
  debug_cflags_debug_assert="-DDEBUG_ASSERT"
  debug_cflags_debug_mem="-DDEBUG_MEM"
  debug_cflags_debug_dns="-DDEBUG_DNS"
  debug_stdcflags_debug=""
  debug_stdcflags_debug_assert=""
  debug_stdcflags_debug_mem=""
  debug_stdcflags_debug_dns=""
  debug_debcflags_debug=""
  debug_debcflags_debug_assert=""
  debug_debcflags_debug_mem=""
  debug_debcflags_debug_dns=""
])


dnl EGG_DEBUG_OPTIONS()
dnl
AC_DEFUN([EGG_DEBUG_OPTIONS],
[
  for enable_option in $debug_options; do

    eval enable_value=\$enable_$enable_option

    # Check if either --enable-<opt> or --disable-<opt> was used
    if test "$enable_value" != auto; then
      # Make sure an invalid option wasn't passed as --enable-<opt>=foo
      if test "$enable_value" != yes && test "$enable_value" != no; then
        opt_name=`echo $enable_option | sed 's/_/-/g'`
        eval opt_default=\$default_$enable_option
        AC_MSG_WARN([Invalid option '$enable_value' passed to --enable-${opt_name}, defaulting to '$opt_default'])
        eval enable_$enable_option="auto"
      fi
    fi

    if test "$enable_value" = auto; then
      # Note: options generally should not end up in both std and deb but
      # there may be options in the future where this behavior is desired.
      if test `eval echo '${'default_std_$enable_option'}'` = yes; then
        eval `echo debug_stdcflags_$enable_option`=\$debug_cflags_$enable_option
      fi
      if test `eval echo '${'default_deb_$enable_option'}'` = yes; then
        eval `echo debug_debcflags_$enable_option`=\$debug_cflags_$enable_option
      fi
    else
      if test "$enable_value" = yes; then
        # If option defaults to 'yes' for debug, always put it in stdcflags
        # when the option is forced on because someone may want it enabled
        # for a non-debug build.
        if test `eval echo '${'default_deb_$enable_option'}'` = yes; then
          eval `echo debug_stdcflags_$enable_option`=\$debug_cflags_$enable_option
        else
          # option defaulted to 'no' so put it in debcflags
          eval `echo debug_debcflags_$enable_option`=\$debug_cflags_$enable_option
        fi
      fi
    fi
  done
])


dnl EGG_DEBUG_CFLAGS()
dnl
AC_DEFUN([EGG_DEBUG_CFLAGS],
[
  for cflg_option in $debug_options; do
    eval stdcflg_value=\$debug_stdcflags_$cflg_option
    EGG_APPEND_VAR(CFLGS, $stdcflg_value)

    eval debcflg_value=\$debug_debcflags_$cflg_option
    EGG_APPEND_VAR(DEBCFLGS, $debcflg_value)
  done

  # Disable debug symbol stripping if compiled with --enable-debug
  # This will result in core dumps that are actually useful.
  if test "x$debug_stdcflags_debug" != x; then
    STRIP="touch"
    MOD_STRIP="touch"
    SHLIB_STRIP="touch"
  fi

  AC_SUBST(CFLGS)
  AC_SUBST(DEBCFLGS)
])


dnl EGG_ENABLE_DEBUG_CONTEXT()
dnl
AC_DEFUN([EGG_ENABLE_DEBUG_CONTEXT],
[
  # Check if either --enable-debug-context or --disable-debug-context was used
  if test "$enable_debug_context" != auto; then

    # Make sure an invalid option wasn't passed as --enable-debug-context=foo
    if test "$enable_debug_context" != yes && test "$enable_debug_context" != no; then
      AC_MSG_WARN([Invalid option '$enable_debug_context' passed to --enable-debug-context, defaulting to '$default_debug_context'])
      enable_debug_context="$default_debug_context"
    fi
  else
    enable_debug_context="$default_debug_context"
  fi

  if test "$enable_debug_context" = yes; then
    AC_DEFINE(DEBUG_CONTEXT, 1, [Define for context debugging.])
  else
    cat << 'EOF' >&2
configure: WARNING:

  You have disabled context debugging.

  Eggdrop will not be able to provide context information if it crashes.
  Bug reports without context are less helpful when tracking down bugs.

EOF
  fi
])


dnl EGG_CATCH_MAKEFILE_REBUILD()
dnl
AC_DEFUN([EGG_CATCH_MAKEFILE_REBUILD],
[
  AC_CONFIG_COMMANDS([catch-make-rebuild], [[
    if test -f .modules; then
      $srcdir/misc/modconfig --top_srcdir="$srcdir/src" Makefile
    fi
  ]])
])


dnl EGG_SAVE_PARAMETERS()
dnl
dnl Remove --cache-file and --srcdir arguments so they do not pile up.
dnl
AC_DEFUN([EGG_SAVE_PARAMETERS],
[
  egg_ac_parameters=
  ac_prev=
  for ac_arg in $ac_configure_args; do
    if test -n "$ac_prev"; then
      ac_prev=
      continue
    fi
    case $ac_arg in
    -cache-file | --cache-file | --cache-fil | --cache-fi | --cache-f | --cache- | --cache | --cach | --cac | --ca | --c)
      ac_prev=cache_file ;;
    -cache-file=* | --cache-file=* | --cache-fil=* | --cache-fi=* | --cache-f=* | --cache-=* | --cache=* | --cach=* | --cac=* | --ca=* | --c=*)
      ;;
    --config-cache | -C)
      ;;
    -srcdir | --srcdir | --srcdi | --srcd | --src | --sr)
      ac_prev=srcdir ;;
    -srcdir=* | --srcdir=* | --srcdi=* | --srcd=* | --src=* | --sr=*)
      ;;
    *) egg_ac_parameters="$egg_ac_parameters $ac_arg" ;;
    esac
  done

  AC_SUBST(egg_ac_parameters)
  AC_DEFINE_UNQUOTED(EGG_AC_ARGS_RAW, $egg_ac_parameters, [Arguments passed to configure])
])


dnl EGG_IPV6_COMPAT
dnl
AC_DEFUN([EGG_IPV6_COMPAT],
[
if test "$enable_ipv6" = "yes"; then
  AC_CHECK_FUNCS([inet_pton gethostbyname2])
  AC_CHECK_TYPES([struct in6_addr], egg_cv_var_have_in6_addr="yes", egg_cv_var_have_in6_addr="no", [
    #include <sys/types.h>
    #include <netinet/in.h>
  ])
  if test "$egg_cv_var_have_in6_addr" = "yes"; then
    # Check for in6addr_any
    AC_CACHE_CHECK([for the in6addr_any constant], [egg_cv_var_have_in6addr_any], [
      AC_TRY_COMPILE([
        #include <sys/types.h>
        #include <netinet/in.h>
      ], [struct in6_addr i6 = in6addr_any;],
      [egg_cv_var_have_in6addr_any="yes"], [egg_cv_var_have_in6addr_any="no"])
    ])
    if test "$egg_cv_var_have_in6addr_any" = "yes"; then
      AC_DEFINE(HAVE_IN6ADDR_ANY, 1, [Define to 1 if you have the in6addr_any constant.])
    fi
    # Check for in6addr_loopback
    AC_CACHE_CHECK([for the in6addr_loopback constant], [egg_cv_var_have_in6addr_loopback], [
      AC_TRY_COMPILE([
        #include <sys/types.h>
        #include <netinet/in.h>
      ], [struct in6_addr i6 = in6addr_loopback;],
      [egg_cv_var_have_in6addr_loopback="yes"], [egg_cv_var_have_in6addr_loopback="no"])
    ])
    if test "$egg_cv_var_have_in6addr_loopback" = "yes"; then
      AC_DEFINE(HAVE_IN6ADDR_LOOPBACK, 1, [Define to 1 if you have the in6addr_loopback constant.])
    fi
    AC_CHECK_TYPES([struct sockaddr_in6], , , [
      #include <sys/types.h>
      #include <netinet/in.h>
    ])
  else
    AC_MSG_NOTICE([no in6_addr found, skipping dependent checks. Custom definitions will be used.])
  fi
fi
])


dnl EGG_IPV6_ENABLE
dnl
AC_DEFUN([EGG_IPV6_ENABLE],
[
  ipv6_enabled="no"
  AC_ARG_ENABLE(ipv6,
    [  --enable-ipv6           enable IPv6 support (autodetect)],
    [enable_ipv6="$enableval"], [enable_ipv6="$egg_cv_var_ipv6_supported"])
  AC_ARG_ENABLE(ipv6,
    [  --disable-ipv6          disable IPv6 support ], [enable_ipv6="$enableval"])

  if test "$enable_ipv6" = "yes"; then
    if test "$egg_cv_var_ipv6_supported" = "no"; then
      AC_MSG_WARN([You have enabled IPv6 but your system doesn't seem to support it.])
      AC_MSG_WARN([Eggdrop will compile but will be limited to IPv4 on this host.])
    fi
    AC_DEFINE(IPV6, 1, [Define to 1 if you want to enable IPv6 support.])
    ipv6_enabled="yes"
  fi
])


dnl EGG_IPV6_STATUS
dnl
AC_DEFUN([EGG_IPV6_STATUS],
[
  AC_CACHE_CHECK([for system IPv6 support], [egg_cv_var_ipv6_supported], [
    AC_RUN_IFELSE([AC_LANG_PROGRAM([[
      #include <unistd.h>
      #include <sys/socket.h>
      #include <netinet/in.h>
    ]], [[
        int s = socket(AF_INET6, SOCK_STREAM, 0);

        if (s != -1)
          close(s);

        return((s == -1));
    ]])], [
      egg_cv_var_ipv6_supported="yes"
     ], [
      egg_cv_var_ipv6_supported="no"
    ], [
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <unistd.h>
        #include <sys/socket.h>
        #include <netinet/in.h>
      ]], [[
          int s = socket(AF_INET6, SOCK_STREAM, 0);
  
          if (s != -1)
            close(s);
  
          return((s == -1));
      ]])], [
        egg_cv_var_ipv6_supported="yes"
       ], [
        egg_cv_var_ipv6_supported="no"
      ])
    ])
  ])
])


dnl EGG_TLS_ENABLE
dnl
AC_DEFUN([EGG_TLS_ENABLE],
[
  AC_MSG_CHECKING([whether to enable TLS support])
  AC_ARG_ENABLE(tls,
    [  --enable-tls            enable TLS support (autodetect)],
    [enable_tls="$enableval"])
  AC_ARG_ENABLE(tls,
    [  --disable-tls           disable TLS support ], [enable_tls="$enableval"],
    [enable_tls="autodetect"])

  AC_MSG_RESULT([$enable_tls])
])


dnl EGG_TLS_WITHSSL
dnl
AC_DEFUN(EGG_TLS_WITHSSL,
[
  save_LIBS="$LIBS"
  AC_ARG_WITH(sslinc, [  --with-sslinc=PATH      Path to OpenSSL headers], [
    if test "$enable_tls" != "no"; then
      if test -d "$withval"; then
        save_CC="$CC"
        save_CPP="$CPP"
        CC="$CC -I$withval"
        CPP="$CPP -I$withval"
        AC_CHECK_HEADERS([openssl/ssl.h openssl/x509v3.h], [sslinc="-I$withval"], [
          AC_MSG_WARN([Invalid path to OpenSSL headers. $withval/openssl/ doesn't contain the required files.])
          sslinc=""
          break
        ], [[
          #ifdef CYGWIN_HACKS
          #  ifndef __int64
          #    define __int64 long long
          #  endif
          #endif
        ]])
        AC_SUBST(SSL_INCLUDES, [$sslinc])
        CC="$save_CC"
        CPP="$save_CPP"
      else
        AC_MSG_WARN([Invalid path to OpenSSL headers. $withval is not a directory.])
      fi
    fi
  ])

  AC_ARG_WITH(ssllib, [  --with-ssllib=PATH      Path to OpenSSL libraries],
  [
    if test "$enable_tls" != "no"; then
      if test -d "$withval"; then
        AC_CHECK_LIB(crypto, X509_digest, , [havessllib="no"], [-L$withval -lssl])
        AC_CHECK_LIB(ssl, SSL_accept, , [havessllib="no"], [-L$withval -lcrypto])
        if test "$havessllib" = "no"; then
          AC_MSG_WARN([Invalid path to OpenSSL libs. $withval doesn't contain the required files.])
        else
          AC_SUBST(SSL_LIBS, [-L$withval])
          LDFLAGS="${LDFLAGS} -L$withval"
        fi
      else
        AC_MSG_WARN([You have specified an invalid path to OpenSSL libs. $withval is not a directory.])
      fi
    fi
  ])
])


dnl EGG_TLS_DETECT
dnl
AC_DEFUN([EGG_TLS_DETECT],
[
  tls_enabled="no"
  if test "$enable_tls" != "no"; then
    if test -z "$SSL_INCLUDES"; then
      AC_CHECK_HEADERS([openssl/ssl.h openssl/x509v3.h], , [havesslinc="no"], [
        #ifdef CYGWIN_HACKS
        #  ifndef __int64
        #    define __int64 long long
        #  endif
        #endif
      ])
    fi
    if test -z "$SSL_LIBS"; then
      AC_CHECK_LIB(crypto, X509_digest, , [havessllib="no"], [-lssl])
      AC_CHECK_LIB(ssl, SSL_accept, , [havessllib="no"], [-lcrypto])
      AC_CHECK_FUNCS([EVP_md5 EVP_sha1 a2i_IPADDRESS], , [[
        havessllib="no"
        break
      ]])
    fi
    AC_CHECK_FUNC(OPENSSL_buf2hexstr, ,
      AC_CHECK_FUNC(hex_to_string,
          AC_DEFINE([OPENSSL_buf2hexstr], [hex_to_string], [Define this to hex_to_string when using OpenSSL < 1.1.0])
        , [[
          havessllib="no"
          break
      ]])
    )
    AC_CHECK_FUNC(OPENSSL_hexstr2buf, ,
      AC_CHECK_FUNC(string_to_hex,
          AC_DEFINE([OPENSSL_hexstr2buf], [string_to_hex], [Define this to string_to_hex when using OpenSSL < 1.1.0])
        , [[
          havessllib="no"
          break
      ]])
    )
    if test "$enable_tls" = "yes"; then
      if test "$havesslinc" = "no"; then
        AC_MSG_WARN([Cannot find OpenSSL headers.])
        AC_MSG_WARN([Please specify the path to the openssl include dir using --with-sslinc=path])
      fi
      if test "$havessllib" = "no"; then
        AC_MSG_WARN([Cannot find OpenSSL libraries.])
        AC_MSG_WARN([Please specify the path to libssl and libcrypto using --with-ssllib=path])
      fi
    fi
    AC_MSG_CHECKING([for OpenSSL])
    if test "$havesslinc" = "no" || test "$havessllib" = "no"; then
      AC_MSG_RESULT([no (make sure you have version 0.9.8 or higher installed)])
      LIBS="$save_LIBS"
    else
      AC_MSG_RESULT([yes])
      if test "$EGG_CYGWIN" = "yes"; then
        AC_CHECK_TYPE([__int64], , [
          AC_DEFINE([__int64], [long long], [Define this to a 64-bit type on Cygwin to satisfy OpenSSL dependencies.])
        ])
      fi
      AC_CHECK_FUNCS([RAND_status])
      AC_DEFINE(TLS, 1, [Define this to enable SSL support.])
      AC_CHECK_FUNC(ASN1_STRING_get0_data,
        AC_DEFINE([egg_ASN1_string_data], [ASN1_STRING_get0_data], [Define this to ASN1_STRING_get0_data when using OpenSSL 1.1.0+, ASN1_STRING_data otherwise.])
        , AC_DEFINE([egg_ASN1_string_data], [ASN1_STRING_data], [Define this to ASN1_STRING_get0_data when using OpenSSL 1.1.0+, ASN1_STRING_data otherwise.])
      )
      tls_enabled="yes"
      EGG_MD5_COMPAT
    fi
  fi
])


dnl EGG_MD5_COMPAT
dnl
AC_DEFUN([EGG_MD5_COMPAT],
[
  save_CC="$CC"
  save_CPP="$CPP"
  CC="$CC $sslinc"
  CPP="$CPP $sslinc"
  AC_CHECK_HEADERS([openssl/md5.h], [
    AC_CHECK_FUNCS([MD5_Init MD5_Update MD5_Final], , [havesslmd5="no"])
  ])
  if test "$havesslmd5" != "no"; then
    AC_DEFINE(HAVE_OPENSSL_MD5, 1, [Define this if your OpenSSL library has MD5 cipher support.])
  fi
  CC="$save_CC"
  CPP="$save_CPP"
])
