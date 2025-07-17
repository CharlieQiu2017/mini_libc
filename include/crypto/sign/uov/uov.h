#ifndef UOV_H
#define UOV_H

#include <stdint.h>
#include <stddef.h>

/* Public key of UOV has 278432 bytes, consisting of 3 * m matrices called
   P^(1), P^(2), and P^(3), each being a family of m matrices.

   P^(1) is upper triangular and has size v * v.
   Thus it takes m * v * (v + 1) / 2 bytes.

   P^(2) is rectangular and has size v * m.
   Thus it takes m * v * m bytes.

   P^(3) is upper triangular and has size m * m.
   Thus it takes m * m * m bytes.

   Since v + m = n, together they take m * n * (n + 1) / 2 bytes.

   See genkey.c for encoding convention of these matrices.

   Secret key of UOV has 237896 bytes, consisting of a secret seed,
   a matrix called O, and 2 * m matrices called P^(1) and S, each being a family of m matrices.

   The secret seed takes 32 bytes.

   P^(1) is the same as above.

   S is rectangular and has size v * m, and takes m * v * m bytes.

   O is rectangular and has size v * m, and takes v * m bytes.

   Signature of UOV has 128 bytes, consisting of a vector (n = 112 bytes) and a salt (16 bytes).
 */

#ifdef __cplusplus
extern "C" {
#endif

void uov_gen_key (uint8_t * p1_out, uint8_t * p2_out, uint8_t * p3_out, uint8_t * seed_out, uint8_t * o_out, uint8_t * s_out);

uint8_t uov_sign (const uint8_t * seed_sk, const uint8_t * o, const uint8_t * p1, const uint8_t * s, const uint8_t * msg, size_t len, const uint8_t * salt, uint8_t * out);

uint8_t uov_verify (const uint8_t * p1, const uint8_t * p2, const uint8_t * p3, const uint8_t * msg, size_t len, const uint8_t * salt, const uint8_t * sig);

#ifdef __cplusplus
}
#endif

#endif
