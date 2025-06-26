#include <stdint.h>
#include <string.h>
#include <random.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  char str[1024];

  /* Fill str with random non-zero bytes */
  getrandom (str, 1024, 0);

  for (uint32_t i = 0; i < 1024; ++i) { if (!str[i]) str[i] = 1; }

  for (uint32_t i = 0; i < 64; ++i) {
    /* Test 1: small n and length */
    for (uint32_t j = 0; j < 32; ++j) {
      char t = str[i + j];
      str[i + j] = 0;

      for (uint32_t k = 0; k < 32; ++k) {
	uint32_t expected = j > k ? k : j;
	if (strnlen (str + i, k) != expected) { exit (1); }
      }

      str[i + j] = t;
    }

    /* Test 2: large length, n either smaller or larger than length */
    for (uint32_t j = 800; j < 900; ++j) {
      char t = str[i + j];
      str[i + j] = 0;

      for (uint32_t k = j - 16; k < j + 16; ++k) {
	uint32_t expected = j > k ? k : j;
	if (strnlen (str + i, k) != expected) { exit (1); }
      }

      str[i + j] = t;
    }
  }

  exit (0);
}
