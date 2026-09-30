#ifndef UNIQUE_PTR_HPP
#define UNIQUE_PTR_HPP

#include <stdint.h>
#include <exception>
#include <concepts>
#include <type_traits>
#include <utility.hpp>

namespace minilib {

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T > && std::is_destructible_v < T >)
class unique_ptr {
private:
  T * ptr;

public:
  constexpr unique_ptr () : ptr (nullptr) { }

  unique_ptr (const unique_ptr < T > &) = delete;

  constexpr unique_ptr (unique_ptr < T > && other) : ptr (other.ptr) { other.ptr = nullptr; }

  template < typename... Args >
  requires (std::constructible_from < T, Args... >)
  constexpr void construct (Args&&... args) {
    if (ptr != nullptr) std::terminate ();
    ptr = minilib::allocator < T > :: allocate (1);
    if (ptr == nullptr) std::terminate ();
    minilib::construct_at (ptr, minilib::forward < Args > (args)...);
  }

  /* DANGEROUS: If default-initialization leaves some members uninitialized,
     and one later attempts to copy T, the behavior is undefined.
   */
  constexpr void construct_default () requires (minilib::is_truly_default_constructible_v < T >) {
    if (ptr != nullptr) std::terminate ();
    ptr = minilib::allocator < T > :: allocate (1);
    if (ptr == nullptr) std::terminate ();
    minilib::default_construct_at < T > (ptr);
  }

  /* We deliberately do not support constructing unique_ptr from existing pointers.
     The idea is that this unique_ptr only ever manages pointers allocated within itself.
   */

  constexpr void destruct () {
    if (ptr != nullptr) {
      minilib::destroy_at (ptr);
      minilib::allocator < T > :: deallocate (ptr, 1);
      ptr = nullptr;
    }
  }

  constexpr ~unique_ptr () { destruct (); }

  unique_ptr < T > & operator= (const unique_ptr < T > &) = delete;

  constexpr unique_ptr < T > & operator= (unique_ptr < T > && other) {
    if (this == &other) return *this;
    destruct ();
    ptr = other.ptr;
    other.ptr = nullptr;
    return *this;
  }

  /* In theory, we could make clone() the copy constructor of unique_ptr.
     However that goes against the common notion of unique_ptr too much.
   */
  constexpr unique_ptr < T > clone () const requires (std::is_copy_constructible_v < T >) {
    unique_ptr < T > new_p;
    if (ptr == nullptr) return new_p;
    new_p.construct (*ptr);
    return new_p;
  }

  constexpr T& operator* () {
    if (ptr == nullptr) std::terminate ();
    return *ptr;
  }

  constexpr const T& operator* () const {
    if (ptr == nullptr) std::terminate ();
    return *ptr;
  }

  constexpr T* operator-> () {
    if (ptr == nullptr) std::terminate ();
    return ptr;
  }

  constexpr const T* operator-> () const {
    if (ptr == nullptr) std::terminate ();
    return ptr;
  }

  constexpr explicit operator bool () const {
    return ptr != nullptr;
  }
};

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T > && std::is_destructible_v < T >)
class unique_array_ptr {
private:
  size_t len;
  T * ptr;

public:
  constexpr unique_array_ptr () : len (0), ptr (nullptr) { }

  unique_array_ptr (const unique_array_ptr < T > &) = delete;

  constexpr unique_array_ptr (unique_array_ptr < T > && other) : len (other.len), ptr (other.ptr) { other.len = 0; other.ptr = nullptr; }

  /* This function only allows const lvalue arguments. The reason is that if any argument is rvalue,
     it would be moved upon the first constructor call, breaking all subsequent constructor calls.
     If rvalue constructor arguments are absolutely necessary, use the generate() function below.
   */
  template < typename... Args >
  requires (std::constructible_from < T, const Args&... >)
  constexpr void construct (size_t len_, const Args&... args) {
    if (ptr != nullptr) std::terminate ();
    /* If len_ == 0, aligned_alloc will return nullptr.
       Hence the caller must check len_ > 0.
       The caller must also ensure sizeof (T) * len_ does not overflow.
     */
#ifdef CONFIG_DEFENSIVE
    if (len_ == 0 || __builtin_mul_overflow_p (sizeof (T), len_, 0ull)) std::terminate ();
#endif
    len = len_;
    ptr = minilib::allocator < T > :: allocate (len_);
    if (ptr == nullptr) std::terminate ();
    for (size_t i = 0; i < len_; i++) minilib::construct_at < T > (ptr + i, args...);
  }

  /* DANGEROUS: See unique_ptr::construct_default()
   */
  constexpr void construct_default (size_t len_) requires (minilib::is_truly_default_constructible_v < T >) {
    if (ptr != nullptr) std::terminate ();
#ifdef CONFIG_DEFENSIVE
    if (len_ == 0 || __builtin_mul_overflow_p (sizeof (T), len_, 0ull)) std::terminate ();
#endif
    len = len_;
    ptr = minilib::allocator < T > :: allocate (len_);
    if (ptr == nullptr) std::terminate ();
    for (size_t i = 0; i < len_; i++) minilib::default_construct_at < T > (ptr + i);
  }

  constexpr void construct (const unique_array_ptr < T > &other) requires (std::copy_constructible < T >) {
    if (ptr != nullptr) std::terminate ();
    len = other.len;
    if (len == 0) return;
    /* If sizeof (T) * len overflows, `other` would already be in UB state, and would have been caught if CONFIG_DEFENSIVE is enabled */
    ptr = minilib::allocator < T > :: allocate (len);
    if (ptr == nullptr) std::terminate ();
    for (size_t i = 0; i < len; i++) minilib::construct_at < T > (ptr + i, other.ptr[i]);
  }

  template < typename... ArgGenerators >
  requires (requires (ArgGenerators&&... gs, size_t i) { T (gs (i)...); })
  constexpr void generate (size_t len_, ArgGenerators&&... gs) {
    if (ptr != nullptr) std::terminate();
#ifdef CONFIG_DEFENSIVE
    if (len_ == 0 || __builtin_mul_overflow_p (sizeof (T), len_, 0ull)) std::terminate ();
#endif
    len = len_;
    ptr = minilib::allocator < T > :: allocate (len);
    if (ptr == nullptr) std::terminate();
    for (size_t i = 0; i < len; ++i) minilib::construct_at < T > (ptr + i, gs (i)...);
  }

  constexpr void destruct () {
    if (ptr != nullptr) {
      /* Destructors are customarily called in reverse order from constructors.
	 However, we cannot write for (size_t i = len - 1; i >= 0; i--) here,
	 otherwise the loop will never terminate since i >= 0 is always true.
       */
      for (size_t i = len; i >= 1; i--) minilib::destroy_at (ptr + (i - 1));
      minilib::allocator < T > :: deallocate (ptr, len);
      ptr = nullptr;
      len = 0;
    }
  }

  constexpr ~unique_array_ptr () { destruct (); }

  void operator= (const unique_array_ptr < T > &) = delete;

  constexpr void operator= (unique_array_ptr < T > && other) {
    if (this == &other) return;
    destruct ();
    ptr = other.ptr;
    len = other.len;
    other.ptr = nullptr;
    other.len = 0;
  }

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
};

}

#endif
