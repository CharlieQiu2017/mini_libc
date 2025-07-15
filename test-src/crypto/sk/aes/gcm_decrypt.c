#include <stdint.h>
#include <stddef.h>
#include <crypto/sk/aes/aes.h>
#include <io.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  uint32_t key[4];
  uint32_t exkey[44];

  unsigned char iv[12];

  size_t ct_len = 0, add_data_len = 0;
  unsigned char ct[256];
  unsigned char add_data[256];

  unsigned char data[256];
  unsigned char tag[16];

  read (0, key, 16);
  read (0, iv, 12);
  read (0, &ct_len, 1);
  if (ct_len > 0) read (0, ct, ct_len);
  read (0, &add_data_len, 1);
  if (add_data_len > 0) read (0, add_data, add_data_len);

  aes128_expandkey ((unsigned char *) key, (unsigned char *) exkey);

  aes128_decrypt_gcm ((unsigned char *) exkey, iv, add_data, add_data_len, ct, ct_len, data, tag);

  write (1, data, ct_len);
  write (1, tag, 16);

  exit (0);
}
