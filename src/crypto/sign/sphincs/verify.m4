`#include <stdint.h>'
`#include <string.h>'
`#include <random.h>'
`#include <crypto/sign/sphincs/params/common.h>'
`#include <crypto/sign/sphincs/hash.h>'
`#include <crypto/sign/sphincs/fors.h>'
`#include <crypto/sign/sphincs/ht.h>'
`#include <crypto/sign/sphincs/sphincs.h>'

define(`BODY',`
  const unsigned char * pk_seed = pk;
  const unsigned char * pk_root = pk + SPHINCS_N;
  const unsigned char * r = sig;

  /* Generate digest */
  unsigned char digest[SPHINCS_DIGEST_BYTES];
  sphincs_hash_h_msg (r, pk_seed, pk_root, msg, msg_len, digest, SPHINCS_DIGEST_BYTES);

  /* Extract idx_tree and idx_leaf */
  uint64_t idx_tree = 0;
  for (uint32_t i = 0; i < SPHINCS_TREE_BYTES; ++i) {
    idx_tree <<= 8;
    idx_tree += digest[SPHINCS_MD_BYTES + i];
  }
  idx_tree &= (1ull << SPHINCS_TREE_BITS) - 1ull;

  uint16_t idx_leaf = 0;
  for (uint32_t i = 0; i < SPHINCS_LEAF_BYTES; ++i) {
    idx_leaf <<= 8;
    idx_leaf += digest[SPHINCS_MD_BYTES + SPHINCS_TREE_BYTES + i];
  }
  idx_leaf &= (1ull << SPHINCS_XH) - 1ull;

  /* FORS public key */
  unsigned char fors_pk[SPHINCS_N];
  sphincs_fors_sig_to_pk (pk_seed, __builtin_bswap64 (idx_tree), __builtin_bswap16 (idx_leaf), digest, sig + SPHINCS_N, fors_pk);

  /* HT public key */
  unsigned char ht_pk[SPHINCS_N];
  sphincs_ht_sig_to_pk (pk_seed, idx_tree, idx_leaf, fors_pk, sig + SPHINCS_N + SPHINCS_K * (SPHINCS_A + 1) * SPHINCS_N, ht_pk);

  return safe_memcmp (ht_pk, pk_root, SPHINCS_N);
')dnl

define(`INST',`
uint64_t sphincs_$1_$2_verify (const unsigned char * pk, const unsigned char * msg, size_t msg_len, const unsigned char * sig) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
