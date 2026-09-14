#ifndef SPHINCS_SPHINCS_H
#define SPHINCS_SPHINCS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
#ifndef _Bool
#define _Bool bool
#endif
extern "C" {
#endif

void sphincs_128f_shake_simple_gen_key (unsigned char * sk_out, unsigned char * pk_out);

void sphincs_128f_shake_simple_sign_from_rand (const unsigned char * sk, const unsigned char * msg, size_t msg_len, const unsigned char * randomness, unsigned char * out);

void sphincs_128f_shake_simple_sign (const unsigned char * sk, const unsigned char * msg, size_t msg_len, unsigned char * out);

void sphincs_128f_shake_simple_sign_randomize (const unsigned char * sk, const unsigned char * msg, size_t msg_len, unsigned char * out);

_Bool sphincs_128f_shake_simple_verify (const unsigned char * pk, const unsigned char * msg, size_t msg_len, const unsigned char * sig);

#ifdef __cplusplus
}
#endif

#endif
