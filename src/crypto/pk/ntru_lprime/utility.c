#include <stdint.h>
#include <string.h>
#include <crypto/common.h>
#include <crypto/sort.h>
#include <crypto/sk/aes/aes.h>
#include <crypto/hash/keccak/keccak_p.h>
#include <crypto/pk/ntru_lprime/ntru_lprime.h>

#define NTRU_LPR_P 653
#define NTRU_LPR_Q 4621
#define NTRU_LPR_W 252
#define NTRU_LPR_SHORT_ENC_LEN ((NTRU_LPR_P - 1) / 4 + 1)

/* Barrett reduction requires a good approximation of 1/Q, in the form of m/2^k.
   The maximum number we have to reduce against Q = 4621 is 2^32 - 1.
   We have to ensure that for every x <= 2^32 - 1 we have floor (x * m / 2^k) = floor (x / Q).
   Assume that m / 2^k > 1/Q. Then for every x we have floor (x * m / 2^k) >= floor (x / Q).
   Suppose that for some x we have floor (x * m / 2^k) > floor (x / Q).
   This means r <= x / Q < r + 1 but x * m / 2^k >= r + 1.
   Note that if x / Q < r + 1 then (r + 1) - x / Q >= 1 / Q.
   Also, difference between x * m / 2^k and x / Q increases linearly as x increases.
   Thus if we can ensure that (x * m / 2^k) - (x / Q) < (1 / Q) for the largest possible x,
   it is impossible to have floor (x * m / 2^k) > floor (x / Q).
   By experiment it can be found that 1903504225 / 2^43 > 1 / 4621, and
   4294967295 * 1903504225 / 2^43 - 4294967295 / 4621 = 0.000160295...
   while 1/4621 = 0.000216403...
   Therefore (x * 1903504225) >> 43 is a good approximation of x / 4621.
   Also, 4294967295 * 1903504225 = 8175488392269321375 and does not overflow within 64-bit arithmetics.

   General steps in Mathematica to find Barrett reduction constants:
   1. Determine largest x that we will want to reduce.
   2. RealDigits[1 / Q, 2, 64]
   3. RealDigits[1 / Q + 1 / (Q * x), 2, 64]
   4. Truncate the result of step 3 at the first digit that diverges from step 2 (keep that digit).
   5. Convert the truncated number back to rational form.
 */

#define NTRU_LPR_K 43
#define NTRU_LPR_M 1903504225ull

/* In this file we assume all polynomials use the [0, Q-1] representation */

void ntrulpr_653_expand_seed (const unsigned char * seed, uint16_t * out_g) {
  uint32_t aes_exkey[60];
  uint32_t aes_in[4] = {0}, aes_out[4];
  uint32_t poly[NTRU_LPR_P];

  aes256_expandkey (seed, (unsigned char *) aes_exkey);

  for (uint32_t i = 0; i < NTRU_LPR_P; ++i) {
    if ((i & 0x03) == 0) {
      aes256_encrypt_one_block ((const unsigned char *) aes_exkey, (const unsigned char *) aes_in, (unsigned char *) aes_out);
      aes_in[0]++;
    }

    poly[i] = aes_out[i & 0x03];

    /* Barrett reduction */
    uint32_t div = (((uint64_t) poly[i]) * NTRU_LPR_M) >> NTRU_LPR_K;
    poly[i] = poly[i] - div * NTRU_LPR_Q;

    /* If p_i >= (q - 1)/2, subtract (q - 1)/2, otherwise add (q + 1)/2 */
    poly[i] = uint32_cmp_ge_branch (poly[i], (NTRU_LPR_Q - 1) / 2, poly[i] - (NTRU_LPR_Q - 1) / 2, poly[i] + (NTRU_LPR_Q + 1) / 2);
  }

  for (uint32_t i = 0; i < NTRU_LPR_P; ++i) {
    out_g[i] = poly[i];
  }
}

/* Multiply G with a where a is short (all coefficients in {-1, 0, 1}).
   We follow the encoding of short polynomials in the spec.
   Each coefficient of a is represented with 2 bits: 00 -> -1, 01 -> 0, 10 -> 1.
   Only compute the coefficients of the terms x^0, x^1, ..., x^(out_terms - 1).
 */
void ntrulpr_653_poly_mult_short (const uint16_t * g, const uint8_t * a, uint32_t out_terms, uint16_t * out) {
  /* g_times_xi means g * x^i. */
  uint16_t g_times_xi[NTRU_LPR_P];

  memset (out, 0, 2 * out_terms);
  memcpy (g_times_xi, g, 2 * NTRU_LPR_P);

  /* We want to avoid copying the entire g_times_xi array during each loop iteration.
     Therefore, we make the following convention: the coefficient of x^0 is stored in g_times_xi[NTRU_LPR_P - i],
     the next term is stored in g_times_xi[NTRU_LPR_P - i + 1],
     and so on until it wraps back to 0.
     The highest term is stored in g_times_xi[NTRU_LPR_P - i - 1].
     The first iteration is special, handle it separately.
   */

  /* Iteration 0 */
  uint32_t coeff0 = a[0] & 0x03;
  for (uint32_t j = 0; j < out_terms; ++j) {
    uint32_t addent = uint32_cmp_ge_branch (coeff0, 1, uint32_cmp_ge_branch (coeff0, 2, g_times_xi[j], 0), NTRU_LPR_Q - g_times_xi[j]);
    /* out[j] is zero, but g_times_xi[j] might be zero, so addent might need reduce */
    addent = uint32_cmp_ge_branch (addent, NTRU_LPR_Q, addent - NTRU_LPR_Q, addent);
    out[j] = addent;
  }

  /* At this point g_times_xi[NTRU_LPR_P - 1] becomes the new zeroth term */
  uint32_t linear_term0 = g_times_xi[0] + g_times_xi[NTRU_LPR_P - 1];
  linear_term0 = uint32_cmp_ge_branch (linear_term0, NTRU_LPR_Q, linear_term0 - NTRU_LPR_Q, linear_term0);
  g_times_xi[0] = linear_term0;

  for (uint32_t i = 1; i < NTRU_LPR_P; ++i) {
    uint32_t coeff = (a[i >> 2] >> (2 * (i & 0x03))) & 0x03;

    for (uint32_t j = NTRU_LPR_P - i; j < NTRU_LPR_P && j - (NTRU_LPR_P - i) < out_terms; ++j) {
      uint32_t addent = uint32_cmp_ge_branch (coeff, 1, uint32_cmp_ge_branch (coeff, 2, g_times_xi[j], 0), NTRU_LPR_Q - g_times_xi[j]);
      addent += out[j - (NTRU_LPR_P - i)];
      addent = uint32_cmp_ge_branch (addent, NTRU_LPR_Q, addent - NTRU_LPR_Q, addent);
      out[j - (NTRU_LPR_P - i)] = addent;
    }

    for (uint32_t j = 0; j < NTRU_LPR_P - i && j + i < out_terms; ++j) {
      uint32_t addent = uint32_cmp_ge_branch (coeff, 1, uint32_cmp_ge_branch (coeff, 2, g_times_xi[j], 0), NTRU_LPR_Q - g_times_xi[j]);
      addent += out[j + i];
      addent = uint32_cmp_ge_branch (addent, NTRU_LPR_Q, addent - NTRU_LPR_Q, addent);
      out[j + i] = addent;
    }

    /* Update g_times_xi
       The new zeroth term is g_times_xi[NTRU_LPR_P - i - 1]
     */

    uint32_t linear_term = g_times_xi[NTRU_LPR_P - i - 1] + g_times_xi[NTRU_LPR_P - i];
    linear_term = uint32_cmp_ge_branch (linear_term, NTRU_LPR_Q, linear_term - NTRU_LPR_Q, linear_term);
    g_times_xi[NTRU_LPR_P - i] = linear_term;
  }

  return;
}

/* The round function. We assume q = 1 mod 6, hence q = 1 mod 3.
   If k <= (Q - 1)/2 then it is the same under [0, Q-1] representation and [-Q/2, Q/2] representation.
   We check its remainder modulo 3. If rem == 0, 1 then we round downward; if rem == 2 then we round upward.
   If k >= (Q + 1)/2 then we have to subtract Q to get its [-Q/2, Q/2] representation.
   If k = 0 mod 3 then k - q = 2 mod 3, and we have to round upward.
   If k = 1 or 2 mod 3 then k - q = 0 or 1 mod 3, and we have to round downward.
 */
void ntrulpr_653_round (uint16_t * g) {
  for (uint32_t i = 0; i < NTRU_LPR_P; ++i) {
    uint64_t coeff = g[i];
    /* Barrett reduction for 3, with max 4020 */
    uint64_t div = (coeff * 5462) >> 14;
    uint32_t rem = coeff - div * 3;
    /* Rounding when coeff <= (Q - 1)/2 */
    uint32_t round1 = uint32_cmp_ge_branch (rem, 1, uint32_cmp_ge_branch (rem, 2, coeff + 1, coeff - 1), coeff);
    /* Rounding when coeff >= (Q + 1)/2 */
    uint32_t round2 = uint32_cmp_ge_branch (rem, 1, uint32_cmp_ge_branch (rem, 2, coeff - 1, coeff), coeff + 1);
    round2 = uint32_cmp_ge_branch (round2, NTRU_LPR_Q, 0, round2);
    g[i] = uint32_cmp_ge_branch (coeff, (NTRU_LPR_Q + 1) / 2, round2, round1);
  }

  return;
}

/* The HashShort function */
void ntrulpr_653_hashshort (const unsigned char * input, uint8_t * out) {
  /* Hash input with Keccak-512 */
  uint32_t hash_out[8];
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  uint8_t init_byte = 5;
  sponge_keccak_1600_absorb (state, &curr_offset, &init_byte, 1, 72);
  sponge_keccak_1600_absorb (state, &curr_offset, input, 32, 72);
  sponge_keccak_1600_finalize (state, curr_offset, 2 + 4, 72);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, (unsigned char *) hash_out, 32, 72);

  /* Expand hash output */
  uint32_t aes_exkey[60];
  uint32_t aes_in[4] = {0}, aes_out[4];
  uint32_t poly[NTRU_LPR_P];

  aes256_expandkey ((const unsigned char *) hash_out, (unsigned char *) aes_exkey);

  for (uint32_t i = 0; i < NTRU_LPR_P; ++i) {
    if ((i & 0x03) == 0) {
      aes256_encrypt_one_block ((const unsigned char *) aes_exkey, (const unsigned char *) aes_in, (unsigned char *) aes_out);
      aes_in[0]++;
    }

    poly[i] = aes_out[i & 0x03];
  }

  /* Clear the bottom bit of first w integers */
  for (uint32_t i = 0; i < NTRU_LPR_W; ++i) {
    poly[i] &= ~1u;
  }

  /* Set the bottom bit, and clear the next bit of remaining integers */
  for (uint32_t i = NTRU_LPR_W; i < NTRU_LPR_P; ++i) {
    poly[i] |= 1;
    poly[i] &= ~2u;
  }

  /* Sort the integers */
  safe_sort_uint32 (poly, 653);

  /* Reduce each integer modulo 4 */
  memset (out, 0, NTRU_LPR_SHORT_ENC_LEN);

  for (uint32_t i = 0; i < NTRU_LPR_P; ++i) {
    uint32_t coeff = poly[i] & 0x03;
    out[i >> 2] |= coeff << (2 * (i & 0x03));
  }
}
