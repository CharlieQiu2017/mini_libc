#include <stdint.h>
#include <crypto/sign/cross/cross.h>
#include <crypto/sign/cross/parameters.h>
#include <io.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  struct cross_sk_t sk;
  struct cross_pk_t pk;

  read (0, sk.seed_sk, CROSS_KEYPAIR_SEED_SIZE);

  cross_rsdpg_1_fast_gen_key_from_seed (&sk, &pk);

  write (1, &pk, sizeof (struct cross_pk_t));

  exit (0);
}
