#ifndef FARFALLE_WBC_H
#define FARFALLE_WBC_H

#include <stdint.h>
#include <crypto/sk/farfalle/farfalle.h>

/* The only internal state of WBC is the permuted key k',
   which will be used to re-initialize Farfalle upon every call
 */
struct farfalle_kravatte_wbc_state {
  uint64_t k[25];
};

#ifdef __cplusplus
extern "C" {
#endif

void farfalle_kravatte_wbc_16_init (struct farfalle_kravatte_wbc_state * st, const unsigned char * k);
void farfalle_kravatte_wbc_24_init (struct farfalle_kravatte_wbc_state * st, const unsigned char * k);
void farfalle_kravatte_wbc_32_init (struct farfalle_kravatte_wbc_state * st, const unsigned char * k);

void farfalle_kravatte_wbc_encrypt (const struct farfalle_kravatte_wbc_state * st, const unsigned char * block, size_t block_len, const unsigned char * tweak, size_t tweak_len, unsigned char * out);

void farfalle_kravatte_wbc_decrypt (const struct farfalle_kravatte_wbc_state * st, const unsigned char * ct, size_t ct_len, const unsigned char * tweak, size_t tweak_len, unsigned char * msg_out);

/* The caller of ae_wrap should provide an out buffer of length block_len + tag_len. */
void farfalle_kravatte_wbc_ae_wrap (const struct farfalle_kravatte_wbc_state * st, const unsigned char * block, size_t block_len, const unsigned char * meta, size_t meta_len, size_t tag_len, unsigned char * out);

/* The caller of ae_unwrap should provide an msg_out buffer of length ct_len, not ct_len - tag_len.
   The caller must never call this function with tag_len > ct_len.
   See comments in wbc.j2.
 */
_Bool farfalle_kravatte_wbc_ae_unwrap (const struct farfalle_kravatte_wbc_state * st, const unsigned char * ct, size_t ct_len, const unsigned char * meta, size_t meta_len, size_t tag_len, unsigned char * msg_out);

#ifdef __cplusplus
}
#endif

#endif
