#include <stdint.h>
#include <string.h>
#include <crypto/hash/keccak/keccak_p.h>
#include <crypto/sign/cross/cross.h>
#include <crypto/sign/cross/parameters.h>
#include <crypto/sign/cross/csprng.h>
#include <crypto/sign/cross/pack_unpack.h>
#include <crypto/sign/cross/fz_arith.h>
#include <crypto/sign/cross/fp_arith.h>

_Bool cross_rsdpg_1_fast_verify (const struct cross_pk_t * pk, const unsigned char * msg, size_t msg_len, const struct cross_sig_t * sig) {
  /* Retrieve syndrome */
  uint16_t syndrome[CROSS_N - CROSS_K];

  if (! unpack_syndrome (pk->s, syndrome)) return 0;

  /* Retrieve random matrices W and V */
  uint8_t mat_w[CROSS_M * (CROSS_N - CROSS_M)];
  uint16_t mat_v[CROSS_K * (CROSS_N - CROSS_K)];
  cross_csprng_mat_zp (pk->seed_pk, CROSS_KEYPAIR_SEED_SIZE, NULL, 0, 3 * CROSS_T + 2, mat_w, mat_v);

  /* Define H and M as in gen_key.c */

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
    sponge_keccak_1600_absorb (state, &curr_offset, sig->digest_cmt, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    sponge_keccak_1600_absorb (state, &curr_offset, sig->salt, CROSS_SALT_SIZE, CROSS_SHAKE_RATE);
    uint16_t dom_id = 1 << 15;
    sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
    sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, digest_chall1, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
  }

  /* chall1 <- CSPRNG_F_p^t(digest_chall1 || t + c with c == 2t - 1 */
  uint16_t chall1[CROSS_T];
  cross_csprng_vec_pt (digest_chall1, CROSS_HASH_DIGEST_SIZE, NULL, 0, 3 * CROSS_T - 1, chall1);

  /* chall2 <- CSPRNG_hamming_ball(digest_chall2 || t + c + 1) */
  uint8_t chall2[(CROSS_T - 1) / 8 + 1];
  cross_csprng_hamming_ball (sig->digest_chall2, CROSS_HASH_DIGEST_SIZE, NULL, 0, 3 * CROSS_T, chall2);

  /* For each round, recover cmt0, cmt1, y */

  uint32_t proof_ptr = 0, resp_ptr = 0;
  uint8_t cmt0[CROSS_T * CROSS_HASH_DIGEST_SIZE];
  uint8_t cmt1[CROSS_T * CROSS_HASH_DIGEST_SIZE];
  uint8_t y_packed[CROSS_T * CROSS_PACKED_RESP_Y_SIZE];

  for (uint32_t i = 0; i < CROSS_T; ++i) {
    if (chall2[i / 8] & (1 << (i % 8))) {

      /* cmt0[i] recorded in Proof */
      memcpy (cmt0 + i * CROSS_HASH_DIGEST_SIZE, sig->proof + proof_ptr * CROSS_HASH_DIGEST_SIZE, CROSS_HASH_DIGEST_SIZE);

      /* cmt1[i] <- Hash(Seed[i] || Salt || i + c) */
      /* The spec uses 1-indexed round numbers. See sign.c. */
      {
	uint64_t state[25] = {0};
	uint32_t curr_offset = 0;
	sponge_keccak_1600_absorb (state, &curr_offset, sig->path + proof_ptr * CROSS_SEED_SIZE, CROSS_SEED_SIZE, CROSS_SHAKE_RATE);
	sponge_keccak_1600_absorb (state, &curr_offset, sig->salt, CROSS_SALT_SIZE, CROSS_SHAKE_RATE);
	uint16_t dom_id = (2 * CROSS_T + i) | (1 << 15);
	sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
	sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
	curr_offset = 0;
	sponge_keccak_1600_squeeze (state, &curr_offset, cmt1 + i * CROSS_HASH_DIGEST_SIZE, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
      }

      /* e_bar_g_prime[i], u_prime[i] <- CSPRNG(Seed[i] || Salt || i + c) */
      uint8_t e_bar_g_prime[CROSS_M];
      uint16_t u_prime[CROSS_N];
      cross_csprng_vec_zm_pn (sig->path + proof_ptr * CROSS_SEED_SIZE, CROSS_SEED_SIZE, sig->salt, CROSS_SALT_SIZE, 2 * CROSS_T + i, e_bar_g_prime, u_prime);

      /* e_bar_prime[i] <- e_bar_g_prime[i] * M */
      uint8_t e_bar_prime[CROSS_N] = {0};

      for (uint32_t j = 0; j < CROSS_N - CROSS_M; ++j) {
	for (uint32_t k = 0; k < CROSS_M; ++k) {
	  e_bar_prime[j] = fz_add (e_bar_prime[j], fz_mult (e_bar_g_prime[k], mat_w[k * (CROSS_N - CROSS_M) + j]));
	}
      }

      for (uint32_t j = CROSS_N - CROSS_M; j < CROSS_N; ++j) {
	e_bar_prime[j] = e_bar_g_prime[j - (CROSS_N - CROSS_M)];
      }

      /* e_prime[i] <- g^e_bar_prime[i] */
      uint16_t e_prime[CROSS_N];

      for (uint32_t j = 0; j < CROSS_N; ++j) {
	e_prime[j] = fz_to_fp (e_bar_prime[j]);
      }

      /* y[i] <- u_prime[i] + chall1[i] * e_prime[i] */
      uint16_t y[CROSS_N];

      for (uint32_t j = 0; j < CROSS_N; ++j) {
	y[j] = fp_add (u_prime[j], fp_mult (e_prime[j], chall1[i]));
      }

      pack_resp_y (y, y_packed + i * CROSS_PACKED_RESP_Y_SIZE);

      proof_ptr++;

    } else {

      /* cmt1[i] <- resp_1[resp_ptr] */
      memcpy (cmt1 + i * CROSS_HASH_DIGEST_SIZE, sig->resp_1[resp_ptr], CROSS_HASH_DIGEST_SIZE);

      /* (y[i], v_bar_g[i]) <- resp_0[resp_ptr] */
      uint8_t v_bar_g[CROSS_M];
      uint16_t y[CROSS_N];

      _Bool padding_bound_check = 1;
      padding_bound_check &= unpack_resp_y (sig->resp_0[resp_ptr].y, y);
      padding_bound_check &= unpack_resp_v (sig->resp_0[resp_ptr].v, v_bar_g);
      if (! padding_bound_check) return 0;

      /* Record packed y */
      memcpy (y_packed + i * CROSS_PACKED_RESP_Y_SIZE, sig->resp_0[resp_ptr].y, CROSS_PACKED_RESP_Y_SIZE);

      /* v_bar[i] <- v_bar_g[i] * M */
      uint8_t v_bar[CROSS_N] = {0};

      for (uint32_t j = 0; j < CROSS_N - CROSS_M; ++j) {
	for (uint32_t k = 0; k < CROSS_M; ++k) {
	  v_bar[j] = fz_add (v_bar[j], fz_mult (v_bar_g[k], mat_w[k * (CROSS_N - CROSS_M) + j]));
	}
      }

      for (uint32_t j = CROSS_N - CROSS_M; j < CROSS_N; ++j) {
	v_bar[j] = v_bar_g[j - (CROSS_N - CROSS_M)];
      }

      /* v[i] <- g^v_bar[i] */
      uint16_t v[CROSS_N];

      for (uint32_t j = 0; j < CROSS_N; ++j) {
	v[j] = fz_to_fp (v_bar[j]);
      }

      /* y_prime[i] <- v[i] * y[i] */
      uint16_t y_prime[CROSS_N];

      for (uint32_t j = 0; j < CROSS_N; ++j) {
	y_prime[j] = fp_mult (v[j], y[j]);
      }

      /* syndrome_prime[i] <- y_prime[i] * H^T - chall1[i] * s */
      uint16_t syndrome_prime[CROSS_N - CROSS_K] = {0};

      for (uint32_t j = 0; j < CROSS_N - CROSS_K; ++j) {
	for (uint32_t k = 0; k < CROSS_K; ++k) {
	  syndrome_prime[j] = fp_add (syndrome_prime[j], fp_mult (y_prime[k], mat_v[k * (CROSS_N - CROSS_K) + j]));
	}
	syndrome_prime[j] = fp_add (syndrome_prime[j], y_prime[CROSS_K + j]);
	syndrome_prime[j] = fp_add (syndrome_prime[j], fp_neg (fp_mult (chall1[i], syndrome[j])));
      }

      /* cmt0[i] <- Hash(syndrome_prime[i] || v_bar_g[i] || Salt || i + c) */
      uint8_t syndrome_prime_packed[CROSS_PACKED_SYN_SIZE];

      pack_syndrome (syndrome_prime, syndrome_prime_packed);

      {
	uint64_t state[25] = {0};
	uint32_t curr_offset = 0;
	sponge_keccak_1600_absorb (state, &curr_offset, syndrome_prime_packed, CROSS_PACKED_SYN_SIZE, CROSS_SHAKE_RATE);
	sponge_keccak_1600_absorb (state, &curr_offset, sig->resp_0[resp_ptr].v, CROSS_PACKED_RESP_V_SIZE, CROSS_SHAKE_RATE);
	sponge_keccak_1600_absorb (state, &curr_offset, sig->salt, CROSS_SALT_SIZE, CROSS_SHAKE_RATE);
	uint16_t dom_id = (2 * CROSS_T + i) | (1 << 15);
	sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
	sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
	curr_offset = 0;
	sponge_keccak_1600_squeeze (state, &curr_offset, cmt0 + i * CROSS_HASH_DIGEST_SIZE, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
      }

      resp_ptr++;
    }
  }

  /* digest_cmt0 <- TreeRoot(cmt0) */
  const uint32_t leaf_offset1 = (CROSS_T - 1) / 4 + 1;
  const uint32_t leaf_offset2 = leaf_offset1 + (CROSS_T - 2) / 4 + 1;
  const uint32_t leaf_offset3 = leaf_offset2 + (CROSS_T - 3) / 4 + 1;

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

  /* digest_prime_cmt <- Hash(digest_cmt0 || digest_cmt1) */
  uint8_t digest_prime_cmt[CROSS_HASH_DIGEST_SIZE];

  {
    uint64_t state[25] = {0};
    uint32_t curr_offset = 0;
    sponge_keccak_1600_absorb (state, &curr_offset, digest_cmt0, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    sponge_keccak_1600_absorb (state, &curr_offset, digest_cmt1, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    uint16_t dom_id = 1 << 15;
    sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
    sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, digest_prime_cmt, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
  }

  /* digest_prime_chall2 <- Hash(y || digest_chall1) */
  uint8_t digest_prime_chall2[CROSS_HASH_DIGEST_SIZE];

  {
    uint64_t state[25] = {0};
    uint32_t curr_offset = 0;
    sponge_keccak_1600_absorb (state, &curr_offset, y_packed, CROSS_T * CROSS_PACKED_RESP_Y_SIZE, CROSS_SHAKE_RATE);
    sponge_keccak_1600_absorb (state, &curr_offset, digest_chall1, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
    uint16_t dom_id = 1 << 15;
    sponge_keccak_1600_absorb (state, &curr_offset, (unsigned char *) &dom_id, 2, CROSS_SHAKE_RATE);
    sponge_keccak_1600_finalize (state, curr_offset, 15 + 16, CROSS_SHAKE_RATE);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (state, &curr_offset, digest_prime_chall2, CROSS_HASH_DIGEST_SIZE, CROSS_SHAKE_RATE);
  }

  _Bool result = ! uint64_to_bool (safe_memcmp (sig->digest_cmt, digest_prime_cmt, CROSS_HASH_DIGEST_SIZE));
  result &= ! uint64_to_bool (safe_memcmp (sig->digest_chall2, digest_prime_chall2, CROSS_HASH_DIGEST_SIZE));
  return result;
}
