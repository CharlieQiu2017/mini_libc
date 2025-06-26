#include <stddef.h>
#include <stdint.h>

#define ONES ((size_t) -1 / 255) /* 0x0101010101010101 */
#define HIGHS (ONES * 128) /* 0x8080808080808080 */
#define HASZERO(x) (((x) - ONES) & ~ (x) & HIGHS)

int strncmp (const char * sl, const char * sr, size_t n) {
  const unsigned char * l = (const unsigned char *) sl;
  const unsigned char * r = (const unsigned char *) sr;

  /* 1. Compare initial bytes until L is aligned */
  while (n && ((uintptr_t) l & 7) && *l && *l == *r) { l++; r++; n--; }
  if (!n) return 0;
  if (*l != *r) return ((int) *l) - ((int) *r);
  if (*l == 0) return 0;

  /* 2. If R is also aligned, compare 8 bytes of L, R at a time */
  uint64_t l_buf, l_buf2, l_buf3, r_buf, end_flag = 0, neq_flag = 0;

  if (((uintptr_t) r & 7) == 0) {
    while (n >= 8) {
      l_buf = * ((const uint64_t *) l);
      r_buf = * ((const uint64_t *) r);
      if (HASZERO (l_buf) || l_buf != r_buf) break;
      l += 8;
      r += 8;
      n -= 8;
    }

    if (!n) return 0;

    if (n < 8) {
      l_buf = * ((const uint64_t *) l);
      r_buf = * ((const uint64_t *) r);

      while (n && (l_buf & 0xff) && ((l_buf & 0xff) == (r_buf & 0xff))) { l_buf >>= 8; r_buf >>= 8; n--; }
      if (!n) return 0;
      return ((int) (l_buf & 0xff)) - ((int) (r_buf & 0xff));
    }

    while ((l_buf & 0xff) && ((l_buf & 0xff) == (r_buf & 0xff))) { l_buf >>= 8; r_buf >>= 8; }
    return ((int) (l_buf & 0xff)) - ((int) (r_buf & 0xff));
  }

  uint32_t r_off = (uintptr_t) r & 7;
  l_buf = * ((const uint64_t *) l);
  r_buf = * ((const uint64_t *) (r - r_off));
  r_buf >>= 8 * r_off;

  /* 3. Compare the next (8 - r_off) bytes */

  uint32_t i = 8 - r_off;
  while (i && n && (l_buf & 0xff) && (l_buf & 0xff) == (r_buf & 0xff)) { l_buf >>= 8; r_buf >>= 8; i--; n--; }
  if (!n) return 0;
  if (i) return ((int) (l_buf & 0xff)) - ((int) (r_buf & 0xff));

  l += 8;
  r += (8 - r_off);

  /* 4. Repeat read and compare 8 bytes of L, R */
  while (n >= 8) {
    l_buf2 = * ((const uint64_t *) l);
    r_buf = * ((const uint64_t *) r);
    l_buf3 = l_buf | (l_buf2 << (8 * r_off));

    if (HASZERO (l_buf2)) { end_flag = 1; break; }
    if (l_buf3 != r_buf) { neq_flag = 1; break; }

    l_buf = l_buf2 >> (8 * (8 - r_off));
    l += 8;
    r += 8;
    n -= 8;
  }

  if (!n) return 0;

  /* 5. Compare final bytes
     There are three different ways the above loop could end:
     1. n < 8;
     2. l_buf2 contains a zero byte;
     3. l_buf3 != r_buf.

     In the first case, we compare the remaining bytes one by one.
     Notice that there are still r_off bytes remaining in l_buf uncompared.

     In the second case, we first compare the bytes in l_buf3,
     and then the remaining bytes in l_buf2.

     In the third case, we simply compare the bytes in l_buf3.
   */

  if (n < 8) {
    /* At this point, all bytes of R upto *r have been compared,
       they are all equal to L, and there are no NUL bytes.
       We read the next 8 bytes of R. Since n < 8, we do not need
       further bytes of R.
     */
    r_buf = * ((const uint64_t *) r);
    /* There are still r_off bytes in l_buf not yet compared, compare them */
    i = r_off;
    while (i && n && (l_buf & 0xff) && (l_buf & 0xff) == (r_buf & 0xff)) { l_buf >>= 8; r_buf >>= 8; i--; n--; }
    if (!n) return 0;
    if (i) { return ((int) (l_buf & 0xff)) - ((int) (r_buf & 0xff)); }

    /* If we reach this point, then there are (8 - r_off) bytes
       remaining in r_buf, but n < 8 - r_off.
     */
    l_buf = * ((const uint64_t *) l);
    while (n && (l_buf & 0xff) && (l_buf & 0xff) == (r_buf & 0xff)) { l_buf >>= 8; r_buf >>= 8; n--; }
    if (!n) return 0;
    return ((int) (l_buf & 0xff)) - ((int) (r_buf & 0xff));
  }

  if (end_flag) {
    /* If we reach this point, then n >= 8, hence no need to check
       for n in the first part.
     */
    i = 8;
    while (i && (l_buf3 & 0xff) && (l_buf3 & 0xff) == (r_buf & 0xff)) { l_buf3 >>= 8; r_buf >>= 8; i--; }
    if (i) return ((int) (l_buf3 & 0xff)) - ((int) (r_buf & 0xff));

    /* If we reach this point, then the NUL byte in l_buf2 must be
       in the highest r_off bytes of l_buf2.
     */
    l_buf = l_buf2 >> (8 * (8 - r_off));
    r += 8;
    n -= 8;
    r_buf = * ((const uint64_t *) r);

    while (n && (l_buf & 0xff) && (l_buf & 0xff) == (r_buf & 0xff)) { l_buf >>= 8; r_buf >>= 8; n--; }
    if (!n) return 0;
    return ((int) (l_buf & 0xff)) - ((int) (r_buf & 0xff));
  }

  if (neq_flag) {
    /* If we reach this point, then n >= 8, hence no need to check for n. */
    while ((l_buf3 & 0xff) && (l_buf3 & 0xff) == (r_buf & 0xff)) { l_buf3 >>= 8; r_buf >>= 8; }
    return ((int) (l_buf3 & 0xff)) - ((int) (r_buf & 0xff));
  }

  /* Should not reach here */
  return 0;
}
