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
   Now if (x - ONES) & ~ x & HIGHS != 0, then at least one byte must be zero.
 */

#define HASZERO(x) (((x) - ONES) & ~ (x) & HIGHS)

size_t strlen (const char * s) {
  /* 1. Store the initial position */
  const char * orig_s = s;

  /* 2. Check for NUL until s is aligned */
  while ((uintptr_t) s & 7) { if (*s == 0) return s - orig_s; s++; }

  /* 3. Repeat read 8 bytes of s and check for NUL */
  uint64_t buf;

  while (1) {
    buf = * ((uint64_t *) s);
    if (HASZERO (buf)) break;
    s += 8;
  }

  /* 4. Individually check the final bytes */

  while (buf & 0xff) { buf >>= 8; s++; }

  return s - orig_s;
}
