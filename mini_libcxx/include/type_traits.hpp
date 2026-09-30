#ifndef TYPE_TRAITS_HPP
#define TYPE_TRAITS_HPP

#include <new>
#include <type_traits>
#include <forward.hpp>

/* This file exists for two reasons. The first is that the std type traits are often not what we actually want, so we have to redefine them.

   Second, this file resolves a difficult headache: you cannot check the type traits of an incomplete type.

   Let's say you define a type as follows:

   class node {
     int data;
     minilib::vector < node > children;
   };

   This code will not compile, because as soon as you define it the compiler will attempt to synthesize the "lifecycle functions":
   (1) default constructor; (2) destructor; (3) copy constructor; (4) move constructor; (5) copy assignment operator; (6) move assignment operator.
   And it will choke on this synthesis, because choosing destructor of minilib::vector requires checking the value of std::is_destructible_v < node >,
   but std::is_destructible_v < node > is undefined because node is still an incomplete type.

   To break free from this paradox we create these overridable wrappers for standard concepts.
   To make the above code compile, first forward declare class node, then define specializations of these wrappers for node.
   See test-src/pimpl.cpp for an example that compiles.
 */

namespace minilib {

template < typename T >
struct is_destructible {
  static constexpr bool value = std::is_destructible_v < T >;
};

template < typename T >
inline constexpr bool is_destructible_v = minilib::is_destructible < T > :: value;

/* Standard C++ std::is_X_constructible < T > is MISLEADINGLY named.
   It implicitly also tests whether T is destructible.
   Write our own concept that ONLY tests whether T can be constructed.
 */

template < typename T >
concept copy_constructible = requires (void * ptr, const T& t) { ::new (ptr) T (t); };

template < typename T >
struct is_copy_constructible {
  static constexpr bool value = minilib::copy_constructible < T >;
};

template < typename T >
inline constexpr bool is_copy_constructible_v = minilib::is_copy_constructible < T > :: value;

template < typename T >
concept move_constructible = requires (void * ptr, T&& t) { ::new (ptr) T (minilib::move (t)); };

template < typename T >
struct is_move_constructible {
  static constexpr bool value = minilib::move_constructible < T >;
};

template < typename T >
inline constexpr bool is_move_constructible_v = minilib::is_move_constructible < T > :: value;

template < typename T, typename... Args >
concept constructible = requires (void * ptr, Args&&... args) { ::new (ptr) T (minilib::forward < Args > (args)...); };

template < typename T, typename... Args >
struct is_constructible {
  static constexpr bool value = minilib::constructible < T, Args... >;
};

template < typename T, typename... Args >
inline constexpr bool is_constructible_v = minilib::is_constructible < T, Args... > :: value;

/* std::is_default_constructible is ESPECIALLY MISLEADINGLY named.
   It actually tests whether T can be value-initialized with no arguments (T ()).
   Write our own concept that actually tests whether T can be default-constructed.
 */

template < typename T >
concept truly_default_constructible = requires (void * ptr) { ::new (ptr) T; };

template < typename T >
struct is_truly_default_constructible {
  static constexpr bool value = minilib::truly_default_constructible < T >;
};

template < typename T >
inline constexpr bool is_truly_default_constructible_v = minilib::is_truly_default_constructible < T > :: value;

template < typename T >
concept copy_assignable = requires (T& dst, const T& src) { dst = src; };

template < typename T >
struct is_copy_assignable {
  static constexpr bool value = minilib::copy_assignable < T >;
};

template < typename T >
inline constexpr bool is_copy_assignable_v = minilib::is_copy_assignable < T > :: value;

template < typename T >
concept move_assignable = requires (T& dst, T&& src) { dst = minilib::move (src); };

template < typename T >
struct is_move_assignable {
  static constexpr bool value = minilib::move_assignable < T >;
};

template < typename T >
inline constexpr bool is_move_assignable_v = minilib::is_move_assignable < T > :: value;

/* std::is_trivially_default_constructible_v also tests whether T has trivial destructor.
   This is also the case with all other std::is_trivially_X traits.
   See: https://cplusplus.github.io/LWG/issue2116

   Beware further that is_trivially_X does not in general imply the X function we actually call is trivial.
   This is a very subtle point, the idea is that a templated member function can eclipse a special member function.
   std::is_trivially_X always checks whether the non-templated special member function is trivial,
   but when we actually make the call, overload resolution will bring us to the templated member function.
   This is now considered an anti-pattern: if one really intends to let a templated member function eclipse a special member function,
   the special member function should be marked as deleted.
   See: https://www.foonathan.net/2021/03/trivially-copyable/

   Before C++20, the main use-case of the above pattern is to conditionally override the implicit special member function with enable_if_t.
   This is because before C++20 there was no way to express "keep the implicit special member function if condition P is true, otherwise override it."
   Starting from C++20, we can constrain non-templated constructors with concepts, and this use-case no longer matters.

   Therefore, the decision of this library is to NOT support any class that uses a template to eclipse a special member function,
   but do not mark that special member function as deleted. Hence, we shall assume is_trivially_X means the X function we call is trivial.
 */

/* We also document here the exact guarantees provided by various is_trivially_X traits.

   is_trivially_destructible: We can implicitly terminate the lifetime of an object without calling ~T().
   The object's lifetime ends automatically when its storage is released or reused.

   is_trivially_default_constructible: placement new default initialization and std::uninitialized_default_construct compile to no-op.
   If is_trivially_default_constructible == true, then T is an implicit-lifetime type,
   and in non-constexpr context std::start_lifetime_as can be used to simulate the effect of the default constructor.

   is_trivially_copy_constructible/is_trivially_move_constructible: placement new copy/move initialization compile to memcpy.
   If is_trivially_copy_constructible or is_trivially_move_constructible is true, then T is also an implicit lifetime type.
   In this case, memcpy/memmove *implicitly* starts the lifetime of T.
   Therefore, calls to the copy/move constructor can be replaced with memcpy.
   Also, if the byte-representation of T was previously written to disk or sent through network, and later read back into memory,
   then one can use std::start_lifetime_as to simulate the effect of the trivial copy/move constructor.

   is_trivially_copy_assignable/is_trivially_move_assignable: if both src, dst are arrays of already-existing objects,
   memcpy/memmove (dst, src, n) is legal way to assign src to dst.

   is_trivially_copyable: (1) there is at least one of copy/move constructor/assignment operator;
   (2) every copy/move constructor/assignment operator is either trivial or deleted;
   (3) T also has a trivial destructor.
 */

/* In this library, we further more make the following assumptions on the types we support:
   1. If T has a copy constructor, a copy-assignment operator, and a destructor, then destroying an existing object
   and copy-constructing a new object at the same location has the same logical effect as copy-assigning the new object to the existing object.
   2. If T has a move constructor, a move-assignment operator, and a destructor, then destroying an existing object
   and move-constructing a new object at the same location has the same logical effect as move-assigning the new object to the existing object.
   3. If T has copy constructor, move constructor, and destructor,
   then copy-constructing A at C has the same logical effect as first copy-constructing A at B, then move-construct B at C, then destroy B.
   4. If T has copy constructor, move constructor, and destructor,
   then move-constructing A at C has the same logical effect as first move-constructing A at B, then move-construct B at C, then destroy B.

   If T does not satisfy the assumptions above, then the special member functions of T should be marked as deleted,
   and the container implementation will not attempt to exploit optimizations based on these assumptions.
 */

template < typename T >
struct is_trivially_default_constructible {
  static constexpr bool value = std::is_trivially_default_constructible_v < T >;
};

template < typename T >
inline constexpr bool is_trivially_default_constructible_v = minilib::is_trivially_default_constructible < T > :: value;

template < typename T >
struct is_trivially_copy_constructible {
  static constexpr bool value = std::is_trivially_copy_constructible_v < T >;
};

template < typename T >
inline constexpr bool is_trivially_copy_constructible_v = minilib::is_trivially_copy_constructible < T > :: value;

template < typename T >
struct is_trivially_move_constructible {
  static constexpr bool value = std::is_trivially_move_constructible_v < T >;
};

template < typename T >
inline constexpr bool is_trivially_move_constructible_v = minilib::is_trivially_move_constructible < T > :: value;

template < typename T >
struct is_trivially_copy_assignable {
  static constexpr bool value = std::is_trivially_copy_assignable_v < T >;
};

template < typename T >
inline constexpr bool is_trivially_copy_assignable_v = minilib::is_trivially_copy_assignable < T > :: value;

template < typename T >
struct is_trivially_move_assignable {
  static constexpr bool value = std::is_trivially_move_assignable_v < T >;
};

template < typename T >
inline constexpr bool is_trivially_move_assignable_v = minilib::is_trivially_move_assignable < T > :: value;

template < typename T >
struct is_trivially_destructible {
  static constexpr bool value = std::is_trivially_destructible_v < T >;
};

template < typename T >
inline constexpr bool is_trivially_destructible_v = minilib::is_trivially_destructible < T > :: value;

/* Check whether a value of type "From" can be returned in a function whose return type is "To". */
template < typename From, typename To >
concept returnable_to = requires (From x) {
  []() -> To { return x; };
};

}

#endif
