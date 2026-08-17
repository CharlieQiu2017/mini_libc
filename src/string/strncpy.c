#include <string.h>
#include <stdint.h>
#include <string.h>
#include <string_internal.h>

#define ONES ((size_t) -1 / 255)
#define HIGHS (ONES * 128)
#define HASZERO(x) (((x) - ONES) & ~ (x) & HIGHS)

static size_t strncpy_internal (char * restrict dest, const char * restrict src, size_t n) {
  uintptr_t orig_d = (uintptr_t) dest;
  uintptr_t d = (uintptr_t) dest;
  uintptr_t s = (uintptr_t) src;

  const unsigned char * sp = const_ptr_from_uint (s);
  unsigned char * dp = ptr_from_uint (d);

  /* 1. Align s */
  while (n && (s & 7) && *sp) { *dp = *sp; s++; d++; n--; sp = const_ptr_from_uint (s); dp = ptr_from_uint (d); }
  if (!n || *sp == 0) return d - orig_d;

  uint32_t d_off = d & 7;

  if (d_off == 0) {
    /* 2. Read 8 bytes of s at once and write to d */
    uint64_t s_buf;

    while (n >= 8) {
      s_buf = read_u64 (s);
      if (HASZERO (s_buf)) goto aligned_s_end_flag;
      write_u64 (d, s_buf);
      s += 8; d += 8; n -= 8;
    }

    if (!n) return d - orig_d;

    s_buf = read_u64 (s);
    while (n && (s_buf & 0xff)) { dp = ptr_from_uint (d); *dp = s_buf & 0xff; s_buf >>= 8; d++; n--; }
    return d - orig_d;

aligned_s_end_flag:
    /* If this line is reached, s_buf contains NUL byte */
    while (s_buf & 0xff) { dp = ptr_from_uint (d); *dp = s_buf & 0xff; s_buf >>= 8; d++; }
    return d - orig_d;
  }

  /* 2. Read 8 bytes of s */
  uint64_t s_buf1 = read_u64 (s), s_buf2, s_buf3;

  if (HASZERO (s_buf1)) {
    while (n && (s_buf1 & 0xff)) { dp = ptr_from_uint (d); *dp = s_buf1 & 0xff; s_buf1 >>= 8; d++; n--; }
    return d - orig_d;
  }

  /* 3. Write (8 - d_off) bytes to d, so that d is now aligned.
     Since we checked HASZERO(s_buf1) above, we do not need to check for NUL here.
   */

  uint32_t i = 8 - d_off;
  while (i && n) {
    dp = ptr_from_uint (d);
    *dp = s_buf1 & 0xff;
    s_buf1 >>= 8; d++; i--; n--;
  }

  if (i || !n) return d - orig_d;

  /* 4. Repeat write 8 bytes to R */
  while (n >= 8) {
    s += 8;
    s_buf2 = read_u64 (s);
    s_buf3 = s_buf1 | (s_buf2 << (8 * d_off));
    if (HASZERO (s_buf2)) goto s_end_flag;

    write_u64 (d, s_buf3);
    s_buf1 = s_buf2 >> (8 * (8 - d_off));
    d += 8; n -= 8;
  }

  if (!n) return d - orig_d;

  /* 5. Finish final bytes */

  i = d_off;
  while (i && n && (s_buf1 & 0xff)) { dp = ptr_from_uint (d); *dp = s_buf1 & 0xff; s_buf1 >>= 8; d++; i--; n--; }
  if (i || !n) return d - orig_d;

  s += 8; s_buf1 = read_u64 (s);
  while (n && (s_buf1 & 0xff)) { dp = ptr_from_uint (d); *dp = s_buf1 & 0xff; s_buf1 >>= 8; d++; n--; }
  return d - orig_d;

s_end_flag:
  i = 8;
  while (i && (s_buf3 & 0xff)) { dp = ptr_from_uint (d); *dp = s_buf3 & 0xff; s_buf3 >>= 8; d++; i--; }
  if (i) return d - orig_d;

  s_buf1 = s_buf2 >> (8 * (8 - d_off));
  n -= 8;

  while (n && (s_buf1 & 0xff)) { dp = ptr_from_uint (d); *dp = s_buf1 & 0xff; s_buf1 >>= 8; d++; n--; }
  return d - orig_d;
}

char * strncpy (char * restrict d, const char * restrict s, size_t n) {
  size_t byte_written = strncpy_internal (d, s, n);
  if (byte_written < n) memset (d + byte_written, 0, n - byte_written);
  return d;
}
