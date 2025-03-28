#ifndef SPHINCS_WOTS_H
#define SPHINCS_WOTS_H

#include <stdint.h>

void sphincs_128f_shake_simple_wots_gen_key (const unsigned char * pk_seed, const unsigned char * sk_seed, uint8_t layer, uint64_t tree_addr_swapped, uint16_t keypair_swapped, unsigned char * out);

void sphincs_128f_shake_simple_wots_gen_pk (const unsigned char * pk_seed, const unsigned char * sk_seed, uint8_t layer, uint64_t tree_addr_swapped, uint16_t keypair_swapped, unsigned char * pk_out);

void sphincs_128f_shake_simple_wots_sign_and_pk (const unsigned char * pk_seed, const unsigned char * sk_seed, uint8_t layer, uint64_t tree_addr_swapped, uint16_t keypair_swapped, const unsigned char * msg, unsigned char * out, unsigned char * pk_out);

void sphincs_128f_shake_simple_wots_sig_to_pk (const unsigned char * pk_seed, uint8_t layer, uint64_t tree_addr_swapped, uint16_t keypair_swapped, const unsigned char * msg, const unsigned char * sig, unsigned char * pk_out);

#endif
