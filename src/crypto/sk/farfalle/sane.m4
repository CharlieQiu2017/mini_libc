`
/* SANE mode of Farfalle
   Requires user to provide nonce at beginning of each session.
   Nonce does not need to be unpredictable, but needs to be unique.
  */
'

`
#include <string.h>
#include <crypto/common.h>
#include <crypto/sk/farfalle/farfalle.h>
#include <crypto/sk/farfalle/farfalle_helper.h>
#include <crypto/sk/farfalle/sane.h>
'

define(`INIT_FUNC',`
void farfalle_$1_sane_$2_init (struct farfalle_$1_sane_state * st, const unsigned char * k, size_t tag_len) {
  farfalle_$1_$2_init (&st->fst, k);
  st->tag_len = tag_len;
}
')dnl

define(`INST',`
#include <crypto/sk/farfalle/params/$1.h>

INIT_FUNC($1,16)

INIT_FUNC($1,24)

INIT_FUNC($1,32)

void farfalle_$1_sane_start_session_common (struct farfalle_$1_sane_state * st, const unsigned char * nonce, size_t nonce_len) {
  farfalle_$1_reset (&st->fst);
  farfalle_$1_add_string (&st->fst, nonce, nonce_len);
  st->parity = 0;
  farfalle_$1_begin_extract (&st->fst);
}

void farfalle_$1_sane_start_session (struct farfalle_$1_sane_state * st, const unsigned char * nonce, size_t nonce_len) {
  farfalle_$1_sane_start_session_common (st, nonce, nonce_len);
  farfalle_$1_skip_output (&st->fst, st->tag_len);
}

void farfalle_$1_sane_start_session_with_tag (struct farfalle_$1_sane_state * st, const unsigned char * nonce, size_t nonce_len, unsigned char * tag_out) {
  farfalle_$1_sane_start_session_common (st, nonce, nonce_len);
  farfalle_$1_extract (&st->fst, tag_out, st->tag_len);
}

unsigned char farfalle_$1_sane_start_session_check_tag (struct farfalle_$1_sane_state * st, const unsigned char * nonce, size_t nonce_len, const unsigned char * tag) {
  farfalle_$1_sane_start_session_common (st, nonce, nonce_len);
  return farfalle_$1_extract_and_compare (&st->fst, tag, st->tag_len);
}

void farfalle_$1_sane_wrap (struct farfalle_$1_sane_state * st, const unsigned char * msg, size_t msg_len, const unsigned char * meta, size_t meta_len, unsigned char * ct_out, unsigned char * tag_out) {
  farfalle_$1_extract (&st->fst, ct_out, msg_len);
  for (uint32_t i = 0; i < msg_len; ++i) ct_out[i] ^= msg[i];

  struct farfalle_$1_helper_state hst;

  if (meta_len > 0 || msg_len == 0) {
    unsigned char final_byte = st->parity * 2;
    farfalle_$1_helper_init (&hst);
    farfalle_$1_add_string_part (&st->fst, &hst, meta, meta_len);
    farfalle_$1_add_string_part (&st->fst, &hst, &final_byte, 1);
    farfalle_$1_finalize_string (&st->fst, &hst);
  }

  if (msg_len > 0) {
    unsigned char final_byte = st->parity * 2 + 1;
    farfalle_$1_helper_init (&hst);
    farfalle_$1_add_string_part (&st->fst, &hst, ct_out, msg_len);
    farfalle_$1_add_string_part (&st->fst, &hst, &final_byte, 1);
    farfalle_$1_finalize_string (&st->fst, &hst);
  }

  farfalle_$1_begin_extract (&st->fst);
  farfalle_$1_extract (&st->fst, tag_out, st->tag_len);

  st->parity ^= 1;
}

unsigned char farfalle_$1_sane_unwrap (struct farfalle_$1_sane_state * st, const unsigned char * ct, size_t ct_len, const unsigned char * meta, size_t meta_len, const unsigned char * tag, unsigned char * msg_out) {
  /* Backup state in case unwrapping fails */
  struct farfalle_$1_state st_bak = st->fst;

  farfalle_$1_extract (&st->fst, msg_out, ct_len);
  for (uint32_t i = 0; i < ct_len; ++i) msg_out[i] ^= ct[i];

  struct farfalle_$1_helper_state hst;

  if (meta_len > 0 || ct_len == 0) {
    unsigned char final_byte = st->parity * 2;
    farfalle_$1_helper_init (&hst);
    farfalle_$1_add_string_part (&st->fst, &hst, meta, meta_len);
    farfalle_$1_add_string_part (&st->fst, &hst, &final_byte, 1);
    farfalle_$1_finalize_string (&st->fst, &hst);
  }

  if (ct_len > 0) {
    unsigned char final_byte = st->parity * 2 + 1;
    farfalle_$1_helper_init (&hst);
    farfalle_$1_add_string_part (&st->fst, &hst, ct, ct_len);
    farfalle_$1_add_string_part (&st->fst, &hst, &final_byte, 1);
    farfalle_$1_finalize_string (&st->fst, &hst);
  }

  farfalle_$1_begin_extract (&st->fst);
  unsigned char result = uint8_to_bool (farfalle_$1_extract_and_compare (&st->fst, tag, st->tag_len));

  /* If result != 0, rollback state */
  /* Otherwise, keep current state, flip parity */

  cond_memcpy (result, &st->fst, &st_bak, sizeof (struct farfalle_$1_state));
  st->parity ^= 1 - result;

  return result;
}

#include <crypto/sk/farfalle/params/clear.h>
')dnl

INST(kravatte)
