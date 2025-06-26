#include <stdint.h>
#include <string.h>
#include <random.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  char src[1024], dst[1024];

  getrandom (src, 1024, 0);
  getrandom (dst, 1024, 0);

  for (uint32_t i = 0; i < 1024; ++i) { if (!src[i]) src[i] = 1; }

  /* Place n in the outermost loop.
     Otherwise, we are always writing the same content into dst.
   */

  for (uint32_t n = 0; n < 24; ++n) {
    for (uint32_t i = 0; i < 24; ++i) {
      for (uint32_t j = 0; j < 24; ++j) {
	for (uint32_t k = 0; k < 24; ++k) {
	  char t = src[i + k];
	  src[i + k] = 0;

	  strncpy (dst + j, src + i, n);

	  uint32_t bytes_copied = n <= k + 1 ? n : k + 1;
	  for (uint32_t l = 0; l < bytes_copied; ++l) {
	    if (dst[j + l] != src[i + l]) {
	      exit (1);
	    }
	  }

	  src[i + k] = t;
	}
      }
    }
  }

  exit (0);
}
