/* memxor.c
   Similar to memcpy, but instead of overwriting dest with src,
   perform exclusive-or between dest and src, and write to dest.
   Used in certain cryptographic procedures.
 */

#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <string_internal.h>

void * memxor (void * restrict dest, const void * restrict src, size_t n) {
  uintptr_t d = (uintptr_t) dest;
  uintptr_t s = (uintptr_t) src;
  const unsigned char * sp;
  unsigned char * dp;

  /* 1. Align s */
  while ((s & 7) && n) { sp = const_ptr_from_uint (s); dp = ptr_from_uint (d); *dp = *dp ^ *sp; s++; d++; n--; }

  if (!n) return dest;

  uint32_t d_off = d & 7;

  if (d_off == 0) {
    /* 2. Read 8 bytes of s and d at once */
    uint64_t s_buf, d_buf;

    while (n >= 8) {
      s_buf = read_u64 (s);
      d_buf = read_u64 (d);
      d_buf ^= s_buf;
      write_u64 (d, d_buf);
      s += 8; d += 8; n -= 8;
    }

    if (!n) return dest;

    /* 3. Finish remaining bytes */
    s_buf = read_u64 (s);
    d_buf = read_u64 (d);
    d_buf ^= s_buf;

    while (n) { dp = ptr_from_uint (d); *dp = d_buf & 0xff; d_buf >>= 8; d++; n--; }

    return dest;
  }

  /* 2. Read 8 bytes of s and d */
  uint64_t s_buf1 = read_u64 (s), s_buf2, d_buf = read_u64 (d - d_off);
  d_buf >>= 8 * d_off;
  d_buf ^= s_buf1;

  /* 3. Write (8 - d_off) bytes to d */
  uint32_t i = 8 - d_off;
  while (i && n) { dp = ptr_from_uint (d); *dp = d_buf & 0xff; d_buf >>= 8; d++; i--; n--; }

  if (!n) return dest;

  s += 8; s_buf1 >>= 8 * (8 - d_off);

  /* 4. Process 8 bytes at once */
  while (n >= 8) {
    s_buf2 = read_u64 (s);
    d_buf = read_u64 (d);
    d_buf ^= s_buf1 | (s_buf2 << (8 * d_off));

    write_u64 (d, d_buf);
    s_buf1 = s_buf2 >> (8 * (8 - d_off));
    s += 8; d += 8; n -= 8;
  }

  if (!n) return dest;

  /* Process remaining bytes */
  if (n <= d_off) s_buf2 = 0; else s_buf2 = read_u64 (s);
  d_buf = read_u64 (d);
  d_buf ^= s_buf1 | (s_buf2 << (8 * d_off));

  while (n) { dp = ptr_from_uint (d); *dp = d_buf & 0xff; d_buf >>= 8; d++; n--; }

  return dest;
}
