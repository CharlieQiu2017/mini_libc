#include <stdint.h>
#include <crypto/sign/uov/gf256.h>

/* We follow the data layout in the UOV signature specification.

   The matrix O (dimension v * m) is stored in column-major order, i.e.
   O_{i, j} = O[v * j + i].

   Families of matrices P^(2) and S are stored as
   M[i]_{j, k} = M[(j * row_len + k) * family_len + i].

   Families of matrices P^(1) and P^(3) are upper triangular.

   Layout for P^(1) and P^(3): If j <= k,
   M[i]_{j, k} = M[((2 * row_len - j + 1) * j / 2 + (k - j)) * family_len + i]
   If j > k then
   M[i]_{j, k} = 0

   family_len is always m.
   Dimension of P^(1) is v * v.
   Dimension of P^(2) is v * m.
   Dimension of P^(3) is m * m.
   Dimension of S is v * m.
 */

#define UOV_N 112
#define UOV_M 44
#define UOV_V 68

#define O_ENTRY(o,i,j) (o[UOV_V * (j) + (i)])
#define P1_ENTRY(p1,i,j,k) (p1[((2 * UOV_V - (j) + 1) * (j) / 2 + ((k) - (j))) * UOV_M + (i)])
#define P2_ENTRY(p2,i,j,k) (p2[((j) * UOV_M + (k)) * UOV_M + (i)])
#define P3_ENTRY(p3,i,j,k) (p3[((2 * UOV_M - (j) + 1) * (j) / 2 + ((k) - (j))) * UOV_M + (i)])
#define S_ENTRY(s,i,j,k) (s[((j) * UOV_M + (k)) * UOV_M + (i)])

void compute_p3 (const uint8_t * o, const uint8_t * p1, const uint8_t * p2, uint8_t * p3_out) {
  uint8_t op1[UOV_M * UOV_V];
  uint8_t op2[UOV_M * UOV_M];
  uint8_t opo[UOV_M * UOV_M];

  for (uint32_t i = 0; i < UOV_M; ++i) {
    /* Compute O^T * P^(1) */
    /* Iterate through each row of O^T (in fact, each column of O) */
    for (uint32_t j = 0; j < UOV_M; ++j) {
      /* Iterate through each column of P^(1) */
      for (uint32_t k = 0; k < UOV_V; ++k) {
	op1[k * UOV_M + j] = 0;
	/* Since P^(1) is upper triangular, only the first (k+1) entries are needed */
	for (uint32_t l = 0; l <= k; ++l) {
	  /* Multiply O_{l, j} and P^(1)[i]_{l, k} */
	  op1[k * UOV_M + j] ^= gf256_mul (O_ENTRY (o, l, j), P1_ENTRY (p1, i, l, k));
	}
      }
    }

    /* Compute O^T * P^(1) * O */
    /* Iterate through each row of O^T * P^(1) */
    for (uint32_t j = 0; j < UOV_M; ++j) {
      /* Iterate through each column of O */
      for (uint32_t k = 0; k < UOV_M; ++k) {
	opo[k * UOV_M + j] = 0;

	for (uint32_t l = 0; l < UOV_V; ++l) {
	  /* Multiply (O^T * P^(1))_{j, l} and O_{l, k} */
	  opo[k * UOV_M + j] ^= gf256_mul (op1[l * UOV_M + j], O_ENTRY (o, l, k));
	}
      }
    }

    /* Compute O^T * P^(2) */
    /* Iterate through each row of O^T (in fact, each column of O) */
    for (uint32_t j = 0; j < UOV_M; ++j) {
      /* Iterate through each column of P^(2) */
      for (uint32_t k = 0; k < UOV_M; ++k) {
	op2[k * UOV_M + j] = 0;
	for (uint32_t l = 0; l < UOV_V; ++l) {
	  /* Multiply O_{l, j} and P^(2)[i]_{l, k} */
	  op2[k * UOV_M + j] ^= gf256_mul (O_ENTRY (o, l, j), P2_ENTRY (p2, i, l, k));
	}
      }
    }

    /* P^(3) = Upper (-O^T * P^(1) * O - O^T * P^(2)) */
    for (uint32_t j = 0; j < UOV_M; ++j) {
      P3_ENTRY (p3_out, i, j, j) = opo[j * UOV_M + j] ^ op2[j * UOV_M + j];
      for (uint32_t k = j + 1; k < UOV_M; ++k) {
	P3_ENTRY (p3_out, i, j, k) = opo[j * UOV_M + k] ^ op2[j * UOV_M + k] ^ opo[k * UOV_M + j] ^ op2[k * UOV_M + j];
      }
    }
  }
}

void compute_s (const uint8_t * o, const uint8_t * p1, const uint8_t * p2, uint8_t * s_out) {
  for (uint32_t i = 0; i < UOV_M; ++i) {
    /* Iterate through the rows of P^(1) */
    for (uint32_t j = 0; j < UOV_V; ++j) {
      /* Iterate through the columns of O */
      for (uint32_t k = 0; k < UOV_M; ++k) {
	S_ENTRY (s_out, i, j, k) = 0;
	for (uint32_t l = 0; l < UOV_V; ++l) {
	  uint8_t t = P1_ENTRY (p1, i, j, l) ^ P1_ENTRY (p1, i, l, j);
	  S_ENTRY (s_out, i, j, k) ^= gf256_mul (t, O_ENTRY (o, l, k));
	}
        S_ENTRY (s_out, i, j, k) ^= P2_ENTRY (p2, i, j, k);
      }
    }
  }
}

#undef UOV_N
#undef UOV_M
#undef UOV_V
