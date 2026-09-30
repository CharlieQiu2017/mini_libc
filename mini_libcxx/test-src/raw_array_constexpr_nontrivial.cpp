#include <stdint.h>
#include <utility.hpp>
#include <raw_array.hpp>
#include <type_traits.hpp>

struct Probe {
  int32_t x;
  constexpr Probe (int32_t v) : x (v) { }
  constexpr ~Probe () { x = 0; } // Non-trivial destructor
};

static_assert(! minilib::is_trivially_destructible_v < Probe > );
static_assert(! minilib::is_trivially_destructible_v < minilib::raw_array < Probe, 3 > >);

constexpr int32_t test_probe () {
    minilib::raw_array < Probe, 3 > arr;
    arr.construct_at (0, 42);
    int32_t val = arr[0].x;
    arr.destroy_at (0);
    return val;
}

constexpr const int32_t m = test_probe ();

void main ([[maybe_unused]] void * sp) {
  if (m != 42) exit (1);
  exit (0);
}
