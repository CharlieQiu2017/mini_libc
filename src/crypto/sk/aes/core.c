#include <stdint.h>
#include <arm_neon.h>
#include <crypto/sk/aes/aes.h>

uint8x16_t aes128_encrypt_one_block_neon (const unsigned char * exkey, uint8x16_t block) {
  for (uint32_t i = 0; i < 9; ++i) {
    uint8x16_t rkey = vld1q_u8 ((const uint8_t *) (exkey + 16 * i));
    block = vaeseq_u8 (block, rkey);
    block = vaesmcq_u8 (block);
  }

  uint8x16_t rkey9 = vld1q_u8 ((const uint8_t *) (exkey + 16 * 9));
  block = vaeseq_u8 (block, rkey9);

  uint8x16_t rkey10 = vld1q_u8 ((const uint8_t *) (exkey + 16 * 10));
  block = veorq_u8 (block, rkey10);

  return block;
}

void aes128_encrypt_one_block (const unsigned char * exkey, const unsigned char * data, unsigned char * ct_out) {
  uint8x16_t block = vld1q_u8 ((const uint8_t *) data);
  block = aes128_encrypt_one_block_neon (exkey, block);
  vst1q_u8 ((uint8_t *) ct_out, block);
  return;
}

uint8x16_t aes128_decrypt_one_block_neon (const unsigned char * exkey, uint8x16_t block) {
  uint8x16_t rkey10 = vld1q_u8 ((const uint8_t *) (exkey + 16 * 10));
  block = vaesdq_u8 (block, rkey10);
  block = vaesimcq_u8 (block);

  for (uint32_t i = 9; i > 1; --i) {
    uint8x16_t rkey = vld1q_u8 ((const uint8_t *) (exkey + 16 * i));
    rkey = vaesimcq_u8 (rkey);
    block = vaesdq_u8 (block, rkey);
    block = vaesimcq_u8 (block);
  }

  uint8x16_t rkey1 = vld1q_u8 ((const uint8_t *) (exkey + 16));
  rkey1 = vaesimcq_u8 (rkey1);
  block = vaesdq_u8 (block, rkey1);

  uint8x16_t rkey0 = vld1q_u8 ((const uint8_t *) exkey);
  block = veorq_u8 (block, rkey0);

  return block;
}

void aes128_decrypt_one_block (const unsigned char * exkey, const unsigned char * ct, unsigned char * data_out) {
  uint8x16_t block = vld1q_u8 ((const uint8_t *) ct);
  block = aes128_decrypt_one_block_neon (exkey, block);
  vst1q_u8 ((uint8_t *) data_out, block);
  return;
}

uint8x16_t aes256_encrypt_one_block_neon (const unsigned char * exkey, uint8x16_t block) {
  for (uint32_t i = 0; i < 13; ++i) {
    uint8x16_t rkey = vld1q_u8 ((const uint8_t *) (exkey + 16 * i));
    block = vaeseq_u8 (block, rkey);
    block = vaesmcq_u8 (block);
  }

  uint8x16_t rkey13 = vld1q_u8 ((const uint8_t *) (exkey + 16 * 13));
  block = vaeseq_u8 (block, rkey13);

  uint8x16_t rkey14 = vld1q_u8 ((const uint8_t *) (exkey + 16 * 14));
  block = veorq_u8 (block, rkey14);

  return block;
}

void aes256_encrypt_one_block (const unsigned char * exkey, const unsigned char * data, unsigned char * ct_out) {
  uint8x16_t block = vld1q_u8 ((const uint8_t *) data);
  block = aes256_encrypt_one_block_neon (exkey, block);
  vst1q_u8 ((uint8_t *) ct_out, block);
  return;
}

uint8x16_t aes256_decrypt_one_block_neon (const unsigned char * exkey, uint8x16_t block) {
  uint8x16_t rkey14 = vld1q_u8 ((const uint8_t *) (exkey + 16 * 14));
  block = vaesdq_u8 (block, rkey14);
  block = vaesimcq_u8 (block);

  for (uint32_t i = 13; i > 1; --i) {
    uint8x16_t rkey = vld1q_u8 ((const uint8_t *) (exkey + 16 * i));
    rkey = vaesimcq_u8 (rkey);
    block = vaesdq_u8 (block, rkey);
    block = vaesimcq_u8 (block);
  }

  uint8x16_t rkey1 = vld1q_u8 ((const uint8_t *) (exkey + 16));
  rkey1 = vaesimcq_u8 (rkey1);
  block = vaesdq_u8 (block, rkey1);

  uint8x16_t rkey0 = vld1q_u8 ((const uint8_t *) exkey);
  block = veorq_u8 (block, rkey0);

  return block;
}

void aes256_decrypt_one_block (const unsigned char * exkey, const unsigned char * ct, unsigned char * data_out) {
  uint8x16_t block = vld1q_u8 ((const uint8_t *) ct);
  block = aes256_decrypt_one_block_neon (exkey, block);
  vst1q_u8 ((uint8_t *) data_out, block);
  return;
}
