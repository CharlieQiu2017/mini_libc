`#include <stdint.h>'
`#include <crypto/sign/sphincs/params/common.h>'
`#include <crypto/sign/sphincs/adrs.h>'
`#include <crypto/sign/sphincs/hash.h>'
`#include <crypto/sign/sphincs/wots.h>'
`#include <crypto/sign/sphincs/xmss.h>'

define(`BODY',`
  uint32_t adrs[8];
  sphincs_adrs_set_layer (adrs, layer);
  sphincs_adrs_set_tree_addr_swapped (adrs, tree_addr_swapped);
  sphincs_adrs_set_keypair_swapped (adrs, 0);
  sphincs_adrs_set_type (adrs, SPHINCS_TREE);

  unsigned char tmp1[SPHINCS_N], tmp2[SPHINCS_N];
  sphincs_wots_sig_to_pk (pk_seed, layer, tree_addr_swapped, __builtin_bswap16 (keypair), msg, sig, tmp1);

  for (uint32_t i = 1; i <= SPHINCS_XH; ++i) {
    sphincs_adrs_set_word6 (adrs, i << 24);
    sphincs_adrs_set_word7 (adrs, ((uint32_t) (keypair >> i)) << 24);
    if (keypair & (1u << (i - 1))) {
      if (i & 1) {
	sphincs_hash_h (pk_seed, adrs, sig + (SPHINCS_WLEN + i - 1) * SPHINCS_N, tmp1, tmp2);
      } else {
	sphincs_hash_h (pk_seed, adrs, sig + (SPHINCS_WLEN + i - 1) * SPHINCS_N, tmp2, tmp1);
      }
    } else {
      if (i & 1) {
	sphincs_hash_h (pk_seed, adrs, tmp1, sig + (SPHINCS_WLEN + i - 1) * SPHINCS_N, tmp2);
      } else {
	sphincs_hash_h (pk_seed, adrs, tmp2, sig + (SPHINCS_WLEN + i - 1) * SPHINCS_N, tmp1);
      }
    }
  }

  if (SPHINCS_XH & 1) {
    for (uint32_t i = 0; i < SPHINCS_N; ++i) pk_out[i] = tmp2[i];
  } else {
    for (uint32_t i = 0; i < SPHINCS_N; ++i) pk_out[i] = tmp1[i];
  }
')dnl

define(`INST',`
void sphincs_$1_$2_xmss_sig_to_pk (const unsigned char * pk_seed, uint8_t layer, uint64_t tree_addr_swapped, uint16_t keypair, const unsigned char * msg, const unsigned char * sig, unsigned char * pk_out) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
