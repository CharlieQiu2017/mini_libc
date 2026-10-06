#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <stdint.h>
#include <exception>
#include <type_traits.hpp>
#include <utility.hpp>
#include <compare.hpp>

namespace minilib {

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class vector {
private:
  size_t alloc_len, len;
  T * ptr;

public:
  constexpr vector () : alloc_len (0), len (0), ptr (nullptr) { }

  constexpr vector (vector&& other) :
    alloc_len (other.alloc_len), len (other.len), ptr (other.ptr)
  {
    other.alloc_len = 0;
    other.len = 0;
    other.ptr = nullptr;
  }

  constexpr vector (const vector& other) requires (minilib::is_copy_constructible_v < T >) :
    alloc_len (other.len), len (other.len), ptr (nullptr)
  {
    if (len == 0) return;
    /* sizeof (T) * len would not overflow, otherwise `other` is already in UB */
    ptr = minilib::allocator < T > :: allocate (len);
    if (ptr == nullptr) std::terminate ();

    if ! consteval {
      if constexpr (minilib::is_trivially_copy_constructible_v < T >) {
	/* See type_traits.h on why memcpy can replace construct_at calls here.
	   Because we are in freestanding environment, the compiler does not recognize the semantics of memcpy.
	   Use __builtin_memcpy instead. The compiler will recognize the semantics and emit memcpy.
	 */
	__builtin_memcpy (ptr, other.ptr, sizeof (T) * len);
	return;
      }
    }

    for (size_t i = 0; i < len; i++) minilib::construct_at < T > (ptr + i, other.ptr[i]);
  }

  /* clear() only destroys elements but does not free storage.
     To clear only a suffix, use pop_back_many() below.
     It is customary to destroy elements in reverse index order.
     However, this does not really guarantee elements are always
     destroyed in reverse order of constructions. This is because
     insert() and emplace() may construct elements out-of-order.
     See below.

     This functions does not require is_destructible_v, but panics if it needs to destruct elements.
     Hence, if is_destructible_v == false, it is equivalent to checking that the user has indeed destructed all existing elements.
   */
  constexpr void clear () {
    if (len > 0) {
      if constexpr (minilib::is_destructible_v < T >) {
	/* minilib::destroy_at already checks is_trivially_destructible_v.
	   Therefore, if T is trivially destructible, this loop is a no-op and will be optimized away.
	   We do not check it again here.
	 */
	for (size_t i = len; i >= 1; i--) minilib::destroy_at < T > (ptr + (i - 1));
	len = 0;
      } else {
	std::terminate ();
      }
    }
  }

  /* Resize storage space, moving and destructing elements if necessary. */
  constexpr void resize_storage (size_t final_size) {
#ifdef CONFIG_DEFENSIVE
    if (__builtin_mul_overflow_p (sizeof (T), final_size, 0ull)) std::terminate ();
#endif

    if (alloc_len == 0) {

      if (final_size == 0) return;
      ptr = minilib::allocator < T > :: allocate (final_size);
      if (ptr == nullptr) std::terminate ();
      alloc_len = final_size;
      return;

    } else if (len == 0) {

      /* If len == 0 we don't have to move or destruct anything */
      minilib::allocator < T > :: deallocate (ptr, alloc_len);
      if (final_size == 0) { alloc_len = 0; ptr = nullptr; return; }
      ptr = minilib::allocator < T > :: allocate (final_size);
      if (ptr == nullptr) std::terminate ();
      alloc_len = final_size;
      return;
      
    } else if (final_size == 0) {

      /* If final_size == 0 we only have to destruct elements, not move elements */
      if constexpr (minilib::is_destructible_v < T >) {
	for (size_t i = len; i > 0; i--) minilib::destroy_at < T > (ptr + (i - 1));
	minilib::allocator < T > :: deallocate (ptr, alloc_len);
	alloc_len = 0;
	len = 0;
	ptr = nullptr;
	return;
      } else {
	std::terminate ();
      }

    } else if constexpr (minilib::is_move_constructible_v < T > && minilib::is_destructible_v < T >) {

      T * new_ptr = minilib::allocator < T > :: allocate (final_size);
      if (new_ptr == nullptr) std::terminate ();
      size_t copy_range = minilib::min (len, final_size);

      if ! consteval {
	if constexpr (minilib::is_trivially_move_constructible_v < T >) {
	  __builtin_memcpy (new_ptr, ptr, sizeof (T) * copy_range);
	  minilib::allocator < T > :: deallocate (ptr, alloc_len);
	  ptr = new_ptr;
	  alloc_len = final_size;
	  len = copy_range;
	  return;
	}
      }

      for (size_t i = 0; i < copy_range; i++) minilib::construct_at < T > (new_ptr + i, minilib::move (ptr[i]));
      for (size_t i = len; i >= 1; i--) minilib::destroy_at < T > (ptr + (i - 1));
      minilib::allocator < T > :: deallocate (ptr, alloc_len);
      ptr = new_ptr;
      alloc_len = final_size;
      len = copy_range;

    } else {

      /* We have to move and destruct existing elements, but we can't, so panic. */
      std::terminate ();

    }
  }

  constexpr ~vector () { resize_storage (0); }

  constexpr void reserve (size_t size) {
    if (size <= alloc_len) return;
    resize_storage (size);
  }

  /* Copy elements as if existing elements are destroyed and new elements copy-constructed.
     Recall that we assume: if T provides both copy-assignment, copy constructor, and destructor,
     then destroying existing element and copy construct new element in-place is logically the same as copy-assign the new element to the old element.
     We thus try to exploit is_trivially_copy_assignable where possible.
     However, if is_trivially_copy_constructible == true, then T has trivial destructor as well.
     In that case, by the above assumption, memcpy has the same logical effect as copy-assignment even if is_trivially_copy_assignable == false.
     We thus prefer memcpy in that case.
     Recall that if is_trivially_copy_constructible == true, then T is implicit-lifetime type,
     and memcpy can be seen as destroying original object and copy-construct a new one in-place.
     This function does not require T to be destructible,
     but if we need to destruct elements and T is not destructible, we abort.
  */
  constexpr vector& operator= (const vector& other) requires (minilib::is_copy_constructible_v < T >) {
    if (this == minilib::addressof (other)) return *this;

    /* This code contains the following cases:
       1. other.len <= alloc_len, hence no re-allocation is necessary.
	  1.1. T is trivially copy-constructible and trivially copy-assignable, then we just do memcpy.
	  1.2. T is not trivially copyable, but is copy-assignable, then try to do assignment where possible.
	  1.3. T is neither trivially copyable nor copy-assignable, then fallback to case 2 below.
       2. other.len > alloc_len, hence a re-allocation is necessary. Destroy all existing elements and call resize_storage.
	  2.1. T is trivially copyable. Then we just do memcpy.
	  2.2. T is not trivially copyable, then copy-construct all elements.
     */

    if (other.len <= alloc_len) {
      if ! consteval {
	/* As a slight optimization, if other.len <= len we don't need trivial copy constructor because there is nothing to construct. */
	if constexpr (minilib::is_trivially_copy_assignable_v < T > || minilib::is_trivially_copy_constructible_v < T >) { /* Case 1.1 */
	  if (minilib::is_trivially_copy_constructible_v < T > || other.len <= len) {
	    if (other.len == 0) { len = 0; return *this; }
	    __builtin_memcpy (ptr, other.ptr, sizeof (T) * other.len);
	    len = other.len;
	    return *this;
	  }
	}
      }

      if constexpr (minilib::is_copy_assignable_v < T >) { /* Case 1.2 */
	size_t assign_range = minilib::min (len, other.len);

	if ! consteval {
	  if constexpr (minilib::is_trivially_copy_assignable_v < T >) {
	    /* If we enter this branch, T is a strange type that is trivially copy assignable but not trivially copy constructible.
	       Otherwise it would have been handled by case 1.1 above.
	       Weird, but we try to support memcpy optimization anyway.
	     */
	    if (assign_range == 0) goto after_assign;
	    __builtin_memcpy (ptr, other.ptr, sizeof (T) * assign_range);
	    goto after_assign;
	  }
	}

	for (size_t i = 0; i < assign_range; i++) ptr[i] = other.ptr[i];

[[maybe_unused]] after_assign:
	if (len < other.len) { /* Hence assign_range == len */
	  if ! consteval {
	    if constexpr (minilib::is_trivially_copy_constructible_v < T >) {
	      __builtin_memcpy (ptr + len, other.ptr + len, sizeof (T) * (other.len - len));
	      goto after_handle_tail;
	    }
	  }

	  for (size_t i = assign_range; i < other.len; i++) minilib::construct_at < T > (ptr + i, other.ptr[i]);
	}

	if (len > other.len) {
	  if constexpr (minilib::is_destructible_v < T >) {
	    /* Again, minilib::destroy_at already handles is_trivially_destructible case */
	    for (size_t i = len; i > assign_range; i--) minilib::destroy_at < T > (ptr + (i - 1));
	  } else {
	    std::terminate ();
	  }
	}

[[maybe_unused]] after_handle_tail:
	len = other.len;
	return *this;
      }
    }

    /* Case 1.3 + Case 2 */
    clear ();
    /* If we come from case 1.3 then we don't need to re-allocate. */
    if (other.len > alloc_len) resize_storage (other.len);
    len = other.len;
    if (len == 0) return *this;

    if ! consteval {
      if constexpr (minilib::is_trivially_copy_constructible_v < T >) {
	__builtin_memcpy (ptr, other.ptr, sizeof (T) * other.len);
	return *this;
      }
    }

    for (size_t i = 0; i < other.len; i++) minilib::construct_at < T > (ptr + i, other.ptr[i]);
    return *this;
  }

  constexpr vector& operator= (vector&& other) {
    if (this == minilib::addressof (other)) return *this;
    resize_storage (0);
    alloc_len = other.alloc_len;
    len = other.len;
    ptr = other.ptr;
    other.alloc_len = 0;
    other.len = 0;
    other.ptr = nullptr;
    return *this;
  }

private:
  /* The following functions exist to support push_back/emplace_back/generate/insert/emplace/generate_at functions. They are not public.
     If an external caller is interested in doing the same thing, just copy the code.
   */

  /* Given the number of new slots to allocate, recommend a new allocation size.
     Since this is a private function, we shall assume the caller has already checked alloc_len - len < expand_len.
   */
  constexpr size_t compute_expanded_size (size_t expand_len) {
#ifdef CONFIG_DEFENSIVE
    size_t expected_size;
    if (__builtin_uaddl_overflow (len, expand_len, &expected_size)) std::terminate ();
#else
    size_t expected_size = len + expand_len;
#endif

    /* Let's assume a typical element of vector has 4 bytes.
       Then 4 * 8 = 32, the minimal size class supported by our malloc.
       Also, any typical use of vector would probably store at least 8 elements.
       For special situations (including when sizeof (T) is very large),
       the caller can exercise fine control using resize_storage().
     */
    if (expected_size <= 8) expected_size = 8;

    /* We follow libstdc++ and use expansion factor 2. Clang uses 1.5.
       It is unlikely that alloc_len * 2 will overflow.
       The virtual address space would have been exhausted long before this limit.
     */
#ifdef CONFIG_DEFENSIVE
    if (alloc_len >= (1ull << 63)) std::terminate ();
#endif

    /* If (2 * alloc_len * sizeof (T)) overflows, and we are defensive, later the check in resize_storage() will catch it */
    return minilib::max (expected_size, 2 * alloc_len);
  }

  constexpr T * allocate_raw_array (size_t size) {
#ifdef CONFIG_DEFENSIVE
    if (__builtin_mul_overflow_p (sizeof (T), size, 0ull)) std::terminate ();
#endif

    return minilib::allocator < T > :: allocate (size);
  }

  constexpr void move_vector_to_array_and_destroy (T * dst) requires (minilib::is_move_constructible_v < T > && minilib::is_destructible_v < T >) {
    if ! consteval {
      if constexpr (minilib::is_trivially_move_constructible_v < T >) {
	/* is_trivially_move_constructible also implies is_trivially_destructible */
	if (len == 0) return;
	__builtin_memcpy (dst, ptr, sizeof (T) * len);
	return;
      }
    }

    for (size_t i = 0; i < len; i++) {
      minilib::construct_at < T > (dst + i, minilib::move (ptr[i]));
      minilib::destroy_at < T > (ptr + i);
    }
  }

  /* The caller should have checked index <= len */
  constexpr void move_vector_to_array_with_shift_and_destroy (T * dst, size_t index, size_t shift) requires (minilib::is_move_constructible_v < T > && minilib::is_destructible_v < T >) {
    if ! consteval {
      if constexpr (minilib::is_trivially_move_constructible_v < T >) {
	if (index > 0) __builtin_memcpy (dst, ptr, sizeof (T) * index);
	if (index < len) __builtin_memcpy (dst + index + shift, ptr + index, sizeof (T) * (len - index));
	return;
      }
    }

    for (size_t i = 0; i < index; i++) {
      minilib::construct_at < T > (dst + i, minilib::move (ptr[i]));
      minilib::destroy_at < T > (ptr + i);
    }
    for (size_t i = index; i < len; i++) {
      minilib::construct_at < T > (dst + i + shift, minilib::move (ptr[i]));
      minilib::destroy_at < T > (ptr + i);
    }
  }

public:
  /* is_copy_constructible implies is_move_constructible. */
  constexpr void push_back (const T& t) requires (minilib::is_copy_constructible_v < T >) {
    /* If len < alloc_len, no re-allocation necessary */
    if (len < alloc_len) {
      minilib::construct_at < T > (ptr + len, t);
      len++;
      return;
    }

    /* An extremely subtle case of push_back is that t might be an element within the current vector.
       If a re-allocation occurs, the reference will be invalidated.
       Therefore, allocate a new raw array, and copy t to the end.
       Then move-construct existing elements to the new array.
       Finally, destroy existing elements, free storage, and move the new storage to ourselves.
     */
    if constexpr (minilib::is_destructible_v < T >) {
      size_t new_alloc_len = compute_expanded_size (1);
      T * new_ptr = allocate_raw_array (new_alloc_len);
      minilib::construct_at < T > (new_ptr + len, t);
      move_vector_to_array_and_destroy (new_ptr);
      if (ptr != nullptr) minilib::allocator < T > :: deallocate (ptr, alloc_len);
      alloc_len = new_alloc_len;
      ptr = new_ptr;
      len++;
    } else {
      std::terminate ();
    }
  }

  constexpr void push_back (T&& t) requires (minilib::is_move_constructible_v < T >) {
    if (len < alloc_len) {
      minilib::construct_at < T > (ptr + len, minilib::move (t));
      len++;
      return;
    }

    if constexpr (minilib::is_destructible_v < T >) {
      size_t new_alloc_len = compute_expanded_size (1);
      T * new_ptr = allocate_raw_array (new_alloc_len);
      minilib::construct_at < T > (new_ptr + len, minilib::move (t));
      move_vector_to_array_and_destroy (new_ptr);
      if (ptr != nullptr) minilib::allocator < T > :: deallocate (ptr, alloc_len);
      alloc_len = new_alloc_len;
      ptr = new_ptr;
      len++;
    } else {
      std::terminate ();
    }
  }

  constexpr void push_back_many (const T& t, size_t count) requires (minilib::is_copy_constructible_v < T >) {
    if (alloc_len - len >= count) {
      for (size_t i = len; i < len + count; i++) minilib::construct_at < T > (ptr + i, t);
      len += count;
      return;
    }

    if constexpr (minilib::is_destructible_v < T >) {
      size_t new_alloc_len = compute_expanded_size (count);
      T * new_ptr = allocate_raw_array (new_alloc_len);
      for (size_t i = 0; i < count; i++) minilib::construct_at < T > (new_ptr + len + i, t);
      move_vector_to_array_and_destroy (new_ptr);
      if (ptr != nullptr) minilib::allocator < T > :: deallocate (ptr, alloc_len);
      alloc_len = new_alloc_len;
      ptr = new_ptr;
      len += count;
    } else {
      std::terminate ();
    }
  }

  /* For emplace_back(), emplace_back_many(), generate(), we promise to construct the elements in-place. */
  template < typename... Args >
  requires (minilib::is_constructible_v < T, Args&&... >)
  constexpr void emplace_back (Args&&... args) {
    if (len < alloc_len) {
      minilib::construct_at < T > (ptr + len, minilib::forward < Args > (args)...);
      len++;
      return;
    }

    if constexpr (minilib::is_destructible_v < T >) {
      size_t new_alloc_len = compute_expanded_size (1);
      T * new_ptr = allocate_raw_array (new_alloc_len);
      minilib::construct_at < T > (new_ptr + len, minilib::forward < Args > (args)...);
      move_vector_to_array_and_destroy (new_ptr);
      if (ptr != nullptr) minilib::allocator < T > :: deallocate (ptr, alloc_len);
      alloc_len = new_alloc_len;
      ptr = new_ptr;
      len++;
    } else {
      std::terminate ();
    }
  }

  /* DANGEROUS: If default-initialization leaves some members uninitialized,
     and any later operation reads this value, including internal re-allocation,
     the behavior is undefined because we are copying uninitialized values.
     The caller must promise to immediately initialize these members.
   */
  constexpr void emplace_back_default () requires (minilib::is_truly_default_constructible_v < T >) {
    if (len < alloc_len) {
      minilib::default_construct_at < T > (ptr + len);
      len++;
      return;
    }

    if constexpr (minilib::is_destructible_v < T >) {
      size_t new_alloc_len = compute_expanded_size (1);
      T * new_ptr = allocate_raw_array (new_alloc_len);
      minilib::default_construct_at < T > (new_ptr + len);
      move_vector_to_array_and_destroy (new_ptr);
      if (ptr != nullptr) minilib::allocator < T > :: deallocate (ptr, alloc_len);
      alloc_len = new_alloc_len;
      ptr = new_ptr;
      len++;
    } else {
      std::terminate ();
    }
  }

  template < typename... Args >
  requires (minilib::is_constructible_v < T, const Args&... >)
  constexpr void emplace_back_many (size_t count, const Args&... args) {
    if (alloc_len - len >= count) {
      for (size_t i = len; i < len + count; i++) minilib::construct_at < T > (ptr + i, args...);
      len += count;
      return;
    }

    if constexpr (minilib::is_destructible_v < T >) {
      size_t new_alloc_len = compute_expanded_size (count);
      T * new_ptr = allocate_raw_array (new_alloc_len);
      for (size_t i = 0; i < count; i++) minilib::construct_at < T > (new_ptr + len + i, args...);
      move_vector_to_array_and_destroy (new_ptr);
      if (ptr != nullptr) minilib::allocator < T > :: deallocate (ptr, alloc_len);
      alloc_len = new_alloc_len;
      ptr = new_ptr;
      len += count;
    } else {
      std::terminate ();
    }
  }

  constexpr void emplace_back_many_default (size_t count) requires (minilib::is_truly_default_constructible_v < T >) {
    if (alloc_len - len >= count) {
      for (size_t i = len; i < len + count; i++) minilib::default_construct_at < T > (ptr + i);
      len += count;
      return;
    }

    if constexpr (minilib::is_destructible_v < T >) {
      size_t new_alloc_len = compute_expanded_size (count);
      T * new_ptr = allocate_raw_array (new_alloc_len);
      for (size_t i = 0; i < count; i++) minilib::default_construct_at < T > (new_ptr + len + i);
      move_vector_to_array_and_destroy (new_ptr);
      if (ptr != nullptr) minilib::allocator < T > :: deallocate (ptr, alloc_len);
      alloc_len = new_alloc_len;
      ptr = new_ptr;
      len += count;
    } else {
      std::terminate ();
    }
  }

  /* DANGEROUS: Tells the container that the caller has directly called placement new on uninitialized elements.
     The intention is to cover initialization patterns that construct_at() and default_construct_at() could not cover.
     Do not use this unless you know what you are doing.
   */
  constexpr void inform_emplace_back_many (size_t count) {
    if (count > alloc_len - len) std::terminate ();
    len += count;
  }

  template < typename... ArgGenerators >
  requires (minilib::is_constructible_v < T, std::invoke_result_t < ArgGenerators, size_t > ... >)
  constexpr void generate (size_t count, ArgGenerators&&... gs) {
    if (alloc_len - len >= count) {
      for (size_t i = 0; i < count; i++) minilib::construct_at < T > (ptr + len + i, gs (i)...);
      len += count;
      return;
    }

    if constexpr (minilib::is_destructible_v < T >) {
      size_t new_alloc_len = compute_expanded_size (count);
      T * new_ptr = allocate_raw_array (new_alloc_len);
      for (size_t i = 0; i < count; i++) minilib::construct_at < T > (new_ptr + len + i, gs (i)...);
      move_vector_to_array_and_destroy (new_ptr);
      if (ptr != nullptr) minilib::allocator < T > :: deallocate (ptr, alloc_len);
      alloc_len = new_alloc_len;
      ptr = new_ptr;
      len += count;
    } else {
      std::terminate ();
    }
  }

  constexpr void pop_back () requires (minilib::is_destructible_v < T >) {
    if (len == 0) std::terminate ();
    minilib::destroy_at < T > (ptr + (len - 1));
    len--;
  }

  constexpr void pop_back_many (size_t n) requires (minilib::is_destructible_v < T >) {
    if (len < n) std::terminate ();

    /* If T is trivially destructible, we expect the compiler to optimize this to len -= n; */
    while (n) {
      minilib::destroy_at < T > (ptr + (len - 1));
      len--; n--;
    }
  }

  /* DANGEROUS: The converse of inform_emplace_back_many */
  constexpr void inform_pop_back_many (size_t n) {
    if (n > len) std::terminate ();
    len -= n;
  }

  constexpr T& operator[] (size_t index) {
    if (index >= len) std::terminate ();
    return ptr[index];
  }

  constexpr const T& operator[] (size_t index) const {
    if (index >= len) std::terminate ();
    return ptr[index];
  }

  constexpr T * data () { return ptr; }

  constexpr const T * data () const { return ptr; }

  constexpr size_t size () const { return len; }

  constexpr size_t capacity () const { return alloc_len; }

  constexpr explicit operator bool () const { return len != 0; }

  /* We deliberately do not implement iterators. */

  /* shift_by() creates a gap [index, index + shift). The caller must fill the gap immediately.
     The caller must ensure index <= len and len + shift does not overflow (unlikely).

     The caller must be aware that this function will re-allocate if alloc_len - len < + shift, which requires T to be destructible.
     If re-allocation happens, elements in the range [index, index + shift) are guaranteed to be destroyed.

     If re-allocation DOES NOT happen:
     - If is_destructible == true && no_destruct == false, elements in the range [index, index + shift) are guaranteed to be destroyed. (Default)
     - If is_destructible == false || no_destruct == true, elements in the range [index, index + shift) are NOT destroyed,
       and it is left to the caller to handle.

     The minimal requirement we need on this function is move_constructible.
     If index + shift >= len and len + shift <= alloc_len, this is the only requirement we need.
     Otherwise, we also need to overwrite existing elements.
     In this case we need T to be either move-assignable or destructible.
     If we need to overwrite but T provides neither option, we panic.

     DANGEROUS: We leave this function public to cover cases where the caller may want to manually call placement new on the elements within the gap.
     This is to cover initialization patterns that construct_at() and default_construct_at() could not cover.
     Do not do this unless you know what you are doing.
   */
  constexpr void shift_by (size_t index, size_t shift, bool no_destruct = false) requires (minilib::is_move_constructible_v < T >) {
    if (index > len) std::terminate ();
    if (shift == 0) return;

    if (alloc_len - len < shift) {

      size_t new_alloc_len = compute_expanded_size (shift);
      T * new_ptr = allocate_raw_array (new_alloc_len);

      if (len == 0) {
	if (ptr != nullptr) minilib::allocator < T > :: deallocate (ptr, alloc_len);
	alloc_len = new_alloc_len;
	ptr = new_ptr;
	len = shift;
	return;
      } else if constexpr (minilib::is_destructible_v < T >) {
	move_vector_to_array_with_shift_and_destroy (new_ptr, index, shift);
	if (ptr != nullptr) minilib::allocator < T > :: deallocate (ptr, alloc_len);
	alloc_len = new_alloc_len;
	ptr = new_ptr;
	len += shift;
	return;
      } else {
	std::terminate ();
      }

    } else {

      if (len == 0 || index == len) { len += shift; return; }
      /* Hence index < len, shift > 0, and len > 0 below */

      /* If T is trivially move constructible, use memmove to do the shift. Destructor of T is trivial. */
      if ! consteval {
	if constexpr (minilib::is_trivially_move_constructible_v < T >) {
	  __builtin_memmove (ptr + index + shift, ptr + index, sizeof (T) * (len - index));
	  len += shift;
	  return;
	}
      }

      /* Otherwise, do the destroy/construct loop, but exploit is_move_assignable and is_trivially_move_assignable where possible. */
      if (index + shift < len) {

	for (size_t i = len + shift - 1; i >= len; i--) minilib::construct_at < T > (ptr + i, minilib::move (ptr[i - shift]));

	if constexpr (minilib::is_move_assignable_v < T >) {

	  if ! consteval {
	    if constexpr (minilib::is_trivially_move_assignable_v < T >) {
	      __builtin_memmove (ptr + (index + shift), ptr + index, sizeof (T) * (len - (index + shift)));
	      goto after_move;
	    }
	  }

	  for (size_t i = len - 1; i >= index + shift; i--) ptr[i] = minilib::move (ptr[i - shift]);

	} else if constexpr (minilib::is_destructible_v < T >) {

	  for (size_t i = len - 1; i >= index + shift; i--) {
	    minilib::destroy_at < T > (ptr + i);
	    minilib::construct_at < T > (ptr + i, minilib::move (ptr[i - shift]));
	  }

	} else {

	  std::terminate ();

	}

      } else {

	for (size_t i = len + shift - 1; i >= index + shift; i--) minilib::construct_at < T > (ptr + i, minilib::move (ptr[i - shift]));

      }

[[maybe_unused]] after_move:
      if constexpr (minilib::is_destructible_v < T >) {
	if (! no_destruct) {
	  size_t destroy_range = minilib::min (index + shift, len);
	  for (size_t i = index; i < destroy_range; i++) minilib::destroy_at < T > (ptr + i);
	}
      }

      len += shift;
      return;

    }
  }

  /* The functions insert/emplace/generate_at clearly present special difficulties.
     As described in the comments of push_back(), it is possible that some of the arguments reference existing elements.
     In such cases, invalidating the reference before constructing the object would be UB.
     However, since these functions insert objects in the middle, it is unavoidable that we have to invalidate some references.
     For this reason, we split each of these functions into two versions called X_no_invalidate and X_may_invalidate:
     - X_no_invalidate guarantees not to invalidate any reference before constructing the object.
       However, insert_no_invalidate() will unconditionally create an on-stack copy of the value.
       Similarly, emplace_no_invalidate() and generate_at_no_invalidate() will unconditionally re-allocate.
     - X_may_invalidate may invalidate array references before constructing the object.
       Calling these functions with potentially-invalidated references is UB.
       However, these functions avoid creating on-stack copies and unnecessary re-allocations.
   */

  constexpr void insert_no_invalidate (size_t index, const T& t) requires (minilib::is_copy_constructible_v < T > && minilib::is_destructible_v < T >) {
    if (index > len) std::terminate ();
    if (index == len) { push_back (t); return; }

    T t_ (t);
    if (alloc_len > len) {
      shift_by (index, 1, true);
      if constexpr (minilib::is_move_assignable_v < T >) {
	ptr[index] = minilib::move (t_);
      } else {
	minilib::destroy_at < T > (ptr + index);
	minilib::construct_at < T > (ptr + index, minilib::move (t_));
      }
    } else {
      shift_by (index, 1);
      minilib::construct_at < T > (ptr + index, minilib::move (t_));
    }
  }

  constexpr void insert_no_invalidate (size_t index, T&& t) requires (minilib::is_move_constructible_v < T > && minilib::is_destructible_v < T >) {
    if (index > len) std::terminate ();
    if (index == len) { push_back (minilib::move (t)); return; }

    T t_ (minilib::move (t));
    if (alloc_len > len) {
      shift_by (index, 1, true);
      if constexpr (minilib::is_move_assignable_v < T >) {
	ptr[index] = minilib::move (t_);
      } else {
	minilib::destroy_at < T > (ptr + index);
	minilib::construct_at < T > (ptr + index, minilib::move (t_));
      }
    } else {
      shift_by (index, 1);
      minilib::construct_at < T > (ptr + index, minilib::move (t_));
    }
  }

  constexpr void insert_may_invalidate (size_t index, const T& t) requires (minilib::is_copy_constructible_v < T > && minilib::is_destructible_v < T >) {
    if (index > len) std::terminate ();
    if (index == len) { push_back (t); return; }

    if (alloc_len > len) {
      shift_by (index, 1, true);
      if constexpr (minilib::is_copy_assignable_v < T >) {
	ptr[index] = t;
      } else {
	minilib::destroy_at < T > (ptr + index);
	minilib::construct_at < T > (ptr + index, t);
      }
    } else {
      shift_by (index, 1);
      minilib::construct_at < T > (ptr + index, t);
    }
  }

  constexpr void insert_may_invalidate (size_t index, T&& t) requires (minilib::is_move_constructible_v < T > && minilib::is_destructible_v < T >) {
    if (index > len) std::terminate ();
    if (index == len) { push_back (minilib::move (t)); return; }

    if (alloc_len > len) {
      shift_by (index, 1, true);
      if constexpr (minilib::is_move_assignable_v < T >) {
	ptr[index] = minilib::move (t);
      } else {
	minilib::destroy_at < T > (ptr + index);
	minilib::construct_at < T > (ptr + index, minilib::move (t));
      }
    } else {
      shift_by (index, 1);
      minilib::construct_at < T > (ptr + index, minilib::move (t));
    }
  }

  constexpr void insert_many_no_invalidate (size_t index, size_t count, const T& t) requires (minilib::is_copy_constructible_v < T > && minilib::is_destructible_v < T >) {
    if (index > len) std::terminate ();
    if (count == 0) return;
    if (index == len) { push_back_many (t, count); return; }

    T t_ (t);
    size_t old_len = len;
    if (alloc_len - len >= count) {
      shift_by (index, count, true);
      if (index + count <= old_len) {
	if constexpr (minilib::is_copy_assignable_v < T >) {
	  for (size_t i = index; i < index + count; i++) ptr[i] = t_;
	} else {
	  for (size_t i = index; i < index + count; i++) {
	    minilib::destroy_at < T > (ptr + i);
	    minilib::construct_at < T > (ptr + i, t_);
	  }
	}
      } else {
	if constexpr (minilib::is_copy_assignable_v < T >) {
	  for (size_t i = index; i < old_len; i++) ptr[i] = t_;
	} else {
	  for (size_t i = index; i < old_len; i++) {
	    minilib::destroy_at < T > (ptr + i);
	    minilib::construct_at < T > (ptr + i, t_);
	  }
	}
	for (size_t i = old_len; i < index + count; i++) minilib::construct_at < T > (ptr + i, t_);
      }
    } else {
      shift_by (index, count);
      for (size_t i = index; i < index + count; i++) minilib::construct_at < T > (ptr + i, t_);
    }
  }

  constexpr void insert_many_may_invalidate (size_t index, size_t count, const T& t) requires (minilib::is_copy_constructible_v < T > && minilib::is_destructible_v < T >) {
    if (index > len) std::terminate ();
    if (count == 0) return;
    if (index == len) { push_back_many (t, count); return; }

    size_t old_len = len;
    if (alloc_len - len >= count) {
      shift_by (index, count, true);
      if (index + count <= old_len) {
	if constexpr (minilib::is_copy_assignable_v < T >) {
	  for (size_t i = index; i < index + count; i++) ptr[i] = t;
	} else {
	  for (size_t i = index; i < index + count; i++) {
	    minilib::destroy_at < T > (ptr + i);
	    minilib::construct_at < T > (ptr + i, t);
	  }
	}
      } else {
	if constexpr (minilib::is_copy_assignable_v < T >) {
	  for (size_t i = index; i < old_len; i++) ptr[i] = t;
	} else {
	  for (size_t i = index; i < old_len; i++) {
	    minilib::destroy_at < T > (ptr + i);
	    minilib::construct_at < T > (ptr + i, t);
	  }
	}
	for (size_t i = old_len; i < index + count; i++) minilib::construct_at < T > (ptr + i, t);
      }
    } else {
      shift_by (index, count);
      for (size_t i = index; i < index + count; i++) minilib::construct_at < T > (ptr + i, t);
    }
  }

  template < typename... Args >
  requires (minilib::is_move_constructible_v < T > && minilib::is_destructible_v < T > && minilib::is_constructible_v < T, Args&&... >)
  constexpr void emplace_no_invalidate (size_t index, Args&&... args) {
    if (index > len) std::terminate ();
    if (index == len) { emplace_back (minilib::forward < Args > (args)...); return; }

    size_t new_alloc_len = compute_expanded_size (1);
    T * new_ptr = allocate_raw_array (new_alloc_len);
    minilib::construct_at < T > (new_ptr + index, minilib::forward < Args > (args)...);
    move_vector_to_array_with_shift_and_destroy (new_ptr, index, 1);
    minilib::allocator < T > :: deallocate (ptr, alloc_len);
    alloc_len = new_alloc_len;
    ptr = new_ptr;
    len++;
  }

  template < typename... Args >
  requires (minilib::is_move_constructible_v < T > && minilib::is_destructible_v < T > && minilib::is_constructible_v < T, Args&&... >)
  constexpr void emplace_may_invalidate (size_t index, Args&&... args) {
    if (index > len) std::terminate ();
    if (index == len) { emplace_back (minilib::forward < Args > (args)...); return; }
    shift_by (index, 1);
    minilib::construct_at < T > (ptr + index, minilib::forward < Args > (args)...);
  }

  /* Since emplace_default takes no arguments, it does not need no_invalidate/may_invalidate versions */
  constexpr void emplace_default (size_t index) requires (minilib::is_move_constructible_v < T > && minilib::is_destructible_v < T > && minilib::is_truly_default_constructible_v < T >) {
    if (index > len) std::terminate ();
    if (index == len) { emplace_back_default (); return; }
    shift_by (index, 1);
    minilib::default_construct_at < T > (ptr + index);
  }

  template < typename... Args >
  requires (minilib::is_move_constructible_v < T > && minilib::is_destructible_v < T > && minilib::is_constructible_v < T, const Args&... >)
  constexpr void emplace_many_no_invalidate (size_t index, size_t count, const Args&... args) {
    if (index > len) std::terminate ();
    if (count == 0) return;

    size_t new_alloc_len = compute_expanded_size (count);
    T * new_ptr = allocate_raw_array (new_alloc_len);
    for (size_t i = index; i < index + count; i++) minilib::construct_at < T > (new_ptr + i, args...);
    move_vector_to_array_with_shift_and_destroy (new_ptr, index, count);
    if (ptr != nullptr) minilib::allocator < T > :: deallocate (ptr, alloc_len);
    alloc_len = new_alloc_len;
    ptr = new_ptr;
    len += count;
  }

  template < typename... Args >
  requires (minilib::is_move_constructible_v < T > && minilib::is_destructible_v < T > && minilib::is_constructible_v < T, const Args&... >)
  constexpr void emplace_many_may_invalidate (size_t index, size_t count, const Args&... args) {
    if (index > len) std::terminate ();
    if (count == 0) return;
    shift_by (index, count);
    for (size_t i = index; i < index + count; i++) minilib::construct_at < T > (ptr + i, args...);
  }

  constexpr void emplace_many_default (size_t index, size_t count) requires (minilib::is_move_constructible_v < T > && minilib::is_destructible_v < T > && minilib::is_truly_default_constructible_v < T >) {
    if (index > len) std::terminate ();
    if (count == 0) return;
    shift_by (index, count);
    for (size_t i = index; i < index + count; i++) minilib::default_construct_at < T > (ptr + i);
  }

  template < typename... ArgGenerators >
  requires (minilib::is_move_constructible_v < T > && minilib::is_destructible_v < T > && minilib::is_constructible_v < T, std::invoke_result_t < ArgGenerators, size_t > ... >)
  constexpr void generate_at_no_invalidate (size_t index, size_t count, ArgGenerators&&... gs) {
    if (index > len) std::terminate ();
    if (count == 0) return;

    size_t new_alloc_len = compute_expanded_size (count);
    T * new_ptr = allocate_raw_array (new_alloc_len);
    for (size_t i = index; i < index + count; i++) minilib::construct_at < T > (new_ptr + i, gs (i - index)...);
    move_vector_to_array_with_shift_and_destroy (new_ptr, index, count);
    if (ptr != nullptr) minilib::allocator < T > :: deallocate (ptr, alloc_len);
    alloc_len = new_alloc_len;
    ptr = new_ptr;
    len += count;
  }

  template < typename... ArgGenerators >
  requires (minilib::is_move_constructible_v < T > && minilib::is_destructible_v < T > && minilib::is_constructible_v < T, std::invoke_result_t < ArgGenerators, size_t > ... >)
  constexpr void generate_at_may_invalidate (size_t index, size_t count, ArgGenerators&&... gs) {
    if (index > len) std::terminate ();
    if (count == 0) return;
    shift_by (index, count);
    for (size_t i = index; i < index + count; i++) minilib::construct_at < T > (ptr + i, gs (i - index)...);
  }

  constexpr void erase (size_t index, size_t count) requires (minilib::is_destructible_v < T > && (minilib::is_move_assignable_v < T > || minilib::is_move_constructible_v < T >)) {
    if (index > len || count > len - index) std::terminate ();
    if (count == 0) return;

    /* If T is trivially copy/move assignable, its destructor is also trivial. */
    if ! consteval {
      if constexpr (minilib::is_trivially_move_assignable_v < T >) {
	__builtin_memmove (ptr + index, ptr + index + count, sizeof (T) * (len - (index + count)));
	len -= count;
	return;
      }
    }

    if constexpr (minilib::is_move_assignable_v < T >) {
      for (size_t i = index; i < len - count; i++) ptr[i] = minilib::move (ptr[i + count]);
    } else {
      for (size_t i = index; i < len - count; i++) {
	minilib::destroy_at < T > (ptr + i);
	minilib::construct_at < T > (ptr + i, minilib::move (ptr[i + count]));
      }
    }
    for (size_t i = len; i > len - count; i--) minilib::destroy_at < T > (ptr + (i - 1));
    len -= count;
  }

  constexpr void erase (size_t index) requires (minilib::is_destructible_v < T > && (minilib::is_move_assignable_v < T > || minilib::is_move_constructible_v < T >)) { erase (index, 1); }

  constexpr bool operator== (const vector& other)
  requires (minilib::equality_comparable < T >)
  {
    if (len != other.len) return false;
    for (size_t i = 0; i < len; i++) {
      if ((static_cast < const T& > (ptr[i]) == static_cast < const T& > (other.ptr[i])) == false) return false;
    }
    return true;
  }

  /* Lexicographic ordering */
  friend constexpr minilib::order_result compare_three_way (const vector& a, const vector& b)
  requires (minilib::three_way_comparable < T >)
  {
    size_t len_a = a.size (), len_b = b.size ();
    size_t i = 0;

    while (i < len_a && i < len_b) {
      minilib::order_result cmp = minilib::compare_three_way::operator() (static_cast < const T& > (a[i]), static_cast < const T& > (b[i]));
      if (cmp < 0) return minilib::order_result::less ();
      else if (cmp > 0) return minilib::order_result::greater ();
      i++;
    }

    return minilib::compare_three_way::operator() (len_a, len_b);
  }
};

}

#endif
