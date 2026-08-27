#include <stdint.h>
#include <string.h>
#include <random.h>
#include <crypto/hash/keccak/keccak_p.h>
#include <crypto/sign/cross/cross.h>
#include <crypto/sign/cross/parameters.h>
#include <crypto/sign/cross/csprng.h>
#include <crypto/sign/cross/pack_unpack.h>
#include <crypto/sign/cross/fz_arith.h>
#include <crypto/sign/cross/fp_arith.h>

void cross_rsdpg_1_fast_sign_with_seed_salt (const struct cross_sk_t * sk, const uint8_t * seed, const uint8_t * salt, const unsigned char * msg, size_t msg_len, struct cross_sig_t * sig_out) {
  /* Copy salt to signature */
  memcpy (sig_out->salt, salt, CROSS_SALT_SIZE);

  /* e_bar, e_bar_g, H, M <- ExpandSK(seed_sk) */
  /* See gen_key.c */

  uint8_t seed_pk[CROSS_KEYPAIR_SEED_SIZE], seed_e[CROSS_KEYPAIR_SEED_SIZE];
  uint8_t e_bar_g[CROSS_M];
  uint8_t e_bar[CROSS_N] = {0};
  uint8_t mat_w[CROSS_M * (CROSS_N - CROSS_M)];
  uint16_t mat_v[CROSS_K * (CROSS_N - CROSS_K)];

  cross_csprng_string_2 (sk->seed_sk, CROSS_KEYPAIR_SEED_SIZE, NULL, 0, 3 * CROSS_T + 1, 2, seed_e, seed_pk);
  cross_csprng_mat_zp (seed_pk, CROSS_KEYPAIR_SEED_SIZE, NULL, 0, 3 * CROSS_T + 2, mat_w, mat_v);
  cross_csprng_vec_zm (seed_e, CROSS_KEYPAIR_SEED_SIZE, NULL, 0, 3 * CROSS_T + 3, e_bar_g);

  for (uint32_t i = 0; i < CROSS_N - CROSS_M; ++i) {
    for (uint32_t j = 0; j < CROSS_M; ++j) {
      e_bar[i] = fz_add (e_bar[i], fz_mult (e_bar_g[j], mat_w[j * (CROSS_N - CROSS_M) + i]));
    }
  }

  for (uint32_t i = CROSS_N - CROSS_M; i < CROSS_N; ++i) {
    e_bar[i] = e_bar_g[i - (CROSS_N - CROSS_M)];
  }

  /* SeedLeaves(Seed | Salt) */
  uint8_t seed_layer2[4 * CROSS_SEED_SIZE];
  uint8_t seed_leaves[CROSS_T * CROSS_SEED_SIZE];

  cross_csprng_string (seed, CROSS_SEED_SIZE, salt, CROSS_SALT_SIZE, 0, 4, seed_layer2);

  const uint32_t leaf_offset1 = (CROSS_T - 1) / 4 + 1;
  const uint32_t leaf_offset2 = leaf_offset1 + (CROSS_T - 2) / 4 + 1;
  const uint32_t leaf_offset3 = leaf_offset2 + (CROSS_T - 3) / 4 + 1;
  /* It can be shown that CROSS_T - leaf_offset3 == CROSS_T / 4 */

  cross_csprng_string (seed_layer2, CROSS_SEED_SIZE, salt, CROSS_SALT_SIZE, 1, leaf_offset1, seed_leaves);
  cross_csprng_string (seed_layer2 + CROSS_SEED_SIZE, CROSS_SEED_SIZE, salt, CROSS_SALT_SIZE, 2, leaf_offset2 - leaf_offset1, seed_leaves + leaf_offset1 * CROSS_SEED_SIZE);
  cross_csprng_string (seed_layer2 + 2 * CROSS_SEED_SIZE, CROSS_SEED_SIZE, salt, CROSS_SALT_SIZE, 3, leaf_offset3 - leaf_offset2, seed_leaves + leaf_offset2 * CROSS_SEED_SIZE);
  cross_csprng_string (seed_layer2 + 3 * CROSS_SEED_SIZE, CROSS_SEED_SIZE, salt, CROSS_SALT_SIZE, 4, CROSS_T - leaf_offset3, seed_leaves + leaf_offset3 * CROSS_SEED_SIZE);

  /* for i from 1 to t do ... */
  /* The values computed in the loop that we need later are u_prime, v_bar_g, e_prime, cmt0, cmt1 */
  uint16_t u_prime[CROSS_T * CROSS_N];
  /* v_bar_g will only be used in packed form outside the loop */
  uint8_t v_bar_g_packed[CROSS_T * CROSS_PACKED_RESP_V_SIZE];
  uint16_t e_prime[CROSS_T * CROSS_N];
  uint8_t cmt0[CROSS_T * CROSS_HASH_DIGEST_SIZE];
  uint8_t cmt1[CROSS_T * CROSS_HASH_DIGEST_SIZE];

  for (uint32_t i = 0; i < CROSS_T; ++i) {
    /* e_bar_g_prime, u_prime <- CSPRNG(Seed[i], Salt, i + c), c == 2t - 1 */
    /* Note that the spec uses 1-indexed round numbers, so replace i with i + 1 */
    uint8_t e_bar_g_prime[CROSS_M];
    cross_csprng_vec_zm_pn (seed_leaves + i * CROSS_SEED_SIZE, CROSS_SEED_SIZE, salt, CROSS_SALT_SIZE, 2 * CROSS_T + i, e_bar_g_prime, u_prime + i * CROSS_N);

    /* v_bar_g <- e_bar_g - e_bar_g_prime */
    uint8_t v_bar_g[CROSS_M];

    for (uint32_t j = 0; j < CROSS_M; ++j) {
      v_bar_g[j] = fz_add (e_bar_g[j], fz_neg (e_bar_g_prime[j]));
    }

    /* e_bar_prime <- e_bar_g_prime * M */
    uint8_t e_bar_prime[CROSS_N] = {0};

    for (uint32_t j = 0; j < CROSS_N - CROSS_M; ++j) {
      for (uint32_t k = 0; k < CROSS_M; ++k) {
	e_bar_prime[j] = fz_add (e_bar_prime[j], fz_mult (e_bar_g_prime[k], mat_w[k * (CROSS_N - CROSS_M) + j]));
      }
    }

    for (uint32_t j = CROSS_N - CROSS_M; j < CROSS_N; ++j) {
      e_bar_prime[j] = e_bar_g_prime[j - (CROSS_N - CROSS_M)];
    }

    /* e_prime <- g^e_bar_prime */
    for (uint32_t j = 0; j < CROSS_N; ++j) {
      e_prime[i * CROSS_N + j] = fz_to_fp (e_bar_prime[j]);
    }

    /* v <- g^v_bar where v_bar = e_bar - e_bar_prime */
    uint16_t v[CROSS_N];

    for (uint32_t j = 0; j < CROSS_N; ++j) {
      v[j] = fz_to_fp (fz_add (e_bar[j], fz_neg (e_bar_prime[j])));
    }

    /* u <- v * u_prime with component-wise multiplication */
    uint16_t u[CROSS_N];

    for (uint32_t j = 0; j < CROSS_N; ++j) {
      u[j] = fp_mult (v[j], u_prime[i * CROSS_N + j]);
    }

    /* syndrome_prime <- u * H^T */
    uint16_t syndrome_prime[CROSS_N - CROSS_K] = {0};

    for (uint32_t j = 0; j < CROSS_N - CROSS_K; ++j) {
      for (uint32_t k = 0; k < CROSS_K; ++k) {
	syndrome_prime[j] = fp_add (syndrome_prime[j], fp_mult (u[k], mat_v[k * (CROSS_N - CROSS_K) + j]));
      }
      syndrome_prime[j] = fp_add (syndrome_prime[j], u[CROSS_K + j]);
    }

    /* cmt0 <- Hash(syndrome_prime || v_bar_g || Salt || i + c) */
    /* According to reference implementation, syndrome_prime and v_bar_g are converted to packed format before hashing */
    uint8_t syndrome_prime_packed[CROSS_PACKED_SYN_SIZE];

    pack_syndrome (syndrome_prime, syndrome_prime_packed);
    pack_resp_v (v_bar_g, v_bar_g_packed + i * CROSS_PACKED_RESP_V_SIZE);

    {
      uint64_t state[25] = {0};
      uint32_t curr_offset = 0;
      sponge_keccak_1600_absorb (state, &curr_offset, syndrome_prime_packed, CROSS_PACKED_SYN_SIZE, CROSS_SHAKE_RATE);
      sponge_keccak_1600_absorb (state, &curr_offset, v_bar_g_packed + i * CROSS_PACKED_RESP_V_SIZE, CROSS_PACKED_RESP_V_SIZE, CROSS_SHAKE_RATE);
      sponge_keccak_1600_absorb (state, &curr_offset, salt, CROSS_SALT_SIZE, CROSS_SHAKE_RATE);
      uint16_t dom_id = (2 * CROSS_T + i) | (1 << 15);
      sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
      sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
      curr_offset = 0;
      sponge_keccak_1600_squeeze (state, &curr_offset, cmt0 + i * CROSS_HASH_DIGEST_SIZE, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    }

    /* cmt1 <- Hash(Seed[i] || Salt || i + c) */
    {
      uint64_t state[25] = {0};
      uint32_t curr_offset = 0;
      sponge_keccak_1600_absorb (state, &curr_offset, seed_leaves + i * CROSS_SEED_SIZE, CROSS_SEED_SIZE, CROSS_SHAKE_RATE);
      sponge_keccak_1600_absorb (state, &curr_offset, salt, CROSS_SALT_SIZE, CROSS_SHAKE_RATE);
      uint16_t dom_id = (2 * CROSS_T + i) | (1 << 15);
      sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
      sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
      curr_offset = 0;
      sponge_keccak_1600_squeeze (state, &curr_offset, cmt1 + i * CROSS_HASH_DIGEST_SIZE, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    }
  }

  /* digest_cmt0 <- TreeRoot(cmt0) */
  uint8_t digest_cmt0_layer2[4 * CROSS_HASH_DIGEST_SIZE];
  uint8_t digest_cmt0[CROSS_HASH_DIGEST_SIZE];

  {
    uint64_t state[25] = {0};
    uint32_t curr_offset = 0;
    sponge_keccak_1600_absorb (state, &curr_offset, cmt0, leaf_offset1 * CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    uint16_t dom_id = 1 << 15;
    sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
    sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, digest_cmt0_layer2, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
  }

  {
    uint64_t state[25] = {0};
    uint32_t curr_offset = 0;
    sponge_keccak_1600_absorb (state, &curr_offset, cmt0 + leaf_offset1 * CROSS_HASH_DIGEST_SIZE, (leaf_offset2 - leaf_offset1) * CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    uint16_t dom_id = 1 << 15;
    sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
    sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, digest_cmt0_layer2 + CROSS_HASH_DIGEST_SIZE, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
  }

  {
    uint64_t state[25] = {0};
    uint32_t curr_offset = 0;
    sponge_keccak_1600_absorb (state, &curr_offset, cmt0 + leaf_offset2 * CROSS_HASH_DIGEST_SIZE, (leaf_offset3 - leaf_offset2) * CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    uint16_t dom_id = 1 << 15;
    sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
    sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, digest_cmt0_layer2 + 2 * CROSS_HASH_DIGEST_SIZE, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
  }

  {
    uint64_t state[25] = {0};
    uint32_t curr_offset = 0;
    sponge_keccak_1600_absorb (state, &curr_offset, cmt0 + leaf_offset3 * CROSS_HASH_DIGEST_SIZE, (CROSS_T - leaf_offset3) * CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    uint16_t dom_id = 1 << 15;
    sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
    sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, digest_cmt0_layer2 + 3 * CROSS_HASH_DIGEST_SIZE, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
  }

  {
    uint64_t state[25] = {0};
    uint32_t curr_offset = 0;
    sponge_keccak_1600_absorb (state, &curr_offset, digest_cmt0_layer2, 4 * CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    uint16_t dom_id = 1 << 15;
    sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
    sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, digest_cmt0, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
  }

  /* digest_cmt1 <- Hash(cmt1) */
  uint8_t digest_cmt1[CROSS_HASH_DIGEST_SIZE];

  {
    uint64_t state[25] = {0};
    uint32_t curr_offset = 0;
    sponge_keccak_1600_absorb (state, &curr_offset, cmt1, CROSS_T * CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    uint16_t dom_id = 1 << 15;
    sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
    sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, digest_cmt1, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
  }

  /* digest_cmt <- Hash(digest_cmt0 || digest_cmt1) */
  {
    uint64_t state[25] = {0};
    uint32_t curr_offset = 0;
    sponge_keccak_1600_absorb (state, &curr_offset, digest_cmt0, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    sponge_keccak_1600_absorb (state, &curr_offset, digest_cmt1, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    uint16_t dom_id = 1 << 15;
    sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
    sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, sig_out->digest_cmt, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
  }

  /* digest_msg <- Hash(Msg) */
  uint8_t digest_msg[CROSS_HASH_DIGEST_SIZE];

  {
    uint64_t state[25] = {0};
    uint32_t curr_offset = 0;
    sponge_keccak_1600_absorb (state, &curr_offset, msg, msg_len, CROSS_SHAKE_RATE);
    uint16_t dom_id = 1 << 15;
    sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
    sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, digest_msg, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
  }

  /* digest_chall1 <- Hash(digest_msg || digest_cmt || Salt) */
  uint8_t digest_chall1[CROSS_HASH_DIGEST_SIZE];

  {
    uint64_t state[25] = {0};
    uint32_t curr_offset = 0;
    sponge_keccak_1600_absorb (state, &curr_offset, digest_msg, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    sponge_keccak_1600_absorb (state, &curr_offset, sig_out->digest_cmt, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    sponge_keccak_1600_absorb (state, &curr_offset, salt, CROSS_SALT_SIZE, CROSS_SHAKE_RATE);
    uint16_t dom_id = 1 << 15;
    sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
    sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, digest_chall1, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
  }

  /* chall1 <- CSPRNG_F_p^t(digest_chall1 || t + c) */
  uint16_t chall1[CROSS_T];
  cross_csprng_vec_pt (digest_chall1, CROSS_HASH_DIGEST_SIZE, NULL, 0, 3 * CROSS_T - 1, chall1);

  /* y[i] <- u_prime[i] + chall1[i] * e_prime[i]  */
  uint8_t y_packed[CROSS_T * CROSS_PACKED_RESP_Y_SIZE];

  for (uint32_t i = 0; i < CROSS_T; ++i) {
    uint16_t y[CROSS_N];

    for (uint32_t j = 0; j < CROSS_N; ++j) {
      y[j] = fp_add (u_prime[i * CROSS_N + j], fp_mult (e_prime[i * CROSS_N + j], chall1[i]));
    }

    pack_resp_y (y, y_packed + i * CROSS_PACKED_RESP_Y_SIZE);
  }

  /* digest_chall2 <- Hash(y || digest_chall1) */
  {
    uint64_t state[25] = {0};
    uint32_t curr_offset = 0;
    sponge_keccak_1600_absorb (state, &curr_offset, y_packed, CROSS_T * CROSS_PACKED_RESP_Y_SIZE, CROSS_SHAKE_RATE);
    sponge_keccak_1600_absorb (state, &curr_offset, digest_chall1, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    uint16_t dom_id = 1 << 15;
    sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
    sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, sig_out->digest_chall2, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
  }

  /* chall2 <- CSPRNG_hamming_ball(digest_chall2 || t + c + 1) */
  uint8_t chall2[(CROSS_T - 1) / 8 + 1];
  cross_csprng_hamming_ball (sig_out->digest_chall2, CROSS_HASH_DIGEST_SIZE, NULL, 0, 3 * CROSS_T, chall2);

  /* Proof <- list of cmt0 values with chall2[i] == 1 */
  /* Path <- list of seed values with chall2[i] == 1 */

  /* for each i with chall2[i] == 0 */
  /* resp_0 <- (y[i], v_bar_g[i]) */
  /* resp_1 <- cmt1[i] */

  /* chall2 can be recomputed from the signature.
     Therefore, branching on chall2 is not side-channel.
   */
  uint32_t proof_ptr = 0, resp_ptr = 0;

  for (uint32_t i = 0; i < CROSS_T; ++i) {
    if (chall2[i / 8] & (1 << (i % 8))) {
      memcpy (sig_out->proof + proof_ptr * CROSS_HASH_DIGEST_SIZE, cmt0 + i * CROSS_HASH_DIGEST_SIZE, CROSS_HASH_DIGEST_SIZE);
      memcpy (sig_out->path + proof_ptr * CROSS_SEED_SIZE, seed_leaves + i * CROSS_SEED_SIZE, CROSS_SEED_SIZE);
      proof_ptr++;
    } else {
      memcpy (sig_out->resp_0[resp_ptr].y, y_packed + i * CROSS_PACKED_RESP_Y_SIZE, CROSS_PACKED_RESP_Y_SIZE);
      memcpy (sig_out->resp_0[resp_ptr].v, v_bar_g_packed + i * CROSS_PACKED_RESP_V_SIZE, CROSS_PACKED_RESP_V_SIZE);
      memcpy (sig_out->resp_1[resp_ptr], cmt1 + i * CROSS_HASH_DIGEST_SIZE, CROSS_HASH_DIGEST_SIZE);
      resp_ptr++;
    }
  }
}

void cross_rsdpg_1_fast_sign (const struct cross_sk_t * sk, const unsigned char * msg, size_t msg_len, struct cross_sig_t * sig_out) {
  uint8_t seed[CROSS_SEED_SIZE];
  uint8_t salt[CROSS_SALT_SIZE];

  getrandom (seed, CROSS_SEED_SIZE, 0);
  getrandom (salt, CROSS_SALT_SIZE, 0);

  cross_rsdpg_1_fast_sign_with_seed_salt (sk, seed, salt, msg, msg_len, sig_out);
}
