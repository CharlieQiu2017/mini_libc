/* AES-GCM */

#include <stdint.h>
#include <string.h>
#include <arm_neon.h>
#include <crypto/sk/aes/aes_neon.h>

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

static inline uint32x4_t gcm_inc_ctr (uint32x4_t ctr) {
  uint32_t last = vgetq_lane_u32 (ctr, 3);
  last = __builtin_bswap32 (last);
  last++;
  last = __builtin_bswap32 (last);
  return vsetq_lane_u32 (last, ctr, 3);
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
  uint32x4_t h = vdupq_n_u32 (0);
  h = vreinterpretq_u32_u8 (aes128_encrypt_one_block_neon (exkey, vreinterpretq_u8_u32 (h)));

  /* Reverse the bits of H */
  h = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (h)));

  /* Prepare J0 block */
  uint32_t j0_mem[4];
  memcpy (j0_mem, iv, 12);
  j0_mem[3] = 1u << 24;
  uint32x4_t j0 = vld1q_u32 (j0_mem);

  /* Encrypt J0, we need this later */
  uint32x4_t j0_enc = vreinterpretq_u32_u8 (aes128_encrypt_one_block_neon (exkey, vreinterpretq_u8_u32 (j0)));

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
    ghash = gcm_mult (ghash, h);

    add_data_len -= 16; add_data += 16;
  }

  /* Final bytes of additional data */
  if (add_data_len > 0) {
    uint8_t final_add_data_mem[16] = {0};
    memcpy (final_add_data_mem, add_data, add_data_len);

    uint32x4_t add_data_block = vreinterpretq_u32_u8 (vld1q_u8 (final_add_data_mem));
    add_data_block = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (add_data_block)));
    ghash = veorq_u32 (ghash, add_data_block);
    ghash = gcm_mult (ghash, h);
  }

  /* Encrypt whole blocks of data */
  while (data_len >= 16) {
    /* Increment the counter */
    j0 = gcm_inc_ctr (j0);

    /* Encrypt the counter to get the data block */
    uint32x4_t ctr_enc_block = vreinterpretq_u32_u8 (aes128_encrypt_one_block_neon (exkey, vreinterpretq_u8_u32 (j0)));

    /* XOR encrypted counter and data */
    uint32x4_t data_block = vreinterpretq_u32_u8 (vld1q_u8 ((const uint8_t *) data));
    ctr_enc_block = veorq_u32 (ctr_enc_block, data_block);
    vst1q_u8 (ct_out, vreinterpretq_u8_u32 (ctr_enc_block));
    ct_out += 16;

    /* Add to ghash and multiply by H */
    ctr_enc_block = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (ctr_enc_block)));
    ghash = veorq_u32 (ghash, ctr_enc_block);
    ghash = gcm_mult (ghash, h);

    data_len -= 16; data += 16;
  }

  /* Final bytes of data */
  if (data_len > 0) {
    j0 = gcm_inc_ctr (j0);

    uint32x4_t ctr_enc_block = vreinterpretq_u32_u8 (aes128_encrypt_one_block_neon (exkey, vreinterpretq_u8_u32 (j0)));

    uint8_t final_data_mem[16] = {0};
    memcpy (final_data_mem, data, data_len);

    uint32x4_t data_block = vreinterpretq_u32_u8 (vld1q_u8 (final_data_mem));
    ctr_enc_block = veorq_u32 (ctr_enc_block, data_block);

    /* We have to clear the last (16 - data_len) bytes of ctr_enc_block */
    uint64x2_t mask = vdupq_n_u64 (0);
    if (data_len < 8) {
      mask = vsetq_lane_u64 ((1ull << (8 * data_len)) - 1, mask, 0);
    } else {
      mask = vsetq_lane_u64 (~0ull, mask, 0);
      mask = vsetq_lane_u64 ((1ull << (8 * (data_len - 8))) - 1, mask, 1);
    }

    ctr_enc_block = vandq_u32 (ctr_enc_block, vreinterpretq_u32_u64 (mask));

    /* Write first to final_data_mem, then memcpy, since ct_out might not have sufficient space */
    vst1q_u8 (final_data_mem, vreinterpretq_u8_u32 (ctr_enc_block));
    memcpy (ct_out, final_data_mem, data_len);

    /* Add to ghash and multiply by H */
    ctr_enc_block = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (ctr_enc_block)));
    ghash = veorq_u32 (ghash, ctr_enc_block);
    ghash = gcm_mult (ghash, h);
  }

  /* Final block: len(A) || len(C) */
  uint64x2_t final_block = vdupq_n_u64 (add_data_len_orig);
  final_block = vsetq_lane_u64 (data_len_orig, final_block, 1);
  uint32x4_t final_block_u32 = vreinterpretq_u32_u64 (final_block);

  final_block_u32 = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (final_block_u32)));
  ghash = veorq_u32 (ghash, final_block_u32);
  ghash = gcm_mult (ghash, h);

  /* Reverse bits of ghash */
  ghash = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (ghash)));

  /* XOR with j0_enc */
  ghash = veorq_u32 (ghash, j0_enc);

  vst1q_u8 (tag_out, vreinterpretq_u8_u32 (ghash));

  return;
}

void aes128_decrypt_gcm (const unsigned char * exkey, const unsigned char * iv, const unsigned char * add_data, size_t add_data_len, const unsigned char * ct, size_t ct_len, unsigned char * data_out, unsigned char * tag_out) {
  /* Compute the H value */
  uint32x4_t h = vdupq_n_u32 (0);
  h = vreinterpretq_u32_u8 (aes128_encrypt_one_block_neon (exkey, vreinterpretq_u8_u32 (h)));

  /* Reverse the bits of H */
  h = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (h)));

  /* Prepare J0 block */
  uint32_t j0_mem[4];
  memcpy (j0_mem, iv, 12);
  j0_mem[3] = 1u << 24;
  uint32x4_t j0 = vld1q_u32 (j0_mem);

  /* Encrypt J0 */
  uint32x4_t j0_enc = vreinterpretq_u32_u8 (aes128_encrypt_one_block_neon (exkey, vreinterpretq_u8_u32 (j0)));

  /* Length in bits */
  size_t add_data_len_orig = __builtin_bswap64 (add_data_len * 8), data_len_orig = __builtin_bswap64 (ct_len * 8);

  uint32x4_t ghash = vdupq_n_u32 (0);

  /* Process whole blocks of additional data */
  while (add_data_len >= 16) {
    /* Load a complete block, reverse its bits, and add to ghash */
    uint32x4_t add_data_block = vreinterpretq_u32_u8 (vld1q_u8 ((const uint8_t *) add_data));
    add_data_block = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (add_data_block)));
    ghash = veorq_u32 (ghash, add_data_block);

    /* Multiply by H */
    ghash = gcm_mult (ghash, h);

    add_data_len -= 16; add_data += 16;
  }

  /* Final bytes of additional data */
  if (add_data_len > 0) {
    uint8_t final_add_data_mem[16] = {0};
    memcpy (final_add_data_mem, add_data, add_data_len);

    uint32x4_t add_data_block = vreinterpretq_u32_u8 (vld1q_u8 (final_add_data_mem));
    add_data_block = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (add_data_block)));
    ghash = veorq_u32 (ghash, add_data_block);
    ghash = gcm_mult (ghash, h);
  }

  /* Decrypt whole blocks of data */
  while (ct_len >= 16) {
    /* Add ct to ghash first */
    uint32x4_t ct_block = vreinterpretq_u32_u8 (vld1q_u8 (ct));
    uint32x4_t ct_block_rev = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (ct_block)));
    ghash = veorq_u32 (ghash, ct_block_rev);
    ghash = gcm_mult (ghash, h);

    /* Increment the counter */
    j0 = gcm_inc_ctr (j0);

    /* Encrypt the counter */
    uint32x4_t ctr_enc_block = vreinterpretq_u32_u8 (aes128_encrypt_one_block_neon (exkey, vreinterpretq_u8_u32 (j0)));

    /* XOR encrypted counter and ct to get plaintext */
    ctr_enc_block = veorq_u32 (ctr_enc_block, ct_block);
    vst1q_u8 (data_out, vreinterpretq_u8_u32 (ctr_enc_block));
    data_out += 16;

    ct_len -= 16; ct += 16;
  }

  /* Final bytes of data */
  if (ct_len > 0) {
    uint8_t final_ct_mem[16] = {0};
    memcpy (final_ct_mem, ct, ct_len);

    uint32x4_t ct_block = vreinterpretq_u32_u8 (vld1q_u8 (final_ct_mem));
    uint32x4_t ct_block_rev = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (ct_block)));
    ghash = veorq_u32 (ghash, ct_block_rev);
    ghash = gcm_mult (ghash, h);

    j0 = gcm_inc_ctr (j0);

    uint32x4_t ctr_enc_block = vreinterpretq_u32_u8 (aes128_encrypt_one_block_neon (exkey, vreinterpretq_u8_u32 (j0)));
    ctr_enc_block = veorq_u32 (ctr_enc_block, ct_block);

    vst1q_u8 (final_ct_mem, vreinterpretq_u8_u32 (ctr_enc_block));
    memcpy (data_out, final_ct_mem, ct_len);
  }

  /* Final block: len(A) || len(C) */
  uint64x2_t final_block = vdupq_n_u64 (add_data_len_orig);
  final_block = vsetq_lane_u64 (data_len_orig, final_block, 1);
  uint32x4_t final_block_u32 = vreinterpretq_u32_u64 (final_block);

  final_block_u32 = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (final_block_u32)));
  ghash = veorq_u32 (ghash, final_block_u32);
  ghash = gcm_mult (ghash, h);

  /* Reverse bits of ghash */
  ghash = vreinterpretq_u32_u8 (vrbitq_u8 (vreinterpretq_u8_u32 (ghash)));

  /* XOR with j0_enc */
  ghash = veorq_u32 (ghash, j0_enc);

  vst1q_u8 (tag_out, vreinterpretq_u8_u32 (ghash));

  return;
}
