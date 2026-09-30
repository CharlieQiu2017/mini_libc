/* A wrapper for arrays with compile-time determined length and statically allocated storage.
   Default-initialization will NOT initialize any element in the array.
   It is up to the user to manually manage the lifecycle of every element in the array using placement new and destroy_at.
   A pointer to the whole array can be passed around as T*.
   All methods should work under constexpr.
   However, if some element of the array is not destructed when the whole array is destructed,
   and T is not trivially destructible, the constant evaluator will complain.
   The wrapper has trivial copy/move constructor/assignment operator if T has trivial copy/move constructor/assignment operator.
   The wrapper has trivial destructor if T has trivial destructor.
   HOWEVER, the wrapper is never trivially default constructible because the trivial default constructor of a union will not activate any member.
   Instead, the default constructor has to explicitly call std::start_lifetime() (P3762R2).
   This code is only possible under C++26 (>= GCC 17).
 */

#ifndef RAW_ARRAY_HPP
#define RAW_ARRAY_HPP

#include <stdint.h>
#include <exception>
#include <type_traits.hpp>
#include <utility.hpp>

namespace minilib {

namespace detail {

template < typename T, size_t N >
union raw_array_storage {
  T data[N];

  /* After GCC fully supports R3074R7 and R3726R2 (slated for GCC 17),
     the following two definitions can be merged into one, which is to call std::start_lifetime on the array.
   */

  /* If the default constructor of T is trivial, then the default constructor of the union is also trivial. */
  constexpr raw_array_storage () requires (minilib::is_trivially_default_constructible_v < T >) = default;

  /* If the default constructor of T is non-trivial, the default constructor of the union is deleted.
     We provide a user-defined constructor that does nothing.
   */
  constexpr raw_array_storage () requires (! minilib::is_trivially_default_constructible_v < T >) { }

  /* After GCC fully supports R3074R7, unions will always have trivial default destructors,
     and the following two definitions are not needed.
   */

  /* Destructor of T is trivial. In this case the default destructor for raw_array_storage is also trivial. */
  constexpr ~raw_array_storage () requires (minilib::is_trivially_destructible_v < T >) = default;

  /* Destructor of T is non-trivial. In this case the default destructor for raw_array_storage is deleted.
     We provide a user-defined destructor that does nothing.
   */
  constexpr ~raw_array_storage () requires (! minilib::is_trivially_destructible_v < T >) { };

  /* Maintain trivial copy/move constructor/move assignment operator */
  constexpr raw_array_storage (const raw_array_storage& other) requires (minilib::is_trivially_copy_constructible_v < T >) = default;

  constexpr raw_array_storage (raw_array_storage&& other) requires (minilib::is_trivially_move_constructible_v < T >) = default;

  constexpr raw_array_storage& operator= (const raw_array_storage& other) requires (minilib::is_trivially_copy_assignable_v < T >) = default;

  constexpr raw_array_storage& operator= (raw_array_storage&& other) requires (minilib::is_trivially_move_assignable_v < T >) = default;
};

}

template < typename T, size_t N >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T > && (N > 0))
class raw_array {
private:
  minilib::detail::raw_array_storage < T, N > storage;

public:
  /* Constructor: do nothing. The user is supposed to manage the lifecycle of each element. */
  constexpr raw_array () requires (minilib::is_trivially_default_constructible_v < T >) = default;

  constexpr raw_array () requires (! minilib::is_trivially_default_constructible_v < T >) { }

  /* Inherit trivial copy/move constructor/assignment operator */

  constexpr raw_array (const raw_array& other) requires (minilib::is_trivially_copy_constructible_v < T >) = default;

  constexpr raw_array (raw_array&& other) requires (minilib::is_trivially_move_constructible_v < T >) = default;

  constexpr raw_array& operator= (const raw_array& other) requires (minilib::is_trivially_copy_assignable_v < T >) = default;

  constexpr raw_array& operator= (raw_array&& other) requires (minilib::is_trivially_move_assignable_v < T >) = default;

  /* For other types, raw_array will not provide copy/move functionality,
     since every element has its own lifetime maintained separately,
     and we don't know which elements are currently valid.
   */

  /* Destructor: do nothing. If the user forgets to destruct any element, and the destructor of T is not trivial, the behavior is undefined. */
  constexpr ~raw_array () = default;

  template < typename... Args >
  requires (minilib::is_constructible_v < T, Args... >)
  constexpr void construct_at (size_t index, Args&&... args) {
    minilib::construct_at < T > (storage.data + index, minilib::forward < Args > (args)...);
  }

  template < typename... Args >
  requires (minilib::is_truly_default_constructible_v < T >)
  constexpr void default_construct_at (size_t index) {
    minilib::default_construct_at < T > (storage.data + index);
  }

  constexpr void destroy_at (size_t index) requires (minilib::is_destructible_v < T >) {
    minilib::destroy_at < T > (storage.data + index);
  }

  constexpr T * data () { return storage.data; }

  constexpr const T * data () const { return storage.data; }

  constexpr T& operator[] (size_t index) {
    if (index >= N) std::terminate ();
    return storage.data[index];
  }

  constexpr const T& operator[] (size_t index) const {
    if (index >= N) std::terminate ();
    return storage.data[index];
  }
};

}

#endif
