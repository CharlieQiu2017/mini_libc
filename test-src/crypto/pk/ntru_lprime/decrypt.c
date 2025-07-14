#include <stdint.h>
#include <crypto/pk/ntru_lprime/ntru_lprime.h>
#include <io.h>
#include <exit.h>

#define NTRU_LPR_P 653
#define NTRU_LPR_Q 4621
#define NTRU_LPR_ROUND_ENC_LEN 865

void main (__attribute__((unused)) void * sp) {
  unsigned char sk[(NTRU_LPR_P - 1) / 4 + 1 + NTRU_LPR_ROUND_ENC_LEN + 32 + 32 + 32];
  unsigned char ct[NTRU_LPR_ROUND_ENC_LEN + 128 + 32];
  unsigned char key[32];

  read (0, sk, (NTRU_LPR_P - 1) / 4 + 1 + NTRU_LPR_ROUND_ENC_LEN + 32 + 32 + 32);
  read (0, ct, NTRU_LPR_ROUND_ENC_LEN + 128 + 32);

  ntrulpr_653_decapsulate (sk, ct, key);

  write (1, key, 32);

  exit (0);
}
