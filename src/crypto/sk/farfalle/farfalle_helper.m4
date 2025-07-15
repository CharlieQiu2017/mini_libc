`#include <stdint.h>'
`#include <string.h>'
`#include <crypto/hash/keccak/keccak_p.h>'
`#include <crypto/sk/farfalle/farfalle.h>'
`#include <crypto/sk/farfalle/farfalle_helper.h>'

define(`INST',`
#include <crypto/sk/farfalle/params/$1.h>

void farfalle_$1_add_string_part (struct farfalle_$1_state * st, struct farfalle_$1_helper_state * hst, const unsigned char * str, size_t str_len) {
  uint32_t off = hst->offset_ctr;
  unsigned char * k_ptr = (unsigned char *) st->k;
  unsigned char * p_ptr = (unsigned char *) hst->p;

  if (str_len < FARFALLE_PERM_LEN && str_len + off < FARFALLE_PERM_LEN) {
    for (uint32_t i = 0; i < str_len; ++i) {
      p_ptr[off + i] = k_ptr[off + i] ^ str[i];
    }

    hst->offset_ctr += str_len;
  } else {
    uint32_t r = FARFALLE_PERM_LEN - off;

    for (uint32_t i = 0; i < r; ++i) {
      p_ptr[off + i] = k_ptr[off + i] ^ str[i];
    }

    farfalle_perm_c (hst->p);

    for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
      st->sum[i] ^= hst->p[i];
    }

    str += r;
    str_len -= r;
    farfalle_roll_c (st->k);

    while (str_len >= FARFALLE_PERM_LEN) {
      for (uint32_t i = 0; i < FARFALLE_PERM_LEN; ++i) {
	p_ptr[i] = k_ptr[i] ^ str[i];
      }

      farfalle_perm_c (hst->p);

      for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
	st->sum[i] ^= hst->p[i];
      }

      str += FARFALLE_PERM_LEN;
      str_len -= FARFALLE_PERM_LEN;
      farfalle_roll_c (st->k);
    }

    for (uint32_t i = 0; i < str_len; ++i) {
      p_ptr[i] = k_ptr[i] ^ str[i];
    }

    hst->offset_ctr = str_len;
  }
}

void farfalle_$1_finalize_string (struct farfalle_$1_state * st, struct farfalle_$1_helper_state * hst) {
  uint32_t off = hst->offset_ctr;
  unsigned char * k_ptr = (unsigned char *) st->k;
  unsigned char * p_ptr = (unsigned char *) hst->p;

  p_ptr[off] = k_ptr[off] ^ 1;

  for (uint32_t i = off + 1; i < FARFALLE_PERM_LEN; ++i) {
    p_ptr[i] = k_ptr[i];
  }

  farfalle_perm_c (hst->p);

  for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
    st->sum[i] ^= hst->p[i];
  }

  farfalle_roll_c (st->k);
  farfalle_roll_c (st->k);
}

#include <crypto/sk/farfalle/params/clear.h>
')dnl

INST(kravatte)dnl
