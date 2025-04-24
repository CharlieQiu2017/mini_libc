#include <stdint.h>

#define ROL(a, offset) (((a) << (offset)) ^ ((a) >> (64 - (offset))))

void farfalle_kravatte_roll_c (uint64_t * st) {
  uint64_t x0 = st[20];
  uint64_t x1 = st[21];

  st[20] = st[21];
  st[21] = st[22];
  st[22] = st[23];
  st[23] = st[24];
  st[24] = ROL (x0, 7) ^ x1 ^ (x1 >> 3);
}

void farfalle_kravatte_roll_e (uint64_t * st) {
  uint64_t x0 = st[20];
  uint64_t x1 = st[21];
  uint64_t x2 = st[22];

  st[20] = st[21];
  st[21] = st[22];
  st[22] = st[23];
  st[23] = st[24];
  st[24] = st[15];
  st[15] = st[16];
  st[16] = st[17];
  st[17] = st[18];
  st[18] = st[19];
  st[19] = ROL (x0, 7) ^ ROL (x1, 18) ^ (x2 & (x1 >> 1));
}
