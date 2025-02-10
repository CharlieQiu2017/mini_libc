/* memmove.c
   Derived from musl-libc src/string/memmove.c
 */

#include <stddef.h>
#include <string.h>
#include <stdint.h>

typedef __attribute__((__may_alias__)) size_t WT;

void * memmove (void * dest, const void * src, size_t n) {
  char * d = dest;
  const char * s = src;

  if (d == s) return d;

  /* Since n is unsigned, -2 * n is also unsigned.
     It is equal to 2^64 - (2 * n), assuming 2 * n does not overflow.
     Now assume both d and s point to valid memory regions of size at least n.
     Hence s <= 2^64 - n, and d <= 2^64 - n.
     If s > d, then 0 < s - d <= 2^64 - n.
     So s - d - n > 2^64 - (2 * n) only if the subtraction overflowed, i.e. s - d < n.
     If s < d, then s - d = 2^64 - (d - s), and d - s <= 2^64 - n, so s - d >= n.
     So to test whether d - s >= n we also only needs to test s - d - n <= 2^64 - 2 * n.
   */
  if ((uintptr_t) s - (uintptr_t) d - n <= -2 * n) return memcpy(d, s, n);

  if (d < s) {
    if ((uintptr_t) s % 8 == (uintptr_t) d % 8) {
      while ((uintptr_t) d % 8) {
	if (!n--) return dest;
	*d++ = *s++;
      }
      for (/* empty */; n >= 8; n -= 8, d += 8, s += 8) *(WT *) d = *(WT *) s;
    }
    for (/* empty */; n; n--) *d++ = *s++;
  } else {
    if ((uintptr_t) s % 8 == (uintptr_t) d % 8) {
      while ((uintptr_t)(d + n) % 8) {
	if (!n--) return dest;
	d[n] = s[n];
      }
      while (n >= 8) n -= 8, *(WT *)(d + n) = *(WT *)(s + n);
    }
    while (n) { n--; d[n] = s[n]; }
  }

  return dest;
}
