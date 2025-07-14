/* AES Key Schedule
   Adapted from Linux kernel arch/arm64/crypto/aes-ce-core.S and related files
   We also adapted code from the patch "arm64/crypto: use crypto instructions to generate AES key schedule"
   See https://review.lineageos.org/c/LineageOS/android_kernel_xiaomi_msm8996/+/216351
 */

#include <stdint.h>
#include <string.h>
#include <arm_neon.h>
#include <crypto/sk/aes/aes.h>

static inline uint32_t aes_sub (uint32_t input) {
  uint32x4_t duplicated_input = vdupq_n_u32 (input);
  uint8x16_t duplicated_ret = vaeseq_u8 (vreinterpretq_u8_u32 (duplicated_input), vdupq_n_u8 (0));
  return vdups_laneq_u32 (vreinterpretq_u32_u8 (duplicated_ret), 0);
}

void aes128_expandkey (const unsigned char * key, unsigned char * out_exkey) {
  uint32_t buf[44];

  const uint8_t rcon[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};

  memcpy (buf, key, 16);

  for (uint32_t i = 0; i < 10; ++i) {
    uint32_t t = aes_sub (buf[i * 4 + 3]);
    t = (t >> 8) | (t << 24);
    t ^= rcon[i] ^ buf[i * 4];

    buf[(i + 1) * 4] = t; t ^= buf[i * 4 + 1];
    buf[(i + 1) * 4 + 1] = t; t ^= buf[i * 4 + 2];
    buf[(i + 1) * 4 + 2] = t; t ^= buf[i * 4 + 3];
    buf[(i + 1) * 4 + 3] = t;
  }

  memcpy (out_exkey, buf, 44 * 4);

  return;
}

void aes256_expandkey (const unsigned char * key, unsigned char * out_exkey) {
  uint32_t buf[60];

  const uint8_t rcon[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40};

  memcpy (buf, key, 32);

  /* First six iterations are full */
  for (uint32_t i = 0; i < 6; ++i) {
    uint32_t t = aes_sub (buf[i * 8 + 7]);
    t = (t >> 8) | (t << 24);
    t ^= rcon[i] ^ buf[i * 8];

    buf[(i + 1) * 8] = t; t ^= buf[i * 8 + 1];
    buf[(i + 1) * 8 + 1] = t; t ^= buf[i * 8 + 2];
    buf[(i + 1) * 8 + 2] = t; t ^= buf[i * 8 + 3];
    buf[(i + 1) * 8 + 3] = t; t = aes_sub (t) ^ buf[i * 8 + 4];
    buf[(i + 1) * 8 + 4] = t; t ^= buf[i * 8 + 5];
    buf[(i + 1) * 8 + 5] = t; t ^= buf[i * 8 + 6];
    buf[(i + 1) * 8 + 6] = t; t ^= buf[i * 8 + 7];
    buf[(i + 1) * 8 + 7] = t;
  }

  /* Final iteration */
  uint32_t t = aes_sub (buf[55]);
  t = (t >> 8) | (t << 24);
  t ^= rcon[6] ^ buf[48];

  buf[56] = t; t ^= buf[49];
  buf[57] = t; t ^= buf[50];
  buf[58] = t; t ^= buf[51];
  buf[59] = t;

  memcpy (out_exkey, buf, 60 * 4);
}
