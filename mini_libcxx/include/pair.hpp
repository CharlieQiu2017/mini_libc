#ifndef PAIR_HPP
#define PAIR_HPP

#include <stdint.h>
#include <utility>
#include <forward.hpp>
#include <utility.hpp>
#include <type_traits.hpp>
#include <compare.hpp>

namespace minilib {

template < typename T1, typename T2 >
struct pair {
  using first_type = T1;
  using second_type = T2;

  T1 first;
  T2 second;

  constexpr pair () = default;

  constexpr pair (const T1& a, const T2& b)
    : first (a), second (b) {}

  template < typename U1, typename U2 >
  requires (minilib::is_constructible_v < T1, U1&& > && minilib::is_constructible_v < T2, U2&& >)
  constexpr pair (U1&& a, U2&& b)
    : first (minilib::forward < U1 > (a)), second (minilib::forward < U2 > (b)) {}

  template < typename U1, typename U2 >
  requires (minilib::is_constructible_v < T1, const U1& > && minilib::is_constructible_v < T2, const U2& >)
  constexpr pair (const pair < U1, U2 >& other)
    : first (other.first), second (other.second) {}

  template < typename U1, typename U2 >
  requires (minilib::is_constructible_v < T1, U1&& > && minilib::is_constructible_v < T2, U2&& >)
  constexpr pair (pair < U1, U2 >&& other)
    : first (minilib::forward < U1 > (other.first)), second (minilib::forward < U2 > (other.second)) {}

  constexpr pair (const pair&) = default;
  constexpr pair (pair&&) = default;
  constexpr pair& operator= (const pair&) = default;
  constexpr pair& operator= (pair&&) = default;
  constexpr ~pair () = default;

  constexpr void swap (pair& other) {
    auto tmp_first = minilib::move (first);
    first = minilib::move (other.first);
    other.first = minilib::move (tmp_first);
    auto tmp_second = minilib::move (second);
    second = minilib::move (other.second);
    other.second = minilib::move (tmp_second);
  }

  friend constexpr bool operator== (const pair&, const pair&) = default;

  template < typename U1 = T1, typename U2 = T2 >
  requires (requires (const U1& a1, const U1& b1, const U2& a2, const U2& b2) {
    minilib::compare_three_way::operator () (a1, b1);
    minilib::compare_three_way::operator () (a2, b2);
  })
  friend constexpr minilib::order_result compare_three_way (const pair& a, const pair& b) {
    auto res = minilib::compare_three_way::operator () (a.first, b.first);
    if (res != 0) return res;
    return minilib::compare_three_way::operator () (a.second, b.second);
  }
};

template < typename T1, typename T2 >
constexpr minilib::pair < std::decay_t < T1 >, std::decay_t < T2 > > make_pair (T1&& a, T2&& b) {
  return minilib::pair < std::decay_t < T1 >, std::decay_t < T2 > > (minilib::forward < T1 > (a), minilib::forward < T2 > (b));
}

template < size_t I, typename T1, typename T2 >
constexpr auto& get (pair < T1, T2 >& p) {
  if constexpr (I == 0) return p.first;
  else if constexpr (I == 1) return p.second;
}

template < size_t I, typename T1, typename T2 >
constexpr const auto& get (const pair < T1, T2 >& p) {
  if constexpr (I == 0) return p.first;
  else if constexpr (I == 1) return p.second;
}

template < size_t I, typename T1, typename T2 >
constexpr auto&& get (pair < T1, T2 >&& p) {
  if constexpr (I == 0) return minilib::move (p.first);
  else if constexpr (I == 1) return minilib::move (p.second);
}

template < size_t I, typename T1, typename T2 >
constexpr const auto&& get (const pair < T1, T2 >&& p) {
  if constexpr (I == 0) return minilib::move (p.first);
  else if constexpr (I == 1) return minilib::move (p.second);
}

}

#endif
