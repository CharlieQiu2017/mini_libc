#include <stdint.h>
#include <crypto/sign/cross/cross.h>
#include <crypto/sign/cross/parameters.h>
#include <io.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  struct cross_sk_t sk;
  uint8_t msg[32];
  uint8_t seed[CROSS_SEED_SIZE];
  uint8_t salt[CROSS_SALT_SIZE];
  struct cross_sig_t sig;

  read (0, &sk, sizeof (struct cross_sk_t));
  read (0, msg, 32);
  read (0, seed, CROSS_SEED_SIZE);
  read (0, salt, CROSS_SALT_SIZE);

  cross_rsdpg_1_fast_sign_with_seed_salt (&sk, seed, salt, (const unsigned char *) msg, 32, &sig);

  write (1, &sig, sizeof (struct cross_sig_t));

  exit (0);
}
