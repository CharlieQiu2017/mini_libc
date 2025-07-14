#include <stdint.h>
#include <string.h>
#include <random.h>
#include <crypto/hash/keccak/keccak_p.h>
#include <crypto/pk/ntru_lprime/ntru_lprime.h>

#define NTRU_LPR_P 653
#define NTRU_LPR_ROUND_ENC_LEN 865
#define NTRU_LPR_SHORT_ENC_LEN ((NTRU_LPR_P - 1) / 4 + 1)
#define NTRU_LPR_PK_LEN (32 + NTRU_LPR_ROUND_ENC_LEN)

void ntrulpr_653_gen_key (unsigned char * sk_out, unsigned char * pk_out) {
  /* Generate seed S */
  getrandom (pk_out, 32, 0);

  /* Expand S to G */
  uint16_t g[NTRU_LPR_P];
  ntrulpr_653_expand_seed (pk_out, g);

  /* Generate a short a
     We randomly choose a seed and then call hashshort
   */
  unsigned char a_seed[32];
  getrandom (a_seed, 32, 0);
  ntrulpr_653_hashshort (a_seed, sk_out);

  /* Compute aG */
  uint16_t ag[NTRU_LPR_P];
  ntrulpr_653_poly_mult_short (g, sk_out, NTRU_LPR_P, ag);

  /* Round(aG) */
  ntrulpr_653_round (ag, ag);
  ntrulpr_653_encode_poly_round (ag, pk_out + 32);

  /* Copy public key to secret key */
  memcpy (sk_out + NTRU_LPR_SHORT_ENC_LEN, pk_out, NTRU_LPR_PK_LEN);

  /* Random bytes rho */
  getrandom (sk_out + NTRU_LPR_SHORT_ENC_LEN + NTRU_LPR_PK_LEN, 32, 0);

  /* Key-hash cache */
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  uint8_t init_byte = 4;
  sponge_keccak_1600_absorb (state, &curr_offset, &init_byte, 1, 72);
  sponge_keccak_1600_absorb (state, &curr_offset, pk_out, NTRU_LPR_PK_LEN, 72);
  sponge_keccak_1600_finalize (state, curr_offset, 2 + 4, 72);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, sk_out + NTRU_LPR_SHORT_ENC_LEN + NTRU_LPR_PK_LEN + 32, 32, 72);

  return;
}
