#include <stdint.h>
#include <crypto/sign/cross/cross.h>
#include <io.h>
#include <exit.h>

void main (__attribute__((unused)) void * sp) {
  struct cross_pk_t pk;
  uint8_t msg[32];
  struct cross_sig_t sig;

  read (0, &pk, sizeof (struct cross_pk_t));
  read (0, msg, 32);
  read (0, &sig, sizeof (struct cross_sig_t));

  _Bool result = cross_rsdpg_1_fast_verify (&pk, (const unsigned char *) msg, 32, &sig);

  exit (result ^ 1);
}
