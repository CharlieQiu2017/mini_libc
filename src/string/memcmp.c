/* memcmp.c
   Derived from musl-libc src/string/memcmp.c
 */

/* Surprise! On aarch64 platforms "char" is "unsigned char"! */

#include <stddef.h>
#include <stdint.h>

int32_t memcmp (const void * vl, const void * vr, size_t n) {
  const unsigned char * l = (const unsigned char *) vl;
  const unsigned char * r = (const unsigned char *) vr;

  /* 1. Compare initial bytes until L is aligned */

  while (n && ((uintptr_t) l & 7) && *l == *r) { l++; r++; n--; }
  if (!n) return 0;
  if (*l != *r) return ((int32_t) *l) - ((int32_t) *r);

  uint64_t l_buf, l_buf2, l_buf3, r_buf;

  /* 2. If R is also aligned, compare 8 bytes of L, R at a time */

  if (((uintptr_t) r & 7) == 0) {
    while (n >= 8) {
      l_buf = * ((const uint64_t *) l);
      r_buf = * ((const uint64_t *) r);

      if (l_buf != r_buf) {
	while ((l_buf & 0xff) == (r_buf & 0xff)) { l_buf >>= 8; r_buf >>= 8; }
	return ((int32_t) (l_buf & 0xff)) - ((int32_t) (r_buf & 0xff));
      }

      l += 8; r += 8; n -= 8;
    }

    if (!n) return 0;

    l_buf = * ((const uint64_t *) l);
    r_buf = * ((const uint64_t *) r);

    while (n && (l_buf & 0xff) == (r_buf & 0xff)) { l_buf >>= 8; r_buf >>= 8; n--; }

    if (!n) return 0;

    return ((int32_t) (l_buf & 0xff)) - ((int32_t) (r_buf & 0xff));
  }

  uint32_t r_off = (uintptr_t) r & 7;
  l_buf = * ((const uint64_t *) l);
  r_buf = * ((const uint64_t *) (r - r_off));
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
    l_buf2 = * ((const uint64_t *) l);
    r_buf = * ((const uint64_t *) r);
    l_buf3 = l_buf | (l_buf2 << (8 * r_off));

    if (l_buf3 != r_buf) goto neq_flag;

    l_buf = l_buf2 >> (8 * (8 - r_off));
    l += 8;
    r += 8;
    n -= 8;
  }

  if (!n) return 0;

  /* 5. Compare final bytes */

  if (n <= r_off) l_buf2 = 0; else l_buf2 = * ((const uint64_t *) l);
  r_buf = * ((const uint64_t *) r);
  l_buf3 = l_buf | (l_buf2 << (8 * r_off));

  while (n && (l_buf3 & 0xff) == (r_buf & 0xff)) { l_buf3 >>= 8; r_buf >>= 8; n--; }

  if (!n) return 0;
  return ((int32_t) (l_buf3 & 0xff)) - ((int32_t) (r_buf & 0xff));

neq_flag:
  while ((l_buf3 & 0xff) == (r_buf & 0xff)) { l_buf3 >>= 8; r_buf >>= 8; }
  return ((int32_t) (l_buf3 & 0xff)) - ((int32_t) (r_buf & 0xff));
}
