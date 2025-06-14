#include <stdint.h>
#include <string.h>
#include <crypto/common.h>
#include <crypto/sign/uov/gf256.h>
#include <crypto/sign/uov/uov.h>
#include <crypto/hash/keccak/keccak_p.h>

/* pk_seed_len is 16 bytes
   sk_seed_len is 32 bytes
   salt_len is 16 bytes
 */

#define UOV_N 112
#define UOV_M 44
#define UOV_V 68

#define O_ENTRY(o,i,j) (o[UOV_V * (j) + (i)])
#define P1_ENTRY(p1,i,j,k) (p1[((2 * UOV_V - (j) + 1) * (j) / 2 + ((k) - (j))) * UOV_M + (i)])
#define P2_ENTRY(p2,i,j,k) (p2[((j) * UOV_M + (k)) * UOV_M + (i)])
#define P3_ENTRY(p3,i,j,k) (p3[((2 * UOV_M - (j) + 1) * (j) / 2 + ((k) - (j))) * UOV_M + (i)])
#define S_ENTRY(s,i,j,k) (s[((j) * UOV_M + (k)) * UOV_M + (i)])

uint8_t uov_sign (const uint8_t * seed_sk, const uint8_t * o, const uint8_t * p1, const uint8_t * s, const uint8_t * msg, size_t len, const uint8_t * salt, uint8_t * out) {
  uint8_t t[UOV_M];
  uint8_t v[UOV_V];
  uint8_t y[UOV_M];
  /* L is in row-major order */
  uint8_t l[UOV_M * UOV_M];
#define L_ENTRY(l,i,j) (l[(i) * UOV_M + (j)])
  uint8_t ctr = 0;

  uint64_t hash_st[25] = {0}, hash_st_copy[25];
  uint32_t curr_offset = 0, curr_offset_copy;
  sponge_keccak_1600_absorb (hash_st, &curr_offset, msg, len, 136);
  sponge_keccak_1600_absorb (hash_st, &curr_offset, salt, 16, 136);

  /* Make a copy of hash state, so that when computing v
     we do not need to hash again the msg */
  memcpy (hash_st_copy, hash_st, 200);
  curr_offset_copy = curr_offset;

  sponge_keccak_1600_finalize (hash_st, curr_offset, 15 + 16, 136);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (hash_st, &curr_offset, t, UOV_M, 136);

  /* In the following loop, each iteration computes a new value of v.
     The only thing that changes in the computation of v is ctr.
     Therefore, hash seed_sk now.
   */
  sponge_keccak_1600_absorb (hash_st_copy, &curr_offset_copy, seed_sk, 32, 136);

  do {
    /* Step 1: compute v */
    memcpy (hash_st, hash_st_copy, 200);
    curr_offset = curr_offset_copy;
    sponge_keccak_1600_absorb (hash_st, &curr_offset, &ctr, 1, 136);
    sponge_keccak_1600_finalize (hash_st, curr_offset, 15 + 16, 136);
    curr_offset = 0;
    sponge_keccak_1600_squeeze (hash_st, &curr_offset, v, UOV_V, 136);

    /* Step 2: compute t - y = t - v^T * P^(1) * v */
    for (uint32_t i = 0; i < UOV_M; ++i) {
      y[i] = t[i];
      for (uint32_t j = 0; j < UOV_V; ++j) {
	for (uint32_t k = j; k < UOV_V; ++k) {
	  y[i] ^= gf256_mul (gf256_mul (v[j], v[k]), P1_ENTRY (p1, i, j, k));
	}
      }
    }

    /* Step 3: compute L */
    for (uint32_t i = 0; i < UOV_M; ++i) {
      for (uint32_t j = 0; j < UOV_M; ++j) {
	L_ENTRY (l, i, j) = 0;
	for (uint32_t k = 0; k < UOV_V; ++k) {
	  L_ENTRY (l, i, j) ^= gf256_mul (v[k], S_ENTRY (s, i, k, j));
	}
      }
    }

    /* Step 4: simultaneously invert L and compute L^-1 * (t - y) */
    uint8_t inv_succ_flag = 1;

    for (uint32_t i = 0; i < UOV_M; ++i) {

      /* Step 4.1: find a pivot row */
      uint8_t flag = 1 - uint8_to_bool (L_ENTRY (l, i, i));

      for (uint32_t j = i + 1; j < UOV_M; ++j) {
	/* If flag == 1, add L[j][k] to L[i][k].
	   Only the part with k >= i needs to be added,
	   since at this point all entries with j < i should be zero, except the diagonals.
	 */
	for (uint32_t k = i; k < UOV_M; ++k) {
	  L_ENTRY (l, i, k) ^= flag * L_ENTRY (l, j, k);
	}

	/* Also add entries in (t - y) */
	y[i] ^= flag * y[j];

	/* Update flag */
	flag = 1 - uint8_to_bool (L_ENTRY (l, i, i));
      }

      /* If flag == 1 at end of loop, matrix is not invertible */
      inv_succ_flag *= (1 - flag);

      /* Step 4.2: invert pivot row */
      uint8_t inv = gf256_inv (L_ENTRY (l, i, i));
      L_ENTRY (l, i, i) = 1;

      for (uint32_t j = i + 1; j < UOV_M; ++j) {
	L_ENTRY (l, i, j) = gf256_mul (L_ENTRY (l, i, j), inv);
      }

      y[i] = gf256_mul (y[i], inv);

      /* Step 4.3: clear column i */
      for (uint32_t j = 0; j < UOV_M; ++j) {
	if (j == i) continue;

	uint8_t coeff = L_ENTRY (l, j, i);
	L_ENTRY (l, j, i) = 0;

	for (uint32_t k = i + 1; k < UOV_M; ++k) {
	  L_ENTRY (l, j, k) ^= gf256_mul (L_ENTRY (l, i, k), coeff);
	}

	y[j] ^= gf256_mul (y[i], coeff);
      }

    }

    /* If L is not invertible, enter next iteration.
       This does not constitute a side-channel,
       because the number of iterations needed for signing
       is leaked to the adversary anyway.
     */
    if (inv_succ_flag == 0) { ++ctr; continue; }

    /* At this point, array y stores x = L^-1 * (t - y) */

    /* Step 5: compute s = [v 0]^T + [O I]^T * x */
    for (uint32_t i = 0; i < UOV_V; ++i) {
      out[i] = v[i];

      for (uint32_t j = 0; j < UOV_M; ++j) {
	out[i] ^= gf256_mul (O_ENTRY (o, i, j), y[j]);
      }
    }

    for (uint32_t i = UOV_V; i < UOV_N; ++i) {
      out[i] = y[i - UOV_V];
    }

    return 0;

  } while (ctr != 0);

  /* Signing failure. Probability of occurring should be negligible. */
  return 1;
}

uint8_t uov_verify (const uint8_t * p1, const uint8_t * p2, const uint8_t * p3, const uint8_t * msg, size_t len, const uint8_t * salt, const uint8_t * sig) {
  uint8_t t[UOV_M];
  uint8_t y[UOV_M];

  uint64_t hash_st[25] = {0};
  uint32_t curr_offset = 0;
  sponge_keccak_1600_absorb (hash_st, &curr_offset, msg, len, 136);
  sponge_keccak_1600_absorb (hash_st, &curr_offset, salt, 16, 136);
  sponge_keccak_1600_finalize (hash_st, curr_offset, 15 + 16, 136);
  curr_offset = 0;
  sponge_keccak_1600_squeeze (hash_st, &curr_offset, t, UOV_M, 136);

  for (uint32_t i = 0; i < UOV_M; ++i) {
    y[i] = 0;

    for (uint32_t j = 0; j < UOV_V; ++j) {
      for (uint32_t k = j; k < UOV_V; ++k) {
	y[i] ^= gf256_mul (gf256_mul (sig[j], sig[k]), P1_ENTRY (p1, i, j, k));
      }
    }

    for (uint32_t j = 0; j < UOV_V; ++j) {
      for (uint32_t k = UOV_V; k < UOV_N; ++k) {
	y[i] ^= gf256_mul (gf256_mul (sig[j], sig[k]), P2_ENTRY (p2, i, j, k - UOV_V));
      }
    }

    for (uint32_t j = UOV_V; j < UOV_N; ++j) {
      for (uint32_t k = j; k < UOV_N; ++k) {
	y[i] ^= gf256_mul (gf256_mul (sig[j], sig[k]), P3_ENTRY (p3, i, j - UOV_V, k - UOV_V));
      }
    }
  }

  return (memcmp (t, y, UOV_M) == 0);
}

#undef UOV_N
#undef UOV_M
#undef UOV_V
