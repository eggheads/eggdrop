dnl configure.ac: this file is processed by autoconf to produce ./configure.

AC_PREREQ(2.71)

sinclude(../eggmod.m4)
builtin(include,../../../m4/python.m4)

AC_INIT([Eggdrop Python Module],[1.9.3],[bugs@eggheads.org])

AC_CONFIG_SRCDIR(python.c)
AC_CONFIG_AUX_DIR(../../../misc)

AC_COPYRIGHT([Copyright (C) 1999 - 2022 Eggheads Development Team])
AC_REVISION([m4_esyscmd([../../../misc/getcommit])])

AC_PROG_FGREP
EGG_PYTHON_ENABLE
EGG_PYTHON_WITHCONFIG

python_avail="false"

if test "x$egg_enable_python" != "xno"; then
  if test "x$egg_with_python_config" = "x"; then
    AC_PATH_PROGS([python_config_bin], [python3-config python-config])
  else
    if test -d "$egg_with_python_config"; then
      AC_MSG_NOTICE([Checking for python-config binaries in $egg_with_python_config])
      AC_PATH_PROGS([python_config_bin], [python3-config python-config], [], [$egg_with_python_config])
    else
      if test -x "$egg_with_python_config"; then
        python_config_bin="$egg_with_python_config"
      else
        AC_MSG_WARN([Specified --with-python-config=$egg_with_python_config does not exist.])
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

# Disable the module
if test "x$python_avail" = "xfalse"; then
  if test "x$egg_enable_python" != "xno"; then
    cat >&2 <<EOF
configure: warning:

  Your system does not provide a working python-config binary.
  The python module will therefore be disabled.

EOF
  fi
  EGG_REMOVE_MOD(python)
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
