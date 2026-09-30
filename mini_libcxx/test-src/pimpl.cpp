/* See include/type_traits.hpp */

#include <stdint.h>
#include <exit.h>
#include <vector.hpp>
#include <type_traits.hpp>

class node;

template < > struct minilib::is_destructible < node > {
  static constexpr bool value = true;
};

template < > struct minilib::is_copy_constructible < node > {
  static constexpr bool value = true;
};

template < > struct minilib::is_move_constructible < node > {
  static constexpr bool value = true;
};

class node {
public:
  int32_t data;
  minilib::vector < node > children;
};

void main ([[maybe_unused]] void * sp) {
  node x;
  x.data = 3;
  exit (0);
}
