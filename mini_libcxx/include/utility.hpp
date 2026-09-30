#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <stdlib.h>
#include <new>
#include <memory>
#include <utility>
#include <type_traits.hpp>
#include <forward.hpp>

namespace minilib {

/* To prevent a function from being called at runtime,
   let it contain a call to this function.
 */
__attribute__((error("consteval feature used in non-consteval context")))
void do_not_call_this ();

}

/* The following code exists solely to make constexpr allocator work in freestanding environment.
   This is because GCC allows calling ::operator new in constexpr code only if the caller is std::allocator::allocate.
   This is so stupid.
 */

namespace std {

template < typename T >
struct allocator {
  static constexpr T * allocate (size_t n) {
    if consteval {
      if constexpr (alignof (T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
	return static_cast < T * > (::operator new (sizeof (T) * n));
      } else {
	return static_cast < T * > (::operator new (sizeof (T) * n, align_val_t (alignof (T))));
      }
    } else {
      minilib::do_not_call_this ();
      return nullptr;
    }
  }

  static constexpr void deallocate (T * ptr, [[maybe_unused]] size_t n) {
    if consteval {
      if constexpr (alignof (T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
	::operator delete (static_cast < void * > (ptr));
      } else {
	::operator delete (static_cast < void * > (ptr), align_val_t (alignof (T)));
      }
    } else {
      minilib::do_not_call_this ();
    }
  }
};

}

namespace minilib {

/* We shall not support array types for allocator, construct_at, destroy_at. */

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T >)
class allocator {
public:
  static constexpr T * allocate (size_t n) {
    if consteval {
      return std::allocator < T > :: allocate (n);
    } else {
      /* aligned_alloc() and free() will be supplied by a custom bare-metal library.
	 This implementation does not care about whether size is a multiple of alignment.
       */
      return static_cast < T * > (aligned_alloc (alignof (T), sizeof (T) * n));
    }
  }

  static constexpr void deallocate (T * ptr, [[maybe_unused]] size_t n) {
    if consteval {
      std::allocator < T > :: deallocate (ptr, n);
    } else {
      free (ptr);
    }
  }
};

/* The following code assumes that placement new is constexpr. This requires P2747R2 (>= GCC 15). */

/* The standard signature requires construct_at to return p which is stupid.
   The original intention was related to pointer aliasing.
   If a storage space that was originally used to store type A
   was repurposed to store type B, then the original pointer
   to A could not be legally used to access B.
   The pointer returned from placement new was the only way to
   access the new object of type B.

   After C++17 the rules have changed and the old pointer could
   be used to access the new object in many but not all situations.
   For the remaining cases just use std::launder().

   Just return void here.
 */
template < typename T, typename... Args >
requires (! std::is_array_v < T > && minilib::is_constructible_v < T, Args&&... >)
constexpr void construct_at (T * p, Args&&... args) {
  ::new (static_cast < void * > (p)) T (minilib::forward < Args > (args)...);
}

/* std::construct_at does not provide a way to default-initialize T.
   Therefore, we provide an extra helper default_construct_at().
 */
template < typename T >
requires (! std::is_array_v < T > && minilib::is_truly_default_constructible_v < T >)
constexpr void default_construct_at (T * p) {
  ::new (static_cast < void * > (p)) T;
}

/* This function deliberately uses std::is_destructible_v instead of minilib::is_destructible_v.
   The idea is that if the user erred in specializing minilib::is_destructible_v we can still catch it.
 */
template < typename T >
requires (! std::is_array_v < T > && std::is_destructible_v < T >)
constexpr void destroy_at (T * p) {
  /* We shall assume the caller has checked p != nullptr */
  if ! consteval {
    if constexpr (std::is_trivially_destructible_v < T >) return;
  }
  p->~T();
}

/* See libstdc++/bits/move.h */
template < typename T >
requires (! std::is_reference_v < T >)
constexpr T * addressof (T& arg) {
  return __builtin_addressof (arg);
}

template < typename T >
requires (requires (const T& a, const T& b) { a < b; })
constexpr const T& min (const T& a, const T& b) {
  if (b < a) return b; else return a;
}

template < typename T >
requires (requires (const T& a, const T& b) { a < b; })
constexpr const T& max (const T& a, const T& b) {
  if (a < b) return b; else return a;
}

}

#endif
