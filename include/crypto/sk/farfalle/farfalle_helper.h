#ifndef FARFALLE_HELPER_H
#define FARFALLE_HELPER_H

#include <stdint.h>
#include <crypto/sk/farfalle/farfalle.h>

struct farfalle_kravatte_helper_state {
  uint64_t p[25];
  uint32_t offset_ctr;
};

static inline void farfalle_kravatte_helper_init (struct farfalle_kravatte_helper_state * hst) {
  hst->offset_ctr = 0;
}

void farfalle_kravatte_add_string_part (struct farfalle_kravatte_state * st, struct farfalle_kravatte_helper_state * hst, const unsigned char * str, size_t str_len);
void farfalle_kravatte_finalize_string (struct farfalle_kravatte_state * st, struct farfalle_kravatte_helper_state * hst);

#endif
