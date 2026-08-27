#ifndef CROSS_PARAMETERS_H
#define CROSS_PARAMETERS_H

#include <stdint.h>

#define CROSS_P 509
#define CROSS_LOG_P 9
#define CROSS_Z 127
#define CROSS_LOG_Z 7

/* Barrett reduction constants */
/* See src/crypto/pk/ntru_prime/utility.c */
#define CROSS_P_C1 131845
#define CROSS_P_C2 26
#define CROSS_Z_C1 8257
#define CROSS_Z_C2 20

/* The generator g for the subgroup of multiplicative order 127 and its powers */
#define CROSS_G 16
#define CROSS_G_2 256
#define CROSS_G_4 384
#define CROSS_G_8 355
#define CROSS_G_16 302
#define CROSS_G_32 93
#define CROSS_G_64 505

#define CROSS_LAMBDA 128
#define CROSS_SHAKE_RATE 168
#define CROSS_N 55
#define CROSS_K 36
#define CROSS_M 25
#define CROSS_T 147
#define CROSS_W 76

/* Number of attempts to generate each F_p or F_z element */
#define CROSS_GEN_ATTEMPT 20

#define CROSS_SEED_SIZE (CROSS_LAMBDA / 8)
#define CROSS_HASH_DIGEST_SIZE (CROSS_LAMBDA / 4)
#define CROSS_KEYPAIR_SEED_SIZE (CROSS_LAMBDA / 4)
#define CROSS_SALT_SIZE (CROSS_LAMBDA / 4)

/* When vectors of F_p and F_z are serialized, they are stored in a packed format.
   Three kinds of vectors need to be serialized:
   1. Syndrome s in the public key, where each element takes CROSS_LOG_P bits;
   2. Vector v in the challenge response, where each element is represented by its discrete log, and takes CROSS_LOG_Z bits;
   3. Vector y in the challenge response, where each element takes CROSS_LOG_P bits.
 */
#define CROSS_PACKED_SYN_SIZE (((CROSS_N - CROSS_K) * CROSS_LOG_P - 1) / 8 + 1)
#define CROSS_PACKED_RESP_V_SIZE ((CROSS_M * CROSS_LOG_Z - 1) / 8 + 1)
#define CROSS_PACKED_RESP_Y_SIZE ((CROSS_N * CROSS_LOG_P - 1) / 8 + 1)

#endif
