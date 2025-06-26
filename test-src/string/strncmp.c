#include <stdint.h>
#include <string.h>
#include <random.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  char str1[1024];
  char str2[1024];

  getrandom (str1, 1024, 0);
  getrandom (str2, 1024, 0);

  for (uint32_t i = 0; i < 1024; ++i) { if (!str1[i]) str1[i] = 1; }
  for (uint32_t i = 0; i < 1024; ++i) { if (!str2[i]) str2[i] = 1; }

  /* i = start index of str1
     j = start index of str2
     k = length of str1
     l = length of str2
     m = first index where str1 and str2 differ
     n = max number of bytes to compare
   */

  for (uint32_t i = 0; i < 24; ++i) {
    for (uint32_t j = 0; j < 24; ++j) {
      /* Copy 64 bytes of str1 into str2 */
      for (uint32_t x = 0; x < 64; ++x) { str2[j + x] = str1[i + x]; }

      for (uint32_t k = 0; k < 24; ++k) {
	char t = str1[i + k];
	str1[i + k] = 0;

	for (uint32_t l = 0; l < 24; ++l) {
	  char s = str2[j + l];
	  str2[j + l] = 0;

	  uint32_t min_len = k < l ? k : l;

	  for (uint32_t m = 0; m < min_len; ++m) {
	    str1[i + m] += 3;
	    uint32_t n_min = m < 8 ? 0 : m - 8;
	    for (uint32_t n = n_min; n < m + 8; ++n) {
	      int result = strncmp (str1 + i, str2 + j, n);
	      if (n <= m) {
		if (result != 0) {
		  exit (1);
		}
	      } else if (result != ((int) str1[i + m]) - ((int) str2[j + m])) {
		exit (1);
	      }
	    }
	    str1[i + m] -= 3;
	  }

	  if (k == l) {
	    uint32_t n_min = k < 8 ? 0 : k - 8;
	    for (uint32_t n = n_min; n < k + 8; ++n) {
	      if (strncmp (str1 + i, str2 + j, n) != 0) {
		exit (1);
	      }
	    }
	  } else if (k < l) {
	    uint32_t n_min = k < 8 ? 0 : k - 8;
	    for (uint32_t n = n_min; n < k + 8; ++n) {
	      int result = strncmp (str1 + i, str2 + j, n);
	      if (n <= k) {
		if (result != 0) {
		  exit (1);
		}
	      } else if (result != -((int) str2[j + k])) {
		exit(1);
	      }
	    }
	  } else if (k > l) {
	    uint32_t n_min = l < 8 ? 0 : l - 8;
	    for (uint32_t n = n_min; n < l + 8; ++n) {
	      int result = strncmp (str1 + i, str2 + j, n);
	      if (n <= l) {
		if (result != 0) {
		  exit (1);
		}
	      } else if (result != ((int) str1[i + l])) {
		exit(1);
	      }
	    }
	  }

	  str2[j + l] = s;
	}

	str1[i + k] = t;
      }
    }
  }

  exit (0);
}
