#ifndef AES_NEON_H
#define AES_NEON_H

#include <arm_neon.h>

uint8x16_t aes128_encrypt_one_block_neon (const unsigned char * exkey, uint8x16_t block);
uint8x16_t aes128_decrypt_one_block_neon (const unsigned char * exkey, uint8x16_t block);
uint8x16_t aes256_encrypt_one_block_neon (const unsigned char * exkey, uint8x16_t block);
uint8x16_t aes256_decrypt_one_block_neon (const unsigned char * exkey, uint8x16_t block);

#endif
