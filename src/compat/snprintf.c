/*
 * snprintf.c - a portable implementation of snprintf and vsnprintf
 *
 * $Id: snprintf.c,v 1.1 2000/03/22 00:42:57 fabian Exp $
 */
/* Portions Copyright (C) 2000  Eggheads
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "main.h"
#include "snprintf.h"


#ifndef HAVE_VSNPRINTF

/* NOTE
 *   The following vsnprintf() routine is a slightly modified version
 *   of the original snprintf() from Mark Martinec.
 */

/*
 * AUTHOR
 *   Mark Martinec <mark.martinec@ijs.si>, April 1999
 *   Copyright: Mark Martinec
 *
 * TERMS AND CONDITIONS
 *   for copying, distribution and modification, NO WARRANTY:
 *   GNU GENERAL PUBLIC LICENSE, Version 2 or later
 *   (see file LICENSE in the distribution kit)
 *
 * FEATURES
 * - careful adherence to specs regarding flags, field width and precision;
 * - good performance for large string handling (large format, large
 *   argument or large paddings). Performance is similar to system's sprintf
 *   and in several cases significantly better (make sure you compile with
 *   optimizations turned on, tell the compiler the code is strict ANSI
 *   if necessary to give it more freedom for optimizations);
 * - written in standard ANSI C - requires an ANSI C compiler.
 *
 * SUPPORTED FORMATS AND DATA TYPES
 *
 * This snprintf only supports format specifiers:
 * s, c, d, o, u, x, X, p  (and synonyms: i, D, U, O - see below)
 * with flags: '-', '+', ' ', '0' and '#'.
 * An asterisk is supported for field width as well as precision.
 *
 * Data type modifiers 'h' (short int), 'l' (long int)
 * and 'll' (long long int) are supported.
 * NOTE:
 *   If macro SNPRINTF_LONGLONG_SUPPORT is not defined (default) the
 *   data type modifier 'll' is recognized but treated the same as 'l',
 *   which may cause argument value truncation! Defining
 *   SNPRINTF_LONGLONG_SUPPORT requires that your system's sprintf also
 *   handles data type modifier 'll'. long long int is a language
 *   extension which may not be portable.
 *
 * Conversion of numeric data (formats d, o, u, x, X, p) with data type
 * modifiers (none or h, l, ll) is left to the system routine sprintf,
 * but all handling of flags, field width and precision as well as c and
 * s formats is done very carefully by this portable routine. If a string
 * precision (truncation) is specified (e.g. %.8s) it is guaranteed the
 * string beyond the specified precision will not be referenced.
 *
 * Data type modifiers h, l and ll are ignored for c and s formats (data
 * types wint_t and wchar_t are not supported).
 *
 * The following common synonyms for conversion characters are supported:
 *   - i is a synonym for d
 *   - D is a synonym for ld, explicit data type modifiers are ignored
 *   - U is a synonym for lu, explicit data type modifiers are ignored
 *   - O is a synonym for lo, explicit data type modifiers are ignored
 *
 * The following is specifically not supported:
 *   - flag ' (thousands' grouping character) is recognized but ignored
 *   - numeric formats: f, e, E, g, G and synonym F
 *   - data type modifier 'L' (long double) and 'q' (quad - use 'll' instead)
 *   - wide character/string formats: C, lc, S, ls
 *   - writeback of converted string length: conversion character n
 *   - the n$ specification for direct reference to n-th argument
 *   - locales
 *
 * AVAILABILITY
 *   http://www.ijs.si/software/snprintf/
 *
 * REVISION HISTORY
 *    Apr 1999	V0.9  Mark Martinec
 *		- initial version, some modifications after comparing printf
 *		  man pages for Digital Unix 4.0, Solaris 2.6 and HPUX 10,
 *		  and checking how Perl handles sprintf (differently!);
 *  9 Apr 1999	V1.0  Mark Martinec <mark.martinec@ijs.si>
 *		- added main test program, fixed remaining inconsistencies,
 *                added optional (long long int) support;
 * 12 Apr 1999  V1.1  Mark Martinec <mark.martinec@ijs.si>
 *		- support the 'p' format (pointer to void);
 *		- if a string precision is specified
 *		  make sure the string beyond the specified precision
 *                will not be referenced (e.g. by strlen);
 * 13 Apr 1999  V1.2  Mark Martinec <mark.martinec@ijs.si>
 *		- support synonyms %D=%ld, %U=%lu, %O=%lo;
 *		- speed up the case of long format string with few conversions;
 * 30 Jun 1999  V1.3  Mark Martinec <mark.martinec@ijs.si>
 *		- fixed runaway loop (eventually crashing when str_l wraps
 *		  beyond 2*31) while copying format string without
 *		  conversion specifiers to a buffer that is too short
 *		  (thanks to Edwin Young <edwiny@autonomy.com> for
 *		  spoting the problem)
 *		- added macros PORTABLE_SNPRINTF_VERSION_(MAJOR|MINOR)
 *		  to snprintf.h
 */


/* Define the following macros if desired:
 *   SOLARIS_COMPATIBLE, SOLARIS_BUG_COMPATIBLE,
 *   HPUX_COMPATIBLE, HPUX_BUG_COMPATIBLE,
 *   DIGITAL_UNIX_COMPATIBLE, DIGITAL_UNIX_BUG_COMPATIBLE,
 *   PERL_COMPATIBLE, PERL_BUG_COMPATIBLE,
 *
 * - For portable applications it is best not to rely on peculiarities
 *   of a given implementation so it may be best not to define any
 *   of the macros that select compatibility and to avoid features
 *   that vary among the systems.
 *
 * - Selecting compatibility with more than one operating system
 *   is not strictly forbidden but is not recommended.
 *
 * - 'x'_BUG_COMPATIBLE implies 'x'_COMPATIBLE .
 *
 * - 'x'_COMPATIBLE refers to (and enables) a behaviour that is
 *   documented in a sprintf man page on a given operating system
 *   and actually adhered to by the system's sprintf (but not on
 *   most other operating systems). It may also refer to and enable
 *   a behaviour that is declared 'undefined' or 'implementation specific'
 *   in the man page but a given implementation behaves predictably
 *   in a certain way.
 *
 * - 'x'_BUG_COMPATIBLE refers to (and enables) a behaviour of system's sprintf
 *   that contradicts the sprintf man page on the same operating system.
 */

#if defined(SOLARIS_BUG_COMPATIBLE) && !defined(SOLARIS_COMPATIBLE)
#define SOLARIS_COMPATIBLE
#endif

#if defined(HPUX_BUG_COMPATIBLE) && !defined(HPUX_COMPATIBLE)
#define HPUX_COMPATIBLE
#endif

#if defined(DIGITAL_UNIX_BUG_COMPATIBLE) && !defined(DIGITAL_UNIX_COMPATIBLE)
#define DIGITAL_UNIX_COMPATIBLE
#endif

#if defined(PERL_BUG_COMPATIBLE) && !defined(PERL_COMPATIBLE)
#define PERL_COMPATIBLE
#endif

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef isdigit
#undef isdigit
#endif
#define isdigit(c) ((c) >= '0' && (c) <= '9')

/* prototype */
int egg_vsnprintf(char *str, size_t str_m, const char *fmt, va_list ap);

/* declaration */
int egg_vsnprintf(char *str, size_t str_m, const char *fmt, va_list ap)
{
  size_t str_l = 0;
  const char *p = fmt;

  if (str_m < 1) return -1;

  if (!p) p = "";
  while (*p) {
    if (*p != '%') {
   /* if (str_l < str_m) str[str_l++] = *p++;    -- this would be sufficient */
   /* but the following code achieves better performance for cases
    * where format string is long and contains few conversions */
      const char *q = strchr(p+1,'%');
      int n = !q ? strlen(p) : (q-p);
      int avail = (int)(str_m-str_l);
      if (avail > 0) {
        register int k; register char *r; register const char* p1;
        for (p1=p, r=str+str_l, k=(n>avail?avail:n); k>0; k--) *r++ = *p1++;
      }
      p += n; str_l += n;
    } else {
      const char *starting_p;
      int min_field_width = 0, precision = 0;
      int zero_padding = 0, precision_specified = 0, justify_left = 0;
      int alternative_form = 0, force_sign = 0;
      int space_for_positive = 1; /* If both the ' ' and '+' flags appear,
                                     the ' ' flag should be ignored. */
      char data_type_modifier = '\0';      /* allowed valued: \0, h, l, L, p */
      char tmp[32];/* temporary buffer for simple numeric->string conversion */

      const char *str_arg = 0;/* string address in case of string arguments  */
      int str_arg_l;  /* natural field width of arg without padding and sign */

      long int long_arg;  /* long int argument value - always defined
        in case of numeric arguments, regardless of data type modifiers.
        In case of data type modifier 'll' the value is stored in long_long_arg
        and only the sign of long_arg is guaranteed to be correct */
      void *ptr_arg; /* pointer argument value - only defined for p format   */
      int int_arg;   /* int argument value - only defined if no h or l modif.*/
      int number_of_zeros_to_pad = 0;
      int zero_padding_insertion_ind = 0;
      char fmt_spec = '\0';            /* current format specifier character */

      starting_p = p; p++;  /* skip '%' */
   /* parse flags */
      while (*p == '0' || *p == '-' || *p == '+' ||
             *p == ' ' || *p == '#' || *p == '\'') {
        switch (*p) {
        case '0': zero_padding = 1; break;
        case '-': justify_left = 1; break;
        case '+': force_sign = 1; space_for_positive = 0; break;
        case ' ': force_sign = 1;
     /* If both the ' ' and '+' flags appear, the ' ' flag should be ignored */
#ifdef PERL_COMPATIBLE
     /* ... but in Perl the last of ' ' and '+' applies */
                  space_for_positive = 1;
#endif
                  break;
        case '#': alternative_form = 1; break;
        case '\'': break;
        }
        p++;
      }
   /* If the '0' and '-' flags both appear, the '0' flag should be ignored. */

   /* parse field width */
      if (*p == '*') {
        p++; min_field_width = va_arg(ap, int);
        if (min_field_width < 0)
          { min_field_width = -min_field_width; justify_left = 1; }
      } else if (isdigit((int)(*p))) {
        min_field_width = *p++ - '0';
        while (isdigit((int)(*p)))
          min_field_width = 10*min_field_width + (*p++ - '0');
      }
   /* parse precision */
      if (*p == '.') {
        p++; precision_specified = 1;
        if (*p == '*') {
          p++; precision = va_arg(ap, int);
          if (precision < 0) {
            precision_specified = 0; precision = 0;
         /* NOTE:
          *   Solaris 2.6 man page claims that in this case the precision
          *   should be set to 0.  Digital Unix 4.0 and HPUX 10 man page
          *   claim that this case should be treated as unspecified precision,
          *   which is what we do here.
          */
          }
        } else if (isdigit((int)(*p))) {
          precision = *p++ - '0';
          while (isdigit((int)(*p))) precision = 10*precision + (*p++ - '0');
        }
      }
   /* parse 'h', 'l' and 'll' data type modifiers */
      if (*p == 'h' || *p == 'l') {
        data_type_modifier = *p; p++;
        if (data_type_modifier == 'l' && *p == 'l') {/* double l = long long */
          data_type_modifier = 'l';                /* treat it as single 'l' */
          p++;
        }
      }
      fmt_spec = *p;
   /* common synonyms: */
      switch (fmt_spec) {
      case 'i': fmt_spec = 'd'; break;
      case 'D': fmt_spec = 'd'; data_type_modifier = 'l'; break;
      case 'U': fmt_spec = 'u'; data_type_modifier = 'l'; break;
      case 'O': fmt_spec = 'o'; data_type_modifier = 'l'; break;
      default: break;
      }
   /* get parameter value, do initial processing */
      switch (fmt_spec) {
      case '%': /* % behaves similar to 's' regarding flags and field widths */
      case 'c': /* c behaves similar to 's' regarding flags and field widths */
      case 's':
        data_type_modifier = '\0';       /* wint_t and wchar_t not supported */
     /* the result of zero padding flag with non-numeric format is undefined */
     /* Solaris and HPUX 10 does zero padding in this case, Digital Unix not */
#ifdef DIGITAL_UNIX_COMPATIBLE
        zero_padding = 0;        /* turn zero padding off for string formats */
#endif
        str_arg_l = 1;
        switch (fmt_spec) {
        case '%':
          str_arg = p; break;
        case 'c':
          { int j = va_arg(ap, int); str_arg = (const char*) &j; }
          break;
        case 's':
          str_arg = va_arg(ap, const char *);
          if (!str_arg) str_arg_l = 0;
       /* make sure not to address string beyond the specified precision !!! */
          else if (!precision_specified) str_arg_l = strlen(str_arg);
       /* truncate string if necessary as requested by precision */
          else if (precision <= 0) str_arg_l = 0;
          else {
            const char *q = memchr(str_arg,'\0',(size_t)precision);
            str_arg_l = !q ? precision : (q-str_arg);
          }
          break;
        default: break;
        }
        break;
      case 'd': case 'o': case 'u': case 'x': case 'X': case 'p':
        long_arg = 0; int_arg = 0; ptr_arg = NULL;
        if (fmt_spec == 'p') {
        /* HPUX 10: An l, h, ll or L before any other conversion character
         *   (other than d, i, o, u, x, or X) is ignored.
         * Digital Unix:
         *   not specified, but seems to behave as HPUX does.
         * Solaris: If an h, l, or L appears before any other conversion
         *   specifier (other than d, i, o, u, x, or X), the behavior
         *   is undefined. (Actually %hp converts only 16-bits of address
         *   and %llp treats address as 64-bit data which is incompatible
         *   with (void *) argument on a 32-bit system).
         */
#ifdef SOLARIS_COMPATIBLE
#  ifdef SOLARIS_BUG_COMPATIBLE
          /* keep data type modifiers even if it represents 'll' */
#  else
          if (data_type_modifier == '2') data_type_modifier = '\0';
#  endif
#else
          data_type_modifier = '\0';
#endif
          ptr_arg = va_arg(ap, void *); long_arg = !ptr_arg ? 0 : 1;
        } else {
          switch (data_type_modifier) {
          case '\0':
          case 'h':
         /* It is non-portable to specify a second argument of char or short
          * to va_arg, because arguments seen by the called function
          * are not char or short.  C converts char and short arguments
          * to int before passing them to a function.
          */
            int_arg = va_arg(ap, int); long_arg = int_arg; break;
          case 'l':
            long_arg = va_arg(ap, long int); break;
          }
        }
        str_arg = tmp; str_arg_l = 0;
     /* NOTE:
      *   For d, i, o, u, x, and X conversions, if precision is specified,
      *   the '0' flag should be ignored. This is so with Solaris 2.6,
      *   Digital UNIX 4.0 and HPUX 10;  but not with Perl.
      */
#ifndef PERL_COMPATIBLE
        if (precision_specified) zero_padding = 0;
#endif
        if (fmt_spec == 'd') {
          if (force_sign && long_arg >= 0)
            tmp[str_arg_l++] = space_for_positive ? ' ' : '+';
         /* leave negative numbers for sprintf to handle,
            to avoid handling tricky cases like (short int)(-32768) */
        } else if (alternative_form) {
          if (long_arg != 0 && (fmt_spec == 'x' || fmt_spec == 'X') )
            { tmp[str_arg_l++] = '0'; tmp[str_arg_l++] = fmt_spec; }
#ifdef HPUX_COMPATIBLE
          else if (fmt_spec == 'p'
         /* HPUX 10: for an alternative form of p conversion,
          *          a nonzero result is prefixed by 0x. */
#ifndef HPUX_BUG_COMPATIBLE
         /* Actually it uses 0x prefix even for a zero value. */
                   && long_arg != 0
#endif
                  ) { tmp[str_arg_l++] = '0'; tmp[str_arg_l++] = 'x'; }
#endif
        }
        zero_padding_insertion_ind = str_arg_l;
        if (!precision_specified) precision = 1;   /* default precision is 1 */
        if (precision == 0 && long_arg == 0
#ifdef HPUX_BUG_COMPATIBLE
            && fmt_spec != 'p'
         /* HPUX 10 man page claims: With conversion character p the result of
          * converting a zero value with a precision of zero is a null string.
          * Actually it returns all zeroes. */
#endif
        ) {  /* converted to null string */  }
        else {
          char f[5]; int f_l = 0;
          f[f_l++] = '%';
          if (!data_type_modifier) { }
          else if (data_type_modifier=='2') { f[f_l++] = 'l'; f[f_l++] = 'l'; }
          else f[f_l++] = data_type_modifier;
          f[f_l++] = fmt_spec; f[f_l++] = '\0';
          if (fmt_spec == 'p') str_arg_l+=sprintf(tmp+str_arg_l, f, ptr_arg);
          else {
            switch (data_type_modifier) {
            case '\0':
            case 'h': str_arg_l+=sprintf(tmp+str_arg_l, f, int_arg);  break;
            case 'l': str_arg_l+=sprintf(tmp+str_arg_l, f, long_arg); break;
            }
          }
          if (zero_padding_insertion_ind < str_arg_l &&
              tmp[zero_padding_insertion_ind] == '-')
            zero_padding_insertion_ind++;
        }
        { int num_of_digits = str_arg_l - zero_padding_insertion_ind;
          if (alternative_form && fmt_spec == 'o'
#ifdef HPUX_COMPATIBLE                                  /* ("%#.o",0) -> ""  */
              && (str_arg_l > 0)
#endif
#ifdef DIGITAL_UNIX_BUG_COMPATIBLE                      /* ("%#o",0) -> "00" */
#else
              && !(zero_padding_insertion_ind < str_arg_l
                   && tmp[zero_padding_insertion_ind] == '0')
#endif
          ) {      /* assure leading zero for alternative-form octal numbers */
            if (!precision_specified || precision < num_of_digits+1)
              { precision = num_of_digits+1; precision_specified = 1; }
          }
       /* zero padding to specified precision? */
          if (num_of_digits < precision) 
            number_of_zeros_to_pad = precision - num_of_digits;
        }
     /* zero padding to specified minimal field width? */
        if (!justify_left && zero_padding) {
          int n = min_field_width - (str_arg_l+number_of_zeros_to_pad);
          if (n > 0) number_of_zeros_to_pad += n;
        }
        break;
      default:  /* unrecognized format, keep format string unchanged */
        zero_padding = 0;   /* turn zero padding off for non-numeric formats */
#ifndef DIGITAL_UNIX_COMPATIBLE
        justify_left = 1; min_field_width = 0;                /* reset flags */
#endif
#ifdef PERL_COMPATIBLE
     /* keep the entire format string unchanged */
        str_arg = starting_p; str_arg_l = p - starting_p;
#else
     /* discard the unrecognized format, just keep the unrecognized fmt char */
        str_arg = p; str_arg_l = 0;
#endif
        if (*p) str_arg_l++;  /* include invalid fmt specifier if not at EOS */
        break;
      }
      if (*p) p++;          /* step over the just processed format specifier */
   /* insert padding to the left as requested by min_field_width */
      if (!justify_left) {                /* left padding with blank or zero */
        int n = min_field_width - (str_arg_l+number_of_zeros_to_pad);
        if (n > 0) {
          int avail = (int)(str_m-str_l);
          if (avail > 0) {      /* memset(str+str_l, zp, (n>avail?avail:n)); */
            const char zp = (zero_padding ? '0' : ' ');
            register int k; register char *r;
            for (r=str+str_l, k=(n>avail?avail:n); k>0; k--) *r++ = zp;
          }
          str_l += n;
        }
      }
   /* zero padding as requested by the precision for numeric formats requred?*/
      if (number_of_zeros_to_pad <= 0) {
     /* will not copy first part of numeric here,   *
      * force it to be copied later in its entirety */
        zero_padding_insertion_ind = 0;
      } else {
     /* insert first part of numerics (sign or '0x') before zero padding */
        int n = zero_padding_insertion_ind;
        if (n > 0) {
          int avail = (int)(str_m-str_l);
          if (avail > 0) memcpy(str+str_l, str_arg, (size_t)(n>avail?avail:n));
          str_l += n;
        }
     /* insert zero padding as requested by the precision */
        n = number_of_zeros_to_pad;
        if (n > 0) {
          int avail = (int)(str_m-str_l);
          if (avail > 0) {     /* memset(str+str_l, '0', (n>avail?avail:n)); */
            register int k; register char *r;
            for (r=str+str_l, k=(n>avail?avail:n); k>0; k--) *r++ = '0';
          }
          str_l += n;
        }
      }
   /* insert formatted string (or unmodified format for unknown formats) */
      { int n = str_arg_l - zero_padding_insertion_ind;
        if (n > 0) {
          int avail = (int)(str_m-str_l);
          if (avail > 0) memcpy(str+str_l, str_arg+zero_padding_insertion_ind,
                                (size_t)(n>avail ? avail : n) );
          str_l += n;
        }
      }
   /* insert right padding */
      if (justify_left) {          /* right blank padding to the field width */
        int n = min_field_width - (str_arg_l+number_of_zeros_to_pad);
        if (n > 0) {
          int avail = (int)(str_m-str_l);
          if (avail > 0) {     /* memset(str+str_l, ' ', (n>avail?avail:n)); */
            register int k; register char *r;
            for (r=str+str_l, k=(n>avail?avail:n); k>0; k--) *r++ = ' ';
          }
          str_l += n;
        }
      }
    }
  }
  if (str_m > 0)  /* make sure the string is null-terminated
                     even at the expense of overwriting the last character */
    str[str_l <= str_m-1 ? str_l : str_m-1] = '\0';
  return str_l;  /* return the number of characters formatted
                    (excluding trailing null character),
                    that is, the number of characters that would have been
                    written to the buffer if it were large enough */
}

#endif /* HAVE_VSNPRINTF */


#ifndef HAVE_SNPRINTF

/* int egg_snprintf(char *str, size_t str_m, const char *fmt, ...)
 */
int egg_snprintf EGG_VARARGS_DEF(char *, arg1)
{
  va_list	 ap;
  register int	 ret;
  char		*str;
  size_t	 str_m;
  const char	*fmt;

  str   = EGG_VARARGS_START(char *, arg1, ap);
  str_m = va_arg(ap, size_t);
  fmt   = va_arg(ap, const char *);

  ret = egg_vsnprintf(str, str_m, fmt, ap);
  va_end(ap);
  return ret;
}

#endif /* !HAVE_SNPRINTF */
