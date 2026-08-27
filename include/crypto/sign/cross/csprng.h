#ifndef CROSS_CSPRNG_H
#define CROSS_CSPRNG_H

#include <stdint.h>
#include <crypto/sign/cross/parameters.h>

/* The input to CSPRNG always consists of three parts: a seed, a salt (optional), and a domain-separation number.
   We assume the caller will zeroize state.
 */
void cross_csprng_prepare (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint64_t * state);

/* Sample one bit-string of length a * LAMBDA / 8 */
void cross_csprng_string (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint32_t a, unsigned char * out);

/* Sample two bit-strings of length a * LAMBDA / 8 */
void cross_csprng_string_2 (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint32_t a, unsigned char * out1, unsigned char * out2);

/* The specification of CROSS does not make clear how it stores matrices (row-major or column-major order).
   The reference implementation: https://github.com/CROSS-signature/CROSS-implementation/blob/main/Reference_Implementation/include/csprng_hash.h
   According to the reference implementation, F_z^(m * (n - m)) is stored in row-major order,
   but F_p^((n - k) * k) is stored in column-major order.
   One may thus interpret every matrix of dimension (n - k) * k in the spec as being transposed, and the actual dimension is k * (n - k).

   To generate a vector or a matrix over F_p from a seed,
   first call SHAKE128 on the seed to get an arbitrarily long bit-string.
   The first byte corresponds to bit 0-7, the second byte corresponds to bit 8-15, etc.
   Extract the first k bits and check if it corresponds to a number less than p.
   If so, the number is accepted. Otherwise, it is dropped, and we extract another k bits.

   For implement CROSS-R-SDP(G), p is either 127 or 509.
   Hence the probability of dropping each sample is at most 0.8% or 1/125.
   If we take 20 samples, the probability of rejecting all of them is at most 2^-139.
   Hence if we fail to generate a number after taking 20 samples, we simply return 0.
   This ensures the amount of output we need from SHAKE128 is truly bounded.
   The probability of our implementation being incompatible with the reference implementation should be negligible.
 */

/* Sample random vector F_z^m */
void cross_csprng_vec_zm (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint8_t * out);

/* Sample two random vectors F_z^m and F_p^n */
void cross_csprng_vec_zm_pn (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint8_t * out_z, uint16_t * out_p);

/* Sample two random matrices F_z^(m * (n - m)) and F_p^(k * (n - k)) */
void cross_csprng_mat_zp (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint8_t * out_z, uint16_t * out_p);

/* Sample t elements in F_p. */
/* The spec document says elements of this vector must be non-zero.
   However, we have contacted the CROSS team and confirmed that allowing zero-elements here does not affect security.
   In fact, it makes the scheme slightly more secure.
   Therefore, we drop the non-zero requirement.
   This is deliberately incompatible with reference implementation.
 */
void cross_csprng_vec_pt (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, uint16_t * out);

/* Generate a bit-string of t bits, of which exactly w bits are 1.
   The suggestion of the specification is to use Fisher-Yates shuffle.
   We adopt a much simpler approach used in NTRU-LPrime:
   We generate T random 32-bit numbers, set the LSB of the first W numbers, and clear the LSB of the rest, then sort the numbers.
   This is deliberately incompatible with reference implementation.
 */
void cross_csprng_hamming_ball (const unsigned char * seed, size_t seed_len, const unsigned char * salt, size_t salt_len, uint16_t dom_id, unsigned char * out);

/* Calls to Hash are more complex and we simply let the callers use the low-level Keccak functions.
   In the spec document, each Hash call may or may not be appended with a domain separation number.
   If a domain separation number is not specified, treat it as if there is a separation number equal to 0.
   Then the spec requires that the domain separation number have its highest-bit set.
   Outputs of Hash calls are CROSS_HASH_DIGEST_SIZE == LAMBDA / 4 bytes.
 */

#endif
