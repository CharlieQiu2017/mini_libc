/* gf256.h
   Finite field operations for UOV signature.
   Partially adapted from pqov src/gf16.h
 */

#ifndef UOV_GF256_H
#define UOV_GF256_H

#include <stdint.h>
#include <arm_acle.h>
#include <arm_neon.h>
#include <crypto/common.h>

/* We work in the field F_{2^8} = F_2[x] / (x^8 + x^4 + x^3 + x + 1) */
#define UOV_FIELD_REP 0x1b
/* The quotient of x^15 reduced by (x^8 + x^4 + x^3 + x + 1) */
#define UOV_FIELD_QUOT 0x8d

/* Multiply two polynomials without reducing the result */
static inline uint16_t gf256_mul_no_mod (uint8_t a, uint8_t b) {
  poly8x8_t a_v = vcreate_p8 ((uint64_t) a);
  poly8x8_t b_v = vcreate_p8 ((uint64_t) b);
  poly16x8_t prod = vmull_p8 (a_v, b_v);
  return vgetq_lane_u16 (vreinterpretq_u16_p16 (prod), 0);
}

/* Reduce a polynomial X by (x^8 + x^4 + x^3 + x + 1)
   For an explanation of this algorithm see
   https://cdrdv2-public.intel.com/836172/clmul-wp-rev-2-02-2014-04-20.pdf

   We first write X = x^8 * A + B where deg A, B <= 7.
   A is called the "high part" and B is called the "low part".
   We want to compute A * x^8 mod g(x) where g(x) is (x^8 + x^4 + x^3 + x + 1).

   Suppose that A * x^8 = q(x) * g(x) + p(x), then the coefficients of x^0, ..., x^7 in q(x) * g(x) and p(x) must be equal.
   Therefore p(x) is simply the last 8 terms of q(x) * g(x).

   If A = a7 x^7 + a6 x^6 + ... + a0,
   let u_k = quotient (x^(8 + k), g(x))
   then q(x) = a7 * u_7 + ... + a0 * u_0.

   Now it can be easily seen that for each 0 <= k < 7 there exists constant b_k such that u_{k + 1} = u_k * x + b_k.
   Proof: First observe that deg u_k = k.
   If x^(8 + k) = g(x) * u_k + r_k(x) with deg r_k(x) < 8,
   then x^(8 + k + 1) = g(x) * x * u_k + x * r_k(x).
   If deg x * r_k(x) < 8 then u_{k + 1} = u_k * x and we can take b_k = 0.
   Otherwise, deg x * r_k(x) = 8 and b_k is the coefficient of x^8 in x * r_k(x).

   For the particular field used by UOV,
   we have quotient (x^15, g(x)) = x^7 + x^3 + x^2 + 1.

   Now we can compute q(x) in the following way:
   Multiply A by (x^7 + x^3 + x^2 + 1), and discard the lower 7 terms.
 */
static inline uint8_t gf256_reduce (uint16_t x) {
  uint16_t low = x & 0xff;
  uint16_t high = x >> 8;
  uint16_t high_quot = gf256_mul_no_mod (high, UOV_FIELD_QUOT) >> 7;
  uint16_t high_rem = gf256_mul_no_mod (high_quot, UOV_FIELD_REP) & 0xff;
  return low ^ high_rem;
}

static inline uint8_t gf256_mul (uint16_t a, uint16_t b) {
  return gf256_reduce (gf256_mul_no_mod (a, b));
}

static inline uint8_t gf256_inv (uint8_t x) {
  /* For an explanation of this algorithm, see
     Daniel J. Bernstein & Bo-Yin Yang,
     Fast constant-time gcd computation and modular inversion.

     See also https://zhuanlan.zhihu.com/p/593503857 (in Chinese)
   */

  /* Degree of f(z) is exactly 8 */
  /* Degree of g(z) is at most 7 */
  uint32_t f = 0x100 + UOV_FIELD_REP;
  uint32_t g = x;
  f = __rbit (f) >> 23;
  g = __rbit (g) >> 24;

  /* The invariants of B&Y algorithm are :
     1. Constant term of f is non-zero;
     2. If g_i = 0 then gcd(f, g) = f_i, otherwise gcd(f, g) = gcd(f_i, g_i);
     3. deg f_i <= m_i, deg g_i <= n_i;
     4. m_i + n_i = m + n - i;
     5. f * r_i + g * s_i = f_i, f * u_i + g * v_i = g_i;
     6. z^(i - 1) * r_i, z^(i - 1) * s_i, z^i * u_i, z^i * v_i are polynomials;
     7. z^i * (r_i * v_i - s_i * u_i) is a polynomial with non-zero constant term;
     Define delta_i = m_i - n_i, then:
     8. 2 deg z^(i - 1) * r_i <= i + delta_i - (2 + delta_0);
     9. 2 deg z^(i - 1) * s_i <= i + delta_i - (2 - delta_0);
     10. 2 deg z^i * u_i <= i - delta_i - delta_0;
     11. 2 deg z^i * v_i <= i - delta_i + delta_0.
   */

  /* By invariant 6, we will represent r_i, s_i, u_i, v_i by the polynomials z^i * r_i, etc.
     Initially r = v = 1, s = u = 0.
   */
  int32_t m = 8, n = 7;
  uint32_t r = 1, s = 0, u = 0, v = 1;

  for (uint32_t i = 1; i <= 15; ++i) {
    uint32_t g_tail = uint32_value_barrier (g & 1);
    uint32_t flag = int32_cmp_ge (n, m) | (1 - g_tail);

    uint32_t new_f_1 = f;
    uint32_t new_g_1 = ((f * g_tail) ^ g) >> 1;
    int32_t new_m_1 = m;
    int32_t new_n_1 = n - 1;
    uint32_t new_r_1 = r << 1;
    uint32_t new_s_1 = s << 1;
    uint32_t new_u_1 = u ^ (r * g_tail);
    uint32_t new_v_1 = v ^ (s * g_tail);

    uint32_t new_f_2 = g;
    uint32_t new_g_2 = (f ^ g) >> 1;
    int32_t new_m_2 = n;
    int32_t new_n_2 = m - 1;
    uint32_t new_r_2 = u << 1;
    uint32_t new_s_2 = v << 1;
    uint32_t new_u_2 = u ^ r;
    uint32_t new_v_2 = v ^ s;

    f = new_f_1 * flag + new_f_2 * (1 - flag);
    g = new_g_1 * flag + new_g_2 * (1 - flag);
    m = new_m_1 * flag + new_m_2 * (1 - flag);
    n = new_n_1 * flag + new_n_2 * (1 - flag);
    r = new_r_1 * flag + new_r_2 * (1 - flag);
    s = new_s_1 * flag + new_s_2 * (1 - flag);
    u = new_u_1 * flag + new_u_2 * (1 - flag);
    v = new_v_1 * flag + new_v_2 * (1 - flag);

    /* The above code simulates the following code in constant time.
    if (flag) {
      g = ((f * g_tail) ^ g) >> 1;
      n = n - 1;
      u = u ^ (r * g_tail);
      v = v ^ (s * g_tail);
      r = r << 1;
      s = s << 1;
    } else {
      uint32_t old_f = f;
      f = g;
      g = (old_f ^ g) >> 1;
      int8_t old_m = m;
      m = n;
      n = old_m - 1;
      uint32_t old_u = u;
      u = u ^ r;
      r = old_u << 1;
      uint32_t old_v = v;
      v = v ^ s;
      s = old_v << 1;
    }
    */
  }

  /* The inverse of g is z^(-n - n_{m+n}) * s_{m+n}(1/z) */
  /* s is currently z^(m+n) * s_{m+n} */
  /* If we reverse the bits of s we get z^(31 - m - n) * s_{m+n}(1/z) */
  /* We right shift it by (31 - m + n_{m+n}) */
  s = __rbit (s) >> (31 - 8 + n);
  return s;
}

#endif
