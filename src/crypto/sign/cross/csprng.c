#include <stdint.h>
#include <string.h>
#include <crypto/common.h>
#include <crypto/sort.h>
#include <crypto/hash/keccak/keccak_p.h>
#include <crypto/sign/cross/parameters.h>
#include <crypto/sign/cross/csprng.h>

void cross_csprng_prepare (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint64_t * state) {
  uint32_t curr_offset = 0;
  sponge_keccak_1600_absorb (state, &curr_offset, seed, seed_len, CROSS_SHAKE_RATE);
  sponge_keccak_1600_absorb (state, &curr_offset, salt, salt_len, CROSS_SHAKE_RATE);
  /* dom_id is in little-endian. */
  sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
  sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
}

void cross_csprng_string (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint32_t a, unsigned char * out) {
  uint64_t state[25] = {0};
  cross_csprng_prepare (seed, seed_len, salt, salt_len, dom_id, state);
  uint32_t curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, out, a * CROSS_LAMBDA / 8, CROSS_SHAKE_RATE);
}

void cross_csprng_string_2 (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint32_t a, unsigned char * out1, unsigned char * out2) {
  uint64_t state[25] = {0};
  cross_csprng_prepare (seed, seed_len, salt, salt_len, dom_id, state);
  uint32_t curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, out1, a * CROSS_LAMBDA / 8, CROSS_SHAKE_RATE);
  sponge_keccak_1600_squeeze (state, &curr_offset, out2, a * CROSS_LAMBDA / 8, CROSS_SHAKE_RATE);
}

void cross_csprng_vec_zm (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint8_t * out) {
  uint64_t state[25] = {0};
  cross_csprng_prepare (seed, seed_len, salt, salt_len, dom_id, state);
  uint32_t curr_offset = 0;
#define HASH_BYTES ((CROSS_LOG_Z * CROSS_M * CROSS_GEN_ATTEMPT - 1) / 8 + 1)
  unsigned char hash_data[HASH_BYTES];
  sponge_keccak_1600_squeeze (state, &curr_offset, hash_data, HASH_BYTES, CROSS_SHAKE_RATE);
#undef HASH_BYTES

  uint32_t hash_ptr = 0, buf = 0, buf_bits = 0;
  for (uint32_t i = 0; i < CROSS_M; ++i) {
    out[i] = 0;
    for (uint32_t j = 0; j < CROSS_GEN_ATTEMPT; ++j) {
      while (buf_bits < CROSS_LOG_Z) {
	buf |= ((uint32_t) hash_data[hash_ptr]) << buf_bits;
	hash_ptr++; buf_bits += 8;
      }
      uint32_t val = buf & ((1 << CROSS_LOG_Z) - 1);
      buf_bits -= CROSS_LOG_Z;
      buf >>= CROSS_LOG_Z;
      _Bool check = uint32_cmp_ge (CROSS_Z - 1, val);
      if (check) { out[i] = val; break; }
    }
  }
}

void cross_csprng_vec_zm_pn (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint8_t * out_z, uint16_t * out_p) {
  uint64_t state[25] = {0};
  cross_csprng_prepare (seed, seed_len, salt, salt_len, dom_id, state);
  uint32_t curr_offset = 0;
#define HASH_BYTES (((CROSS_LOG_Z * CROSS_M + CROSS_LOG_P * CROSS_N) * CROSS_GEN_ATTEMPT - 1) / 8 + 1)
  unsigned char hash_data[HASH_BYTES];
  sponge_keccak_1600_squeeze (state, &curr_offset, hash_data, HASH_BYTES, CROSS_SHAKE_RATE);
#undef HASH_BYTES

  uint32_t hash_ptr = 0, buf = 0, buf_bits = 0;

  for (uint32_t i = 0; i < CROSS_M; ++i) {
    out_z[i] = 0;
    for (uint32_t j = 0; j < CROSS_GEN_ATTEMPT; ++j) {
      while (buf_bits < CROSS_LOG_Z) {
	buf |= ((uint32_t) hash_data[hash_ptr]) << buf_bits;
	hash_ptr++; buf_bits += 8;
      }
      uint32_t val = buf & ((1 << CROSS_LOG_Z) - 1);
      buf_bits -= CROSS_LOG_Z;
      buf >>= CROSS_LOG_Z;
      _Bool check = uint32_cmp_ge (CROSS_Z - 1, val);
      if (check) { out_z[i] = val; break; }
    }
  }

  for (uint32_t i = 0; i < CROSS_N; ++i) {
    out_p[i] = 0;
    for (uint32_t j = 0; j < CROSS_GEN_ATTEMPT; ++j) {
      while (buf_bits < CROSS_LOG_P) {
	buf |= ((uint32_t) hash_data[hash_ptr]) << buf_bits;
	hash_ptr++; buf_bits += 8;
      }
      uint32_t val = buf & ((1 << CROSS_LOG_P) - 1);
      buf_bits -= CROSS_LOG_P;
      buf >>= CROSS_LOG_P;
      _Bool check = uint32_cmp_ge (CROSS_P - 1, val);
      if (check) { out_p[i] = val; break; }
    }
  }
}

void cross_csprng_mat_zp (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint8_t * out_z, uint16_t * out_p) {
  uint64_t state[25] = {0};
  cross_csprng_prepare (seed, seed_len, salt, salt_len, dom_id, state);
  uint32_t curr_offset = 0;
#define HASH_BYTES (((CROSS_LOG_Z * CROSS_M * (CROSS_N - CROSS_M) + CROSS_LOG_P * CROSS_K * (CROSS_N - CROSS_K)) * CROSS_GEN_ATTEMPT - 1) / 8 + 1)
  unsigned char hash_data[HASH_BYTES];
  sponge_keccak_1600_squeeze (state, &curr_offset, hash_data, HASH_BYTES, CROSS_SHAKE_RATE);
#undef HASH_BYTES

  uint32_t hash_ptr = 0, buf = 0, buf_bits = 0;

  for (uint32_t i = 0; i < CROSS_M * (CROSS_N - CROSS_M); ++i) {
    out_z[i] = 0;
    for (uint32_t j = 0; j < CROSS_GEN_ATTEMPT; ++j) {
      while (buf_bits < CROSS_LOG_Z) {
	buf |= ((uint32_t) hash_data[hash_ptr]) << buf_bits;
	hash_ptr++; buf_bits += 8;
      }
      uint32_t val = buf & ((1 << CROSS_LOG_Z) - 1);
      buf_bits -= CROSS_LOG_Z;
      buf >>= CROSS_LOG_Z;
      _Bool check = uint32_cmp_ge (CROSS_Z - 1, val);
      if (check) { out_z[i] = val; break; }
    }
  }

  for (uint32_t i = 0; i < CROSS_K * (CROSS_N - CROSS_K); ++i) {
    out_p[i] = 0;
    for (uint32_t j = 0; j < CROSS_GEN_ATTEMPT; ++j) {
      while (buf_bits < CROSS_LOG_P) {
	buf |= ((uint32_t) hash_data[hash_ptr]) << buf_bits;
	hash_ptr++; buf_bits += 8;
      }
      uint32_t val = buf & ((1 << CROSS_LOG_P) - 1);
      buf_bits -= CROSS_LOG_P;
      buf >>= CROSS_LOG_P;
      _Bool check = uint32_cmp_ge (CROSS_P - 1, val);
      if (check) { out_p[i] = val; break; }
    }
  }
}

void cross_csprng_vec_pt (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint16_t * out) {
  uint64_t state[25] = {0};
  cross_csprng_prepare (seed, seed_len, salt, salt_len, dom_id, state);
  uint32_t curr_offset = 0;
#define HASH_BYTES ((CROSS_LOG_P * CROSS_T * CROSS_GEN_ATTEMPT - 1) / 8 + 1)
  unsigned char hash_data[HASH_BYTES];
  sponge_keccak_1600_squeeze (state, &curr_offset, hash_data, HASH_BYTES, CROSS_SHAKE_RATE);
#undef HASH_BYTES

  uint32_t hash_ptr = 0, buf = 0, buf_bits = 0;
  for (uint32_t i = 0; i < CROSS_T; ++i) {
    out[i] = 0;
    for (uint32_t j = 0; j < CROSS_GEN_ATTEMPT; ++j) {
      while (buf_bits < CROSS_LOG_P) {
	buf |= ((uint32_t) hash_data[hash_ptr]) << buf_bits;
	hash_ptr++; buf_bits += 8;
      }
      uint32_t val = buf & ((1 << CROSS_LOG_P) - 1);
      buf_bits -= CROSS_LOG_P;
      buf >>= CROSS_LOG_P;
      _Bool check = uint32_cmp_ge (CROSS_P - 1, val);
      if (check) { out[i] = val; break; }
    }
  }
}

void cross_csprng_hamming_ball (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, unsigned char * out) {
  uint64_t state[25] = {0};
  cross_csprng_prepare (seed, seed_len, salt, salt_len, dom_id, state);
  uint32_t curr_offset = 0;
  uint32_t tags[CROSS_T];
  sponge_keccak_1600_squeeze (state, &curr_offset, (unsigned char *) tags, 4 * CROSS_T, CROSS_SHAKE_RATE);

  memset (out, 0, (CROSS_T - 1) / 8 + 1);

  for (uint32_t i = 0; i < CROSS_W; ++i) {
    tags[i] |= 1;
  }

  for (uint32_t i = CROSS_W; i < CROSS_T; ++i) {
    tags[i] &= ~1u;
  }

  /* Hard coded sorting length for now */
  safe_sort_uint32_147 (tags);

  for (uint32_t i = 0; i < CROSS_T; ++i) {
    out[i / 8] |= (tags[i] & 1) << (i % 8);
  }
}
