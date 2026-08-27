#ifndef CROSS_FZ_ARITH_H
#define CROSS_FZ_ARITH_H

#include <crypto/common.h>
#include <crypto/sign/cross/parameters.h>
#include <crypto/sign/cross/fp_arith.h>

static inline uint8_t fz_add (uint8_t x, uint8_t y) {
  uint32_t sum = x;
  sum += y;
  return uint32_cmp_ge_branch (sum, CROSS_Z, sum - CROSS_Z, sum);
}

static inline uint8_t fz_neg (uint8_t x) { return uint32_cmp_ge_branch (x, 1, CROSS_Z - x, 0); }

static inline uint8_t fz_mult (uint8_t x, uint8_t y) {
  uint64_t prod = x;
  prod *= y;
  uint64_t div = (prod * CROSS_Z_C1) >> CROSS_Z_C2;
  return prod - div * CROSS_Z;
}

static inline uint16_t fz_to_fp (uint8_t x) {
  uint32_t result = uint32_cmp_ge_branch (x & 0x01, 1, CROSS_G, 1);
  uint32_t tmp;

  tmp = fp_mult (result, CROSS_G_2);
  result = uint32_cmp_ge_branch (x & 0x02, 1, tmp, result);

  tmp = fp_mult (result, CROSS_G_4);
  result = uint32_cmp_ge_branch (x & 0x04, 1, tmp, result);

  tmp = fp_mult (result, CROSS_G_8);
  result = uint32_cmp_ge_branch (x & 0x08, 1, tmp, result);

  tmp = fp_mult (result, CROSS_G_16);
  result = uint32_cmp_ge_branch (x & 0x10, 1, tmp, result);

  tmp = fp_mult (result, CROSS_G_32);
  result = uint32_cmp_ge_branch (x & 0x20, 1, tmp, result);

  tmp = fp_mult (result, CROSS_G_64);
  result = uint32_cmp_ge_branch (x & 0x40, 1, tmp, result);

  return result;
}

#endif
