dnl eggmod.m4
dnl
dnl $Id: eggmod.m4,v 1.8 2004/07/25 11:17:34 wcc Exp $

dnl EGG_REMOVE_MOD(MODULE-NAME)
dnl
dnl Removes a module from the list of modules to be compiled.
define(EGG_REMOVE_MOD,
[
  ${srcdir}/../../../misc/modconfig -q --top_srcdir=${srcdir}/../../.. --bindir=../../.. del $1
])
