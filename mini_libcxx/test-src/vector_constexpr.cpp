#include <stdint.h>
#include <exit.h>
#include <vector.hpp>
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

// 1. Default constructor
constexpr bool test_default_ctor () {
  minilib::vector < int32_t > v;
  ASSERT_TRUE (v.size () == 0);
  ASSERT_TRUE (v.capacity () == 0);
  ASSERT_TRUE (v.data () == nullptr);
  ASSERT_TRUE (!v);
  ASSERT_TRUE (!static_cast < bool > (v));
  return true;
}

// 2. Copy constructor
constexpr bool test_copy_ctor () {
  // Empty copy
  {
    minilib::vector < int32_t > v1;
    minilib::vector < int32_t > v2 (v1);
    ASSERT_TRUE (v2.size () == 0);
    ASSERT_TRUE (v2.capacity () == 0);
    ASSERT_TRUE (v2.data () == nullptr);
  }
  // Non-empty trivial copy
  {
    minilib::vector < int32_t > v1;
    v1.push_back (1);
    v1.push_back (2);
    v1.push_back (3);
    minilib::vector < int32_t > v2 (v1);
    ASSERT_TRUE (v2.size () == 3);
    ASSERT_TRUE (v2.capacity () == 3);
    ASSERT_TRUE (v2[0] == 1 && v2[1] == 2 && v2[2] == 3);
    // Deep copy verification
    v1[0] = 99;
    ASSERT_TRUE (v2[0] == 1);
  }
  // Non-empty non-trivial copy
  {
    minilib::vector < NonTrivial > v1;
    v1.emplace_back (1, 10);
    v1.emplace_back (2, 20);
    minilib::vector < NonTrivial > v2 (v1);
    ASSERT_TRUE (v2.size () == 2);
    ASSERT_TRUE (v2[0].val == 10 && v2[1].val == 20);
    // Copy constructor adds 100 to id
    ASSERT_TRUE (v2[0].id == 101 && v2[1].id == 102);
  }
  return true;
}

// 3. Move constructor
constexpr bool test_move_ctor () {
  // Empty move
  {
    minilib::vector < int32_t > v1;
    minilib::vector < int32_t > v2 (minilib::move (v1));
    ASSERT_TRUE (v2.size () == 0 && v2.capacity () == 0 && v2.data () == nullptr);
    ASSERT_TRUE (v1.size () == 0 && v1.capacity () == 0 && v1.data () == nullptr);
  }
  // Non-empty move
  {
    minilib::vector < int32_t > v1;
    v1.push_back (10);
    v1.push_back (20);
    size_t old_cap = v1.capacity ();
    int32_t * old_data = v1.data ();

    minilib::vector < int32_t > v2 (minilib::move (v1));
    ASSERT_TRUE (v2.size () == 2);
    ASSERT_TRUE (v2.capacity () == old_cap);
    ASSERT_TRUE (v2.data () == old_data);
    ASSERT_TRUE (v2[0] == 10 && v2[1] == 20);

    ASSERT_TRUE (v1.size () == 0);
    ASSERT_TRUE (v1.capacity () == 0);
    ASSERT_TRUE (v1.data () == nullptr);
  }
  return true;
}

// 4. Copy assignment
constexpr bool test_copy_assignment () {
  // Self assignment
  {
    minilib::vector < int32_t > v;
    v.push_back (1);
    v.push_back (2);
    v = v;
    ASSERT_TRUE (v.size () == 2);
    ASSERT_TRUE (v[0] == 1 && v[1] == 2);
  }
  // other.len <= alloc_len (no reallocation)
  // Case 1: other.len == 0
  {
    minilib::vector < int32_t > v1;
    v1.push_back (1);
    v1.push_back (2);
    minilib::vector < int32_t > v2;
    v1 = v2;
    ASSERT_TRUE (v1.size () == 0);
  }
  // Case 2: len < other.len <= alloc_len
  {
    minilib::vector < int32_t > v1;
    v1.reserve (10);
    v1.push_back (1);
    minilib::vector < int32_t > v2;
    v2.push_back (10);
    v2.push_back (20);
    v2.push_back (30);
    v1 = v2;
    ASSERT_TRUE (v1.size () == 3);
    ASSERT_TRUE (v1[0] == 10 && v1[1] == 20 && v1[2] == 30);
  }
  // Case 3: len > other.len
  {
    minilib::vector < int32_t > v1;
    v1.push_back (1);
    v1.push_back (2);
    v1.push_back (3);
    minilib::vector < int32_t > v2;
    v2.push_back (100);
    v1 = v2;
    ASSERT_TRUE (v1.size () == 1);
    ASSERT_TRUE (v1[0] == 100);
  }
  // other.len > alloc_len (reallocation)
  {
    minilib::vector < int32_t > v1;
    v1.push_back (1);
    minilib::vector < int32_t > v2;
    for (int32_t i = 0; i < 20; i++) v2.push_back (i);
    v1 = v2;
    ASSERT_TRUE (v1.size () == 20);
    for (int32_t i = 0; i < 20; i++) ASSERT_TRUE (v1[i] == i);
  }
  // Non-trivial copy assignment
  {
    minilib::vector < NonTrivial > v1;
    v1.reserve (8);
    v1.emplace_back (1, 10);
    minilib::vector < NonTrivial > v2;
    v2.emplace_back (2, 20);
    v2.emplace_back (3, 30);
    v1 = v2;
    ASSERT_TRUE (v1.size () == 2);
    ASSERT_TRUE (v1[0].val == 20 && v1[1].val == 30);
  }
  return true;
}

// 5. Move assignment
constexpr bool test_move_assignment () {
  // Self assignment
  {
    minilib::vector < int32_t > v;
    v.push_back (1);
    v = minilib::move (v);
    ASSERT_TRUE (v.size () == 1 && v[0] == 1);
  }
  // Move assignment into non-empty vector
  {
    minilib::vector < int32_t > v1;
    v1.push_back (1);
    v1.push_back (2);
    minilib::vector < int32_t > v2;
    v2.push_back (10);
    v2.push_back (20);
    v2.push_back (30);
    int32_t * old_data = v2.data ();
    size_t old_cap = v2.capacity ();

    v1 = minilib::move (v2);
    ASSERT_TRUE (v1.size () == 3);
    ASSERT_TRUE (v1.capacity () == old_cap);
    ASSERT_TRUE (v1.data () == old_data);
    ASSERT_TRUE (v1[0] == 10 && v1[1] == 20 && v1[2] == 30);

    ASSERT_TRUE (v2.size () == 0 && v2.capacity () == 0 && v2.data () == nullptr);
  }
  return true;
}

// 6. Clear and Storage (reserve, resize_storage, clear)
constexpr bool test_clear_and_storage () {
  minilib::vector < int32_t > v;
  // Reserve on empty
  v.reserve (12);
  ASSERT_TRUE (v.capacity () >= 12);
  ASSERT_TRUE (v.size () == 0);
  ASSERT_TRUE (v.data () != nullptr);

  // Reserve smaller does nothing
  size_t cap = v.capacity ();
  v.reserve (5);
  ASSERT_TRUE (v.capacity () == cap);

  // Push elements
  for (int32_t i = 0; i < 5; i++) v.push_back (i * 10);
  ASSERT_TRUE (v.size () == 5);

  // clear
  v.clear ();
  ASSERT_TRUE (v.size () == 0);
  ASSERT_TRUE (v.capacity () == cap);
  ASSERT_TRUE (!v);

  // clear on empty is no-op
  v.clear ();
  ASSERT_TRUE (v.size () == 0);

  // resize_storage:
  // expand
  v.push_back (100);
  v.push_back (200);
  v.resize_storage (30);
  ASSERT_TRUE (v.capacity () == 30);
  ASSERT_TRUE (v.size () == 2);
  ASSERT_TRUE (v[0] == 100 && v[1] == 200);

  // shrink with truncation
  v.push_back (300);
  v.push_back (400);
  // currently size == 4 [100, 200, 300, 400]
  v.resize_storage (3);
  ASSERT_TRUE (v.capacity () == 3);
  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[0] == 100 && v[1] == 200 && v[2] == 300);

  // resize_storage to 0
  v.resize_storage (0);
  ASSERT_TRUE (v.size () == 0);
  ASSERT_TRUE (v.capacity () == 0);
  ASSERT_TRUE (v.data () == nullptr);

  return true;
}

// 7. Push back (lvalue, rvalue, self-reference, growth)
constexpr bool test_push_back () {
  minilib::vector < int32_t > v;
  int32_t x = 10;
  v.push_back (x); // lvalue
  v.push_back (20); // rvalue
  ASSERT_TRUE (v.size () == 2);
  ASSERT_TRUE (v.capacity () >= 8);
  ASSERT_TRUE (v[0] == 10 && v[1] == 20);

  // Fill up to capacity
  while (v.size () < v.capacity ()) {
    v.push_back (static_cast < int32_t > (v.size ()));
  }
  size_t full_cap = v.capacity ();
  ASSERT_TRUE (v.size () == full_cap);

  // Self-push_back edge case: passing element from current vector when reallocation triggers
  v.push_back (v[0]);
  ASSERT_TRUE (v.size () == full_cap + 1);
  ASSERT_TRUE (v.capacity () > full_cap);
  ASSERT_TRUE (v[full_cap] == 10);

  return true;
}

// 8. Push back many
constexpr bool test_push_back_many () {
  minilib::vector < int32_t > v;
  v.push_back (1);
  v.push_back_many (2, 0); // count == 0
  ASSERT_TRUE (v.size () == 1);

  // within capacity
  v.push_back_many (7, 3);
  ASSERT_TRUE (v.size () == 4);
  ASSERT_TRUE (v[0] == 1 && v[1] == 7 && v[2] == 7 && v[3] == 7);

  // requiring expansion
  v.push_back_many (9, 20);
  ASSERT_TRUE (v.size () == 24);
  for (size_t i = 4; i < 24; i++) ASSERT_TRUE (v[i] == 9);

  // self-reference with expansion
  v.push_back_many (v[0], 50);
  ASSERT_TRUE (v.size () == 74);
  for (size_t i = 24; i < 74; i++) ASSERT_TRUE (v[i] == 1);

  return true;
}

// 9. Emplace back and default
constexpr bool test_emplace_back () {
  minilib::vector < Point > v;
  v.emplace_back (3, 4);
  v.emplace_back (5, 6);
  ASSERT_TRUE (v.size () == 2);
  ASSERT_TRUE (v[0].x == 3 && v[0].y == 4);
  ASSERT_TRUE (v[1].x == 5 && v[1].y == 6);

  // emplace_back_default
  minilib::vector < DefaultInitProbe > vp;
  vp.emplace_back_default ();
  ASSERT_TRUE (vp.size () == 1);
  ASSERT_TRUE (vp[0].val == 12345);

  // emplace_back_many
  vp.emplace_back_many (3, 777);
  ASSERT_TRUE (vp.size () == 4);
  ASSERT_TRUE (vp[1].val == 777 && vp[2].val == 777 && vp[3].val == 777);

  // emplace_back_many_default
  vp.emplace_back_many_default (2);
  ASSERT_TRUE (vp.size () == 6);
  ASSERT_TRUE (vp[4].val == 12345 && vp[5].val == 12345);

  return true;
}

// 10. Inform emplace / pop back many
constexpr bool test_inform_methods () {
  minilib::vector < int32_t > v;
  v.reserve (10);
  minilib::construct_at (v.data() + v.size (), 101);
  minilib::construct_at (v.data() + v.size () + 1, 102);
  minilib::construct_at (v.data() + v.size () + 2, 103);
  v.inform_emplace_back_many (3);

  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[0] == 101 && v[1] == 102 && v[2] == 103);

  // inform_pop_back_many
  minilib::destroy_at (v.data () + v.size () - 1);
  minilib::destroy_at (v.data () + v.size () - 2);
  v.inform_pop_back_many (2);

  ASSERT_TRUE (v.size () == 1);
  ASSERT_TRUE (v[0] == 101);

  return true;
}

// 11. Generate
constexpr bool test_generate () {
  minilib::vector < int32_t > v;
  v.push_back (999);
  v.generate (4, [](size_t i) { return static_cast < int32_t > ((i + 1) * 10); });
  ASSERT_TRUE (v.size () == 5);
  ASSERT_TRUE (v[0] == 999);
  ASSERT_TRUE (v[1] == 10 && v[2] == 20 && v[3] == 30 && v[4] == 40);

  // Multi-argument generator for Point
  minilib::vector < Point > vp;
  vp.generate (3, [](size_t i) { return static_cast < int32_t > (i); },
                  [](size_t i) { return static_cast < int32_t > (i + 100); });
  ASSERT_TRUE (vp.size () == 3);
  ASSERT_TRUE (vp[0].x == 0 && vp[0].y == 100);
  ASSERT_TRUE (vp[1].x == 1 && vp[1].y == 101);
  ASSERT_TRUE (vp[2].x == 2 && vp[2].y == 102);

  return true;
}

// 12. Pop back
constexpr bool test_pop_back () {
  minilib::vector < int32_t > v;
  v.push_back (10);
  v.push_back (20);
  v.push_back (30);
  v.push_back (40);
  v.push_back (50);

  v.pop_back ();
  ASSERT_TRUE (v.size () == 4);
  ASSERT_TRUE (v[3] == 40);

  v.pop_back_many (0);
  ASSERT_TRUE (v.size () == 4);

  v.pop_back_many (2);
  ASSERT_TRUE (v.size () == 2);
  ASSERT_TRUE (v[0] == 10 && v[1] == 20);

  v.pop_back_many (2);
  ASSERT_TRUE (v.size () == 0);

  // Non-trivial pop_back_many
  minilib::vector < NonTrivial > vn;
  vn.emplace_back (1, 10);
  vn.emplace_back (2, 20);
  vn.emplace_back (3, 30);
  vn.pop_back ();
  ASSERT_TRUE (vn.size () == 2);
  vn.pop_back_many (2);
  ASSERT_TRUE (vn.size () == 0);

  return true;
}

// 13. Element access and const correctness
constexpr bool test_element_access () {
  minilib::vector < int32_t > v;
  v.push_back (11);
  v.push_back (22);

  // Mutable access
  v[0] = 111;
  ASSERT_TRUE (v[0] == 111);
  *v.data () = 112;
  ASSERT_TRUE (v[0] == 112);

  // Const access
  const minilib::vector < int32_t > & cv = v;
  ASSERT_TRUE (cv.size () == 2);
  ASSERT_TRUE (cv[0] == 112 && cv[1] == 22);
  ASSERT_TRUE (cv.data ()[0] == 112);
  ASSERT_TRUE (static_cast < bool > (cv));

  return true;
}

// 14. Shift by
constexpr bool test_shift_by () {
  minilib::vector < int32_t > v;
  v.push_back (1);
  v.push_back (2);
  v.push_back (3);

  // shift_by at end (index == len)
  v.shift_by (3, 2);
  minilib::construct_at (v.data () + 3, 4);
  minilib::construct_at (v.data () + 4, 5);
  ASSERT_TRUE (v.size () == 5);
  ASSERT_TRUE (v[0] == 1 && v[1] == 2 && v[2] == 3 && v[3] == 4 && v[4] == 5);

  // shift_by with shift == 0
  v.shift_by (1, 0);
  ASSERT_TRUE (v.size () == 5);

  // shift_by in middle: index=1, shift=2 -> [0] unchanged, [1, 2] shifted to [3, 4]
  v.shift_by (1, 2);
  // gap is [1, 3). Old elements moved to [3, 7).
  // Fill gap:
  // Note: in shift_by, locations [1, 3) were moved from, so we can assign to them
  v[1] = 88;
  v[2] = 99;
  ASSERT_TRUE (v.size () == 7);
  ASSERT_TRUE (v[0] == 1 && v[1] == 88 && v[2] == 99 && v[3] == 2 && v[4] == 3 && v[5] == 4 && v[6] == 5);

  return true;
}

// 15. Insert (single element)
constexpr bool test_insert () {
  minilib::vector < int32_t > v;
  v.push_back (10);
  v.push_back (30);

  // insert in middle
  int32_t val20 = 20;
  v.insert_no_invalidate (1, val20); // lvalue
  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[0] == 10 && v[1] == 20 && v[2] == 30);

  // insert at front (index 0)
  v.insert_no_invalidate (0, 5); // rvalue
  ASSERT_TRUE (v.size () == 4);
  ASSERT_TRUE (v[0] == 5 && v[1] == 10 && v[2] == 20 && v[3] == 30);

  // insert at back (index == len)
  v.insert_no_invalidate (4, 40);
  ASSERT_TRUE (v.size () == 5);
  ASSERT_TRUE (v[4] == 40);

  // self-reference insert
  v.insert_no_invalidate (2, v[0]);
  ASSERT_TRUE (v.size () == 6);
  ASSERT_TRUE (v[2] == 5);

  return true;
}

// 16. Insert many
constexpr bool test_insert_many () {
  minilib::vector < int32_t > v;
  v.push_back (1);
  v.push_back (4);

  // count == 0
  v.insert_many_no_invalidate (1, 0, 99);
  ASSERT_TRUE (v.size () == 2);

  // index + count <= old_len
  v.insert_many_no_invalidate (1, 1, 2);
  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[0] == 1 && v[1] == 2 && v[2] == 4);

  // index + count > old_len
  // old_len is 3, index is 2, count is 3 -> index + count is 5 > 3
  v.insert_many_no_invalidate (2, 3, 3);
  ASSERT_TRUE (v.size () == 6);
  ASSERT_TRUE (v[0] == 1 && v[1] == 2 && v[2] == 3 && v[3] == 3 && v[4] == 3 && v[5] == 4);

  // at back (index == len)
  v.insert_many_no_invalidate (6, 2, 5);
  ASSERT_TRUE (v.size () == 8);
  ASSERT_TRUE (v[6] == 5 && v[7] == 5);

  return true;
}

// 17. Emplace and default at index
constexpr bool test_emplace_at_index () {
  minilib::vector < Point > v;
  v.emplace_back (1, 1);
  v.emplace_back (3, 3);

  // emplace in middle
  v.emplace_no_invalidate (1, 2, 2);
  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[0].x == 1 && v[1].x == 2 && v[2].x == 3);

  // emplace at back
  v.emplace_no_invalidate (3, 4, 4);
  ASSERT_TRUE (v.size () == 4);
  ASSERT_TRUE (v[3].x == 4);

  // emplace at front
  v.emplace_no_invalidate (0, 0, 0);
  ASSERT_TRUE (v.size () == 5);
  ASSERT_TRUE (v[0].x == 0);

  // emplace_default
  minilib::vector < DefaultInitProbe > vd;
  vd.emplace_back (10);
  vd.emplace_back (30);
  vd.emplace_default (1);
  ASSERT_TRUE (vd.size () == 3);
  ASSERT_TRUE (vd[0].val == 10 && vd[1].val == 12345 && vd[2].val == 30);

  // emplace_many
  // index + count <= old_len
  minilib::vector < Point > vp;
  vp.emplace_back (1, 1);
  vp.emplace_back (2, 2);
  vp.emplace_back (5, 5);
  vp.emplace_many_no_invalidate (2, 1, 9, 9);
  ASSERT_TRUE (vp.size () == 4);
  ASSERT_TRUE (vp[2].x == 9 && vp[3].x == 5);

  // index + count > old_len
  vp.emplace_many_no_invalidate (3, 2, 8, 8);
  ASSERT_TRUE (vp.size () == 6);
  ASSERT_TRUE (vp[3].x == 8 && vp[4].x == 8 && vp[5].x == 5);

  // emplace_many_default
  minilib::vector < DefaultInitProbe > vde;
  vde.emplace_back (1);
  vde.emplace_back (2);
  vde.emplace_many_default (1, 2);
  ASSERT_TRUE (vde.size () == 4);
  ASSERT_TRUE (vde[0].val == 1 && vde[1].val == 12345 && vde[2].val == 12345 && vde[3].val == 2);

  // generate_at
  minilib::vector < int32_t > vg;
  vg.push_back (1);
  vg.push_back (5);
  vg.generate_at_no_invalidate (1, 3, [](size_t i) { return static_cast < int32_t > (i + 2); });
  ASSERT_TRUE (vg.size () == 5);
  ASSERT_TRUE (vg[0] == 1 && vg[1] == 2 && vg[2] == 3 && vg[3] == 4 && vg[4] == 5);

  return true;
}

// 17b. Insert (may_invalidate)
constexpr bool test_insert_may_invalidate () {
  minilib::vector < int32_t > v;
  v.push_back (10);
  v.push_back (30);

  int32_t val20 = 20;
  v.insert_may_invalidate (1, val20); // lvalue
  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[0] == 10 && v[1] == 20 && v[2] == 30);

  v.insert_may_invalidate (0, 5); // rvalue
  ASSERT_TRUE (v.size () == 4);
  ASSERT_TRUE (v[0] == 5 && v[1] == 10 && v[2] == 20 && v[3] == 30);

  v.insert_may_invalidate (4, 40); // at len
  ASSERT_TRUE (v.size () == 5);
  ASSERT_TRUE (v[4] == 40);

  // insert causing reallocation
  while (v.size () < v.capacity ()) {
    v.push_back (100);
  }
  size_t full_cap = v.capacity ();
  v.insert_may_invalidate (1, 999);
  ASSERT_TRUE (v.size () == full_cap + 1);
  ASSERT_TRUE (v.capacity () > full_cap);
  ASSERT_TRUE (v[1] == 999);

  return true;
}

// 17c. Insert many (may_invalidate)
constexpr bool test_insert_many_may_invalidate () {
  minilib::vector < int32_t > v;
  v.push_back (1);
  v.push_back (4);

  v.insert_many_may_invalidate (1, 0, 99); // count == 0
  ASSERT_TRUE (v.size () == 2);

  v.insert_many_may_invalidate (1, 1, 2);
  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[0] == 1 && v[1] == 2 && v[2] == 4);

  v.insert_many_may_invalidate (2, 3, 3);
  ASSERT_TRUE (v.size () == 6);
  ASSERT_TRUE (v[0] == 1 && v[1] == 2 && v[2] == 3 && v[3] == 3 && v[4] == 3 && v[5] == 4);

  v.insert_many_may_invalidate (6, 2, 5);
  ASSERT_TRUE (v.size () == 8);
  ASSERT_TRUE (v[6] == 5 && v[7] == 5);

  // insert_many requiring reallocation
  v.insert_many_may_invalidate (1, 20, 77);
  ASSERT_TRUE (v.size () == 28);
  for (size_t i = 1; i < 21; i++) {
    ASSERT_TRUE (v[i] == 77);
  }

  return true;
}

// 17d. Emplace and generate_at (may_invalidate)
constexpr bool test_emplace_may_invalidate_at_index () {
  minilib::vector < Point > v;
  v.emplace_back (1, 1);
  v.emplace_back (3, 3);

  v.emplace_may_invalidate (1, 2, 2);
  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[0].x == 1 && v[1].x == 2 && v[2].x == 3);

  v.emplace_may_invalidate (3, 4, 4);
  ASSERT_TRUE (v.size () == 4);
  ASSERT_TRUE (v[3].x == 4);

  v.emplace_may_invalidate (0, 0, 0);
  ASSERT_TRUE (v.size () == 5);
  ASSERT_TRUE (v[0].x == 0);

  // emplace_may_invalidate requiring reallocation
  while (v.size () < v.capacity ()) {
    v.emplace_back (9, 9);
  }
  size_t full_cap = v.capacity ();
  v.emplace_may_invalidate (2, 88, 88);
  ASSERT_TRUE (v.size () == full_cap + 1);
  ASSERT_TRUE (v.capacity () > full_cap);
  ASSERT_TRUE (v[2].x == 88 && v[2].y == 88);

  minilib::vector < Point > vp;
  vp.emplace_back (1, 1);
  vp.emplace_back (5, 5);
  vp.emplace_many_may_invalidate (1, 0, 9, 9);
  ASSERT_TRUE (vp.size () == 2);

  vp.emplace_many_may_invalidate (1, 2, 2, 2);
  ASSERT_TRUE (vp.size () == 4);
  ASSERT_TRUE (vp[0].x == 1 && vp[1].x == 2 && vp[2].x == 2 && vp[3].x == 5);

  vp.emplace_many_may_invalidate (2, 20, 7, 7);
  ASSERT_TRUE (vp.size () == 24);
  for (size_t i = 2; i < 22; i++) {
    ASSERT_TRUE (vp[i].x == 7 && vp[i].y == 7);
  }

  minilib::vector < int32_t > vg;
  vg.push_back (1);
  vg.push_back (5);
  vg.generate_at_may_invalidate (1, 0, [](size_t i) { return static_cast < int32_t > (i); });
  ASSERT_TRUE (vg.size () == 2);

  vg.generate_at_may_invalidate (1, 3, [](size_t i) { return static_cast < int32_t > (i + 2); });
  ASSERT_TRUE (vg.size () == 5);
  ASSERT_TRUE (vg[0] == 1 && vg[1] == 2 && vg[2] == 3 && vg[3] == 4 && vg[4] == 5);

  vg.generate_at_may_invalidate (2, 20, [](size_t i) { return static_cast < int32_t > (i + 50); });
  ASSERT_TRUE (vg.size () == 25);
  for (size_t i = 2; i < 22; i++) {
    ASSERT_TRUE (vg[i] == static_cast < int32_t > (i - 2 + 50));
  }

  return true;
}

// 18. Erase
constexpr bool test_erase () {
  minilib::vector < int32_t > v;
  for (int32_t i = 0; i < 6; i++) v.push_back (i);

  // erase single in middle: erase 2 -> [0, 1, 3, 4, 5]
  v.erase (2);
  ASSERT_TRUE (v.size () == 5);
  ASSERT_TRUE (v[0] == 0 && v[1] == 1 && v[2] == 3 && v[3] == 4 && v[4] == 5);

  // erase single at front: erase 0 -> [1, 3, 4, 5]
  v.erase (0);
  ASSERT_TRUE (v.size () == 4);
  ASSERT_TRUE (v[0] == 1 && v[1] == 3 && v[2] == 4 && v[3] == 5);

  // erase single at back: erase 3 -> [1, 3, 4]
  v.erase (3);
  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[0] == 1 && v[1] == 3 && v[2] == 4);

  // erase range: count == 0
  v.erase (1, 0);
  ASSERT_TRUE (v.size () == 3);

  // erase range in middle
  v.push_back (5);
  v.push_back (6);
  // v is [1, 3, 4, 5, 6], erase index 1 count 2 -> [1, 5, 6]
  v.erase (1, 2);
  ASSERT_TRUE (v.size () == 3);
  ASSERT_TRUE (v[0] == 1 && v[1] == 5 && v[2] == 6);

  // erase all
  v.erase (0, 3);
  ASSERT_TRUE (v.size () == 0);

  return true;
}

// 19. Move-only type support
constexpr bool test_move_only () {
  minilib::vector < MoveOnly > v;
  v.push_back (MoveOnly (10));
  v.emplace_back (20);
  v.insert_no_invalidate (1, MoveOnly (15));
  v.emplace_no_invalidate (0, 5);
  v.insert_may_invalidate (2, MoveOnly (12));
  v.emplace_may_invalidate (1, 7);
  ASSERT_TRUE (v.size () == 6);
  ASSERT_TRUE (v[0].val == 5 && v[1].val == 7 && v[2].val == 10 && v[3].val == 12 && v[4].val == 15 && v[5].val == 20);

  v.pop_back ();
  v.pop_back ();
  v.pop_back ();
  v.pop_back ();
  ASSERT_TRUE (v.size () == 2);
  ASSERT_TRUE (v[0].val == 5 && v[1].val == 7);

  v.erase (1);
  ASSERT_TRUE (v.size () == 1);
  ASSERT_TRUE (v[0].val == 5);
  v.push_back (MoveOnly (15));
  ASSERT_TRUE (v.size () == 2);
  ASSERT_TRUE (v[0].val == 5 && v[1].val == 15);

  // Move constructor
  minilib::vector < MoveOnly > v2 (minilib::move (v));
  ASSERT_TRUE (v2.size () == 2 && v.size () == 0);
  ASSERT_TRUE (v2[0].val == 5 && v2[1].val == 15);

  // Move assignment
  minilib::vector < MoveOnly > v3;
  v3 = minilib::move (v2);
  ASSERT_TRUE (v3.size () == 2 && v2.size () == 0);
  ASSERT_TRUE (v3[0].val == 5 && v3[1].val == 15);

  v3.reserve (20);
  ASSERT_TRUE (v3.capacity () >= 20);
  ASSERT_TRUE (v3[0].val == 5 && v3[1].val == 15);

  v3.clear ();
  ASSERT_TRUE (v3.size () == 0);

  return true;
}

// 20. Non-trivial lifecycle in constexpr
constexpr bool test_nontrivial_lifecycle () {
  minilib::vector < NonTrivial > v;
  v.emplace_back (1, 100);
  v.push_back (NonTrivial (2, 200));
  v.emplace_no_invalidate (1, 3, 300);
  ASSERT_TRUE (v.size () == 3);

  v.erase (1);
  ASSERT_TRUE (v.size () == 2);

  v.resize_storage (10);
  ASSERT_TRUE (v.size () == 2);
  ASSERT_TRUE (v.capacity () == 10);

  v.clear ();
  ASSERT_TRUE (v.size () == 0);

  return true;
}

constexpr bool test_all () {
  ASSERT_TRUE (test_default_ctor ());
  ASSERT_TRUE (test_copy_ctor ());
  ASSERT_TRUE (test_move_ctor ());
  ASSERT_TRUE (test_copy_assignment ());
  ASSERT_TRUE (test_move_assignment ());
  ASSERT_TRUE (test_clear_and_storage ());
  ASSERT_TRUE (test_push_back ());
  ASSERT_TRUE (test_push_back_many ());
  ASSERT_TRUE (test_emplace_back ());
  ASSERT_TRUE (test_inform_methods ());
  ASSERT_TRUE (test_generate ());
  ASSERT_TRUE (test_pop_back ());
  ASSERT_TRUE (test_element_access ());
  ASSERT_TRUE (test_shift_by ());
  ASSERT_TRUE (test_insert ());
  ASSERT_TRUE (test_insert_may_invalidate ());
  ASSERT_TRUE (test_insert_many ());
  ASSERT_TRUE (test_insert_many_may_invalidate ());
  ASSERT_TRUE (test_emplace_at_index ());
  ASSERT_TRUE (test_emplace_may_invalidate_at_index ());
  ASSERT_TRUE (test_erase ());
  ASSERT_TRUE (test_move_only ());
  ASSERT_TRUE (test_nontrivial_lifecycle ());
  return true;
}

static_assert (test_default_ctor ());
static_assert (test_copy_ctor ());
static_assert (test_move_ctor ());
static_assert (test_copy_assignment ());
static_assert (test_move_assignment ());
static_assert (test_clear_and_storage ());
static_assert (test_push_back ());
static_assert (test_push_back_many ());
static_assert (test_emplace_back ());
static_assert (test_inform_methods ());
static_assert (test_generate ());
static_assert (test_pop_back ());
static_assert (test_element_access ());
static_assert (test_shift_by ());
static_assert (test_insert ());
static_assert (test_insert_may_invalidate ());
static_assert (test_insert_many ());
static_assert (test_insert_many_may_invalidate ());
static_assert (test_emplace_at_index ());
static_assert (test_emplace_may_invalidate_at_index ());
static_assert (test_erase ());
static_assert (test_move_only ());
static_assert (test_nontrivial_lifecycle ());

constexpr bool all_passed = test_all ();

void main ([[maybe_unused]] void * sp) {
  if (!all_passed) exit (1);
  exit (0);
}
