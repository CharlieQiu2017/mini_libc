#ifndef SPHINCS_ADRS_H
#define SPHINCS_ADRS_H

#include <stdint.h>

/* The ADRS structure used in SPHINCS+ consists of 8 32-bit integers.
   The "tree address" field occupies 96 bits.
   However, all recommended instantiations of SPHINCS+ use only 64 bits of them.
   As always we assume little-endian architecture.
   Unfortunately, SPHINCS+ chooses to use big-endian encoding.
 */

static inline void sphincs_adrs_set_layer (uint32_t * adrs, uint8_t layer) {
  adrs[0] = ((uint32_t) layer) << 24;
}

static inline void sphincs_adrs_set_tree_addr_swapped (uint32_t * adrs, uint64_t tree_addr) {
  adrs[1] = 0;
  adrs[2] = tree_addr & 0xffffffff;
  adrs[3] = tree_addr >> 32;
}

static inline void sphincs_adrs_set_tree_addr (uint32_t * adrs, uint64_t tree_addr) {
  tree_addr = __builtin_bswap64 (tree_addr);
  sphincs_adrs_set_tree_addr_swapped (adrs, tree_addr);
}

static inline void sphincs_adrs_set_type_swapped (uint32_t * adrs, uint32_t type) {
  adrs[4] = type;
}

/* SPHINCS+ ADRS types */

#define SPHINCS_WOTS_HASH 0
#define SPHINCS_WOTS_PK 1
#define SPHINCS_TREE 2
#define SPHINCS_FORS_TREE 3
#define SPHINCS_FORS_ROOTS 4
#define SPHINCS_WOTS_PRF 5
#define SPHINCS_FORS_PRF 6

static inline void sphincs_adrs_set_type (uint32_t * adrs, uint32_t type) {
  type = __builtin_bswap32 (type);
  adrs[4] = type;
}

static inline void sphincs_adrs_set_keypair_swapped (uint32_t * adrs, uint16_t keypair) {
  adrs[5] = ((uint32_t) keypair) << 16;
}

static inline void sphincs_adrs_set_keypair (uint32_t * adrs, uint16_t keypair) {
  keypair = __builtin_bswap16 (keypair);
  sphincs_adrs_set_keypair_swapped (adrs, keypair);
}

static inline void sphincs_adrs_set_word6 (uint32_t * adrs, uint32_t w) {
  adrs[6] = w;
}

static inline void sphincs_adrs_set_word7 (uint32_t * adrs, uint32_t w) {
  adrs[7] = w;
}

#endif
