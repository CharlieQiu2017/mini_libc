#include <stdint.h>
#include <crypto/pk/ntru_lprime/ntru_lprime.h>
#include <io.h>
#include <exit.h>

#define NTRU_LPR_P 653
#define NTRU_LPR_Q 4621
#define NTRU_LPR_ROUND_ENC_LEN 865

void main (__attribute__((unused)) void * sp) {
  unsigned char pk[32 + NTRU_LPR_ROUND_ENC_LEN];
  unsigned char ct[NTRU_LPR_ROUND_ENC_LEN + 128 + 32];
  unsigned char key[32];

  read (0, pk, 32 + NTRU_LPR_ROUND_ENC_LEN);

  ntrulpr_653_encapsulate (pk, ct, key);

  write (1, ct, NTRU_LPR_ROUND_ENC_LEN + 128 + 32);
  write (2, key, 32);

  exit (0);
}
