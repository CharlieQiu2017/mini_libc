#include <stdint.h>
#include <string.h>
#include <random.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  char src[1024], dst[1024];

  getrandom (src, 1024, 0);
  getrandom (dst, 1024, 0);

  for (uint32_t i = 0; i < 64; ++i) {
    for (uint32_t j = 0; j < 64; ++j) {
      for (uint32_t k = 0; k < 256; ++k) {
	char x = dst[j + k];

	memcpy (dst + j, src + i, k);

	for (uint32_t l = 0; l < k; ++l) {
	  if (dst[j + l] != src[i + l]) {
	    exit (1);
	  }
	}

	if (dst[j + k] != x) {
	  exit (1);
	}
      }
    }
  }

  exit (0);
}
