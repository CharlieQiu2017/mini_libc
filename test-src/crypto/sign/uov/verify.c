#include <stdint.h>
#include <crypto/sign/uov/uov.h>
#include <io.h>
#include <exit.h>

#define UOV_N 112
#define UOV_M 44
#define UOV_V 68

void main (__attribute__((unused)) void * sp) {
  uint8_t p1[UOV_M * UOV_V * (UOV_V + 1) / 2];
  uint8_t p2[UOV_M * UOV_V * UOV_M];
  uint8_t p3[UOV_M * UOV_M * UOV_M];

  read (0, p1, UOV_M * UOV_V * (UOV_V + 1) / 2);
  read (0, p2, UOV_M * UOV_V * UOV_M);
  read (0, p3, UOV_M * UOV_M * UOV_M);

  unsigned char msg[32];
  unsigned char sig[UOV_N + 16];

  read (0, msg, 32);
  read (0, sig, UOV_N + 16);

  uint64_t result = uov_verify (p1, p2, p3, msg, 32, sig + UOV_N, sig);

  if (result) exit (1);

  exit (0);
}
