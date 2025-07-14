#ifndef FARFALLE_H
#define FARFALLE_H

#include <stdint.h>
#include <stddef.h>

struct farfalle_kravatte_state {
  /* If the value of k computed upon init is k_orig, then at any given moment k = roll_c^input_block_count (k_orig) */
  uint64_t k[25];
  uint64_t k_orig[25];
  uint64_t sum[25];
  /* If the value of y computed upon begin_extract is y_orig, then at any given moment y = roll_e^output_block_count (y_orig) */
  uint64_t y[25];
  uint64_t output_buf[25];
  uint32_t output_offset;
};

#ifdef __cplusplus
extern "C" {
#endif

/* Internal rolling functions */

void farfalle_kravatte_roll_c (uint64_t * st);
void farfalle_kravatte_roll_e (uint64_t * st);

void farfalle_kravatte_16_init (struct farfalle_kravatte_state * st, const unsigned char * k);
void farfalle_kravatte_24_init (struct farfalle_kravatte_state * st, const unsigned char * k);
void farfalle_kravatte_32_init (struct farfalle_kravatte_state * st, const unsigned char * k);
void farfalle_kravatte_reset (struct farfalle_kravatte_state * st);

void farfalle_kravatte_add_string (struct farfalle_kravatte_state * st, const unsigned char * str, size_t str_len);
void farfalle_kravatte_begin_extract (struct farfalle_kravatte_state * st);
/* begin_extract_short replaces perm_d with identity. It is used for the WBC mode. */
void farfalle_kravatte_begin_extract_short (struct farfalle_kravatte_state * st);
void farfalle_kravatte_extract (struct farfalle_kravatte_state * st, unsigned char * out, size_t len);
unsigned char farfalle_kravatte_extract_and_compare (struct farfalle_kravatte_state * st, const unsigned char * ref, size_t len);
void farfalle_kravatte_skip_output (struct farfalle_kravatte_state * st, size_t len);

#ifdef __cplusplus
}
#endif

#endif
