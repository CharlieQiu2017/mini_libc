#ifndef FORWARD_HPP
#define FORWARD_HPP

/* type_traits.hpp depends on forward, but utility.hpp depends on type_traits.hpp,
   so we cannot put this in utility.hpp.
 */

#include <type_traits>

namespace minilib {

template < typename T >
constexpr T&& forward (std::remove_reference_t < T > & arg) {
  return static_cast < T&& > (arg);
}

template < typename T >
requires (! std::is_lvalue_reference_v < T >)
constexpr T&& forward (std::remove_reference_t < T > && arg) {
  return static_cast < T&& > (arg);
}

template < typename T >
constexpr std::remove_reference_t < T > && move (T&& t) {
  return static_cast < std::remove_reference_t < T > && > (t);
}

}

#endif
