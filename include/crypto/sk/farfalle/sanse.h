#ifndef FARFALLE_SANSE_H
#define FARFALLE_SANSE_H

#include <stdint.h>
#include <crypto/sk/farfalle/farfalle.h>

struct farfalle_kravatte_sanse_state {
  struct farfalle_kravatte_state fst;
  size_t tag_len;
  uint8_t parity;
};

#ifdef __cplusplus
extern "C" {
#endif

void farfalle_kravatte_sanse_16_init (struct farfalle_kravatte_sanse_state * st, const unsigned char * k, size_t tag_len);
void farfalle_kravatte_sanse_24_init (struct farfalle_kravatte_sanse_state * st, const unsigned char * k, size_t tag_len);
void farfalle_kravatte_sanse_32_init (struct farfalle_kravatte_sanse_state * st, const unsigned char * k, size_t tag_len);
void farfalle_kravatte_sanse_wrap (struct farfalle_kravatte_sanse_state * st, const unsigned char * msg, size_t msg_len, const unsigned char * meta, size_t meta_len, unsigned char * ct_out, unsigned char * tag_out);
_Bool farfalle_kravatte_sanse_unwrap (struct farfalle_kravatte_sanse_state * st, const unsigned char * ct, size_t ct_len, const unsigned char * meta, size_t meta_len, const unsigned char * tag, unsigned char * msg_out);

#ifdef __cplusplus
}
#endif

#endif
