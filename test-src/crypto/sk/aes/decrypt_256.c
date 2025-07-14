#include <stdint.h>
#include <crypto/sk/aes/aes.h>
#include <io.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  uint32_t key[8];
  uint32_t exkey[60];
  uint32_t block[4];
  uint32_t out[4];

  read (0, key, 32);
  read (0, block, 16);

  aes256_expandkey ((const unsigned char *) key, (unsigned char *) exkey);

  aes256_decrypt_one_block ((const unsigned char *) exkey, (const unsigned char *) block, (unsigned char *) out);

  write (1, out, 16);

  exit (0);
}
