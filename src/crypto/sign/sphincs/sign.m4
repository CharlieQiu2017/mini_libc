`#include <stdint.h>'
`#include <string.h>'
`#include <random.h>'
`#include <crypto/sign/sphincs/params/common.h>'
`#include <crypto/sign/sphincs/hash.h>'
`#include <crypto/sign/sphincs/fors.h>'
`#include <crypto/sign/sphincs/ht.h>'
`#include <crypto/sign/sphincs/sphincs.h>'

define(`BODY',`
  const unsigned char * sk_seed = sk;
  const unsigned char * sk_prf = sk + SPHINCS_N;
  const unsigned char * pk_seed = sk + 2 * SPHINCS_N;
  const unsigned char * pk_root = sk + 3 * SPHINCS_N;

  /* Generate randomness */
  unsigned char r[SPHINCS_N];
  if (randomize) {
    unsigned char rand[SPHINCS_N];
    getrandom (rand, SPHINCS_N, 0);
    sphincs_hash_prf_msg (sk_prf, rand, msg, msg_len, r);
  } else {
    sphincs_hash_prf_msg (sk_prf, pk_seed, msg, msg_len, r);
  }

  for (uint32_t i = 0; i < SPHINCS_N; ++i) out[i] = r[i];

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

  /* FORS signature */
  unsigned char fors_pk[SPHINCS_N];
  sphincs_fors_sign_and_pk (pk_seed, sk_seed, __builtin_bswap64 (idx_tree), __builtin_bswap16 (idx_leaf), digest, out + SPHINCS_N, fors_pk);

  /* HT signature */
  sphincs_ht_sign (pk_seed, sk_seed, idx_tree, idx_leaf, fors_pk, out + SPHINCS_N + SPHINCS_K * (SPHINCS_A + 1) * SPHINCS_N);
')dnl

define(`INST',`
void sphincs_$1_$2_sign (const unsigned char * sk, const unsigned char * msg, size_t msg_len, _Bool randomize, unsigned char * out) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
