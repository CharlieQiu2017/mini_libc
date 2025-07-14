/* cond_memcpy.c
   Cryptographically-safe conditional memcpy.
   cond is either 0 or 1.
   If cond is 0, no action is performed.
   If cond is 1, dst is overwritten with src.
 */

#include <stddef.h>
#include <stdint.h>

void cond_memcpy (uint8_t cond, void * restrict vd, const void * restrict vs, size_t n) {
  const unsigned char * s = (const unsigned char *) vs;
  unsigned char * d = (unsigned char *) vd;

  uint8_t mask_src = cond * 0xff;
  uint8_t mask_dst = (1 - cond) * 0xff;

  uint64_t mask_src_long = cond * ((uint64_t) -1);
  uint64_t mask_dst_long = (1 - cond) * ((uint64_t) -1);

  while (n && ((uintptr_t) s & 7)) {
    *d = (*d & mask_dst) | (*s & mask_src);
    s++; d++; n--;
  }

  if (!n) return;

  uint64_t s_buf, s_buf2, s_buf3, d_buf;

  if (((uintptr_t) d & 7) == 0) {
    while (n >= 8) {
      s_buf = * ((const uint64_t *) s);
      d_buf = * ((uint64_t *) d);
      d_buf = (s_buf & mask_src_long) | (d_buf & mask_dst_long);
      * ((uint64_t *) d) = d_buf;

      s += 8; d += 8; n -= 8;
    }

    if (!n) return;

    s_buf = * ((const uint64_t *) s);
    d_buf = * ((uint64_t *) d);
    d_buf = (s_buf & mask_src_long) | (d_buf & mask_dst_long);

    while (n) {
      *d = d_buf & 0xff;
      d_buf >>= 8; d++; n--;
    }

    return;
  }

  uint32_t d_off = (uintptr_t) d & 7;
  s_buf = * ((const uint64_t *) s);
  d_buf = * ((uint64_t *) (d - d_off));
  d_buf >>= 8 * d_off;
  d_buf = (s_buf & mask_src_long) | (d_buf & mask_dst_long);

  uint32_t i = 8 - d_off;
  while (i && n) { *d = d_buf & 0xff; d_buf >>= 8; d++; i--; n--; }
  if (!n) return;
  s_buf >>= 8 * (8 - d_off);

  while (n >= 8) {
    s_buf2 = * ((const uint64_t *) s);
    d_buf = * ((uint64_t *) d);
    s_buf3 = s_buf | (s_buf2 << (8 * d_off));
    d_buf = (s_buf3 & mask_src_long) | (d_buf & mask_dst_long);
    * ((uint64_t *) d) = d_buf;

    s_buf = s_buf2 >> 8 * (8 - d_off);
    s += 8; d += 8; n -= 8;
  }

  if (!n) return;

  if (n <= d_off) s_buf2 = 0; else s_buf2 = * ((const uint64_t *) s);
  d_buf = * ((uint64_t *) d);
  s_buf3 = s_buf | (s_buf2 << (8 * d_off));
  d_buf = (s_buf3 & mask_src_long) | (d_buf & mask_dst_long);

  while (n) {
    *d = d_buf & 0xff;
    d++; d_buf >>= 8; n--;
  }

  return;
}
