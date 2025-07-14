/* Test the optimized poly_mult_short procedure, compare it to a slower implementation */

#include <stdint.h>
#include <string.h>
#include <crypto/pk/ntru_lprime/ntru_lprime.h>
#include <random.h>
#include <exit.h>

#define NTRU_LPR_P 653
#define NTRU_LPR_Q 4621

void poly_mult_short_ref (const uint16_t * g, const uint8_t * a, uint16_t * out) {
  uint16_t g_times_xi[NTRU_LPR_P];

  memset (out, 0, 2 * NTRU_LPR_P);
  memcpy (g_times_xi, g, 2 * NTRU_LPR_P);

  for (uint32_t i = 0; i < NTRU_LPR_P; ++i) {
    uint32_t coeff = (a[i >> 2] >> (2 * (i & 0x03))) & 0x03;

    for (uint32_t j = 0; j < NTRU_LPR_P; ++j) {
      uint32_t addent = 0;

      if (coeff == 0) addent = NTRU_LPR_Q - g_times_xi[j];
      if (coeff == 1) addent = 0;
      if (coeff == 2) addent = g_times_xi[j];

      addent += out[j];
      addent %= NTRU_LPR_Q;
      out[j] = addent;
    }

    uint32_t high_term = g_times_xi[NTRU_LPR_P - 1];

    for (uint32_t i = NTRU_LPR_P - 1; i > 0; --i) {
      g_times_xi[i] = g_times_xi[i - 1];
    }

    g_times_xi[0] = high_term;
    g_times_xi[1] += high_term;
    g_times_xi[1] %= NTRU_LPR_Q;
  }
}

void main (__attribute__((unused)) void * sp) {
  uint32_t g_rnd[NTRU_LPR_P];
  uint32_t short_rnd[NTRU_LPR_P];

  uint16_t g_enc[NTRU_LPR_P];
  uint8_t short_enc[(NTRU_LPR_P - 1) / 4 + 1];

  uint16_t prod1[NTRU_LPR_P], prod2[NTRU_LPR_P];

  getrandom (g_rnd, NTRU_LPR_P * 4, 0);
  getrandom (short_rnd, NTRU_LPR_P * 4, 0);

  memset (short_enc, 0, (NTRU_LPR_P - 1) / 4 + 1);

  for (uint32_t i = 0; i < NTRU_LPR_P; ++i) {
    g_enc[i] = g_rnd[i] % NTRU_LPR_Q;
    short_rnd[i] %= 3;
    short_enc[i >> 2] |= short_rnd[i] << (2 * (i & 0x03));
  }

  ntrulpr_653_poly_mult_short (g_enc, short_enc, NTRU_LPR_P, prod1);
  poly_mult_short_ref (g_enc, short_enc, prod2);

  for (uint32_t i = 0; i < NTRU_LPR_P; ++i) {
    if (prod1[i] != prod2[i]) exit (1);
  }

  /* Also test computing the bottom 256 terms */
  ntrulpr_653_poly_mult_short (g_enc, short_enc, 256, prod1);

  for (uint32_t i = 0; i < 256; ++i) {
    if (prod1[i] != prod2[i]) exit (1);
  }

  exit (0);
}
