#include <stdint.h>
#include <exit.h>
#include <raw_array.hpp>

static_assert (minilib::is_trivially_default_constructible_v < minilib::raw_array < uint32_t, 5 > >, "Constructor not trivial!");
static_assert (minilib::is_trivially_destructible_v < minilib::raw_array < uint32_t, 5 > >, "Destructor not trivial!");

constexpr uint32_t test_func() {
  minilib::raw_array < uint32_t, 5 > arr;
  arr.construct_at (0);
  arr.construct_at (1);
  arr.construct_at (2);
  arr.construct_at (3);
  arr[0] = 1;
  arr[1] = 2;
  arr[2] = 3;
  /* Leave fourth element uninitialized, and fifth element unconstructed */
  uint32_t result = arr[0] + arr[1] + arr[2];
  /* Because uint32_t is trivially destructible, it should not be necessary to destruct the elements explicitly */
  return result;
}

constexpr const uint32_t m = test_func ();

void main ([[maybe_unused]] void * sp) {
  if (m != 6) exit (1);
  exit (0);
}
