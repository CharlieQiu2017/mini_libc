/* Test indirect.hpp with class having private destructors under constexpr */

#include <stdint.h>
#include <exit.h>
#include <indirect.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

#define ASSERT_TRUE(cond) do { if (!(cond)) return false; } while (0)

/* Probe has a private destructor.
   - std::is_destructible_v < Probe > and minilib::is_destructible_v < Probe > are false.
   - indirect::destruct() and indirect_array::destruct() will panic if called with ptr != nullptr.
   - To safely destroy indirect<Probe> or indirect_array<Probe>, the user must manually
     destroy the object(s) via a friend function, then call inform_destruct()
     to deallocate the storage and set ptr = nullptr.
   - When ptr == nullptr, ~indirect() and ~indirect_array() complete safely.
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

private:
  constexpr ~Probe () { }
};

static_assert (! minilib::is_destructible_v < Probe >, "Probe must not be destructible outside friend");
static_assert (minilib::is_copy_constructible_v < Probe >);
static_assert (minilib::is_move_constructible_v < Probe >);
static_assert (minilib::is_truly_default_constructible_v < Probe >);

// ===================================================
// Workflows for minilib::indirect<Probe>
// ===================================================

// 1. Basic lifecycle: construct -> access -> friend destroy -> inform_destruct
constexpr bool test_indirect_single_lifecycle () {
  minilib::indirect < Probe > ind;
  ASSERT_TRUE (!ind);
  ASSERT_TRUE (ind.data () == nullptr);

  // In-place construction with argument
  ind.construct (100);
  ASSERT_TRUE (static_cast < bool > (ind));
  ASSERT_TRUE (ind.data () != nullptr);
  ASSERT_TRUE (ind->val == 100);
  ASSERT_TRUE ((*ind).val == 100);

  // Manual destruction via friend function
  destroy_probe (ind.data ());
  // Inform container that object has been destructed to deallocate storage
  ind.inform_destruct ();

  ASSERT_TRUE (!ind);
  ASSERT_TRUE (ind.data () == nullptr);
  // ~indirect() runs with ptr == nullptr safely
  return true;
}

// 2. construct_default workflow
constexpr bool test_indirect_construct_default () {
  minilib::indirect < Probe > ind;
  ind.construct_default ();
  ASSERT_TRUE (static_cast < bool > (ind));
  ASSERT_TRUE (ind->val == 42);

  destroy_probe (ind.data ());
  ind.inform_destruct ();
  ASSERT_TRUE (ind.data () == nullptr);
  return true;
}

// 3. construct_null and external friend factory construction
constexpr bool test_indirect_construct_null () {
  minilib::indirect < Probe > ind;
  ind.construct_null ();
  ASSERT_TRUE (ind.data () != nullptr);

  // External friend construction in allocated storage
  construct_probe (ind.data (), 777);
  ASSERT_TRUE (ind->val == 777);

  destroy_probe (ind.data ());
  ind.inform_destruct ();
  ASSERT_TRUE (ind.data () == nullptr);
  return true;
}

// 4. Move semantics: transfer ownership without destruction
constexpr bool test_indirect_move_workflow () {
  minilib::indirect < Probe > ind1;
  ind1.construct (250);
  Probe * original_ptr = ind1.data ();

  // Move construct ind2 from ind1:
  // ind1 becomes nullptr and can safely destruct at scope exit
  minilib::indirect < Probe > ind2 (minilib::move (ind1));
  ASSERT_TRUE (!ind1 && ind1.data () == nullptr);
  ASSERT_TRUE (ind2.data () == original_ptr);
  ASSERT_TRUE (ind2->val == 250);

  // Move assign into an empty indirect ind3
  minilib::indirect < Probe > ind3;
  ind3 = minilib::move (ind2);
  ASSERT_TRUE (!ind2 && ind2.data () == nullptr);
  ASSERT_TRUE (ind3.data () == original_ptr);
  ASSERT_TRUE (ind3->val == 250);

  // Clean up ind3
  destroy_probe (ind3.data ());
  ind3.inform_destruct ();
  return true;
}

// 5. Copy semantics: duplicate Probe into a new indirect
constexpr bool test_indirect_copy_workflow () {
  minilib::indirect < Probe > ind1;
  ind1.construct (50);

  // Copy construct ind2: uses construct_at, which does not require destructibility
  minilib::indirect < Probe > ind2 (ind1);
  ASSERT_TRUE (ind2.data () != ind1.data ());
  ASSERT_TRUE (ind2->val == 50);

  // Deep copy check
  ind1->val = 999;
  ASSERT_TRUE (ind2->val == 50);

  // Both must be manually cleaned up
  destroy_probe (ind1.data ());
  ind1.inform_destruct ();

  destroy_probe (ind2.data ());
  ind2.inform_destruct ();
  return true;
}

// 6. RAII Manager pattern for indirect<Probe>
class ProbeIndirectOwner {
private:
  minilib::indirect < Probe > ind;

public:
  constexpr ProbeIndirectOwner () = default;
  constexpr explicit ProbeIndirectOwner (int32_t val) {
    ind.construct (val);
  }

  constexpr bool has_value () const { return static_cast < bool > (ind); }
  constexpr int32_t get () const { return ind->val; }
  constexpr void set (int32_t val) { ind->val = val; }

  constexpr ~ProbeIndirectOwner () {
    if (ind) {
      destroy_probe (ind.data ());
      ind.inform_destruct ();
    }
  }
};

constexpr bool test_indirect_raii_owner () {
  ProbeIndirectOwner owner (88);
  ASSERT_TRUE (owner.has_value ());
  ASSERT_TRUE (owner.get () == 88);
  owner.set (99);
  ASSERT_TRUE (owner.get () == 99);
  // owner destructs safely at scope exit
  return true;
}

// ===================================================
// Workflows for minilib::indirect_array<Probe>
// ===================================================

// Helper to destroy all elements in an indirect_array and inform container
constexpr void destroy_all_array (minilib::indirect_array < Probe > & arr) {
  for (size_t i = arr.size (); i > 0; i--) {
    destroy_probe (&arr[i - 1]);
  }
  arr.inform_destruct ();
}

// 7. Array basic lifecycle: construct -> access -> reverse destroy -> inform_destruct
constexpr bool test_indirect_array_lifecycle () {
  minilib::indirect_array < Probe > arr;
  ASSERT_TRUE (arr.size () == 0);
  ASSERT_TRUE (arr.data () == nullptr);

  // In-place construction with length and argument
  arr.construct (3, 10);
  ASSERT_TRUE (arr.size () == 3);
  ASSERT_TRUE (arr[0].val == 10 && arr[1].val == 10 && arr[2].val == 10);

  destroy_all_array (arr);
  ASSERT_TRUE (arr.size () == 0 && arr.data () == nullptr);
  return true;
}

// 8. Array construct_default workflow
constexpr bool test_indirect_array_construct_default () {
  minilib::indirect_array < Probe > arr;
  arr.construct_default (3);
  ASSERT_TRUE (arr.size () == 3);
  ASSERT_TRUE (arr[0].val == 42 && arr[1].val == 42 && arr[2].val == 42);

  destroy_all_array (arr);
  return true;
}

// 9. Array generate workflow (generator returns constructor argument)
constexpr bool test_indirect_array_generate () {
  minilib::indirect_array < Probe > arr;
  arr.generate (4, [](size_t i) { return static_cast < int32_t > (100 + i); });
  ASSERT_TRUE (arr.size () == 4);
  ASSERT_TRUE (arr[0].val == 100 && arr[1].val == 101 && arr[2].val == 102 && arr[3].val == 103);

  destroy_all_array (arr);
  return true;
}

// 10. Array construct_null and external placement new
constexpr bool test_indirect_array_construct_null () {
  minilib::indirect_array < Probe > arr;
  arr.construct_null (3);
  ASSERT_TRUE (arr.size () == 3);
  ASSERT_TRUE (arr.data () != nullptr);

  construct_probe (arr.data (), 1);
  construct_probe (arr.data () + 1, 2);
  construct_probe (arr.data () + 2, 3);

  ASSERT_TRUE (arr[0].val == 1 && arr[1].val == 2 && arr[2].val == 3);

  destroy_all_array (arr);
  return true;
}

// 11. Array move workflow
constexpr bool test_indirect_array_move_workflow () {
  minilib::indirect_array < Probe > arr1;
  arr1.construct (2, 20);
  Probe * ptr1 = arr1.data ();

  // Move construct arr2: arr1 becomes len = 0, ptr = nullptr (safely destructible)
  minilib::indirect_array < Probe > arr2 (minilib::move (arr1));
  ASSERT_TRUE (arr1.size () == 0 && arr1.data () == nullptr);
  ASSERT_TRUE (arr2.size () == 2 && arr2.data () == ptr1);
  ASSERT_TRUE (arr2[0].val == 20 && arr2[1].val == 20);

  // Move assign into empty arr3
  minilib::indirect_array < Probe > arr3;
  arr3 = minilib::move (arr2);
  ASSERT_TRUE (arr2.size () == 0 && arr2.data () == nullptr);
  ASSERT_TRUE (arr3.size () == 2 && arr3.data () == ptr1);

  destroy_all_array (arr3);
  return true;
}

// 12. Array copy workflow
constexpr bool test_indirect_array_copy_workflow () {
  minilib::indirect_array < Probe > arr1;
  arr1.construct (2, 30);

  minilib::indirect_array < Probe > arr2 (arr1);
  ASSERT_TRUE (arr2.size () == 2);
  ASSERT_TRUE (arr2.data () != arr1.data ());
  ASSERT_TRUE (arr2[0].val == 30 && arr2[1].val == 30);

  // Deep copy check
  arr1[0].val = 999;
  ASSERT_TRUE (arr2[0].val == 30);

  destroy_all_array (arr1);
  destroy_all_array (arr2);
  return true;
}

// 13. RAII Manager pattern for indirect_array<Probe>
class ProbeArrayOwner {
private:
  minilib::indirect_array < Probe > storage;

public:
  constexpr ProbeArrayOwner () = default;
  constexpr explicit ProbeArrayOwner (size_t n, int32_t val) {
    storage.construct (n, val);
  }

  constexpr size_t size () const { return storage.size (); }
  constexpr int32_t operator[] (size_t i) const { return storage[i].val; }
  constexpr void set (size_t i, int32_t val) { storage[i].val = val; }

  constexpr ~ProbeArrayOwner () {
    if (storage) {
      destroy_all_array (storage);
    }
  }
};

constexpr bool test_indirect_array_raii_owner () {
  ProbeArrayOwner owner (4, 7);
  ASSERT_TRUE (owner.size () == 4);
  for (size_t i = 0; i < 4; i++) {
    ASSERT_TRUE (owner[i] == 7);
  }
  owner.set (2, 42);
  ASSERT_TRUE (owner[2] == 42);
  // owner cleans up storage and destructs Probe elements in ~ProbeArrayOwner()
  return true;
}

// Master constexpr test
constexpr bool test_all () {
  ASSERT_TRUE (test_indirect_single_lifecycle ());
  ASSERT_TRUE (test_indirect_construct_default ());
  ASSERT_TRUE (test_indirect_construct_null ());
  ASSERT_TRUE (test_indirect_move_workflow ());
  ASSERT_TRUE (test_indirect_copy_workflow ());
  ASSERT_TRUE (test_indirect_raii_owner ());

  ASSERT_TRUE (test_indirect_array_lifecycle ());
  ASSERT_TRUE (test_indirect_array_construct_default ());
  ASSERT_TRUE (test_indirect_array_generate ());
  ASSERT_TRUE (test_indirect_array_construct_null ());
  ASSERT_TRUE (test_indirect_array_move_workflow ());
  ASSERT_TRUE (test_indirect_array_copy_workflow ());
  ASSERT_TRUE (test_indirect_array_raii_owner ());
  return true;
}

static_assert (test_indirect_single_lifecycle ());
static_assert (test_indirect_construct_default ());
static_assert (test_indirect_construct_null ());
static_assert (test_indirect_move_workflow ());
static_assert (test_indirect_copy_workflow ());
static_assert (test_indirect_raii_owner ());

static_assert (test_indirect_array_lifecycle ());
static_assert (test_indirect_array_construct_default ());
static_assert (test_indirect_array_generate ());
static_assert (test_indirect_array_construct_null ());
static_assert (test_indirect_array_move_workflow ());
static_assert (test_indirect_array_copy_workflow ());
static_assert (test_indirect_array_raii_owner ());

static_assert (test_all (), "All indirect private destructor tests must pass under constexpr");

constexpr bool all_passed = test_all ();

void main ([[maybe_unused]] void * sp) {
  if (!all_passed) exit (1);
  exit (0);
}
