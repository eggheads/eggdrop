dnl configure.ac: this file is processed by autoconf to produce ./configure.

AC_PREREQ(2.69)

sinclude(../eggmod.m4)
builtin(include,../../../m4/python.m4)
builtin(include,../../../m4/ax_compare_version.m4)

AC_INIT([Eggdrop Python Module],[1.10.0],[bugs@eggheads.org])

AC_CONFIG_SRCDIR(python.c)
AC_CONFIG_AUX_DIR(../../../misc)

AC_COPYRIGHT([Copyright (C) 1999 - 2024 Eggheads Development Team])
AC_REVISION([m4_esyscmd([../../../misc/getcommit])])

AC_PROG_FGREP
EGG_PYTHON_WITHCONFIG

python_avail="false"

if test "x$egg_enable_python" != "xno"; then
  if test "x$egg_with_python_config" = "x"; then
    # based on serial 20 https://www.gnu.org/software/autoconf-archive/ax_python.html
    AC_PATH_PROGS([python_config_bin], [python3-config python3.12-config python3.11-config python3.10-config python3.9-config python3.8-config python-config])
  else
    if test -d "$egg_with_python_config"; then
      AC_MSG_NOTICE([Checking for python-config binaries in $egg_with_python_config])
      AC_PATH_PROGS([python_config_bin], [python3-config python3.12-config python3.11-config python3.10-config python3.9-config python3.8-config python-config], [], [$egg_with_python_config])
    else
      if test -x "$egg_with_python_config"; then
        python_config_bin="$egg_with_python_config"
      fi
    fi
  fi
else
  AC_MSG_NOTICE([Python module disabled.])
fi

if test "x$python_config_bin" != "x"; then
  AC_MSG_CHECKING([whether python-config supports --embed])
  AS_IF([$python_config_bin --help | $FGREP -q -- --embed], [
    AC_SUBST([PYTHON_CONFIG_ARGS], "--embed")
    AC_MSG_RESULT([yes])
  ], [
    AC_SUBST([PYTHON_CONFIG_ARGS], "")
    AC_MSG_RESULT([no])
  ])

  AC_MSG_CHECKING([for python C flags])
  PYTHON_CFLAGS=`$python_config_bin $PYTHON_CONFIG_ARGS --cflags`
  AC_MSG_RESULT([$PYTHON_CFLAGS])
  AC_SUBST(PYTHON_CFLAGS)

  AC_MSG_CHECKING([for python LD flags])
  PYTHON_LDFLAGS=`$python_config_bin $PYTHON_CONFIG_ARGS --ldflags`
  AC_MSG_RESULT([$PYTHON_LDFLAGS])
  AC_SUBST(PYTHON_LDFLAGS)
  python_avail="true"
fi

# Check python path
AC_ARG_VAR([PYTHON_VERSION],[The installed Python
version to use, for example '2.3'. This string
will be appended to the Python interpreter
canonical name.])

PYTHON_VERSION=`echo $PYTHON_LDFLAGS | sed 's/.*-lpython\(.*[[0-9]]\).*/\1/'`

# Check for python version
AX_COMPARE_VERSION([$PYTHON_VERSION],[ge],[3.8.0], [ax_check_ver="yes"], [ax_check_ver="no"])
AC_MSG_CHECKING([python version is >= 3.8.0])
if test "x$ax_check_ver" = x"no"; then
  python_avail="false"
  AC_MSG_RESULT([no ($PYTHON_VERSION)])
  AC_MSG_WARN([Eggdrop requires python version 3.8.0 or higher])
else
  AC_MSG_RESULT([yes ($PYTHON_VERSION)])
fi

# Disable the module
if test "x$python_avail" = "xfalse"; then
  if test "x$egg_enable_python" != "xno"; then
    cat >&2 <<EOF

  There was an issue with your python installation.
  Please read python.mod/config.log for more details.
  The python module will be disabled.

EOF
  fi
  EGG_REMOVE_MOD(python)
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
