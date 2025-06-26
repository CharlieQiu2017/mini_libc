/* memmove.c
   Derived from musl-libc src/string/memmove.c
 */

#include <stddef.h>
#include <string.h>
#include <stdint.h>

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
  if ((uintptr_t) s - (uintptr_t) d - n <= -2 * n) return memcpy (d, s, n);

  if ((uintptr_t) d < (uintptr_t) s) {

    /* 1. Align s */
    while (n && ((uintptr_t) s & 7)) { *d = *s; s++; d++; n--; }
    if (!n) return dest;

    /* 2. If d is also aligned, copy 8 bytes at once */
    uint64_t s_buf, s_buf2;

    if (((uintptr_t) d & 7) == 0) {
      while (n >= 8) {
	s_buf = * ((const uint64_t *) s);
	* ((uint64_t *) d) = s_buf;

	s += 8; d += 8; n -= 8;
      }

      while (n) { *d = *s; s++; d++; n--; }
      return dest;
    }

    /* 3. Otherwise, first copy (8 - d_off) bytes to align d */
    uint32_t d_off = (uintptr_t) d & 7;
    s_buf = * ((const uint64_t *) s);

    uint32_t i = 8 - d_off;
    while (i && n) { *d = s_buf & 0xff; s_buf >>= 8; d++; i--; n--; }
    if (!n) return dest;

    s += 8;

    /* 4. Repeat copy 8 bytes to d at once */
    while (n >= 8) {
      s_buf2 = * ((const uint64_t *) s);
      * ((uint64_t *) d) = s_buf | (s_buf2 << (8 * d_off));

      s_buf = s_buf2 >> (8 * (8 - d_off));
      s += 8;
      d += 8;
      n -= 8;
    }

    if (!n) return dest;

    /* 5. Copy final bytes */
    if (n <= d_off) s_buf2 = 0; else s_buf2 = * ((const uint64_t *) s);
    s_buf = s_buf | (s_buf2 << (8 * d_off));

    while (n) { *d = s_buf & 0xff; s_buf >>= 8; d++; n--; }
    return dest;

  } else {

    /* Symmetric to the case above,
       except we copy backwards. */
    s = s + n;
    d = d + n;

    while (n && ((uintptr_t) s & 7)) { s--; d--; *d = *s; n--; }
    if (!n) return dest;

    uint64_t s_buf, s_buf2;

    if (((uintptr_t) d & 7) == 0) {
      while (n >= 8) {
	s -= 8;
	d -= 8;

	s_buf = * ((const uint64_t *) s);
	* ((uint64_t *) d) = s_buf;

	n -= 8;
      }

      while (n) { s--; d--; *d = *s; n--; }
      return dest;
    }

    uint32_t d_off = (uintptr_t) d & 7;
    s -= 8;
    s_buf = * ((const uint64_t *) s);

    uint32_t i = d_off;
    while (i && n) { d--; *d = (s_buf >> 56); s_buf <<= 8; i--; n--; }
    if (!n) return dest;

    while (n >= 8) {
      s -= 8;
      d -= 8;

      s_buf2 = * ((const uint64_t *) s);
      * ((uint64_t *) d) = s_buf | (s_buf2 >> (8 * (8 - d_off)));

      s_buf = s_buf2 << (8 * d_off);
      n -= 8;
    }

    if (!n) return dest;

    s -= 8;
    if (n <= 8 - d_off) s_buf2 = 0; else s_buf2 = * ((const uint64_t *) s);
    s_buf = s_buf | (s_buf2 >> (8 * (8 - d_off)));

    while (n) { d--; *d = (s_buf >> 56); s_buf <<= 8; n--; }
    return dest;

  }
}
