#ifndef AES_H
#define AES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void aes128_expandkey (const unsigned char * key, unsigned char * out_exkey);
void aes128_encrypt_one_block (const unsigned char * exkey, const unsigned char * data, unsigned char * ct_out);
void aes128_decrypt_one_block (const unsigned char * exkey, const unsigned char * ct, unsigned char * data_out);

void aes256_expandkey (const unsigned char * key, unsigned char * out_exkey);
void aes256_encrypt_one_block (const unsigned char * exkey, const unsigned char * data, unsigned char * ct_out);
void aes256_decrypt_one_block (const unsigned char * exkey, const unsigned char * ct, unsigned char * data_out);

void aes128_encrypt_gcm (const unsigned char * exkey, const unsigned char * iv, const unsigned char * add_data, size_t add_data_len, const unsigned char * data, size_t data_len, unsigned char * ct_out, unsigned char * tag_out);

#ifdef __cplusplus
}
#endif

#endif
