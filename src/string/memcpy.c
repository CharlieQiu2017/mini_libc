/* memcpy.c
   Derived from musl-libc src/string/memcpy.c
 */

#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <string_internal.h>

void * memcpy (void * restrict dest, const void * restrict src, size_t n) {
  const unsigned char * s = src;
  unsigned char * d = dest;

  /* 1. Copy initial bytes until s is aligned */

  while (n && ((uintptr_t) s & 7)) { *d = *s; s++; d++; n--; }
  if (!n) return dest;

  uint64_t s_buf, s_buf2, s_buf3;

  /* 2. If d is also aligned, copy 8 bytes at a time */

  if (((uintptr_t) d & 7) == 0) {
    while (n >= 8) {
      s_buf = read_u64 (s);
      *((uint64_alias_t *) d) = s_buf;
      s += 8; d += 8; n -= 8;
    }

    if (!n) return dest;

    s_buf = read_u64 (s);

    while (n) { *d = s_buf & 0xff; s_buf >>= 8; d++; n--; }

    return dest;
  }

  /* 3. Align d */

  uint32_t d_off = (uintptr_t) d & 7;
  s_buf = read_u64 (s);

  uint32_t i = 8 - d_off;
  while (i && n) { *d = s_buf & 0xff; s_buf >>= 8; d++; n--; i--; }
  if (!n) return dest;

  /* 4. Copy 8 bytes at once */

  s += 8;

  while (n >= 8) {
    s_buf2 = read_u64 (s);
    s_buf3 = s_buf | (s_buf2 << (8 * d_off));
    *((uint64_alias_t *) d) = s_buf3;

    s_buf = s_buf2 >> (8 * (8 - d_off));
    s += 8; d += 8; n -= 8;
  }

  if (!n) return dest;

  /* 5. Copy final bytes */

  if (n <= d_off) s_buf2 = 0; else s_buf2 = read_u64 (s);
  s_buf3 = s_buf | (s_buf2 << (8 * d_off));
  while (n) { *d = s_buf3 & 0xff; s_buf3 >>= 8; d++; n--; }
  return dest;
}
