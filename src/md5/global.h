/* 
 * global.h
 *   RSAREF types and constants
 * 
 * $Id: global.h,v 1.4 1999/12/22 20:30:03 guppy Exp $
 */

#ifndef _EGG_MD5_GLOBAL_H
#define _EGG_MD5_GLOBAL_H

/* 
 * PROTOTYPES should be set to one if and only if the compiler
 * supports function argument prototyping.
 */
/* 
 * The following makes PROTOTYPES default to 1 if it has not
 * already been defined with C compiler flags.
 */
#ifndef PROTOTYPES
#  define PROTOTYPES 1
#endif

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT4 defines a four byte word */
typedef u_32bit_t UINT4;

/* 
 * PROTO_LIST is defined depending on how PROTOTYPES is defined above.
 * If using PROTOTYPES, then PROTO_LIST returns the list,
 * otherwise it returns an empty list.
 */
#if PROTOTYPES
#  define PROTO_LIST(list) list
#else
#  define PROTO_LIST(list) ()
#endif

#endif				/* _EGG_MD5_GLOBAL_H */
