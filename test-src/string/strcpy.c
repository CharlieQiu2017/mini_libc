#include <stdint.h>
#include <string.h>
#include <random.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  char src[1024], dst[1024];

  getrandom (src, 1024, 0);
  getrandom (dst, 1024, 0);

  for (uint32_t i = 0; i < 1024; ++i) { if (!src[i]) src[i] = 1; }

  for (uint32_t i = 0; i < 64; ++i) {
    for (uint32_t j = 0; j < 64; ++j) {
      for (uint32_t k = 0; k < 64; ++k) {
	char t = src[i + k];
	src[i + k] = 0;

	/* strcpy should not write past final NUL byte */
	char x = dst[j + k + 1];

	strcpy (dst + j, src + i);

	for (uint32_t l = 0; l <= k; ++l) {
	  if (dst[j + l] != src[i + l]) {
	    exit (1);
	  }
	}

	if (dst[j + k + 1] != x) {
	  exit (1);
	}

	src[i + k] = t;
      }
    }
  }

  exit (0);
}
