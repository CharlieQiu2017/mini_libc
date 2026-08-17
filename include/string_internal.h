#ifndef STRING_INTERNAL_H
#define STRING_INTERNAL_H

#include <stdint.h>

/* If ptr is 8-byte-aligned, and at least one byte between ptr and ptr+7
   can be read, we shall assume that read_u64(ptr) always returns
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

   Also notice that the type of ptr is uintptr_t rather than const void *.
   The purpose is to let the compiler "forget" any provenance information
   the pointer may carry.
 */

static inline __attribute__((always_inline)) uint64_t read_u64 (uintptr_t ptr) {
  uint64_t ret;
  __asm__ volatile (
    "ldr %[ret], [%[ptr]]"
  : [ret] "=r" (ret)
  : [ptr] "r" (ptr)
  :
  );
  return ret;
}

/* I used to think that writing to aligned 8 bytes safely can be achieve using

   typedef uint64_t __attribute__((may_alias)) uint64_alias_t;

   However this is not strictly correct. The reason is that the 8 bytes do not
   need to belong to the same allocation unit, but two allocation units that
   happen to be contiguous. For example, if two variables are allocated on the
   stack, their addresses would be contiguous, but the compiler may assume that
   pointer to the first variable may never be used to access the second variable.
   Therefore, we also use inline assembly to write to aligned 8 bytes.
 */

static inline __attribute__((always_inline)) void write_u64 (uintptr_t ptr, uint64_t val) {
  __asm__ volatile (
    "str %[val], [%[ptr]]"
  :
  : [ptr] "r" (ptr), [val] "r" (val)
  : "memory"
  );
}

/* Memory barrier that prevents the compiler from making assumptions on pointer provenance */
/* Since char is the smallest unit of memory, the pointer obtained this way can always
   be used to access that *exact* char without provenance issues */
static inline __attribute__((always_inline)) unsigned char * ptr_from_uint (uintptr_t ptr) {
  __asm__ ("" : "+r" (ptr));
  return (unsigned char *) ptr;
}

static inline __attribute__((always_inline)) const unsigned char * const_ptr_from_uint (uintptr_t ptr) {
  __asm__ ("" : "+r" (ptr));
  return (const unsigned char *) ptr;
}

#endif
