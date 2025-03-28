#ifndef SPHINCS_HASH_H
#define SPHINCS_HASH_H

#include <stdint.h>
#include <stddef.h>

void sphincs_shake_h_msg_16 (const unsigned char * r, const unsigned char * pk_seed, const unsigned char * pk_root, const unsigned char * msg, size_t msg_len, unsigned char * out, size_t out_len);

void sphincs_shake_prf_16 (const unsigned char * pk_seed, const unsigned char * sk_seed, const uint32_t * adrs, unsigned char * out);

void sphincs_shake_prf_msg_16 (const unsigned char * sk_prf, const unsigned char * optrand, const unsigned char * msg, size_t msg_len, unsigned char * out);

void sphincs_shake_simple_f_16 (const unsigned char * pk_seed, const uint32_t * adrs, const unsigned char * msg, unsigned char * out);

void sphincs_shake_simple_h_16 (const unsigned char * pk_seed, const uint32_t * adrs, const unsigned char * msg1, const unsigned char * msg2, unsigned char * out);

/* msg_len is measured in multiples of 16 bytes */
void sphincs_shake_simple_t_16 (const unsigned char * pk_seed, const uint32_t * adrs, const unsigned char * msg, size_t msg_len, unsigned char * out);

#endif
