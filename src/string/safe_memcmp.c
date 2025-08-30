/* safe_memcmp.c
   Cryptographically-safe version of memcmp
   Checks whether two memory regions are identical in constant time
   Returns *false* (0) when the two regions are identical, to remain compatible with memcmp
   Returns non-zero value when two regions are different.
 */

#include <stddef.h>
#include <stdint.h>

uint64_t safe_memcmp (const void * vl, const void * vr, size_t n) {
  const unsigned char * l = vl, * r = vr;
  uint64_t result = 0;

  while (n && ((uintptr_t) l & 7)) { result |= *l ^ *r; l++; r++; n--; }
  if (!n) return result;

  uint64_t l_buf, l_buf2, l_buf3, r_buf, mask;

  if (((uintptr_t) r & 7) == 0) {
    while (n >= 8) {
      l_buf = * ((const uint64_t *) l);
      r_buf = * ((const uint64_t *) r);
      result |= l_buf ^ r_buf;

      l += 8; r += 8; n -= 8;
    }

    if (!n) return result;

    l_buf = * ((const uint64_t *) l);
    r_buf = * ((const uint64_t *) r);

    mask = ((uint64_t) -1) << (8 * n);
    l_buf |= mask;
    r_buf |= mask;
    result |= l_buf ^ r_buf;

    return result;
  }

  uint32_t r_off = (uintptr_t) r & 7;
  l_buf = * ((const uint64_t *) l);
  r_buf = * ((const uint64_t *) (r - r_off));
  r_buf >>= 8 * r_off;

  if (n <= 8 - r_off) {
    mask = ((uint64_t) -1) << (8 * n);
    l_buf |= mask;
    r_buf |= mask;
    result |= l_buf ^ r_buf;
    return result;
  }

  mask = ((uint64_t) -1) << (8 * (8 - r_off));
  result |= (l_buf | mask) ^ (r_buf | mask);

  l_buf >>= (8 * (8 - r_off));
  l += 8;
  r += (8 - r_off);
  n -= (8 - r_off);

  while (n >= 8) {
    l_buf2 = * ((const uint64_t *) l);
    r_buf = * ((const uint64_t *) r);
    l_buf3 = l_buf | (l_buf2 << (8 * r_off));
    result |= l_buf3 ^ r_buf;

    l_buf = l_buf2 >> (8 * (8 - r_off));
    l += 8;
    r += 8;
    n -= 8;
  }

  if (!n) return 0;

  if (n <= r_off) l_buf2 = 0; else l_buf2 = * ((const uint64_t *) l);
  r_buf = * ((const uint64_t *) r);
  l_buf3 = l_buf | (l_buf2 << (8 * r_off));

  mask = ((uint64_t) -1) << (8 * n);
  l_buf3 |= mask;
  r_buf |= mask;
  result |= l_buf3 ^ r_buf;

  return result;
}
