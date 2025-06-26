#include <stdint.h>
#include <string.h>
#include <random.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  char str1[1024];
  char str2[1024];

  getrandom (str1, 1024, 0);
  getrandom (str2, 1024, 0);

  /* i = start index of str1
     j = start index of str2
     n = number of bytes to compare
     m = first index where str1 and str2 differ
   */

  for (uint32_t i = 0; i < 24; ++i) {
    for (uint32_t j = 0; j < 24; ++j) {
      /* Copy 64 bytes of str1 into str2 */
      for (uint32_t x = 0; x < 64; ++x) { str2[j + x] = str1[i + x]; }

      for (uint32_t n = 0; n < 24; ++n) {
	for (uint32_t m = 0; m < 24; ++m) {
	  str1[i + m] += 3;

	  int result = memcmp (str1 + i, str2 + j, n);
	  if (n <= m) {
	    if (result != 0) {
	      exit (1);
	    }
	  } else {
	    if (result != ((int) str1[i + m]) - ((int) str2[j + m])) {
	      exit (1);
	    }
	  }

	  str1[i + m] -= 3;
	}
      }
    }
  }

  exit (0);
}
