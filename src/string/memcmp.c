/* memcmp.c
   Derived from musl-libc src/string/memcmp.c
 */

#include <stddef.h>

int memcmp (const void * vl, const void * vr, size_t n) {
  const unsigned char * l = vl, * r = vr;
  for (/* empty */; n && *l == *r; n--, l++, r++);
  return n ? *l - *r : 0;
}
