/*
 * match.c
 *   wildcard matching functions
 *   hostmask matching
 *   cidr matching
 *
 * $Id: match.c,v 1.18 2010/07/01 16:10:49 thommey Exp $
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

int cidr_support = 0;

int casecharcmp(unsigned char a, unsigned char b)
{
  return (rfc_toupper(a) - rfc_toupper(b));
}

int charcmp(unsigned char a, unsigned char b)
{
  return (a - b);
}

/* Wildcard-matches mask m to n. Uses the comparison function cmp1. If chgpoint
 * isn't NULL, it points at the first character in n where we start using cmp2.
 */
int _wild_match_per(register unsigned char *m, register unsigned char *n,
                           int (*cmp1)(unsigned char, unsigned char),
                           int (*cmp2)(unsigned char, unsigned char),
                           unsigned char *chgpoint)
{
  unsigned char *ma = m, *lsm = 0, *lsn = 0, *lpm = 0, *lpn = 0;
  int match = 1, saved = 0, space;
  register unsigned int sofar = 0;

  /* null strings should never match */
  if ((m == 0) || (n == 0) || (!*n) || (!cmp1))
    return NOMATCH;

  if (!cmp2)                    /* Don't change cmpfunc if it's not valid */
    chgpoint = NULL;

  while (*n) {
    if (*m == WILDT) {          /* Match >=1 space */
      space = 0;                /* Don't need any spaces */
      do {
        m++;
        space++;
      }                         /* Tally 1 more space ... */
      while ((*m == WILDT) || (*m == ' '));     /*  for each space or ~ */
      sofar += space;           /* Each counts as exact */
      while (*n == ' ') {
        n++;
        space--;
      }                         /* Do we have enough? */
      if (space <= 0)
        continue;               /* Had enough spaces! */
    }
    /* Do the fallback       */
    else {
      switch (*m) {
      case 0:
        do
          m--;                  /* Search backwards */
        while ((m > ma) && (*m == '?'));        /* For first non-? char */
        if ((m > ma) ? ((*m == '*') && (m[-1] != QUOTE)) : (*m == '*'))
          return PERMATCH;      /* nonquoted * = match */
        break;
      case WILDP:
        while (*(++m) == WILDP);        /* Zap redundant %s */
        if (*m != WILDS) {      /* Don't both if next=* */
          if (*n != ' ') {      /* WILDS can't match ' ' */
            lpm = m;
            lpn = n;            /* Save '%' fallback spot */
            saved += sofar;
            sofar = 0;          /* And save tally count */
          }
          continue;             /* Done with '%' */
        }
        /* FALL THROUGH */
      case WILDS:
        do
          m++;                  /* Zap redundant wilds */
        while ((*m == WILDS) || (*m == WILDP));
        lsm = m;
        lsn = n;
        lpm = 0;                /* Save '*' fallback spot */
        match += (saved + sofar);       /* Save tally count */
        saved = sofar = 0;
        continue;               /* Done with '*' */
      case WILDQ:
        m++;
        n++;
        continue;               /* Match one char */
      case QUOTE:
        m++;                    /* Handle quoting */
      }
      if (((!chgpoint || n < chgpoint) && !(*cmp1)(*m, *n)) ||
          (chgpoint && n >= chgpoint && !(*cmp2)(*m, *n))) { /* If matching */
        m++;
        n++;
        sofar++;
        continue;               /* Tally the match */
      }
#ifdef WILDT
    }
#endif
    if (lpm) {                  /* Try to fallback on '%' */
      n = ++lpn;
      m = lpm;
      sofar = 0;                /* Restore position */
      if ((*n | 32) == 32)
        lpm = 0;                /* Can't match 0 or ' ' */
      continue;                 /* Next char, please */
    }
    if (lsm) {                  /* Try to fallback on '*' */
      n = ++lsn;
      m = lsm;                  /* Restore position */
      saved = sofar = 0;
      continue;                 /* Next char, please */
    }
    return NOMATCH;             /* No fallbacks=No match */
  }
  while ((*m == WILDS) || (*m == WILDP))
    m++;                        /* Zap leftover %s & *s */
  return (*m) ? NOMATCH : PERMATCH;     /* End of both = match */
}

/* Generic string matching, use addr_match() for hostmasks! */
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
    /* If the mask runs out of chars before the string, fall back on
     * a wildcard or fail. */
    if (m < ma) {
      if (lsm) {
        n = --lsn;
        m = lsm;
        if (n < na)
          lsm = 0;
        sofar = 0;
      }
      else
        return NOMATCH;
    }

    switch (*m) {
    case WILDS:                /* Matches anything */
      do
        m--;                    /* Zap redundant wilds */
      while ((m >= ma) && (*m == WILDS));
      lsm = m;
      lsn = n;
      match += sofar;
      sofar = 0;                /* Update fallback pos */
      if (m < ma)
        return MATCH;
      continue;                 /* Next char, please */
    case WILDQ:
      m--;
      n--;
      continue;                 /* '?' always matches */
    }
    if (toupper(*m) == toupper(*n)) {   /* If matching char */
      m--;
      n--;
      sofar++;                  /* Tally the match */
      continue;                 /* Next char, please */
    }
    if (lsm) {                  /* To to fallback on '*' */
      n = --lsn;
      m = lsm;
      if (n < na)
        lsm = 0;                /* Rewind to saved pos */
      sofar = 0;
      continue;                 /* Next char, please */
    }
    return NOMATCH;             /* No fallback=No match */
  }
  while ((m >= ma) && (*m == WILDS))
    m--;                        /* Zap leftover %s & *s */
  return (m >= ma) ? NOMATCH : MATCH;   /* Start of both = match */
}

/* cidr and RFC1459 compatible host matching
 * Returns: 1 if the address in n matches the hostmask in m.
 * If cmp != 0, m and n will be compared as masks. Returns 1
 * if m is broader, 0 otherwise.
 * If user != 0, the masks are eggdrop user hosts and should
 * be matched regardless of the cidr_support variable.
 * This is required as userhost matching shouldn't depend on
 * server support of cidr.
 */
int addr_match(char *m, char *n, int user, int cmp)
{
  char *p, *q, *r = 0, *s = 0;
  char mu[UHOSTLEN], nu[UHOSTLEN];

  /* copy the strings into our own buffers
     and convert to rfc uppercase */
  for (p = mu; *m && (p - mu < UHOSTLEN - 1); m++) {
    if (*m == '@')
      r = p;
    *p++ = rfc_toupper(*m);
  }
  for (q = nu; *n && (q - nu < UHOSTLEN - 1); n++) {
    if (*n == '@')
      s = q;
    *q++ = rfc_toupper(*n);
  }
  *p = *q = 0;
  if ((!user && !cidr_support) || !r || !s)
    return wild_match(mu, nu) ? 1 : NOMATCH;

  *r++ = *s++ = 0;
  if (!wild_match(mu, nu))
    return NOMATCH; /* nick!ident parts don't match */
  if (!*r && !*s)
    return 1; /* end of nonempty strings */

  /* check for CIDR notation and perform
     generic string matching if not found */
  if (!(p = strrchr(r, '/')) || !str_isdigit(p + 1))
    return wild_match(r, s) ? 1 : NOMATCH;
  /* if the two strings are both cidr masks,
     use the broader prefix */
  if (cmp && (q = strrchr(s, '/')) && str_isdigit(q + 1)) {
    if (atoi(p + 1) > atoi(q + 1))
      return NOMATCH;
    *q = 0;
  }
  *p = 0;
  /* looks like a cidr mask */
  return cidr_match(r, s, atoi(p + 1));
}

/* Checks for overlapping masks
 * Returns: > 0 if the two masks in m and n overlap, 0 otherwise.
 */
int mask_match(char *m, char *n)
{
  int prefix;
  char *p, *q, *r = 0, *s = 0;
  char mu[UHOSTLEN], nu[UHOSTLEN];

  for (p = mu; *m && (p - mu < UHOSTLEN - 1); m++) {
    if (*m == '@')
      r = p;
    *p++ = rfc_toupper(*m);
  }
  for (q = nu; *n && (q - nu < UHOSTLEN - 1); n++) {
    if (*n == '@')
      s = q;
    *q++ = rfc_toupper(*n);
  }
  *p = *q = 0;
  if (!cidr_support || !r || !s)
    return (wild_match(mu, nu) || wild_match(nu, mu));

  *r++ = *s++ = 0;
  if (!wild_match(mu, nu) && !wild_match(nu, mu))
    return 0;

  if (!*r && !*s)
    return 1;
  p = strrchr(r, '/');
  q = strrchr(s, '/');
  if ((!p || !str_isdigit(p + 1)) && (!q || !str_isdigit(q + 1)))
    return (wild_match(r, s) || wild_match(s, r));

  if (p) {
    *p = 0;
    prefix = atoi(p + 1);
  } else
    prefix = (strchr(r, ':') ? 128 : 32);
  if (q) {
    *q = 0;
    if (atoi(q + 1) < prefix)
      prefix = atoi(q + 1);
  }
  return cidr_match(r, s, prefix);
}

/* Performs bitwise comparison of two IP addresses stored in presentation
 * (string) format. IPs are first internally converted to binary form.
 * Returns: 1 if the first count bits are equal, 0 otherwise.
 */
int cidr_match(char *m, char *n, int count)
{
#ifdef IPV6
  int c, af = AF_INET;
  u_8bit_t block[16], addr[16];

  if (count < 1)
    return NOMATCH;
  if (strchr(m, ':') || strchr(n, ':')) {
    af = AF_INET6;
    if (count > 128)
      return NOMATCH;
  } else if (count > 32)
      return NOMATCH;
  if (inet_pton(af, m, &block) != 1 ||
      inet_pton(af, n, &addr) != 1)
    return NOMATCH;
  for (c = 0; c < (count / 8); c++)
    if (block[c] != addr[c])
      return NOMATCH;
  if (!(count % 8))
    return 1;
  count = 8 - (count % 8);
  return ((block[c] >> count) == (addr[c] >> count));

#else
  IP block, addr;

  if (count < 1 || count > 32)
    return NOMATCH;
  block = ntohl(inet_addr(m));
  addr = ntohl(inet_addr(n));
  if (block == INADDR_NONE || addr == INADDR_NONE)
    return NOMATCH;
  count = 32 - count;
  return ((block >> count) == (addr >> count));
#endif
}

/* Inline for cron_match (obviously).
 * Matches a single field of a crontab expression.
 */
inline int cron_matchfld(char *mask, int match)
{
  int skip = 0, f, t;
  char *p, *q;

  for (p = mask; mask && *mask; mask = p) {
    /* loop through a list of values, if such is given */
    if ((p = strchr(mask, ',')))
      *p++ = 0;
    /* check for the step operator */
    if ((q = strchr(mask, '/'))) {
      if (q == mask)
        continue;
      *q++ = 0;
      skip = atoi(q);
    }
    if (!strcmp(mask, "*") && (!skip || !(match % skip)))
      return 1;
    /* ranges, e.g 10-20 */
    if (strchr(mask, '-')) {
      if (sscanf(mask, "%d-%d", &f, &t) != 2)
        continue;
      if (t < f) {
        if (match <= t)
          match += 60;
        t += 60;
      }
      if ((match >= f && match <= t) &&
          (!skip || !((match - f) % skip)))
        return 1;
    }
    /* no operator found, should be exact match */
    f = strtol(mask, &q, 10);
    if ((q > mask) &&
        (skip ? !((match - f) % skip) : (match == f)))
      return 1;
  }
  return 0;
}

/* Check if the current time matches a crontab-like specification.
 *
 * mask contains a cron-style series of time fields. The following
 * crontab operators are supported: ranges '-', asterisks '*',
 * lists ',' and steps '/'.
 * match must have 5 space separated integers representing in order
 * the current minute, hour, day of month, month and weekday.
 * It should look like this: "53 17 01 03 06", which means
 * Sunday 01 March, 17:53.
 */
int cron_match(const char *mask, const char *match)
{
  int d = 0, i, m = 1, t[5];
  char *p, *q, *buf;

  if (!mask[0])
    return 0;
  if (sscanf(match, "%d %d %d %d %d",
             &t[0], &t[1], &t[2], &t[3], &t[4]) < 5)
    return 0;
  buf = nmalloc(strlen(mask) + 1);
  strcpy(buf, mask);
  for (p = buf, i = 0; *p && i < 5; i++) {
    q = newsplit(&p);
    if (!strcmp(q, "*"))
      continue;
    m = (cron_matchfld(q, t[i]) ||
        (i == 4 && !t[i] && cron_matchfld(q, 7)));
    if (i == 2)
      d = m;
    else if (!m || (i == 3 && d))
      break;
  }
  nfree(buf);
  return m;
}
