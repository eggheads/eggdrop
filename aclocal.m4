dnl aclocal.m4: macros autoconf uses when building configure from configure.ac
dnl
dnl Copyright (C) 1999, 2000, 2001, 2002, 2003 Eggheads Development Team
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
dnl $Id: aclocal.m4,v 1.86 2004/01/18 03:57:26 wcc Exp $
dnl

dnl EGG_MSG_CONFIGURE_START()
AC_DEFUN([EGG_MSG_CONFIGURE_START],
[
  AC_MSG_RESULT()
  AC_MSG_RESULT([This is Eggdrop's GNU configure script.])
  AC_MSG_RESULT([It's going to run a bunch of tests to hopefully make your compile])
  AC_MSG_RESULT([work without much twiddling.])
  AC_MSG_RESULT()
])


dnl EGG_MSG_CONFIGURE_END()
AC_DEFUN([EGG_MSG_CONFIGURE_END],
[
  AC_MSG_RESULT()
  AC_MSG_RESULT([Configure is done.])
  AC_MSG_RESULT()
  AC_MSG_RESULT([Type 'make config' to configure the modules, or type 'make iconfig'])
  AC_MSG_RESULT([to interactively choose which modules to compile.])
  AC_MSG_RESULT()
])


dnl EGG_CHECK_CC()
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

dnl EGG_CHECK_CCPIPE()
dnl
dnl This function checks whether or not the compiler supports the `-pipe' flag,
dnl which speeds up the compilation.
AC_DEFUN([EGG_CHECK_CCPIPE],
[
  if test -z "$no_pipe"; then
    if test -n "$GCC"; then
      AC_CACHE_CHECK([whether the compiler understands -pipe],
                     egg_cv_var_ccpipe,
      [
         ac_old_CC="$CC"
         CC="$CC -pipe"
         AC_COMPILE_IFELSE([[int main () { return(0); }]],
                           [egg_cv_var_ccpipe="yes"],
                           [egg_cv_var_ccpipe="no"])
         CC="$ac_old_CC"
      ])
      if test "$egg_cv_var_ccpipe" = "yes"; then
        CC="$CC -pipe"
      fi
    fi
  fi
])


dnl EGG_PROG_HEAD_1()
dnl
dnl This function checks for the proper 'head -1' command variant to use.
AC_DEFUN([EGG_PROG_HEAD_1],
[
  cat << 'EOF' > conftest.head
a
b
c
EOF

  for ac_prog in 'head -1' 'head -n 1' 'sed 1q'; do
    AC_MSG_CHECKING([whether $ac_prog works])
    AC_CACHE_VAL([ac_cv_prog_HEAD_1],
    [
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

dnl EGG_CHECK_OS()
dnl
dnl FIXME/NOTICE:
dnl   This function is obsolete. Any NEW code/checks should be written as
dnl   individual tests that will be checked on ALL operating systems.
AC_DEFUN([EGG_CHECK_OS],
[
  LINUX="no"
  IRIX="no"
  SUNOS="no"
  HPUX="no"
  EGG_CYGWIN="no"
  MOD_CC="$CC"
  MOD_LD="$CC"
  MOD_STRIP="$STRIP"
  SHLIB_CC="$CC"
  SHLIB_LD="$CC"
  SHLIB_STRIP="$STRIP"
  NEED_DL=1
  DEFAULT_MAKE="debug"
  MOD_EXT="so"

  AC_CACHE_CHECK([system type],
                 egg_cv_var_system_type,
                 [egg_cv_var_system_type=`$UNAME -s`])
  AC_CACHE_CHECK([system release],
                 egg_cv_var_system_release,
                 [egg_cv_var_system_release=`$UNAME -r`])

  case "$egg_cv_var_system_type" in
    BSD/OS)
      case "`echo $egg_cv_var_system_release | cut -d . -f 1`" in
        2)
          NEED_DL=0
          DEFAULT_MAKE="static"
        ;;
        3)
          MOD_CC="shlicc"
          MOD_LD="shlicc"
          if test ! "$STRIP" = "touch"; then
            MOD_STRIP="$STRIP -d"
          fi
          SHLIB_LD="shlicc -r"
          SHLIB_STRIP="touch"
          AC_DEFINE(MODULES_OK, 1, [Define if modules will work on your system.])
        ;;
        *)
          CFLAGS="$CFLAGS -Wall"
          MOD_LD="$CC"
          if test ! "$STRIP" = "touch"; then
            MOD_STRIP="$STRIP -d"
          fi
          SHLIB_CC="$CC -export-dynamic -fPIC"
          SHLIB_LD="$CC -shared -nostartfiles"
          AC_DEFINE(MODULES_OK, 1, [Define if modules will work on your system.])
        ;;
      esac
    ;;
    CYGWI*)
      case "`echo $egg_cv_var_system_release | cut -c 1-3`" in
        1.*)
          NEED_DL=0
          SHLIB_LD="$CC -shared"
          AC_PROG_CC_WIN32
          CC="$CC $WIN32FLAGS"
          MOD_CC="$CC"
          MOD_LD="$CC"
          AC_MSG_CHECKING([for /usr/lib/binmode.o])
          if test -r /usr/lib/binmode.o; then
            AC_MSG_RESULT([yes])
            LIBS="$LIBS /usr/lib/binmode.o"
          else
            AC_MSG_RESULT([no])
            AC_MSG_WARN([Make sure the directory Eggdrop is installed into is mounted in binary mode.])
          fi
          MOD_EXT="dll"
          AC_DEFINE(MODULES_OK, 1, [Define if modules will work on your system.])
        ;;
        *)
          NEED_DL=0
          DEFAULT_MAKE="static"
        ;;
      esac
      EGG_CYGWIN="yes"
      AC_DEFINE(CYGWIN_HACKS, 1, [Define if running under Cygwin.])
    ;;
    HP-UX)
      HPUX="yes"
      MOD_LD="$CC -fPIC -shared"
      SHLIB_CC="$CC -fPIC"
      SHLIB_LD="ld -b"
      NEED_DL=0
      AC_DEFINE(MODULES_OK, 1, [Define if modules will work on your system.])
      AC_DEFINE(HPUX_HACKS, 1, [Define if running on HPUX that supports dynamic linking.])
      if test "`echo $egg_cv_var_system_release | cut -d . -f 2`" = "10"
      then
        AC_DEFINE(HPUX10_HACKS, 1, [Define if running on HPUX 10.x.])
      fi
    ;;
    dell)
      AC_MSG_RESULT(Dell SVR4)
      SHLIB_STRIP="touch"
      NEED_DL=0
      MOD_LD="$CC -lelf -lucb"
    ;;
    IRIX)
      SHLIB_LD="ld -n32 -shared -rdata_shared"
      IRIX="yes"
      SHLIB_STRIP="touch"
      NEED_DL=0
      DEFAULT_MAKE="static"
    ;;
    Ultrix)
      NEED_DL=0
      SHLIB_STRIP="touch"
      DEFAULT_MAKE="static"
      SHELL="/bin/sh5"
    ;;
    SINIX*)
      NEED_DL=0
      SHLIB_STRIP="touch"
      DEFAULT_MAKE="static"
      SHLIB_CC="cc -G"
    ;;
    BeOS)
      NEED_DL=0
      DEFAULT_MAKE="static"
    ;;
    Linux)
      LINUX="yes"
      CFLAGS="$CFLAGS -Wall"
      MOD_LD="$CC"
      SHLIB_CC="$CC -fPIC"
      SHLIB_LD="$CC -shared -nostartfiles"
      AC_DEFINE(MODULES_OK, 1, [Define if modules will work on your system.])
    ;;
    Lynx)
      NEED_DL=0
      DEFAULT_MAKE="static"
    ;;
    QNX)
      NEED_DL=0
      DEFAULT_MAKE="static"
      SHLIB_LD="ld -shared"
    ;;
    OSF1)
      case "`echo $egg_cv_var_system_release | cut -d . -f 1`" in
        V*)
          # FIXME: we should check this in a separate test
          # Digital OSF uses an ancient version of gawk
          if test "$AWK" = "gawk"; then
            AWK="awk"
          fi
          SHLIB_LD="ld -shared -expect_unresolved \"'*'\""
          SHLIB_STRIP="touch"
          AC_DEFINE(MODULES_OK, 1, [Define if modules will work on your system.])
        ;;
        1.0|1.1|1.2)
          SHLIB_LD="ld -R -export $@:"
          AC_DEFINE(BROKEN_SNPRINTF, 1, [Define to use Eggdrop's snprintf functions regardless of HAVE_SNPRINTF.])
          AC_DEFINE(MODULES_OK, 1, [Define if modules will work on your system.])
          AC_DEFINE(OSF1_HACKS, 1, [Define if running on OSF/1 platform.])
        ;;
        1.*)
          SHLIB_CC="$CC -fpic"
          SHLIB_LD="ld -shared"
          AC_DEFINE(BROKEN_SNPRINTF, 1, [Define to use Eggdrop's snprintf functions regardless of HAVE_SNPRINTF.])
          AC_DEFINE(MODULES_OK, 1, [Define if modules will work on your system.])
          AC_DEFINE(OSF1_HACKS, 1, [Define if running on OSF/1 platform.])
        ;;
        *)
          AC_DEFINE(BROKEN_SNPRINTF, 1, [Define to use Eggdrop's snprintf functions regardless of HAVE_SNPRINTF.])
          NEED_DL=0
          DEFAULT_MAKE="static"
        ;;
      esac
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
        AC_DEFINE(DLOPEN_1, 1, [Define if running on SunOS 4.0.])
      fi
      AC_DEFINE(MODULES_OK, 1, [Define if modules will work on your system.])
    ;;
    *BSD)
      # FreeBSD/OpenBSD/NetBSD
      SHLIB_CC="$CC -fPIC"
      SHLIB_LD="ld -Bshareable -x"
      AC_DEFINE(MODULES_OK, 1, [Define if modules will work on your system.])
    ;;
    Darwin)
      # Mac OS X
      SHLIB_CC="$CC -fPIC"
      SHLIB_LD="ld -Bshareable -x"
      AC_DEFINE(MODULES_OK, 1, [Define if modules will work on your system.])
    ;;
    *)
      AC_MSG_CHECKING([if system is Mach based])
      if test -r /mach; then
        AC_MSG_RESULT([yes])
        NEED_DL=0
        DEFAULT_MAKE="static"
        AC_DEFINE(BORGCUBES, 1, [Define if running on NeXT Step.])
      else
        AC_MSG_RESULT([no])
        AC_MSG_CHECKING([if system is QNX])
        if test -r /cmds; then
          AC_MSG_RESULT([yes])
          SHLIB_STRIP="touch"
          NEED_DL=0
          DEFAULT_MAKE="static"
        else
          AC_MSG_RESULT([no])
          AC_MSG_RESULT([unknown])
          AC_MSG_RESULT([If you get modules to work, be sure to let the development team know how (eggdev@eggheads.org).])
          NEED_DL=0
          DEFAULT_MAKE="static"
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
  AC_SUBST(DEFAULT_MAKE)
  AC_SUBST(MOD_EXT)
  AC_DEFINE_UNQUOTED(EGG_MOD_EXT, "$MOD_EXT", [Defines the extension of Eggdrop modules.])
])


dnl EGG_CHECK_LIBS()
AC_DEFUN([EGG_CHECK_LIBS],
[
  # FIXME: this needs to be fixed so that it works on IRIX
  if test "$IRIX" = "yes"
  then
    AC_MSG_WARN([Skipping library tests because they CONFUSE IRIX.])
  else
    AC_CHECK_LIB(socket, socket)
    AC_CHECK_LIB(nsl, connect)
    AC_CHECK_LIB(dns, gethostbyname)
    AC_CHECK_LIB(dl, dlopen)
    AC_CHECK_LIB(m, tan, EGG_MATH_LIB="-lm")
    # This is needed for Tcl libraries compiled with thread support
    AC_CHECK_LIB(pthread, pthread_mutex_init,
    [
      ac_cv_lib_pthread_pthread_mutex_init="yes"
      ac_cv_lib_pthread="-lpthread"
    ], [
      AC_CHECK_LIB(pthread, __pthread_mutex_init,
      [
        ac_cv_lib_pthread_pthread_mutex_init="yes"
        ac_cv_lib_pthread="-lpthread"
      ], [
        AC_CHECK_LIB(pthreads, pthread_mutex_init,
        [
          ac_cv_lib_pthread_pthread_mutex_init="yes"
          ac_cv_lib_pthread="-lpthreads"
        ], [
          AC_CHECK_FUNC(pthread_mutex_init,
          [
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


dnl EGG_FUNC_VPRINTF()
AC_DEFUN([EGG_FUNC_VPRINTF],
[
  if test "$ac_cv_func_vprintf" = "no"; then
    cat << 'EOF' >&2
configure: error:

  Your system does not have the vprintf/vsprintf/sprintf libraries.
  These are required to compile almost anything. Sorry.

EOF
    exit 1
  fi
])


dnl EGG_HEADER_STDC()
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


dnl EGG_CHECK_LIBSAFE_SSCANF()
AC_DEFUN([EGG_CHECK_LIBSAFE_SSCANF],
[
  AC_CACHE_CHECK([for broken libsafe sscanf], [egg_cv_var_libsafe_sscanf],
  [
    AC_RUN_IFELSE(
    [[
      #include <stdio.h>

      int main()
      {
        char *src = "0x001,guppyism\n", dst[10];
        int idx;

        if (sscanf(src, "0x%x,%10c", &idx, dst) == 1)
          exit(1);
        return 0;
      }
    ]], [
      egg_cv_var_libsafe_sscanf="no"
    ], [
      egg_cv_var_libsafe_sscanf="yes"
    ], [
      egg_cv_var_libsafe_sscanf="no"
    ])
  ])
  if test "$egg_cv_var_libsafe_sscanf" = "yes"; then
    AC_DEFINE(LIBSAFE_HACKS, 1, [Define if you have a version of libsafe with a broken sscanf().])
  fi
])


dnl EGG_EXEEXT()
dnl
dnl Test for executable suffix and define Eggdrop's executable name accordingly.
AC_DEFUN([EGG_EXEEXT], [
  EGGEXEC="eggdrop"
  AC_EXEEXT
  if test ! "${EXEEXT-x}" = "x"; then
    EGGEXEC="eggdrop$EXEEXT"
  fi
  AC_SUBST(EGGEXEC)
])


dnl EGG_TCL_ARG_WITH()
AC_DEFUN([EGG_TCL_ARG_WITH],
[
  AC_ARG_WITH(tcllib,
              [  --with-tcllib=PATH      full path to Tcl library],
              [tcllibname="$withval"])
  AC_ARG_WITH(tclinc,
              [  --with-tclinc=PATH      full path to Tcl header],
              [tclincname="$withval"])

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
AC_DEFUN([EGG_TCL_FIND_LIBRARY],
[
  # Look for Tcl library: if $TCLLIB is set, check there first
  if test "${TCLLIBFN-x}" = "x"; then
    if test ! "${TCLLIB-x}" = "x"; then
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
  fi
])


dnl EGG_TCL_FIND_HEADER()
AC_DEFUN([EGG_TCL_FIND_HEADER],
[
  # Look for Tcl header: if $TCLINC is set, check there first
  if test "${TCLINCFN-x}" = "x"; then
    if test ! "${TCLINC-x}" = "x"; then
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
  fi
])


dnl EGG_TCL_CHECK_LIBRARY()
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
AC_DEFUN([EGG_CACHE_UNSET], [unset $1])


dnl EGG_TCL_DETECT_CHANGE()
dnl
dnl Detect whether the Tcl system has changed since our last configure run.
dnl Set egg_tcl_changed accordingly.
dnl
dnl Tcl related feature and version checks should re-run their checks as soon
dnl as egg_tcl_changed is set to "yes".
AC_DEFUN([EGG_TCL_DETECT_CHANGE],
[
  AC_MSG_CHECKING([whether the Tcl system has changed])
  egg_tcl_changed="yes"
  egg_tcl_id="${TCLLIB}:${TCLLIBFN}:${TCLINC}:${TCLINCFN}"
  if test ! "$egg_tcl_id" = ":::"; then
    egg_tcl_cached="yes"
    AC_CACHE_VAL(egg_cv_var_tcl_id,
    [
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
    AC_CACHE_VAL(egg_cv_var_tcl_version,
    [
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
    AC_CACHE_VAL(egg_cv_var_tcl_patch_level,
    [
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
  --with-tclinc='/path/to/tcl.h' options. See doc/COMPILING's 'Tcl Detection
  and Installation' section for more information.

EOF
    exit 1
  fi
])


dnl EGG_TCL_CHECK_PRE70()
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
  ${tclrecommendsite}. See doc/COMPILING's 'Tcl
  Detection and Installation' section for more information.

EOF
    exit 1
  fi
])


dnl EGG_TCL_TESTLIBS()
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
AC_DEFUN([EGG_TCL_CHECK_FREE],
[
  if test "$egg_tcl_changed" = "yes"; then
    EGG_CACHE_UNSET(egg_cv_var_tcl_free)
  fi

  # Check for Tcl_Free()
  AC_CHECK_LIB($TCL_TEST_LIB,
               Tcl_Free,
               [egg_cv_var_tcl_free="yes"],
               [egg_cv_var_tcl_free="no"],
               $TCL_TEST_OTHERLIBS)

  if test "$egg_cv_var_tcl_free" = "yes"; then
    AC_DEFINE(HAVE_TCL_FREE, 1, [Define for Tcl that has Tcl_Free() (7.5p1 and later).])
  fi
])


dnl EGG_TCL_ENABLE_THREADS()
AC_DEFUN([EGG_TCL_ENABLE_THREADS],
[
  AC_ARG_ENABLE(tcl-threads,
                [  --disable-tcl-threads   disable threaded Tcl support if detected ],
                [enable_tcl_threads="$enableval"],
                [enable_tcl_threads="yes"])
])


dnl EGG_TCL_CHECK_THREADS()
AC_DEFUN([EGG_TCL_CHECK_THREADS],
[
  if test "$egg_tcl_changed" = "yes"; then
    EGG_CACHE_UNSET(egg_cv_var_tcl_threaded)
  fi

  # Check for TclpFinalizeThreadData()
  AC_CHECK_LIB($TCL_TEST_LIB,
               TclpFinalizeThreadData,
               [egg_cv_var_tcl_threaded="yes"],
               [egg_cv_var_tcl_threaded="no"],
               $TCL_TEST_OTHERLIBS)

  if test "$egg_cv_var_tcl_threaded" = "yes"; then
    if test "$enable_tcl_threads" = "no"; then

      cat << 'EOF' >&2
configure: warning:

  You have disabled threads support on a system with a threaded Tcl library.
  Tcl features that rely on scheduled events may not function properly.

EOF
    else
      AC_DEFINE(HAVE_TCL_THREADS, 1, [Define for Tcl that has threads.])
    fi

    # Add pthread library to $LIBS if we need it
    if test ! "${ac_cv_lib_pthread-x}" = "x"; then
      LIBS="$ac_cv_lib_pthread $LIBS"
    fi
  fi
])


dnl EGG_TCL_LIB_REQS()
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


dnl EGG_FUNC_DLOPEN()
AC_DEFUN([EGG_FUNC_DLOPEN],
[
  if test "$NEED_DL" = 1 && test "$ac_cv_func_dlopen" = "no"; then
    if test "$LINUX" = "yes"; then
      cat << 'EOF' >&2
configure: warning:

  libdl cannot be found. Since you are on a Linux system, this is a known
  problem. A kludge is known for it,
EOF

      if test -r "/lib/libdl.so.1"; then
        cat << 'EOF' >&2
  and you seem to have it. We'll use that.

EOF
        AC_DEFINE(HAVE_DLOPEN, 1, [Define if we have/need dlopen() (for module support).])
        LIBS="/lib/libdl.so.1 $LIBS"
      else
        cat << 'EOF' >&2
  which you DON'T seem to have... doh! If you do have dlopen on your system,
  and manage to figure out where it's located, add it to your XLIBS= lines
  and #define HAVE_DLOPEN in config.h. We'll proceed on anyway, but you
  probably won't be able to 'make eggdrop'. The default make will now be set
  to static.

  If you do manage to get modules working on this system, please let the
  development team know how (eggdev@eggheads.org).

EOF
        DEFAULT_MAKE="static"
      fi
    else
      cat << 'EOF' >&2
configure: warning:

  dlopen could not be found on this system. If you do have dlopen on your
  system, and manage to figure out where it's located, add it to your XLIBS=
  lines and #define HAVE_DLOPEN in config.h. We'll proceed on anyway, but you
  probably won't be able to 'make eggdrop'. The default make will now be set
  to static.

  If you do manage to get modules working on this system, please let the
  development team know how (eggdev@eggheads.org).

EOF
      DEFAULT_MAKE="static"
    fi
  fi
])


dnl EGG_SUBST_EGGVERSION()
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
dnl need to keep absolute paths in mind (which don't need a "../" pre-pended).
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
m4_define(EGG_REPLACE_IF_CHANGED,
[
  AC_CONFIG_COMMANDS([replace-if-changed],
  [[
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
AC_DEFUN([EGG_TCL_LUSH],
[
  EGG_REPLACE_IF_CHANGED(lush.h,
  [
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
AC_DEFUN([EGG_CATCH_MAKEFILE_REBUILD],
[
  AC_CONFIG_COMMANDS([catch-make-rebuild],
  [[
    if test -f .modules; then
      $srcdir/misc/modconfig --top_srcdir="$srcdir/src" Makefile
    fi
  ]])
])


dnl EGG_SAVE_PARAMETERS()
dnl
dnl Remove --cache-file and --srcdir arguments so they do not pile up.
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
