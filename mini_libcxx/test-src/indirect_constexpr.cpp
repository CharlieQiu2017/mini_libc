/* Test indirect.hpp under constexpr */

#include <stdint.h>
#include <exit.h>
#include <indirect.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

#define ASSERT_TRUE(cond) do { if (!(cond)) return false; } while (0)

struct Point {
  int32_t x;
  int32_t y;
  constexpr Point () : x (0), y (0) {}
  constexpr Point (int32_t x_, int32_t y_) : x (x_), y (y_) {}
};

struct DefaultInitProbe {
  int32_t val;
  constexpr DefaultInitProbe () : val (12345) {}
  constexpr DefaultInitProbe (int32_t v) : val (v) {}
};

struct NonTrivial {
  int32_t id;
  int32_t val;

  constexpr NonTrivial () : id (0), val (42) {}
  constexpr NonTrivial (int32_t i, int32_t v) : id (i), val (v) {}
  constexpr NonTrivial (const NonTrivial & o) : id (o.id + 100), val (o.val) {}
  constexpr NonTrivial (NonTrivial && o) : id (o.id), val (o.val) {
    o.id = -1;
    o.val = -1;
  }
  constexpr NonTrivial & operator= (const NonTrivial & o) {
    id = o.id + 100;
    val = o.val;
    return *this;
  }
  constexpr NonTrivial & operator= (NonTrivial && o) {
    id = o.id;
    val = o.val;
    o.id = -1;
    o.val = -1;
    return *this;
  }
  constexpr ~NonTrivial () {
    id = -999;
    val = -999;
  }
};

struct MoveOnly {
  int32_t val;
  constexpr MoveOnly () : val (0) {}
  constexpr explicit MoveOnly (int32_t v) : val (v) {}
  constexpr MoveOnly (const MoveOnly &) = delete;
  constexpr MoveOnly & operator= (const MoveOnly &) = delete;
  constexpr MoveOnly (MoveOnly && o) : val (o.val) { o.val = -1; }
  constexpr MoveOnly & operator= (MoveOnly && o) {
    val = o.val;
    o.val = -1;
    return *this;
  }
  constexpr ~MoveOnly () {}
};

// ==========================================
// Tests for minilib::indirect<T>
// ==========================================

constexpr bool test_indirect_default_ctor () {
  minilib::indirect < int32_t > ind;
  ASSERT_TRUE (!ind);
  ASSERT_TRUE (!static_cast < bool > (ind));
  ASSERT_TRUE (ind.data () == nullptr);
  return true;
}

constexpr bool test_indirect_construct () {
  // construct with value
  minilib::indirect < int32_t > ind1;
  ind1.construct (42);
  ASSERT_TRUE (static_cast < bool > (ind1));
  ASSERT_TRUE (ind1.data () != nullptr);
  ASSERT_TRUE (*ind1 == 42);

  // construct with multiple arguments
  minilib::indirect < Point > ind2;
  ind2.construct (10, 20);
  ASSERT_TRUE (ind2->x == 10 && ind2->y == 20);
  ASSERT_TRUE ((*ind2).x == 10 && (*ind2).y == 20);

  // construct_default
  minilib::indirect < DefaultInitProbe > ind3;
  ind3.construct_default ();
  ASSERT_TRUE (ind3->val == 12345);

  // construct_null and manual placement new
  minilib::indirect < int32_t > ind4;
  ind4.construct_null ();
  minilib::construct_at (ind4.data (), 999);
  ASSERT_TRUE (*ind4 == 999);

  return true;
}

constexpr bool test_indirect_copy () {
  // copy empty
  {
    minilib::indirect < int32_t > ind1;
    minilib::indirect < int32_t > ind2 (ind1);
    ASSERT_TRUE (!ind2);
  }
  // copy populated (trivially copyable)
  {
    minilib::indirect < int32_t > ind1;
    ind1.construct (100);
    minilib::indirect < int32_t > ind2 (ind1);
    ASSERT_TRUE (static_cast < bool > (ind2));
    ASSERT_TRUE (*ind2 == 100);
    ASSERT_TRUE (ind1.data () != ind2.data ());
    *ind1 = 200;
    ASSERT_TRUE (*ind2 == 100); // deep copy
  }
  // copy populated (non-trivial)
  {
    minilib::indirect < NonTrivial > ind1;
    ind1.construct (1, 55);
    minilib::indirect < NonTrivial > ind2 (ind1);
    ASSERT_TRUE (ind2->id == 101 && ind2->val == 55);
  }
  // copy assignment: self
  {
    minilib::indirect < int32_t > ind;
    ind.construct (77);
    ind = ind;
    ASSERT_TRUE (*ind == 77);
  }
  // copy assignment: non-empty to non-empty
  {
    minilib::indirect < int32_t > ind1;
    ind1.construct (10);
    minilib::indirect < int32_t > ind2;
    ind2.construct (20);
    ind1 = ind2;
    ASSERT_TRUE (*ind1 == 20);
    ASSERT_TRUE (ind1.data () != ind2.data ());
  }
  // copy assignment: empty to non-empty
  {
    minilib::indirect < int32_t > ind1;
    ind1.construct (10);
    minilib::indirect < int32_t > ind2;
    ind1 = ind2;
    ASSERT_TRUE (!ind1);
  }
  // copy assignment: non-empty to empty
  {
    minilib::indirect < int32_t > ind1;
    minilib::indirect < int32_t > ind2;
    ind2.construct (30);
    ind1 = ind2;
    ASSERT_TRUE (static_cast < bool > (ind1));
    ASSERT_TRUE (*ind1 == 30);
  }
  return true;
}

constexpr bool test_indirect_move () {
  // move empty
  {
    minilib::indirect < int32_t > ind1;
    minilib::indirect < int32_t > ind2 (minilib::move (ind1));
    ASSERT_TRUE (!ind1 && !ind2);
  }
  // move populated
  {
    minilib::indirect < int32_t > ind1;
    ind1.construct (500);
    int32_t * original_ptr = ind1.data ();
    minilib::indirect < int32_t > ind2 (minilib::move (ind1));
    ASSERT_TRUE (!ind1);
    ASSERT_TRUE (ind2.data () == original_ptr);
    ASSERT_TRUE (*ind2 == 500);
  }
  // move assignment: self
  {
    minilib::indirect < int32_t > ind;
    ind.construct (88);
    ind = minilib::move (ind);
    ASSERT_TRUE (*ind == 88);
  }
  // move assignment: populated to populated
  {
    minilib::indirect < int32_t > ind1;
    ind1.construct (10);
    minilib::indirect < int32_t > ind2;
    ind2.construct (20);
    int32_t * ptr2 = ind2.data ();
    ind1 = minilib::move (ind2);
    ASSERT_TRUE (ind1.data () == ptr2);
    ASSERT_TRUE (*ind1 == 20);
    ASSERT_TRUE (!ind2);
  }
  // move-only type
  {
    minilib::indirect < MoveOnly > ind1;
    ind1.construct (777);
    minilib::indirect < MoveOnly > ind2 (minilib::move (ind1));
    ASSERT_TRUE (!ind1);
    ASSERT_TRUE (ind2->val == 777);
  }
  return true;
}

constexpr bool test_indirect_destruct () {
  minilib::indirect < int32_t > ind;
  ind.construct (123);
  ASSERT_TRUE (static_cast < bool > (ind));
  ind.destruct ();
  ASSERT_TRUE (!ind);
  ASSERT_TRUE (ind.data () == nullptr);
  // calling destruct on null is safe no-op
  ind.destruct ();
  ASSERT_TRUE (!ind);
  return true;
}

constexpr bool test_indirect_const_correctness () {
  minilib::indirect < Point > ind;
  ind.construct (3, 4);

  // Mutable
  ind->x = 30;
  *ind = Point (33, 44);
  ASSERT_TRUE (ind->x == 33 && ind->y == 44);

  // Const reference provides deep constness
  const minilib::indirect < Point > & cind = ind;
  ASSERT_TRUE (cind->x == 33 && cind->y == 44);
  ASSERT_TRUE ((*cind).x == 33);
  ASSERT_TRUE (cind.data ()->x == 33);
  ASSERT_TRUE (static_cast < bool > (cind));

  return true;
}

// ==========================================
// Tests for minilib::indirect_array<T>
// ==========================================

constexpr bool test_indirect_array_default_ctor () {
  minilib::indirect_array < int32_t > arr;
  ASSERT_TRUE (arr.size () == 0);
  ASSERT_TRUE (arr.data () == nullptr);
  ASSERT_TRUE (!arr);
  ASSERT_TRUE (!static_cast < bool > (arr));
  return true;
}

constexpr bool test_indirect_array_construct () {
  // construct with length and value
  minilib::indirect_array < int32_t > arr1;
  arr1.construct (4, 10);
  ASSERT_TRUE (arr1.size () == 4);
  ASSERT_TRUE (static_cast < bool > (arr1));
  ASSERT_TRUE (arr1.data () != nullptr);
  for (size_t i = 0; i < 4; i++) {
    ASSERT_TRUE (arr1[i] == 10);
  }

  // construct with length 0
  minilib::indirect_array < int32_t > arr0;
  arr0.construct (0, 10);
  ASSERT_TRUE (arr0.size () == 0);
  ASSERT_TRUE (arr0.data () == nullptr);
  ASSERT_TRUE (!arr0);

  // construct with multiple arguments
  minilib::indirect_array < Point > arr2;
  arr2.construct (3, 5, 6);
  ASSERT_TRUE (arr2.size () == 3);
  for (size_t i = 0; i < 3; i++) {
    ASSERT_TRUE (arr2[i].x == 5 && arr2[i].y == 6);
  }

  // construct_default
  minilib::indirect_array < DefaultInitProbe > arr3;
  arr3.construct_default (2);
  ASSERT_TRUE (arr3.size () == 2);
  ASSERT_TRUE (arr3[0].val == 12345 && arr3[1].val == 12345);

  // generate
  minilib::indirect_array < int32_t > arr4;
  arr4.generate (5, [](size_t i) { return static_cast < int32_t > (i * 10); });
  ASSERT_TRUE (arr4.size () == 5);
  for (size_t i = 0; i < 5; i++) {
    ASSERT_TRUE (arr4[i] == static_cast < int32_t > (i * 10));
  }

  // construct_null and manual placement new
  minilib::indirect_array < int32_t > arr5;
  arr5.construct_null (3);
  minilib::construct_at (arr5.data (), 101);
  minilib::construct_at (arr5.data () + 1, 102);
  minilib::construct_at (arr5.data () + 2, 103);
  ASSERT_TRUE (arr5.size () == 3);
  ASSERT_TRUE (arr5[0] == 101 && arr5[1] == 102 && arr5[2] == 103);

  return true;
}

constexpr bool test_indirect_array_copy () {
  // copy empty
  {
    minilib::indirect_array < int32_t > arr1;
    minilib::indirect_array < int32_t > arr2 (arr1);
    ASSERT_TRUE (arr2.size () == 0 && arr2.data () == nullptr);
  }
  // copy populated (trivially copyable)
  {
    minilib::indirect_array < int32_t > arr1;
    arr1.generate (4, [](size_t i) { return static_cast < int32_t > (i + 1); });
    minilib::indirect_array < int32_t > arr2 (arr1);
    ASSERT_TRUE (arr2.size () == 4);
    for (size_t i = 0; i < 4; i++) ASSERT_TRUE (arr2[i] == static_cast < int32_t > (i + 1));
    // deep copy
    arr1[0] = 999;
    ASSERT_TRUE (arr2[0] == 1);
  }
  // copy populated (non-trivial)
  {
    minilib::indirect_array < NonTrivial > arr1;
    arr1.construct (2, 1, 42);
    minilib::indirect_array < NonTrivial > arr2 (arr1);
    ASSERT_TRUE (arr2.size () == 2);
    ASSERT_TRUE (arr2[0].id == 101 && arr2[0].val == 42);
    ASSERT_TRUE (arr2[1].id == 101 && arr2[1].val == 42);
  }
  // copy assignment: self
  {
    minilib::indirect_array < int32_t > arr;
    arr.construct (3, 7);
    arr = arr;
    ASSERT_TRUE (arr.size () == 3 && arr[0] == 7);
  }
  // copy assignment: populated to populated
  {
    minilib::indirect_array < int32_t > arr1;
    arr1.construct (2, 11);
    minilib::indirect_array < int32_t > arr2;
    arr2.construct (4, 22);
    arr1 = arr2;
    ASSERT_TRUE (arr1.size () == 4);
    for (size_t i = 0; i < 4; i++) ASSERT_TRUE (arr1[i] == 22);
  }
  // copy assignment: empty to populated
  {
    minilib::indirect_array < int32_t > arr1;
    arr1.construct (3, 99);
    minilib::indirect_array < int32_t > arr2;
    arr1 = arr2;
    ASSERT_TRUE (arr1.size () == 0 && arr1.data () == nullptr);
  }
  return true;
}

constexpr bool test_indirect_array_move () {
  // move empty
  {
    minilib::indirect_array < int32_t > arr1;
    minilib::indirect_array < int32_t > arr2 (minilib::move (arr1));
    ASSERT_TRUE (arr1.size () == 0 && arr2.size () == 0);
  }
  // move populated
  {
    minilib::indirect_array < int32_t > arr1;
    arr1.construct (3, 55);
    int32_t * original_ptr = arr1.data ();
    minilib::indirect_array < int32_t > arr2 (minilib::move (arr1));
    ASSERT_TRUE (arr1.size () == 0 && arr1.data () == nullptr);
    ASSERT_TRUE (arr2.size () == 3 && arr2.data () == original_ptr);
    ASSERT_TRUE (arr2[0] == 55 && arr2[1] == 55 && arr2[2] == 55);
  }
  // move assignment: self
  {
    minilib::indirect_array < int32_t > arr;
    arr.construct (2, 33);
    arr = minilib::move (arr);
    ASSERT_TRUE (arr.size () == 2 && arr[0] == 33);
  }
  // move assignment: populated to populated
  {
    minilib::indirect_array < int32_t > arr1;
    arr1.construct (2, 1);
    minilib::indirect_array < int32_t > arr2;
    arr2.construct (3, 2);
    int32_t * ptr2 = arr2.data ();
    arr1 = minilib::move (arr2);
    ASSERT_TRUE (arr1.size () == 3 && arr1.data () == ptr2);
    ASSERT_TRUE (arr2.size () == 0 && arr2.data () == nullptr);
  }
  // move-only array
  {
    minilib::indirect_array < MoveOnly > arr1;
    arr1.generate (3, [](size_t i) { return MoveOnly (static_cast < int32_t > (i + 10)); });
    minilib::indirect_array < MoveOnly > arr2 (minilib::move (arr1));
    ASSERT_TRUE (arr1.size () == 0 && arr2.size () == 3);
    ASSERT_TRUE (arr2[0].val == 10 && arr2[1].val == 11 && arr2[2].val == 12);
  }
  return true;
}

constexpr bool test_indirect_array_destruct () {
  minilib::indirect_array < int32_t > arr;
  arr.construct (5, 77);
  ASSERT_TRUE (arr.size () == 5);
  arr.destruct ();
  ASSERT_TRUE (arr.size () == 0 && arr.data () == nullptr);
  // safe to call destruct again
  arr.destruct ();
  ASSERT_TRUE (arr.size () == 0);
  return true;
}

constexpr bool test_indirect_array_const_correctness () {
  minilib::indirect_array < int32_t > arr;
  arr.construct (3, 10);
  arr[1] = 20;

  const minilib::indirect_array < int32_t > & carr = arr;
  ASSERT_TRUE (carr.size () == 3);
  ASSERT_TRUE (carr[0] == 10 && carr[1] == 20 && carr[2] == 10);
  ASSERT_TRUE (carr.data () == arr.data ());
  ASSERT_TRUE (static_cast < bool > (carr));

  return true;
}

constexpr bool test_all () {
  ASSERT_TRUE (test_indirect_default_ctor ());
  ASSERT_TRUE (test_indirect_construct ());
  ASSERT_TRUE (test_indirect_copy ());
  ASSERT_TRUE (test_indirect_move ());
  ASSERT_TRUE (test_indirect_destruct ());
  ASSERT_TRUE (test_indirect_const_correctness ());

  ASSERT_TRUE (test_indirect_array_default_ctor ());
  ASSERT_TRUE (test_indirect_array_construct ());
  ASSERT_TRUE (test_indirect_array_copy ());
  ASSERT_TRUE (test_indirect_array_move ());
  ASSERT_TRUE (test_indirect_array_destruct ());
  ASSERT_TRUE (test_indirect_array_const_correctness ());
  return true;
}

static_assert (test_indirect_default_ctor ());
static_assert (test_indirect_construct ());
static_assert (test_indirect_copy ());
static_assert (test_indirect_move ());
static_assert (test_indirect_destruct ());
static_assert (test_indirect_const_correctness ());

static_assert (test_indirect_array_default_ctor ());
static_assert (test_indirect_array_construct ());
static_assert (test_indirect_array_copy ());
static_assert (test_indirect_array_move ());
static_assert (test_indirect_array_destruct ());
static_assert (test_indirect_array_const_correctness ());

static_assert (test_all (), "All indirect constexpr tests must pass");

constexpr bool all_passed = test_all ();

void main ([[maybe_unused]] void * sp) {
  if (!all_passed) exit (1);
  exit (0);
}
