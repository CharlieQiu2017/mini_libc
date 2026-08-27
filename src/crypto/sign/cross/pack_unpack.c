#include <stddef.h>
#include <stdint.h>
#include <crypto/common.h>
#include <crypto/sign/cross/parameters.h>
#include <crypto/sign/cross/pack_unpack.h>

/* Length of out is expected to be (len * 7 - 1) / 8 + 1 */
void pack_7_bit (const uint8_t * in, size_t len, uint8_t * out) {
  while (len >= 8) {
    out[0] = in[0] | (in[1] << 7);
    out[1] = (in[1] >> 1) | (in[2] << 6);
    out[2] = (in[2] >> 2) | (in[3] << 5);
    out[3] = (in[3] >> 3) | (in[4] << 4);
    out[4] = (in[4] >> 4) | (in[5] << 3);
    out[5] = (in[5] >> 5) | (in[6] << 2);
    out[6] = (in[6] >> 6) | (in[7] << 1);

    len -= 8; in += 8; out += 7;
  }

  /* We have at most 7 elements left, hence 49 bits */
  uint64_t buf = 0;
  uint32_t buf_bits = 0;

  while (len) {
    buf |= ((uint64_t) *in) << buf_bits;
    buf_bits += 7;
    in++; len--;
  }

  while (buf_bits >= 8) {
    *out = buf;
    buf >>= 8;
    buf_bits -= 8;
    out++;
  }

  if (buf_bits) *out = buf;
}

/* Length of out is expected to be (len * 9 - 1) / 8 + 1 */
void pack_9_bit (const uint16_t * in, size_t len, uint8_t * out) {
  while (len >= 8) {
    out[0] = in[0];
    out[1] = (in[0] >> 8) | (in[1] << 1);
    out[2] = (in[1] >> 7) | (in[2] << 2);
    out[3] = (in[2] >> 6) | (in[3] << 3);
    out[4] = (in[3] >> 5) | (in[4] << 4);
    out[5] = (in[4] >> 4) | (in[5] << 5);
    out[6] = (in[5] >> 3) | (in[6] << 6);
    out[7] = (in[6] >> 2) | (in[7] << 7);
    out[8] = in[7] >> 1;

    len -= 8; in += 8; out += 9;
  }

  /* We have at most 7 elements left, hence 63 bits */
  uint64_t buf = 0;
  uint32_t buf_bits = 0;

  while (len) {
    buf |= ((uint64_t) *in) << buf_bits;
    buf_bits += 9;
    in++; len--;
  }

  while (buf_bits >= 8) {
    *out = buf;
    buf >>= 8;
    buf_bits -= 8;
    out++;
  }

  if (buf_bits) *out = buf;
}

/* Length of in is expected to be (out_len * 7 - 1) / 8 + 1 */
_Bool unpack_7_bit (const uint8_t * in, uint8_t * out, size_t out_len) {
  while (out_len >= 8) {
    out[0] = in[0] & 0x7F;
    out[1] = (in[0] >> 7) | ((in[1] & 0x3F) << 1);
    out[2] = (in[1] >> 6) | ((in[2] & 0x1F) << 2);
    out[3] = (in[2] >> 5) | ((in[3] & 0x0F) << 3);
    out[4] = (in[3] >> 4) | ((in[4] & 0x07) << 4);
    out[5] = (in[4] >> 3) | ((in[5] & 0x03) << 5);
    out[6] = (in[5] >> 2) | ((in[6] & 0x01) << 6);
    out[7] = in[6] >> 1;

    out_len -= 8; in += 7; out += 8;
  }

  /* If we stop here, then no padding is needed, and there is nothing to check */
  if (! out_len) return 1;

  /* If we reach this point, there are at most 7 elements left, or 49 bits */
  uint64_t buf = 0;
  uint32_t buf_bits = 0;

  while (buf_bits < out_len * 7) {
    buf |= ((uint64_t) *in) << buf_bits;
    buf_bits += 8;
    in++;
  }

  /* Bits higher than out_len * 7 should be all-zero. If they are all-zero then uint64_to_bool returns false, so padding_check == true. */
  _Bool padding_check = ! uint64_to_bool (buf >> (out_len * 7));

  /* We write the elements anyway to ensure constant-time, but maybe not necessary here. */
  while (out_len) {
    *out = buf & 0x7F;
    buf >>= 7;
    out_len--; out++;
  }

  return padding_check;
}

/* Length of in is expected to be (out_len * 9 - 1) / 8 + 1 */
_Bool unpack_9_bit (const uint8_t * in, uint16_t * out, size_t out_len) {
  while (out_len >= 8) {
    out[0] = ((uint16_t) in[0]) | ((((uint16_t) in[1]) & 0x01) << 8);
    out[1] = (((uint16_t) in[1]) >> 1) | ((((uint16_t) in[2]) & 0x03) << 7);
    out[2] = (((uint16_t) in[2]) >> 2) | ((((uint16_t) in[3]) & 0x07) << 6);
    out[3] = (((uint16_t) in[3]) >> 3) | ((((uint16_t) in[4]) & 0x0F) << 5);
    out[4] = (((uint16_t) in[4]) >> 4) | ((((uint16_t) in[5]) & 0x1F) << 4);
    out[5] = (((uint16_t) in[5]) >> 5) | ((((uint16_t) in[6]) & 0x3F) << 3);
    out[6] = (((uint16_t) in[6]) >> 6) | ((((uint16_t) in[7]) & 0x7F) << 2);
    out[7] = (((uint16_t) in[7]) >> 7) | (((uint16_t) in[8]) << 1);

    out_len -= 8; in += 9; out += 8;
  }

  /* If we stop here, then no padding is needed, and there is nothing to check */
  if (! out_len) return 1;

  /* If we reach this point, there are at most 7 elements left, or 63 bits */
  uint64_t buf = 0;
  uint32_t buf_bits = 0;

  while (buf_bits < out_len * 9) {
    buf |= ((uint64_t) *in) << buf_bits;
    buf_bits += 8;
    in++;
  }

  /* Bits higher than out_len * 9 should be all-zero. If they are all-zero then uint64_to_bool returns false, so padding_check == true. */
  _Bool padding_check = ! uint64_to_bool (buf >> (out_len * 9));

  /* We write the elements anyway to ensure constant-time, but maybe not necessary here. */
  while (out_len) {
    *out = buf & 0x1FF;
    buf >>= 9;
    out_len--; out++;
  }

  return padding_check;
}

void pack_syndrome (const uint16_t * in, uint8_t * out) {
  pack_9_bit (in, CROSS_N - CROSS_K, out);
}

void pack_resp_v (const uint8_t * in, uint8_t * out) {
  pack_7_bit (in, CROSS_M, out);
}

void pack_resp_y (const uint16_t * in, uint8_t * out) {
  pack_9_bit (in, CROSS_N, out);
}

_Bool unpack_syndrome (const uint8_t * in, uint16_t * out) {
  if (! unpack_9_bit (in, out, CROSS_N - CROSS_K)) return 0;

  _Bool bound_check = 1;

  for (uint32_t i = 0; i < CROSS_N - CROSS_K; ++i) {
    bound_check &= uint32_cmp_ge (CROSS_P - 1, out[i]);
  }

  return bound_check;
}

_Bool unpack_resp_v (const uint8_t * in, uint8_t * out) {
  if (! unpack_7_bit (in, out, CROSS_M)) return 0;

  _Bool bound_check = 1;

  for (uint32_t i = 0; i < CROSS_M; ++i) {
    bound_check &= uint32_cmp_ge (CROSS_Z - 1, out[i]);
  }

  return bound_check;
}

_Bool unpack_resp_y (const uint8_t * in, uint16_t * out) {
  if (! unpack_9_bit (in, out, CROSS_N)) return 0;

  _Bool bound_check = 1;

  for (uint32_t i = 0; i < CROSS_N; ++i) {
    bound_check &= uint32_cmp_ge (CROSS_P - 1, out[i]);
  }

  return bound_check;
}
