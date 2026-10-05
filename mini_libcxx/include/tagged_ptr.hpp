/* Tagged pointers and tagged references

   General idea: One of the trickiest issue in C++ programming is dangling references.
   If we accidentally dereference a dangling pointer/reference, we trigger UB.
   However, when we dereference a pointer there is no general way to tell whether it is dangling or not.
   This leads to the general principle of not storing references around, and retrieving them from containers only when necessary.

   However, storing references can still sometimes be useful, especially for complex tree and graph structures.
   One example is red-black tree-based set and map containers.
   If we force the user to query the container for a reference for every access,
   then every access incurs an O(log n) cost, and it is not cache-friendly.
   If we allow the user to store a reference into the container then the access becomes O(1).
   This dilemma plagues every functional programming language.
   For example, OCaml resolves this issue by allowing mutable references.

   We resolve this issue by introducing tagged pointers and tagged references,
   a construct that has the performance of pointers but the semantics of arena handles.
   Here is how it works, using RB-tree as an example. When RB-tree allocates a node it does not call the usual malloc() function.
   Instead it calls aligned_tagged_alloc(), our custom extension to malloc(), with the following signature:

   void * aligned_tagged_alloc (size_t alignment, size_t size);

   This function returns a "tagged pointer" that has the following properties:
   * ptr is aligned to 8.
   * (ptr + 16) is aligned to "alignment".
   * The total allocated space is at least (size + 16).
   * The tagged pointer can be freed using free() as usual.

   Hence the initial 16 bytes of the space can be used to store custom metadata, and we use it as follows:
   * The first 8 bytes are used to store an ID of the container that made this allocation (more on this later).
   * The next 8 bytes can be used in two different ways.
     ** The first way is called "counter mode", and it counts how many external references currently exist for this node.
	If an allocation is going to be deallocated, but there are still external references,
	we do not deallocate immediately, but mark the allocation as a "tombstone".
	When the last external reference to this allocation is destroyed, the destructor of the external reference shall deallocate the storage.
	The drawback of this approach is that some unused allocations may not be freed for a long time.
     ** The second way is called "linked list mode". It is basically the head of a linked list of all external references to this node.
	By chaining the references into a list, we can nullify all dangling references to an allocation upon deallocation.
	Thus we circumvent the drawback of the counter mode, and allow allocations to be deallocated immediately upon request.
	The cost is that each reference now takes 24 bytes (8 bytes for a pointer to the allocation, and 16 bytes for prev + next pointers).
	We think this is a reasonable price to pay if immediate deallocation of unused storage is desired.
     ** We differentiate between the two modes by the highest bit of the 8 bytes.
	Since there cannot be (1ull << 60) simultaneous reference to an object, and the highest bits of a usermode address are always 0,
	the highest bits are always unused under both modes, and we can use it as an indicator of modes.
	If bit 63 is set, we are in counter mode, otherwise we are in linked list mode.
	Additionally, under counter mode, if bit 62 is set, this allocation is a tombstone.
   * The actual allocated Node follows this 16-byte metadata.

   When a caller requests a reference to a tree node, we do not return Node& directly.
   Instead it must choose between obtaining two kinds of references:
   * A counted reference, corresponding to the "counter mode" above;
   * A linked reference, corresponding to the "linked list mode" above.

   For any single tagged allocation, at any given moment the existing references to it MUST be of the same type.
   For example, if there is already a counted reference that is not cleared, it cannot request a linked reference.
   Violating this rule leads to an immediate panic.

   Counted references work as follows:
   * Upon creating a counted reference, we set the metadata mode to "counter mode" if it is not already so, and increase the counter by 1.
   * Moving the reference does not affect the counter, but copying and destructing the reference will increase and decrease the counter by 1.
   * Even though we could theoretically convert a reference to Node& (since it already holds the address value), we do not allow so.
   * Rather, external holders of references MUST call the container to convert a reference into an actual pointer.
   * The container will check the metadata, confirming that it is the exact container that handed out this reference.
   * This effectively turns the container into a memory arena, and tagged references are handles into this arena.
   * The container calls tagged_ptr::deallocate() to free an allocation. If there are still dangling counted references,
     the allocation will be marked as a tombstone. Later, when the dangling references are destroyed, the destructor frees the allocation.

   Linked references are similar to counted references, but:
   * Upon creating a linked reference, we add it to the head of the linked list of references.
   * Upon moving the reference, the head, prev, and next pointers must be correspondingly updated.
   * When the container frees an allocation, it crawls the linked list and sets each of them to nullptr.

   Now when we create a container we need to assign it a container ID, which will be the tag of all internal allocations made by this container.
   This tag allows disambiguating same-type handles created by different containers.
   To allocate IDs, one possibility is to use the address of the container object itself.
   This has the advantage of ensuring two contemporary containers never have the same ID.
   However, this has the obvious drawback of not allowing moving containers around,
   since any attempt to move the storage will invalidate all tagged references pointing into the container.
   We adopt a better idea: we let each thread maintain a thread-local counter, and the ID consists of 14 bits of thread ID and 50 bits of counter value.
   We expect applications that create 16384 threads throughout its lifetime are very rare, and we do not expect a 50-bit counter to overflow.
   Even if two containers accidentally get the same ID, properly-written code will still function perfectly,
   but buggy code may confuse tagged references of two containers.
   When a container gets moved, the moved-to object inherits the ID of the original container, while the original object gets a new ID.

   A minor difficulty with the above approach is we cannot have a global counter variable in a constexpr environment.
   Instead, the constexpr function that creates containers must also create a counter object and pass its reference into the container.
   (We cannot just pass in an ID number, because recall that when a container is moved, the moved-from object needs to be assigned a new ID.)
   There are two ways to resolve this issue. The first way is to simply assign the same ID number to every container under constexpr.
   This has the advantage of maintaining an identical interface under both constexpr and non-constexpr.
   However, it also means some buggy code that mixes handles of different containers will not be caught under constexpr.
   The second way is to let the caller create a counter object and pass its reference to the container.
   The tricky problem is to allow the container to store this reference in constexpr context,
   but without paying any extra storage cost at runtime.
   This issue can be resolved as follows:

   Suppose the static storage of container < T > is container_static < T >.
   We define:
   struct container_static_constexpr < T > {
     container_static < T > storage;
     counter_type &ctr; // The global counter object that constexpr users must pass in.
   };

   union container < T > {
     container_static < T > storage_real;
     container_static_constexpr < T > * storage_constexpr; // Notice the pointer!
   };

   In a constexpr environment we strictly only access storage_constexpr; in a non-constexpr environment we strictly only use storage_real,
   and we do not allow leaking constexpr containers into non-constexpr runtime.
   In the constexpr environment, the constructor dynamically allocates container_static_constexpr and stores its pointer.
   Since the container is a union, the storage for this pointer is eclipsed by storage_real.
   We observe that this technique can also be used to circumvent obstacles where the program implicitly relies on some non-constexpr API.

   A further difficulty with using tagged_ptr under constexpr is that, we often want to rely on the global ordering of pointers.
   This allows us to build a red-black tree that uses handles as keys.
   However, under constexpr this is not possible, because pointers cannot be compared under constexpr.
   To resolve this, the constexpr emulation of tagged_ptr has an additional field alloc_id.
   Each container that internally allocates tagged_ptr should maintain an additional counter that assigns alloc_id for each allocation.
   Then pointer comparison will be based on this alloc_id, rather than actual pointer address.
   This counter is only necessary under constexpr, and can be hidden from runtime using the same trick above.
   We do not expect that handles of different containers will be compared. Therefore, a container-local counter is sufficient.

   This code currently works only for single-threaded code.
 */

#ifndef TAGGED_PTR_HPP
#define TAGGED_PTR_HPP

#include <stdint.h>
#include <exception>
#include <type_traits.hpp>
#include <raw_array.hpp>
#include <utility.hpp>
#include <compare.hpp>

namespace minilib {

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class linked_ref;

namespace detail {

/* A constexpr emulation of tagged_ptr */
template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class tagged_ptr_constexpr_impl {
public:
  uint64_t id;
  bool is_counter;
  union { size_t counter; minilib::linked_ref < T > * link_ref_head; };
  uint64_t alloc_id; /* Used to emulate pointer global order under constexpr */
  minilib::raw_array < T, 1 > data;

  /* If this function is called in non-constexpr context, we get a compile-time error. */
  constexpr tagged_ptr_constexpr_impl () {
    if ! consteval {
      minilib::do_not_call_this ();
    }
  }

  tagged_ptr_constexpr_impl (const tagged_ptr_constexpr_impl&) = delete;
  tagged_ptr_constexpr_impl (tagged_ptr_constexpr_impl&&) = delete;
  tagged_ptr_constexpr_impl& operator= (const tagged_ptr_constexpr_impl&) = delete;
  tagged_ptr_constexpr_impl& operator= (tagged_ptr_constexpr_impl&&) = delete;

  constexpr ~tagged_ptr_constexpr_impl () = default;
};

}

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class tagged_ptr;

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class counted_ref {
  union {
    uintptr_t ptr_real;
    minilib::detail::tagged_ptr_constexpr_impl < T > * ptr_constexpr;
  };

public:
  constexpr counted_ref () {
    if consteval {
      ptr_constexpr = nullptr;
    } else {
      ptr_real = 0;
    }
  }

  constexpr counted_ref (const counted_ref& other) {
    if consteval {
      ptr_constexpr = other.ptr_constexpr;
      if (ptr_constexpr != nullptr) ptr_constexpr->counter++;
    } else {
      ptr_real = other.ptr_real;
      if (ptr_real != 0) {
	size_t * ctr_ptr = reinterpret_cast < size_t * > (ptr_real + 8);
	(*ctr_ptr)++;
      }
    }
  }

  constexpr counted_ref (counted_ref&& other) {
    if consteval {
      ptr_constexpr = other.ptr_constexpr;
      other.ptr_constexpr = nullptr;
    } else {
      ptr_real = other.ptr_real;
      other.ptr_real = 0;
    }
  }

  constexpr counted_ref& operator= (const std::nullptr_t&) {
    if consteval {
      if (ptr_constexpr != nullptr) {
	ptr_constexpr->counter--;
	if (ptr_constexpr->counter == (1ull << 62)) minilib::allocator < minilib::detail::tagged_ptr_constexpr_impl < T > > :: deallocate (ptr_constexpr, 1);
	ptr_constexpr = nullptr;
      }
    } else {
      if (ptr_real != 0) {
	size_t * ctr_ptr = reinterpret_cast < size_t * > (ptr_real + 8);
	(*ctr_ptr)--;
	if (*ctr_ptr == (1ull << 62)) free (reinterpret_cast < void * > (ptr_real));
	ptr_real = 0;
      }
    }
    return *this;
  }

  constexpr counted_ref& operator= (const counted_ref& other) {
    if (this == minilib::addressof (other)) return *this;

    *this = nullptr;

    if consteval {
      ptr_constexpr = other.ptr_constexpr;
      if (ptr_constexpr != nullptr) ptr_constexpr->counter++;
    } else {
      ptr_real = other.ptr_real;
      if (ptr_real != 0) {
	size_t * ctr_ptr = reinterpret_cast < size_t * > (ptr_real + 8);
	(*ctr_ptr)++;
      }
    }

    return *this;
  }

  constexpr counted_ref& operator= (counted_ref&& other) {
    if (this == minilib::addressof (other)) return *this;

    *this = nullptr;

    if consteval {
      ptr_constexpr = other.ptr_constexpr;
      other.ptr_constexpr = nullptr;
    } else {
      ptr_real = other.ptr_real;
      other.ptr_real = 0;
    }

    return *this;
  }

  constexpr ~counted_ref () { *this = nullptr; }

  constexpr uint64_t get_id () const {
    if consteval {
      if (ptr_constexpr == nullptr) std::terminate ();
      return ptr_constexpr->id;
    } else {
      if (ptr_real == 0) std::terminate ();
      uint64_t * id_ptr = reinterpret_cast < uint64_t * > (ptr_real);
      return *id_ptr;
    }
  }

  constexpr explicit operator bool () const {
    if consteval {
      if (ptr_constexpr == nullptr) return false;
      return (ptr_constexpr->counter & (1ull << 62)) == 0;
    } else {
      if (ptr_real == 0) return false;
      size_t * ctr_ptr = reinterpret_cast < size_t * > (ptr_real + 8);
      return ((*ctr_ptr) & (1ull << 62)) == 0;
    }
  }

  /* For containers to check whether a handle is valid and can be dereferenced */
  constexpr bool check (uint64_t id) const {
    if consteval {
      if (ptr_constexpr == nullptr) return false;
      if (ptr_constexpr->id != id) return false;
      return (ptr_constexpr->counter & (1ull << 62)) == 0;
    } else {
      if (ptr_real == 0) return false;
      uint64_t * id_ptr = reinterpret_cast < uint64_t * > (ptr_real);
      size_t * ctr_ptr = reinterpret_cast < size_t * > (ptr_real + 8);
      if (*id_ptr != id) return false;
      return ((*ctr_ptr) & (1ull << 62)) == 0;
    }
  }

  /* For containers which allow null. Returns 0 if null, 1 if invalid, 2 if valid */
  constexpr uint32_t check_maybe_null (uint64_t id) const {
    if consteval {
      if (ptr_constexpr == nullptr) return 0;
      if (ptr_constexpr->id != id) return 1;
      return (ptr_constexpr->counter & (1ull << 62)) == 0 ? 2 : 1;
    } else {
      if (ptr_real == 0) return 0;
      uint64_t * id_ptr = reinterpret_cast < uint64_t * > (ptr_real);
      size_t * ctr_ptr = reinterpret_cast < size_t * > (ptr_real + 8);
      if (*id_ptr != id) return 1;
      return ((*ctr_ptr) & (1ull << 62)) == 0 ? 2 : 1;
    }
  }

  /* For cases where one is certain this is not a tombstone, and only needs to know if it is not nullptr */
  constexpr bool is_not_null () const {
    if consteval {
      return ! (ptr_constexpr == nullptr);
    } else {
      return ! (ptr_real == 0);
    }
  }

  constexpr bool operator== (const counted_ref& other) const {
    if consteval {
      return ptr_constexpr == other.ptr_constexpr;
    } else {
      return ptr_real == other.ptr_real;
    }
  }

  /* In theory, we could do an ID check here, but we choose not to for performance.
     For example, in an RB-tree lookup, the container only has to check the ID once, not at every comparison.
   */
  friend constexpr minilib::order_result compare_three_way (const counted_ref& a, const counted_ref& b) {
    if consteval {
      if (a.ptr_constexpr == nullptr) {
	if (b.ptr_constexpr == nullptr) return minilib::order_result::equal ();
	else return minilib::order_result::less ();
      } else {
	if (b.ptr_constexpr == nullptr) return minilib::order_result::greater ();
	else return minilib::compare_three_way::operator() (a.ptr_constexpr->alloc_id, b.ptr_constexpr->alloc_id);
      }
    } else {
      return minilib::compare_three_way::operator() (a.ptr_real, b.ptr_real);
    }
  }

  friend class minilib::tagged_ptr < T >;
};

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class linked_ref {
  union {
    uintptr_t ptr_real;
    minilib::detail::tagged_ptr_constexpr_impl < T > * ptr_constexpr;
  };
  linked_ref * prev, * next;

public:
  constexpr linked_ref () {
    if consteval {
      ptr_constexpr = nullptr;
    } else {
      ptr_real = 0;
    }
    prev = nullptr;
    next = nullptr;
  }

  constexpr linked_ref (const linked_ref& other) {
    if consteval {
      ptr_constexpr = other.ptr_constexpr;
      prev = nullptr;
      if (ptr_constexpr != nullptr) {
	next = ptr_constexpr->link_ref_head;
	if (next != nullptr) next->prev = this;
	ptr_constexpr->link_ref_head = this;
      } else {
	next = nullptr;
      }
    } else {
      ptr_real = other.ptr_real;
      prev = nullptr;
      if (ptr_real != 0) {
	size_t * head_ptr = reinterpret_cast < size_t * > (ptr_real + 8);
	next = reinterpret_cast < linked_ref * > (*head_ptr);
	if (next != nullptr) next->prev = this;
	*head_ptr = reinterpret_cast < size_t > (this);
      } else {
	next = nullptr;
      }
    }
  }

  constexpr linked_ref (linked_ref&& other) {
    if consteval {
      ptr_constexpr = other.ptr_constexpr;
      other.ptr_constexpr = nullptr;
    } else {
      ptr_real = other.ptr_real;
      other.ptr_real = 0;
    }
    prev = other.prev;
    next = other.next;
    other.prev = nullptr;
    other.next = nullptr;
    if (next != nullptr) next->prev = this;
    if (prev != nullptr) {
      prev->next = this;
    } else {
      if consteval {
	if (ptr_constexpr != nullptr) {
	  ptr_constexpr->link_ref_head = this;
	}
      } else {
	if (ptr_real != 0) {
	  size_t * head_ptr = reinterpret_cast < size_t * > (ptr_real + 8);
	  *head_ptr = reinterpret_cast < size_t > (this);
	}
      }
    }
  }

  constexpr linked_ref& operator= (const std::nullptr_t&) {
    if (next != nullptr) next->prev = prev;
    if (prev != nullptr) {
      prev->next = next;
    } else {
      if consteval {
	if (ptr_constexpr != nullptr) {
	  ptr_constexpr->link_ref_head = next;
	}
      } else {
	if (ptr_real != 0) {
	  size_t * head_ptr = reinterpret_cast < size_t * > (ptr_real + 8);
	  *head_ptr = reinterpret_cast < size_t > (next);
	}
      }
    }
    if consteval {
      ptr_constexpr = nullptr;
    } else {
      ptr_real = 0;
    }
    prev = nullptr;
    next = nullptr;
    return *this;
  }

  constexpr linked_ref& operator= (const linked_ref& other) {
    if (this == minilib::addressof (other)) return *this;

    *this = nullptr;

    /* Similar to copy constructor, but prev, next are already set to nullptr above */
    if consteval {
      ptr_constexpr = other.ptr_constexpr;
      if (ptr_constexpr != nullptr) {
	next = ptr_constexpr->link_ref_head;
	if (next != nullptr) next->prev = this;
	ptr_constexpr->link_ref_head = this;
      }
    } else {
      ptr_real = other.ptr_real;
      if (ptr_real != 0) {
	size_t * head_ptr = reinterpret_cast < size_t * > (ptr_real + 8);
	next = reinterpret_cast < linked_ref * > (*head_ptr);
	if (next != nullptr) next->prev = this;
	*head_ptr = reinterpret_cast < size_t > (this);
      }
    }

    return *this;
  }

  constexpr linked_ref& operator= (linked_ref&& other) {
    if (this == minilib::addressof (other)) return *this;

    *this = nullptr;

    /* Similar to move constructor */
    if consteval {
      ptr_constexpr = other.ptr_constexpr;
      other.ptr_constexpr = nullptr;
    } else {
      ptr_real = other.ptr_real;
      other.ptr_real = 0;
    }
    prev = other.prev;
    next = other.next;
    other.prev = nullptr;
    other.next = nullptr;
    if (next != nullptr) next->prev = this;
    if (prev != nullptr) {
      prev->next = this;
    } else {
      if consteval {
	if (ptr_constexpr != nullptr) {
	  ptr_constexpr->link_ref_head = this;
	}
      } else {
	if (ptr_real != 0) {
	  size_t * head_ptr = reinterpret_cast < size_t * > (ptr_real + 8);
	  *head_ptr = reinterpret_cast < size_t > (this);
	}
      }
    }

    return *this;
  }

  constexpr ~linked_ref () { *this = nullptr; }

  constexpr uint64_t get_id () const {
    if consteval {
      if (ptr_constexpr == nullptr) std::terminate ();
      return ptr_constexpr->id;
    } else {
      if (ptr_real == 0) std::terminate ();
      uint64_t * id_ptr = reinterpret_cast < uint64_t * > (ptr_real);
      return *id_ptr;
    }
  }

  constexpr explicit operator bool () const {
    if consteval {
      return ptr_constexpr != nullptr;
    } else {
      return ptr_real != 0;
    }
  }

  constexpr bool check (uint64_t id) const {
    if consteval {
      if (ptr_constexpr == nullptr) return false;
      if (ptr_constexpr->id != id) return false;
      return true;
    } else {
      if (ptr_real == 0) return false;
      uint64_t * id_ptr = reinterpret_cast < uint64_t * > (ptr_real);
      if (*id_ptr != id) return false;
      return true;
    }
  }

  constexpr uint32_t check_maybe_null (uint64_t id) const {
    if consteval {
      if (ptr_constexpr == nullptr) return 0;
      if (ptr_constexpr->id != id) return 1;
      return 2;
    } else {
      if (ptr_real == 0) return 0;
      uint64_t * id_ptr = reinterpret_cast < uint64_t * > (ptr_real);
      if (*id_ptr != id) return 1;
      return 2;
    }
  }

  constexpr bool is_not_null () const {
    if consteval {
      return ! (ptr_constexpr == nullptr);
    } else {
      return ! (ptr_real == 0);
    }
  }

  constexpr bool operator== (const linked_ref& other) const {
    if consteval {
      return ptr_constexpr == other.ptr_constexpr;
    } else {
      return ptr_real == other.ptr_real;
    }
  }

  /* Beware: if some linked_ref gets invalidated due to deallocation, then the relative order of linked_ref will change.
     Therefore, one should be very careful when using linked_ref as red-black tree keys.
     In particular, if one sees a node in the tree whose key has been invalidated, one should immediately remove the node from the tree.
   */
  friend constexpr minilib::order_result compare_three_way (const linked_ref& a, const linked_ref& b) {
    if consteval {
      if (a.ptr_constexpr == nullptr) {
	if (b.ptr_constexpr == nullptr) return minilib::order_result::equal ();
	else return minilib::order_result::less ();
      } else {
	if (b.ptr_constexpr == nullptr) return minilib::order_result::greater ();
	else return minilib::compare_three_way::operator() (a.ptr_constexpr->alloc_id, b.ptr_constexpr->alloc_id);
      }
    } else {
      return minilib::compare_three_way::operator() (a.ptr_real, b.ptr_real);
    }
  }

  friend class minilib::tagged_ptr < T >;
};

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class tagged_ptr {
  union {
    uintptr_t ptr_real;
    minilib::detail::tagged_ptr_constexpr_impl < T > * ptr_constexpr;
  };

public:
  constexpr tagged_ptr () {
    if consteval {
      ptr_constexpr = nullptr;
    } else {
      ptr_real = 0;
    }
  }

  /* The following five members are trivial. */
  constexpr tagged_ptr (const tagged_ptr&) = default;
  constexpr tagged_ptr (tagged_ptr&&) = default;
  tagged_ptr& operator= (const tagged_ptr&) = default;
  tagged_ptr& operator= (tagged_ptr&&) = default;
  ~tagged_ptr () = default;

  /* This function allows directly specifying alignment and size.
     This is to support use-cases where T is not the actual data structure,
     but only a constexpr emulation of the actual structure.
     An example is an array whose length is determined at runtime.
     If we do not allow specifying custom alignment and size,
     then such use-cases cannot be accommodated by any type T
     since sizeof (T) must be determined at compile-time.
     To create a tagged dynamic-length array, set T to raw_dyn_array < U > where U is the element type.
     Then, under constexpr, this code creates storage for raw_dyn_array < U >
     which can then be initialized to emulate storage for a dynamic-length array.
     Under the non-constexpr context, this code creates storage for the dynamic-length array directly.
     Use std::start_lifetime_as_array() to create the virtual array object.
     Users of such use-cases MUST use if consteval to differentiate between consteval and real contexts.

     This function take an additional parameter called alloc_id.
     This parameter is only used under constexpr.
     Its purpose is to emulate a pointer global order.

     We provide two overloads of this function. The second overload does not have the alignment and size arguments.
     The reason is that, we expect the vast majority of users of this API do not need to specify alignment and size.
     In that case, forcing the caller to specify alignment and size (even implicitly via default values) wastes a few registers and CPU cycles.
   */
  static constexpr tagged_ptr allocate (uint64_t id, [[maybe_unused]] uint64_t alloc_id, size_t alignment, size_t size) {
    tagged_ptr result;
    if consteval {
      result.ptr_constexpr = minilib::allocator < minilib::detail::tagged_ptr_constexpr_impl < T > > :: allocate (1);
      /* If constexpr allocate fails, the compiler will complain. */
      minilib::construct_at < minilib::detail::tagged_ptr_constexpr_impl < T > > (result.ptr_constexpr);
      result.ptr_constexpr->id = id;
      result.ptr_constexpr->is_counter = true;
      result.ptr_constexpr->counter = 0;
      result.ptr_constexpr->alloc_id = alloc_id;
    } else {
      result.ptr_real = reinterpret_cast < uintptr_t > (aligned_tagged_alloc (alignment, size));
      if (result.ptr_real == 0) std::terminate ();
      /* Below we call default_construct_at (which should be no-op) to avoid a technical UB.
	 Since fundamental types are trivially destructible, we do not need to call destroy_at during deallocation.
       */
      uint64_t * id_ptr = reinterpret_cast < uint64_t * > (result.ptr_real);
      minilib::default_construct_at < uint64_t > (id_ptr);
      *id_ptr = id;
      size_t * ctr_ptr = reinterpret_cast < size_t * > (result.ptr_real + 8);
      minilib::default_construct_at < size_t > (ctr_ptr);
      *ctr_ptr = 1ull << 63;
    }
    return result;
  }

  static constexpr tagged_ptr allocate (uint64_t id, [[maybe_unused]] uint64_t alloc_id = 0) {
    tagged_ptr result;
    if consteval {
      result.ptr_constexpr = minilib::allocator < minilib::detail::tagged_ptr_constexpr_impl < T > > :: allocate (1);
      minilib::construct_at < minilib::detail::tagged_ptr_constexpr_impl < T > > (result.ptr_constexpr);
      result.ptr_constexpr->id = id;
      result.ptr_constexpr->is_counter = true;
      result.ptr_constexpr->counter = 0;
      result.ptr_constexpr->alloc_id = alloc_id;
    } else {
      result.ptr_real = reinterpret_cast < uintptr_t > (aligned_tagged_alloc (alignof (T), sizeof (T)));
      if (result.ptr_real == 0) std::terminate ();
      uint64_t * id_ptr = reinterpret_cast < uint64_t * > (result.ptr_real);
      minilib::default_construct_at < uint64_t > (id_ptr);
      *id_ptr = id;
      size_t * ctr_ptr = reinterpret_cast < size_t * > (result.ptr_real + 8);
      minilib::default_construct_at < size_t > (ctr_ptr);
      *ctr_ptr = 1ull << 63;
    }
    return result;
  }

  static constexpr void deallocate (const tagged_ptr& ptr) {
    if consteval {
      if (ptr.ptr_constexpr == nullptr) return;
      if (ptr.ptr_constexpr->is_counter) {
	if (ptr.ptr_constexpr->counter != 0) {
	  ptr.ptr_constexpr->counter |= (1ull << 62);
	  return;
	}
      } else {
	minilib::linked_ref < T > * p = ptr.ptr_constexpr->link_ref_head;
	while (p != nullptr) {
	  minilib::linked_ref < T > * next = p->next;
	  p->ptr_constexpr = nullptr;
	  p->prev = nullptr;
	  p->next = nullptr;
	  p = next;
	}
      }
      minilib::allocator < minilib::detail::tagged_ptr_constexpr_impl < T > > :: deallocate (ptr.ptr_constexpr, 1);
    } else {
      if (ptr.ptr_real == 0) return;
      size_t * ctr_ptr = reinterpret_cast < size_t * > (ptr.ptr_real + 8);
      size_t ctr = *ctr_ptr;
      if (ctr & (1ull << 63)) {
	if (ctr != (1ull << 63)) {
	  *ctr_ptr = ctr | (1ull << 62);
	  return;
	}
      } else {
	minilib::linked_ref < T > * p = reinterpret_cast < minilib::linked_ref < T > * > (ctr);
	while (p != nullptr) {
	  minilib::linked_ref < T > * next = p->next;
	  p->ptr_real = 0;
	  p->prev = nullptr;
	  p->next = nullptr;
	  p = next;
	}
      }
      free (reinterpret_cast < void * > (ptr.ptr_real));
    }
  }

  constexpr tagged_ptr& operator= (const std::nullptr_t&) {
    if consteval {
      ptr_constexpr = nullptr;
    } else {
      ptr_real = 0;
    }
    return *this;
  }

  /* The following functions assume this is not a tombstone.
     After an allocation becomes a tombstone, the container SHOULD NOT continue to hold any tagged_ptr to it.
     Therefore, the container should never call any of the following functions on a tombstone.
   */
  constexpr uint64_t get_id () const {
    if consteval {
      if (ptr_constexpr == nullptr) std::terminate ();
      return ptr_constexpr->id;
    } else {
      if (ptr_real == 0) std::terminate ();
      uint64_t * id_ptr = reinterpret_cast < uint64_t * > (ptr_real);
      return *id_ptr;
    }
  }

  /* If the non-constexpr data structure differs from the constexpr type T,
     callers MUST use reinterpret_cast to cast the pointer to the correct type.
   */
  constexpr T& operator* () const {
    if consteval {
      if (ptr_constexpr == nullptr) std::terminate ();
      return ptr_constexpr->data[0];
    } else {
      if (ptr_real == 0) std::terminate ();
      T * p = reinterpret_cast < T * > (ptr_real + 16);
      return *p;
    }
  }

  constexpr T * operator-> () const {
    if consteval {
      if (ptr_constexpr == nullptr) std::terminate ();
      return minilib::addressof (ptr_constexpr->data[0]);
    } else {
      if (ptr_real == 0) std::terminate ();
      T * p = reinterpret_cast < T * > (ptr_real + 16);
      return p;
    }
  }

  constexpr T * data () const {
    if consteval {
      if (ptr_constexpr == nullptr) std::terminate ();
      return minilib::addressof (ptr_constexpr->data[0]);
    } else {
      if (ptr_real == 0) std::terminate ();
      T * p = reinterpret_cast < T * > (ptr_real + 16);
      return p;
    }
  }

  /* Used when one is absolutely certain the pointer is not nullptr */
  constexpr T * data_unsafe () const {
    if consteval {
      return minilib::addressof (ptr_constexpr->data[0]);
    } else {
      T * p = reinterpret_cast < T * > (ptr_real + 16);
      return p;
    }
  }

  constexpr explicit operator bool () const {
    if consteval {
      return ptr_constexpr != nullptr;
    } else {
      return ptr_real != 0;
    }
  }

  /* In theory, for the following two functions we should just return minilib::counted_ref or minilib::linked_ref.
     However, current bugs in GCC constant evaluator prevent this from working.
     Specifically, if you write minilib::counted_ref < T > ref = p.create_counted_ref();
     Then C++ considers this a prvalue initialization.
     According to C++17 mandatory copy elision, in this case create_counted_ref() should construct the return value directly in ref.
     However, that's not how GCC constant evaluator handles this case.
     Instead, the constant evaluator creates an intermediate object called RESULT_DECL.
     The return value is constructed in RESULT_DECL, and the value representation of RESULT_DECL is copied into val.
     However, this means the linked list pointers are not updated and still point to RESULT_DECL!
     To work around this, we let the user construct the reference object first, and then pass a reference to this function.
     This is slightly unidiomatic C++, but it is the only way we have.
     https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101295
   */
  constexpr void create_counted_ref (minilib::counted_ref < T > & result) const {
    result = nullptr;
    if consteval {
      if (ptr_constexpr == nullptr) std::terminate ();
      if (ptr_constexpr->is_counter) {
	ptr_constexpr->counter++;
      } else {
	if (ptr_constexpr->link_ref_head == nullptr) {
	  ptr_constexpr->is_counter = true;
	  ptr_constexpr->counter = 1;
	} else {
	  std::terminate ();
	}
      }
      result.ptr_constexpr = ptr_constexpr;
    } else {
      if (ptr_real == 0) std::terminate ();
      size_t * ctr_ptr = reinterpret_cast < size_t * > (ptr_real + 8);
      size_t ctr = *ctr_ptr;
      if (ctr & (1ull << 63)) {
	*ctr_ptr = ctr + 1;
      } else {
	if (ctr == 0) {
	  *ctr_ptr = (1ull << 63) | 1;
	} else {
	  std::terminate ();
	}
      }
      result.ptr_real = ptr_real;
    }
  }

  constexpr void create_linked_ref (minilib::linked_ref < T > & result) const {
    result = nullptr;
    if consteval {
      if (ptr_constexpr == nullptr) std::terminate ();
      if (ptr_constexpr->is_counter) {
	if (ptr_constexpr->counter == 0) {
	  ptr_constexpr->is_counter = false;
	  ptr_constexpr->link_ref_head = minilib::addressof (result);
	} else {
	  std::terminate ();
	}
      } else {
	result.next = ptr_constexpr->link_ref_head;
	if (ptr_constexpr->link_ref_head != nullptr) ptr_constexpr->link_ref_head->prev = minilib::addressof (result);
	ptr_constexpr->link_ref_head = minilib::addressof (result);
      }
      result.prev = nullptr;
      result.ptr_constexpr = ptr_constexpr;
    } else {
      if (ptr_real == 0) std::terminate ();
      size_t * ctr_ptr = reinterpret_cast < size_t * > (ptr_real + 8);
      size_t ctr = *ctr_ptr;
      if (ctr & (1ull << 63)) {
	if (ctr == (1ull << 63)) {
	  *ctr_ptr = reinterpret_cast < size_t > (minilib::addressof (result));
	} else {
	  std::terminate ();
	}
      } else {
	minilib::linked_ref < T > * head = reinterpret_cast < minilib::linked_ref < T > * > (ctr);
	result.next = head;
	if (head != nullptr) head->prev = minilib::addressof (result);
	*ctr_ptr = reinterpret_cast < size_t > (minilib::addressof (result));
      }
      result.prev = nullptr;
      result.ptr_real = ptr_real;
    }
  }

  /* The following functions are static and public, but they should ONLY be called by the container that originally allocated the pointer.
     In general, we expect containers to re-export counted_ref < T > and linked_ref < T > as container < T > :: handle_type.
     Users of the containers should NOT depend on implementation details of the handle type,
     and therefore SHOULD NOT call these functions directly.
     The container that indirectly calls these functions SHOULD check that the handle is not null and has consistent ID value by calling ref.check (id).

     Additionally, if the actual runtime data structure differs from T, then one should not use these functions, since *p is UB.
     Instead, after calling ref.check(id), use from_counted_ref().data_unsafe() and do a reinterpret_cast.
   */
  static constexpr T& deref_counted_ref (const minilib::counted_ref < T > & ref) {
    if consteval {
      if (ref.ptr_constexpr == nullptr) std::terminate ();
      size_t ctr = ref.ptr_constexpr->counter;
      if (ctr & (1ull << 62)) std::terminate ();
      return ref.ptr_constexpr->data[0];
    } else {
      if (ref.ptr_real == 0) std::terminate ();
      size_t * ctr_ptr = reinterpret_cast < size_t * > (ref.ptr_real + 8);
      size_t ctr = *ctr_ptr;
      if (ctr & (1ull << 62)) std::terminate ();
      T * p = reinterpret_cast < T * > (ref.ptr_real + 16);
      return *p;
    }
  }

  /* If the container has already called ref.check(id), then use this to avoid doing the checks again */
  static constexpr T& deref_counted_ref_unsafe (const minilib::counted_ref < T > & ref) {
    if consteval {
      return ref.ptr_constexpr->data[0];
    } else {
      T * p = reinterpret_cast < T * > (ref.ptr_real + 16);
      return *p;
    }
  }

  static constexpr T& deref_linked_ref (const minilib::linked_ref < T > & ref) {
    if consteval {
      if (ref.ptr_constexpr == nullptr) std::terminate ();
      return ref.ptr_constexpr->data[0];
    } else {
      if (ref.ptr_real == 0) std::terminate ();
      T * p = reinterpret_cast < T * > (ref.ptr_real + 16);
      return *p;
    }
  }

  static constexpr T& deref_linked_ref_unsafe (const minilib::linked_ref < T > & ref) {
    if consteval {
      return ref.ptr_constexpr->data[0];
    } else {
      T * p = reinterpret_cast < T * > (ref.ptr_real + 16);
      return *p;
    }
  }

  /* Before casting a reference back to tagged_ptr, the container MUST check for consistent ID and tombstone */
  static constexpr tagged_ptr from_counted_ref (const minilib::counted_ref < T > & ref) {
    tagged_ptr result;
    if consteval {
      result.ptr_constexpr = ref.ptr_constexpr;
    } else {
      result.ptr_real = ref.ptr_real;
    }
    return result;
  }

  static constexpr tagged_ptr from_linked_ref (const minilib::linked_ref < T > & ref) {
    tagged_ptr result;
    if consteval {
      result.ptr_constexpr = ref.ptr_constexpr;
    } else {
      result.ptr_real = ref.ptr_real;
    }
    return result;
  }

  constexpr bool operator== (const tagged_ptr& other) const {
    if consteval {
      return ptr_constexpr == other.ptr_constexpr;
    } else {
      return ptr_real == other.ptr_real;
    }
  }

  friend constexpr minilib::order_result compare_three_way (const tagged_ptr& a, const tagged_ptr& b) {
    if consteval {
      if (a.ptr_constexpr == nullptr) {
	if (b.ptr_constexpr == nullptr) return minilib::order_result::equal ();
	else return minilib::order_result::less ();
      } else {
	if (b.ptr_constexpr == nullptr) return minilib::order_result::greater ();
	else return minilib::compare_three_way::operator() (a.ptr_constexpr->alloc_id, b.ptr_constexpr->alloc_id);
      }
    } else {
      return minilib::compare_three_way::operator() (a.ptr_real, b.ptr_real);
    }
  }
};

}

#endif
