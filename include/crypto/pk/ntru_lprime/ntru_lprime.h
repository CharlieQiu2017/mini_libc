/* NTRU-LPrime
   Specification: https://ntruprime.cr.yp.to/nist/ntruprime-20201007.pdf
   We replace SHA-512 used in the specification with SHA-3.
 */

#ifndef NTRU_LPR_H
#define NTRU_LPR_H

#include <stdint.h>

/* Public key of NTRU LPrime consists of two parts:
   1. Seed S for G (32 bytes)
   2. round(aG) = aG + e (865 bytes, see below)

   Secret key of NTRU LPrime consists of four parts:
   1. The short polynomial a (164 bytes)
   2. The public key (897 bytes)
   3. A random sequence of bytes rho (32 bytes)
   4. Key-hash cache (Hash_4 (public key), see p. 20 of spec, 32 bytes)
   The third part is optional and used in implicit rejection.

   Ciphertext of NTRU LPrime consists of three parts:
   1. round(bG) (865 bytes)
   2. T array (256 elements each taking 4 bits, hence 128 bytes)
   3. Hash confirmation (32 bytes)
 */

/* In the specification document, the coefficient of each polynomial is between -(Q-1)/2 and (Q-1)/2.
   We call this the [-Q/2, Q/2] representation.
   When a polynomial in [-Q/2, Q/2] representation needs to be encoded into a byte string,
   we add Q/2 to each coefficient to convert it into a number between 0 and Q-1.
   We call this the byte-string representation.

   However, negative numbers can be tricky to deal with.
   Therefore, in our implementation we represent each coefficient with a number between 0 and Q-1.
   We call this the [0, Q-1] representation. 
   It is NOT the same thing as byte-string representation.
   To encode a polynomial in [0, Q-1] representation into byte-string,
   we need to convert the numbers as follows:
   If 0 <= k <= (Q-1)/2, add (Q-1)/2 to the number.
   If (Q+1)/2 <= k < Q, subtract (Q+1)/2 from the number.
 */

/* NTRU LPrime uses a rather strange way to encode polynomials.
   This only applies to public keys.
   The secret key is a "short" polynomial and is encoded using 2 bits for each coefficient.

   The specification document defines both encoding regular polynomials and "rounded polynomials."
   NTRU LPrime only uses the rounded version.
   In NTRU LPrime 653, a polynomial has 653 terms, each in 0, 1, ..., 1540.
   The encoding begins by converting the polynomial into an array of coefficients r_0, ..., r_652, where r_i is the coefficient of term x^i.

   Now we use U * k + U' to represent an array of length k+1, where the first k elements have an (exclusive) upper bound U,
   while the last element has an (exclusive) upper bound U'.
   If all elements have the same upper bound, we simply write U * k.
   Thus the initial array can be represented as 1541 * 653.

   The encoding is described as a recursive procedure.
   At each step, we group the array elements into pairs of two.
   If the length is odd, ignore the final element.
   For each pair, we multiply their upper bounds together.
   Let M be the product.
   If M < 16384, we replace the pair with a single number r_i + m_i * r_{i+1} where r_i is the array element, and m_i is its upper bound.
   The upper bound of the new element is M.
   If 16384 <= M < 4194304, we output (r_i + m_i * r_{i+1}) mod 256, and replace the pair with a single number (r_i + m_i * r_{i+1}) / 256.
   The upper bound of the new element is ceil(M / 256).
   If 4194304 <= M < 1073741824, we output (r_i + m_i * r_{i+1}) mod 65536 (in little-endian), and replace the pair with a single number (r_i + m_i * r_{i+1}) / 65536.
   The upper bound of the new element is ceil(M / 65536).

   Now 1541 * 1541 = 2374681 = 9276 * 256 + 25.
   We extract one byte from each pair of array elements (see spec document). We get: 326 bytes
   The remaining array can be represented as 9277 * 326 + 1541

   Now 9277 * 9277 = 86062729 = 336182 * 256 + 137, and 336183 = 1313 * 256 + 55.
   We extract two bytes from each pair of array elements. We get: 326 bytes
   The remaining array can be represented as 1314 * 163 + 1541

   Now 1314 * 1314 = 1726596 = 6744 * 256 + 132, and 1314 * 1541 = 2024874 = 7909 * 256 + 170.
   We extract one byte from each pair of array elements. We get: 82 bytes
   The remaining array can be represented as 6745 * 81 + 7910

   Now 6745 * 6745 = 177714 * 256 + 241, 177714 = 694 * 256 + 50, 6745 * 7910 = 208409 * 256 + 246, 208409 = 814 * 256 + 25.
   We extract two bytes from each pair of array elements. We get: 82 bytes
   The remaining array can be represented as 695 * 40 + 815

   Now 695 * 695 = 483025 = 1886 * 256 + 209.
   We extract one byte from each pair of array elements. We get: 20 bytes
   The remaining array can be represented as 1887 * 20 + 815

   Now 1887 * 1887 = 3560769 = 13909 * 256 + 65.
   We extract one byte from each pair of array elements. We get: 10 bytes
   The remaining array can be represented as 13910 * 10 + 815

   Now 13910 * 13910 = 193488100 = 755812 * 256 + 228, 755812 = 2952 * 256 + 100.
   We extract two bytes from each pair of array elements. We get: 10 bytes
   The remaining array can be represented as 2953 * 5 + 815

   Now 2953 * 2953 = 8720209 = 34063 * 256 + 81, 34063 = 133 * 256 + 15, 2953 * 815 = 9401 * 256 + 39.
   We extract two bytes from the first two pairs, and one byte from the last pair. We get: 5 bytes
   The remaining array can be represented as 134 * 2 + 9402

   Now 134 * 134 = 17956 = 70 * 256 + 36.
   We extract one byte from the only pair. We get: 1 byte
   The remaining array can be represented as 71 * 1 + 9402

   Let the remaining elements be R_0, R_1, then compute R_0 + R_1 * 71, and encode that number in little-endian.
   We get: 3 bytes

   Now 326 + 326 + 82 + 82 + 20 + 10 + 10 + 5 + 1 + 3 = 865 bytes.
   Together with the 32 bytes for the seed S, we get the total length of the public key which is 897 bytes.
 */

#ifdef __cplusplus
extern "C" {
#endif

void ntrulpr_653_encode_poly_round (const uint16_t * poly, unsigned char * enc_out);

void ntrulpr_653_decode_poly_round (const unsigned char * enc, uint16_t * poly);

void ntrulpr_653_expand_seed (const unsigned char * seed, uint16_t * out_g);

void ntrulpr_653_poly_mult_short (const uint16_t * g, const uint8_t * a, uint32_t out_terms, uint16_t * out);

void ntrulpr_653_round (const uint16_t * g, uint16_t * out);

void ntrulpr_653_safesort (uint32_t * poly);

void ntrulpr_653_hashshort (const unsigned char * input, uint8_t * out);

void ntrulpr_653_gen_key (unsigned char * sk_out, unsigned char * pk_out);

void ntrulpr_653_encapsulate_internal (const unsigned char * pk, const unsigned char * input, const unsigned char * key_hash_cache, unsigned char * ct_out);

void ntrulpr_653_encapsulate (const unsigned char * pk, unsigned char * ct_out, unsigned char * key_out);

void ntrulpr_653_decapsulate (const unsigned char * sk, const unsigned char * ct, unsigned char * key_out);

#ifdef __cplusplus
}
#endif

#endif
