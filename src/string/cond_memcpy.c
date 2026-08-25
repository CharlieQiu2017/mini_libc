/* cond_memcpy.c
   Cryptographically-safe conditional memcpy.
   cond is either 0 or 1.
   If cond is 0, no action is performed.
   If cond is 1, dst is overwritten with src.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <string_internal.h>
#include <crypto/common.h>

void cond_memcpy (_Bool cond, void * restrict vd, const void * restrict vs, size_t n) {
  uint32_t cond_val = uint32_value_barrier (cond);
  uintptr_t s = (uintptr_t) vs;
  uintptr_t d = (uintptr_t) vd;
  const unsigned char * sp;
  unsigned char * dp;

  uint8_t mask_src = cond_val * 0xff;
  uint8_t mask_dst = (cond_val ^ 1) * 0xff;

  uint64_t mask_src_long = cond_val * ((uint64_t) -1);
  uint64_t mask_dst_long = (cond_val ^ 1) * ((uint64_t) -1);

  while (n && (s & 7)) {
    sp = const_ptr_from_uint (s);
    dp = ptr_from_uint (d);
    *dp = (*dp & mask_dst) | (*sp & mask_src);
    s++; d++; n--;
  }

  if (!n) return;

  uint64_t s_buf, s_buf2, s_buf3, d_buf;

  if ((d & 7) == 0) {
    while (n >= 8) {
      s_buf = read_u64 (s);
      d_buf = read_u64 (d);
      d_buf = (s_buf & mask_src_long) | (d_buf & mask_dst_long);
      write_u64 (d, d_buf);

      s += 8; d += 8; n -= 8;
    }

    if (!n) return;

    s_buf = read_u64 (s);
    d_buf = read_u64 (d);
    d_buf = (s_buf & mask_src_long) | (d_buf & mask_dst_long);

    while (n) {
      dp = ptr_from_uint (d);
      *dp = d_buf & 0xff;
      d_buf >>= 8; d++; n--;
    }

    return;
  }

  uint32_t d_off = d & 7;
  s_buf = read_u64 (s);
  d_buf = read_u64 (d - d_off);
  d_buf >>= 8 * d_off;
  d_buf = (s_buf & mask_src_long) | (d_buf & mask_dst_long);

  uint32_t i = 8 - d_off;
  while (i && n) { dp = ptr_from_uint (d); *dp = d_buf & 0xff; d_buf >>= 8; d++; i--; n--; }
  if (!n) return;
  s_buf >>= 8 * (8 - d_off);
  s += 8;

  while (n >= 8) {
    s_buf2 = read_u64 (s);
    d_buf = read_u64 (d);
    s_buf3 = s_buf | (s_buf2 << (8 * d_off));
    d_buf = (s_buf3 & mask_src_long) | (d_buf & mask_dst_long);
    write_u64 (d, d_buf);

    s_buf = s_buf2 >> 8 * (8 - d_off);
    s += 8; d += 8; n -= 8;
  }

  if (!n) return;

  if (n <= d_off) s_buf2 = 0; else s_buf2 = read_u64 (s);
  d_buf = read_u64 (d);
  s_buf3 = s_buf | (s_buf2 << (8 * d_off));
  d_buf = (s_buf3 & mask_src_long) | (d_buf & mask_dst_long);

  while (n) {
    dp = ptr_from_uint (d);
    *dp = d_buf & 0xff;
    d++; d_buf >>= 8; n--;
  }

  return;
}
