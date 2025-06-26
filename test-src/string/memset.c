#include <stdint.h>
#include <string.h>
#include <random.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  char str[1024];
  uint32_t ctr = 0;

  getrandom (str, 1024, 0);

  for (uint32_t i = 32; i < 64; ++i) {
    for (uint32_t j = 0; j < 256; ++j) {
      char x = str[i + j];

      memset (str + i, ctr, j);

      for (uint32_t k = 0; k < j; ++k) {
	if (str[i + k] != (ctr & 0xff)) {
	  exit (1);
	}
      }

      if (str[i + j] != x) {
	exit (1);
      }

      ++ctr;
    }
  }

  exit (0);
}
