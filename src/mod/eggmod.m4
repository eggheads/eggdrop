dnl eggmod.m4
dnl   macros eggdrop modules should use instead of the original autoconf
dnl   versions.
dnl
dnl $Id: eggmod.m4,v 1.1 2000/02/25 21:51:30 fabian Exp $

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

# This tells the code in ../eggmod.sh to search for the file `$1'.
# That way we can make sure we're started from the correct directory.
ac_egg_uniquefile=$1
. ../eggmod.sh

# Standard autoconf commands/tests follow below.])
