`#include <stdint.h>'
`#include <string.h>'
`#include <random.h>'
`#include <crypto/sign/sphincs/params/common.h>'
`#include <crypto/sign/sphincs/hash.h>'
`#include <crypto/sign/sphincs/fors.h>'
`#include <crypto/sign/sphincs/ht.h>'
`#include <crypto/sign/sphincs/sphincs.h>'

define(`BODY',`
  getrandom (sk_out, 3 * SPHINCS_N, 0);
  const unsigned char * sk_seed = sk_out;
  const unsigned char * pk_seed = sk_out + 2 * SPHINCS_N;
  sphincs_ht_gen_pk (pk_seed, sk_seed, sk_out + 3 * SPHINCS_N);
  memcpy (pk_out, sk_out + 2 * SPHINCS_N, 2 * SPHINCS_N);
')dnl

define(`INST',`
void sphincs_$1_$2_gen_key (unsigned char * sk_out, unsigned char * pk_out) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
