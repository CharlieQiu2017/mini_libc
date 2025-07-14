#ifndef FARFALLE_SANE_H
#define FARFALLE_SANE_H

#include <stdint.h>
#include <crypto/sk/farfalle/farfalle.h>

struct farfalle_kravatte_sane_state {
  struct farfalle_kravatte_state fst;
  size_t tag_len;
  uint8_t parity;
};

#ifdef __cplusplus
extern "C" {
#endif

void farfalle_kravatte_sane_16_init (struct farfalle_kravatte_sane_state * st, const unsigned char * k, size_t tag_len);
void farfalle_kravatte_sane_24_init (struct farfalle_kravatte_sane_state * st, const unsigned char * k, size_t tag_len);
void farfalle_kravatte_sane_32_init (struct farfalle_kravatte_sane_state * st, const unsigned char * k, size_t tag_len);
void farfalle_kravatte_sane_start_session_common (struct farfalle_kravatte_sane_state * st, const unsigned char * nonce, size_t nonce_len);
void farfalle_kravatte_sane_start_session (struct farfalle_kravatte_sane_state * st, const unsigned char * nonce, size_t nonce_len);
void farfalle_kravatte_sane_start_session_with_tag (struct farfalle_kravatte_sane_state * st, const unsigned char * nonce, size_t nonce_len, unsigned char * tag_out);
unsigned char farfalle_kravatte_sane_start_session_check_tag (struct farfalle_kravatte_sane_state * st, const unsigned char * nonce, size_t nonce_len, const unsigned char * tag);
void farfalle_kravatte_sane_wrap (struct farfalle_kravatte_sane_state * st, const unsigned char * msg, size_t msg_len, const unsigned char * meta, size_t meta_len, unsigned char * ct_out, unsigned char * tag_out);
unsigned char farfalle_kravatte_sane_unwrap (struct farfalle_kravatte_sane_state * st, const unsigned char * ct, size_t ct_len, const unsigned char * meta, size_t meta_len, const unsigned char * tag, unsigned char * msg_out);

#ifdef __cplusplus
}
#endif

#endif
