#ifndef SPHINCS_SPHINCS_H
#define SPHINCS_SPHINCS_H

#include <stdint.h>
#include <stddef.h>

void sphincs_128f_shake_simple_gen_key (unsigned char * sk_out, unsigned char * pk_out);

void sphincs_128f_shake_simple_sign (const unsigned char * sk, const unsigned char * msg, size_t msg_len, _Bool randomize, unsigned char * out);

uint64_t sphincs_128f_shake_simple_verify (const unsigned char * pk, const unsigned char * msg, size_t msg_len, const unsigned char * sig);

#endif
