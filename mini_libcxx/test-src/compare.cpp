#include <stdint.h>
#include <exit.h>
#include <compare.hpp>

using namespace minilib;

void main ([[maybe_unused]] void * sp) {
  if (! (minilib::compare_three_way::operator() (-1ll, 1l) < 0)) exit (1);
  if (! (minilib::compare_three_way::operator() (1ull, -1) > 0)) exit (1);
  exit (0);
}
