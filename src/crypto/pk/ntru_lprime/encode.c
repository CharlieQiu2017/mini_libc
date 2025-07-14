#include <stdint.h>
#include <crypto/common.h>
#include <crypto/pk/ntru_lprime/ntru_lprime.h>

#define NTRU_LPR_Q 4621

/* The following code is NOT constant-time because it uses division by constant.
   However, insofar as they are only used to handle public keys, it should not be a problem.

   poly is in [0, Q - 1] representation.
   Each coefficient under the [-Q/2, Q/2] representation should be a multiple of 3.
 */

void ntrulpr_653_encode_poly_round (const uint16_t * poly, unsigned char * enc_out) {
  uint32_t buf[653];

  /* Convert to byte-string representation, and divide every entry by 3 */
  for (uint32_t i = 0; i < 653; ++i) {
    buf[i] = uint32_cmp_ge_branch (poly[i], (NTRU_LPR_Q + 1) / 2, poly[i] - (NTRU_LPR_Q + 1) / 2, poly[i] + (NTRU_LPR_Q - 1) / 2);
    buf[i] = buf[i] / 3;
  }

  unsigned char * ptr = enc_out;
  uint32_t r_final;

  /* 1541 * 653 -> 9277 * 326 + 1541 */
  for (uint32_t i = 0; i < 326; ++i) {
    uint32_t r = buf[i * 2] + buf[i * 2 + 1] * 1541;
    *ptr = r & 0xff; ptr++;
    buf[i] = r >> 8;
  }

  buf[326] = buf[652];

  /* 9277 * 326 + 1541 -> 1314 * 163 + 1541 */
  for (uint32_t i = 0; i < 163; ++i) {
    uint32_t r = buf[i * 2] + buf[i * 2 + 1] * 9277;
    *ptr = r & 0xff; ptr++; r >>= 8;
    *ptr = r & 0xff; ptr++;
    buf[i] = r >> 8;
  }

  buf[163] = buf[326];

  /* 1314 * 163 + 1541 -> 6745 * 81 + 7910 */
  for (uint32_t i = 0; i < 81; ++i) {
    uint32_t r = buf[i * 2] + buf[i * 2 + 1] * 1314;
    *ptr = r & 0xff; ptr++;
    buf[i] = r >> 8;
  }

  r_final = buf[162] + buf[163] * 1314;
  *ptr = r_final & 0xff; ptr++;
  buf[81] = r_final >> 8;

  /* 6745 * 81 + 7910 -> 695 * 40 + 815 */
  for (uint32_t i = 0; i < 40; ++i) {
    uint32_t r = buf[i * 2] + buf[i * 2 + 1] * 6745;
    *ptr = r & 0xff; ptr++; r >>= 8;
    *ptr = r & 0xff; ptr++;
    buf[i] = r >> 8;
  }

  r_final = buf[80] + buf[81] * 6745;
  *ptr = r_final & 0xff; ptr++; r_final >>= 8;
  *ptr = r_final & 0xff; ptr++;
  buf[40] = r_final >> 8;

  /* 695 * 40 + 815 -> 1887 * 20 + 815 */
  for (uint32_t i = 0; i < 20; ++i) {
    uint32_t r = buf[i * 2] + buf[i * 2 + 1] * 695;
    *ptr = r & 0xff; ptr++;
    buf[i] = r >> 8;
  }

  buf[20] = buf[40];

  /* 1887 * 20 + 815 -> 13910 * 10 + 815 */
  for (uint32_t i = 0; i < 10; ++i) {
    uint32_t r = buf[i * 2] + buf[i * 2 + 1] * 1887;
    *ptr = r & 0xff; ptr++;
    buf[i] = r >> 8;
  }

  buf[10] = buf[20];

  /* 13910 * 10 + 815 -> 2953 * 5 + 815 */
  for (uint32_t i = 0; i < 5; ++i) {
    uint32_t r = buf[i * 2] + buf[i * 2 + 1] * 13910;
    *ptr = r & 0xff; ptr++; r >>= 8;
    *ptr = r & 0xff; ptr++;
    buf[i] = r >> 8;
  }

  buf[5] = buf[10];

  /* 2953 * 5 + 815 -> 134 * 2 + 9402 */
  r_final = buf[0] + buf[1] * 2953;
  *ptr = r_final & 0xff; ptr++; r_final >>= 8;
  *ptr = r_final & 0xff; ptr++;
  buf[0] = r_final >> 8;

  r_final = buf[2] + buf[3] * 2953;
  *ptr = r_final & 0xff; ptr++; r_final >>= 8;
  *ptr = r_final & 0xff; ptr++;
  buf[1] = r_final >> 8;

  r_final = buf[4] + buf[5] * 2953;
  *ptr = r_final & 0xff; ptr++;
  buf[2] = r_final >> 8;

  /* 134 * 2 + 9402 -> 71 * 1 + 9402 */
  r_final = buf[0] + buf[1] * 134;
  *ptr = r_final & 0xff; ptr++; r_final >>= 8;

  /* Final step */
  r_final += buf[2] * 71;
  *ptr = r_final & 0xff; ptr++; r_final >>= 8;
  *ptr = r_final & 0xff; ptr++; r_final >>= 8;
  *ptr = r_final & 0xff;

  return;
}

void ntrulpr_653_decode_poly_round (const unsigned char * enc, uint16_t * poly) {
  uint32_t buf[653];

  uint32_t r_final;

  /* Encoding length is 865 bytes */
  const unsigned char * ptr = enc + 864;

  /* Final step of encoding */
  r_final = *ptr; ptr--;
  r_final = (r_final << 8) + *ptr; ptr--;
  r_final = (r_final << 8) + *ptr; ptr--;

  buf[2] = r_final / 71;
  r_final = r_final % 71;

  /* 71 * 1 + 9402 -> 134 * 2 + 9402 */
  r_final = (r_final << 8) + *ptr; ptr--;
  buf[1] = r_final / 134;
  buf[0] = r_final % 134;

  /* 134 * 2 + 9402 -> 2953 * 5 + 815 */
  r_final = buf[2];
  r_final = (r_final << 8) + *ptr; ptr--;
  buf[5] = r_final / 2953;
  buf[4] = r_final % 2953;

  r_final = buf[1];
  r_final = (r_final << 8) + *ptr; ptr--;
  r_final = (r_final << 8) + *ptr; ptr--;
  buf[3] = r_final / 2953;
  buf[2] = r_final % 2953;

  r_final = buf[0];
  r_final = (r_final << 8) + *ptr; ptr--;
  r_final = (r_final << 8) + *ptr; ptr--;
  buf[1] = r_final / 2953;
  buf[0] = r_final % 2953;

  /* 2953 * 5 + 815 -> 13910 * 10 + 815 */
  buf[10] = buf[5];

  for (uint32_t i = 4; i < 5; --i) {
    uint32_t r = buf[i];
    r = (r << 8) + *ptr; ptr--;
    r = (r << 8) + *ptr; ptr--;
    buf[i * 2 + 1] = r / 13910;
    buf[i * 2] = r % 13910;
  }

  /* 13910 * 10 + 815 -> 1887 * 20 + 815 */
  buf[20] = buf[10];

  for (uint32_t i = 9; i < 10; --i) {
    uint32_t r = buf[i];
    r = (r << 8) + *ptr; ptr--;
    buf[i * 2 + 1] = r / 1887;
    buf[i * 2] = r % 1887;
  }

  /* 1887 * 20 + 815 -> 695 * 40 + 815 */
  buf[40] = buf[20];

  for (uint32_t i = 19; i < 20; --i) {
    uint32_t r = buf[i];
    r = (r << 8) + *ptr; ptr--;
    buf[i * 2 + 1] = r / 695;
    buf[i * 2] = r % 695;
  }

  /* 695 * 40 + 815 -> 6745 * 81 + 7910 */
  r_final = buf[40];
  r_final = (r_final << 8) + *ptr; ptr--;
  r_final = (r_final << 8) + *ptr; ptr--;
  buf[81] = r_final / 6745;
  buf[80] = r_final % 6745;

  for (uint32_t i = 39; i < 40; --i) {
    uint32_t r = buf[i];
    r = (r << 8) + *ptr; ptr--;
    r = (r << 8) + *ptr; ptr--;
    buf[i * 2 + 1] = r / 6745;
    buf[i * 2] = r % 6745;
  }

  /* 6745 * 81 + 7910 -> 1314 * 163 + 1541 */
  r_final = buf[81];
  r_final = (r_final << 8) + *ptr; ptr--;
  buf[163] = r_final / 1314;
  buf[162] = r_final % 1314;

  for (uint32_t i = 80; i < 81; --i) {
    uint32_t r = buf[i];
    r = (r << 8) + *ptr; ptr--;
    buf[i * 2 + 1] = r / 1314;
    buf[i * 2] = r % 1314;
  }

  /* 1314 * 163 + 1541 -> 9277 * 326 + 1541 */
  buf[326] = buf[163];

  for (uint32_t i = 162; i < 163; --i) {
    uint32_t r = buf[i];
    r = (r << 8) + *ptr; ptr--;
    r = (r << 8) + *ptr; ptr--;
    buf[i * 2 + 1] = r / 9277;
    buf[i * 2] = r % 9277;
  }

  /* 9277 * 326 + 1541 -> 1541 * 653 */
  buf[652] = buf[326];

  for (uint32_t i = 325; i < 326; --i) {
    uint32_t r = buf[i];
    r = (r << 8) + *ptr; ptr--;
    buf[i * 2 + 1] = r / 1541;
    buf[i * 2] = r % 1541;
  }

  /* Multiply each entry by 3, convert to [0, Q-1] representation */
  for (uint32_t i = 0; i < 653; ++i) {
    buf[i] = buf[i] * 3;
    poly[i] = uint32_cmp_ge_branch (buf[i], (NTRU_LPR_Q - 1) / 2, buf[i] - (NTRU_LPR_Q - 1) / 2, buf[i] + (NTRU_LPR_Q + 1) / 2);
  }

  return;
}
