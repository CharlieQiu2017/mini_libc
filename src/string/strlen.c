/* strlen.c
   Derived from musl-libc src/string/strlen.c
 */

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#define ALIGN (sizeof (size_t))
#define ONES ((size_t) -1 / UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) (((x) - ONES) & ~ (x) & HIGHS)

uint64_t strlen (const char *s) {
  const char *a = s;
  typedef uint64_t __attribute__((__may_alias__)) word;
  const word *w;
  for (/* empty */; (uintptr_t) s % ALIGN; s++) if (! (*s)) return (s - a);
  for (w = (const void *) s; ! HASZERO (*w); w++);
  s = (const void *) w;
  for (/* empty */; *s; s++);
  return (s - a);
}
