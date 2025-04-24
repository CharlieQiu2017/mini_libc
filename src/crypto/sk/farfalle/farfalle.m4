`
#include <stdint.h>
#include <string.h>
#include <crypto/hash/keccak/keccak_p.h>
#include <crypto/sk/farfalle/farfalle.h>
'

define(`INIT_BODY',`
  unsigned char * k_ptr = (unsigned char *) st->k;

  for (uint32_t i = 0; i < FARFALLE_K_LEN; ++i) {
    k_ptr[i] = k[i];
  }

  k_ptr[FARFALLE_K_LEN] = 1;

  for (uint32_t i = FARFALLE_K_LEN + 1; i < FARFALLE_PERM_LEN; ++i) {
    k_ptr[i] = 0;
  }

  farfalle_perm_b (st->k);

  for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
    st->k_orig[i] = st->k[i];
  }

  for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
    st->sum[i] = 0;
  }
')dnl

define(`INST',`
#include <crypto/sk/farfalle/params/$1.h>

void farfalle_$1_16_init (struct farfalle_$1_state * st, const unsigned char * k) {
#define FARFALLE_K_LEN 16
INIT_BODY
#undef FARFALLE_K_LEN
}

void farfalle_$1_24_init (struct farfalle_$1_state * st, const unsigned char * k) {
#define FARFALLE_K_LEN 24
INIT_BODY
#undef FARFALLE_K_LEN
}

void farfalle_$1_32_init (struct farfalle_$1_state * st, const unsigned char * k) {
#define FARFALLE_K_LEN 32
INIT_BODY
#undef FARFALLE_K_LEN
}

void farfalle_$1_reset (struct farfalle_$1_state * st) {
  for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
    st->k[i] = st->k_orig[i];
    st->sum[i] = 0;
  }
}

void farfalle_$1_add_string (struct farfalle_$1_state * st, const unsigned char * str, size_t str_len) {
  uint64_t p[FARFALLE_PERM_LEN / 8];

  unsigned char * k_ptr = (unsigned char *) st->k;
  unsigned char * p_ptr = (unsigned char *) p;

  while (str_len >= FARFALLE_PERM_LEN) {
    for (uint32_t i = 0; i < FARFALLE_PERM_LEN; ++i) {
      p_ptr[i] = k_ptr[i] ^ str[i];
    }

    farfalle_perm_c (p);

    for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
      st->sum[i] ^= p[i];
    }

    str += FARFALLE_PERM_LEN;
    str_len -= FARFALLE_PERM_LEN;
    farfalle_roll_c (st->k);
  }

  for (uint32_t i = 0; i < str_len; ++i) {
    p_ptr[i] = k_ptr[i] ^ str[i];
  }

  p_ptr[str_len] = k_ptr[str_len] ^ 1;

  for (uint32_t i = str_len + 1; i < FARFALLE_PERM_LEN; ++i) {
    p_ptr[i] = k_ptr[i];
  }

  farfalle_perm_c (p);

  for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
    st->sum[i] ^= p[i];
  }

  farfalle_roll_c (st->k);
  farfalle_roll_c (st->k);
}

void farfalle_$1_begin_extract (struct farfalle_$1_state * st) {
  for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
    st->y[i] = st->sum[i];
  }

  farfalle_perm_d (st->y);

  for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
    st->output_buf[i] = st->y[i];
  }

  farfalle_perm_e (st->output_buf);

  for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
    st->output_buf[i] ^= st->k[i];
  }

  st->output_offset = 0;
}

void farfalle_$1_begin_extract_short (struct farfalle_$1_state * st) {
  for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
    st->y[i] = st->sum[i];
    st->output_buf[i] = st->sum[i];
  }

  farfalle_perm_e (st->output_buf);

  for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
    st->output_buf[i] ^= st->k[i];
  }

  st->output_offset = 0;
}

void farfalle_$1_extract (struct farfalle_$1_state * st, unsigned char * out, size_t len) {
  uint32_t off = st->output_offset;
  unsigned char * buf_ptr = (unsigned char *) st->output_buf;
  unsigned char * k_ptr = (unsigned char *) st->k;

  if (len <= FARFALLE_PERM_LEN && off + len <= FARFALLE_PERM_LEN) {
    for (uint32_t i = 0; i < len; ++i) {
      out[i] = buf_ptr[off + i];
    }

    st->output_offset = off + len;
  } else {
    uint32_t r = FARFALLE_PERM_LEN - off;

    for (uint32_t i = 0; i < r; ++i) {
      out[i] = buf_ptr[off + i];
    }

    out += r;
    len -= r;

    while (len >= FARFALLE_PERM_LEN) {
      farfalle_roll_e (st->y);

      for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
	st->output_buf[i] = st->y[i];
      }

      farfalle_perm_e (st->output_buf);

      for (uint32_t i = 0; i < FARFALLE_PERM_LEN; ++i) {
	out[i] = k_ptr[i] ^ buf_ptr[i];
      }

      out += FARFALLE_PERM_LEN;
      len -= FARFALLE_PERM_LEN;
    }

    if (len > 0) {
      farfalle_roll_e (st->y);

      for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
	st->output_buf[i] = st->y[i];
      }

      farfalle_perm_e (st->output_buf);

      for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
	st->output_buf[i] ^= st->k[i];
      }

      for (uint32_t i = 0; i < len; ++i) {
	out[i] = buf_ptr[i];
      }

      st->output_offset = len;
    } else {
      st->output_offset = FARFALLE_PERM_LEN;
    }
  }
}

unsigned char farfalle_$1_extract_and_compare (struct farfalle_$1_state * st, const unsigned char * ref, size_t len) {
  uint32_t off = st->output_offset;
  unsigned char * buf_ptr = (unsigned char *) st->output_buf;

  if (len <= FARFALLE_PERM_LEN && off + len <= FARFALLE_PERM_LEN) {
    unsigned char result = safe_memcmp (buf_ptr + off, ref, len);
    st->output_offset = off + len;
    return result;
  } else {
    uint32_t r = FARFALLE_PERM_LEN - off;
    unsigned char result = safe_memcmp (buf_ptr + off, ref, r);

    len -= r;

    while (len >= FARFALLE_PERM_LEN) {
      farfalle_roll_e (st->y);

      for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
	st->output_buf[i] = st->y[i];
      }

      farfalle_perm_e (st->output_buf);

      for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
	st->output_buf[i] ^= st->k[i];
      }

      result |= safe_memcmp (buf_ptr, ref, FARFALLE_PERM_LEN);

      len -= FARFALLE_PERM_LEN;
    }

    if (len > 0) {
      farfalle_roll_e (st->y);

      for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
	st->output_buf[i] = st->y[i];
      }

      farfalle_perm_e (st->output_buf);

      for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
	st->output_buf[i] ^= st->k[i];
      }

      result |= safe_memcmp (buf_ptr, ref, len);

      st->output_offset = len;
    } else {
      st->output_offset = FARFALLE_PERM_LEN;
    }

    return result;
  }
}

void farfalle_$1_skip_output (struct farfalle_$1_state * st, size_t len) {
  uint32_t off = st->output_offset;

  if (len <= FARFALLE_PERM_LEN && off + len <= FARFALLE_PERM_LEN) {
    st->output_offset = off + len;
  } else {
    len -= FARFALLE_PERM_LEN - off;

    while (len >= FARFALLE_PERM_LEN) {
      farfalle_roll_e (st->y);

      for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
	st->output_buf[i] = st->y[i];
      }

      farfalle_perm_e (st->output_buf);

      len -= FARFALLE_PERM_LEN;
    }

    if (len > 0) {
      farfalle_roll_e (st->y);

      for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
	st->output_buf[i] = st->y[i];
      }

      farfalle_perm_e (st->output_buf);

      for (uint32_t i = 0; i < FARFALLE_PERM_LEN / 8; ++i) {
	st->output_buf[i] ^= st->k[i];
      }

      st->output_offset = len;
    } else {
      st->output_offset = FARFALLE_PERM_LEN;
    }
  }
}

#include <crypto/sk/farfalle/params/clear.h>
')dnl

INST(kravatte)
