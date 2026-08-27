#include <stdint.h>
#include <string.h>
#include <random.h>
#include <crypto/sign/cross/cross.h>
#include <crypto/sign/cross/parameters.h>
#include <crypto/sign/cross/csprng.h>
#include <crypto/sign/cross/fz_arith.h>
#include <crypto/sign/cross/fp_arith.h>
#include <crypto/sign/cross/pack_unpack.h>

void cross_rsdpg_1_fast_gen_key (struct cross_sk_t * sk_out, struct cross_pk_t * pk_out) {
  /* Sample 2 * LAMBDA random bits as seed_sk */
  getrandom (sk_out->seed_sk, CROSS_KEYPAIR_SEED_SIZE, 0);

  /* Expand the seed */
  cross_rsdpg_1_fast_gen_key_from_seed (sk_out, pk_out);
}

void cross_rsdpg_1_fast_gen_key_from_seed (const struct cross_sk_t * sk, struct cross_pk_t * pk_out) {
  /* (seed_e, seed_pk) <- CSPRNG(seed_sk | 3t + 1) */
  uint8_t seed_e[CROSS_KEYPAIR_SEED_SIZE];
  cross_csprng_string_2 (sk->seed_sk, CROSS_KEYPAIR_SEED_SIZE, NULL, 0, 3 * CROSS_T + 1, 2, seed_e, pk_out->seed_pk);

  /* Sample random matrices W and V */
  uint8_t mat_w[CROSS_M * (CROSS_N - CROSS_M)];
  uint16_t mat_v[CROSS_K * (CROSS_N - CROSS_K)];
  cross_csprng_mat_zp (pk_out->seed_pk, CROSS_KEYPAIR_SEED_SIZE, NULL, 0, 3 * CROSS_T + 2, mat_w, mat_v);

  /* The spec says: H <- [V | Id_(n-k)].
     But V in the spec is the transposition of our mat_v here.
     So we have actually have: H^T <- [   mat_v  ]
                                      [----------]
                                      [ Id_(n-k) ]
   */

  /* The spec says: M <- [W | Id_m] */

  /* e_bar_g <- CSPRNG(seed_e | 3t + 3) */
  uint8_t e_bar_g[CROSS_M];
  cross_csprng_vec_zm (seed_e, CROSS_KEYPAIR_SEED_SIZE, NULL, 0, 3 * CROSS_T + 3, e_bar_g);

  /* e_bar <- e_bar_g * M */
  uint8_t e_bar[CROSS_N] = {0};

  for (uint32_t i = 0; i < CROSS_N - CROSS_M; ++i) {
    for (uint32_t j = 0; j < CROSS_M; ++j) {
      e_bar[i] = fz_add (e_bar[i], fz_mult (e_bar_g[j], mat_w[j * (CROSS_N - CROSS_M) + i]));
    }
  }

  for (uint32_t i = CROSS_N - CROSS_M; i < CROSS_N; ++i) {
    e_bar[i] = e_bar_g[i - (CROSS_N - CROSS_M)];
  }

  /* e <- g^e_bar */
  uint16_t e[CROSS_N];

  for (uint32_t i = 0; i < CROSS_N; ++i) {
    e[i] = fz_to_fp (e_bar[i]);
  }

  /* s <- e * H^T */
  uint16_t syndrome[CROSS_N - CROSS_K] = {0};

  for (uint32_t i = 0; i < CROSS_N - CROSS_K; ++i) {
    for (uint32_t j = 0; j < CROSS_K; ++j) {
      syndrome[i] = fp_add (syndrome[i], fp_mult (e[j], mat_v[j * (CROSS_N - CROSS_K) + i]));
    }
    syndrome[i] = fp_add (syndrome[i], e[CROSS_K + i]);
  }

  /* Write pk_out->s */
  pack_syndrome (syndrome, pk_out->s);
}
