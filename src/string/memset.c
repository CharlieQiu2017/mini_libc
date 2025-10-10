/* memset.c
   Derived from musl-libc src/string/memset.c
 */

#include <string.h>
#include <stdint.h>
#include <string_internal.h>

void * memset (void * dest, uint32_t c, size_t n) {
  unsigned char * d = dest;
  c &= 0xff;
  uint64_t c_long = ((uint64_t) c) * 0x0101010101010101;

  while (n && ((uintptr_t) d & 7)) { *d = c; d++; n--; }

  while (n >= 8) {
    *((uint64_alias_t *) d) = c_long;
    d += 8; n -= 8;
  }

  while (n) { *d = c; d++; n--; }

  return dest;
}
