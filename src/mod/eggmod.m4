dnl eggmod.m4
dnl   macros eggdrop modules should use instead of the original autoconf
dnl   versions.
dnl
dnl $Id: eggmod.m4,v 1.5 2000/03/23 23:17:56 fabian Exp $

dnl
dnl EGG_REMOVE_MOD(MODULE-NAME)
dnl
define(EGG_REMOVE_MOD,
[${srcdir}/../../../misc/modconfig -q --top_srcdir=${srcdir}/../../.. --bindir=../../.. del $1])

dnl
dnl EGG_INIT(UNIQUE-SOURCE-FILE)
dnl
define(EGG_INIT,
[AC_INIT($1)
## SPLIT
# configure  --  Special configure variant for eggdrop modules based on the
#                GNU autoconf scripts.
#
# Automatically created by src/mod/eggautoconf from `configure.in'
#

echo "running in eggdrop mode."
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
	. [$]{ac_egg_srcdir}/../eggmod.sh
else
	echo "[$]0: error: failed to locate eggmod.sh in [$]{ac_egg_srcdir}/.." >&2
	exit 1
fi

# Standard autoconf commands/tests follow below.])
