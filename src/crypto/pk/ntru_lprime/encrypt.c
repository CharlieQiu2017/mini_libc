#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <random.h>
#include <crypto/common.h>
#include <crypto/hash/keccak/keccak_p.h>
#include <crypto/pk/ntru_lprime/ntru_lprime.h>

#define NTRU_LPR_P 653
#define NTRU_LPR_Q 4621
#define NTRU_LPR_ROUND_ENC_LEN 865

void ntrulpr_653_encapsulate_internal (const unsigned char * pk, const unsigned char * input, const unsigned char * key_hash_cache, unsigned char * ct_out) {
  /* Expand G */
  uint16_t g[NTRU_LPR_P];
  ntrulpr_653_expand_seed (pk, g);

  /* Decode Round(aG) */
  uint16_t ag_round[NTRU_LPR_P];
  ntrulpr_653_decode_poly_round (pk + 32, ag_round);

  /* Call HashShort to generate b */
  uint8_t b[(NTRU_LPR_P - 1) / 4 + 1];
  ntrulpr_653_hashshort (input, b);

  /* Compute Round(bG) */
  uint16_t bg[NTRU_LPR_P];
  ntrulpr_653_poly_mult_short (g, b, NTRU_LPR_P, bg);
  ntrulpr_653_round (bg, bg);
  ntrulpr_653_encode_poly_round (bg, ct_out);

  /* Compute bottom 256 terms of b * Round(aG) */
  uint16_t b_ag_round[256];
  ntrulpr_653_poly_mult_short (ag_round, b, 256, b_ag_round);

  /* Compute T array */
  unsigned char * t_array = ct_out + NTRU_LPR_ROUND_ENC_LEN;
  memset (t_array, 0, 128);

  for (uint32_t i = 0; i < 256; ++i) {
    /* We need to compute Top(r_i * (q - 1) / 2 + b_ag_round[i]).
       However, note that Top(C) is defined upon the [-Q/2, Q/2] representation,
       as (T1 * (C + T0) + (1 << 14)) >> 15.
       Since we work with the [0, Q-1] representation, we have to convert it to the [-Q/2, Q/2] representation first.

       If C <= (q - 1) / 2 then we simply compute (T1 * (C + T0) + (1 << 14)).
       Expanding the expression gives us C * 113 + 262159
       If C >= (q + 1) / 2 then we compute (T1 * (C - Q + T0) + (1 << 14)).
       Expanding the expression gives us C * 113 - 260014
     */

    uint32_t t_i = (input[i >> 3] >> (i & 0x07)) & 1;
    t_i = t_i * ((NTRU_LPR_Q - 1) / 2) + b_ag_round[i];
    t_i = uint32_cmp_ge_branch (t_i, NTRU_LPR_Q, t_i - NTRU_LPR_Q, t_i);
    uint32_t top1 = 113 * t_i + 262159;
    /* 262159 + 260014 = 522173 */
    uint32_t top2 = top1 - 522173;
    t_i = uint32_cmp_ge_branch (t_i, (NTRU_LPR_Q + 1) / 2, top2, top1);
    t_i >>= 15;

    /* T_i is between 0 and 15, so use 4 bits to encode T_i */
    t_array[i >> 1] |= t_i << (4 * (i & 1));
  }

  /* Compute Key-hash */
  unsigned char key_hash[32];
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  uint8_t init_byte = 4;

  if (key_hash_cache == NULL) {
    sponge_keccak_1600_absorb (state, &curr_offset, &init_byte, 1, 72);
    sponge_keccak_1600_absorb (state, &curr_offset, pk, 32 + NTRU_LPR_ROUND_ENC_LEN, 72);
    sponge_keccak_1600_finalize (state, curr_offset, 2 + 4, 72);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, key_hash, 32, 72);
    key_hash_cache = key_hash;
  }

  /* Compute HashConfirm */
  init_byte = 2;
  memset (state, 0, 200);
  curr_offset = 0;
  sponge_keccak_1600_absorb (state, &curr_offset, &init_byte, 1, 72);
  sponge_keccak_1600_absorb (state, &curr_offset, input, 32, 72);
  sponge_keccak_1600_absorb (state, &curr_offset, key_hash_cache, 32, 72);
  sponge_keccak_1600_finalize (state, curr_offset, 2 + 4, 72);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, ct_out + NTRU_LPR_ROUND_ENC_LEN + 128, 32, 72);
}

void ntrulpr_653_encapsulate (const unsigned char * pk, unsigned char * ct_out, unsigned char * key_out) {
  /* Generate random input */
  unsigned char input[32];
  getrandom (input, 32, 0);

  /* Build ciphertext */
  ntrulpr_653_encapsulate_internal (pk, input, NULL, ct_out);

  /* Compute HashSession */
  uint64_t state[25] = {0};
  uint32_t curr_offset = 0;
  uint8_t init_byte = 1;
  sponge_keccak_1600_absorb (state, &curr_offset, &init_byte, 1, 72);
  sponge_keccak_1600_absorb (state, &curr_offset, input, 32, 72);
  sponge_keccak_1600_absorb (state, &curr_offset, ct_out, NTRU_LPR_ROUND_ENC_LEN + 128 + 32, 72);
  sponge_keccak_1600_finalize (state, curr_offset, 2 + 4, 72);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (state, &curr_offset, key_out, 32, 72);
}
