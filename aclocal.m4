dnl aclocal.m4: macros autoconf uses when building configure from configure.ac
dnl
dnl Copyright (C) 1999 - 2008 Eggheads Development Team
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
dnl $Id: aclocal.m4,v 1.104 2008/06/18 06:03:20 tothwolf Exp $
dnl


dnl
dnl Message functions.
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


dnl EGG_MSG_WEIRDOS
dnl
dnl Print some messages at the end of configure to give extra information to
dnl users of 'weird' operating systems.
dnl
AC_DEFUN([EGG_MSG_WEIRDOS],
[
  AC_MSG_RESULT([Operating System: $egg_cv_var_system_type $egg_cv_var_system_release])
  AC_MSG_RESULT
  if test "$UNKNOWN_OS" = "yes"; then
    AC_MSG_RESULT([Warning:])
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
    if test "$WEIRD_OS" = "yes"; then
      AC_MSG_RESULT([Warning:])
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


dnl
dnl Compiler checks.
dnl


dnl EGG_CHECK_CC()
dnl
dnl Check for a working C compiler.
dnl
AC_DEFUN([EGG_CHECK_CC],
[
  if test "${cross_compiling-x}" = "x"; then
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
  if test "$ac_cv_header_stdc" = "no"; then
    cat << 'EOF' >&2
configure: error:

  Your system must support ANSI C Header files.
  These are required for the language support. Sorry.

EOF
    exit 1
  fi
])


dnl EGG_CHECK_CCPIPE()
dnl
dnl This function checks whether or not the compiler supports the `-pipe' flag,
dnl which speeds up the compilation.
dnl
AC_DEFUN([EGG_CHECK_CCPIPE],
[
  if test -n "$GCC" && test -z "$no_pipe"; then
    AC_CACHE_CHECK([whether the compiler understands -pipe], egg_cv_var_ccpipe, [
        ac_old_CC="$CC"
        CC="$CC -pipe"
        AC_COMPILE_IFELSE([[
          int main ()
          {
            return(0);
          }
        ]], [
          egg_cv_var_ccpipe="yes"
        ], [
          egg_cv_var_ccpipe="no"
        ])
        CC="$ac_old_CC"
    ])

    if test "$egg_cv_var_ccpipe" = "yes"; then
      CC="$CC -pipe"
    fi
  fi
])


dnl EGG_CHECK_CCWALL()
dnl
dnl See if the compiler supports -Wall.
dnl
AC_DEFUN([EGG_CHECK_CCWALL],
[
  if test -n "$GCC" && test -z "$no_wall"; then
    AC_CACHE_CHECK([whether the compiler understands -Wall], egg_cv_var_ccwall, [
      ac_old_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS -Wall"
       AC_COMPILE_IFELSE([[
         int main ()
         {
           return(0);
         }
       ]], [
         egg_cv_var_ccwall="yes"
       ], [
         egg_cv_var_ccwall="no"
       ])
      CFLAGS="$ac_old_CFLAGS"
    ])

    if test "$egg_cv_var_ccwall" = "yes"; then
      CFLAGS="$CFLAGS -Wall"
    fi
  fi
])


dnl
dnl Checks for types and functions.
dnl


dnl EGG_CHECK_SOCKLEN_T()
dnl
dnl Check for the socklen_t type.
dnl
AC_DEFUN([EGG_CHECK_SOCKLEN_T],
[
  AC_CACHE_CHECK([for socklen_t], egg_cv_socklen_t, [
    AC_RUN_IFELSE([[
      #include <unistd.h>
      #include <sys/param.h>
      #include <sys/types.h>
      #include <sys/socket.h>
      #include <netinet/in.h>
      #include <arpa/inet.h>

      int main()
      {
        socklen_t test = 55;

        if (test != 55)
          exit(1);

        return(0);
      }
    ]], [
      egg_cv_socklen_t="yes"
    ], [
      egg_cv_socklen_t="no"
    ], [
      egg_cv_socklen_t="cross"
    ])
  ])

  if test "$egg_cv_socklen_t" = "yes"; then
    AC_DEFINE(HAVE_SOCKLEN_T, 1, [Define to 1 if you have the `socklen_t' type.])
  fi
])


dnl EGG_FUNC_VPRINTF()
dnl
AC_DEFUN([EGG_FUNC_VPRINTF],
[
  AC_FUNC_VPRINTF
  if test "$ac_cv_func_vprintf" = "no"; then
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
dnl This function checks for the proper 'head -1' command variant to use.
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
        if test "`cat conftest.head | $ac_prog`" = "a"; then
          AC_MSG_RESULT([yes])
          ac_cv_prog_HEAD_1="$ac_prog"
        else
          AC_MSG_RESULT([no])
        fi
      fi
    ])
    test -n "$ac_cv_prog_HEAD_1" && break
  done

  if test "${ac_cv_prog_HEAD_1-x}" = "x"; then
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
  AC_ARG_ENABLE([strip],
                [  --enable-strip          enable stripping of executables ],
                [enable_strip="$enableval"],
                [enable_strip="no"])
  if test "$enable_strip" = "yes"; then
    AC_CHECK_PROG(STRIP, strip, strip)
    if test "${STRIP-x}" = "x"; then
      STRIP=touch
    else
      AC_DEFINE(ENABLE_STRIP, 1, [Define if stripping is enabled.])
      cat << 'EOF' >&2
configure: warning:

  Stripping the executable, while saving some disk space, will make bug
  reports nearly worthless. If Eggdrop crashes and you wish to report
  a bug, you will need to recompile with stripping disabled.

EOF
    fi
  else
    STRIP=touch
  fi
])


dnl EGG_PROG_AWK()
dnl
AC_DEFUN([EGG_PROG_AWK],
[
  AC_PROG_AWK
  if test "${AWK-x}" = "x"; then
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
  if test "${BASENAME-x}" = "x"; then
    cat << 'EOF' >&2
configure: error:

  This system seems to lack a working 'basename' command.
  A working 'basename' command is required to compile Eggdrop.

EOF
    exit 1
  fi
])


dnl
dnl Checks for operating system and module support.
dnl


dnl EGG_OS_VERSION()
dnl
AC_DEFUN([EGG_OS_VERSION],
[
  AC_CACHE_CHECK([system type], egg_cv_var_system_type, [egg_cv_var_system_type=`$UNAME -s`])
  AC_CACHE_CHECK([system release], egg_cv_var_system_release, [egg_cv_var_system_release=`$UNAME -r`])
])


dnl EGG_CYGWIN_BINMODE
dnl
dnl Check for binmode.o on Cygwin.
dnl
AC_DEFUN([EGG_CYGWIN_BINMODE],
[
  if test $EGG_CYGWIN = "yes"; then
    AC_MSG_CHECKING([for /usr/lib/binmode.o])
    if test -r /usr/lib/binmode.o; then
      AC_MSG_RESULT([yes])
      LIBS="$LIBS /usr/lib/binmode.o"
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

  if test "x${BUNDLE}" = "x"; then
    cat << 'EOF' >&2
configure: warning:

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

  case "$egg_cv_var_system_type" in
    BSD/OS)
      if test "`echo $egg_cv_var_system_release | cut -d . -f 1`" = "2"; then
        MODULES_OK="no"
      fi
    ;;
    CYGWI*)
      WEIRD_OS="no"
      MOD_EXT="dll"
    ;;
    HP-UX)
      LOAD_METHOD="shl"
    ;;
    dell)
      # Fallthrough.
    ;;
    IRIX)
      # Fallthrough.
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
      # Fallthrough.
    ;;
    QNX)
      # Fallthrough.
      # QNX (recent versions at least) support dlopen().
    ;;
    OSF1)
      case "`echo $egg_cv_var_system_release | cut -d . -f 1`" in
        1.*) LOAD_METHOD="loader" ;;
      esac
    ;;
    SunOS)
      if test "`echo $egg_cv_var_system_release | cut -d . -f 1`" = "5"; then
        # We've had quite a bit of testing on Solaris.
        WEIRD_OS="no"
      else
        # SunOS 4
        AC_DEFINE(DLOPEN_1, 1, [Define if running on SunOS 4.0.])
      fi
    ;;
    *BSD)
      # FreeBSD/OpenBSD/NetBSD all support dlopen() and have had plenty of
      # testing with Eggdrop.
      WEIRD_OS="no"
    ;;
    Darwin)
      # We should support Mac OS X (at least 10.1 and later) now.
      # Use rld on < 10.1.
      if test "$ac_cv_func_NSLinkModule" = "no"; then
        LOAD_METHOD="rld"
      fi
      LOAD_METHOD="dyld"
      EGG_DARWIN_BUNDLE
      MODULE_XLIBS="${BUNDLE} ${MODULE_XLIBS}"
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

  if test "$MODULES_OK" = "yes"; then
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

  if test "$WEIRD_OS" = "yes"; then
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
  RANDMAX="RAND_MAX"

  case "$egg_cv_var_system_type" in
    BSD/OS)
      case "`echo $egg_cv_var_system_release | cut -d . -f 1`" in
        2)
          # Fallthrough.
        ;;
        3)
          MOD_CC="shlicc"
          MOD_LD="shlicc"
          if test ! "$STRIP" = "touch"; then
            MOD_STRIP="$STRIP -d"
          fi
          SHLIB_LD="shlicc -r"
          SHLIB_STRIP="touch"
        ;;
        *)
          if test ! "$STRIP" = "touch"; then
            MOD_STRIP="$STRIP -d"
          fi
          SHLIB_CC="$CC -export-dynamic -fPIC"
          SHLIB_LD="$CC -shared -nostartfiles"
        ;;
      esac
    ;;
    CYGWI*)
      AC_PROG_CC_WIN32
      SHLIB_LD="$CC -shared"
      CC="$CC $WIN32FLAGS"
      MOD_CC="$CC"
      MOD_LD="$CC"
      EGG_CYGWIN="yes"
      EGG_CYGWIN_BINMODE
      AC_DEFINE(CYGWIN_HACKS, 1, [Define if running under Cygwin.])
    ;;
    HP-UX)
      HPUX="yes"
      if test "$CC" = "cc"; then
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
      # Fallthrough.
    ;;
    Linux)
      LINUX="yes"
      MOD_LD="$CC"
      SHLIB_CC="$CC -fPIC"
      SHLIB_LD="$CC -shared -nostartfiles"
    ;;
    Lynx)
      # Fallthrough.
    ;;
    QNX)
      SHLIB_LD="ld -shared"
    ;;
    OSF1)
      case "`echo $egg_cv_var_system_release | cut -d . -f 1`" in
        V*)
          # Digital OSF uses an ancient version of gawk
          if test "$AWK" = "gawk"; then
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
      if test "`echo $egg_cv_var_system_release | cut -d . -f 1`" = "5"; then
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
      # Solaris defines (2^31)-1 as the limit for random() rather than RAND_MAX.
      RANDMAX="0x7FFFFFFF"
    ;;
    *BSD)
      # FreeBSD/OpenBSD/NetBSD
      SHLIB_CC="$CC -fPIC"
      SHLIB_LD="ld -Bshareable -x"
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
  AC_DEFINE_UNQUOTED(RANDOM_MAX, $RANDMAX, [Define limit of random() function.])
])


dnl
dnl Library tests.
dnl


dnl EGG_CHECK_LIBS()
dnl
AC_DEFUN([EGG_CHECK_LIBS],
[
  # FIXME: this needs to be fixed so that it works on IRIX
  if test "$IRIX" = "yes"; then
    AC_MSG_WARN([Skipping library tests because they CONFUSE IRIX.])
  else
    AC_CHECK_LIB(socket, socket)
    AC_CHECK_LIB(nsl, connect)
    AC_CHECK_LIB(dns, gethostbyname)
    AC_CHECK_LIB(dl, dlopen)
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

    if test "$SUNOS" = "yes"; then
      # For suns without yp
      AC_CHECK_LIB(dl, main)
    else
      if test "$HPUX" = "yes"; then
        AC_CHECK_LIB(dld, shl_load)
      fi
    fi
  fi
])


dnl EGG_CHECK_LIBSAFE_SSCANF()
dnl
AC_DEFUN([EGG_CHECK_LIBSAFE_SSCANF],
[
  AC_CACHE_CHECK([for broken libsafe sscanf], egg_cv_var_libsafe_sscanf, [
    AC_RUN_IFELSE([[
      #include <stdio.h>

      int main()
      {
        char *src = "0x001,guppyism\n", dst[10];
        int idx;

        if (sscanf(src, "0x%x,%10c", &idx, dst) == 1)
          exit(1);

        return(0);
      }
    ]], [
      egg_cv_var_libsafe_sscanf="no"
    ], [
      egg_cv_var_libsafe_sscanf="yes"
    ], [
      egg_cv_var_libsafe_sscanf="cross"
    ])
  ])

  if test "$egg_cv_var_libsafe_sscanf" = "yes"; then
    AC_DEFINE(LIBSAFE_HACKS, 1, [Define if you have a version of libsafe with a broken sscanf().])
  fi
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
  if test ! "${EXEEXT-x}" = "x"; then
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
  AC_ARG_WITH(tcllib, [  --with-tcllib=PATH      full path to Tcl library], [tcllibname="$withval"])
  AC_ARG_WITH(tclinc, [  --with-tclinc=PATH      full path to Tcl header],  [tclincname="$withval"])

  WARN=0
  # Make sure either both or neither $tcllibname and $tclincname are set
  if test ! "${tcllibname-x}" = "x"; then
    if test "${tclincname-x}" = "x"; then
      WARN=1
      tcllibname=""
      TCLLIB=""
      TCLINC=""
    fi
  else
    if test ! "${tclincname-x}" = "x"; then
      WARN=1
      tclincname=""
      TCLLIB=""
      TCLINC=""
    fi
  fi

  if test "$WARN" = 1; then
    cat << 'EOF' >&2
configure: warning:

  You must specify both --with-tcllib and --with-tclinc for either to work.

  configure will now attempt to autodetect both the Tcl library and header.

EOF
  fi
])


dnl EGG_TCL_ENV()
dnl
AC_DEFUN([EGG_TCL_ENV],
[
  WARN=0
  # Make sure either both or neither $TCLLIB and $TCLINC are set
  if test ! "${TCLLIB-x}" = "x"; then
    if test "${TCLINC-x}" = "x"; then
      WARN=1
      WVAR1=TCLLIB
      WVAR2=TCLINC
      TCLLIB=""
    fi
  else
    if test ! "${TCLINC-x}" = "x"; then
      WARN=1
      WVAR1=TCLINC
      WVAR2=TCLLIB
      TCLINC=""
    fi
  fi

  if test "$WARN" = 1; then
    cat << EOF >&2
configure: warning:

  Environment variable $WVAR1 was set, but I did not detect ${WVAR2}.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.

  configure will now attempt to autodetect both the Tcl library and header.

EOF
  fi
])


dnl EGG_TCL_WITH_TCLLIB()
dnl
AC_DEFUN([EGG_TCL_WITH_TCLLIB],
[
  # Look for Tcl library: if $tcllibname is set, check there first
  if test ! "${tcllibname-x}" = "x"; then
    if test -f "$tcllibname" && test -r "$tcllibname"; then
      TCLLIB=`echo $tcllibname | sed 's%/[[^/]][[^/]]*$%%'`
      TCLLIBFN=`$BASENAME $tcllibname | cut -c4-`
      TCLLIBEXT=".`echo $TCLLIBFN | $AWK '{j=split([$]1, i, "."); print i[[j]]}'`"
      TCLLIBFNS=`$BASENAME $tcllibname $TCLLIBEXT | cut -c4-`
    else
      cat << EOF >&2
configure: warning:

  The file '$tcllibname' given to option --with-tcllib is not valid.

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
  if test ! "${tclincname-x}" = "x"; then
    if test -f "$tclincname" && test -r "$tclincname"; then
      TCLINC=`echo $tclincname | sed 's%/[[^/]][[^/]]*$%%'`
      TCLINCFN=`$BASENAME $tclincname`
    else
      cat << EOF >&2
configure: warning:

  The file '$tclincname' given to option --with-tclinc is not valid.

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


dnl EGG_TCL_FIND_LIBRARY()
dnl
AC_DEFUN([EGG_TCL_FIND_LIBRARY],
[
  # Look for Tcl library: if $TCLLIB is set, check there first
  if test "${TCLLIBFN-x}" = "x" && test ! "${TCLLIB-x}" = "x"; then
    if test -d "$TCLLIB"; then
      for tcllibfns in $tcllibnames; do
        for tcllibext in $tcllibextensions; do
          if test -r "${TCLLIB}/lib${tcllibfns}${tcllibext}"; then
            TCLLIBFN="${tcllibfns}${tcllibext}"
            TCLLIBEXT="$tcllibext"
            TCLLIBFNS="$tcllibfns"
            break 2
          fi
        done
      done
    fi

    if test "${TCLLIBFN-x}" = "x"; then
      cat << 'EOF' >&2
configure: warning:

  Environment variable TCLLIB was set, but incorrectly.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.

  configure will now attempt to autodetect both the Tcl library and header.

EOF
      TCLLIB=""
      TCLLIBFN=""
      TCLINC=""
      TCLINCFN=""
    fi
  fi
])


dnl EGG_TCL_FIND_HEADER()
dnl
AC_DEFUN([EGG_TCL_FIND_HEADER],
[
  # Look for Tcl header: if $TCLINC is set, check there first
  if test "${TCLINCFN-x}" = "x" && test ! "${TCLINC-x}" = "x"; then
    if test -d "$TCLINC"; then
      for tclheaderfn in $tclheadernames; do
        if test -r "${TCLINC}/${tclheaderfn}"; then
          TCLINCFN="$tclheaderfn"
          break
        fi
      done
    fi

    if test "${TCLINCFN-x}" = "x"; then
      cat << 'EOF' >&2
configure: warning:

  Environment variable TCLINC was set, but incorrectly.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.

  configure will now attempt to autodetect both the Tcl library and header.

EOF
      TCLLIB=""
      TCLLIBFN=""
      TCLINC=""
      TCLINCFN=""
    fi
  fi
])


dnl EGG_TCL_CHECK_LIBRARY()
dnl
AC_DEFUN([EGG_TCL_CHECK_LIBRARY],
[
  AC_MSG_CHECKING([for Tcl library])

  # Attempt autodetect for $TCLLIBFN if it's not set
  if test ! "${TCLLIBFN-x}" = "x"; then
    AC_MSG_RESULT([using ${TCLLIB}/lib${TCLLIBFN}])
  else
    for tcllibfns in $tcllibnames; do
      for tcllibext in $tcllibextensions; do
        for tcllibpath in $tcllibpaths; do
          if test -r "${tcllibpath}/lib${tcllibfns}${tcllibext}"; then
            AC_MSG_RESULT([found ${tcllibpath}/lib${tcllibfns}${tcllibext}])
            TCLLIB="$tcllibpath"
            TCLLIBFN="${tcllibfns}${tcllibext}"
            TCLLIBEXT="$tcllibext"
            TCLLIBFNS="$tcllibfns"
            break 3
          fi
        done
      done
    done
  fi

  # Show if $TCLLIBFN wasn't found
  if test "${TCLLIBFN-x}" = "x"; then
    AC_MSG_RESULT([not found])
  fi

  AC_SUBST(TCLLIB)
  AC_SUBST(TCLLIBFN)
])


dnl EGG_TCL_CHECK_HEADER()
dnl
AC_DEFUN([EGG_TCL_CHECK_HEADER],
[
  AC_MSG_CHECKING([for Tcl header])

  # Attempt autodetect for $TCLINCFN if it's not set
  if test ! "${TCLINCFN-x}" = "x"; then
    AC_MSG_RESULT([using ${TCLINC}/${TCLINCFN}])
  else
    for tclheaderpath in $tclheaderpaths; do
      for tclheaderfn in $tclheadernames; do
        if test -r "${tclheaderpath}/${tclheaderfn}"; then
          AC_MSG_RESULT([found ${tclheaderpath}/${tclheaderfn}])
          TCLINC="$tclheaderpath"
          TCLINCFN="$tclheaderfn"
          break 2
        fi
      done
    done

    # FreeBSD hack ...
    if test "${TCLINCFN-x}" = "x"; then
      for tcllibfns in $tcllibnames; do
        for tclheaderpath in $tclheaderpaths; do
          for tclheaderfn in $tclheadernames; do
            if test -r "${tclheaderpath}/${tcllibfns}/${tclheaderfn}"; then
              AC_MSG_RESULT([found ${tclheaderpath}/${tcllibfns}/${tclheaderfn}])
              TCLINC="${tclheaderpath}/${tcllibfns}"
              TCLINCFN="$tclheaderfn"
              break 3
            fi
          done
        done
      done
    fi
  fi

  if test "${TCLINCFN-x}" = "x"; then
    AC_MSG_RESULT({not found})
  fi

  AC_SUBST(TCLINC)
  AC_SUBST(TCLINCFN)
])


dnl EGG_CACHE_UNSET(CACHE-ID)
dnl
dnl Unsets a certain cache item. Typically called before using the AC_CACHE_*()
dnl macros.
dnl
AC_DEFUN([EGG_CACHE_UNSET], [unset $1])


dnl EGG_TCL_DETECT_CHANGE()
dnl
dnl Detect whether the Tcl system has changed since our last configure run.
dnl Set egg_tcl_changed accordingly.
dnl
dnl Tcl related feature and version checks should re-run their checks as soon
dnl as egg_tcl_changed is set to "yes".
dnl
AC_DEFUN([EGG_TCL_DETECT_CHANGE],
[
  AC_MSG_CHECKING([whether the Tcl system has changed])
  egg_tcl_changed="yes"
  egg_tcl_id="${TCLLIB}:${TCLLIBFN}:${TCLINC}:${TCLINCFN}"
  if test ! "$egg_tcl_id" = ":::"; then
    egg_tcl_cached="yes"
    AC_CACHE_VAL(egg_cv_var_tcl_id, [
      egg_cv_var_tcl_id="$egg_tcl_id"
      egg_tcl_cached="no"
    ])
    if test "$egg_tcl_cached" = "yes"; then
      if test "${egg_cv_var_tcl_id-x}" = "${egg_tcl_id-x}"; then
        egg_tcl_changed="no"
      else
        egg_cv_var_tcl_id="$egg_tcl_id"
      fi
    fi
  fi

  if test "$egg_tcl_changed" = "yes"; then
    AC_MSG_RESULT([yes])
  else
    AC_MSG_RESULT([no])
  fi
])


dnl EGG_TCL_CHECK_VERSION()
dnl
AC_DEFUN([EGG_TCL_CHECK_VERSION],
[
  # Both TCLLIBFN & TCLINCFN must be set, or we bail
  TCL_FOUND=0
  if test ! "${TCLLIBFN-x}" = "x" && test ! "${TCLINCFN-x}" = "x"; then
    TCL_FOUND=1

    # Check Tcl's version
    if test "$egg_tcl_changed" = "yes"; then
      EGG_CACHE_UNSET(egg_cv_var_tcl_version)
    fi

    AC_MSG_CHECKING([for Tcl version])
    AC_CACHE_VAL(egg_cv_var_tcl_version, [
      egg_cv_var_tcl_version=`grep TCL_VERSION $TCLINC/$TCLINCFN | $HEAD_1 | $AWK '{gsub(/\"/, "", [$]3); print [$]3}'`
    ])

    if test ! "${egg_cv_var_tcl_version-x}" = "x"; then
      AC_MSG_RESULT([$egg_cv_var_tcl_version])
    else
      AC_MSG_RESULT([not found])
      TCL_FOUND=0
    fi

    # Check Tcl's patch level (if available)
    if test "$egg_tcl_changed" = "yes"; then
      EGG_CACHE_UNSET(egg_cv_var_tcl_patch_level)
    fi
    AC_MSG_CHECKING([for Tcl patch level])
    AC_CACHE_VAL(egg_cv_var_tcl_patch_level, [
      eval "egg_cv_var_tcl_patch_level=`grep TCL_PATCH_LEVEL $TCLINC/$TCLINCFN | $HEAD_1 | $AWK '{gsub(/\"/, "", [$]3); print [$]3}'`"
    ])

    if test ! "${egg_cv_var_tcl_patch_level-x}" = "x"; then
      AC_MSG_RESULT([$egg_cv_var_tcl_patch_level])
    else
      egg_cv_var_tcl_patch_level="unknown"
      AC_MSG_RESULT([unknown])
    fi
  fi

  # Check if we found Tcl's version
  if test "$TCL_FOUND" = 0; then
    cat << 'EOF' >&2
configure: error:

  Tcl cannot be found on this system.

  Eggdrop requires Tcl to compile. If you already have Tcl installed on
  this system, and I just wasn't looking in the right place for it, re-run
  ./configure using the --with-tcllib='/path/to/libtcl.so' and
  --with-tclinc='/path/to/tcl.h' options.

  See doc/COMPILE-GUIDE's 'Tcl Detection and Installation' section for more
  information.

EOF
    exit 1
  fi
])


dnl EGG_TCL_CHECK_PRE70()
dnl
AC_DEFUN([EGG_TCL_CHECK_PRE70],
[
  # Is this version of Tcl too old for us to use ?
  TCL_VER_PRE70=`echo $egg_cv_var_tcl_version | $AWK '{split([$]1, i, "."); if (i[[1]] < 7) print "yes"; else print "no"}'`
  if test "$TCL_VER_PRE70" = "yes"; then
    cat << EOF >&2
configure: error:

  Your Tcl version is much too old for Eggdrop to use. You should
  download and compile a more recent version. The most reliable
  current version is $tclrecommendver and can be downloaded from
  ${tclrecommendsite}.

  See doc/COMPILE-GUIDE's 'Tcl Detection and Installation' section
  for more information.

EOF
    exit 1
  fi
])


dnl EGG_TCL_TESTLIBS()
dnl
AC_DEFUN([EGG_TCL_TESTLIBS],
[
  # Set variables for Tcl library tests
  TCL_TEST_LIB="$TCLLIBFNS"
  TCL_TEST_OTHERLIBS="-L$TCLLIB $EGG_MATH_LIB"
  if test ! "${ac_cv_lib_pthread-x}" = "x"; then
    TCL_TEST_OTHERLIBS="$TCL_TEST_OTHERLIBS $ac_cv_lib_pthread"
  fi
])


dnl EGG_TCL_CHECK_FREE()
dnl
AC_DEFUN([EGG_TCL_CHECK_FREE],
[
  if test "$egg_tcl_changed" = "yes"; then
    EGG_CACHE_UNSET(egg_cv_var_tcl_free)
  fi

  # Check for Tcl_Free()
  AC_CHECK_LIB($TCL_TEST_LIB, Tcl_Free, [egg_cv_var_tcl_free="yes"], [egg_cv_var_tcl_free="no"], $TCL_TEST_OTHERLIBS)

  if test "$egg_cv_var_tcl_free" = "yes"; then
    AC_DEFINE(HAVE_TCL_FREE, 1, [Define for Tcl that has Tcl_Free() (7.5p1 and later).])
  fi
])


dnl EGG_TCL_ENABLE_THREADS()
dnl
AC_DEFUN([EGG_TCL_ENABLE_THREADS],
[
  AC_ARG_ENABLE(tcl-threads, [  --enable-tcl-threads    force threaded Tcl library support enabled], [enable_tcl_threads="$enableval"], [enable_tcl_threads="auto"])
  AC_ARG_ENABLE(tcl-threads, [  --disable-tcl-threads   force threaded Tcl library support disabled], [enable_tcl_threads="$enableval"], [enable_tcl_threads="auto"])
])


dnl EGG_TCL_CHECK_THREADS()
dnl
AC_DEFUN([EGG_TCL_CHECK_THREADS],
[
  if test "$egg_tcl_changed" = "yes"; then
    EGG_CACHE_UNSET(egg_cv_var_tcl_threaded)
  fi

  # Check for TclpFinalizeThreadData()
  AC_CHECK_LIB($TCL_TEST_LIB, TclpFinalizeThreadData, [egg_cv_var_tcl_threaded="yes"], [egg_cv_var_tcl_threaded="no"], $TCL_TEST_OTHERLIBS)

  WANT_TCL_THREADS=0

  # Do this check first just in case someone tries to use both
  # --enable-tcl-threads and --disable-tcl-threads or gives an
  # invalid --enable-tcl-threads=foo option.
  if test "$egg_cv_var_tcl_threaded" = "yes"; then
    WANT_TCL_THREADS=1
  fi

  # Check if either --enable-tcl-threads or --disable-tcl-threads was used
  if test ! "$enable_tcl_threads" = "auto"; then

    # Make sure an invalid option wasn't passed as --enable-tcl-threads=foo
    if test ! "$enable_tcl_threads" = "yes" && test ! "$enable_tcl_threads" = "no"; then
      AC_MSG_WARN([Invalid option '$enable_tcl_threads' passed to --enable-tcl-threads, defaulting to 'yes'])

      enable_tcl_threads=yes
    fi

    # The --enable-tcl-threads option was used
    if test "$enable_tcl_threads" = "yes"; then
      WANT_TCL_THREADS=1

      if test "$egg_cv_var_tcl_threaded" = "no"; then
        cat << 'EOF' >&2
configure: WARNING:

  You have enabled threaded Tcl library support but a threaded
  Tcl library was not detected.

  Eggdrop may fail to compile or run properly if threaded Tcl
  library support is enabled and the Tcl library isn't threaded.

EOF
      fi        
    fi

    # The --disable-tcl-threads option was used
    if test "$enable_tcl_threads" = "no"; then
      WANT_TCL_THREADS=0

      if test "$egg_cv_var_tcl_threaded" = "yes"; then
        cat << 'EOF' >&2
configure: WARNING:

  You have disabled threaded Tcl library support but a threaded
  Tcl library was detected.

  Eggdrop may fail to compile or run properly if threaded Tcl
  library support is disabled and the Tcl library is threaded.

EOF
      fi
    fi
  fi

  AC_MSG_CHECKING([whether threaded Tcl library support will be enabled])

  if test $WANT_TCL_THREADS = 1; then
    AC_MSG_RESULT([yes])
    AC_DEFINE(HAVE_TCL_THREADS, 1, [Define if Tcl library is threads-enabled.])

    # Add pthread library to $LIBS if we need it for threaded Tcl
    if test ! "${ac_cv_lib_pthread-x}" = "x"; then
      LIBS="$ac_cv_lib_pthread $LIBS"
    fi
  else
    AC_MSG_RESULT([no])
  fi
])


dnl EGG_TCL_LIB_REQS()
dnl
AC_DEFUN([EGG_TCL_LIB_REQS],
[
  if test "$EGG_CYGWIN" = "yes"; then
    TCL_REQS="${TCLLIB}/lib${TCLLIBFN}"
    TCL_LIBS="-L$TCLLIB -l$TCLLIBFNS $EGG_MATH_LIB"
  else
    if test ! "$TCLLIBEXT" = ".a"; then
      TCL_REQS="${TCLLIB}/lib${TCLLIBFN}"
      TCL_LIBS="-L$TCLLIB -l$TCLLIBFNS $EGG_MATH_LIB"
    else
      # Set default make as static for unshared Tcl library
      if test ! "$DEFAULT_MAKE" = "static"; then
        cat << 'EOF' >&2
configure: warning:

  Your Tcl library is not a shared lib.
  configure will now set default make type to static.

EOF
        DEFAULT_MAKE="static"
        AC_SUBST(DEFAULT_MAKE)
      fi

      # Are we using a pre 7.4 Tcl version ?
      TCL_VER_PRE74=`echo $egg_cv_var_tcl_version | $AWK '{split([$]1, i, "."); if (((i[[1]] == 7) && (i[[2]] < 4)) || (i[[1]] < 7)) print "yes"; else print "no"}'`
      if test "$TCL_VER_PRE74" = "no"; then

        # Was the --with-tcllib option given ?
        if test ! "${tcllibname-x}" = "x"; then
          TCL_REQS="${TCLLIB}/lib${TCLLIBFN}"
          TCL_LIBS="${TCLLIB}/lib${TCLLIBFN} $EGG_MATH_LIB"
        else
          TCL_REQS="${TCLLIB}/lib${TCLLIBFN}"
          TCL_LIBS="-L$TCLLIB -l$TCLLIBFNS $EGG_MATH_LIB"
        fi
      else
        cat << EOF >&2
configure: warning:

  Your Tcl version ($egg_cv_var_tcl_version) is older then 7.4.
  There are known problems, but we will attempt to work around them.

EOF
        TCL_REQS="libtcle.a"
        TCL_LIBS="-L`pwd` -ltcle $EGG_MATH_LIB"
      fi
    fi
  fi

  AC_SUBST(TCL_REQS)
  AC_SUBST(TCL_LIBS)
])


dnl EGG_SUBST_EGGVERSION()
dnl
AC_DEFUN([EGG_SUBST_EGGVERSION],
[
  EGGVERSION=`grep 'char.egg_version' $srcdir/src/main.c | $AWK '{gsub(/(\"|\;)/, "", [$]4); print [$]4}'`
  egg_version_num=`echo $EGGVERSION | $AWK 'BEGIN {FS = "."} {printf("%d%02d%02d", [$]1, [$]2, [$]3)}'`
  AC_SUBST(EGGVERSION)
  AC_DEFINE_UNQUOTED(EGG_VERSION, $egg_version_num, [Defines the current Eggdrop version.])
])


dnl EGG_SUBST_DEST()
AC_DEFUN([EGG_SUBST_DEST],
[
  if test "${DEST-x}" = "x"; then
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
  EGG_REPLACE_IF_CHANGED(lush.h, [
    cat > conftest.out << EOF

/* Ignore me but do not erase me. I am a kludge. */

#include "${egg_tclinc}/${egg_tclincfn}"

EOF
  ], [
    egg_tclinc="$TCLINC"
    egg_tclincfn="$TCLINCFN"
  ])
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
])


dnl AC_PROG_CC_WIN32()
dnl
AC_DEFUN([AC_PROG_CC_WIN32],
[
  AC_MSG_CHECKING([how to access the Win32 API])
  WIN32FLAGS=
  AC_COMPILE_IFELSE([[
    #ifndef WIN32
    #  ifndef _WIN32
    #    error WIN32 or _WIN32 not defined
    #  endif
    #endif
  ]], [
    AC_MSG_RESULT([present by default])
  ], [
    ac_compile_save="$ac_compile"
    save_CC="$CC"
    ac_compile="$ac_compile -mwin32"
    CC="$CC -mwin32"
    AC_COMPILE_IFELSE([[
      #ifndef WIN32
      #  ifndef _WIN32
      #    error WIN32 or _WIN32 not defined
      #  endif
      #endif
    ]], [
      AC_MSG_RESULT([found via -mwin32])
      ac_compile="$ac_compile_save"
      CC="$save_CC"
      WIN32FLAGS="-mwin32"
    ], [
      ac_compile="$ac_compile_save"
      CC="$save_CC"
      AC_MSG_RESULT([not found])
    ])
  ])
])
