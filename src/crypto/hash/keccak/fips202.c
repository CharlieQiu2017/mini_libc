/* Constructions standardized in FIPS 202 and SP 800-185.
   SHA3, SHAKE, cSHAKE, KMAC, and TupleHash.
   All constructions are based on Keccak-p[1600, 24].
 */

#include <stdint.h>
#include <crypto/hash/keccak/keccak_p.h>
#include <crypto/hash/keccak/fips202.h>

static inline __attribute__((always_inline)) uint32_t byte_len (uint64_t n) {
  uint32_t len = 0;
  while (n > 0) { len++; n >>= 8; }
  if (len == 0) return 1; else return len;
}

void left_encode (uint64_t * state, uint32_t * curr_offset, uint64_t n, uint32_t rate) {
  uint32_t l = byte_len (n);
  sponge_keccak_1600_absorb (state, curr_offset, (unsigned char *) &l, 1, rate);
  n = __builtin_bswap64 (n);
  n >>= (8 * (8 - l));
  sponge_keccak_1600_absorb (state, curr_offset, (unsigned char *) &n, l, rate);
}

void right_encode (uint64_t * state, uint32_t * curr_offset, uint64_t n, uint32_t rate) {
  uint32_t l = byte_len (n);
  n = __builtin_bswap64 (n);
  n >>= (8 * (8 - l));
  sponge_keccak_1600_absorb (state, curr_offset, (unsigned char *) &n, l, rate);
  sponge_keccak_1600_absorb (state, curr_offset, (unsigned char *) &l, 1, rate);
}

/* state should be zero-initialized */
void cshake_prepare_state (uint64_t * state, const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, uint32_t rate) {
  uint32_t curr_offset = 0;

  /* bytepad */
  uint16_t pad = 1 + (rate << 8);
  sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &pad, 2, rate);

  /* encoding of str_n */
  left_encode (state, &curr_offset, str_n_len << 3, rate);
  sponge_keccak_1600_absorb (state, &curr_offset, str_n, str_n_len, rate);

  /* encoding of str_s */
  left_encode (state, &curr_offset, str_s_len << 3, rate);
  sponge_keccak_1600_absorb (state, &curr_offset, str_s, str_s_len, rate);

  /* finish bytepad */
  if (curr_offset != 0) { keccak_p_1600_permute (state); }
}

/* state should be either zero-initialized, or initialized with cshake_prepare_state */
void sha3_prepare_msg (uint64_t * state, const unsigned char * data, size_t len, unsigned char final_byte, uint32_t rate) {
  uint32_t curr_offset = 0;
  sponge_keccak_1600_absorb (state, &curr_offset, data, len, rate);
  sponge_keccak_1600_finalize (state, curr_offset, final_byte, rate);
}

void sha3_template (const unsigned char * data, size_t len, unsigned char * out, size_t out_len, unsigned char final_byte, uint32_t rate) {
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  sha3_prepare_msg (state, data, len, final_byte, rate);
  sponge_keccak_1600_squeeze (state, &curr_offset, out, out_len, rate);
}

/* capacity = 448, rate = 1152 = 144 bytes */
void sha3_224 (const unsigned char * data, size_t len, unsigned char * out) {
  /* final byte = domain-separation flag + first part of padding */
  sha3_template (data, len, out, 28, 2 + 4, 144);
}

/* capacity = 512, rate = 1088 = 136 bytes */
void sha3_256 (const unsigned char * data, size_t len, unsigned char * out) {
  sha3_template (data, len, out, 32, 2 + 4, 136);
}

/* capacity = 768, rate = 832 = 104 bytes */
void sha3_384 (const unsigned char * data, size_t len, unsigned char * out) {
  sha3_template (data, len, out, 48, 2 + 4, 104);
}

/* capacity = 1024, rate = 576 = 72 bytes */
void sha3_512 (const unsigned char * data, size_t len, unsigned char * out) {
  sha3_template (data, len, out, 64, 2 + 4, 72);
}

/* capacity = 256, rate = 1344 = 168 bytes */
void shake128 (const unsigned char * data, size_t len, unsigned char * out, size_t out_len) {
  sha3_template (data, len, out, out_len, 15 + 16, 168);
}

/* capacity = 512, rate = 1088 = 136 bytes */
void shake256 (const unsigned char * data, size_t len, unsigned char * out, size_t out_len) {
  sha3_template (data, len, out, out_len, 15 + 16, 136);
}

void shake128_xof_prepare (uint64_t * state, const unsigned char * data, size_t len) {
  sha3_prepare_msg (state, data, len, 15 + 16, 168);
}

void shake128_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len) {
  sponge_keccak_1600_squeeze (state, curr_offset, out, out_len, 168);
}

void shake256_xof_prepare (uint64_t * state, const unsigned char * data, size_t len) {
  sha3_prepare_msg (state, data, len, 15 + 16, 136);
}

void shake256_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len) {
  sponge_keccak_1600_squeeze (state, curr_offset, out, out_len, 136);
}

/* This template does not consider the case where both str_n and str_s are empty. */
void cshake_template (const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len, unsigned char final_byte, uint32_t rate) {
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;

  cshake_prepare_state (state, str_n, str_n_len, str_s, str_s_len, rate);
  sha3_prepare_msg (state, data, len, final_byte, rate);
  sponge_keccak_1600_squeeze (state, &curr_offset, out, out_len, rate);
}

void cshake128 (const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len) {
  if (str_n_len == 0 && str_s_len == 0) { shake128 (data, len, out, out_len); return; }
  cshake_template (str_n, str_n_len, str_s, str_s_len, data, len, out, out_len, 0 + 4, 168);
}

void cshake256 (const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len) {
  if (str_n_len == 0 && str_s_len == 0) { shake256 (data, len, out, out_len); return; }
  cshake_template (str_n, str_n_len, str_s, str_s_len, data, len, out, out_len, 0 + 4, 136);
}

void cshake128_xof_prepare (uint64_t * state, const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, const unsigned char * data, size_t len) {
  if (str_n_len != 0 || str_s_len != 0) cshake_prepare_state (state, str_n, str_n_len, str_s, str_s_len, 168);
  sha3_prepare_msg (state, data, len, 0 + 4, 168);
}

void cshake128_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len) {
  sponge_keccak_1600_squeeze (state, curr_offset, out, out_len, 168);
}

void cshake256_xof_prepare (uint64_t * state, const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, const unsigned char * data, size_t len) {
  if (str_n_len != 0 || str_s_len != 0) cshake_prepare_state (state, str_n, str_n_len, str_s, str_s_len, 136);
  sha3_prepare_msg (state, data, len, 0 + 4, 136);
}

void cshake256_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len) {
  sponge_keccak_1600_squeeze (state, curr_offset, out, out_len, 136);
}

void kmac_prepare_state (uint64_t * state, const unsigned char * str_s, size_t str_s_len, uint32_t rate) {
  cshake_prepare_state (state, (const unsigned char *) "KMAC", 4, str_s, str_s_len, rate);
}

void kmac_prepare_key (uint64_t * state, const unsigned char * key, size_t key_len, uint32_t rate) {
  uint32_t curr_offset = 0;

  /* bytepad */
  uint16_t pad = 1 + (rate << 8);
  sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &pad, 2, rate);

  /* encoding of key */
  left_encode (state, &curr_offset, key_len << 3, rate);
  sponge_keccak_1600_absorb (state, &curr_offset, key, key_len, rate);

  /* finish bytepad */
  if (curr_offset != 0) { keccak_p_1600_permute (state); }
}

/* KMAC requires appending output length after msg */
void kmac_prepare_msg (uint64_t * state, const unsigned char * data, size_t len, size_t out_len, unsigned char final_byte, uint32_t rate) {
  uint32_t curr_offset = 0;
  sponge_keccak_1600_absorb (state, &curr_offset, data, len, rate);
  right_encode (state, &curr_offset, out_len << 3, rate);
  sponge_keccak_1600_finalize (state, curr_offset, final_byte, rate);
}

void kmac_template (const unsigned char * str_s, size_t str_s_len, const unsigned char * key, size_t key_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len, unsigned char final_byte, uint32_t rate) {
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  kmac_prepare_state (state, str_s, str_s_len, rate);
  kmac_prepare_key (state, key, key_len, rate);
  kmac_prepare_msg (state, data, len, out_len, final_byte, rate);
  sponge_keccak_1600_squeeze (state, &curr_offset, out, out_len, rate);
}

void kmac128 (const unsigned char * str_s, size_t str_s_len, const unsigned char * key, size_t key_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len) {
  kmac_template (str_s, str_s_len, key, key_len, data, len, out, out_len, 0 + 4, 168);
}

void kmac256 (const unsigned char * str_s, size_t str_s_len, const unsigned char * key, size_t key_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len) {
  kmac_template (str_s, str_s_len, key, key_len, data, len, out, out_len, 0 + 4, 136);
}

void kmac128_xof_prepare (uint64_t * state, const unsigned char * str_s, size_t str_s_len, const unsigned char * key, size_t key_len, const unsigned char * data, size_t len) {
  kmac_prepare_state (state, str_s, str_s_len, 168);
  kmac_prepare_key (state, key, key_len, 168);
  kmac_prepare_msg (state, data, len, 0, 0 + 4, 168);
}

void kmac128_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len) {
  sponge_keccak_1600_squeeze (state, curr_offset, out, out_len, 168);
}

void kmac256_xof_prepare (uint64_t * state, const unsigned char * str_s, size_t str_s_len, const unsigned char * key, size_t key_len, const unsigned char * data, size_t len) {
  kmac_prepare_state (state, str_s, str_s_len, 136);
  kmac_prepare_key (state, key, key_len, 136);
  kmac_prepare_msg (state, data, len, 0, 0 + 4, 136);
}

void kmac256_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len) {
  sponge_keccak_1600_squeeze (state, curr_offset, out, out_len, 136);
}

void tuplehash_prepare_state (uint64_t * state, const unsigned char * str_s, size_t str_s_len, uint32_t rate) {
  cshake_prepare_state (state, (const unsigned char *) "TupleHash", 9, str_s, str_s_len, rate);
}

void tuplehash_add_msg (uint64_t * state, uint32_t * curr_offset, const unsigned char * str, size_t len, uint32_t rate) {
  left_encode (state, curr_offset, len << 3, rate);
  sponge_keccak_1600_absorb (state, curr_offset, str, len, rate);
}

void tuplehash_finalize (uint64_t * state, uint32_t * curr_offset, size_t out_len, unsigned char final_byte, uint32_t rate) {
  right_encode (state, curr_offset, out_len << 3, rate);
  sponge_keccak_1600_finalize (state, *curr_offset, final_byte, rate);
}

void tuplehash_template (const unsigned char * str_s, size_t str_s_len, const unsigned char * const * str_list, size_t * len_list, size_t num_of_str, unsigned char * out, size_t out_len, unsigned char final_byte, uint32_t rate) {
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  tuplehash_prepare_state (state, str_s, str_s_len, rate);
  for (size_t i = 0; i < num_of_str; ++i) {
    tuplehash_add_msg (state, &curr_offset, str_list[i], len_list[i], rate);
  }
  tuplehash_finalize (state, &curr_offset, out_len, final_byte, rate);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, out, out_len, rate);
}

void tuplehash128 (const unsigned char * str_s, size_t str_s_len, const unsigned char * const * str_list, size_t * len_list, size_t num_of_str, unsigned char * out, size_t out_len) {
  tuplehash_template (str_s, str_s_len, str_list, len_list, num_of_str, out, out_len, 0 + 4, 168);
}

void tuplehash256 (const unsigned char * str_s, size_t str_s_len, const unsigned char * const * str_list, size_t * len_list, size_t num_of_str, unsigned char * out, size_t out_len) {
  tuplehash_template (str_s, str_s_len, str_list, len_list, num_of_str, out, out_len, 0 + 4, 136);
}

void tuplehash128_xof_prepare (uint64_t * state, const unsigned char * str_s, size_t str_s_len, const unsigned char * const * str_list, size_t * len_list, size_t num_of_str) {
  uint32_t curr_offset = 0;
  tuplehash_prepare_state (state, str_s, str_s_len, 168);
  for (size_t i = 0; i < num_of_str; ++i) {
    tuplehash_add_msg (state, &curr_offset, str_list[i], len_list[i], 168);
  }
  tuplehash_finalize (state, &curr_offset, 0, 0 + 4, 168);
}

void tuplehash128_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len) {
  sponge_keccak_1600_squeeze (state, curr_offset, out, out_len, 168);
}

void tuplehash256_xof_prepare (uint64_t * state, const unsigned char * str_s, size_t str_s_len, const unsigned char * const * str_list, size_t * len_list, size_t num_of_str) {
  uint32_t curr_offset = 0;
  tuplehash_prepare_state (state, str_s, str_s_len, 136);
  for (size_t i = 0; i < num_of_str; ++i) {
    tuplehash_add_msg (state, &curr_offset, str_list[i], len_list[i], 136);
  }
  tuplehash_finalize (state, &curr_offset, 0, 0 + 4, 136);
}

void tuplehash256_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len) {
  sponge_keccak_1600_squeeze (state, curr_offset, out, out_len, 136);
}
