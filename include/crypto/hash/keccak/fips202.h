#ifndef FIPS202_H
#define FIPS202_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Helper functions for constructing Keccak input
   Do not use these without understanding their purpose.
 */

void left_encode (uint64_t * state, uint32_t * curr_offset, uint64_t n, uint32_t rate);
void right_encode (uint64_t * state, uint32_t * curr_offset, uint64_t n, uint32_t rate);
void cshake_prepare_state (uint64_t * state, const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, uint32_t rate);
void sha3_prepare_msg (uint64_t * state, const unsigned char * data, size_t len, unsigned char final_byte, uint32_t rate);
void sha3_template (const unsigned char * data, size_t len, unsigned char * out, size_t out_len, unsigned char final_byte, uint32_t rate);
void cshake_template (const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len, unsigned char final_byte, uint32_t rate);
void kmac_prepare_state (uint64_t * state, const unsigned char * str_s, size_t str_s_len, uint32_t rate);
void kmac_prepare_key (uint64_t * state, const unsigned char * key, size_t key_len, uint32_t rate);
void kmac_prepare_msg (uint64_t * state, const unsigned char * data, size_t len, size_t out_len, unsigned char final_byte, uint32_t rate);
void kmac_template (const unsigned char * str_s, size_t str_s_len, const unsigned char * key, size_t key_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len, unsigned char final_byte, uint32_t rate);
void tuplehash_prepare_state (uint64_t * state, const unsigned char * str_s, size_t str_s_len, uint32_t rate);
void tuplehash_add_msg (uint64_t * state, uint32_t * curr_offset, const unsigned char * str, size_t len, uint32_t rate);
void tuplehash_finalize (uint64_t * state, uint32_t * curr_offset, size_t out_len, unsigned char final_byte, uint32_t rate);
void tuplehash_template (const unsigned char * str_s, size_t str_s_len, const unsigned char * const * str_list, size_t * len_list, size_t num_of_str, unsigned char * out, size_t out_len, unsigned char final_byte, uint32_t rate);

/* The "high-level" functions */

void sha3_224 (const unsigned char * data, size_t len, unsigned char * out);
void sha3_256 (const unsigned char * data, size_t len, unsigned char * out);
void sha3_384 (const unsigned char * data, size_t len, unsigned char * out);
void sha3_512 (const unsigned char * data, size_t len, unsigned char * out);

void shake128 (const unsigned char * data, size_t len, unsigned char * out, size_t out_len);
void shake256 (const unsigned char * data, size_t len, unsigned char * out, size_t out_len);

void cshake128 (const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len);
void cshake256 (const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len);

void shake128_xof_prepare (uint64_t * state, const unsigned char * data, size_t len);
void shake128_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len);
void shake256_xof_prepare (uint64_t * state, const unsigned char * data, size_t len);
void shake256_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len);

void cshake128 (const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len);
void cshake256 (const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len);

void cshake128_xof_prepare (uint64_t * state, const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, const unsigned char * data, size_t len);
void cshake128_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len);
void cshake256_xof_prepare (uint64_t * state, const unsigned char * str_n, size_t str_n_len, const unsigned char * str_s, size_t str_s_len, const unsigned char * data, size_t len);
void cshake256_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len);

void kmac128 (const unsigned char * str_s, size_t str_s_len, const unsigned char * key, size_t key_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len);
void kmac256 (const unsigned char * str_s, size_t str_s_len, const unsigned char * key, size_t key_len, const unsigned char * data, size_t len, unsigned char * out, size_t out_len);

void kmac128_xof_prepare (uint64_t * state, const unsigned char * str_s, size_t str_s_len, const unsigned char * key, size_t key_len, const unsigned char * data, size_t len);
void kmac128_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len);
void kmac256_xof_prepare (uint64_t * state, const unsigned char * str_s, size_t str_s_len, const unsigned char * key, size_t key_len, const unsigned char * data, size_t len);
void kmac256_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len);

void tuplehash128 (const unsigned char * str_s, size_t str_s_len, const unsigned char * const * str_list, size_t * len_list, size_t num_of_str, unsigned char * out, size_t out_len);
void tuplehash256 (const unsigned char * str_s, size_t str_s_len, const unsigned char * const * str_list, size_t * len_list, size_t num_of_str, unsigned char * out, size_t out_len);

void tuplehash128_xof_prepare (uint64_t * state, const unsigned char * str_s, size_t str_s_len, const unsigned char * const * str_list, size_t * len_list, size_t num_of_str);
void tuplehash128_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len);
void tuplehash256_xof_prepare (uint64_t * state, const unsigned char * str_s, size_t str_s_len, const unsigned char * const * str_list, size_t * len_list, size_t num_of_str);
void tuplehash256_xof_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len);

#ifdef __cplusplus
}
#endif

#endif
