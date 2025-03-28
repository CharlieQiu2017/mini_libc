`#include <stdint.h>'
`#include <crypto/hash/keccak/keccak_p.h>'
`#include <crypto/sign/sphincs/hash.h>'

define(`SHAKE_H_MSG',`
void sphincs_shake_h_msg_$1 (const unsigned char * r, const unsigned char * pk_seed, const unsigned char * pk_root, const unsigned char * msg, size_t msg_len, unsigned char * out, size_t out_len) {
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  sponge_keccak_1600_absorb (state, &curr_offset, r, $1, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, pk_seed, $1, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, pk_root, $1, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, msg, msg_len, 136);
  sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, 136);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, out, out_len, 136);
}
')dnl

SHAKE_H_MSG(16)dnl

define(`SHAKE_PRF',`
void sphincs_shake_prf_$1 (const unsigned char * pk_seed, const unsigned char * sk_seed, const uint32_t * adrs, unsigned char * out) {
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  sponge_keccak_1600_absorb (state, &curr_offset, pk_seed, $1, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, (const unsigned char *) adrs, 32, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, sk_seed, $1, 136);
  sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, 136);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, out, $1, 136);
}
')dnl

SHAKE_PRF(16)dnl

define(`SHAKE_PRF_MSG',`
void sphincs_shake_prf_msg_$1 (const unsigned char * sk_prf, const unsigned char * optrand, const unsigned char * msg, size_t msg_len, unsigned char * out) {
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  sponge_keccak_1600_absorb (state, &curr_offset, sk_prf, $1, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, optrand, $1, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, msg, msg_len, 136);
  sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, 136);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, out, $1, 136);
}
')dnl

SHAKE_PRF_MSG(16)dnl

define(`SHAKE_SIMPLE_F',`
void sphincs_shake_simple_f_$1 (const unsigned char * pk_seed, const uint32_t * adrs, const unsigned char * msg, unsigned char * out) {
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  sponge_keccak_1600_absorb (state, &curr_offset, pk_seed, $1, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, (const unsigned char *) adrs, 32, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, msg, $1, 136);
  sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, 136);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, out, $1, 136);
}
')dnl

SHAKE_SIMPLE_F(16)dnl

define(`SHAKE_SIMPLE_H',`
void sphincs_shake_simple_h_$1 (const unsigned char * pk_seed, const uint32_t * adrs, const unsigned char * msg1, const unsigned char * msg2, unsigned char * out) {
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  sponge_keccak_1600_absorb (state, &curr_offset, pk_seed, $1, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, (const unsigned char *) adrs, 32, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, msg1, $1, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, msg2, $1, 136);
  sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, 136);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, out, $1, 136);
}
')dnl

SHAKE_SIMPLE_H(16)dnl

define(`SHAKE_SIMPLE_T',`
void sphincs_shake_simple_t_$1 (const unsigned char * pk_seed, const uint32_t * adrs, const unsigned char * msg, size_t msg_len, unsigned char * out) {
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  sponge_keccak_1600_absorb (state, &curr_offset, pk_seed, $1, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, (const unsigned char *) adrs, 32, 136);
  sponge_keccak_1600_absorb (state, &curr_offset, msg, msg_len * $1, 136);
  sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, 136);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, out, $1, 136);
}
')dnl

SHAKE_SIMPLE_T(16)dnl
