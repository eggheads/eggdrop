/**
 * $Id: check_tclversion.c,v 1.1 2002/08/15 00:01:47 wcc Exp $
 *
 * check_tclversion.c
 *
 * Matches the Tcl version in the headers with the Tcl version in
 * the libraries linked against. If the version mismatches the libraries
 * print an error to stdout and exit with nonzero status.
 *
 * This software is covered under the GNU General Public License (GPL)
 *
 * See the accompanying file COPYING for details before copying any part
 * of this software.
 *
 * This software is provided as-is, has no guarantee to work, and is provided
 * with no guarantee of mercantibility or fitness. Basically, if you use it
 * and it doesn't do what you expect it to do, it's *your* fault, not mine.
 * 
 * Use at your own risk (although it's severely doubtful this code could be
 * damaging in any way, shape, or form).
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tcl.h>

inline void nrerror( int rstatus, const char *message );

int main( int argc, const char *argv[] )
{
   Tcl_Interp *inter;

   char       *buf;
   int         res;

   res = 0;
   inter = Tcl_CreateInterp();

   if ( inter )
   {
     if ( Tcl_Eval( inter, "info patchlevel" ) == TCL_OK )
     {
        buf  = strdup( inter->result );

#ifdef DEBUG
        printf( "Tcl patchlevel: (hdr) %s (lib) %s\n", 
                TCL_PATCH_LEVEL,    buf );
#endif

        if ( !strcmp( TCL_PATCH_LEVEL, buf ) )
           res = 1;
        else 
           res = 0;

        free( buf );
     }
     else if ( Tcl_Eval( inter, "info tclversion" ) == TCL_OK )
     {
        buf  = strdup( inter->result );

        if ( !strcmp( TCL_VERSION, buf ) )
           res = 1;
        else 
           res = 0;

        free( buf );
     }
     else
        nrerror( 2, "Tcl library broken on eval \"info patchlevel\"\n" );
   }
   else
   {
      nrerror( 4, "Cannot create interpreter\n" );
   }

   Tcl_DeleteInterp( inter );

   if ( res )
      nrerror( 0, "Version matches OK\n" );  
   else
      nrerror( 1, "Version mismatch\n" );

   return 0;
}

inline void nrerror( int rstatus, const char *message )
{
   fprintf( stderr, message );
   exit( rstatus );
}

/*
 * $Log: check_tclversion.c,v $
 * Revision 1.1  2002/08/15 00:01:47  wcc
 * Forgot cvs add on last commit.
 *
 * Revision 1.1  2002/08/08 09:22:24  khudson
 * Initial revision
 *
 *
 */
