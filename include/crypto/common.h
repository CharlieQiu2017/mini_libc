/* Constant time helper functions for cryptography */

#ifndef CRYPTO_COMMON_H
#define CRYPTO_COMMON_H

#include <stddef.h>
#include <stdint.h>

/* This function simply returns its input.
   It prevents the compiler from making assumptions on the value of input.
 */
static inline uint8_t value_barrier (uint8_t input) {
  asm (
    ""
  : [input_reg] "+r" (input)
  :
  :
  );
  return input;
}

/* If input is non-zero, return 1, otherwise return 0 */
static inline uint8_t uint8_to_bool (uint8_t input) {
  uint8_t output;
  asm (
    "\tcmp %[input_reg], #0\n"
    "\tcset %[output_reg], ne\n"
  : [output_reg] "=r" (output)
  : [input_reg] "r" (input)
  : "cc"
  );
  return output;
}

/* If input is non-zero, return a, otherwise return b */
static inline uint8_t uint8_branch (uint8_t input, uint8_t a, uint8_t b) {
  return uint8_to_bool (input) * (a - b) + b;
}

/* If a >= b return 1, otherwise return 0 */
static inline uint8_t int8_cmp_ge (int8_t a, int8_t b) {
  uint8_t output;
  asm (
    "\tcmp %[a_reg], %[b_reg]\n"
    "\tcset %[output_reg], ge\n"
  : [output_reg] "=r" (output)
  : [a_reg] "r" (a), [b_reg] "r" (b)
  : "cc"
  );
  return output;
}

/* Conditional memcpy, cond is assumed to be 0 or 1, performs memcpy only if cond is 1 */
static inline void cond_memcpy (uint8_t cond, void * restrict dst, void * restrict src, size_t n) {
  uint8_t mask_src = cond * 0xff;
  uint8_t mask_dst = (1 - cond) * 0xff;
  unsigned char * dst_ptr = (unsigned char *) dst;
  unsigned char * src_ptr = (unsigned char *) src;
  for (uint32_t i = 0; i < n; ++i) {
    dst_ptr[i] = (dst_ptr[i] & mask_dst) | (src_ptr[i] & mask_src);
  }
}

#endif
