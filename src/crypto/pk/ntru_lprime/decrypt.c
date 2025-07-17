#include <stdint.h>
#include <string.h>
#include <random.h>
#include <crypto/common.h>
#include <crypto/hash/keccak/keccak_p.h>
#include <crypto/pk/ntru_lprime/ntru_lprime.h>

#define NTRU_LPR_P 653
#define NTRU_LPR_Q 4621
#define NTRU_LPR_W 252
#define NTRU_LPR_T2 2031
#define NTRU_LPR_T3 290
#define NTRU_LPR_ROUND_ENC_LEN 865
#define NTRU_LPR_SHORT_ENC_LEN ((NTRU_LPR_P - 1) / 4 + 1)
#define NTRU_LPR_PK_LEN (32 + NTRU_LPR_ROUND_ENC_LEN)
#define NTRU_LPR_CT_LEN (NTRU_LPR_ROUND_ENC_LEN + 128 + 32)

void ntrulpr_653_decapsulate (const unsigned char * sk, const unsigned char * ct, unsigned char * key_out) {
  /* Decode B = Round(bG) */
  uint16_t b[NTRU_LPR_P];
  ntrulpr_653_decode_poly_round (ct, b);

  /* Compute bottom 256 terms of aB */
  uint16_t ab[256];
  ntrulpr_653_poly_mult_short (b, (const uint8_t *) sk, 256, ab);

  /* Compute Right(T_i) - (aB)_i + 4w + 1.

     The Top(C) and Right(C) functions are defined such that,
     for every element x in Z/q, let y = Right(Top(x)) - x,
     then y under the [-Q/2, Q/2] representation satisfies 0 <= y <= 289.
     Since 289 < (Q - 1) / 2, it does not matter whether we work in the [-Q/2, Q/2] representation or [0, Q-1] representation.
     We simply interpret Right(T_i) as a value in the [0, Q-1] representation.
   */
  const unsigned char * t_array = ct + NTRU_LPR_ROUND_ENC_LEN;
  unsigned char secret[32] = {0};

  for (uint32_t i = 0; i < 256; ++i) {
    uint32_t r_i = (t_array[i >> 1] >> (4 * (i & 1))) & 0x0f;
    r_i = r_i * NTRU_LPR_T3 + (NTRU_LPR_Q - NTRU_LPR_T2) + 4 * NTRU_LPR_W + 1;
    r_i = uint32_cmp_ge_branch (r_i, NTRU_LPR_Q, r_i - NTRU_LPR_Q, r_i);
    r_i += NTRU_LPR_Q - ab[i];
    r_i = uint32_cmp_ge_branch (r_i, NTRU_LPR_Q, r_i - NTRU_LPR_Q, r_i);

    r_i = uint32_cmp_ge_branch (r_i, (NTRU_LPR_Q + 1) / 2, 1, 0);
    secret[i >> 3] |= r_i << (i & 0x07);
  }

  /* Encapsulate again and compare the ciphertext */
  unsigned char new_ct[NTRU_LPR_ROUND_ENC_LEN + 128 + 32];
  ntrulpr_653_encapsulate_internal (sk + NTRU_LPR_SHORT_ENC_LEN, secret, sk + NTRU_LPR_SHORT_ENC_LEN + NTRU_LPR_PK_LEN + 32, new_ct);
  uint64_t cmp = safe_memcmp (ct, new_ct, NTRU_LPR_CT_LEN);
  uint8_t cmp_byte = 1 - (uint8_t) uint64_to_bool (cmp);
  cond_memcpy (1 - cmp_byte, secret, sk + NTRU_LPR_SHORT_ENC_LEN + NTRU_LPR_PK_LEN, 32);

  /* Compute HashSession */
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  sponge_keccak_1600_absorb (state, &curr_offset, &cmp_byte, 1, 72);
  sponge_keccak_1600_absorb (state, &curr_offset, secret, 32, 72);
  sponge_keccak_1600_absorb (state, &curr_offset, ct, NTRU_LPR_CT_LEN, 72);
  sponge_keccak_1600_finalize (state, curr_offset, 2 + 4, 72);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, key_out, 32, 72);
}
