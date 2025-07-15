/* AES-GCM */

#include <stdint.h>
#include <string.h>
#include <arm_neon.h>
#include <crypto/sk/aes/aes.h>

/* AES-GCM has a confusing byte-order and bit-order convention.
   The counter block is interpreted as a big-endian integer.
   Thus to increment the counter, we first perform byte-swap on the last 4 bytes,
   increase it by 1, and perform a byte-swap again.

   The Galois addition and multiplication interprets a block as a degree-127 polynomial.
   The byte-order is little-endian (byte 0 contains x^0 through x^7),
   but within each byte the highest bit represents the lowest term.
   This is the "normal" representation, but it's difficult to use.

   Therefore, we use the "bit-reversed" representation, where the bits within each byte are reversed.
   Thus to add a block to GHASH, we have to reverse the bits in each byte of that block.
 */

/* We assume the IV is always 96-bits (12 bytes) */

static inline void gcm_inc_ctr (uint32_t * ctr) {
  ctr[3] = __builtin_bswap32 (ctr[3]);
  ctr[3]++;
  ctr[3] = __builtin_bswap32 (ctr[3]);
}

/* GCM Multiplication
   See https://conradoplg.modp.net/files/2010/12/gcm14.pdf for explanation.
 */

static inline uint32x4_t gcm_mult (uint32x4_t ghash, uint32x4_t h) {
  /* The operands */
  uint32x4_t a = ghash, b = h;

  /* A zeroed register z */
  uint32x4_t z = vdupq_n_u32 (0);

  /* Output and clobber registers */
  uint32x4_t r0, r1, t0, t1;

  asm ("pmull %[r0].1q, %[a].1d, %[b].1d\n\t"
       "pmull2 %[r1].1q, %[a].2d, %[b].2d\n\t"
       "ext %[t0].16b, %[b].16b, %[b].16b, #8\n\t"
       "pmull %[t1].1q, %[a].1d, %[t0].1d\n\t"
       "pmull2 %[t0].1q, %[a].2d, %[t0].2d\n\t"
       "eor %[t0].16b, %[t0].16b, %[t1].16b\n\t"
       "ext %[t1].16b, %[z].16b, %[t0].16b, #8\n\t"
       "eor %[r0].16b, %[r0].16b, %[t1].16b\n\t"
       "ext %[t1].16b, %[t0].16b, %[z].16b, #8\n\t"
       "eor %[r1].16b, %[r1].16b, %[t1].16b\n\t"
  : [r0] "=&w" (r0), [r1] "=&w" (r1), [t0] "=&w" (t0), [t1] "=&w" (t1)
  : [a] "w" (a), [b] "w" (b), [z] "w" (z)
  : );

  /* At this point, the lower 128 bits are stored in r0, and the higher 128 bits are stored in r1 */

  /* We need a register p with the constant 0x00000000000000870000000000000087 */
  uint32x4_t p = vreinterpretq_u32_u64 (vdupq_n_u64 (0x87));
  z = vdupq_n_u32 (0);

  asm ("pmull2 %[t0].1q, %[r1].2d, %[p].2d\n\t"
       "ext %[t1].16b, %[t0].16b, %[z].16b, #8\n\t"
       "eor %[r1].16b, %[r1].16b, %[t1].16b\n\t"
       "ext %[t1].16b, %[z].16b, %[t0].16b, #8\n\t"
       "eor %[r0].16b, %[r0].16b, %[t1].16b\n\t"
       "pmull %[t0].1q, %[r1].1d, %[p].1d\n\t"
       "eor %[a].16b, %[r0].16b, %[t0].16b\n\t"
  : [a] "=&w" (a), [r0] "+&w" (r0), [r1] "+&w" (r1), [t0] "=&w" (t0), [t1] "=&w" (t1)
  : [p] "w" (p), [z] "w" (z)
  : );

  return a;
}

void aes128_encrypt_gcm (const unsigned char * exkey, const unsigned char * iv, const unsigned char * add_data, size_t add_data_len, const unsigned char * data, size_t data_len, unsigned char * ct_out, unsigned char * tag_out) {
  /* Compute the H value */
  uint32_t h_block[4] = {0};
  uint32_t h[4];
  aes128_encrypt_one_block (exkey, (unsigned char *) h_block, (unsigned char *) h);

  /* Reverse the bits of H */
  uint32x4_t h_rev = vld1q_u32 (h);
  h_rev = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (h_rev)));

  /* Prepare J0 block */
  uint32_t j0[4];
  memcpy (j0, iv, 12);
  j0[3] = 1u << 24;

  /* Encrypt J0, we need this later */
  uint32_t j0_enc[4];
  aes128_encrypt_one_block (exkey, (unsigned char *) j0, (unsigned char *) j0_enc);

  /* From this point on we no longer need the value of J0.
     We reuse it as the counter.
   */

  /* Length in bits */
  size_t add_data_len_orig = __builtin_bswap64 (add_data_len * 8), data_len_orig = __builtin_bswap64 (data_len * 8);

  /* We shall always assume ghash is in bit-reversed representation. */
  uint32x4_t ghash = vdupq_n_u32 (0);

  /* Process whole blocks of additional data */
  while (add_data_len >= 16) {
    /* Load a complete block, reverse its bits, and add to ghash */
    uint32x4_t add_data_block = vreinterpretq_u32_u8 (vld1q_u8 ((const uint8_t *) add_data));
    add_data_block = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (add_data_block)));
    ghash = veorq_u32 (ghash, add_data_block);

    /* Multiply by H */
    ghash = gcm_mult (ghash, h_rev);

    add_data_len -= 16; add_data += 16;
  }

  /* Final bytes of additional data */
  if (add_data_len > 0) {
    uint8_t final_add_data[16] = {0};
    memcpy (final_add_data, add_data, add_data_len);

    uint32x4_t add_data_block = vreinterpretq_u32_u8 (vld1q_u8 (final_add_data));
    add_data_block = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (add_data_block)));
    ghash = veorq_u32 (ghash, add_data_block);
    ghash = gcm_mult (ghash, h_rev);
  }

  /* Encrypt whole blocks of data */
  while (data_len >= 16) {
    /* Increment the counter */
    gcm_inc_ctr (j0);

    /* Encrypt the counter to get the data block */
    uint32_t ctr_enc[4];
    aes128_encrypt_one_block (exkey, (unsigned char *) j0, (unsigned char *) ctr_enc);

    /* XOR encrypted counter and data */
    uint32x4_t ctr_enc_block = vld1q_u32 (ctr_enc);
    uint32x4_t data_block = vreinterpretq_u32_u8 (vld1q_u8 (data));
    ctr_enc_block = veorq_u32 (ctr_enc_block, data_block);
    vst1q_u8 (ct_out, vreinterpretq_u8_u32 (ctr_enc_block));
    ct_out += 16;

    /* Add to ghash and multiply by H */
    ctr_enc_block = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (ctr_enc_block)));
    ghash = veorq_u32 (ghash, ctr_enc_block);
    ghash = gcm_mult (ghash, h_rev);

    data_len -= 16; data += 16;
  }

  /* Final bytes of data */
  if (data_len > 0) {
    gcm_inc_ctr (j0);

    uint8_t final_data[16] = {0};
    memcpy (final_data, data, data_len);

    uint32_t ctr_enc[4];
    aes128_encrypt_one_block (exkey, (unsigned char *) j0, (unsigned char *) ctr_enc);

    /* We cannot simply call veorq here, because we have to truncate the final ciphertext block */
    memxor (final_data, ctr_enc, data_len);
    memcpy (ct_out, final_data, data_len);

    uint32x4_t ctr_enc_block = vreinterpretq_u32_u8 (vld1q_u8 (final_data));
    ctr_enc_block = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (ctr_enc_block)));
    ghash = veorq_u32 (ghash, ctr_enc_block);
    ghash = gcm_mult (ghash, h_rev);
  }

  /* Final block: len(A) || len(C) */
  uint64x2_t final_block = vdupq_n_u64 (add_data_len_orig);
  final_block = vsetq_lane_u64 (data_len_orig, final_block, 1);
  uint32x4_t final_block_u32 = vreinterpretq_u32_u64 (final_block);

  final_block_u32 = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (final_block_u32)));
  ghash = veorq_u32 (ghash, final_block_u32);
  ghash = gcm_mult (ghash, h_rev);

  /* Reverse bits of ghash */
  ghash = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (ghash)));

  /* XOR with j0_enc */
  uint32x4_t j0_enc_block = vld1q_u32 (j0_enc);
  ghash = veorq_u32 (ghash, j0_enc_block);

  vst1q_u8 (tag_out, vreinterpretq_u8_u32 (ghash));

  return;
}
