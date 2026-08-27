#ifndef CROSS_FP_ARITH_H
#define CROSS_FP_ARITH_H

#include <crypto/common.h>
#include <crypto/sign/cross/parameters.h>

static inline uint16_t fp_add (uint16_t x, uint16_t y) {
  uint32_t sum = x;
  sum += y;
  return uint32_cmp_ge_branch (sum, CROSS_P, sum - CROSS_P, sum);
}

static inline uint16_t fp_neg (uint16_t x) { return uint32_cmp_ge_branch (x, 1, CROSS_P - x, 0); }

static inline uint16_t fp_mult (uint16_t x, uint16_t y) {
  uint64_t prod = x;
  prod *= y;
  uint64_t div = (prod * CROSS_P_C1) >> CROSS_P_C2;
  return prod - div * CROSS_P;
}

#endif
