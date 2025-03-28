`#include <stdint.h>'
`#include <crypto/sign/sphincs/adrs.h>'
`#include <crypto/sign/sphincs/hash.h>'
`#include <crypto/sign/sphincs/fors.h>'

`/* pk_seed: SPHINCS_N bytes, no assumptions, no data dependence'
`   tree_addr_swapped: No assumptions, no data dependence'
`   keypair_swapped: No assumptions, no data dependence'
`   msg: SPHINCS_MD_BYTES bytes, no assumptions, control flow dependence'
`   sig: SPHINCS_K * (SPHINCS_A + 1) * SPHINCS_N bytes, no assumptions, no data dependence'
`   pk_out: SPHINCS_N bytes'
` */'

define(`BODY',`
  uint16_t indices[SPHINCS_K];
  unsigned char tmp1[SPHINCS_N], tmp2[SPHINCS_N];
  uint32_t adrs[8];
  sphincs_adrs_set_layer (adrs, 0);
  sphincs_adrs_set_tree_addr_swapped (adrs, tree_addr_swapped);
  sphincs_adrs_set_keypair_swapped (adrs, keypair_swapped);

  sphincs_fors_msg_to_indices (msg, indices);

  unsigned char roots[SPHINCS_K * SPHINCS_N];
  for (uint32_t i = 0; i < SPHINCS_K; ++i) {
    uint32_t idx = (i << SPHINCS_A) + indices[i];
    sphincs_adrs_set_type (adrs, SPHINCS_FORS_TREE);
    sphincs_adrs_set_word6 (adrs, 0);
    sphincs_adrs_set_word7 (adrs, __builtin_bswap32 (idx));
    sphincs_hash_f (pk_seed, adrs, sig, tmp1);
    sig += SPHINCS_N;

    for (uint32_t j = 1; j <= SPHINCS_A; ++j) {
      uint32_t idx_ = idx >> j;
      sphincs_adrs_set_word6 (adrs, j << 24);
      sphincs_adrs_set_word7 (adrs, __builtin_bswap32 (idx_));

      if (indices[i] & (1u << (j - 1))) {
	if (j & 1) {
	  sphincs_hash_h (pk_seed, adrs, sig, tmp1, tmp2);
	} else {
	  sphincs_hash_h (pk_seed, adrs, sig, tmp2, tmp1);
	}
      } else {
	if (j & 1) {
	  sphincs_hash_h (pk_seed, adrs, tmp1, sig, tmp2);
	} else {
	  sphincs_hash_h (pk_seed, adrs, tmp2, sig, tmp1);
	}
      }

      sig += SPHINCS_N;
    }

    if (SPHINCS_A & 1) {
      for (uint32_t j = 0; j < SPHINCS_N; ++j) roots[i * SPHINCS_N + j] = tmp2[j];
    } else {
      for (uint32_t j = 0; j < SPHINCS_N; ++j) roots[i * SPHINCS_N + j] = tmp1[j];
    }
  }

  sphincs_adrs_set_type (adrs, SPHINCS_FORS_ROOTS);
  sphincs_adrs_set_word6 (adrs, 0);
  sphincs_adrs_set_word7 (adrs, 0);
  sphincs_hash_t (pk_seed, adrs, roots, SPHINCS_K, pk_out);
')dnl

define(`INST',`
void sphincs_$1_$2_fors_sig_to_pk (const unsigned char * pk_seed, uint64_t tree_addr_swapped, uint16_t keypair_swapped, const unsigned char * msg, const unsigned char * sig, unsigned char * pk_out) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
