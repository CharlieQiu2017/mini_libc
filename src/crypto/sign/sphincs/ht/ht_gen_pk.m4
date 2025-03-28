`#include <stdint.h>'
`#include <crypto/sign/sphincs/params/common.h>'
`#include <crypto/sign/sphincs/adrs.h>'
`#include <crypto/sign/sphincs/xmss.h>'
`#include <crypto/sign/sphincs/ht.h>'

define(`BODY', `
  sphincs_xmss_gen_pk (pk_seed, sk_seed, SPHINCS_D - 1, 0, pk_out);
')dnl

define(`INST',`
void sphincs_$1_$2_ht_gen_pk (const unsigned char * pk_seed, const unsigned char * sk_seed, unsigned char * pk_out) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
