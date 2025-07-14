/* Given any random array of 653 values between 0 and 4620,
   such that if k <= 2310 then k is a multiple of 3, otherwise k = 1 mod 3,
   decode should exactly reverse encode */

#include <stdint.h>
#include <crypto/pk/ntru_lprime/ntru_lprime.h>
#include <random.h>
#include <exit.h>

#define NTRU_LPR_P 653
#define NTRU_LPR_Q 4621
#define NTRU_LPR_ROUND_ENC_LEN 865

void main (__attribute__((unused)) void * sp) {
  uint32_t rnd[NTRU_LPR_P];
  uint16_t orig[NTRU_LPR_P];
  unsigned char enc[NTRU_LPR_ROUND_ENC_LEN];
  uint16_t dec[NTRU_LPR_P];

  getrandom (rnd, NTRU_LPR_P * 4, 0);

  for (uint32_t i = 0; i < NTRU_LPR_P; ++i) {
    orig[i] = rnd[i] % NTRU_LPR_Q;
    if (orig[i] <= (NTRU_LPR_Q - 1) / 2) orig[i] = orig[i] - orig[i] % 3; else orig[i] = orig[i] - orig[i] % 3 + 1;
    if (orig[i] == NTRU_LPR_Q) orig[i] = NTRU_LPR_Q - 3;
  }

  ntrulpr_653_encode_poly_round (orig, enc);
  ntrulpr_653_decode_poly_round (enc, dec);

  for (uint32_t i = 0; i < NTRU_LPR_P; ++i) {
    if (orig[i] != dec[i]) exit (1);
  }

  exit (0);
}
