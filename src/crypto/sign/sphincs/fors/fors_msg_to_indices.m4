`#include <stdint.h>'

`/* msg: SPHINCS_MD_BYTES bytes, no assumptions, no data dependence'
`   indices: SPHINCS_K elements'
` */'

define(`BODY',`
  uint32_t offset = 0;
  uint64_t c = 0;
  uint32_t rem_bits = 0;

  for (uint32_t i = 0; i < SPHINCS_K; ++i) {
    while (rem_bits < SPHINCS_A) {
      c = (c << 8) + msg[offset++];
      rem_bits += 8;
    }
    indices[i] = c >> (rem_bits - SPHINCS_A);
    c &= (1ull << (rem_bits - SPHINCS_A)) - 1;
    rem_bits -= SPHINCS_A;
  }
')dnl

define(`INST',`
void sphincs_$1_fors_msg_to_indices (const unsigned char * msg, uint16_t * indices) {
  #include <crypto/sign/sphincs/params/params_$1_$2.h>
BODY
  #include <crypto/sign/sphincs/params/clear.h>
}')dnl

INST(`128f',`shake_simple')
