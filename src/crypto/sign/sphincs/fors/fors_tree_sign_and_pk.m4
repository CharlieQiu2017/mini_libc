`#include <stdint.h>'
`#include <crypto/sign/sphincs/adrs.h>'
`#include <crypto/sign/sphincs/hash.h>'
`#include <crypto/sign/sphincs/fors.h>'

`/* pk_seed: SPHINCS_N bytes, no assumptions, no data dependence'
`   sk_seed: SPHINCS_N bytes, no assumptions, no data dependence'
`   tree_addr_swapped: No assumptions, no data dependence'
`   keypair_swapped: No assumptions, no data dependence'
`   idx_tree: No assumptions, no data dependence'
`   idx_leaf: 0 ~ (1 << SPHINCS_A) - 1, control flow dependence'
`   out: (SPHINCS_A + 1) * SPHINCS_N bytes'
`   pk_out: SPHINCS_N bytes'

`   Calls:'
`   sphincs_prf: 1 << SPHINCS_A times, no output assumptions, no output data dependence'
`   sphincs_f: 1 << SPHINCS_A times, no output assumptions, no output data dependence'
`   sphincs_h: (1 << SPHINCS_A) - 1 times, no output assumptions, no output data dependence'
` */'

define(`BODY',`
  /* SPHINCS_A <= 15, so no overflow */
  const uint16_t num_leaves = 1u << SPHINCS_A;
  unsigned char keys[SPHINCS_A + 1][SPHINCS_N];
  unsigned char tmp[SPHINCS_N];
  uint32_t adrs[8];
  /* The following 3 fields do not change during the entire computation */
  sphincs_adrs_set_layer (adrs, 0);
  sphincs_adrs_set_tree_addr_swapped (adrs, tree_addr_swapped);
  sphincs_adrs_set_keypair_swapped (adrs, keypair_swapped);

  for (uint16_t i = 0; i < num_leaves; ++i) {
    uint32_t idx = (((uint32_t) idx_tree) << SPHINCS_A) + i;
    sphincs_adrs_set_type (adrs, SPHINCS_FORS_PRF);
    sphincs_adrs_set_word6 (adrs, 0);
    sphincs_adrs_set_word7 (adrs, __builtin_bswap32 (idx));
    sphincs_hash_prf (pk_seed, sk_seed, adrs, tmp);

    if (i == idx_leaf) {
      for (uint32_t j = 0; j < SPHINCS_N; ++j) out[j] = tmp[j];
    }

    uint32_t pos = __builtin_popcount (i);
    sphincs_adrs_set_type (adrs, SPHINCS_FORS_TREE);
    /* word6 and word7 does not change from above */
    sphincs_hash_f (pk_seed, adrs, tmp, keys[pos]);

    uint32_t pos_next = __builtin_popcount (i + 1);
    for (uint32_t j = pos; j >= pos_next; --j) {
      uint32_t h = pos - j + 1;
      uint32_t idx_ = idx >> h;

      if (i >> h == idx_leaf >> h) {
	if (idx_leaf & (1u << (h - 1))) {
	  for (uint32_t k = 0; k < SPHINCS_N; ++k) out[h * SPHINCS_N + k] = keys[j - 1][k];
	} else {
	  for (uint32_t k = 0; k < SPHINCS_N; ++k) out[h * SPHINCS_N + k] = keys[j][k];
	}
      }

      /* ADRS type is still FORS_TREE */
      /* word6 (tree height) should be set to h in big-endian. */
      /* Since h occupies at most one byte, simply shift it to the highest byte. */
      sphincs_adrs_set_word6 (adrs, h << 24);
      sphincs_adrs_set_word7 (adrs, __builtin_bswap32 (idx_));
      sphincs_hash_h (pk_seed, adrs, keys[j - 1], keys[j], tmp);
      for (uint32_t k = 0; k < SPHINCS_N; ++k) keys[j - 1][k] = tmp[k];
    }
  }

  for (uint32_t i = 0; i < SPHINCS_N; ++i) pk_out[i] = keys[0][i];
')dnl

define(`INST',`
void sphincs_$1_$2_fors_tree_sign_and_pk (const unsigned char * pk_seed, const unsigned char * sk_seed, uint64_t tree_addr_swapped, uint16_t keypair_swapped, uint8_t idx_tree, uint16_t idx_leaf, unsigned char * out, unsigned char * pk_out) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
