dnl eggmod.m4
dnl
dnl $Id: eggmod.m4,v 1.7 2004/06/15 07:20:55 wcc Exp $

dnl EGG_REMOVE_MOD(MODULE-NAME)
dnl
dnl Removes a module from the list of modules to be compiled.
define(EGG_REMOVE_MOD,
[
  ${srcdir}/../../../misc/modconfig -q --top_srcdir=${srcdir}/../../.. --bindir=../../.. del $1
])

dnl EGG_INIT(UNIQUE-SOURCE-FILE)
define(EGG_INIT,
[
  # This tells the code in eggmod.sh to search for the file `$1'.
  # That way we can make sure we're started from the correct directory.
  ac_egg_uniquefile=$1

  # Scan for out source directory
  ac_egg_srcdir=.
  for i in [$]*; do
    case "[$]{i}" in
      --srcdir=*)
        ac_egg_srcdir=`echo [$]{i} | sed -e 's/^--srcdir=//'`
      ;;
    esac
  done

  if test -r [$]{ac_egg_srcdir}/../eggmod.sh; then
    [$]{ac_egg_srcdir}/../eggmod.sh
  else
    echo "[$]0: error: failed to locate eggmod.sh in [$]{ac_egg_srcdir}/.." >&2
    exit 1
  fi
])
