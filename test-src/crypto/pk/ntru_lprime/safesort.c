/* Test the constant-time sorting procedure */

#include <stdint.h>
#include <string.h>
#include <crypto/pk/ntru_lprime/ntru_lprime.h>
#include <random.h>
#include <exit.h>

#define NTRU_LPR_P 653
#define NTRU_LPR_Q 4621

void sort_ref (uint32_t * poly) {
  for (uint32_t i = 0; i < NTRU_LPR_P; ++i) {
    for (uint32_t j = 0; j < NTRU_LPR_P - 1; ++j) {
      if (poly[j] > poly[j + 1]) {
	uint32_t t = poly[j];
	poly[j] = poly[j + 1];
	poly[j + 1] = t;
      }
    }
  }

  return;
}

void main (__attribute__((unused)) void * sp) {
  uint32_t g_rnd[NTRU_LPR_P], g_copy[NTRU_LPR_P];

  getrandom (g_rnd, NTRU_LPR_P * 4, 0);

  for (uint32_t i = 0; i < NTRU_LPR_P; ++i) {
    g_rnd[i] = g_rnd[i] % NTRU_LPR_Q;
    g_copy[i] = g_rnd[i];
  }

  ntrulpr_653_safesort (g_rnd);
  sort_ref (g_copy);

  for (uint32_t i = 0; i < NTRU_LPR_P; ++i) {
    if (g_rnd[i] != g_copy[i]) exit (1);
  }

  exit (0);
}
