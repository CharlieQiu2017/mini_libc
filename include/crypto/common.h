/* Constant time helper functions for cryptography */

#ifndef CRYPTO_COMMON_H
#define CRYPTO_COMMON_H

#include <stddef.h>
#include <stdint.h>

/* This function simply returns its input.
   It prevents the compiler from making assumptions on the value of input.
 */
static inline uint32_t uint32_value_barrier (uint32_t input) {
  __asm__ (
    ""
  : [input_reg] "+r" (input)
  :
  :
  );
  return input;
}

static inline uint64_t uint64_value_barrier (uint64_t input) {
  __asm__ (
    ""
  : [input_reg] "+r" (input)
  :
  :
  );
  return input;
}

static inline _Bool bool_value_barrier (_Bool input) {
  __asm__ (
    ""
  : [input_reg] "+r" (input)
  :
  :
  );
  return input;
}

/* Boolean operations that are guaranteed not to short circuit.
   Although C bitwise boolean operators are supposed not to short circuit,
   the compiler may still short circuit if it determines the operands have no side-effects.
   Therefore, we use these functions to ensure AND and OR never short circuits.
 */
static inline _Bool bool_and (_Bool input1, _Bool input2) {
  _Bool output;
  __asm__ (
    "and %w[output_reg], %w[input1_reg], %w[input2_reg]"
  : [output_reg] "=r" (output)
  : [input1_reg] "r" (input1), [input2_reg] "r" (input2)
  :
  );
  return output;
}

static inline _Bool bool_or (_Bool input1, _Bool input2) {
  _Bool output;
  __asm__ (
    "orr %w[output_reg], %w[input1_reg], %w[input2_reg]"
  : [output_reg] "=r" (output)
  : [input1_reg] "r" (input1), [input2_reg] "r" (input2)
  :
  );
  return output;
}

/* If input x is not zero, return 1, otherwise return 0 */
static inline _Bool uint32_to_bool (uint32_t x) {
  _Bool output;
  __asm__ (
    "cmp %w[input_reg], #0\n\t"
    "cset %w[output_reg], ne"
  : [output_reg] "=r" (output)
  : [input_reg] "r" (x)
  : "cc"
  );
  return output;
}

static inline _Bool uint64_to_bool (uint64_t x) {
  _Bool output;
  __asm__ (
    "cmp %[input_reg], #0\n\t"
    "cset %[output_reg], ne"
  : [output_reg] "=r" (output)
  : [input_reg] "r" (x)
  : "cc"
  );
  return output;
}

/* If x >= y, return a, otherwise return b */
static inline uint32_t uint32_cmp_ge_branch (uint32_t x, uint32_t y, uint32_t a, uint32_t b) {
  uint32_t output;
  __asm__ (
    "cmp %w[x_reg], %w[y_reg]\n\t"
    "csel %w[output_reg], %w[a_reg], %w[b_reg], hs"
  : [output_reg] "=r" (output)
  : [x_reg] "r" (x), [y_reg] "r" (y), [a_reg] "r" (a), [b_reg] "r" (b)
  : "cc"
  );
  return output;
}

static inline uint64_t uint64_cmp_ge_branch (uint64_t x, uint64_t y, uint64_t a, uint64_t b) {
  uint64_t output;
  __asm__ (
    "cmp %[x_reg], %[y_reg]\n\t"
    "csel %[output_reg], %[a_reg], %[b_reg], hs"
  : [output_reg] "=r" (output)
  : [x_reg] "r" (x), [y_reg] "r" (y), [a_reg] "r" (a), [b_reg] "r" (b)
  : "cc" );
  return output;
}

/* If a >= b return 1, otherwise return 0 */
static inline _Bool int32_cmp_ge (int32_t a, int32_t b) {
  _Bool output;
  __asm__ (
    "cmp %w[a_reg], %w[b_reg]\n\t"
    "cset %w[output_reg], ge"
  : [output_reg] "=r" (output)
  : [a_reg] "r" (a), [b_reg] "r" (b)
  : "cc"
  );
  return output;
}

static inline _Bool int64_cmp_ge (int64_t a, int64_t b) {
  _Bool output;
  __asm__ (
    "cmp %[a_reg], %[b_reg]\n\t"
    "cset %[output_reg], ge"
  : [output_reg] "=r" (output)
  : [a_reg] "r" (a), [b_reg] "r" (b)
  : "cc"
  );
  return output;
}

static inline _Bool uint32_cmp_ge (uint32_t a, uint32_t b) {
  _Bool output;
  __asm__ (
    "cmp %w[a_reg], %w[b_reg]\n\t"
    "cset %w[output_reg], hs"
  : [output_reg] "=r" (output)
  : [a_reg] "r" (a), [b_reg] "r" (b)
  : "cc"
  );
  return output;
}

static inline _Bool uint64_cmp_ge (uint64_t a, uint64_t b) {
  _Bool output;
  __asm__ (
    "cmp %[a_reg], %[b_reg]\n\t"
    "cset %[output_reg], hs"
  : [output_reg] "=r" (output)
  : [a_reg] "r" (a), [b_reg] "r" (b)
  : "cc"
  );
  return output;
}

/* Return the smaller one of a and b */
static inline uint32_t uint32_min (uint32_t a, uint32_t b) {
  uint32_t output;
  __asm__ (
    "cmp %w[a_reg], %w[b_reg]\n\t"
    "csel %w[output_reg], %w[a_reg], %w[b_reg], ls\n\t"
  : [output_reg] "=r" (output)
  : [a_reg] "r" (a), [b_reg] "r" (b)
  : "cc"
  );
  return output;
}

static inline uint64_t uint64_min (uint64_t a, uint64_t b) {
  uint64_t output;
  __asm__ (
    "cmp %[a_reg], %[b_reg]\n\t"
    "csel %[output_reg], %[a_reg], %[b_reg], ls\n\t"
  : [output_reg] "=r" (output)
  : [a_reg] "r" (a), [b_reg] "r" (b)
  : "cc"
  );
  return output;
}

#endif
