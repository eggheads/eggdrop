dnl aclocal.m4
dnl   macros autoconf uses when building configure from configure.in
dnl
dnl $Id: aclocal.m4,v 1.60 2002/12/24 03:10:21 wcc Exp $
dnl


dnl  EGG_MSG_CONFIGURE_START()
dnl
AC_DEFUN(EGG_MSG_CONFIGURE_START, [dnl
AC_MSG_RESULT()
AC_MSG_RESULT(This is Eggdrop's GNU configure script.)
AC_MSG_RESULT(It's going to run a bunch of strange tests to hopefully)
AC_MSG_RESULT(make your compile work without much twiddling.)
AC_MSG_RESULT()
])dnl


dnl  EGG_MSG_CONFIGURE_END()
dnl
AC_DEFUN(EGG_MSG_CONFIGURE_END, [dnl
AC_MSG_RESULT()
AC_MSG_RESULT(Configure is done.)
AC_MSG_RESULT()
AC_MSG_RESULT([Type 'make config' to configure the modules, or type 'make iconfig'])
AC_MSG_RESULT(to interactively choose which modules to compile.)
AC_MSG_RESULT()
])dnl


dnl  EGG_CHECK_CC()
dnl
dnl  FIXME: make a better test
dnl
AC_DEFUN(EGG_CHECK_CC, [dnl
if test "${cross_compiling-x}" = "x"
then
  cat << 'EOF' >&2
configure: error:

  This system does not appear to have a working C compiler.
  A working C compiler is required to compile Eggdrop.

EOF
  exit 1
fi
])dnl


dnl  EGG_CHECK_CCPIPE()
dnl
dnl  Checks whether the compiler supports the `-pipe' flag, which
dnl  speeds up the compilation.
AC_DEFUN(EGG_CHECK_CCPIPE, [dnl
if test -z "$no_pipe"
then
  if test -n "$GCC"
  then
    AC_CACHE_CHECK(whether the compiler understands -pipe, egg_cv_var_ccpipe, [dnl
      ac_old_CC="$CC"
      CC="$CC -pipe"
      AC_TRY_COMPILE(,, egg_cv_var_ccpipe="yes", egg_cv_var_ccpipe="no")
      CC="$ac_old_CC"
    ])
    if test "$egg_cv_var_ccpipe" = "yes"
    then
      CC="$CC -pipe"
    fi
  fi
fi
])dnl


dnl  EGG_PROG_STRIP()
dnl
AC_DEFUN(EGG_PROG_STRIP, [dnl
AC_CHECK_PROG(STRIP, strip, strip)
if test "${STRIP-x}" = "x"
then
  STRIP=touch
fi
])dnl


dnl  EGG_PROG_AWK()
dnl
AC_DEFUN(EGG_PROG_AWK, [dnl
# awk is needed for Tcl library and header checks, and eggdrop version subst
AC_PROG_AWK
if test "${AWK-x}" = "x"
then
  cat << 'EOF' >&2
configure: error:

  This system seems to lack a working 'awk' command.
  A working 'awk' command is required to compile Eggdrop.

EOF
  exit 1
fi
])dnl


dnl  EGG_PROG_BASENAME()
dnl
AC_DEFUN(EGG_PROG_BASENAME, [dnl
# basename is needed for Tcl library and header checks
AC_CHECK_PROG(BASENAME, basename, basename)
if test "${BASENAME-x}" = "x"
then
  cat << 'EOF' >&2
configure: error:

  This system seems to lack a working 'basename' command.
  A working 'basename' command is required to compile Eggdrop.

EOF
  exit 1
fi
])dnl


dnl  EGG_DISABLE_CC_OPTIMIZATION()
dnl
dnl check if user requested to remove -O2 cflag 
dnl would be usefull on some weird *nix
AC_DEFUN(EGG_DISABLE_CC_OPTIMIZATION, [dnl
 AC_ARG_ENABLE(cc-optimization,
   [  --disable-cc-optimization   disable -O2 cflag],  
   CFLAGS=`echo $CFLAGS | sed 's/\-O2//'`)
])dnl

dnl  EGG_CHECK_OS()
dnl
dnl  FIXME/NOTICE:
dnl    This function is obsolete. Any NEW code/checks should be written
dnl    as individual tests that will be checked on ALL operating systems.
dnl
AC_DEFUN(EGG_CHECK_OS, [dnl
LINUX=no
IRIX=no
SUNOS=no
HPUX=no
EGG_CYGWIN=no
MOD_CC="$CC"
MOD_LD="$CC"
MOD_STRIP="$STRIP"
SHLIB_CC="$CC"
SHLIB_LD="$CC"
SHLIB_STRIP="$STRIP"
NEED_DL=1
DEFAULT_MAKE=debug
MOD_EXT=so

AC_CACHE_CHECK(system type, egg_cv_var_system_type, egg_cv_var_system_type=`$UNAME -s`)
AC_CACHE_CHECK(system release, egg_cv_var_system_release, egg_cv_var_system_release=`$UNAME -r`)

case "$egg_cv_var_system_type" in
  BSD/OS)
    case "`echo $egg_cv_var_system_release | cut -d . -f 1`" in
      2)
        NEED_DL=0
        DEFAULT_MAKE=static
      ;;
      3)
        MOD_CC=shlicc
        MOD_LD=shlicc
        MOD_STRIP="$STRIP -d"
        SHLIB_LD="shlicc -r"
        SHLIB_STRIP=touch
        AC_DEFINE(MODULES_OK)dnl
      ;;
      *)
        CFLAGS="$CFLAGS -Wall"
        MOD_LD="$CC"
        MOD_STRIP="$STRIP -d"
        SHLIB_CC="$CC -export-dynamic -fPIC"
        SHLIB_LD="$CC -shared -nostartfiles"
        AC_DEFINE(MODULES_OK)dnl
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
        AC_MSG_CHECKING(for /usr/lib/binmode.o)
        if test -r /usr/lib/binmode.o
        then
          AC_MSG_RESULT(yes)
          LIBS="$LIBS /usr/lib/binmode.o"
        else
          AC_MSG_RESULT(no)
          AC_MSG_WARN(Make sure the directory Eggdrop is installed into is mounted in binary mode.)
        fi
        MOD_EXT=dll
        AC_DEFINE(MODULES_OK)dnl
      ;;
      *)
        NEED_DL=0
        DEFAULT_MAKE=static
        AC_MSG_WARN(Make sure the directory Eggdrop is installed into is mounted in binary mode.)
      ;;
    esac
    EGG_CYGWIN=yes
    AC_DEFINE(CYGWIN_HACKS)
  ;;
  HP-UX)
    HPUX=yes
    MOD_LD="$CC -fPIC -shared"
    SHLIB_CC="$CC -fPIC"
    SHLIB_LD="ld -b"
    NEED_DL=0
    AC_DEFINE(MODULES_OK)dnl
    AC_DEFINE(HPUX_HACKS)dnl
    if test "`echo $egg_cv_var_system_release | cut -d . -f 2`" = "10"
    then
      AC_DEFINE(HPUX10_HACKS)dnl
    fi
  ;;
  dell)
    AC_MSG_RESULT(Dell SVR4)
    SHLIB_STRIP=touch
    NEED_DL=0
    MOD_LD="$CC -lelf -lucb"
  ;;
  IRIX)
    SHLIB_LD="ld -n32 -shared -rdata_shared"
    IRIX=yes
    SHLIB_STRIP=touch
    NEED_DL=0
    DEFAULT_MAKE=static
  ;;
  Ultrix)
    NEED_DL=0
    SHLIB_STRIP=touch
    DEFAULT_MAKE=static
    SHELL=/bin/sh5
  ;;
  SINIX*)
    NEED_DL=0
    SHLIB_STRIP=touch
    DEFAULT_MAKE=static
    SHLIB_CC="cc -G"
  ;;
  BeOS)
    NEED_DL=0
    SHLIB_STRIP=strip
    DEFAULT_MAKE=static
  ;;
  Linux)
    LINUX=yes
    CFLAGS="$CFLAGS -Wall"
    MOD_LD="$CC"
    SHLIB_CC="$CC -fPIC"
    SHLIB_LD="$CC -shared -nostartfiles"
    AC_DEFINE(MODULES_OK)dnl
  ;;
  Lynx)
    NEED_DL=0
    DEFAULT_MAKE=static
    SHLIB_STRIP=strip
  ;;
  QNX)
    NEED_DL=0
    DEFAULT_MAKE=static
    SHLIB_LD="ld -shared"
    SHLIB_STRIP=strip
  ;;
  OSF1)
    case "`echo $egg_cv_var_system_release | cut -d . -f 1`" in
      V*)
        # FIXME: we should check this in a separate test
        # Digital OSF uses an ancient version of gawk
        if test "$AWK" = "gawk"
        then
          AWK=awk
        fi
        MOD_CC=cc
        MOD_LD=cc
        SHLIB_CC=cc
        SHLIB_LD="ld -shared -expect_unresolved \"'*'\""
        SHLIB_STRIP=touch
        AC_DEFINE(MODULES_OK)dnl
      ;;
      1.0|1.1|1.2)
        SHLIB_LD="ld -R -export $@:"
        AC_DEFINE(MODULES_OK)dnl
        AC_DEFINE(OSF1_HACKS)dnl
      ;;
      1.*)
        SHLIB_CC="$CC -fpic"
        SHLIB_LD="ld -shared"
        AC_DEFINE(MODULES_OK)dnl
        AC_DEFINE(OSF1_HACKS)dnl
      ;;
      *)
        NEED_DL=0
        DEFAULT_MAKE=static
      ;;
    esac
    AC_DEFINE(STOP_UAC)dnl
  ;;
  SunOS)
    if test "`echo $egg_cv_var_system_release | cut -d . -f 1`" = "5"
    then
      # Solaris
      if test -n "$GCC"
      then
        SHLIB_CC="$CC -fPIC"
        SHLIB_LD="$CC -shared"
      else
        SHLIB_CC="$CC -KPIC"
        SHLIB_LD="$CC -G -z text"
      fi
    else
      # SunOS 4
      SUNOS=yes
      SHLIB_LD=ld
      SHLIB_CC="$CC -PIC"
      AC_DEFINE(DLOPEN_1)dnl
    fi
    AC_DEFINE(MODULES_OK)dnl
  ;;
  *BSD)
    # FreeBSD/OpenBSD/NetBSD
    SHLIB_CC="$CC -fPIC"
    SHLIB_LD="ld -Bshareable -x"
    AC_DEFINE(MODULES_OK)dnl
  ;;
  *)
    AC_MSG_CHECKING(if system is Mach based)
    if test -r /mach
    then
      AC_MSG_RESULT(yes)
      NEED_DL=0
      DEFAULT_MAKE=static
      AC_DEFINE(BORGCUBES)dnl
    else
      AC_MSG_RESULT(no)
      AC_MSG_CHECKING(if system is QNX)
      if test -r /cmds
      then
        AC_MSG_RESULT(yes)
        SHLIB_STRIP=touch
        NEED_DL=0
        DEFAULT_MAKE=static
      else
        AC_MSG_RESULT(no)
        AC_MSG_RESULT(Something unknown!)
        AC_MSG_RESULT([If you get dynamic modules to work, be sure to let the devel team know HOW :)])
        NEED_DL=0
        DEFAULT_MAKE=static
      fi
    fi
  ;;
esac
AC_SUBST(MOD_LD)dnl
AC_SUBST(MOD_CC)dnl
AC_SUBST(MOD_STRIP)dnl
AC_SUBST(SHLIB_LD)dnl
AC_SUBST(SHLIB_CC)dnl
AC_SUBST(SHLIB_STRIP)dnl
AC_SUBST(DEFAULT_MAKE)dnl
AC_SUBST(MOD_EXT)dnl
AC_DEFINE_UNQUOTED(EGG_MOD_EXT, "$MOD_EXT")dnl
])dnl


dnl  EGG_CHECK_LIBS()
dnl
AC_DEFUN(EGG_CHECK_LIBS, [dnl
# FIXME: this needs to be fixed so that it works on IRIX
if test "$IRIX" = "yes"
then
  AC_MSG_WARN(Skipping library tests because they CONFUSE Irix.)
else
  AC_CHECK_LIB(socket, socket)
  AC_CHECK_LIB(nsl, connect)
  AC_CHECK_LIB(dns, gethostbyname)
  AC_CHECK_LIB(dl, dlopen)
  AC_CHECK_LIB(m, tan, EGG_MATH_LIB="-lm")
  # This is needed for Tcl libraries compiled with thread support
  AC_CHECK_LIB(pthread, pthread_mutex_init, [dnl
  ac_cv_lib_pthread_pthread_mutex_init=yes
  ac_cv_lib_pthread="-lpthread"], [dnl
    AC_CHECK_LIB(pthread, __pthread_mutex_init, [dnl
    ac_cv_lib_pthread_pthread_mutex_init=yes
    ac_cv_lib_pthread="-lpthread"], [dnl
      AC_CHECK_LIB(pthreads, pthread_mutex_init, [dnl
      ac_cv_lib_pthread_pthread_mutex_init=yes
      ac_cv_lib_pthread="-lpthreads"], [dnl
        AC_CHECK_FUNC(pthread_mutex_init, [dnl
        ac_cv_lib_pthread_pthread_mutex_init=yes
        ac_cv_lib_pthread=""],
        ac_cv_lib_pthread_pthread_mutex_init=no)])])])
  if test "$SUNOS" = "yes"
  then
    # For suns without yp or something like that
    AC_CHECK_LIB(dl, main)
  else
    if test "$HPUX" = "yes"
    then
      AC_CHECK_LIB(dld, shl_load)
    fi
  fi
fi
])dnl


dnl  EGG_CHECK_FUNC_VSPRINTF()
dnl
AC_DEFUN(EGG_CHECK_FUNC_VSPRINTF, [dnl
AC_CHECK_FUNCS(vsprintf)
if test "$ac_cv_func_vsprintf" = "no"
then
  cat << 'EOF' >&2
configure: error:

  Your system does not have the sprintf/vsprintf libraries.
  These are required to compile almost anything.  Sorry.

EOF
  exit 1
fi
])dnl


dnl  EGG_HEADER_STDC()
dnl
AC_DEFUN(EGG_HEADER_STDC, [dnl
if test "$ac_cv_header_stdc" = "no"
then
  cat << 'EOF' >&2
configure: error:

  Your system must support ANSI C Header files.
  These are required for the language support.  Sorry.

EOF
  exit 1
fi
])dnl


dnl  EGG_CHECK_LIBSAFE_SSCANF()
dnl
AC_DEFUN(EGG_CHECK_LIBSAFE_SSCANF, [dnl
AC_CACHE_CHECK(for broken libsafe sscanf, egg_cv_var_libsafe_sscanf, [dnl
  AC_TRY_RUN([
#include <stdio.h>

int main()
{
  char *src = "0x001,guppyism\n";
  char dst[10];
  int idx;
  if (sscanf(src, "0x%x,%10c", &idx, dst) == 1)
    exit(1);
  return 0;
}
], egg_cv_var_libsafe_sscanf="no", egg_cv_var_libsafe_sscanf="yes",
egg_cv_var_libsafe_sscanf="no")
])
if test "$egg_cv_var_libsafe_sscanf" = "yes"
then
  AC_DEFINE(LIBSAFE_HACKS)dnl
fi
])dnl


dnl  EGG_EXEEXT()
dnl
dnl  Test for executable suffix and define Eggdrop's executable name
dnl  accordingly.
AC_DEFUN(EGG_EXEEXT, [dnl
EGGEXEC=eggdrop
AC_EXEEXT
if test ! "${EXEEXT-x}" = "x"
then
  EGGEXEC="eggdrop$EXEEXT"
fi
AC_SUBST(EGGEXEC)dnl
])dnl


dnl  EGG_TCL_ARG_WITH()
dnl
AC_DEFUN(EGG_TCL_ARG_WITH, [dnl
# oohh new configure --variables for those with multiple Tcl libs
AC_ARG_WITH(tcllib, [  --with-tcllib=PATH      full path to Tcl library], tcllibname="$withval")
AC_ARG_WITH(tclinc, [  --with-tclinc=PATH      full path to Tcl header], tclincname="$withval")

WARN=0
# Make sure either both or neither $tcllibname and $tclincname are set
if test ! "${tcllibname-x}" = "x"
then
  if test "${tclincname-x}" = "x"
  then
    WARN=1
    tcllibname=""
    TCLLIB=""
    TCLINC=""
  fi
else
  if test ! "${tclincname-x}" = "x"
  then
    WARN=1
    tclincname=""
    TCLLIB=""
    TCLINC=""
  fi
fi
if test "$WARN" = 1
then
  cat << 'EOF' >&2
configure: warning:

  You must specify both --with-tcllib and --with-tclinc for them to work.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
fi
])dnl


dnl  EGG_TCL_ENV()
dnl
AC_DEFUN(EGG_TCL_ENV, [dnl
WARN=0
# Make sure either both or neither $TCLLIB and $TCLINC are set
if test ! "${TCLLIB-x}" = "x"
then
  if test "${TCLINC-x}" = "x"
  then
    WARN=1
    WVAR1=TCLLIB
    WVAR2=TCLINC
    TCLLIB=""
  fi
else
  if test ! "${TCLINC-x}" = "x"
  then
    WARN=1
    WVAR1=TCLINC
    WVAR2=TCLLIB
    TCLINC=""
  fi
fi
if test "$WARN" = 1
then
  cat << EOF >&2
configure: warning:

  Environment variable $WVAR1 was set, but I did not detect ${WVAR2}.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
fi
])dnl


dnl  EGG_TCL_WITH_TCLLIB()
dnl
AC_DEFUN(EGG_TCL_WITH_TCLLIB, [dnl
# Look for Tcl library: if $tcllibname is set, check there first
if test ! "${tcllibname-x}" = "x"
then
  if test -f "$tcllibname" && test -r "$tcllibname"
  then
    TCLLIB=`echo $tcllibname | sed 's%/[[^/]][[^/]]*$%%'`
    TCLLIBFN=`$BASENAME $tcllibname | cut -c4-`
    TCLLIBEXT=".`echo $TCLLIBFN | $AWK '{j=split([$]1, i, "."); print i[[j]]}'`"
    TCLLIBFNS=`$BASENAME $tcllibname $TCLLIBEXT | cut -c4-`
  else
    cat << EOF >&2
configure: warning:

  The file '$tcllibname' given to option --with-tcllib is not valid.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
    tcllibname=""
    tclincname=""
    TCLLIB=""
    TCLLIBFN=""
    TCLINC=""
    TCLINCFN=""
  fi
fi
])dnl


dnl  EGG_TCL_WITH_TCLINC()
dnl
AC_DEFUN(EGG_TCL_WITH_TCLINC, [dnl
# Look for Tcl header: if $tclincname is set, check there first
if test ! "${tclincname-x}" = "x"
then
  if test -f "$tclincname" && test -r "$tclincname"
  then
    TCLINC=`echo $tclincname | sed 's%/[[^/]][[^/]]*$%%'`
    TCLINCFN=`$BASENAME $tclincname`
  else
    cat << EOF >&2
configure: warning:

  The file '$tclincname' given to option --with-tclinc is not valid.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
    tcllibname=""
    tclincname=""
    TCLLIB=""
    TCLLIBFN=""
    TCLINC=""
    TCLINCFN=""
  fi
fi
])dnl


dnl  EGG_TCL_FIND_LIBRARY()
dnl
AC_DEFUN(EGG_TCL_FIND_LIBRARY, [dnl
# Look for Tcl library: if $TCLLIB is set, check there first
if test "${TCLLIBFN-x}" = "x"
then
  if test ! "${TCLLIB-x}" = "x"
  then
    if test -d "$TCLLIB"
    then
      for tcllibfns in $tcllibnames
      do
        for tcllibext in $tcllibextensions
        do
          if test -r "$TCLLIB/lib$tcllibfns$tcllibext"
          then
            TCLLIBFN="$tcllibfns$tcllibext"
            TCLLIBEXT="$tcllibext"
            TCLLIBFNS="$tcllibfns"
            break 2
          fi
        done
      done
    fi
    if test "${TCLLIBFN-x}" = "x"
    then
      cat << 'EOF' >&2
configure: warning:

  Environment variable TCLLIB was set, but incorrect.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
      TCLLIB=""
      TCLLIBFN=""
      TCLINC=""
      TCLINCFN=""
    fi
  fi
fi
])dnl


dnl  EGG_TCL_FIND_HEADER()
dnl
AC_DEFUN(EGG_TCL_FIND_HEADER, [dnl
# Look for Tcl header: if $TCLINC is set, check there first
if test "${TCLINCFN-x}" = "x"
then
  if test ! "${TCLINC-x}" = "x"
  then
    if test -d "$TCLINC"
    then
      for tclheaderfn in $tclheadernames
      do
        if test -r "$TCLINC/$tclheaderfn"
        then
          TCLINCFN="$tclheaderfn"
          break
        fi
      done
    fi
    if test "${TCLINCFN-x}" = "x"
    then
      cat << 'EOF' >&2
configure: warning:

  Environment variable TCLINC was set, but incorrect.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
      TCLLIB=""
      TCLLIBFN=""
      TCLINC=""
      TCLINCFN=""
    fi
  fi
fi
])dnl


dnl  EGG_TCL_CHECK_LIBRARY()
dnl
AC_DEFUN(EGG_TCL_CHECK_LIBRARY, [dnl
AC_MSG_CHECKING(for Tcl library)

# Attempt autodetect for $TCLLIBFN if it's not set
if test ! "${TCLLIBFN-x}" = "x"
then
  AC_MSG_RESULT(using $TCLLIB/lib$TCLLIBFN)
else
  for tcllibfns in $tcllibnames
  do
    for tcllibext in $tcllibextensions
    do
      for tcllibpath in $tcllibpaths
      do
        if test -r "$tcllibpath/lib$tcllibfns$tcllibext"
        then
          AC_MSG_RESULT(found $tcllibpath/lib$tcllibfns$tcllibext)
          TCLLIB="$tcllibpath"
          TCLLIBFN="$tcllibfns$tcllibext"
          TCLLIBEXT="$tcllibext"
          TCLLIBFNS="$tcllibfns"
          break 3
        fi
      done
    done
  done
fi

# Show if $TCLLIBFN wasn't found
if test "${TCLLIBFN-x}" = "x"
then
  AC_MSG_RESULT(not found)
fi
AC_SUBST(TCLLIB)dnl
AC_SUBST(TCLLIBFN)dnl
])dnl


dnl  EGG_TCL_CHECK_HEADER()
dnl
AC_DEFUN(EGG_TCL_CHECK_HEADER, [dnl
AC_MSG_CHECKING(for Tcl header)

# Attempt autodetect for $TCLINCFN if it's not set
if test ! "${TCLINCFN-x}" = "x"
then
  AC_MSG_RESULT(using $TCLINC/$TCLINCFN)
else
  for tclheaderpath in $tclheaderpaths
  do
    for tclheaderfn in $tclheadernames
    do
      if test -r "$tclheaderpath/$tclheaderfn"
      then
        AC_MSG_RESULT(found $tclheaderpath/$tclheaderfn)
        TCLINC="$tclheaderpath"
        TCLINCFN="$tclheaderfn"
        break 2
      fi
    done
  done
  # FreeBSD hack ...
  if test "${TCLINCFN-x}" = "x"
  then
    for tcllibfns in $tcllibnames
    do
      for tclheaderpath in $tclheaderpaths
      do
        for tclheaderfn in $tclheadernames
        do
          if test -r "$tclheaderpath/$tcllibfns/$tclheaderfn"
          then
            AC_MSG_RESULT(found $tclheaderpath/$tcllibfns/$tclheaderfn)
            TCLINC="$tclheaderpath/$tcllibfns"
            TCLINCFN="$tclheaderfn"
            break 3
          fi
        done
      done
    done
  fi
fi

# Show if $TCLINCFN wasn't found
if test "${TCLINCFN-x}" = "x"
then
  AC_MSG_RESULT(not found)
fi
AC_SUBST(TCLINC)dnl
AC_SUBST(TCLINCFN)dnl
])dnl


dnl  EGG_CACHE_UNSET(CACHE-ID)
dnl
dnl  Unsets a certain cache item. Typically called before using
dnl  the AC_CACHE_*() macros.
AC_DEFUN(EGG_CACHE_UNSET, [dnl
  unset $1
])


dnl  EGG_TCL_DETECT_CHANGE()
dnl
dnl  Detect whether the Tcl system has changed since our last
dnl  configure run. Set egg_tcl_changed accordingly.
dnl
dnl  Tcl related feature and version checks should re-run their
dnl  checks as soon as egg_tcl_changed is set to "yes".
AC_DEFUN(EGG_TCL_DETECT_CHANGE, [dnl
  AC_MSG_CHECKING(whether the Tcl system has changed)
  egg_tcl_changed=yes
  egg_tcl_id="$TCLLIB:$TCLLIBFN:$TCLINC:$TCLINCFN"
  if test ! "$egg_tcl_id" = ":::"
  then
    egg_tcl_cached=yes
    AC_CACHE_VAL(egg_cv_var_tcl_id, [dnl
      egg_cv_var_tcl_id="$egg_tcl_id"
      egg_tcl_cached=no
    ])
    if test "$egg_tcl_cached" = "yes"
    then
      if test "${egg_cv_var_tcl_id-x}" = "${egg_tcl_id-x}"
      then
        egg_tcl_changed=no
      else
        egg_cv_var_tcl_id="$egg_tcl_id"
      fi
    fi
  fi
  if test "$egg_tcl_changed" = "yes"
  then
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
])


dnl  EGG_TCL_CHECK_VERSION()
dnl
AC_DEFUN(EGG_TCL_CHECK_VERSION, [dnl
# Both TCLLIBFN & TCLINCFN must be set, or we bail
TCL_FOUND=0
if test ! "${TCLLIBFN-x}" = "x" && test ! "${TCLINCFN-x}" = "x"
then
  TCL_FOUND=1

  # Check Tcl's version
  if test "$egg_tcl_changed" = "yes"
  then
    EGG_CACHE_UNSET(egg_cv_var_tcl_version)
  fi
  AC_MSG_CHECKING(for Tcl version)
  AC_CACHE_VAL(egg_cv_var_tcl_version, [dnl
    egg_cv_var_tcl_version=`grep TCL_VERSION $TCLINC/$TCLINCFN | head -1 | $AWK '{gsub(/\"/, "", [$]3); print [$]3}'`
  ])

  if test ! "${egg_cv_var_tcl_version-x}" = "x"
  then
    AC_MSG_RESULT($egg_cv_var_tcl_version)
  else
    AC_MSG_RESULT(not found)
    TCL_FOUND=0
  fi

  # Check Tcl's patch level (if available)
  if test "$egg_tcl_changed" = "yes"
  then
    EGG_CACHE_UNSET(egg_cv_var_tcl_patch_level)
  fi
  AC_MSG_CHECKING(for Tcl patch level)
  AC_CACHE_VAL(egg_cv_var_tcl_patch_level, [dnl
    eval "egg_cv_var_tcl_patch_level=`grep TCL_PATCH_LEVEL $TCLINC/$TCLINCFN | head -1 | $AWK '{gsub(/\"/, "", [$]3); print [$]3}'`"
  ])

  if test ! "${egg_cv_var_tcl_patch_level-x}" = "x"
  then
    AC_MSG_RESULT($egg_cv_var_tcl_patch_level)
  else
    egg_cv_var_tcl_patch_level="unknown"
    AC_MSG_RESULT(unknown)
  fi
fi

# Check if we found Tcl's version
if test "$TCL_FOUND" = 0
then
  cat << 'EOF' >&2
configure: error:

  I can't find Tcl on this system.

  Eggdrop requires Tcl to compile.  If you already have Tcl installed
  on this system, and I just wasn't looking in the right place for it,
  set the environment variables TCLLIB and TCLINC so I will know where
  to find 'libtcl.a' (or 'libtcl.so') and 'tcl.h' (respectively). Then
  run 'configure' again.

  Read the README file if you don't know what Tcl is or how to get it
  and install it.

EOF
  exit 1
fi
])dnl


dnl  EGG_TCL_CHECK_PRE70()
dnl
AC_DEFUN(EGG_TCL_CHECK_PRE70, [dnl
# Is this version of Tcl too old for us to use ?
TCL_VER_PRE70=`echo $egg_cv_var_tcl_version | $AWK '{split([$]1, i, "."); if (i[[1]] < 7) print "yes"; else print "no"}'`
if test "$TCL_VER_PRE70" = "yes"
then
  cat << EOF >&2
configure: error:

  Your Tcl version is much too old for Eggdrop to use.
  I suggest you download and compile a more recent version.
  The most reliable current version is $tclrecommendver and
  can be downloaded from $tclrecommendsite

EOF
  exit 1
fi
])dnl


dnl  EGG_TCL_CHECK_PRE75()
dnl
AC_DEFUN(EGG_TCL_CHECK_PRE75, [dnl
# Are we using a pre 7.5 Tcl version ?
TCL_VER_PRE75=`echo $egg_cv_var_tcl_version | $AWK '{split([$]1, i, "."); if (((i[[1]] == 7) && (i[[2]] < 5)) || (i[[1]] < 7)) print "yes"; else print "no"}'`
if test "$TCL_VER_PRE75" = "yes"
then
  AC_DEFINE(HAVE_PRE7_5_TCL)dnl
fi
])dnl


dnl  EGG_TCL_TESTLIBS()
dnl
AC_DEFUN(EGG_TCL_TESTLIBS, [dnl
# Set variables for Tcl library tests
TCL_TEST_LIB="$TCLLIBFNS"
TCL_TEST_OTHERLIBS="-L$TCLLIB $EGG_MATH_LIB"

if test ! "${ac_cv_lib_pthread-x}" = "x"
then
  TCL_TEST_OTHERLIBS="$TCL_TEST_OTHERLIBS $ac_cv_lib_pthread"
fi
])dnl


dnl  EGG_TCL_CHECK_FREE()
dnl
AC_DEFUN(EGG_TCL_CHECK_FREE, [dnl
if test "$egg_tcl_changed" = "yes"
then
  EGG_CACHE_UNSET(egg_cv_var_tcl_free)
fi

# Check for Tcl_Free()
AC_CHECK_LIB($TCL_TEST_LIB, Tcl_Free, egg_cv_var_tcl_free="yes", egg_cv_var_tcl_free="no", $TCL_TEST_OTHERLIBS)

if test "$egg_cv_var_tcl_free" = "yes"
then
  AC_DEFINE(HAVE_TCL_FREE)dnl
fi
])dnl


dnl  EGG_TCL_ENABLE_THREADS()
dnl
AC_DEFUN(EGG_TCL_ENABLE_THREADS, [dnl
AC_ARG_ENABLE(tcl-threads,
[  --disable-tcl-threads   Disable threaded Tcl support if detected. (Ignore this
                          option unless you know what you are doing)],
enable_tcl_threads="$enableval",
enable_tcl_threads=yes)
])dnl


dnl  EGG_TCL_CHECK_THREADS()
dnl
AC_DEFUN(EGG_TCL_CHECK_THREADS, [dnl
if test "$egg_tcl_changed" = "yes"
then
  EGG_CACHE_UNSET(egg_cv_var_tcl_threaded)
fi

# Check for TclpFinalizeThreadData()
AC_CHECK_LIB($TCL_TEST_LIB, TclpFinalizeThreadData, egg_cv_var_tcl_threaded="yes", egg_cv_var_tcl_threaded="no", $TCL_TEST_OTHERLIBS)

if test "$egg_cv_var_tcl_threaded" = "yes"
then
  if test "$enable_tcl_threads" = "no"
  then

    cat << 'EOF' >&2
configure: warning:

  You have disabled threads support on a system with a threaded Tcl library.
  Tcl features that rely on scheduled events may not function properly.

EOF
  else
    AC_DEFINE(HAVE_TCL_THREADS)dnl
  fi

  # Add pthread library to $LIBS if we need it
  if test ! "${ac_cv_lib_pthread-x}" = "x"
  then
    LIBS="$ac_cv_lib_pthread $LIBS"
  fi
fi
])dnl


dnl  EGG_TCL_LIB_REQS()
dnl
AC_DEFUN(EGG_TCL_LIB_REQS, [dnl
if test "$EGG_CYGWIN" = "yes"
then
  TCL_REQS="$TCLLIB/lib$TCLLIBFN"
  TCL_LIBS="-L$TCLLIB -l$TCLLIBFNS $EGG_MATH_LIB"
else
if test ! "$TCLLIBEXT" = ".a"
then
  TCL_REQS="$TCLLIB/lib$TCLLIBFN"
  TCL_LIBS="-L$TCLLIB -l$TCLLIBFNS $EGG_MATH_LIB"
else

  # Set default make as static for unshared Tcl library
  if test ! "$DEFAULT_MAKE" = "static"
  then
    cat << 'EOF' >&2
configure: warning:

  Your Tcl library is not a shared lib.
  configure will now set default make type to static...

EOF
    DEFAULT_MAKE=static
    AC_SUBST(DEFAULT_MAKE)dnl
  fi

  # Are we using a pre 7.4 Tcl version ?
  TCL_VER_PRE74=`echo $egg_cv_var_tcl_version | $AWK '{split([$]1, i, "."); if (((i[[1]] == 7) && (i[[2]] < 4)) || (i[[1]] < 7)) print "yes"; else print "no"}'`
  if test "$TCL_VER_PRE74" = "no"
  then

    # Was the --with-tcllib option given ?
    if test ! "${tcllibname-x}" = "x"
    then
      TCL_REQS="$TCLLIB/lib$TCLLIBFN"
      TCL_LIBS="$TCLLIB/lib$TCLLIBFN $EGG_MATH_LIB"
    else
      TCL_REQS="$TCLLIB/lib$TCLLIBFN"
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
AC_SUBST(TCL_REQS)dnl
AC_SUBST(TCL_LIBS)dnl
])dnl


dnl  EGG_FUNC_DLOPEN()
dnl
AC_DEFUN(EGG_FUNC_DLOPEN, [dnl
if test "$NEED_DL" = 1 && test "$ac_cv_func_dlopen" = "no"
then
  if test "$LINUX" = "yes"
  then
    cat << 'EOF' >&2
configure: warning:

  Since you are on a Linux system, this has a known problem...
  I know a kludge for it,
EOF

    if test -r "/lib/libdl.so.1"
    then
      cat << 'EOF' >&2
  and you seem to have it, so we'll do that...

EOF
      AC_DEFINE(HAVE_DLOPEN)dnl
      LIBS="/lib/libdl.so.1 $LIBS"
    else
      cat << 'EOF' >&2
  which you DON'T seem to have... doh!
  perhaps you may still have the stuff lying around somewhere
  if you work out where it is, add it to your XLIBS= lines
  and #define HAVE_DLOPEN in config.h

  we'll proceed on anyway, but you probably won't be able
  to 'make eggdrop' but you might be able to make the
  static bot (I'll default your make to this version).

EOF
      DEFAULT_MAKE=static
    fi
  else
    cat << 'EOF' >&2
configure: warning:

  You don't seem to have libdl anywhere I can find it, this will
  prevent you from doing dynamic modules, I'll set your default
  make to static linking.

EOF
    DEFAULT_MAKE=static
  fi
fi
])dnl


dnl  EGG_SUBST_EGGVERSION()
dnl
AC_DEFUN(EGG_SUBST_EGGVERSION, [dnl
EGGVERSION=`grep 'char.egg_version' $srcdir/src/main.c | $AWK '{gsub(/(\"|\;)/, "", [$]4); print [$]4}'`
egg_version_num=`echo $EGGVERSION | $AWK 'BEGIN {FS = "."} {printf("%d%02d%02d", [$]1, [$]2, [$]3)}'`
AC_SUBST(EGGVERSION)dnl
AC_DEFINE_UNQUOTED(EGG_VERSION, $egg_version_num)dnl
])dnl


dnl  EGG_SUBST_DEST()
dnl
AC_DEFUN(EGG_SUBST_DEST, [dnl
if test "${DEST-x}" = "x"
then
  DEST=\${prefix}
fi
AC_SUBST(DEST)dnl
])dnl


dnl  EGG_SUBST_MOD_UPDIR()
dnl
dnl  Since module's Makefiles aren't generated by configure, some
dnl  paths in src/mod/Makefile.in take care of them. For correct
dnl  path "calculation", we need to keep absolute paths in mind
dnl  (which don't need a "../" pre-pended).
AC_DEFUN(EGG_SUBST_MOD_UPDIR, [dnl
case "$srcdir" in
  [[\\/]]* | ?:[[\\/]]*)
    MOD_UPDIR=""
  ;;
  *)
    MOD_UPDIR="../"
  ;;
esac
AC_SUBST(MOD_UPDIR)dnl
])dnl


dnl  EGG_REPLACE_IF_CHANGED(FILE-NAME, CONTENTS-CMDS, INIT-CMDS)
dnl
dnl  Replace FILE-NAME if the newly created contents differs from the existing
dnl  file contents.  Otherwise, leave the file alone.  This avoids needless
dnl  recompiles.
dnl
define(EGG_REPLACE_IF_CHANGED, [dnl
  AC_OUTPUT_COMMANDS([
egg_replace_file="$1"
echo "creating $1"
$2
if test -f "$egg_replace_file" && cmp -s conftest.out $egg_replace_file
then
  echo "$1 is unchanged"
else
  mv conftest.out $egg_replace_file
fi
rm -f conftest.out], [$3])dnl
])dnl


dnl  EGG_TCL_LUSH()
dnl
AC_DEFUN(EGG_TCL_LUSH, [dnl
    EGG_REPLACE_IF_CHANGED(lush.h, [
cat > conftest.out << EGGEOF
/* Ignore me but do not erase me.  I am a kludge. */

#include "$egg_tclinc/$egg_tclincfn"
EGGEOF], [
    egg_tclinc="$TCLINC"
    egg_tclincfn="$TCLINCFN"])dnl
])dnl


dnl  EGG_CATCH_MAKEFILE_REBUILD()
dnl
AC_DEFUN(EGG_CATCH_MAKEFILE_REBUILD, [dnl
  AC_OUTPUT_COMMANDS([
if test -f .modules
then
  $srcdir/misc/modconfig --top_srcdir="$srcdir/src" Makefile
fi])
])dnl


dnl  EGG_SAVE_PARAMETERS()
dnl
AC_DEFUN(EGG_SAVE_PARAMETERS, [dnl
  # Remove --cache-file and --srcdir arguments so they do not pile up.
  egg_ac_parameters=
  ac_prev=
  for ac_arg in $ac_configure_args; do
    if test -n "$ac_prev"; then
      ac_prev=
      continue
    fi
    case $ac_arg in
    -cache-file | --cache-file | --cache-fil | --cache-fi \
    | --cache-f | --cache- | --cache | --cach | --cac | --ca | --c)
      ac_prev=cache_file ;;
    -cache-file=* | --cache-file=* | --cache-fil=* | --cache-fi=* \
    | --cache-f=* | --cache-=* | --cache=* | --cach=* | --cac=* | --ca=* \
    | --c=*)
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

  AC_SUBST(egg_ac_parameters)dnl
])dnl


AC_DEFUN([AC_PROG_CC_WIN32], [
AC_MSG_CHECKING([how to access the Win32 API])
WIN32FLAGS=
AC_TRY_COMPILE(,[
#ifndef WIN32
# ifndef _WIN32
#  error WIN32 or _WIN32 not defined
# endif
#endif], [
dnl found windows.h with the current config.
AC_MSG_RESULT([present by default])
], [
dnl try -mwin32
ac_compile_save="$ac_compile"
dnl we change CC so config.log looks correct
save_CC="$CC"
ac_compile="$ac_compile -mwin32"
CC="$CC -mwin32"
AC_TRY_COMPILE(,[
#ifndef WIN32
# ifndef _WIN32
#  error WIN32 or _WIN32 not defined
# endif
#endif], [
dnl found windows.h using -mwin32
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
dnl
