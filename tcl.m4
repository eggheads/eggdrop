# tcl.m4 --
#
#	This file provides a set of autoconf macros to help TEA-enable
#	a Tcl extension.
#
# Copyright (c) 1999-2000 Ajuba Solutions.
# Copyright (c) 2002-2005 ActiveState Corporation.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

AC_PREREQ(2.57)

dnl TEA extensions pass us the version of TEA they think they
dnl are compatible with (must be set in TEA_INIT below)
dnl TEA_VERSION="3.10"

# Possible values for key variables defined:
#
# TEA_WINDOWINGSYSTEM - win32 aqua x11 (mirrors 'tk windowingsystem')
# PRACTCL_WINDOWINGSYSTEM - windows cocoa hitheme x11 sdl
# TEA_PLATFORM        - windows unix
# TEA_TK_EXTENSION    - True if this is a Tk extension
# TEACUP_OS           - windows macosx linux generic
# TEACUP_TOOLSET      - Toolset in use (gcc,mingw,msvc,llvm)
# TEACUP_PROFILE      - win32  
#

#------------------------------------------------------------------------
# TEA_PATH_TCLCONFIG --
#
#	Locate the tclConfig.sh file and perform a sanity check on
#	the Tcl compile flags
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-tcl=...
#
#	Defines the following vars:
#		TCL_BIN_DIR	Full path to the directory containing
#				the tclConfig.sh file
#------------------------------------------------------------------------

AC_DEFUN([TEA_PATH_TCLCONFIG], [
    dnl TEA specific: Make sure we are initialized
    AC_REQUIRE([TEA_INIT])
    #
    # Ok, lets find the tcl configuration
    # First, look for one uninstalled.
    # the alternative search directory is invoked by --with-tcl
    #

    if test x"${no_tcl}" = x ; then
	# we reset no_tcl in case something fails here
	no_tcl=true
	AC_ARG_WITH(tcl,
	    AC_HELP_STRING([--with-tcl],
		[directory containing tcl configuration (tclConfig.sh)]),
	    with_tclconfig="${withval}")
	AC_MSG_CHECKING([for Tcl configuration])
	AC_CACHE_VAL(ac_cv_c_tclconfig,[

	    # First check to see if --with-tcl was specified.
	    if test x"${with_tclconfig}" != x ; then
		case "${with_tclconfig}" in
		    */tclConfig.sh )
			if test -f "${with_tclconfig}"; then
			    AC_MSG_WARN([--with-tcl argument should refer to directory containing tclConfig.sh, not to tclConfig.sh itself])
			    with_tclconfig="`echo "${with_tclconfig}" | sed 's!/tclConfig\.sh$!!'`"
			fi ;;
		esac
		if test -f "${with_tclconfig}/tclConfig.sh" ; then
		    ac_cv_c_tclconfig="`(cd "${with_tclconfig}"; pwd)`"
		else
		    AC_MSG_ERROR([${with_tclconfig} directory doesn't contain tclConfig.sh])
		fi
	    fi

	    # then check for a private Tcl installation
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in \
			../tcl \
			`ls -dr ../tcl[[8-9]].[[0-9]].[[0-9]]* 2>/dev/null` \
			`ls -dr ../tcl[[8-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ../tcl[[8-9]].[[0-9]]* 2>/dev/null` \
			../../tcl \
			`ls -dr ../../tcl[[8-9]].[[0-9]].[[0-9]]* 2>/dev/null` \
			`ls -dr ../../tcl[[8-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ../../tcl[[8-9]].[[0-9]]* 2>/dev/null` \
			../../../tcl \
			`ls -dr ../../../tcl[[8-9]].[[0-9]].[[0-9]]* 2>/dev/null` \
			`ls -dr ../../../tcl[[8-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ../../../tcl[[8-9]].[[0-9]]* 2>/dev/null` ; do
		    if test "${TEA_PLATFORM}" = "windows" \
			    -a -f "$i/win/tclConfig.sh" ; then
			ac_cv_c_tclconfig="`(cd $i/win; pwd)`"
			break
		    fi
		    if test -f "$i/unix/tclConfig.sh" ; then
			ac_cv_c_tclconfig="`(cd $i/unix; pwd)`"
			break
		    fi
		done
	    fi

	    # on Darwin, check in Framework installation locations
	    if test "`uname -s`" = "Darwin" -a x"${ac_cv_c_tclconfig}" = x ; then
		for i in `ls -d ~/Library/Frameworks 2>/dev/null` \
			`ls -d /Library/Frameworks 2>/dev/null` \
			`ls -d /Network/Library/Frameworks 2>/dev/null` \
			`ls -d /System/Library/Frameworks 2>/dev/null` \
			; do
		    if test -f "$i/Tcl.framework/tclConfig.sh" ; then
			ac_cv_c_tclconfig="`(cd $i/Tcl.framework; pwd)`"
			break
		    fi
		done
	    fi

	    # TEA specific: on Windows, check in common installation locations
	    if test "${TEA_PLATFORM}" = "windows" \
		-a x"${ac_cv_c_tclconfig}" = x ; then
		for i in `ls -d C:/Tcl/lib 2>/dev/null` \
			`ls -d C:/Progra~1/Tcl/lib 2>/dev/null` \
			; do
		    if test -f "$i/tclConfig.sh" ; then
			ac_cv_c_tclconfig="`(cd $i; pwd)`"
			break
		    fi
		done
	    fi

	    # check in a few common install locations
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in `ls -d ${libdir} 2>/dev/null` \
			`ls -d ${exec_prefix}/lib 2>/dev/null` \
			`ls -d ${prefix}/lib 2>/dev/null` \
			`ls -d /usr/contrib/lib 2>/dev/null` \
			`ls -d /usr/local/lib 2>/dev/null` \
			`ls -d /usr/pkg/lib 2>/dev/null` \
			`ls -d /usr/lib 2>/dev/null` \
			`ls -d /usr/lib64 2>/dev/null` \
                        `ls -d /usr/lib/tcl[[8-9]].[[0-9]] 2>/dev/null` \
                        `ls -d /usr/local/lib/tcl[[8-9]].[[0-9]] 2>/dev/null \
			; do
		    if test -f "$i/tclConfig.sh" ; then
			ac_cv_c_tclconfig="`(cd $i; pwd)`"
			break
		    fi
		done
	    fi

	    # check in a few other private locations
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in \
			${srcdir}/../tcl \
			`ls -dr ${srcdir}/../tcl[[8-9]].[[0-9]].[[0-9]]* 2>/dev/null` \
			`ls -dr ${srcdir}/../tcl[[8-9]].[[0-9]] 2>/dev/null` \
			`ls -dr ${srcdir}/../tcl[[8-9]].[[0-9]]* 2>/dev/null` ; do
		    if test "${TEA_PLATFORM}" = "windows" \
			    -a -f "$i/win/tclConfig.sh" ; then
			ac_cv_c_tclconfig="`(cd $i/win; pwd)`"
			break
		    fi
		    if test -f "$i/unix/tclConfig.sh" ; then
			ac_cv_c_tclconfig="`(cd $i/unix; pwd)`"
			break
		    fi
		done
	    fi
	])

	if test x"${ac_cv_c_tclconfig}" = x ; then
	    TCL_BIN_DIR="# no Tcl configs found"
	    AC_MSG_ERROR([Can't find Tcl configuration definitions. Use --with-tcl to specify a directory containing tclConfig.sh])
	else
	    no_tcl=
	    TCL_BIN_DIR="${ac_cv_c_tclconfig}"
	    AC_MSG_RESULT([found ${TCL_BIN_DIR}/tclConfig.sh])
	fi
    fi
])

##
## Here ends the standard Tcl configuration bits and starts the
## TEA specific functions
##

#------------------------------------------------------------------------
# TEA_INIT --
#
#	Init various Tcl Extension Architecture (TEA) variables.
#	This should be the first called TEA_* macro.
#
# Arguments:
#	none
#
# Results:
#
#	Defines and substs the following vars:
#		CYGPATH
#		EXEEXT
#	Defines only:
#		TEA_VERSION
#		TEA_INITED
#		TEA_PLATFORM (windows or unix)
#
# "cygpath" is used on windows to generate native path names for include
# files. These variables should only be used with the compiler and linker
# since they generate native path names.
#
# EXEEXT
#	Select the executable extension based on the host type.  This
#	is a lightweight replacement for AC_EXEEXT that doesn't require
#	a compiler.
#------------------------------------------------------------------------

AC_DEFUN([TEA_INIT], [
    # TEA extensions pass this us the version of TEA they think they
    # are compatible with.
    TEA_VERSION="3.10"
    AC_MSG_CHECKING([for correct TEA configuration])
    if test x"${PACKAGE_NAME}" = x ; then
	AC_MSG_ERROR([
The PACKAGE_NAME variable must be defined by your TEA configure.ac])
    fi
    if test x"$1" = x ; then
	AC_MSG_ERROR([
TEA version not specified.])
    elif test "$1" != "${TEA_VERSION}" ; then
	AC_MSG_RESULT([warning: requested TEA version "$1", have "${TEA_VERSION}"])
    else
	AC_MSG_RESULT([ok (TEA ${TEA_VERSION})])
    fi

    # If the user did not set CFLAGS, set it now to keep macros
    # like AC_PROG_CC and AC_TRY_COMPILE from adding "-g -O2".
    if test "${CFLAGS+set}" != "set" ; then
	CFLAGS=""
    fi
		TEA_TK_EXTENSION=0
		AC_SUBST(TEA_TK_EXTENSION)
    case "`uname -s`" in
	*win32*|*WIN32*|*MINGW32_*)
	    AC_CHECK_PROG(CYGPATH, cygpath, cygpath -m, echo)
	    EXEEXT=".exe"
	    TEA_PLATFORM="windows"
	    ;;
	*CYGWIN_*)
	    EXEEXT=".exe"
	    # CYGPATH and TEA_PLATFORM are determined later in LOAD_TCLCONFIG
	    ;;
	*)
	    CYGPATH=echo
	    # Maybe we are cross-compiling....
	    case ${host_alias} in
		*mingw32*)
		EXEEXT=".exe"
		TEA_PLATFORM="windows"
		;;
	    *)
		EXEEXT=""
		TEA_PLATFORM="unix"
		;;
	    esac
	    ;;
    esac

    # Check if exec_prefix is set. If not use fall back to prefix.
    # Note when adjusted, so that TEA_PREFIX can correct for this.
    # This is needed for recursive configures, since autoconf propagates
    # $prefix, but not $exec_prefix (doh!).
    if test x$exec_prefix = xNONE ; then
	exec_prefix_default=yes
	exec_prefix=$prefix
    fi

    AC_MSG_NOTICE([configuring ${PACKAGE_NAME} ${PACKAGE_VERSION}])

    AC_SUBST(EXEEXT)
    AC_SUBST(CYGPATH)

    # This package name must be replaced statically for AC_SUBST to work
    AC_SUBST(PKG_LIB_FILE)
    # Substitute STUB_LIB_FILE in case package creates a stub library too.
    AC_SUBST(PKG_STUB_LIB_FILE)

    # We AC_SUBST these here to ensure they are subst'ed,
    # in case the user doesn't call TEA_ADD_...
    AC_SUBST(PKG_STUB_SOURCES)
    AC_SUBST(PKG_STUB_OBJECTS)
    AC_SUBST(PKG_TCL_SOURCES)
    AC_SUBST(PKG_HEADERS)
    AC_SUBST(PKG_INCLUDES)
    AC_SUBST(PKG_LIBS)
    AC_SUBST(PKG_CFLAGS)
])
