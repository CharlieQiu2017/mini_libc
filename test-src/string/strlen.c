#include <stdint.h>
#include <string.h>
#include <random.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  char str[1024];

  /* Fill str with random non-zero bytes */
  getrandom (str, 1024, 0);

  for (uint32_t i = 0; i < 1024; ++i) { if (!str[i]) str[i] = 1; }

  /* Test strlen starting from any offset, with any length */
  for (uint32_t i = 0; i < 64; ++i) {
    for (uint32_t j = 0; j < 900; ++j) {
      char t = str[i + j];
      str[i + j] = 0;

      if (strlen (str + i) != j) {
	exit (1);
      }

      str[i + j] = t;
    }
  }

  exit (0);
}
