/* 
 * md5.h
 *   header file for md5c.c
 * 
 * $Id: md5.h,v 1.3 2000/09/09 11:39:10 fabian Exp $
 */
/* 
 * Copyright (C) 1991, 1992  RSA Data Security, Inc.
 * Created 1991.
 * All rights reserved.
 * 
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 * 
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD5 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 * 
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 * 
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

#ifndef _EGG_MD5_MD5_H
#define _EGG_MD5_MD5_H

/* POINTER defines a generic pointer type */
typedef char *POINTER;

/* MD5 context. */
typedef struct {
  u_32bit_t state[4];                                   /* state (ABCD) */
  u_32bit_t count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
void MD5Final(unsigned char[16], MD5_CTX *);

#endif				/* _EGG_MD5_MD5_H */
