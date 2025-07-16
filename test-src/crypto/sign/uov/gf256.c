#include <stdint.h>
#include <crypto/sign/uov/gf256.h>
#include <io.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  for (uint32_t i = 0; i < 24; ++i) {
    for (uint32_t j = 0; j < 24; ++j) {
      uint32_t prod = gf256_mul_no_mod (i, j);
      write (1, &i, 1);
      write (1, &j, 1);
      write (1, &prod, 2);
    }
  }

  exit (0);
}
