#include <stddef.h>
#include <stdint.h>

#define ONES ((size_t) -1 / 255) /* 0x0101010101010101 */
#define HIGHS (ONES * 128) /* 0x8080808080808080 */
#define HASZERO(x) (((x) - ONES) & ~ (x) & HIGHS)

size_t strnlen (const char * s, size_t n) {
  /* 1. Store the initial position */
  const char * orig_s = s;

  /* 2. Check for NUL until s is aligned */
  while (n && ((uintptr_t) s & 7)) { if (*s == 0) return s - orig_s; s++; n--; }

  if (!n) return s - orig_s;

  /* 3. Repeat read 8 bytes of s and check for NUL */
  uint64_t buf;

  while (n >= 8) {
    buf = * ((uint64_t *) s);
    if (HASZERO (buf)) break;
    s += 8;
    n -= 8;
  }

  /* 4. Individually check the final bytes */

  if (!n) return s - orig_s;

  if (n < 8) {
    buf = * ((uint64_t *) s);
    while (n && (buf & 0xff)) { buf >>= 8; s++; n--; }
    return s - orig_s;
  }

  /* If this line is reached, buf contains NUL */

  while (buf & 0xff) { buf >>= 8; s++; }
  return s - orig_s;
}
