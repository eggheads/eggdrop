/* 
 * acconfig.h
 *   template file autoheader uses when building config.h.in
 * 
 * $Id: acconfig.h,v 1.5 2000/01/08 22:38:19 per Exp $
 */

/* Define if modules will work on your system  */
#undef MODULES_OK

/* Define if running on hpux that supports dynamic linking  */
#undef HPUX_HACKS

/* Define if running on hpux 10.x  */
#undef HPUX10_HACKS

/* Define if running on OSF/1 platform.  */
#undef OSF1_HACKS

/* Define if running on OSF/1 platform.  */
#undef STOP_UAC

/* Define if running on sunos 4.0 *sigh*  */
#undef DLOPEN_1

/* Define if running on NeXT Step  */
#undef BORGCUBES

/* Define if running under cygwin  */
#undef CYGWIN_HACKS

/* Define if we need dlopen (for module support)  */
#undef HAVE_DLOPEN

/* Define for pre Tcl 7.5 compat  */
#undef HAVE_PRE7_5_TCL

/* Define for Tcl that has Tcl_Free() (7.5p1 and later)  */
#undef HAVE_TCL_FREE

/* Define for Tcl that has threads  */
#undef HAVE_TCL_THREADS

/* Defines the current eggdrop version */
#undef EGG_VERSION
