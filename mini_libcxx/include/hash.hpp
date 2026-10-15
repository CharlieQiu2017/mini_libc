/* Hash interface

   In every instance, key is assumed to be 32 bytes of uniformly random data.
   Then hash (key, t) should return a 64-bit non-cryptographic digest of value.
   Despite being non-cryptographic, the implementation should still attempt to
   defend against DoS attack by making the digest unpredictable.
 */

#ifndef HASH_HPP
#define HASH_HPP

#include <stdint.h>
#include <forward.hpp>
#include <utility.hpp>
#include <type_traits.hpp>

namespace minilib {

namespace detail {

/* Poison pill to stop unqualified search */
void hash () = delete;

template < typename T >
concept has_adl_hash = requires (const unsigned char * key, T&& t) {
  /* The function call must resolve successfully */
  hash (key, minilib::forward < T > (t));
  /* The resulting expression can be returned as uint64_t. */
  requires minilib::returnable_to < decltype (hash (key, minilib::forward < T > (t))), uint64_t >;
};

template < typename T >
requires (has_adl_hash < T >)
constexpr uint64_t adl_hash (const unsigned char * key, T&& t) {
  return hash (key, minilib::forward < T > (t));
}

}

struct hash {
  template < typename T >
  requires (minilib::detail::has_adl_hash < T >)
  static constexpr uint64_t operator () (const unsigned char * key, T&& t) {
    return minilib::detail::adl_hash (key, minilib::forward < T > (t));
  }
};

template < typename T >
concept hashable = requires (const unsigned char * key, const T& a) {
  minilib::hash::operator () (key, a);
};

}

#endif
