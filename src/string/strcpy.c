/* strcpy.c
   Derived from musl-libc src/string/stpcpy.c
 */

#include <string.h>
#include <stdint.h>

/* See strlen.c */
#define ONES ((size_t) -1 / 255)
#define HIGHS (ONES * 128)
#define HASZERO(x) (((x) - ONES) & ~ (x) & HIGHS)

char * strcpy (char * restrict d, const char * restrict s) {
  char * orig_d = d;
  typedef size_t __attribute__((__may_alias__)) word;
  word * wd;
  const word * ws;
  if ((uintptr_t) s % 8 == (uintptr_t) d % 8) {
    for (/* empty */; (uintptr_t) s % 8; s++, d++)
      if (! (*d = *s)) return d;
    wd = (void *) d;
    ws = (const void *) s;
    for (/* empty */; ! HASZERO (*ws); *wd++ = *ws++);
    d = (void *) wd;
    s = (const void *) ws;
  }
  for (/* empty */; (*d = *s); s++, d++);

  return orig_d;
}
