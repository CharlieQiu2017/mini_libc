/* memcmp.c
   Derived from musl-libc src/string/memcmp.c
 */

/* Surprise! On aarch64 platforms "char" is "unsigned char"! */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <string_internal.h>

int32_t memcmp (const void * vl, const void * vr, size_t n) {
  uintptr_t l = (uintptr_t) vl;
  uintptr_t r = (uintptr_t) vr;

  /* 1. Compare initial bytes until L is aligned */

  const unsigned char * lp = const_ptr_from_uint (l), * rp = const_ptr_from_uint (r);
  while (n && (l & 7)) {
    if (*lp != *rp) break;
    l++; r++; n--;
    lp = const_ptr_from_uint (l);
    rp = const_ptr_from_uint (r);
  }
  if (!n) return 0;
  if (*lp != *rp) return ((int32_t) *lp) - ((int32_t) *rp);

  uint64_t l_buf, l_buf2, l_buf3, r_buf;

  /* 2. If R is also aligned, compare 8 bytes of L, R at a time */

  if ((r & 7) == 0) {
    while (n >= 8) {
      l_buf = read_u64 (l);
      r_buf = read_u64 (r);

      if (l_buf != r_buf) {
	while ((l_buf & 0xff) == (r_buf & 0xff)) { l_buf >>= 8; r_buf >>= 8; }
	return ((int32_t) (l_buf & 0xff)) - ((int32_t) (r_buf & 0xff));
      }

      l += 8; r += 8; n -= 8;
    }

    if (!n) return 0;

    l_buf = read_u64 (l);
    r_buf = read_u64 (r);

    while (n && (l_buf & 0xff) == (r_buf & 0xff)) { l_buf >>= 8; r_buf >>= 8; n--; }

    if (!n) return 0;

    return ((int32_t) (l_buf & 0xff)) - ((int32_t) (r_buf & 0xff));
  }

  uint32_t r_off = r & 7;
  l_buf = read_u64 (l);
  r_buf = read_u64 (r - r_off);
  r_buf >>= 8 * r_off;

  /* 3. Compare the next (8 - r_off) bytes */

  uint32_t i = 8 - r_off;
  while (i && n && (l_buf & 0xff) == (r_buf & 0xff)) { l_buf >>= 8; r_buf >>= 8; i--; n--; }
  if (!n) return 0;
  if (i) return ((int32_t) (l_buf & 0xff)) - ((int32_t) (r_buf & 0xff));

  l += 8;
  r += (8 - r_off);

  /* 4. Repeat read and compare 8 bytes of L, R */

  while (n >= 8) {
    l_buf2 = read_u64 (l);
    r_buf = read_u64 (r);
    l_buf3 = l_buf | (l_buf2 << (8 * r_off));

    if (l_buf3 != r_buf) goto neq_flag;

    l_buf = l_buf2 >> (8 * (8 - r_off));
    l += 8;
    r += 8;
    n -= 8;
  }

  if (!n) return 0;

  /* 5. Compare final bytes */

  if (n <= r_off) l_buf2 = 0; else l_buf2 = read_u64 (l);
  r_buf = read_u64 (r);
  l_buf3 = l_buf | (l_buf2 << (8 * r_off));

  while (n && (l_buf3 & 0xff) == (r_buf & 0xff)) { l_buf3 >>= 8; r_buf >>= 8; n--; }

  if (!n) return 0;
  return ((int32_t) (l_buf3 & 0xff)) - ((int32_t) (r_buf & 0xff));

neq_flag:
  while ((l_buf3 & 0xff) == (r_buf & 0xff)) { l_buf3 >>= 8; r_buf >>= 8; }
  return ((int32_t) (l_buf3 & 0xff)) - ((int32_t) (r_buf & 0xff));
}
