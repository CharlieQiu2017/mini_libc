`#include <stdint.h>'
`#include <crypto/sign/sphincs/adrs.h>'
`#include <crypto/sign/sphincs/hash.h>'
`#include <crypto/sign/sphincs/fors.h>'

`/* pk_seed: SPHINCS_N bytes, no assumptions, no data dependence'
`   sk_seed: SPHINCS_N bytes, no assumptions, no data dependence'
`   tree_addr_swapped: No assumptions, no data dependence'
`   keypair_swapped: No assumptions, no data dependence'
`   msg: SPHINCS_MD_BYTES bytes, no assumptions, control flow dependence due to fors_tree_sign_and_pk call'
`   out: SPHINCS_K * (SPHINCS_A + 1) * SPHINCS_N bytes'
`   pk_out: SPHINCS_N bytes'

`   Calls:'
`   fors_tree_sign_and_pk: SPHINCS_K times, no output assumptions, no output data dependence'
`   sphincs_fors_compress: 1 time, no output assumptions, no output data dependence'
` */'

define(`BODY',`
  uint16_t indices[SPHINCS_K];

  sphincs_fors_msg_to_indices (msg, indices);

  unsigned char roots[SPHINCS_K * SPHINCS_N];
  for (uint32_t i = 0; i < SPHINCS_K; ++i) {
    sphincs_fors_tree_sign_and_pk (pk_seed, sk_seed, tree_addr_swapped, keypair_swapped, i, indices[i], out + i * (SPHINCS_A + 1) * SPHINCS_N, roots + i * SPHINCS_N);
  }

  uint32_t adrs[8];
  sphincs_adrs_set_layer (adrs, 0);
  sphincs_adrs_set_tree_addr_swapped (adrs, tree_addr_swapped);
  sphincs_adrs_set_keypair_swapped (adrs, keypair_swapped);
  sphincs_adrs_set_type (adrs, SPHINCS_FORS_ROOTS);
  sphincs_adrs_set_word6 (adrs, 0);
  sphincs_adrs_set_word7 (adrs, 0);
  sphincs_hash_t (pk_seed, adrs, roots, SPHINCS_K, pk_out);
')dnl

define(`INST',`
void sphincs_$1_$2_fors_sign_and_pk (const unsigned char * pk_seed, const unsigned char * sk_seed, uint64_t tree_addr_swapped, uint16_t keypair_swapped, const unsigned char * msg, unsigned char * out, unsigned char * pk_out) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
