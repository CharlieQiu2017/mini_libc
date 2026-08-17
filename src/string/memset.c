/* memset.c
   Derived from musl-libc src/string/memset.c
 */

#include <string.h>
#include <stdint.h>
#include <string_internal.h>

void * memset (void * dest, uint32_t c, size_t n) {
  uintptr_t d = (uintptr_t) dest;
  unsigned char * dp;
  c &= 0xff;
  uint64_t c_long = ((uint64_t) c) * 0x0101010101010101;

  while (n && (d & 7)) { dp = ptr_from_uint (d); *dp = c; d++; n--; }

  while (n >= 8) {
    write_u64 (d, c_long);
    d += 8; n -= 8;
  }

  while (n) { dp = ptr_from_uint (d); *dp = c; d++; n--; }

  return dest;
}
