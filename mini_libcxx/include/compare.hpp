#ifndef COMPARE_HPP
#define COMPARE_HPP

#include <stdint.h>
#include <utility.hpp>
#include <type_traits>

/* C++20 introduced the <=> comparison operator.
   However, it is too intimately tied to the standard library.
   This violates our philosophy of including as little libstdc++ code in the compiled binary as possible.
   Therefore, we provide a simple version that provides only strong_order on integral types, and customization points.
 */

namespace minilib {

namespace detail {

class order_result {
private:
  int32_t val;
  constexpr order_result (int32_t val_) : val (val_) { }

public:
  static constexpr order_result less () { return order_result (-1); }
  static constexpr order_result equal () { return order_result (0); }
  static constexpr order_result greater () { return order_result (1); }

  constexpr bool operator< (const order_result& other) const { return val < other.val; }
  constexpr bool operator> (const order_result& other) const { return val > other.val; }
  constexpr bool operator<= (const order_result& other) const { return val <= other.val; }
  constexpr bool operator>= (const order_result& other) const { return val >= other.val; }
  constexpr bool operator== (const order_result& other) const { return val == other.val; }
  constexpr bool operator!= (const order_result& other) const { return val != other.val; }

  constexpr bool operator< (int32_t other) const { return val < other; }
  constexpr bool operator> (int32_t other) const { return val > other; }
  constexpr bool operator<= (int32_t other) const { return val <= other; }
  constexpr bool operator>= (int32_t other) const { return val >= other; }
  constexpr bool operator== (int32_t other) const { return val == other; }
  constexpr bool operator!= (int32_t other) const { return val != other; }

  friend constexpr bool operator< (int32_t val, const order_result& other) { return val < other.val; }
  friend constexpr bool operator> (int32_t val, const order_result& other) { return val > other.val; }
  friend constexpr bool operator<= (int32_t val, const order_result& other) { return val <= other.val; }
  friend constexpr bool operator>= (int32_t val, const order_result& other) { return val >= other.val; }
  friend constexpr bool operator== (int32_t val, const order_result& other) { return val == other.val; }
  friend constexpr bool operator!= (int32_t val, const order_result& other) { return val != other.val; }
};

/* Poison pill to stop unqualified search */
void compare_three_way () = delete;

template < typename T, typename U >
concept has_adl_compare_three_way = requires (T&& t, U&& u) {
  /* The function call must resolve successfully */
  compare_three_way (minilib::forward < T > (t), minilib::forward < U > (u));
  /* The resulting expression can be returned as order_result. */
  requires minilib::returnable_to < decltype (compare_three_way (minilib::forward < T > (t), minilib::forward < U > (u))), minilib::detail::order_result >;
};

template < typename T, typename U >
requires (has_adl_compare_three_way < T, U >)
constexpr minilib::detail::order_result adl_compare_three_way (T&& t, U&& u) {
  return compare_three_way (minilib::forward < T > (t), minilib::forward < U > (u));
}

}

using order_result = minilib::detail::order_result;

struct compare_three_way {
  /* Convention introduced in C++14 to inform container the comparator supports heterogeneous comparison */
  using is_transparent = void;

  template < typename T, typename U >
  requires (minilib::detail::has_adl_compare_three_way < T, U >)
  static constexpr minilib::order_result operator () (T&& t, U&& u) {
    return minilib::detail::adl_compare_three_way (minilib::forward < T > (t), minilib::forward < U > (u));
  }

  template < typename T, typename U >
  requires (! minilib::detail::has_adl_compare_three_way < T, U > && std::is_integral_v < std::remove_cvref_t < T > > && std::is_integral_v < std::remove_cvref_t < U > >)
  static constexpr minilib::order_result operator () (T&& t, U&& u) {
    using UT = std::make_unsigned_t < std::remove_cvref_t < T > >;
    using UU = std::make_unsigned_t < std::remove_cvref_t < U > >;

    if constexpr (std::is_signed_v < std::remove_cvref_t < T > > == std::is_signed_v < std::remove_cvref_t < U > >) {
      return (t < u) ? minilib::order_result::less() : (t > u) ? minilib::order_result::greater() : minilib::order_result::equal();
    } else if constexpr (std::is_signed_v < std::remove_cvref_t < T > >) {
      if (t < 0) return minilib::order_result::less();
      UT ut = static_cast < UT > (t);
      return (ut < u) ? minilib::order_result::less() : (ut > u) ? minilib::order_result::greater() : minilib::order_result::equal();
    } else {
      if (u < 0) return minilib::order_result::greater();
      UU uu = static_cast < UU > (u);
      return (t < uu) ? minilib::order_result::less() : (t > uu) ? minilib::order_result::greater() : minilib::order_result::equal();
    }
  }
};

}

#endif
