/* This file tests whether minilib::allocator works in constexpr environment */

#include <stdint.h>
#include <exit.h>
#include <utility.hpp>

constexpr uint32_t test_func () {
  uint32_t * arr = minilib::allocator < uint32_t > :: allocate (3);
  arr[0] = 1;
  arr[1] = 2;
  arr[2] = 3;
  uint32_t result = arr[0] + arr[1] + arr[2];
  minilib::allocator < uint32_t > :: deallocate (arr, 3);
  return result;
}

constexpr const uint32_t m = test_func ();

void main ([[maybe_unused]] void * sp) {
  if (m != 6) exit (1);
  exit (0);
}
