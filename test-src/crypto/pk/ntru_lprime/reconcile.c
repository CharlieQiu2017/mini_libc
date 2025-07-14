/* Test the lattice reconciliation */

#include <stdint.h>
#include <crypto/pk/ntru_lprime/ntru_lprime.h>
#include <random.h>
#include <io.h>
#include <exit.h>

#define NTRU_LPR_P 653
#define NTRU_LPR_Q 4621

void main (__attribute__((unused)) void * sp) {
  /* Generate a random element G */
  unsigned char g_seed[32];
  uint16_t g[NTRU_LPR_P];
  getrandom (g_seed, 32, 0);
  ntrulpr_653_expand_seed (g_seed, g);

  /* Generate a short element a */
  unsigned char a_seed[32];
  uint8_t a[(NTRU_LPR_P - 1) / 4 + 1];
  getrandom (a_seed, 32, 0);
  ntrulpr_653_hashshort (a_seed, a);

  /* Compute Round(aG) = aG + d */
  uint16_t ag[NTRU_LPR_P];
  // uint16_t round_ag[NTRU_LPR_P];
  ntrulpr_653_poly_mult_short (g, a, NTRU_LPR_P, ag);
  ntrulpr_653_round (ag, ag);

  /* Generate a short element b */
  unsigned char b_seed[32];
  uint8_t b[(NTRU_LPR_P - 1) / 4 + 1];
  getrandom (b_seed, 32, 0);
  ntrulpr_653_hashshort (b_seed, b);

  /* Compute Round(bG) = bG + e */
  uint16_t bg[NTRU_LPR_P];
  // uint16_t round_bg[NTRU_LPR_P];
  ntrulpr_653_poly_mult_short (g, b, NTRU_LPR_P, bg);
  ntrulpr_653_round (bg, bg);

  /* Compute b * Round(aG) */
  uint16_t b_round_ag[256];
  ntrulpr_653_poly_mult_short (ag, b, 256, b_round_ag);

  /* Compute a * Round(bG) */
  uint16_t a_round_bg[256];
  ntrulpr_653_poly_mult_short (bg, a, 256, a_round_bg);

  /* Output everything */
  write (1, g, NTRU_LPR_P * 2);
  write (1, a, (NTRU_LPR_P - 1) / 4 + 1);
  write (1, b, (NTRU_LPR_P - 1) / 4 + 1);
  write (1, ag, NTRU_LPR_P * 2);
  // write (1, round_ag, NTRU_LPR_P * 2);
  write (1, bg, NTRU_LPR_P * 2);
  // write (1, round_bg, NTRU_LPR_P * 2);
  write (1, b_round_ag, NTRU_LPR_P * 2);
  write (1, a_round_bg, NTRU_LPR_P * 2);

  exit (0);
}
