/* memxor.c
   Similar to memcpy, but instead of overwriting dest with src,
   perform exclusive-or between dest and src, and write to dest.
   Used in certain cryptographic procedures.
 */

#include <stddef.h>
#include <string.h>
#include <stdint.h>

void * memxor (void * restrict dest, const void * restrict src, size_t n) {
  unsigned char * d = dest;
  const unsigned char * s = src;

  /* 1. Align s */
  while (((uintptr_t) s & 7) && n) { *d = *d ^ *s; s++; d++; n--; }

  if (!n) return dest;

  uint32_t d_off = (uintptr_t) d & 7;

  if (d_off == 0) {
    /* 2. Read 8 bytes of s and d at once */
    uint64_t s_buf, d_buf;

    while (n >= 8) {
      s_buf = * ((const uint64_t *) s);
      d_buf = * ((const uint64_t *) d);
      d_buf ^= s_buf;
      * ((uint64_t *) d) = d_buf;
      s += 8; d += 8; n -= 8;
    }

    if (!n) return dest;

    /* 3. Finish remaining bytes */
    s_buf = * ((const uint64_t *) s);
    d_buf = * ((const uint64_t *) d);
    d_buf ^= s_buf;

    while (n) { *d = d_buf & 0xff; d_buf >>= 8; d++; n--; }

    return dest;
  }

  /* 2. Read 8 bytes of s and d */
  uint64_t s_buf1 = * ((const uint64_t *) s), s_buf2, d_buf = * ((const uint64_t *) (d - d_off));
  d_buf >>= 8 * d_off;
  d_buf ^= s_buf1;

  /* 3. Write (8 - d_off) bytes to d */
  uint32_t i = 8 - d_off;
  while (i && n) { *d = d_buf & 0xff; d_buf >>= 8; d++; i--; n--; }

  if (!n) return dest;

  s += 8; s_buf1 >>= 8 * (8 - d_off);

  /* 4. Process 8 bytes at once */
  while (n >= 8) {
    s_buf2 = * ((const uint64_t *) s);
    d_buf = * ((const uint64_t *) d);
    d_buf ^= s_buf1 | (s_buf2 << (8 * d_off));

    * ((uint64_t *) d) = d_buf;
    s_buf1 = s_buf2 >> (8 * (8 - d_off));
    s += 8; d += 8; n -= 8;
  }

  if (!n) return dest;

  /* Process remaining bytes */
  if (n <= d_off) s_buf2 = 0; else s_buf2 = * ((const uint64_t *) s);
  d_buf = * ((const uint64_t *) d);
  d_buf ^= s_buf1 | (s_buf2 << (8 * d_off));

  while (n) { *d = d_buf & 0xff; d_buf >>= 8; d++; n--; }

  return dest;
}
