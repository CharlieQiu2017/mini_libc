#ifndef SPHINCS_FORS_H
#define SPHINCS_FORS_H

#include <stdint.h>

/* SPHINCS+ FORS depends on 3 parameters N, K, and A. */

void sphincs_128f_fors_msg_to_indices (const unsigned char * msg, uint16_t * indices);

void sphincs_128f_shake_simple_fors_tree_sign_and_pk (const unsigned char * pk_seed, const unsigned char * sk_seed, uint64_t tree_addr_swapped, uint16_t keypair_swapped, uint8_t idx_tree, uint16_t idx_leaf, unsigned char * out, unsigned char * pk_out);

void sphincs_128f_shake_simple_fors_sign_and_pk (const unsigned char * pk_seed, const unsigned char * sk_seed, uint64_t tree_addr_swapped, uint16_t keypair_swapped, const unsigned char * msg, unsigned char * out, unsigned char * pk_out);

void sphincs_128f_shake_simple_fors_sig_to_pk (const unsigned char * pk_seed, uint64_t tree_addr_swapped, uint16_t keypair_swapped, const unsigned char * msg, const unsigned char * sig, unsigned char * pk_out);

#endif
