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
  uint8_t seed[32];
  uint8_t o[UOV_V * UOV_M];
  uint8_t s[UOV_M * UOV_V * UOV_M];

  uov_gen_key (p1, p2, p3, seed, o, s);

  write (1, p1, UOV_M * UOV_V * (UOV_V + 1) / 2);
  write (1, p2, UOV_M * UOV_V * UOV_M);
  write (1, p3, UOV_M * UOV_M * UOV_M);
  write (2, seed, 32);
  write (2, o, UOV_V * UOV_M);
  write (2, p1, UOV_M * UOV_V * (UOV_V + 1) / 2);
  write (2, s, UOV_M * UOV_V * UOV_M);

  exit (0);
}
