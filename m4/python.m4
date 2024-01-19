dnl python.m4 -- Autoconf macros to compile python.mod
dnl
dnl Copyright (c) 2022 - 2024 Eggheads Development Team
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

dnl EGG_PYTHON_ENABLE
dnl
AC_DEFUN([EGG_PYTHON_ENABLE],
[
  AC_MSG_CHECKING([whether to compile the Python module])
  AC_ARG_ENABLE(python,
    [  --enable-python         enable Python support (autodetect)],
    [egg_enable_python="$enableval"])
  AC_ARG_ENABLE(python,
    [  --disable-python        disable Python support], [egg_enable_python="$enableval"],
    [egg_enable_python="autodetect"])

  AC_MSG_RESULT([$egg_enable_python])
  AC_SUBST(egg_enable_python)
])


dnl EGG_PYTHON_WITHCONFIG
dnl
AC_DEFUN(EGG_PYTHON_WITHCONFIG,
[
  AC_ARG_WITH(python-config, [  --with-python-config=PATH      Path to python-config], [
    if test "x$enable_python" != "xno"; then
      if test -d "$withval" || test -x "$withval"; then
        egg_with_python_config="$withval"
      else
        egg_with_python_config="no"
        AC_MSG_WARN([Invalid path to python-config. $withval is not a directory and not an executable.])
      fi
    fi
  ])
  AC_SUBST(egg_with_python_config)
])

