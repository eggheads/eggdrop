/*
 * match.c
 *   wildcard matching functions
 *
 * $Id: match.c,v 1.6 2002/12/26 02:21:53 wcc Exp $
 *
 * Once this code was working, I added support for % so that I could
 * use the same code both in Eggdrop and in my IrcII client.
 * Pleased with this, I added the option of a fourth wildcard, ~,
 * which matches varying amounts of whitespace (at LEAST one space,
 * though, for sanity reasons).
 * 
 * This code would not have been possible without the prior work and
 * suggestions of various sourced.  Special thanks to Robey for
 * all his time/help tracking down bugs and his ever-helpful advice.
 * 
 * 04/09:  Fixed the "*\*" against "*a" bug (caused an endless loop)
 * 
 *   Chris Fuller  (aka Fred1@IRC & Fwitz@IRC)
 *     crf@cfox.bchs.uh.edu
 * 
 * I hereby release this code into the public domain
 *
 */
#include "main.h"

#define QUOTE '\\' /* quoting character (overrides wildcards) */
#define WILDS '*'  /* matches 0 or more characters (including spaces) */
#define WILDP '%'  /* matches 0 or more non-space characters */
#define WILDQ '?'  /* matches ecactly one character */
#define WILDT '~'  /* matches 1 or more spaces */

#define NOMATCH 0
#define MATCH (match+sofar)
#define PERMATCH (match+saved+sofar)

int _wild_match_per(register unsigned char *m, register unsigned char *n)
{
  unsigned char *ma = m, *lsm = 0, *lsn = 0, *lpm = 0, *lpn = 0;
  int match = 1, saved = 0;
  register unsigned int sofar = 0;

#ifdef WILDT
  int space;
#endif

  /* null strings should never match */
  if ((m == 0) || (n == 0) || (!*n))
    return NOMATCH;

  while (*n) {
#ifdef WILDT
    if (*m == WILDT) {		/* Match >=1 space */
      space = 0;		/* Don't need any spaces */
      do {
	m++;
	space++;
      }				/* Tally 1 more space ... */
      while ((*m == WILDT) || (*m == ' '));	/*  for each space or ~ */
      sofar += space;		/* Each counts as exact */
      while (*n == ' ') {
	n++;
	space--;
      }				/* Do we have enough? */
      if (space <= 0)
	continue;		/* Had enough spaces! */
    }
    /* Do the fallback       */
    else {
#endif
      switch (*m) {
      case 0:
	do
	  m--;			/* Search backwards */
	while ((m > ma) && (*m == '?'));	/* For first non-? char */
	if ((m > ma) ? ((*m == '*') && (m[-1] != QUOTE)) : (*m == '*'))
	  return PERMATCH;		/* nonquoted * = match */
	break;
      case WILDP:
	while (*(++m) == WILDP);	/* Zap redundant %s */
	if (*m != WILDS) {	/* Don't both if next=* */
	  if (*n != ' ') {	/* WILDS can't match ' ' */
	    lpm = m;
	    lpn = n;		/* Save '%' fallback spot */
	    saved += sofar;
	    sofar = 0;		/* And save tally count */
	  }
	  continue;		/* Done with '%' */
	}
	/* FALL THROUGH */
      case WILDS:
	do
	  m++;			/* Zap redundant wilds */
	while ((*m == WILDS) || (*m == WILDP));
	lsm = m;
	lsn = n;
	lpm = 0;		/* Save '*' fallback spot */
	match += (saved + sofar);	/* Save tally count */
	saved = sofar = 0;
	continue;		/* Done with '*' */
      case WILDQ:
	m++;
	n++;
	continue;		/* Match one char */
      case QUOTE:
	m++;			/* Handle quoting */
      }
      if (rfc_toupper(*m) == rfc_toupper(*n)) {		/* If matching */
	m++;
	n++;
	sofar++;
	continue;		/* Tally the match */
      }
#ifdef WILDT
    }
#endif
    if (lpm) {			/* Try to fallback on '%' */
      n = ++lpn;
      m = lpm;
      sofar = 0;		/* Restore position */
      if ((*n | 32) == 32)
	lpm = 0;		/* Can't match 0 or ' ' */
      continue;			/* Next char, please */
    }
    if (lsm) {			/* Try to fallback on '*' */
      n = ++lsn;
      m = lsm;			/* Restore position */
      saved = sofar = 0;
      continue;			/* Next char, please */
    }
    return NOMATCH;		/* No fallbacks=No match */
  }
  while ((*m == WILDS) || (*m == WILDP))
    m++;			/* Zap leftover %s & *s */
  return (*m) ? NOMATCH : PERMATCH;	/* End of both = match */
}

int _wild_match(register unsigned char *m, register unsigned char *n)
{
  unsigned char *ma = m, *na = n, *lsm = 0, *lsn = 0;
  int match = 1;
  register int sofar = 0;

  /* null strings should never match */
  if ((ma == 0) || (na == 0) || (!*ma) || (!*na))
    return NOMATCH;
  /* find the end of each string */
  while (*(++m));
    m--;
  while (*(++n));
    n--;

  while (n >= na) {
    switch (*m) {
    case WILDS:  /* Matches anything */
      do
        m--;  /* Zap redundant wilds */
      while ((m >= ma) && (*m == WILDS));
      lsm = m;
      lsn = n;
      match += sofar;
      sofar = 0;  /* Update fallback pos */
      continue;  /* Next char, please */
    case WILDQ:
      m--;
      n--;
      continue;  /* '?' always matches */
    }
    if (rfc_toupper(*m) == rfc_toupper(*n)) {  /* If matching char */
      m--;
      n--;
      sofar++;  /* Tally the match */
      continue;  /* Next char, please */
    }
    if (lsm) {  /* To to fallback on '*' */
      n = --lsn;
      m = lsm;
      if (n < na)
	lsm = 0;  /* Rewind to saved pos */
      sofar = 0;
      continue;  /* Next char, please */
    }
    return NOMATCH;  /* No fallback=No match */
  }
  while ((m >= ma) && (*m == WILDS))
    m--;  /* Zap leftover %s & *s */
  return (m >= ma) ? NOMATCH : MATCH;  /* Start of both = match */
}
