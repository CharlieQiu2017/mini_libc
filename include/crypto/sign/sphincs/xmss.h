#ifndef SPHINCS_XMSS_H
#define SPHINCS_XMSS_H

#include <stdint.h>

void sphincs_128f_shake_simple_xmss_gen_pk (const unsigned char * pk_seed, const unsigned char * sk_seed, uint8_t layer, uint64_t tree_addr_swapped, unsigned char * pk_out);

void sphincs_128f_shake_simple_xmss_sign_and_pk (const unsigned char * pk_seed, const unsigned char * sk_seed, uint8_t layer, uint64_t tree_addr_swapped, uint16_t keypair, const unsigned char * msg, unsigned char * out, unsigned char * pk_out);

void sphincs_128f_shake_simple_xmss_sig_to_pk (const unsigned char * pk_seed, uint8_t layer, uint64_t tree_addr_swapped, uint16_t keypair, const unsigned char * msg, const unsigned char * sig, unsigned char * pk_out);

#endif
