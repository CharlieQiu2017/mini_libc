/* memcmp_zero.c
   Like memcmp, but checks whether a region of memory is completely zero.
   Returns 0 if the region is completely zero, and a non-zero value if it is not
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <string_internal.h>

uint64_t memcmp_zero (const void * v, size_t n) {
  uintptr_t vp = (uintptr_t) v;
  const unsigned char * p;
  uint64_t result = 0;

  /* 1. Compare initial bytes until p is aligned */

  while (n && (vp & 7)) {
    p = const_ptr_from_uint (vp);
    result |= *p;
    vp++; n--;
  }
  if (!n) return result;

  /* 2. Compare 8 bytes at a time */

  while (n >= 8) {
    uint64_t buf = read_u64 (vp);
    result |= buf;
    vp += 8; n -= 8;
  }
  if (!n) return result;

  /* 3. Compare final bytes */

  while (n) {
    p = const_ptr_from_uint (vp);
    result |= *p;
    vp++; n--;
  }
  return result;
}
