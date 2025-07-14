#ifndef KECCAK_P_H
#define KECCAK_P_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The "low-level" functions
   "state" is an array of 25 uint64_t's.
 */

/* Keccak-p with the default number of rounds.
   Keccak-p[1600] has 24 rounds.
 */

void keccak_p_1600_permute (uint64_t * state);

/* Kravatte uses Keccak-p with reduced rounds */

void keccak_p_1600_6_permute (uint64_t * state);

/* Since we assume little-endian architecture, simply treat state as a sequence of bytes */
static inline __attribute__((always_inline)) void keccak_p_1600_extract_bytes (const uint64_t * state, unsigned char * data, uint32_t offset, uint32_t length) {
  memcpy (data, ((unsigned char *) state) + offset, length);
}

static inline __attribute__((always_inline)) void keccak_p_1600_xor_bytes (uint64_t * state, const unsigned char * data, uint32_t offset, uint32_t length) {
  memxor (((unsigned char *) state) + offset, data, length);
}

/* The "mid-level" functions
   In the following functions, rate is measured in bytes.
   The relation between "rate" and "capacity" is rate + capacity = 1600 bits.
 */

static inline __attribute__((always_inline)) void sponge_keccak_1600_absorb (uint64_t * state, uint32_t * curr_offset, const unsigned char * data, size_t len, uint32_t rate) {
  uint32_t t = rate - *curr_offset;
  if (len < t) {
    keccak_p_1600_xor_bytes (state, data, *curr_offset, len);
    *curr_offset += len;
    return;
  }

  keccak_p_1600_xor_bytes (state, data, *curr_offset, t);
  keccak_p_1600_permute (state);
  data += t;
  len -= t;

  while (len >= rate) {
    keccak_p_1600_xor_bytes (state, data, 0, rate);
    keccak_p_1600_permute (state);
    data += rate;
    len -= rate;
  }

  keccak_p_1600_xor_bytes (state, data, 0, len);
  *curr_offset = len;
}

static inline __attribute__((always_inline)) void sponge_keccak_1600_finalize (uint64_t * state, uint32_t curr_offset, unsigned char final_byte, uint32_t rate) {
  if (curr_offset < rate - 1) {
    keccak_p_1600_xor_bytes (state, &final_byte, curr_offset, 1);
    final_byte = 128;
    keccak_p_1600_xor_bytes (state, &final_byte, rate - 1, 1);
  } else {
    final_byte |= 128;
    keccak_p_1600_xor_bytes (state, &final_byte, rate - 1, 1);
  }

  keccak_p_1600_permute (state);
}

static inline __attribute__((always_inline)) void sponge_keccak_1600_squeeze (uint64_t * state, uint32_t * curr_offset, unsigned char * out, size_t out_len, uint32_t rate) {
  if (*curr_offset == rate) {
    keccak_p_1600_permute (state);
    *curr_offset = 0;
  }

  if (out_len < rate - *curr_offset) {
    keccak_p_1600_extract_bytes (state, out, *curr_offset, out_len);
    *curr_offset += out_len;
    return;
  }

  uint32_t t = rate - *curr_offset;
  keccak_p_1600_extract_bytes (state, out, *curr_offset, t);
  out += t;
  out_len -= t;

  while (out_len > rate) {
    keccak_p_1600_extract_bytes (state, out, 0, rate);
    keccak_p_1600_permute (state);
    out += rate;
    out_len -= rate;
  }

  keccak_p_1600_extract_bytes (state, out, 0, out_len);
  *curr_offset = out_len;
}


#ifdef __cplusplus
}
#endif

#endif
