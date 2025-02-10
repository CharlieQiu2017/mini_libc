/* strlen.c
   Derived from musl-libc src/string/strlen.c
 */

#include <stddef.h>
#include <stdint.h>

#define ONES ((size_t) -1 / 255) /* 0x0101010101010101 */
#define HIGHS (ONES * 128) /* 0x8080808080808080 */

/* Recall that (x - 1) & ~ x is a well-known way to find the least significant bit of x.
   All bits lower than the LSB are set to 1, and all higher bits including the LSB are set to 0.
   If every byte of x is non-zero, then (x - ONES) & ~ x applies this pattern to every byte in x.
   Hence the highest bit of every byte must be set to 0.
   Now if (x - ONES) & ~ x & HIGHS != 0, then at least one byte must be non-zero.
 */

#define HASZERO(x) (((x) - ONES) & ~ (x) & HIGHS)

size_t strlen (const char * s) {
  const char * a = s;
  typedef uint64_t __attribute__((__may_alias__)) word;
  const word * w;
  for (/* empty */; (uintptr_t) s % 8; s++) if (! (*s)) return (s - a);
  for (w = (const void *) s; ! HASZERO (*w); w++);
  s = (const void *) w;
  for (/* empty */; *s; s++);
  return (s - a);
}
