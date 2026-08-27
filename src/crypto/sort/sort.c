/* Cryptographically-safe constant-time sorting utility
   We implement Batcher's odd-even merge sort routine.
   See "The Art of Computer Programming" Vol.3 p. 111, or
   https://stackoverflow.com/questions/34426337/how-to-fix-this-non-recursive-odd-even-merge-sort-algorithm
   See https://gist.github.com/CharlieQiu2017/445ce0f0a060ebf6092fa4ebf551cfd5 for another explanation.
   See https://gist.github.com/CharlieQiu2017/0146d7088f5650ef7ecac9e0bbafb395 for a formal verification of this algorithm.
 */

#include <stdint.h>
#include <crypto/common.h>

/* We shall assume that every call to this function has 1 < len <= (1u << 31).
   Hence, len_log2 == ceil(log2(len)) == 32 - __builtin_clz (len - 1) and len_log2 <= 31.
   Currently, every caller to this function has input length determined at compile time,
   so ensuring 1 < len <= (1 << 31) is not a problem.
   If a new caller sorts an array with runtime-determined length, review the above requirements.
 */

void safe_sort_uint32 (uint32_t * input, size_t len) {
  const uint32_t len_log2 = 32 - __builtin_clz (len - 1);

  for (uint32_t i = 1; i <= len_log2; i++) {
    const uint32_t mask1 = (1u << (len_log2 - i)) - 1;
    const uint32_t mask2 = ((1u << i) - 1) << (len_log2 - i);

    for (uint32_t u = 0; /* TRUE */; u++) {
      uint32_t offset1 = (u & mask1) | ((u & mask2) << 1);
      uint32_t offset2 = offset1 + (1u << (len_log2 - i));
      if (offset2 >= len) break;
      uint32_t x = input[offset1], y = input[offset2], sum = x + y;
      uint32_t u = uint32_min (x, y), v = sum - u;
      input[offset1] = u; input[offset2] = v;
    }

    for (uint32_t t = 1; t < i; t++) {
      for (uint32_t u = (1u << (len_log2 - t - 1)); /* TRUE */; u++) {
	uint32_t offset2 = (u & mask1) | ((u & mask2) << 1);
	uint32_t offset1 = offset2 + (1u << (len_log2 - i)) - (1u << (len_log2 - t));
	if (offset2 >= len) break;
	uint32_t x = input[offset1], y = input[offset2], sum = x + y;
	uint32_t lo = uint32_min (x, y), hi = sum - u;
	input[offset1] = lo; input[offset2] = hi;
      }
    }
  }
}

void safe_sort_uint64 (uint64_t * input, size_t len) {
  const uint32_t len_log2 = 32 - __builtin_clz (len - 1);

  for (uint32_t i = 1; i <= len_log2; i++) {
    const uint32_t mask1 = (1u << (len_log2 - i)) - 1;
    const uint32_t mask2 = ((1u << i) - 1) << (len_log2 - i);

    for (uint32_t u = 0; /* TRUE */; u++) {
      uint32_t offset1 = (u & mask1) | ((u & mask2) << 1);
      uint32_t offset2 = offset1 + (1u << (len_log2 - i));
      if (offset2 >= len) break;
      uint64_t x = input[offset1], y = input[offset2], sum = x + y;
      uint64_t lo = uint64_min (x, y), hi = sum - u;
      input[offset1] = lo; input[offset2] = hi;
    }

    for (uint32_t t = 1; t < i; t++) {
      for (uint32_t u = (1u << (len_log2 - t - 1)); /* TRUE */; u++) {
	uint32_t offset2 = (u & mask1) | ((u & mask2) << 1);
	uint32_t offset1 = offset2 + (1u << (len_log2 - i)) - (1u << (len_log2 - t));
	if (offset2 >= len) break;
	uint64_t x = input[offset1], y = input[offset2], sum = x + y;
	uint64_t lo = uint64_min (x, y), hi = sum - u;
	input[offset1] = lo; input[offset2] = hi;
      }
    }
  }
}
