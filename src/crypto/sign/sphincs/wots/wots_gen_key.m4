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
`   out: SPHINCS_WLEN * SPHINCS_N bytes'
` */'

define(`BODY',`
  uint32_t adrs[8];
  sphincs_adrs_set_layer (adrs, layer);
  sphincs_adrs_set_tree_addr_swapped (adrs, tree_addr_swapped);
  sphincs_adrs_set_keypair_swapped (adrs, keypair_swapped);
  sphincs_adrs_set_type (adrs, SPHINCS_WOTS_PRF);
  sphincs_adrs_set_word7 (adrs, 0);

  for (uint32_t i = 0; i < SPHINCS_WLEN; ++i) {
    sphincs_adrs_set_word6 (adrs, i << 24);
    sphincs_hash_prf (pk_seed, sk_seed, adrs, out + SPHINCS_N * i);
  }
')dnl

define(`INST',`
void sphincs_$1_$2_wots_gen_key (const unsigned char * pk_seed, const unsigned char * sk_seed, uint8_t layer, uint64_t tree_addr_swapped, uint16_t keypair_swapped, unsigned char * out) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
