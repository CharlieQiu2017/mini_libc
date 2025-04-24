#include <stdint.h>
#include <crypto/sk/farfalle/farfalle.h>

struct farfalle_kravatte_sane_state {
  struct farfalle_kravatte_state fst;
  size_t tag_len;
  uint8_t parity;
};
