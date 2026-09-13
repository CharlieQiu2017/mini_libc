#ifndef CRYPTO_SORT_H
#define CRYPTO_SORT_H

#include <stdint.h>

/* Callers of these functions must ensure 1 < len <= (1ull << 63) */
void safe_sort_uint32 (uint32_t * input, size_t len);
void safe_sort_uint64 (uint64_t * input, size_t len);

#endif
