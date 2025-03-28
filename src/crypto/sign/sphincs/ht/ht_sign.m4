`#include <stdint.h>'
`#include <crypto/sign/sphincs/params/common.h>'
`#include <crypto/sign/sphincs/adrs.h>'
`#include <crypto/sign/sphincs/xmss.h>'
`#include <crypto/sign/sphincs/ht.h>'

define(`BODY',`
  unsigned char tmp1[SPHINCS_N], tmp2[SPHINCS_N];
  for (uint32_t i = 0; i < SPHINCS_D; ++i) {
    if (i & 1) {
      sphincs_xmss_sign_and_pk (pk_seed, sk_seed, i, __builtin_bswap64 (idx_tree), idx_leaf, msg, out + i * (SPHINCS_WLEN + SPHINCS_XH) * SPHINCS_N, tmp1);
      msg = tmp1;
    } else {
      sphincs_xmss_sign_and_pk (pk_seed, sk_seed, i, __builtin_bswap64 (idx_tree), idx_leaf, msg, out + i * (SPHINCS_WLEN + SPHINCS_XH) * SPHINCS_N, tmp2);
      msg = tmp2;
    }

    idx_leaf = idx_tree & ((1u << SPHINCS_XH) - 1u);
    idx_tree >>= SPHINCS_XH;
  }
')dnl

define(`INST',`
void sphincs_$1_$2_ht_sign (const unsigned char * pk_seed, const unsigned char * sk_seed, uint64_t idx_tree, uint16_t idx_leaf, const unsigned char * msg, unsigned char * out) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
