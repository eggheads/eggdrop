

AC_DEFUN(EGG_MSG_CONFIGURE_START, [dnl
AC_MSG_RESULT()
AC_MSG_RESULT(This is eggdrop's GNU configure script.)
AC_MSG_RESULT(It's going to run a bunch of strange tests to hopefully)
AC_MSG_RESULT(make your compile work without much twiddling.)
AC_MSG_RESULT()
])dnl


AC_DEFUN(EGG_MSG_CONFIGURE_END, [dnl
AC_MSG_RESULT()
AC_MSG_RESULT(Configure is done.)
AC_MSG_RESULT()
if test -f "./$EGGEXEC"
then
  AC_MSG_RESULT(Type 'make clean' and then 'make' to create the bot.)
else
  AC_MSG_RESULT(Type 'make' to create the bot.)
fi
AC_MSG_RESULT()
])dnl

# FIXME: make a better test
AC_DEFUN(EGG_CHECK_CC, [dnl
if test "x${cross_compiling}" = "x"
then
  cat << 'EOF' >&2
configure: error:

  This system does not appear to have a working C compiler.
  A working C compiler is required to compile eggdrop.

EOF
  exit 1
fi
])dnl


AC_DEFUN(EGG_PROG_STRIP, [dnl
AC_CHECK_PROG(STRIP,strip,strip)
if test "x${STRIP}" = "x"
then
  STRIP=touch
fi
])dnl


AC_DEFUN(EGG_PROG_AWK, [dnl
# awk is needed for Tcl library and header checks, and eggdrop version subst
AC_PROG_AWK
if test "x${AWK}" = "x"
then
  cat << 'EOF' >&2
configure: error:

  This system seems to lack a working 'awk' command.
  A working 'awk' command is required to compile eggdrop.

EOF
  exit 1
fi
])dnl


AC_DEFUN(EGG_PROG_BASENAME, [dnl
# basename is needed for Tcl library and header checks
AC_CHECK_PROG(BASENAME, basename, basename)
if test "x${BASENAME}" = "x"
then
  cat << 'EOF' >&2
configure: error:

  This system seems to lack a working 'basename' command.
  A working 'basename' command is required to compile eggdrop.

EOF
  exit 1
fi
])dnl


AC_DEFUN(EGG_CHECK_OS, [dnl
LINUX=no
IRIX=no
SUNOS=no
HPUX=no
MOD_CC="${CC}"
MOD_LD="${CC}"
MOD_STRIP="${STRIP}"
SHLIB_CC="${CC}"
SHLIB_LD="${CC}"
SHLIB_STRIP="${STRIP}"
NEED_DL=1
DEFAULT_MAKE=eggdrop

AC_MSG_CHECKING(your OS)
if eval "test \"`echo '$''{'egg_cv_var_system'+set}'`\" = set"
then
  echo $ac_n "(cached) $ac_c" 1>&6
else
  eval "egg_cv_var_system=`${UNAME}`"
fi

case "$egg_cv_var_system" in
  BSD/OS)
    if test "x`${UNAME} -r | cut -d . -f 1`" = "x2"
    then
      AC_MSG_RESULT(BSD/OS 2! statically linked modules are the only choice)
      NEED_DL=0
      DEFAULT_MAKE=static
    else
      AC_MSG_RESULT(BSD/OS 3+! ok I spose)
      MOD_CC=shlicc
      MOD_LD=shlicc
      MOD_STRIP="${STRIP} -d"
      SHLIB_LD="shlicc -r"
      SHLIB_STRIP=touch
      AC_DEFINE(MODULES_OK)dnl
    fi
    ;;
  CYGWIN*)
    AC_MSG_RESULT(Cygwin)
    NEED_DL=0
    DEFAULT_MAKE=static
    ;;
  HP-UX)
    AC_MSG_RESULT([HP-UX, just shoot yourself now])
    HPUX=yes
    MOD_LD="gcc -Wl,-E"
    SHLIB_CC="gcc -fPIC"
    SHLIB_LD="ld -b"
    NEED_DL=0
    AC_DEFINE(MODULES_OK)dnl
    AC_DEFINE(HPUX_HACKS)dnl
    if test "x`${UNAME} -r | cut -d . -f 2`" = "x10"
    then
      AC_DEFINE(HPUX10_HACKS)dnl
    fi
    ;;
  IRIX)
    AC_MSG_RESULT(you are cursed with IRIX)
    IRIX=yes
    SHLIB_STRIP=touch
    NEED_DL=0
    DEFAULT_MAKE=static
    ;;
  IRIX64)
    AC_MSG_RESULT(IRIX64)
    IRIX=yes
    SHLIB_STRIP=strip
    NEED_DL=0
    DEFAULT_MAKE=static
    ;;
  Linux)
    AC_MSG_RESULT(Linux! The choice of the GNU generation)
    LINUX=yes
    CFLAGS="$CFLAGS -Wall"
    MOD_LD="${CC}"
    SHLIB_LD="${CC} -shared -nostartfiles"
    AC_DEFINE(MODULES_OK)dnl
    ;;
  Lynx)
    SHLIB_STRIP=touch
    AC_MSG_RESULT(Lynx OS)
    ;;
  OSF1)
    AC_MSG_RESULT(OSF...)
    case `${UNAME} -r | cut -d . -f 1` in
      V*)
        AC_MSG_RESULT([   Digital OSF])
        if test "x$AWK" = "xgawk"
        then
          AWK=awk
        fi
        MOD_CC=cc
        MOD_LD=cc
        SHLIB_CC=cc
        SHLIB_LD="ld -shared -expect_unresolved '*'"
        SHLIB_STRIP=touch
        AC_DEFINE(MODULES_OK)dnl
        ;;
      1.0|1.1|1.2)
        AC_MSG_RESULT([   pre 1.3])
        SHLIB_LD="ld -R -export $@:"
        AC_DEFINE(MODULES_OK)dnl
        AC_DEFINE(OSF1_HACKS)dnl
        ;;
      1.*)
        AC_MSG_RESULT([   1.3+])
        SHLIB_CC="${CC} -fpic"
        SHLIB_LD="ld -shared"
        AC_DEFINE(MODULES_OK)dnl
        AC_DEFINE(OSF1_HACKS)dnl
        ;;
      *)
        AC_MSG_RESULT([   Some other weird OSF! No modules for you...])
        NEED_DL=0
        DEFAULT_MAKE=static
        ;;
    esac
    AC_DEFINE(STOP_UAC)dnl
    ;;
  SunOS)
    if test "x`${UNAME} -r | cut -d . -f 1`" = "x5"
    then
      AC_MSG_RESULT(Solaris -- yay)
      SHLIB_LD="/usr/ccs/bin/ld -G -z text"
    else
      AC_MSG_RESULT(SunOS -- sigh)
      SUNOS=yes
      SHLIB_LD=ld
      SHLIB_STRIP=touch
      AC_DEFINE(DLOPEN_1)dnl
    fi
    MOD_CC="${CC} -fPIC"
    SHLIB_CC="${CC} -fPIC"
    AC_DEFINE(MODULES_OK)dnl
    ;;
  *BSD)
    AC_MSG_RESULT(FreeBSD/NetBSD/OpenBSD - choose your poison)
    SHLIB_CC="${CC} -fpic"
    SHLIB_LD="ld -Bshareable -x"
    AC_DEFINE(MODULES_OK)dnl
    ;;
  *)
    if test -r "/mach"
    then
      AC_MSG_RESULT([NeXT...We are borg, you will be assimilated])
      AC_MSG_RESULT([break out the static modules, it's all you'll ever get :P])
      AC_MSG_RESULT(Hiya DK :P)
      NEED_DL=0
      DEFAULT_MAKE=static
      AC_DEFINE(BORGCUBES)dnl
    else
      if test -r "/cmds"
      then
        AC_MSG_RESULT(QNX)
        SHLIB_STRIP=touch
        NEED_DL=0
        DEFAULT_MAKE=static
      else
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
])dnl


AC_DEFUN(EGG_CHECK_LIBS, [dnl
if test "$IRIX" = "yes"
then
  AC_MSG_WARN(Skipping library tests because they CONFUSE Irix.)
else
  AC_CHECK_LIB(socket, socket)
  AC_CHECK_LIB(nsl, connect)
  AC_CHECK_LIB(dns, gethostbyname)
  AC_CHECK_LIB(dl, dlopen)
  # This is needed for Tcl libraries compiled with thread support
  AC_CHECK_LIB(pthread,pthread_mutex_init,
ac_cv_lib_pthread_pthread_mutex_init=yes,
ac_cv_lib_pthread_pthread_mutex_init=no)
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


AC_DEFUN(EGG_CHECK_FUNC_VSPRINTF, [dnl
AC_CHECK_FUNCS(vsprintf)
if test "x${ac_cv_func_vsprintf}" = "xno"
then
  cat << 'EOF' >&2
configure: error:

  Your system does not have the sprintf/vsprintf libraries.
  These are required to compile almost anything.  Sorry.

EOF
  exit 1
fi
])dnl


AC_DEFUN(EGG_HEADER_STDC, [dnl
if test "x${ac_cv_header_stdc}" = "xno"
then
  cat << 'EOF' >&2
configure: error:

  Your system must support ANSI C Header files.
  These are required for the language support.  Sorry.

EOF
  exit 1
fi
])dnl


AC_DEFUN(EGG_CYGWIN, [dnl
AC_CYGWIN
if test ! "x${CYGWIN}" = "x"
then
  AC_DEFINE(CYGWIN_HACKS)dnl
fi
])dnl


AC_DEFUN(EGG_EXEEXT, [dnl
EGGEXEC=eggdrop
AC_EXEEXT
if test ! "x${EXEEXT}" = "x"
then
  EGGEXEC=eggdrop${EXEEXT}
fi
AC_SUBST(EGGEXEC)dnl
])dnl


AC_DEFUN(EGG_CHECK_FILEDB_STRUCT, [dnl
AC_MSG_CHECKING(space left in file database struct)

if eval "test \"`echo '$''{'egg_cv_struct_filedb_size'+set}'`\" = set"
then
  echo $ac_n "(cached) $ac_c" 1>&6
else
  cat > conftest.$ac_ext << EOF
#include "confdefs.h"
#include <stdio.h>
#include <sys/time.h>
#include "src/mod/filesys.mod/files.h"
int main() {
  fprintf(stdout, "%d/%d %s\n", 512 - sizeof(struct filler), sizeof(filedb), "bytes");
}
EOF
  if { (eval echo configure: \"$ac_link\") 1>&5; (eval $ac_link) 2>&5; } && test -s conftest${ac_exeext}
  then
    eval "egg_cv_struct_filedb_size='`./conftest${ac_exeext}`'"
    rm -rf conftest*
  else
    echo "configure: failed program was:" >&5
    cat conftest.$ac_ext >&5
    eval "egg_cv_struct_filedb_size=unknown"
    rm -rf conftest*
  fi
  rm -f conftest*
fi
AC_MSG_RESULT($egg_cv_struct_filedb_size)
AC_MSG_RESULT([   (standard is currently 48/512 bytes)])
])dnl


AC_DEFUN(EGG_TCL_ARG_WITH, [dnl
# oohh new configure --variables for those with multiple tcl libs
AC_ARG_WITH(tcllib, [  --with-tcllib=PATH      full path to tcl library], tcllibname=$withval)
AC_ARG_WITH(tclinc, [  --with-tclinc=PATH      full path to tcl header], tclincname=$withval)

# Make sure either both or neither $tcllibname and $tclincname are set
if test ! "x${tcllibname}" = "x"
then
  if test "x${tclincname}" = "x"
  then
    cat << 'EOF' >&2
configure: warning:

  You must specify both --with-tcllib and --with-tclinc for them to work.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
    tcllibname=""
    TCLLIB=""
    TCLINC=""
  fi
else
  if test ! "x${tclincname}" = "x"
  then
    cat << 'EOF' >&2
configure: warning:

  You must specify both --with-tcllib and --with-tclinc for them to work.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
    tclincname=""
    TCLLIB=""
    TCLINC=""
  fi
fi
])dnl


AC_DEFUN(EGG_TCL_ENV, [dnl
# Make sure either both or neither $TCLLIB and $TCLINC are set
if test ! "x${TCLLIB}" = "x"
then
  if test "x${TCLINC}" = "x"
  then
    cat << 'EOF' >&2
configure: warning:

  Environment variable TCLLIB was set, but I did not detect TCLINC.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
    TCLLIB=""
  fi
else
  if test ! "x${TCLINC}" = "x"
  then
    cat << 'EOF' >&2
configure: warning:

  Environment variable TCLINC was set, but I did not detect TCLLIB.
  Please set both TCLLIB and TCLINC correctly if you wish to use them.
  configure will now attempt to autodetect both the Tcl library and header...

EOF
    TCLINC=""
  fi
fi
])dnl


AC_DEFUN(EGG_TCL_WITH_TCLLIB, [dnl
# Look for Tcl library: if $tcllibname is set, check there first
if test ! "x${tcllibname}" = "x"
then
  if test -f "$tcllibname" && test -r "$tcllibname"
  then
    TCLLIB=`echo $tcllibname | sed 's%/[^/][^/]*$%%'`
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


AC_DEFUN(EGG_TCL_WITH_TCLINC, [dnl
# Look for Tcl header: if $tclincname is set, check there first
if test ! "x${tclincname}" = "x"
then
  if test -f "$tclincname" && test -r "$tclincname"
  then
    TCLINC=`echo $tclincname | sed 's%/[^/][^/]*$%%'`
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


AC_DEFUN(EGG_TCL_FIND_LIBRARY, [dnl
# Look for Tcl library: if $TCLLIB is set, check there first
if test "x${TCLLIBFN}" = "x"
then
  if test ! "x${TCLLIB}" = "x"
  then
    if test -d "${TCLLIB}"
    then
      for tcllibfns in $tcllibnames
      do
        for tcllibext in $tcllibextensions
        do
          if test -r "$TCLLIB/lib$tcllibfns$tcllibext"
          then
            TCLLIBFN=$tcllibfns$tcllibext
            TCLLIBEXT=$tcllibext
            TCLLIBFNS=$tcllibfns
            break 2
          fi
        done
      done
    fi
    if test "x${TCLLIBFN}" = "x"
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


AC_DEFUN(EGG_TCL_FIND_HEADER, [dnl
# Look for Tcl header: if $TCLINC is set, check there first
if test "x${TCLINCFN}" = "x"
then
  if test ! "x${TCLINC}" = "x"
  then
    if test -d "${TCLINC}"
    then
      for tclheaderfn in $tclheadernames
      do
        if test -r "$TCLINC/$tclheaderfn"
        then
          TCLINCFN=$tclheaderfn
        fi
      done
    fi
    if test "x${TCLINCFN}" = "x"
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


AC_DEFUN(EGG_TCL_CHECK_LIBRARY, [dnl
AC_MSG_CHECKING(for Tcl library)

# Attempt autodetect for $TCLLIBFN if it's not set
if test ! "x${TCLLIBFN}" = "x"
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
          TCLLIB=$tcllibpath
          TCLLIBFN=$tcllibfns$tcllibext
          TCLLIBEXT=$tcllibext
          TCLLIBFNS=$tcllibfns
          break 3
        fi
      done
    done
  done
fi

# Show if $TCLLIBFN wasn't found
if test "x${TCLLIBFN}" = "x"
then
  AC_MSG_RESULT(not found)
fi
AC_SUBST(TCLLIB)dnl
AC_SUBST(TCLLIBFN)dnl
])dnl


AC_DEFUN(EGG_TCL_CHECK_HEADER, [dnl
AC_MSG_CHECKING(for Tcl header)

# Attempt autodetect for $TCLINCFN if it's not set
if test ! "x${TCLINCFN}" = "x"
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
        TCLINC=$tclheaderpath
        TCLINCFN=$tclheaderfn
        break 2
      fi
    done
  done
  # FreeBSD hack ...
  if test "x${TCLINCFN}" = "x"
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
            TCLINC=$tclheaderpath/$tcllibfns
            TCLINCFN=$tclheaderfn
            break 3
          fi
        done
      done
    done
  fi
fi

# Show if $TCLINCFN wasn't found
if test "x${TCLINCFN}" = "x"
then
  AC_MSG_RESULT(not found)
fi
AC_SUBST(TCLINC)dnl
AC_SUBST(TCLINCFN)dnl
])dnl


AC_DEFUN(EGG_TCL_CHECK_VERSION, [dnl
# Both TCLLIBFN & TCLINCFN must be set, or we bail
if test ! "x${TCLLIBFN}" = "x" && test ! "x${TCLINCFN}" = "x"
then
  TCL_FOUND=1

  # Check Tcl's version
  AC_MSG_CHECKING(for Tcl version)
  if eval "test \"`echo '$''{'egg_cv_var_tcl_version'+set}'`\" = set"
  then
    echo $ac_n "(cached) $ac_c" 1>&6
  else
    eval "egg_cv_var_tcl_version=`grep TCL_VERSION $TCLINC/$TCLINCFN | head -1 | $AWK '{gsub(/\"/, "", [$]3); print [$]3}'`"
  fi

  if test ! "x${egg_cv_var_tcl_version}" = "x"
  then
    AC_MSG_RESULT($egg_cv_var_tcl_version)
  else
    AC_MSG_RESULT(not found)
    TCL_FOUND=0
  fi

  # Check Tcl's patch level (if avaliable)
  AC_MSG_CHECKING(for Tcl patch level)
  if eval "test \"`echo '$''{'egg_cv_var_tcl_patch_level'+set}'`\" = set"
  then
    echo $ac_n "(cached) $ac_c" 1>&6
  else
    eval "egg_cv_var_tcl_patch_level=`grep TCL_PATCH_LEVEL $TCLINC/$TCLINCFN | head -1 | $AWK '{gsub(/\"/, "", [$]3); print [$]3}'`"
  fi

  if test ! "x${egg_cv_var_tcl_patch_level}" = "x"
  then
    AC_MSG_RESULT($egg_cv_var_tcl_patch_level)
  else
    egg_cv_var_tcl_patch_level="unknown"
    AC_MSG_RESULT(unknown)
  fi
fi

# Check if we found Tcl's version
if test $TCL_FOUND = 0
then
  cat << 'EOF' >&2
configure: error:

  I can't find Tcl on this system.

  Eggdrop now requires Tcl to compile.  If you already have Tcl
  installed on this system, and I just wasn't looking in the right
  place for it, set the environment variables TCLLIB and TCLINC so
  I will know where to find 'libtcl.a' (or 'libtcl.so') and 'tcl.h'
  (respectively).  Then run 'configure' again.

  Read the README file if you don't know what Tcl is or how to get
  it and install it.

EOF
  exit 1
fi
])dnl


AC_DEFUN(EGG_TCL_CHECK_PRE70, [dnl
# Is this version of Tcl too old for us to use ?
TCL_VER_PRE70=`echo $egg_cv_var_tcl_version | $AWK '{split([$]1, i, "."); if (i[[1]] < 7) print "yes"; else print "no"}'`
if test "$TCL_VER_PRE70" = "xyes"
then
  cat << EOF >&2
configure: error:

  Your Tcl version is much too old for eggdrop to use.
  I suggest you download and complie a more recent version.
  The most reliable current version is $tclrecommendver and
  can be downloaded from $tclrecommendsite

EOF
  exit 1
fi
])dnl


AC_DEFUN(EGG_TCL_CHECK_PRE75, [dnl
# Are we using a pre 7.5 Tcl version ?
TCL_VER_PRE75=`echo $egg_cv_var_tcl_version | $AWK '{split([$]1, i, "."); if (((i[[1]] == 7) && (i[[2]] < 5)) || (i[[1]] < 7)) print "yes"; else print "no"}'`
if test "x${TCL_VER_PRE75}" = "xyes"
then
  AC_DEFINE(HAVE_PRE7_5_TCL)dnl
fi
])dnl


AC_DEFUN(EGG_TCL_TESTLIBS, [dnl
# Setup TCL_TESTLIBS for Tcl library tests
if test ! "x${TCLLIBEXT}" = "x.a"
then
  TCL_TESTLIBS="-L$TCLLIB -l$TCLLIBFNS -lm $LIBS"
else
  if test ! "x${tcllibname}" = "x"
  then
    TCL_TESTLIBS="$TCLLIB/lib$TCLLIBFN -lm $LIBS"
  else
    TCL_TESTLIBS="-L$TCLLIB -l$TCLLIBFNS -lm $LIBS"
  fi
fi
if test "x${ac_cv_lib_pthread_pthread_mutex_init}" = "xyes"
then
  TCL_TESTLIBS="-lpthread $TCL_TESTLIBS"
fi
])dnl


AC_DEFUN(EGG_TCL_CHECK_FREE, [dnl
# Check for Tcl_Free()
AC_MSG_CHECKING(if Tcl library has Tcl_Free)
if eval "test \"`echo '$''{'egg_cv_var_tcl_free'+set}'`\" = set"
then
  echo $ac_n "(cached) $ac_c" 1>&6
else
  ac_save_LIBS="$LIBS"
  LIBS="$TCL_TESTLIBS"
  cat > conftest.$ac_ext << EOF
#include "confdefs.h"
char Tcl_Free();

int main() {
  Tcl_Free();
  return 0;
}
EOF
  if { (eval echo configure: \"$ac_link\") 1>&5; (eval $ac_link) 2>&5; } && test -s conftest${ac_exeext}
  then
    rm -rf conftest*
    eval "egg_cv_var_tcl_free=yes"
  else
    echo "configure: failed program was:" >&5
    cat conftest.$ac_ext >&5
    rm -rf conftest*
    eval "egg_cv_var_tcl_free=no"
  fi
  rm -f conftest*
  LIBS="$ac_save_LIBS"
fi

if test "x${egg_cv_var_tcl_free}" = "xyes"
then
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_TCL_FREE)dnl
else
  AC_MSG_RESULT(no)
fi
])dnl


AC_DEFUN(EGG_TCL_CHECK_THREADS, [dnl
# Check for TclpFinalizeThreadData()
AC_MSG_CHECKING(if Tcl library is multithreaded)
if eval "test \"`echo '$''{'egg_cv_var_tcl_multithreaded'+set}'`\" = set"
then
  echo $ac_n "(cached) $ac_c" 1>&6
else
  ac_save_LIBS="$LIBS"
  LIBS="$TCL_TESTLIBS"
  cat > conftest.$ac_ext << EOF
#include "confdefs.h"
char TclpFinalizeThreadData();

int main() {
  TclpFinalizeThreadData();
  return 0;
}
EOF
  if { (eval echo configure: \"$ac_link\") 1>&5; (eval $ac_link) 2>&5; } && test -s conftest${ac_exeext}
  then
    rm -rf conftest*
    eval "egg_cv_var_tcl_multithreaded=yes"
  else
    echo "configure: failed program was:" >&5
    cat conftest.$ac_ext >&5
    rm -rf conftest*
    eval "egg_cv_var_tcl_multithreaded=no"
  fi
  rm -f conftest*
  LIBS="$ac_save_LIBS"
fi

if test "x${egg_cv_var_tcl_multithreaded}" = "xyes"
then
  AC_MSG_RESULT(yes)
  cat << 'EOF' >&2
configure: warning:

  Your Tcl library is compiled with threads support.
  There are known problems, but we will attempt to work around them.

EOF
  AC_DEFINE(HAVE_TCL_THREADS)dnl

  # Add -lpthread to $LIBS if we have it
  if test "x${ac_cv_lib_pthread_pthread_mutex_init}" = "xyes"
  then
    LIBS="-lpthread $LIBS"
  fi
else
  AC_MSG_RESULT(no)
fi
])dnl


AC_DEFUN(EGG_TCL_LIB_REQS, [dnl
if test ! "x${TCLLIBEXT}" = "x.a"
then
  TCL_REQS="$TCLLIB/lib$TCLLIBFN"
  TCL_LIBS="-L$TCLLIB -l$TCLLIBFNS -lm"
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
  if test "x${TCL_VER_PRE74}" = "xno"
  then

    # Was the --with-tcllib option given ?
    if test ! "x${tcllibname}" = "x"
    then
      TCL_REQS="$TCLLIB/lib$TCLLIBFN"
      TCL_LIBS="$TCLLIB/lib$TCLLIBFN -lm"
    else
      TCL_REQS="$TCLLIB/lib$TCLLIBFN"
      TCL_LIBS="-L$TCLLIB -l$TCLLIBFNS -lm"
    fi
  else
    cat << EOF >&2
configure: warning:

  Your Tcl version ($egg_cv_var_tcl_version) is older then 7.4.
  There are known problems, but we will attempt to work around them.

EOF
    TCL_REQS="libtcle.a"
    TCL_LIBS="-L. -ltcle -lm"
  fi
fi
AC_SUBST(TCL_REQS)dnl
AC_SUBST(TCL_LIBS)dnl
])dnl


AC_DEFUN(EGG_FUNC_DLOPEN, [dnl
if test $NEED_DL = 1 && test "x${ac_cv_func_dlopen}" = "xno"
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


AC_DEFUN(EGG_SUBST_EGGVERSION, [dnl
EGGVERSION=`grep 'char egg_version' src/main.c | $AWK '{gsub(/(\"|\;)/, "", [$]4); print [$]4}'`
AC_SUBST(EGGVERSION)dnl
])dnl


AC_DEFUN(EGG_SUBST_DEST, [dnl
if test "x$DEST" = "x"
then
  DEST=\${prefix}
fi
AC_SUBST(DEST)dnl
])dnl
