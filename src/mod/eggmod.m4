dnl eggmod.m4

dnl EGG_REMOVE_MOD(MODULE-NAME)
dnl
dnl Removes a module from the list of modules to be compiled.
define(EGG_REMOVE_MOD,
[
  ${srcdir}/../../../misc/modconfig -q --top_srcdir=${srcdir}/../../.. --bindir=../../.. del $1
])
