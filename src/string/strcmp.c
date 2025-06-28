#include <stddef.h>
#include <stdint.h>

/* Surprise! On aarch64 platforms "char" is "unsigned char"! */

/* See strlen.c */
#define ONES ((size_t) -1 / 255) /* 0x0101010101010101 */
#define HIGHS (ONES * 128) /* 0x8080808080808080 */
#define HASZERO(x) (((x) - ONES) & ~ (x) & HIGHS)

int32_t strcmp (const char * sl, const char * sr) {
  const unsigned char * l = (const unsigned char *) sl;
  const unsigned char * r = (const unsigned char *) sr;

  /* 1. Compare initial bytes until L is aligned */
  while (((uintptr_t) l & 7) && *l && *l == *r) { ++l; ++r; }
  if (*l != *r) { return ((int32_t) *l) - ((int32_t) *r); }
  if (*l == 0) return 0;

  /* 2. If R is also aligned, compare 8 bytes of L, R at a time */
  uint64_t l_buf, l_buf2, l_buf3, r_buf;

  if (((uintptr_t) r & 7) == 0) {
    while (1) {
      l_buf = * ((const uint64_t *) l);
      r_buf = * ((const uint64_t *) r);
      if (HASZERO (l_buf) || l_buf != r_buf) break;
      l += 8;
      r += 8;
    }

    while ((l_buf & 0xff) && ((l_buf & 0xff) == (r_buf & 0xff))) { l_buf >>= 8; r_buf >>= 8; }
    return ((int32_t) (l_buf & 0xff)) - ((int32_t) (r_buf & 0xff));
  }

  uint32_t r_off = (uintptr_t) r & 7;
  l_buf = * ((const uint64_t *) l);
  r_buf = * ((const uint64_t *) (r - r_off));
  r_buf >>= 8 * r_off;

  /* 3. Compare the next (8 - r_off) bytes */

  uint32_t l_end_flag = HASZERO (l_buf) != 0;

  uint32_t i = 8 - r_off;
  while (i && (l_buf & 0xff) && (l_buf & 0xff) == (r_buf & 0xff)) { l_buf >>= 8; r_buf >>= 8; i--; }
  if (i) return ((int32_t) (l_buf & 0xff)) - ((int32_t) (r_buf & 0xff));

  l += 8;
  r += (8 - r_off);

  if (l_end_flag) {
    r_buf = * ((const uint64_t *) r);
    while ((l_buf & 0xff) && (l_buf & 0xff) == (r_buf & 0xff)) { l_buf >>= 8; r_buf >>= 8; }
    return ((int32_t) (l_buf & 0xff)) - ((int32_t) (r_buf & 0xff));
  }

  /* 4. Repeat read and compare 8 bytes of L, R */
  while (1) {
    l_buf2 = * ((const uint64_t *) l);
    r_buf = * ((const uint64_t *) r);
    l_buf3 = l_buf | (l_buf2 << (8 * r_off));
    if (HASZERO (l_buf2)) goto l_end_flag;
    if (l_buf3 != r_buf) goto neq_flag;

    l_buf = l_buf2 >> (8 * (8 - r_off));
    l += 8;
    r += 8;
  }

  /* 5. Compare final bytes */

l_end_flag:
  i = 8;
  while (i && (l_buf3 & 0xff) && (l_buf3 & 0xff) == (r_buf & 0xff)) { l_buf3 >>= 8; r_buf >>= 8; i--; }
  if (i) return ((int32_t) (l_buf3 & 0xff)) - ((int32_t) (r_buf & 0xff));

  l_buf = l_buf2 >> (8 * (8 - r_off));
  r += 8;
  r_buf = * ((const uint64_t *) r);

  while ((l_buf & 0xff) && (l_buf & 0xff) == (r_buf & 0xff)) { l_buf >>= 8; r_buf >>= 8; }
  return ((int32_t) (l_buf & 0xff)) - ((int32_t) (r_buf & 0xff));

neq_flag:
  while ((l_buf3 & 0xff) && (l_buf3 & 0xff) == (r_buf & 0xff)) { l_buf3 >>= 8; r_buf >>= 8; }
  return ((int32_t) (l_buf3 & 0xff)) - ((int32_t) (r_buf & 0xff));
}
