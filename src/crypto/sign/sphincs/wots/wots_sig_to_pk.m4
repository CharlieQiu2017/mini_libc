`#include <stdint.h>'
`#include <crypto/sign/sphincs/params/common.h>'
`#include <crypto/sign/sphincs/adrs.h>'
`#include <crypto/sign/sphincs/hash.h>'
`#include <crypto/sign/sphincs/wots.h>'

`/* pk_seed: SPHINCS_N bytes, no assumptions, no data dependence'
`   layer: No assumptions, no data dependence'
`   tree_addr_swapped: No assumptions, no data dependence'
`   keypair_swapped: No assumptions, no data dependence'
`   msg: SPHINCS_N bytes, control flow dependence'
`   sig: SPHINCS_WLEN * SPHINCS_N bytes, no assumptions, no data dependence'
`   pk_out: SPHINCS_N bytes'
` */'

define(`BODY',`
  uint32_t adrs[8];
  sphincs_adrs_set_layer (adrs, layer);
  sphincs_adrs_set_tree_addr_swapped (adrs, tree_addr_swapped);
  sphincs_adrs_set_keypair_swapped (adrs, keypair_swapped);
  sphincs_adrs_set_type (adrs, SPHINCS_WOTS_HASH);

  uint16_t sum = 0;
  for (uint32_t i = 0; i < SPHINCS_N; ++i) {
    uint8_t c = msg[i];
    sum += c & 0xf;
    sum += c >> 4;
  }
  sum = 30 * SPHINCS_N - sum;
  uint8_t sum_w[3] = {(uint8_t) (sum >> 8), (uint8_t) ((sum >> 4) & 0xf), (uint8_t) (sum & 0xf)};

  unsigned char wots_roots[SPHINCS_N * SPHINCS_WLEN];
  unsigned char tmp1[SPHINCS_N], tmp2[SPHINCS_N];
  for (uint32_t i = 0; i < SPHINCS_WLEN; ++i) {
    uint8_t c = i < 2 * SPHINCS_N ? (i & 1 ? msg[i / 2] & 0xf : msg[i / 2] >> 4) : sum_w[i - 2 * SPHINCS_N];

    if (c & 1) {
      for (uint32_t j = 0; j < SPHINCS_N; ++j) tmp2[j] = sig[i * SPHINCS_N + j];
    } else {
      for (uint32_t j = 0; j < SPHINCS_N; ++j) tmp1[j] = sig[i * SPHINCS_N + j];
    }

    sphincs_adrs_set_word6 (adrs, i << 24);

    for (uint32_t j = c; j < 15; ++j) {
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
void sphincs_$1_$2_wots_sig_to_pk (const unsigned char * pk_seed, uint8_t layer, uint64_t tree_addr_swapped, uint16_t keypair_swapped, const unsigned char * msg, const unsigned char * sig, unsigned char * pk_out) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
