#include <stdint.h>
#include <exit.h>
#include <pair.hpp>
#include <type_traits.hpp>
#include <utility.hpp>
#include <compare.hpp>

#define ASSERT_TRUE(cond) do { if (!(cond)) return false; } while (0)

constexpr bool test_pair_basic () {
  minilib::pair < int32_t, int64_t > p1;
  p1.first = 10;
  p1.second = 20;
  ASSERT_TRUE (p1.first == 10);
  ASSERT_TRUE (p1.second == 20);

  minilib::pair < int32_t, int64_t > p2 (10, 20);
  ASSERT_TRUE (p2.first == 10);
  ASSERT_TRUE (p2.second == 20);
  ASSERT_TRUE (p1 == p2);

  // make_pair
  auto p3 = minilib::make_pair (10, 20L);
  ASSERT_TRUE (p3.first == 10);
  ASSERT_TRUE (p3.second == 20L);

  // Structured binding
  auto [a, b] = p3;
  ASSERT_TRUE (a == 10);
  ASSERT_TRUE (b == 20L);

  // get<I>
  ASSERT_TRUE (minilib::get < 0 > (p3) == 10);
  ASSERT_TRUE (minilib::get < 1 > (p3) == 20L);

  // swap
  minilib::pair < int32_t, int32_t > pa (1, 2);
  minilib::pair < int32_t, int32_t > pb (3, 4);
  pa.swap (pb);
  ASSERT_TRUE (pa.first == 3 && pa.second == 4);
  ASSERT_TRUE (pb.first == 1 && pb.second == 2);

  // comparison
  minilib::pair < int32_t, int32_t > p_less (1, 2);
  minilib::pair < int32_t, int32_t > p_more (1, 3);
  ASSERT_TRUE (minilib::compare_three_way::operator () (p_less, p_more) < 0);
  ASSERT_TRUE (minilib::compare_three_way::operator () (p_more, p_less) > 0);
  ASSERT_TRUE (minilib::compare_three_way::operator () (p_less, p_less) == 0);

  return true;
}

constexpr bool passed = test_pair_basic ();
static_assert (passed);

void main ([[maybe_unused]] void * sp) {
  if (! test_pair_basic ()) exit (1);
  exit (0);
}
