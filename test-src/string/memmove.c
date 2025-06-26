#include <stdint.h>
#include <string.h>
#include <random.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  char src[1024], src_copy[1024];

  getrandom (src, 1024, 0);

  for (uint32_t i = 0; i < 1024; ++i) src_copy[i] = src[i];

  for (uint32_t i = 128; i < 256; ++i) {
    for (uint32_t j = i - 32; j < i + 32; ++j) {
      for (uint32_t k = 0; k < 32; ++k) {
	char x = src[j + k];

	memmove (src + j, src + i, k);

	for (uint32_t l = 0; l < k; ++l) {
	  if (src[j + l] != src_copy[i + l]) {
	    exit (1);
	  }
	}

	if (src[j + k] != x) {
	  exit (1);
	}

	for (uint32_t l = 0; l < k; ++l) src[j + l] = src_copy[j + l];
      }
    }
  }

  exit (0);
}
