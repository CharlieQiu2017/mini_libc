/* Raw dynamically-allocated storage.
   A class that cares nothing other than allocating storage for an array and freeing it upon destruction.
   It does not even store the length of the array.
   It is up to the caller to manage the lifetime of each individual element.
   This is a dangerous class. Each API documents why it is dangerous to use.
 */

#ifndef RAW_DYN_ARRAY_HPP
#define RAW_DYN_ARRAY_HPP

#include <exception>
#include <type_traits.hpp>
#include <utility.hpp>
#include <indirect.hpp>

namespace minilib {

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class raw_dyn_array {
private:
  /* Under constexpr, std::allocator strictly requires knowing the number of elements to deallocate.
     Therefore, we use indirect_array to emulate raw_dyn_array under constexpr.
   */
  union {
    T * ptr_real;
    minilib::indirect_array < T > * ptr_constexpr;
  };

public:
  constexpr raw_dyn_array () {
    if consteval {
      ptr_constexpr = minilib::allocator < minilib::indirect_array < T > > :: allocate (1);
      minilib::construct_at < minilib::indirect_array < T > > (ptr_constexpr);
    } else {
      ptr_real = nullptr;
    }
  }

  /* No copy constructor, because we don't know the number of elements. */
  raw_dyn_array (const raw_dyn_array&) = delete;

  constexpr raw_dyn_array (raw_dyn_array&& other) {
    if consteval {
      ptr_constexpr = other.ptr_constexpr;
      other.ptr_constexpr = minilib::allocator < minilib::indirect_array < T > > :: allocate (1);
      minilib::construct_at < minilib::indirect_array < T > > (other.ptr_constexpr);
    } else {
      ptr_real = other.ptr_real;
      other.ptr_real = nullptr;
    }
  }

  constexpr void allocate (size_t n) {
    if consteval {
      ptr_constexpr->construct_null (n);
    } else {
      if (ptr_real != nullptr) std::terminate ();
#ifdef CONFIG_DEFENSIVE
      if (__builtin_mul_overflow_p (sizeof (T), n, 0ull)) std::terminate ();
#endif
      ptr_real = minilib::allocator < T > :: allocate (n);
      if (ptr_real == nullptr) std::terminate ();
    }
  }

  /* DANGEROUS: This function DOES NOT call the destructors of elements,
     since we know neither how many elements there are nor which ones are still active.
     Therefore, if the user calls this function with any element remaining, and the destructor of T is not trivial, the behavior is undefined.
   */
  constexpr void deallocate () {
    if consteval {
      if (*ptr_constexpr) {
	ptr_constexpr->inform_destruct ();
      }
    } else {
      if (ptr_real == nullptr) return;
      /* In non-consteval context, the element count argument is unused. See utility.hpp. */
      minilib::allocator < minilib::indirect_array < T > > :: deallocate (ptr_real, 0);
      ptr_real = nullptr;
    }
  }

  raw_dyn_array& operator= (const raw_dyn_array&) = delete;

  /* DANGEROUS: This function calls deallocate(). */
  constexpr raw_dyn_array& operator= (raw_dyn_array&& other) {
    deallocate ();
    if consteval {
      ptr_constexpr = other.ptr_constexpr;
      other.ptr_constexpr = minilib::allocator < minilib::indirect_array < T > > :: allocate (1);
      minilib::construct_at < minilib::indirect_array < T > > (other.ptr_constexpr);
    } else {
      ptr_real = other.ptr_real;
      other.ptr_real = nullptr;
    }
  }

  constexpr ~raw_dyn_array () { deallocate (); }

  template < typename... Args >
  requires (minilib::is_constructible_v < T, Args... >)
  constexpr void construct_at (size_t index, Args&&... args) {
    if consteval {
      if (index >= ptr_constexpr->size ()) std::terminate ();
      T * p = ptr_constexpr->data () + index;
      minilib::construct_at < T > (p, minilib::forward < Args > (args)...);
    } else {
      if (ptr_real == nullptr) std::terminate ();
      T * p = ptr_real + index;
      minilib::construct_at < T > (p, minilib::forward < Args > (args)...);
    }
  }

  template < typename... Args >
  requires (minilib::is_truly_default_constructible_v < T >)
  constexpr void default_construct_at (size_t index) {
    if consteval {
      if (index >= ptr_constexpr->size ()) std::terminate ();
      T * p = ptr_constexpr->data () + index;
      minilib::default_construct_at < T > (p);
    } else {
      if (ptr_real == nullptr) std::terminate ();
      T * p = ptr_real + index;
      minilib::default_construct_at < T > (p);
    }
  }

  constexpr void destroy_at (size_t index) requires (minilib::is_destructible_v < T >) {
    if consteval {
      if (index >= ptr_constexpr->size ()) std::terminate ();
      T * p = ptr_constexpr->data () + index;
      minilib::destroy_at < T > (p);
    } else {
      if (ptr_real == nullptr) std::terminate ();
      T * p = ptr_real + index;
      minilib::destroy_at < T > (p);
    }
  }

  constexpr T& operator[] (size_t index) {
    if consteval {
      if (*ptr_constexpr) {
	return (*ptr_constexpr)[index];
      } else {
	std::terminate ();
      }
    } else {
      if (ptr_real != nullptr) {
	return ptr_real[index];
      } else {
	std::terminate ();
      }
    }
  }

  constexpr const T& operator[] (size_t index) const {
    if consteval {
      if (*ptr_constexpr) {
	return (*ptr_constexpr)[index];
      } else {
	std::terminate ();
      }
    } else {
      if (ptr_real != nullptr) {
	return ptr_real[index];
      } else {
	std::terminate ();
      }
    }
  }

  constexpr T * data () {
    if consteval {
      return ptr_constexpr->get ();
    } else {
      return ptr_real;
    }
  }

  constexpr const T * data () const {
    if consteval {
      return ptr_constexpr->get ();
    } else {
      return ptr_real;
    }
  }

  constexpr explicit operator bool () const {
    if consteval {
      return static_cast < bool > (*ptr_constexpr);
    } else {
      return ptr_real != nullptr;
    }
  }
}

}

#endif
