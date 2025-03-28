`#include <stdint.h>'
`#include <crypto/sign/sphincs/params/common.h>'
`#include <crypto/sign/sphincs/adrs.h>'
`#include <crypto/sign/sphincs/hash.h>'
`#include <crypto/sign/sphincs/wots.h>'
`#include <crypto/sign/sphincs/xmss.h>'

define(`BODY',`
  const uint16_t num_leaves = 1u << SPHINCS_XH;
  unsigned char tmp[SPHINCS_N];
  unsigned char keys[SPHINCS_XH + 1][SPHINCS_N];

  uint32_t adrs[8];
  sphincs_adrs_set_layer (adrs, layer);
  sphincs_adrs_set_tree_addr_swapped (adrs, tree_addr_swapped);
  sphincs_adrs_set_keypair_swapped (adrs, 0);
  sphincs_adrs_set_type (adrs, SPHINCS_TREE);

  for (uint32_t i = 0; i < num_leaves; ++i) {
    uint32_t pos = __builtin_popcount (i);

    sphincs_wots_gen_pk (pk_seed, sk_seed, layer, tree_addr_swapped, __builtin_bswap16 ((uint16_t) i), keys[pos]);

    uint32_t pos_next = __builtin_popcount (i + 1);
    for (uint32_t j = pos; j >= pos_next; --j) {
      uint32_t h = pos - j + 1;
      sphincs_adrs_set_word6 (adrs, h << 24);
      sphincs_adrs_set_word7 (adrs, (i >> h) << 24);
      sphincs_hash_h (pk_seed, adrs, keys[j - 1], keys[j], tmp);
      for (uint32_t k = 0; k < SPHINCS_N; ++k) keys[j - 1][k] = tmp[k];
    }
  }

  for (uint32_t i = 0; i < SPHINCS_N; ++i) pk_out[i] = keys[0][i];
')dnl

define(`INST',`
void sphincs_$1_$2_xmss_gen_pk (const unsigned char * pk_seed, const unsigned char * sk_seed, uint8_t layer, uint64_t tree_addr_swapped, unsigned char * pk_out) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
