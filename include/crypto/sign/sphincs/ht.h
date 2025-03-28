#ifndef SPHINCS_HT_H
#define SPHINCS_HT_H

#include <stdint.h>

void sphincs_128f_shake_simple_ht_gen_pk (const unsigned char * pk_seed, const unsigned char * sk_seed, unsigned char * pk_out);

void sphincs_128f_shake_simple_ht_sign (const unsigned char * pk_seed, const unsigned char * sk_seed, uint64_t idx_tree, uint16_t idx_leaf, const unsigned char * msg, unsigned char * out);

void sphincs_128f_shake_simple_ht_sig_to_pk (const unsigned char * pk_seed, uint64_t idx_tree, uint16_t idx_leaf, const unsigned char * msg, const unsigned char * sig, unsigned char * out);

#endif
