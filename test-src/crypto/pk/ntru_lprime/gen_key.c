#include <stdint.h>
#include <crypto/pk/ntru_lprime/ntru_lprime.h>
#include <io.h>
#include <exit.h>

#define NTRU_LPR_P 653
#define NTRU_LPR_Q 4621
#define NTRU_LPR_ROUND_ENC_LEN 865

void main (__attribute__((unused)) void * sp) {
  unsigned char pk[32 + NTRU_LPR_ROUND_ENC_LEN];
  unsigned char sk[(NTRU_LPR_P - 1) / 4 + 1 + NTRU_LPR_ROUND_ENC_LEN + 32 + 32 + 32];

  ntrulpr_653_gen_key (sk, pk);

  write (1, pk, 32 + NTRU_LPR_ROUND_ENC_LEN);
  write (2, sk, (NTRU_LPR_P - 1) / 4 + 1 + NTRU_LPR_ROUND_ENC_LEN + 32 + 32 + 32);

  exit (0);
}
