/* 
 * This should make things properly case insensitive with respect to RFC 1459
 * or at least get them doing things the same way the IRCd does.
 */
#include <ctype.h>

#ifdef RFC_COMPLIANT
#define rfc_tolower(c) (rfc_tolowertab[(unsigned char)(c)])
#define rfc_toupper(c) (rfc_touppertab[(unsigned char)(c)])
extern unsigned char rfc_tolowertab[];
extern unsigned char rfc_touppertab[];
#else				/* Dalnet?? */
#define rfc_tolower(c) tolower(c)
#define rfc_toupper(c) toupper(c)
#endif				/* RFC_COMPLIANT */
