#ifndef FARFALLE_HELPER_H
#define FARFALLE_HELPER_H

#include <stdint.h>
#include <string.h>
#include <crypto/sk/farfalle/farfalle.h>

/* If a single input string of Farfalle cannot be provided in whole,
   this file provides helper functions to provide the string in multiple parts.
   Note that this is different from multiple calls to farfalle_add_string which adds multiple strings,
   rather than multiple parts of a single string, to Farfalle.
 */

struct farfalle_kravatte_helper_state {
  uint64_t p[25]; /* st->k XOR str absorbed so far */
  uint32_t offset_ctr; /* Index of first byte of p not XOR'ed with str */
};

static inline void farfalle_kravatte_helper_init (struct farfalle_kravatte_state * st, struct farfalle_kravatte_helper_state * hst) {
  memcpy (hst->p, st->k, 200);
  hst->offset_ctr = 0;
}

#ifdef __cplusplus
extern "C" {
#endif

void farfalle_kravatte_add_string_part (struct farfalle_kravatte_state * st, struct farfalle_kravatte_helper_state * hst, const unsigned char * str, size_t str_len);
void farfalle_kravatte_finalize_string (struct farfalle_kravatte_state * st, struct farfalle_kravatte_helper_state * hst);

#ifdef __cplusplus
}
#endif

#endif
