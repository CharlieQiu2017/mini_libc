#include <string.h>
#include <stdint.h>
#include <string.h>

#define ONES ((size_t) -1 / 255)
#define HIGHS (ONES * 128)
#define HASZERO(x) (((x) - ONES) & ~ (x) & HIGHS)

static size_t strncpy_internal (char * restrict d, const char * restrict s, size_t n) {
  char * orig_d = d;

  /* 1. Align s */
  while (n && ((uintptr_t) s & 7) && *s) { *d = *s; s++; d++; n--; }
  if (!n || *s == 0) return d - orig_d;

  uint32_t d_off = (uintptr_t) d & 7;

  if (d_off == 0) {
    /* 2. Read 8 bytes of s at once and write to d */
    uint64_t s_buf, end_flag = 0;

    while (n >= 8) {
      s_buf = * ((const uint64_t *) s);
      if (HASZERO (s_buf)) { end_flag = 1; break; }
      * ((uint64_t *) d) = s_buf;
      s += 8; d += 8; n -= 8;
    }

    if (!n) return d - orig_d;

    if (n < 8) {
      s_buf = * ((const uint64_t *) s);
      while (n && (s_buf & 0xff)) { *d = s_buf & 0xff; s_buf >>= 8; d++; n--; }
      return d - orig_d;
    }

    if (end_flag) {
      while (s_buf & 0xff) { *d = s_buf & 0xff; s_buf >>= 8; d++; }
      return d - orig_d;
    }

    /* Should not reach here */
    return 0;
  }

  /* 2. Read 8 bytes of s */
  uint64_t s_buf1 = * ((const uint64_t *) s), s_buf2, s_buf3, end_flag = 0;

  /* 3. Write (8 - d_off) bytes to d, so that d is now aligned */

  uint32_t i = 8 - d_off;
  while (i && n && (s_buf1 & 0xff)) {
    *d = s_buf1 & 0xff;
    s_buf1 >>= 8; d++; i--; n--;
  }

  if (i || !n) return d - orig_d;

  /* 4. Repeat write 8 bytes to R */
  while (n >= 8) {
    s += 8;
    s_buf2 = * ((const uint64_t *) s);
    s_buf3 = s_buf1 | (s_buf2 << (8 * d_off));
    if (HASZERO (s_buf2)) { end_flag = 1; break; }

    * ((uint64_t *) d) = s_buf3;
    s_buf1 = s_buf2 >> (8 * (8 - d_off));
    d += 8; n -= 8;
  }

  if (!n) return d - orig_d;

  /* 5. Finish final bytes */

  if (n < 8) {
    i = d_off;
    while (i && n && (s_buf1 & 0xff)) { *d = s_buf1 & 0xff; s_buf1 >>= 8; d++; i--; n--; }
    if (i || !n) return d - orig_d;

    s += 8; s_buf1 = * ((const uint64_t *) s);
    while (n && (s_buf1 & 0xff)) { *d = s_buf1 & 0xff; s_buf1 >>= 8; d++; n--; }
    return d - orig_d;
  }

  if (end_flag) {
    i = 8;
    while (i && (s_buf3 & 0xff)) { *d = s_buf3 & 0xff; s_buf3 >>= 8; d++; i--; }
    if (i) return d - orig_d;

    s_buf1 = s_buf2 >> (8 * (8 - d_off));
    n -= 8;

    while (n && (s_buf1 & 0xff)) { *d = s_buf1 & 0xff; s_buf1 >>= 8; d++; n--; }
    return d - orig_d;
  }

  /* Should not reach here */
  return 0;
}

char * strncpy (char * restrict d, const char * restrict s, size_t n) {
  size_t byte_written = strncpy_internal (d, s, n);
  if (byte_written < n) memset (d + byte_written, 0, n - byte_written);
  return d;
}
