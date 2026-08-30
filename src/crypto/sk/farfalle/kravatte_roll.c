#include <stdint.h>

#define ROL(a, offset) (((a) << (offset)) ^ ((a) >> (64 - (offset))))

/* Spec: The rolling function roll_c applies a linear transformation to the five lanes of the plane
   y = 4 of the Keccak-p state and leaves the other 20 lanes unchanged. If we denote the
   five lanes before the transformation by (x0, x1, x2, x3, x4) then the transformation maps
   them to (x1, x2, x3, x4, x5) with
   x5 = (x0 <<< 7) + x1 + (x1 >> 3) where <<< denotes a cyclic shift to the left and >> a shift to the right.
 */
void farfalle_kravatte_roll_c (uint64_t * st) {
  uint64_t x0 = st[20];
  uint64_t x1 = st[21];

  st[20] = st[21];
  st[21] = st[22];
  st[22] = st[23];
  st[23] = st[24];
  st[24] = ROL (x0, 7) ^ x1 ^ (x1 >> 3);
}

/* Spec: The rolling function roll_e applies a non-linear transformation to the ten lanes of the planes
   y = 4 and y = 3 of the Keccak-p state and leaves the other 15 lanes unchanged. If we
   denote the ten lanes before the transformation by (x0, x1, x2, x3, x4, x5, x6, x7, x8, x9) then
   the transformation maps them to (x1, x2, x3, x4, x5, x6, x7, x8, x9, x10) with
   x10 = (x0 <<< 7) + (x1 <<< 18) + x2 * (x1 >> 1).

   Note that the spec says "the planes of y = 4 and y = 3", instead of "the planes of y = 3 and y = 4."
   This suggests x0, x1, x2, x3, x4 should correspond to the rows in y = 4 rather than y = 3.
   However, later spec text and reference implementation contradicts this reading,
   and suggests x0, ..., x4 should be the rows in y = 3, while x5, ..., x9 are the rows in y = 4.
   As far as we can tell either reading has no impact on the security proof.
 */
void farfalle_kravatte_roll_e (uint64_t * st) {
  uint64_t x0 = st[15];
  uint64_t x1 = st[16];
  uint64_t x2 = st[17];

  st[15] = st[16];
  st[16] = st[17];
  st[17] = st[18];
  st[18] = st[19];
  st[19] = st[20];
  st[20] = st[21];
  st[21] = st[22];
  st[22] = st[23];
  st[23] = st[24];
  st[24] = ROL (x0, 7) ^ ROL (x1, 18) ^ (x2 & (x1 >> 1));
}
