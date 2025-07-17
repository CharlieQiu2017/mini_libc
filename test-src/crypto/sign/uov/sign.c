#include <stdint.h>
#include <string.h>
#include <crypto/sign/uov/uov.h>
#include <random.h>
#include <io.h>
#include <exit.h>

#define UOV_N 112
#define UOV_M 44
#define UOV_V 68

void main (__attribute__((unused)) void * sp) {
  uint8_t seed[32];
  uint8_t o[UOV_V * UOV_M];
  uint8_t p1[UOV_M * UOV_V * (UOV_V + 1) / 2];
  uint8_t s[UOV_M * UOV_V * UOV_M];

  read (0, seed, 32);
  read (0, o, UOV_V * UOV_M);
  read (0, p1, UOV_M * UOV_V * (UOV_V + 1) / 2);
  read (0, s, UOV_M * UOV_V * UOV_M);

  unsigned char msg[32];
  unsigned char salt[16];

  read (0, msg, 32);
  getrandom (salt, 16, 0);

  unsigned char sig[UOV_N + 16];

  uov_sign (seed, o, p1, s, msg, 32, salt, sig);
  memcpy (sig + UOV_N, salt, 16);

  write (1, sig, 128);

  exit (0);
}
