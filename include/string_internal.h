#ifndef STRING_INTERNAL_H
#define STRING_INTERNAL_H

#include <stdint.h>

/* If ptr is 8-byte-aligned, we shall assume that read_u64(ptr) always returns
   the 8 bytes stored at this memory address, and does not induce UB.

   We considered other alternatives, such as:

   typedef uint64_t __attribute__((may_alias)) uint64_alias_t;
   return *((uint64_alias_t *) ptr);

   However, there is a problem with this approach.
   Suppose that you provide a 9-byte input to memcpy.
   Then memcpy will call read_u64 on the address of the first byte to read
   the first 8 bytes. It will also call read_u64 on the address of the final
   byte to read the last byte. However, this is actually UB, since we are
   also reading 7 bytes past the end of the buffer.

   We could achieve the same effect with volatile, but that still does not
   work around the theoretical UB. The only way to avoid UB is to use inline
   assembly to read the address.
 */

static inline __attribute__((always_inline)) uint64_t read_u64 (const void * ptr) {
  uint64_t ret;
  __asm__ volatile (
    "ldr %[ret], [%[ptr]]"
  : [ret] "=r" (ret)
  : [ptr] "r" (ptr)
  :
  );
  return ret;
}

/* Writing to 8 bytes in memory simply requires may_alias attribute. */

typedef uint64_t __attribute__((may_alias)) uint64_alias_t;

#endif
