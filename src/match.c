/* ===================================================================
 * 
 * match.c -- wildcard matching functions
 *   (rename to reg.c for ircII)
 *
 * Once this code was working, I added support for % so that I could use
 *  the same code both in Eggdrop and in my IrcII client.  Pleased
 *  with this, I added the option of a fourth wildcard, ~, which
 *  matches varying amounts of whitespace (at LEAST one space, though,
 *  for sanity reasons).
 * This code would not have been possible without the prior work and
 *  suggestions of various sourced.  Special thanks to Robey for
 *  all his time/help tracking down bugs and his ever-helpful advice.
 * 
 * 04/09:  Fixed the "*\*" against "*a" bug (caused an endless loop)
 *
 *   Chris Fuller  (aka Fred1@IRC & Fwitz@IRC)
 *     crf@cfox.bchs.uh.edu
 * 
 * I hereby release this code into the public domain
 *
 * =================================================================== */

/* This will get us around most of the mess and replace the chunk that
 * was removed from the middle of this file.   --+ Dagmar */
/* You'll also want to grab the rfc1459.c file or change all rfc_*()
 * calls to the standard library call to make this work with ircII
 * derivatives now. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Remove the next line to use this in IrcII */
#define EGGDROP

/* ===================================================================
 * Best to leave stuff after this point alone, but go on and change
 * it if you're adventurous...
 * =================================================================== */

/* The quoting character -- what overrides wildcards (do not undef) */
#define QUOTE '\\'

/* The "matches ANYTHING" wildcard (do not undef) */
#define WILDS '*'

/* The "matches ANY NUMBER OF NON-SPACE CHARS" wildcard (do not undef) */
#define WILDP '%'

/* The "matches EXACTLY ONE CHARACTER" wildcard (do not undef) */
#define WILDQ '?'

/* The "matches AT LEAST ONE SPACE" wildcard (undef me to disable!) */
#define WILDT '~'

/* This makes sure WILDT doesn't get used in in the IrcII version of
 * this code.  If ya wanna live dangerously, you can remove these 3
 * lines, but WARNING: IT WOULD MAKE THIS CODE INCOMPATIBLE WITH THE
 * CURRENT reg.c OF IrcII!!!  Support for ~ is NOT is the reg.c of
 * IrcII, and adding it may cause compatibility problems, especially
 * in scripts.  If you don't think you have to worry about that, go
 * for it! */
#ifndef EGGDROP
#undef WILDT
#endif

/* ===================================================================
 * If you edit below this line and it stops working, don't even THINK
 * about whining to *ME* about it!
 * =================================================================== */

/* No problem, you got it wrong anyway, Chris.  You should have gone to
 * uppercase instead of lowercase.  (A really minor mistake) */

/* Changing these is probably counter-productive :) */
#define MATCH (match+saved+sofar)
#define NOMATCH 0

/* ===================================================================
 * EGGDROP:   wild_match_per(char *m, char *n)
 * IrcII:     wild_match(char *m, char *n)
 * 
 * Features:  Forward, case-insensitive, ?, *, %, ~(optional)
 * Best use:  Generic string matching, such as in IrcII-esque bindings
 * =================================================================== */
#ifdef EGGDROP
static int wild_match_per(register unsigned char *m, register unsigned char *n)
#else
int wild_match(register unsigned char *m, register unsigned char *n)
#endif
{
  unsigned char *ma = m, *lsm = 0, *lsn = 0, *lpm = 0, *lpn = 0;
  int match = 1, saved = 0;
  register unsigned int sofar = 0;

#ifdef WILDT
  int space;
#endif

  /* take care of null strings (should never match) */
  if ((m == 0) || (n == 0) || (!*n))
    return NOMATCH;
  /* (!*m) test used to be here, too, but I got rid of it.  After all,
   * If (!*n) was false, there must be a character in the name (the
   * second string), so if the mask is empty it is a non-match.  Since
   * the algorithm handles this correctly without testing for it here
   * and this shouldn't be called with null masks anyway, it should be
   * a bit faster this way */

  while (*n) {
    /* Used to test for (!*m) here, but this scheme seems to work better */
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
	  return MATCH;		/* nonquoted * = match */
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
      /* Used to test for (!*n) here but it wasn't necessary so it's gone */
      saved = sofar = 0;
      continue;			/* Next char, please */
    }
    return NOMATCH;		/* No fallbacks=No match */
  }
  while ((*m == WILDS) || (*m == WILDP))
    m++;			/* Zap leftover %s & *s */
  return (*m) ? NOMATCH : MATCH;	/* End of both = match */
}

#ifndef EGGDROP

/* For IrcII compatibility */

int _wild_match(ma, na)
register unsigned char *ma, *na;
{
  return wild_match(ma, na) - 1;	/* Don't think IrcII's code
					 * actually uses this directly,
					 * but just in case */
}

int match(ma, na)
register unsigned char *ma, *na;
{
  return wild_match(ma, na) ? 1 : 0;	/* Returns 1 for match,
					 * 0 for non-match */
}

#else

/* ===================================================================
 * Remaining code is not used by IrcII
 * =================================================================== */

/* For this matcher, sofar's high bit is used as a flag of whether or
 * not we are quoting.  The other matchers don't need this because
 * when you're going forward, you just skip over the quote char. */
#define UNQUOTED (0x7FFF)
#define QUOTED   (0x8000)

#undef MATCH
#define MATCH ((match+sofar)&UNQUOTED)

/* ===================================================================
 * EGGDROP:   wild_match(char *ma, char *na)
 * IrcII:     NOT USED
 * 
 * Features:  Backwards, case-insensitive, ?, *
 * Best use:  Matching of hostmasks (since they are likely to begin
 *            with a * rather than end with one).
 * =================================================================== */

int _wild_match(register unsigned char *m, register unsigned char *n)
{
  unsigned char *ma = m, *na = n, *lsm = 0, *lsn = 0;
  int match = 1;
  register int sofar = 0;

  /* take care of null strings (should never match) */
  if ((ma == 0) || (na == 0) || (!*ma) || (!*na))
    return NOMATCH;
  /* find the end of each string */
  while (*(++m));
  m--;
  while (*(++n));
  n--;

  while (n >= na) {
    if ((m <= ma) || (m[-1] != QUOTE)) {	/* Only look if no quote */
      switch (*m) {
      case WILDS:		/* Matches anything */
	do
	  m--;			/* Zap redundant wilds */
	while ((m >= ma) && ((*m == WILDS) || (*m == WILDP)));
	if ((m >= ma) && (*m == '\\'))
	  m++;			/* Keep quoted wildcard! */
	lsm = m;
	lsn = n;
	match += sofar;
	sofar = 0;		/* Update fallback pos */
	continue;		/* Next char, please */
      case WILDQ:
	m--;
	n--;
	continue;		/* '?' always matches */
      }
      sofar &= UNQUOTED;	/* Remember not quoted */
    } else
      sofar |= QUOTED;		/* Remember quoted */
    if (rfc_toupper(*m) == rfc_toupper(*n)) {	/* If matching char */
      m--;
      n--;
      sofar++;			/* Tally the match */
      if (sofar & QUOTED)
	m--;			/* Skip the quote char */
      continue;			/* Next char, please */
    }
    if (lsm) {			/* To to fallback on '*' */
      n = --lsn;
      m = lsm;
      if (n < na)
	lsm = 0;		/* Rewind to saved pos */
      sofar = 0;
      continue;			/* Next char, please */
    }
    return NOMATCH;		/* No fallback=No match */
  }
  while ((m >= ma) && ((*m == WILDS) || (*m == WILDP)))
    m--;			/* Zap leftover %s & *s */
  return (m >= ma) ? NOMATCH : MATCH;	/* Start of both = match */
}

/* For this matcher, no "saved" is used to track "%" and no special quoting
 * ability is needed, so we just have (match+sofar) as the result. */

#endif
