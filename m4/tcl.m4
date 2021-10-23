# tcl.m4 --
#
#	This file provides a set of autoconf macros to help TEA-enable
#	a Tcl extension.
#
# Copyright (c) 1999-2000 Ajuba Solutions.
# Copyright (c) 2002-2005 ActiveState Corporation.
# Copyright (c) 2017 - 2021 Eggheads Development Team
#
# Original Tcl/TEA license.terms information for this file:
# This software is copyrighted by the Regents of the University of
# California, Sun Microsystems, Inc., Scriptics Corporation, ActiveState
# Corporation and other parties.  The following terms apply to all files
# associated with the software unless explicitly disclaimed in
# individual files.

# The authors hereby grant permission to use, copy, modify, distribute,
# and license this software and its documentation for any purpose, provided
# that existing copyright notices are retained in all copies and that this
# notice is included verbatim in any distributions. No written agreement,
# license, or royalty fee is required for any of the authorized uses.
# Modifications to this software may be copyrighted by their authors
# and need not follow the licensing terms described here, provided that
# the new terms are clearly indicated on the first page of each file where
# they apply.

# IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
# FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
# ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
# DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
# IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
# NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
# MODIFICATIONS.

# GOVERNMENT USE: If you are acquiring this software on behalf of the
# U.S. government, the Government shall have only "Restricted Rights"
# in the software and related documentation as defined in the Federal
# Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
# are acquiring the software on behalf of the Department of Defense, the
# software shall be classified as "Commercial Computer Software" and the
# Government shall have only "Restricted Rights" as defined in Clause
# 252.227-7014 (b) (3) of DFARs.  Notwithstanding the foregoing, the
# authors grant the U.S. Government and others acting in its behalf
# permission to use and distribute the software in accordance with the
# terms specified in this license.

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
                        `ls -d /usr/local/lib/tcl[[8-9]].[[0-9]] 2>/dev/null` \
                        `ls -d /usr/local/lib/tcl/tcl[[8-9]].[[0-9]] 2>/dev/null` \
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

#------------------------------------------------------------------------
# TEA_LOAD_TCLCONFIG --
#
#	Load the tclConfig.sh file
#
# Arguments:
#
#	Requires the following vars to be set:
#		TCL_BIN_DIR
#
# Results:
#
#	Substitutes the following vars:
#		TCL_BIN_DIR
#		TCL_SRC_DIR
#		TCL_LIB_FILE
#               TCL_ZIP_FILE
#               TCL_ZIPFS_SUPPORT
#------------------------------------------------------------------------

AC_DEFUN([TEA_LOAD_TCLCONFIG], [
    AC_MSG_CHECKING([for existence of ${TCL_BIN_DIR}/tclConfig.sh])

    if test -f "${TCL_BIN_DIR}/tclConfig.sh" ; then
        AC_MSG_RESULT([loading])
	. "${TCL_BIN_DIR}/tclConfig.sh"
    else
        AC_MSG_RESULT([could not find ${TCL_BIN_DIR}/tclConfig.sh])
    fi

    # eval is required to do the TCL_DBGX substitution
    eval "TCL_LIB_FILE=\"${TCL_LIB_FILE}\""
    eval "TCL_STUB_LIB_FILE=\"${TCL_STUB_LIB_FILE}\""

    # If the TCL_BIN_DIR is the build directory (not the install directory),
    # then set the common variable name to the value of the build variables.
    # For example, the variable TCL_LIB_SPEC will be set to the value
    # of TCL_BUILD_LIB_SPEC. An extension should make use of TCL_LIB_SPEC
    # instead of TCL_BUILD_LIB_SPEC since it will work with both an
    # installed and uninstalled version of Tcl.
    if test -f "${TCL_BIN_DIR}/Makefile" ; then
        TCL_LIB_SPEC="${TCL_BUILD_LIB_SPEC}"
        TCL_STUB_LIB_SPEC="${TCL_BUILD_STUB_LIB_SPEC}"
        TCL_STUB_LIB_PATH="${TCL_BUILD_STUB_LIB_PATH}"
    elif test "`uname -s`" = "Darwin"; then
	# If Tcl was built as a framework, attempt to use the libraries
	# from the framework at the given location so that linking works
	# against Tcl.framework installed in an arbitrary location.
	case ${TCL_DEFS} in
	    *TCL_FRAMEWORK*)
		if test -f "${TCL_BIN_DIR}/${TCL_LIB_FILE}"; then
		    for i in "`cd "${TCL_BIN_DIR}"; pwd`" \
			     "`cd "${TCL_BIN_DIR}"/../..; pwd`"; do
			if test "`basename "$i"`" = "${TCL_LIB_FILE}.framework"; then
			    TCL_LIB_SPEC="-F`dirname "$i" | sed -e 's/ /\\\\ /g'` -framework ${TCL_LIB_FILE}"
			    break
			fi
		    done
		fi
		if test -f "${TCL_BIN_DIR}/${TCL_STUB_LIB_FILE}"; then
		    TCL_STUB_LIB_SPEC="-L`echo "${TCL_BIN_DIR}"  | sed -e 's/ /\\\\ /g'` ${TCL_STUB_LIB_FLAG}"
		    TCL_STUB_LIB_PATH="${TCL_BIN_DIR}/${TCL_STUB_LIB_FILE}"
		fi
		;;
	esac
    fi

    # eval is required to do the TCL_DBGX substitution
    eval "TCL_LIB_FLAG=\"${TCL_LIB_FLAG}\""
    eval "TCL_LIB_SPEC=\"${TCL_LIB_SPEC}\""
    eval "TCL_STUB_LIB_FLAG=\"${TCL_STUB_LIB_FLAG}\""
    eval "TCL_STUB_LIB_SPEC=\"${TCL_STUB_LIB_SPEC}\""

    AC_SUBST(TCL_VERSION)
    AC_SUBST(TCL_PATCH_LEVEL)
    AC_SUBST(TCL_BIN_DIR)
    AC_SUBST(TCL_SRC_DIR)

    AC_SUBST(TCL_LIB_FILE)
    AC_SUBST(TCL_LIB_FLAG)
    AC_SUBST(TCL_LIB_SPEC)

    AC_SUBST(TCL_STUB_LIB_FILE)
    AC_SUBST(TCL_STUB_LIB_FLAG)
    AC_SUBST(TCL_STUB_LIB_SPEC)

    AC_MSG_CHECKING([platform])
    hold_cc=$CC; CC="$TCL_CC"
    AC_TRY_COMPILE(,[
	    #ifdef _WIN32
		#error win32
	    #endif
	], [
	    TEA_PLATFORM="unix"
	    CYGPATH=echo
	], [
	    TEA_PLATFORM="windows"
	    AC_CHECK_PROG(CYGPATH, cygpath, cygpath -m, echo)	]
    )
    CC=$hold_cc
    AC_MSG_RESULT($TEA_PLATFORM)

    # The BUILD_$pkg is to define the correct extern storage class
    # handling when making this package
    AC_DEFINE_UNQUOTED(BUILD_${PACKAGE_NAME}, [],
	    [Building extension source?])
    # Do this here as we have fully defined TEA_PLATFORM now
    if test "${TEA_PLATFORM}" = "windows" ; then
	EXEEXT=".exe"
	CLEANFILES="$CLEANFILES *.lib *.dll *.pdb *.exp"
    fi

    # TEA specific:
    AC_SUBST(CLEANFILES)
    AC_SUBST(TCL_LIBS)
    AC_SUBST(TCL_DEFS)
    AC_SUBST(TCL_EXTRA_CFLAGS)
    AC_SUBST(TCL_LD_FLAGS)
    AC_SUBST(TCL_SHLIB_LD_LIBS)
])

#--------------------------------------------------------------------
# TEA_TCL_LINK_LIBS
#
#	Search for the libraries needed to link the Tcl shell.
#	Things like the math library (-lm) and socket stuff (-lsocket vs.
#	-lnsl) are dealt with here.
#
# Arguments:
#	Requires the following vars to be set in the Makefile:
#		DL_LIBS (not in TEA, only needed in core)
#		LIBS
#		MATH_LIBS
#
# Results:
#
#	Substitutes the following vars:
#		TCL_LIBS
#		MATH_LIBS
#
#	Might append to the following vars:
#		LIBS
#
#	Might define the following vars:
#		HAVE_NET_ERRNO_H
#--------------------------------------------------------------------

AC_DEFUN([TEA_TCL_LINK_LIBS], [
    #--------------------------------------------------------------------
    # On a few very rare systems, all of the libm.a stuff is
    # already in libc.a.  Set compiler flags accordingly.
    # Also, Linux requires the "ieee" library for math to work
    # right (and it must appear before "-lm").
    #--------------------------------------------------------------------

    AC_CHECK_FUNC(sin, MATH_LIBS="", MATH_LIBS="-lm")
    AC_CHECK_LIB(ieee, main, [MATH_LIBS="-lieee $MATH_LIBS"])

    #--------------------------------------------------------------------
    # Interactive UNIX requires -linet instead of -lsocket, plus it
    # needs net/errno.h to define the socket-related error codes.
    #--------------------------------------------------------------------

    AC_CHECK_LIB(inet, main, [LIBS="$LIBS -linet"])
    AC_CHECK_HEADER(net/errno.h, [
	AC_DEFINE(HAVE_NET_ERRNO_H, 1, [Do we have <net/errno.h>?])])

    #--------------------------------------------------------------------
    #	Check for the existence of the -lsocket and -lnsl libraries.
    #	The order here is important, so that they end up in the right
    #	order in the command line generated by make.  Here are some
    #	special considerations:
    #	1. Use "connect" and "accept" to check for -lsocket, and
    #	   "gethostbyname" to check for -lnsl.
    #	2. Use each function name only once:  can't redo a check because
    #	   autoconf caches the results of the last check and won't redo it.
    #	3. Use -lnsl and -lsocket only if they supply procedures that
    #	   aren't already present in the normal libraries.  This is because
    #	   IRIX 5.2 has libraries, but they aren't needed and they're
    #	   bogus:  they goof up name resolution if used.
    #	4. On some SVR4 systems, can't use -lsocket without -lnsl too.
    #	   To get around this problem, check for both libraries together
    #	   if -lsocket doesn't work by itself.
    #--------------------------------------------------------------------

    tcl_checkBoth=0
    AC_CHECK_FUNC(connect, tcl_checkSocket=0, tcl_checkSocket=1)
    if test "$tcl_checkSocket" = 1; then
	AC_CHECK_FUNC(setsockopt, , [AC_CHECK_LIB(socket, setsockopt,
	    LIBS="$LIBS -lsocket", tcl_checkBoth=1)])
    fi
    if test "$tcl_checkBoth" = 1; then
	tk_oldLibs=$LIBS
	LIBS="$LIBS -lsocket -lnsl"
	AC_CHECK_FUNC(accept, tcl_checkNsl=0, [LIBS=$tk_oldLibs])
    fi
    AC_CHECK_FUNC(gethostbyname, , [AC_CHECK_LIB(nsl, gethostbyname,
	    [LIBS="$LIBS -lnsl"])])

    # TEA specific: Don't perform the eval of the libraries here because
    # DL_LIBS won't be set until we call TEA_CONFIG_CFLAGS

    TCL_LIBS='${DL_LIBS} ${LIBS} ${MATH_LIBS}'
    AC_SUBST(TCL_LIBS)
    AC_SUBST(MATH_LIBS)
])
