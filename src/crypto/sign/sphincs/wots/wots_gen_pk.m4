`#include <stdint.h>'
`#include <crypto/sign/sphincs/params/common.h>'
`#include <crypto/sign/sphincs/adrs.h>'
`#include <crypto/sign/sphincs/hash.h>'
`#include <crypto/sign/sphincs/wots.h>'

`/* pk_seed: SPHINCS_N bytes, no assumptions, no data dependence'
`   sk_seed: SPHINCS_N bytes, no assumptions, no data dependence'
`   layer: No assumptions, no data dependence'
`   tree_addr_swapped: No assumptions, no data dependence'
`   keypair_swapped: No assumptions, no data dependence'
`   pk_out: SPHINCS_N bytes'

`   External calls:'
`   wots_gen_key: 1 time, no assumptions, no output dependence'
`   sphincs_f: SPHINCS_WLEN * 15 times, no assumptions, no output dependence'
`   sphincs_t: 1 time, no assumptions, no output dependence'
` */'

define(`BODY',`
  unsigned char wots_sk[SPHINCS_N * SPHINCS_WLEN];
  sphincs_wots_gen_key (pk_seed, sk_seed, layer, tree_addr_swapped, keypair_swapped, wots_sk);

  uint32_t adrs[8];
  sphincs_adrs_set_layer (adrs, layer);
  sphincs_adrs_set_tree_addr_swapped (adrs, tree_addr_swapped);
  sphincs_adrs_set_keypair_swapped (adrs, keypair_swapped);
  sphincs_adrs_set_type (adrs, SPHINCS_WOTS_HASH);

  unsigned char tmp1[SPHINCS_N], tmp2[SPHINCS_N];
  unsigned char wots_roots[SPHINCS_N * SPHINCS_WLEN];
  for (uint32_t i = 0; i < SPHINCS_WLEN; ++i) {
    sphincs_adrs_set_word6 (adrs, i << 24);
    sphincs_adrs_set_word7 (adrs, 0);
    sphincs_hash_f (pk_seed, adrs, wots_sk + i * SPHINCS_N, tmp2);
    for (uint32_t j = 1; j < 15; ++j) {
      sphincs_adrs_set_word7 (adrs, j << 24);
      if (j & 1) {
	sphincs_hash_f (pk_seed, adrs, tmp2, tmp1);
      } else {
	sphincs_hash_f (pk_seed, adrs, tmp1, tmp2);
      }
    }
    for (uint32_t j = 0; j < SPHINCS_N; ++j) wots_roots[i * SPHINCS_N + j] = tmp2[j];
  }

  sphincs_adrs_set_type (adrs, SPHINCS_WOTS_PK);
  sphincs_adrs_set_word6 (adrs, 0);
  sphincs_adrs_set_word7 (adrs, 0);
  sphincs_hash_t (pk_seed, adrs, wots_roots, SPHINCS_WLEN, pk_out);
')dnl

define(`INST',`
void sphincs_$1_$2_wots_gen_pk (const unsigned char * pk_seed, const unsigned char * sk_seed, uint8_t layer, uint64_t tree_addr_swapped, uint16_t keypair_swapped, unsigned char * pk_out) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
