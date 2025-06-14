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

  /* 1. Align s */
  while (((uintptr_t) s & 7) && *s) { *d = *s; s++; d++; }

  if (*s == 0) { *d = 0; return orig_d; }

  uint32_t d_off = (uintptr_t) d & 7;

  if (d_off == 0) {
    /* 2. Read 8 bytes of s at once and write to d */
    uint64_t s_buf;

    while (1) {
      s_buf = * ((const uint64_t *) s);
      if (HASZERO (s_buf)) break;
      * ((uint64_t *) d) = s_buf;
      s += 8; d += 8;
    }

    /* 3. Finish remaining bytes */
    while (s_buf & 0xff) {
      *d = s_buf & 0xff;
      s_buf >>= 8; d++;
    }

    *d = 0;
    return orig_d;
  }

  /* 2. Read 8 bytes of s */
  uint64_t s_buf1 = * ((const uint64_t *) s), s_buf2;

  if (HASZERO (s_buf1)) {
    while (s_buf1 & 0xff) {
      *d = s_buf1 & 0xff;
      s_buf1 >>= 8; d++;
    }

    *d = 0;
    return orig_d;
  }

  /* 3. Write (8 - d_off) bytes to d, so that d is now aligned */
  uint32_t i = 8 - d_off;
  while (i) { *d = s_buf1 & 0xff; d++; i--; s_buf1 >>= 8; }

  while (1) {
    s += 8;
    s_buf2 = * ((const uint64_t *) s);
    if (HASZERO (s_buf2)) break;

    * ((uint64_t *) d) = s_buf1 | (s_buf2 << (8 * d_off));
    s_buf1 = s_buf2 >> (8 * (8 - d_off));
    d += 8;
  }

  /* 4. Write d_off bytes in s_buf1 and final bytes in s_buf2 */
  i = d_off;
  while (i) { *d = s_buf1 & 0xff; d++; i--; s_buf1 >>= 8; }

  while (s_buf2 & 0xff) { *d = s_buf2 & 0xff; d++; s_buf2 >>= 8; }

  *d = 0;
  return orig_d;
}
