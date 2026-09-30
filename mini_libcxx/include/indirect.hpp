#ifndef INDIRECT_HPP
#define INDIRECT_HPP

#include <stdint.h>
#include <exception>
#include <type_traits.hpp>
#include <utility.hpp>
#include <compare.hpp>

namespace minilib {

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class indirect {
private:
  T * ptr;

public:
  constexpr indirect () : ptr (nullptr) { }

  constexpr indirect (indirect&& other) : ptr (other.ptr) { other.ptr = nullptr; }

  /* DANGEROUS: Allocate space for T but do not initialize it.
     The caller must immediately obtain the pointer through data() and manually initialize it with placement new.
     This is intended to cover initialization patterns that construct() and construct_default() could not cover.
     If the caller invokes any subsequent operation on indirect before initializing it, the behavior is undefined.
     Do not use this unless you know what you are doing.
   */
  constexpr void construct_null () {
    if (ptr != nullptr) std::terminate ();
    ptr = minilib::allocator < T > :: allocate (1);
    if (ptr == nullptr) std::terminate ();
  }

  constexpr indirect (const indirect& other) requires (minilib::is_copy_constructible_v < T >) : ptr (nullptr) {
    if (other.ptr == nullptr) return;
    construct_null ();
    minilib::construct_at < T > (ptr, *(other.ptr));
  }

  template < typename... Args >
  requires (minilib::is_constructible_v < T, Args... >)
  constexpr void construct (Args&&... args) {
    construct_null ();
    minilib::construct_at < T > (ptr, minilib::forward < Args > (args)...);
  }

  /* DANGEROUS: If default-initialization leaves some members uninitialized,
     and one later attempts to copy T, the behavior is undefined.
   */
  constexpr void construct_default () requires (minilib::is_truly_default_constructible_v < T >) {
    construct_null ();
    minilib::default_construct_at < T > (ptr);
  }

  /* This function does not require is_destructible_v, but if we need to call the destructor but can't, we panic.
     This if is_destructible_v == false, this function can serve as a check that the user has already destructed the object.
   */
  constexpr void destruct () {
    if (ptr != nullptr) {
      if constexpr (minilib::is_destructible_v < T >) {
	minilib::destroy_at (ptr);
	minilib::allocator < T > :: deallocate (ptr, 1);
	ptr = nullptr;
      } else {
	std::terminate ();
      }
    }
  }

  /* If is_destructible_v < T > == false, and the container holds an object first manually destruct the existing object,
     then call inform_destruct() below, and finally use construct() to make the copy.
   */
  constexpr indirect& operator= (const indirect& other) requires (minilib::is_copy_constructible_v < T >) {
    if (this == minilib::addressof (other)) return *this;

    if (other.ptr == nullptr) {

      destruct ();
      return *this;

    } else if (ptr != nullptr) {

      if ! consteval {
	if constexpr (minilib::is_trivially_copy_constructible_v < T > || minilib::is_trivially_copy_assignable_v < T >) {
	  __builtin_memcpy (ptr, other.ptr, sizeof (T));
	  return *this;
	}
      }

      if constexpr (minilib::is_copy_assignable_v < T >) {
	*ptr = *(other.ptr);
	return *this;
      }

      minilib::destroy_at < T > (ptr);
      minilib::construct_at < T > (ptr, *(other.ptr));
      return *this;

    } else {

      construct_null ();
      minilib::construct_at < T > (ptr, *(other.ptr));
      return *this;

    }
  }

  constexpr indirect& operator= (indirect&& other) {
    if (this == minilib::addressof (other)) return *this;
    destruct ();
    ptr = other.ptr;
    other.ptr = nullptr;
    return *this;
  }

  /* DANGEROUS: Inform the container that the contained value has been destructed.
     This is the converse of construct_null().
     This is intended for edge cases where T has a private destructor.
     Thus is_destructible_v < T > would be false, and indirect < T > could not be destructed via normal means.
     If the user still wants to use indirect < T > in this case, the lifecycle of T must be manually managed.
     To destruct T, first manually call T->~T(), then call inform_destruct() to deallocate memory.

     This function intentionally panics if ptr == nullptr.
     Any caller of this function MUST be completely aware of the managed memory state,
     including whether space is allocated or not, and whether an object exists.
     Calling this function with ptr == nullptr is an indication that the caller has confused.
   */
  constexpr void inform_destruct () {
    if (ptr == nullptr) std::terminate ();
    minilib::allocator < T > :: deallocate (ptr, 1);
    ptr = nullptr;
  }

  constexpr ~indirect () { destruct (); }

  constexpr T& operator* () {
    if (ptr == nullptr) std::terminate ();
    return *ptr;
  }

  constexpr const T& operator* () const {
    if (ptr == nullptr) std::terminate ();
    return *ptr;
  }

  constexpr T * operator-> () {
    if (ptr == nullptr) std::terminate ();
    return ptr;
  }

  constexpr const T * operator-> () const {
    if (ptr == nullptr) std::terminate ();
    return ptr;
  }

  constexpr explicit operator bool () const {
    return ptr != nullptr;
  }

  constexpr T * data () { return ptr; }

  constexpr const T * data () const { return ptr; }

  friend constexpr minilib::order_result compare_three_way (const indirect& a, const indirect& b)
  requires (requires (const T& x, const T& y) { minilib::compare_three_way::operator() (x, y); })
  {
    if (! static_cast < bool > (a)) {
      if (! static_cast < bool > (b)) return minilib::order_result::equal ();
      else return minilib::order_result::less ();
    } else {
      if (! static_cast < bool > (b)) return minilib::order_result::greater ();
      else return minilib::compare_three_way::operator() (*a, *b);
    }
  }
};

/* indirect_array does not support zero-length arrays. len == 0 is always equivalent to ptr == nullptr. */

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class indirect_array {
private:
  size_t len;
  T * ptr;

public:
  constexpr indirect_array () : len (0), ptr (nullptr) { }

  constexpr indirect_array (indirect_array&& other) : len (other.len), ptr (other.ptr) { other.len = 0; other.ptr = nullptr; }

  /* We allow calling construct() with length 0, but beware that this will not actually allocate, and subsequent calls to data() will return nullptr. */

  /* DANGEROUS: See indirect::construct_null().
     This is intended to cover cases where the object initialization pattern is not covered by the functions below.
     Also, one can use this to implement a dynamically allocated array where the lifecycle of every element is manually managed.
     Do not use this unless you know what you are doing.
   */
  constexpr void construct_null (size_t len_) {
    if (ptr != nullptr) std::terminate ();
    if (len_ == 0) return;
#ifdef CONFIG_DEFENSIVE
    if (__builtin_mul_overflow_p (sizeof (T), len_, 0ull)) std::terminate ();
#endif
    len = len_;
    ptr = minilib::allocator < T > :: allocate (len_);
    if (ptr == nullptr) std::terminate ();
  }

  constexpr indirect_array (const indirect_array& other) requires (minilib::is_copy_constructible_v < T >) :
    len (0), ptr (nullptr)
  {
    if (other.len == 0) return;
    construct_null (other.len);

    if ! consteval {
      if constexpr (minilib::is_trivially_copy_constructible_v < T >) {
	__builtin_memcpy (ptr, other.ptr, sizeof (T) * len);
	return;
      }
    }

    for (size_t i = 0; i < len; i++) minilib::construct_at < T > (ptr + i, other.ptr[i]);
  }

  /* This function only allows const lvalue arguments. The reason is that if any argument is rvalue,
     it would be moved upon the first constructor call, breaking all subsequent constructor calls.
     If rvalue constructor arguments are absolutely necessary, use the generate() function below.
   */
  template < typename... Args >
  requires (minilib::is_constructible_v < T, const Args&... >)
  constexpr void construct (size_t len_, const Args&... args) {
    construct_null (len_);
    for (size_t i = 0; i < len_; i++) minilib::construct_at < T > (ptr + i, args...);
  }

  /* DANGEROUS: See indirect::construct_default() */
  constexpr void construct_default (size_t len_) requires (minilib::is_truly_default_constructible_v < T >) {
    construct_null (len_);
    for (size_t i = 0; i < len_; i++) minilib::default_construct_at < T > (ptr + i);
  }

  template < typename... ArgGenerators >
  requires (minilib::is_constructible_v < T, std::invoke_result_t < ArgGenerators, size_t > ... >)
  constexpr void generate (size_t len_, ArgGenerators&&... gs) {
    construct_null (len_);
    for (size_t i = 0; i < len_; ++i) minilib::construct_at < T > (ptr + i, gs (i)...);
  }

  /* See comments on indirect::destruct() */
  constexpr void destruct () {
    if (ptr != nullptr) {
      if constexpr (minilib::is_destructible_v < T >) {
	/* Destructors are customarily called in reverse order from constructors.
	   However, we cannot write for (size_t i = len - 1; i >= 0; i--) here,
	   otherwise the loop will never terminate since i >= 0 is always true.
	 */
	for (size_t i = len; i >= 1; i--) minilib::destroy_at (ptr + (i - 1));
	minilib::allocator < T > :: deallocate (ptr, len);
	ptr = nullptr;
	len = 0;
      } else {
	std::terminate ();
      }
    }
  }

  /* As an optimization, try to reuse memory if array lengths equal */
  constexpr indirect_array& operator= (const indirect_array& other) requires (minilib::is_copy_constructible_v < T >) {
    if (this == minilib::addressof (other)) return *this;

    if (other.len == 0) {
      destruct ();
      return *this;
    }

    if (len == other.len) {

      if ! consteval {
	if constexpr (minilib::is_trivially_copy_constructible_v < T > || minilib::is_trivially_copy_assignable_v < T >) {
	  __builtin_memcpy (ptr, other.ptr, sizeof (T) * len);
	  return *this;
	}
      }

      if constexpr (minilib::is_copy_assignable_v < T >) {

	for (size_t i = 0; i < len; i++) ptr[i] = other.ptr[i];
	return *this;

      } else {

	for (size_t i = len; i > 0; i--) minilib::destroy_at < T > (ptr + i);

      }

    } else {

      destruct ();
      construct_null (other.len);

    }

    if ! consteval {
      if constexpr (minilib::is_trivially_copy_constructible_v < T >) {
	__builtin_memcpy (ptr, other.ptr, sizeof (T) * len);
	return *this;
      }
    }

    for (size_t i = 0; i < len; i++) minilib::construct_at < T > (ptr + i, other.ptr[i]);
    return *this;
  }

  constexpr indirect_array& operator= (indirect_array&& other) {
    if (this == minilib::addressof (other)) return *this;
    destruct ();
    ptr = other.ptr;
    len = other.len;
    other.ptr = nullptr;
    other.len = 0;
    return *this;
  }

  /* DANGEROUS: See indirect::inform_destruct().
     In edge cases where T has a private destructor, first manually destruct each individual element,
     then call this function to deallocate storage. The destructor of this class will check whether storage is actually deallocated or not.
   */
  constexpr void inform_destruct () {
    if (ptr == nullptr) std::terminate ();
    minilib::allocator < T > :: deallocate (ptr, len);
    ptr = nullptr;
    len = 0;
  }

  constexpr ~indirect_array () { destruct (); }

  constexpr T& operator[] (size_t index) {
    if (index >= len) std::terminate ();
    return ptr[index];
  }

  constexpr const T& operator[] (size_t index) const {
    if (index >= len) std::terminate ();
    return ptr[index];
  }

  /* DANGEROUS: Get a raw reference to the array */
  constexpr T * data () { return ptr; }

  constexpr const T * data () const { return ptr; }

  /* len == 0 means ptr == nullptr */
  constexpr size_t size () const { return len; }

  constexpr explicit operator bool () const { return len != 0; }

  /* Lexicographic ordering, see vector.hpp */
  friend constexpr minilib::order_result compare_three_way (const indirect_array& a, const indirect_array& b)
  requires (requires (const T& x, const T& y) { minilib::compare_three_way::operator() (x, y); })
  {
    size_t len_a = a.size (), len_b = b.size ();
    size_t i = 0;

    while (i < len_a && i < len_b) {
      minilib::order_result cmp = minilib::compare_three_way::operator() (a[i], b[i]);
      if (cmp < 0) return minilib::order_result::less ();
      else if (cmp > 0) return minilib::order_result::greater ();
      i++;
    }

    return minilib::compare_three_way::operator() (len_a, len_b);
  }
};

}

#endif
