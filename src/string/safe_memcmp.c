/* safe_memcmp.c
   Cryptographically-safe version of memcmp
   Checks whether two memory regions are identical in constant time
   Returns *false* (0) when the two regions are identical, to remain compatible with memcmp
   Returns non-zero value when two regions are different.
 */

#include <stddef.h>
#include <stdbool.h>

unsigned char safe_memcmp (const void * vl, const void * vr, size_t n) {
  const unsigned char * l = vl, * r = vr;
  unsigned char result = 0;
  for (size_t i = 0; i < n; ++i) {
    result |= l[i] ^ r[i];
  }
  return result;
}
