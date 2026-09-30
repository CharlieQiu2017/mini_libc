/* Test vector.hpp with class having private destructors under constexpr */

#include <stdint.h>
#include <exit.h>
#include <vector.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

#define ASSERT_TRUE(cond) do { if (!(cond)) return false; } while (0)

/* Probe has a private destructor.
   - std::is_destructible_v < Probe > and minilib::is_destructible_v < Probe > are false.
   - As a result, vector's destructor (~vector()), clear(), pop_back(), pop_back_many(),
     erase(), and auto-reallocation cannot destroy elements directly.
   - vector::push_back(const T&) and vector::push_back(T&&) cannot be called because
     their implementations instantiate an on-stack temporary "T t_", which requires
     an accessible destructor.
   - However, in-place construction (emplace_back, emplace_back_default, emplace_back_many,
     emplace_back_many_default, generate, inform_emplace_back_many) constructs directly into
     allocated storage without stack temporaries.
   - Elements must be manually destroyed via friend functions, and the vector informed
     via inform_pop_back_many() so that size() becomes 0 before vector destruction or storage resizing.
*/
struct Probe {
  int32_t val;
  constexpr Probe () : val (42) { }
  constexpr explicit Probe (int32_t v) : val (v) { }
  constexpr Probe (const Probe & o) : val (o.val) { }
  constexpr Probe (Probe && o) : val (o.val) { o.val = -1; }
  constexpr Probe & operator= (const Probe & o) { val = o.val; return *this; }
  constexpr Probe & operator= (Probe && o) { val = o.val; o.val = -1; return *this; }

  friend constexpr void destroy_probe (Probe * p) { p->~Probe (); }
  friend constexpr void construct_probe (Probe * p, int32_t v) {
    ::new (static_cast < void * > (p)) Probe (v);
  }
  friend constexpr void push_probe (minilib::vector < Probe > & v, int32_t v_val) {
    v.push_back (Probe (v_val));
  }

private:
  constexpr ~Probe () { }
};

static_assert (! minilib::is_destructible_v < Probe >, "Probe should not be destructible outside friend");
static_assert (minilib::is_copy_constructible_v < Probe >);
static_assert (minilib::is_move_constructible_v < Probe >);
static_assert (minilib::is_truly_default_constructible_v < Probe >);

// Verify insert and emplace methods are disabled at compile-time for non-destructible types
template < typename V, typename P >
concept can_insert_no_inv_copy = requires (V & v, const P & p) { v.insert_no_invalidate (0, p); };

template < typename V, typename P >
concept can_insert_no_inv_move = requires (V & v, P && p) { v.insert_no_invalidate (0, minilib::move (p)); };

template < typename V, typename P >
concept can_insert_may_inv_copy = requires (V & v, const P & p) { v.insert_may_invalidate (0, p); };

template < typename V, typename P >
concept can_insert_may_inv_move = requires (V & v, P && p) { v.insert_may_invalidate (0, minilib::move (p)); };

template < typename V, typename P >
concept can_insert_many_no_inv = requires (V & v, const P & p) { v.insert_many_no_invalidate (0, 1, p); };

template < typename V, typename P >
concept can_insert_many_may_inv = requires (V & v, const P & p) { v.insert_many_may_invalidate (0, 1, p); };

template < typename V >
concept can_emplace_no_inv = requires (V & v) { v.emplace_no_invalidate (0, 42); };

template < typename V >
concept can_emplace_may_inv = requires (V & v) { v.emplace_may_invalidate (0, 42); };

static_assert (! can_insert_no_inv_copy < minilib::vector < Probe >, Probe >);
static_assert (! can_insert_no_inv_move < minilib::vector < Probe >, Probe >);
static_assert (! can_insert_may_inv_copy < minilib::vector < Probe >, Probe >);
static_assert (! can_insert_may_inv_move < minilib::vector < Probe >, Probe >);
static_assert (! can_insert_many_no_inv < minilib::vector < Probe >, Probe >);
static_assert (! can_insert_many_may_inv < minilib::vector < Probe >, Probe >);
static_assert (! can_emplace_no_inv < minilib::vector < Probe > >);
static_assert (! can_emplace_may_inv < minilib::vector < Probe > >);

// Helper: destroy all elements in reverse order and inform vector so its size becomes 0
constexpr void destroy_all (minilib::vector < Probe > & v) {
  for (size_t i = v.size (); i > 0; i--) {
    destroy_probe (&v[i - 1]);
  }
  v.inform_pop_back_many (v.size ());
}

// 1. Basic single element lifecycle (the original test case)
constexpr bool test_single_element_lifecycle () {
  minilib::vector < Probe > v;
  ASSERT_TRUE (v.size () == 0);
  ASSERT_TRUE (v.capacity () == 0);
  ASSERT_TRUE (v.data () == nullptr);
  ASSERT_TRUE (!v);
  ASSERT_TRUE (!static_cast < bool > (v));

  v.resize_storage (1);
  v.emplace_back ();
  ASSERT_TRUE (v.size () == 1);
  ASSERT_TRUE (v[0].val == 42);

  destroy_probe (v.data ());
  v.inform_pop_back_many (1);
  ASSERT_TRUE (v.size () == 0);

  return true;
}

// 2. Batch addition workflows: reserve upfront, emplace_back, default, many, generate
constexpr bool test_batch_addition_and_destruction () {
  minilib::vector < Probe > v;
  // Reserve capacity upfront while empty (avoids reallocation panics when len > 0)
  v.reserve (16);
  ASSERT_TRUE (v.capacity () >= 16);
  ASSERT_TRUE (v.size () == 0);

  // emplace_back with argument
  v.emplace_back (10);
  // emplace_back_default
  v.emplace_back_default ();
  ASSERT_TRUE (v[0].val == 10 && v[1].val == 42);

  // emplace_back_many
  v.emplace_back_many (2, 77);
  ASSERT_TRUE (v[2].val == 77 && v[3].val == 77);

  // emplace_back_many_default
  v.emplace_back_many_default (2);
  ASSERT_TRUE (v[4].val == 42 && v[5].val == 42);

  // generate (lambda returns int32_t constructor argument, constructed in-place)
  v.generate (3, [](size_t i) { return static_cast < int32_t > (100 + i); });
  ASSERT_TRUE (v[6].val == 100 && v[7].val == 101 && v[8].val == 102);

  // emplace_back additional values
  v.emplace_back (200);
  v.emplace_back (300);
  ASSERT_TRUE (v[9].val == 200 && v[10].val == 300);

  // emplace_back_many with different values
  v.emplace_back_many (2, 400);
  ASSERT_TRUE (v[11].val == 400 && v[12].val == 400);
  ASSERT_TRUE (v.size () == 13);

  // Partial destruction (pop suffix of 3 elements)
  for (size_t i = 0; i < 3; i++) {
    destroy_probe (&v[v.size () - 1 - i]);
  }
  v.inform_pop_back_many (3);
  ASSERT_TRUE (v.size () == 10);

  // Destroy remaining elements
  destroy_all (v);
  ASSERT_TRUE (v.size () == 0);

  return true;
}

// 3. push_back and push_back_many when capacity is pre-reserved (alloc_len - len >= count)
// Now supported because vector checks if constexpr (is_destructible_v<T>), skipping the on-stack temporary
constexpr bool test_push_back_with_reserved_capacity () {
  minilib::vector < Probe > v;
  v.reserve (10);
  v.emplace_back (10);
  v.emplace_back (20);

  // push_back lvalue from existing element in vector
  v.push_back (v[0]);
  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[2].val == 10);

  // push_back rvalue from existing element in vector
  v.push_back (minilib::move (v[1]));
  ASSERT_TRUE (v.size () == 4);
  ASSERT_TRUE (v[3].val == 20);

  // push_back_many using existing element reference
  v.push_back_many (v[0], 2);
  ASSERT_TRUE (v.size () == 6);
  ASSERT_TRUE (v[4].val == 10 && v[5].val == 10);

  // Friend helper pushing a newly constructed temporary rvalue
  push_probe (v, 999);
  ASSERT_TRUE (v.size () == 7);
  ASSERT_TRUE (v[6].val == 999);

  destroy_all (v);
  return true;
}

// 4. External factory construction using inform_emplace_back_many
constexpr bool test_external_construction_workflow () {
  minilib::vector < Probe > v;
  v.reserve (8);

  // Externally placement-new objects into reserved space
  construct_probe (v.data () + v.size (), 1);
  construct_probe (v.data () + v.size () + 1, 2);
  construct_probe (v.data () + v.size () + 2, 3);
  v.inform_emplace_back_many (3);

  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[0].val == 1 && v[1].val == 2 && v[2].val == 3);

  // Manual destruction of 2 elements from the back
  destroy_probe (&v[2]);
  destroy_probe (&v[1]);
  v.inform_pop_back_many (2);
  ASSERT_TRUE (v.size () == 1);
  ASSERT_TRUE (v[0].val == 1);

  // Destroy final element
  destroy_probe (&v[0]);
  v.inform_pop_back_many (1);
  ASSERT_TRUE (v.size () == 0);

  return true;
}

// 4. Custom erase workflow (erasing single element and ranges from middle without vector::erase)
constexpr bool test_custom_erase_workflow () {
  minilib::vector < Probe > v;
  v.reserve (8);
  v.emplace_back (10);
  v.emplace_back (20);
  v.emplace_back (30);
  v.emplace_back (40);
  v.emplace_back (50);
  ASSERT_TRUE (v.size () == 5);

  // Single element erase at index 1 (element with val == 20):
  // Shift elements [2..4] into [1..3] using move assignment, then destroy trailing element
  size_t erase_idx = 1;
  for (size_t i = erase_idx; i < v.size () - 1; i++) {
    v[i] = minilib::move (v[i + 1]);
  }
  destroy_probe (&v[v.size () - 1]);
  v.inform_pop_back_many (1);

  ASSERT_TRUE (v.size () == 4);
  ASSERT_TRUE (v[0].val == 10 && v[1].val == 30 && v[2].val == 40 && v[3].val == 50);

  // Range erase: erase 2 elements starting at index 1 (elements 30 and 40):
  // Shift elements [3..3] (element 50) into index 1, then destroy trailing 2 elements
  size_t range_idx = 1;
  size_t range_count = 2;
  for (size_t i = range_idx; i < v.size () - range_count; i++) {
    v[i] = minilib::move (v[i + range_count]);
  }
  for (size_t i = v.size () - range_count; i < v.size (); i++) {
    destroy_probe (&v[i]);
  }
  v.inform_pop_back_many (range_count);

  ASSERT_TRUE (v.size () == 2);
  ASSERT_TRUE (v[0].val == 10 && v[1].val == 50);

  destroy_all (v);
  return true;
}

// 5. Shift by workflow (inserting into middle using shift_by)
constexpr bool test_shift_and_insert_workflow () {
  minilib::vector < Probe > v;
  v.reserve (8);
  v.emplace_back (10);
  v.emplace_back (30);

  // Use shift_by to create a gap at index 1 of size 1
  v.shift_by (1, 1);
  // Locations: v[0]=10, v[1] was moved to v[2]=30.
  // Re-initialize the gap at v[1]:
  destroy_probe (&v[1]);
  construct_probe (&v[1], 20);

  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[0].val == 10 && v[1].val == 20 && v[2].val == 30);

  destroy_all (v);
  return true;
}

// 6. Move semantics workflow (ownership transfer between vectors)
constexpr bool test_move_semantics_workflow () {
  minilib::vector < Probe > v1;
  v1.reserve (8);
  v1.emplace_back (111);
  v1.emplace_back (222);

  // Move construct v2 from v1:
  // v1 becomes empty (len = 0, ptr = nullptr), so v1 can safely destruct
  minilib::vector < Probe > v2 (minilib::move (v1));
  ASSERT_TRUE (v1.size () == 0 && v1.data () == nullptr);
  ASSERT_TRUE (v2.size () == 2);
  ASSERT_TRUE (v2[0].val == 111 && v2[1].val == 222);

  // Move assignment into an empty vector v3
  minilib::vector < Probe > v3;
  v3 = minilib::move (v2);
  ASSERT_TRUE (v2.size () == 0 && v2.data () == nullptr);
  ASSERT_TRUE (v3.size () == 2);
  ASSERT_TRUE (v3[0].val == 111 && v3[1].val == 222);

  // Clean up v3
  destroy_all (v3);
  return true;
}

// 7. Copy constructor workflow
constexpr bool test_copy_semantics_workflow () {
  minilib::vector < Probe > v1;
  v1.reserve (4);
  v1.emplace_back (50);
  v1.emplace_back (60);

  // Copy construct v2:
  // vector copy constructor uses construct_at, which does not require destructor
  minilib::vector < Probe > v2 (v1);
  ASSERT_TRUE (v2.size () == 2);
  ASSERT_TRUE (v2[0].val == 50 && v2[1].val == 60);

  // Deep copy check: modifying v1 does not modify v2
  v1[0].val = 999;
  ASSERT_TRUE (v2[0].val == 50);

  // Both vectors must be manually destroyed before scope exit
  destroy_all (v1);
  destroy_all (v2);
  return true;
}

// 8. Controlled growth / reallocation workflow
// Since vector cannot automatically reallocate when non-destructible elements exist,
// this workflow demonstrates how code can manually grow a vector by moving elements
// to a newly allocated vector and then move-assigning back.
constexpr bool test_manual_reallocation_growth () {
  minilib::vector < Probe > v;
  v.reserve (2);
  v.emplace_back (1);
  v.emplace_back (2);
  ASSERT_TRUE (v.size () == 2 && v.capacity () == 2);

  // We want to add a third element, but capacity is full.
  // Manual growth strategy:
  // 1. Allocate new vector with doubled capacity
  minilib::vector < Probe > v_new;
  v_new.reserve (4);
  // 2. Move elements from v to v_new
  for (size_t i = 0; i < v.size (); i++) {
    v_new.emplace_back (minilib::move (v[i]));
  }
  // 3. Destroy moved-from elements in v
  destroy_all (v);
  // 4. Move v_new into v (since v.size() == 0, move assignment deallocates v's old storage safely)
  v = minilib::move (v_new);

  ASSERT_TRUE (v.size () == 2 && v.capacity () == 4);
  ASSERT_TRUE (v[0].val == 1 && v[1].val == 2);

  // Now we have spare capacity to add the third element directly!
  v.emplace_back (3);
  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[0].val == 1 && v[1].val == 2 && v[2].val == 3);

  destroy_all (v);
  return true;
}

// 9. Repeated reuse lifecycle
constexpr bool test_repeated_reuse_lifecycle () {
  minilib::vector < Probe > v;
  v.reserve (4);

  for (int32_t round = 0; round < 3; round++) {
    v.emplace_back (round * 10);
    v.emplace_back (round * 10 + 1);
    ASSERT_TRUE (v.size () == 2);
    ASSERT_TRUE (v[0].val == round * 10 && v[1].val == round * 10 + 1);

    destroy_all (v);
    ASSERT_TRUE (v.size () == 0);
    // clear() on empty vector is safe
    v.clear ();
  }

  // resize_storage(0) on empty vector frees storage safely
  v.resize_storage (0);
  ASSERT_TRUE (v.capacity () == 0 && v.data () == nullptr);

  return true;
}

// 10. Vector swapping workflow
constexpr bool test_custom_swap_workflow () {
  minilib::vector < Probe > v1;
  v1.reserve (4);
  v1.emplace_back (100);

  minilib::vector < Probe > v2;
  v2.reserve (4);
  v2.emplace_back (200);
  v2.emplace_back (300);

  // Swap two vectors holding non-destructible elements using move operations
  minilib::vector < Probe > tmp (minilib::move (v1));
  v1 = minilib::move (v2);
  v2 = minilib::move (tmp);

  ASSERT_TRUE (v1.size () == 2);
  ASSERT_TRUE (v1[0].val == 200 && v1[1].val == 300);
  ASSERT_TRUE (v2.size () == 1);
  ASSERT_TRUE (v2[0].val == 100);

  destroy_all (v1);
  destroy_all (v2);
  return true;
}

// 11. Shrinking / compaction workflow
constexpr bool test_custom_shrink_workflow () {
  minilib::vector < Probe > v;
  v.reserve (16);
  v.emplace_back (1);
  v.emplace_back (2);
  ASSERT_TRUE (v.capacity () >= 16 && v.size () == 2);

  // Shrink capacity to match size (compacting)
  minilib::vector < Probe > v_compact;
  v_compact.reserve (v.size ());
  for (size_t i = 0; i < v.size (); i++) {
    v_compact.emplace_back (minilib::move (v[i]));
  }
  destroy_all (v);
  v = minilib::move (v_compact);

  ASSERT_TRUE (v.size () == 2);
  ASSERT_TRUE (v.capacity () == 2);
  ASSERT_TRUE (v[0].val == 1 && v[1].val == 2);

  destroy_all (v);
  return true;
}

// 12. RAII Manager pattern
// Real-world pattern: An enclosing manager / handle class with friend access to Probe
// encapsulates minilib::vector < Probe > and automates destruction in its own destructor.
class ProbeManager {
private:
  minilib::vector < Probe > storage;

public:
  constexpr ProbeManager () = default;

  constexpr void add (int32_t val) {
    if (storage.size () == storage.capacity ()) {
      size_t new_cap = storage.capacity () == 0 ? 4 : storage.capacity () * 2;
      minilib::vector < Probe > next;
      next.reserve (new_cap);
      for (size_t i = 0; i < storage.size (); i++) {
        next.emplace_back (minilib::move (storage[i]));
      }
      destroy_all (storage);
      storage = minilib::move (next);
    }
    storage.emplace_back (val);
  }

  constexpr size_t size () const { return storage.size (); }
  constexpr int32_t get (size_t i) const { return storage[i].val; }

  constexpr ~ProbeManager () {
    destroy_all (storage);
  }
};

constexpr bool test_raii_manager_pattern () {
  ProbeManager mgr;
  for (int32_t i = 0; i < 10; i++) {
    mgr.add (i * 5);
  }
  ASSERT_TRUE (mgr.size () == 10);
  for (size_t i = 0; i < 10; i++) {
    ASSERT_TRUE (mgr.get (i) == static_cast < int32_t > (i * 5));
  }
  // When mgr goes out of scope, ~ProbeManager() safely destroys all elements
  return true;
}

// Master constexpr test
constexpr bool test_all () {
  ASSERT_TRUE (test_single_element_lifecycle ());
  ASSERT_TRUE (test_batch_addition_and_destruction ());
  ASSERT_TRUE (test_push_back_with_reserved_capacity ());
  ASSERT_TRUE (test_external_construction_workflow ());
  ASSERT_TRUE (test_custom_erase_workflow ());
  ASSERT_TRUE (test_shift_and_insert_workflow ());
  ASSERT_TRUE (test_move_semantics_workflow ());
  ASSERT_TRUE (test_copy_semantics_workflow ());
  ASSERT_TRUE (test_manual_reallocation_growth ());
  ASSERT_TRUE (test_repeated_reuse_lifecycle ());
  ASSERT_TRUE (test_custom_swap_workflow ());
  ASSERT_TRUE (test_custom_shrink_workflow ());
  ASSERT_TRUE (test_raii_manager_pattern ());
  return true;
}

static_assert (test_single_element_lifecycle ());
static_assert (test_batch_addition_and_destruction ());
static_assert (test_push_back_with_reserved_capacity ());
static_assert (test_external_construction_workflow ());
static_assert (test_custom_erase_workflow ());
static_assert (test_shift_and_insert_workflow ());
static_assert (test_move_semantics_workflow ());
static_assert (test_copy_semantics_workflow ());
static_assert (test_manual_reallocation_growth ());
static_assert (test_repeated_reuse_lifecycle ());
static_assert (test_custom_swap_workflow ());
static_assert (test_custom_shrink_workflow ());
static_assert (test_raii_manager_pattern ());
static_assert (test_all (), "All vector private destructor workflows must pass in constexpr");

constexpr bool all_passed = test_all ();

void main ([[maybe_unused]] void * sp) {
  if (!all_passed) exit (1);
  exit (0);
}
