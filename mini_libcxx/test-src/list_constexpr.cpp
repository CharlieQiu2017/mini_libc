#include <stdint.h>
#include <exit.h>
#include <counter.hpp>
#include <list.hpp>
#include <type_traits.hpp>
#include <utility.hpp>
#include <compare.hpp>

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

struct ConstexprTracker {
  int32_t id;
  int32_t val;
  int32_t * destruct_count;

  constexpr ConstexprTracker () : id (0), val (42), destruct_count (nullptr) {}
  constexpr ConstexprTracker (int32_t i, int32_t v, int32_t * dc = nullptr) : id (i), val (v), destruct_count (dc) {}
  constexpr ConstexprTracker (const ConstexprTracker & o) : id (o.id + 100), val (o.val), destruct_count (o.destruct_count) {}
  constexpr ConstexprTracker (ConstexprTracker && o) : id (o.id), val (o.val), destruct_count (o.destruct_count) {
    o.id = -1;
    o.val = -1;
  }
  constexpr ConstexprTracker & operator= (const ConstexprTracker & o) {
    id = o.id + 100;
    val = o.val;
    destruct_count = o.destruct_count;
    return *this;
  }
  constexpr ConstexprTracker & operator= (ConstexprTracker && o) {
    id = o.id;
    val = o.val;
    destruct_count = o.destruct_count;
    o.id = -1;
    o.val = -1;
    return *this;
  }
  constexpr ~ConstexprTracker () {
    if (destruct_count) (*destruct_count)++;
    id = -999;
    val = -999;
  }
};

struct MoveOnly {
  int32_t val;
  constexpr MoveOnly () : val (0) {}
  constexpr explicit MoveOnly (int32_t v) : val (v) {}
  MoveOnly (const MoveOnly &) = delete;
  MoveOnly & operator= (const MoveOnly &) = delete;
  constexpr MoveOnly (MoveOnly && o) : val (o.val) { o.val = -1; }
  constexpr MoveOnly & operator= (MoveOnly && o) {
    val = o.val;
    o.val = -1;
    return *this;
  }
  constexpr ~MoveOnly () {}
};

struct Probe {
  int32_t val;
  constexpr Probe () : val (42) {}
  constexpr explicit Probe (int32_t v) : val (v) {}

  friend constexpr void destroy_probe (Probe * p) { p->~Probe (); }
  friend constexpr void construct_probe (Probe * p, int32_t v) {
    ::new (static_cast < void * > (p)) Probe (v);
  }

private:
  constexpr ~Probe () {}
};

// 1. Default constructor and empty state
constexpr bool test_default_ctor (minilib::counter & ctr) {
  minilib::list < int32_t > l;
  l.attach_counter (minilib::addressof (ctr));
  ASSERT_TRUE (l.size () == 0);
  ASSERT_TRUE (! static_cast < bool > (l.front ()));
  ASSERT_TRUE (! static_cast < bool > (l.back ()));
  return true;
}

// 2. Push & emplace front and back
constexpr bool test_push_and_emplace_front_back (minilib::counter & ctr) {
  minilib::list < int32_t > l;
  l.attach_counter (minilib::addressof (ctr));

  // push_back
  auto h1 = l.push_back (10);
  ASSERT_TRUE (l.size () == 1);
  ASSERT_TRUE (*(l.data (h1)) == 10);
  ASSERT_TRUE (*(l.data (l.front ())) == 10);
  ASSERT_TRUE (*(l.data (l.back ())) == 10);

  int32_t val20 = 20;
  auto h2 = l.push_back (val20); // const ref overload
  ASSERT_TRUE (l.size () == 2);
  ASSERT_TRUE (*(l.data (h2)) == 20);
  ASSERT_TRUE (*(l.data (l.back ())) == 20);

  // push_front
  auto h0 = l.push_front (0);
  ASSERT_TRUE (l.size () == 3);
  ASSERT_TRUE (*(l.data (h0)) == 0);
  ASSERT_TRUE (*(l.data (l.front ())) == 0);

  int32_t val_neg = -10;
  auto h_neg = l.push_front (val_neg);
  ASSERT_TRUE (l.size () == 4);
  ASSERT_TRUE (*(l.data (h_neg)) == -10);
  ASSERT_TRUE (*(l.data (l.front ())) == -10);

  // emplace_front and emplace_back with non-trivial constructor
  minilib::list < Point > lp;
  lp.attach_counter (minilib::addressof (ctr));
  auto hp1 = lp.emplace_back (1, 2);
  ASSERT_TRUE (lp.size () == 1);
  ASSERT_TRUE (lp.data (hp1)->x == 1 && lp.data (hp1)->y == 2);

  auto hp0 = lp.emplace_front (3, 4);
  ASSERT_TRUE (lp.size () == 2);
  ASSERT_TRUE (lp.data (hp0)->x == 3 && lp.data (hp0)->y == 4);
  ASSERT_TRUE (lp.data (lp.front ())->x == 3);
  ASSERT_TRUE (lp.data (lp.back ())->x == 1);

  // emplace_default
  minilib::list < DefaultInitProbe > ld;
  ld.attach_counter (minilib::addressof (ctr));
  auto hd1 = ld.emplace_back_default ();
  ASSERT_TRUE (ld.data (hd1)->val == 12345);
  auto hd0 = ld.emplace_front_default ();
  ASSERT_TRUE (ld.data (hd0)->val == 12345);
  ASSERT_TRUE (ld.size () == 2);

  // emplace_null
  minilib::list < int32_t > ln;
  ln.attach_counter (minilib::addressof (ctr));
  auto hn1 = ln.emplace_back_null ();
  minilib::construct_at < int32_t > (ln.data (hn1), 99);
  ASSERT_TRUE (*(ln.data (hn1)) == 99);

  auto hn0 = ln.emplace_front_null ();
  minilib::construct_at < int32_t > (ln.data (hn0), 88);
  ASSERT_TRUE (*(ln.data (hn0)) == 88);
  ASSERT_TRUE (ln.size () == 2);
  ASSERT_TRUE (*(ln.data (ln.front ())) == 88);
  ASSERT_TRUE (*(ln.data (ln.back ())) == 99);

  return true;
}

// 3. Insert and emplace before & after
constexpr bool test_insert_and_emplace_before_after (minilib::counter & ctr) {
  minilib::list < int32_t > l;
  l.attach_counter (minilib::addressof (ctr));
  auto h2 = l.push_back (20);

  // insert_before on head -> new head
  auto h1 = l.insert_before (h2, 10);
  ASSERT_TRUE (l.size () == 2);
  ASSERT_TRUE (*(l.data (l.front ())) == 10);
  ASSERT_TRUE (*(l.data (h1)) == 10);

  // insert_after on tail -> new tail
  auto h4 = l.insert_after (h2, 40);
  ASSERT_TRUE (l.size () == 3);
  ASSERT_TRUE (*(l.data (l.back ())) == 40);

  // insert_before in middle
  int32_t v30 = 30;
  auto h3 = l.insert_before (h4, v30);
  ASSERT_TRUE (l.size () == 4);
  ASSERT_TRUE (*(l.data (h3)) == 30);

  // insert_after in middle
  auto h25 = l.insert_after (h2, 25);
  ASSERT_TRUE (l.size () == 5);
  ASSERT_TRUE (*(l.data (h25)) == 25);

  // Verify order: 10, 20, 25, 30, 40
  auto cur = l.front ();
  ASSERT_TRUE (*(l.data (cur)) == 10);
  cur = l.next (cur);
  ASSERT_TRUE (*(l.data (cur)) == 20);
  cur = l.next (cur);
  ASSERT_TRUE (*(l.data (cur)) == 25);
  cur = l.next (cur);
  ASSERT_TRUE (*(l.data (cur)) == 30);
  cur = l.next (cur);
  ASSERT_TRUE (*(l.data (cur)) == 40);
  cur = l.next (cur);
  ASSERT_TRUE (! static_cast < bool > (cur));

  // emplace_before and emplace_after with multi-arg Point
  minilib::list < Point > lp;
  lp.attach_counter (minilib::addressof (ctr));
  auto hp2 = lp.push_back (Point (2, 2));
  auto hp1 = lp.emplace_before (hp2, 1, 1);
  auto hp3 = lp.emplace_after (hp2, 3, 3);
  ASSERT_TRUE (lp.size () == 3);
  ASSERT_TRUE (lp.data (hp1)->x == 1);
  ASSERT_TRUE (lp.data (hp2)->x == 2);
  ASSERT_TRUE (lp.data (hp3)->x == 3);

  // emplace_before_default and emplace_after_default
  minilib::list < DefaultInitProbe > ld;
  ld.attach_counter (minilib::addressof (ctr));
  auto hd1 = ld.push_back (DefaultInitProbe (1));
  auto hd0 = ld.emplace_before_default (hd1);
  auto hd2 = ld.emplace_after_default (hd1);
  ASSERT_TRUE (ld.size () == 3);
  ASSERT_TRUE (ld.data (hd0)->val == 12345);
  ASSERT_TRUE (ld.data (hd1)->val == 1);
  ASSERT_TRUE (ld.data (hd2)->val == 12345);

  // emplace_before_null and emplace_after_null
  minilib::list < int32_t > ln;
  ln.attach_counter (minilib::addressof (ctr));
  auto hmid = ln.push_back (50);
  auto hpre = ln.emplace_before_null (hmid);
  minilib::construct_at < int32_t > (ln.data (hpre), 40);
  auto hpost = ln.emplace_after_null (hmid);
  minilib::construct_at < int32_t > (ln.data (hpost), 60);
  ASSERT_TRUE (ln.size () == 3);
  ASSERT_TRUE (*(ln.data (hpre)) == 40);
  ASSERT_TRUE (*(ln.data (hmid)) == 50);
  ASSERT_TRUE (*(ln.data (hpost)) == 60);

  return true;
}

// 4. Traversal and data access (non-const and const)
constexpr bool test_traversal_and_data_access (minilib::counter & ctr) {
  minilib::list < int32_t > l;
  l.attach_counter (minilib::addressof (ctr));
  for (int32_t i = 1; i <= 5; i++) {
    l.push_back (i * 10);
  }

  // Forward traversal
  int32_t expected = 10;
  auto it = l.front ();
  while (it) {
    ASSERT_TRUE (*(l.data (it)) == expected);
    expected += 10;
    it = l.next (it);
  }
  ASSERT_TRUE (expected == 60);

  // Backward traversal
  expected = 50;
  it = l.back ();
  while (it) {
    ASSERT_TRUE (*(l.data (it)) == expected);
    expected -= 10;
    it = l.prev (it);
  }
  ASSERT_TRUE (expected == 0);

  // Mutating elements through non-const data()
  it = l.front ();
  *(l.data (it)) = 100;
  ASSERT_TRUE (*(l.data (l.front ())) == 100);

  // Const list access
  const minilib::list < int32_t > & cl = l;
  ASSERT_TRUE (cl.size () == 5);
  auto ch = cl.front ();
  const int32_t * val_ptr = cl.data (ch);
  ASSERT_TRUE (*val_ptr == 100);

  auto ch_next = cl.next (ch);
  ASSERT_TRUE (*(cl.data (ch_next)) == 20);
  auto ch_prev = cl.prev (ch_next);
  ASSERT_TRUE (*(cl.data (ch_prev)) == 100);
  ASSERT_TRUE (! static_cast < bool > (cl.prev (ch)));
  ASSERT_TRUE (! static_cast < bool > (cl.next (cl.back ())));

  return true;
}

// 5. Pop front and pop back
constexpr bool test_pop_front_and_pop_back (minilib::counter & ctr) {
  minilib::list < int32_t > l;
  l.attach_counter (minilib::addressof (ctr));
  l.push_back (1);
  l.push_back (2);
  l.push_back (3);

  // pop_front
  l.pop_front ();
  ASSERT_TRUE (l.size () == 2);
  ASSERT_TRUE (*(l.data (l.front ())) == 2);
  ASSERT_TRUE (*(l.data (l.back ())) == 3);

  // pop_back
  l.pop_back ();
  ASSERT_TRUE (l.size () == 1);
  ASSERT_TRUE (*(l.data (l.front ())) == 2);
  ASSERT_TRUE (*(l.data (l.back ())) == 2);

  // Pop last remaining element
  l.pop_front ();
  ASSERT_TRUE (l.size () == 0);
  ASSERT_TRUE (! static_cast < bool > (l.front ()));
  ASSERT_TRUE (! static_cast < bool > (l.back ()));

  // Test single-element pop_back
  l.push_back (42);
  ASSERT_TRUE (l.size () == 1);
  l.pop_back ();
  ASSERT_TRUE (l.size () == 0);
  ASSERT_TRUE (! static_cast < bool > (l.front ()));
  ASSERT_TRUE (! static_cast < bool > (l.back ()));

  return true;
}

// 6. Remove node by handle
constexpr bool test_remove_by_handle (minilib::counter & ctr) {
  // Remove middle element
  {
    minilib::list < int32_t > l;
    l.attach_counter (minilib::addressof (ctr));
    auto h1 = l.push_back (1);
    auto h2 = l.push_back (2);
    auto h3 = l.push_back (3);

    l.remove (h2);
    ASSERT_TRUE (l.size () == 2);
    ASSERT_TRUE (*(l.data (l.front ())) == 1);
    ASSERT_TRUE (*(l.data (l.back ())) == 3);
    ASSERT_TRUE (*(l.data (l.next (h1))) == 3);
    ASSERT_TRUE (*(l.data (l.prev (h3))) == 1);
  }

  // Remove head element
  {
    minilib::list < int32_t > l;
    l.attach_counter (minilib::addressof (ctr));
    auto h1 = l.push_back (1);
    auto h2 = l.push_back (2);

    l.remove (h1);
    ASSERT_TRUE (l.size () == 1);
    ASSERT_TRUE (*(l.data (l.front ())) == 2);
    ASSERT_TRUE (*(l.data (l.back ())) == 2);
    ASSERT_TRUE (! static_cast < bool > (l.prev (h2)));
  }

  // Remove tail element
  {
    minilib::list < int32_t > l;
    l.attach_counter (minilib::addressof (ctr));
    auto h1 = l.push_back (1);
    auto h2 = l.push_back (2);

    l.remove (h2);
    ASSERT_TRUE (l.size () == 1);
    ASSERT_TRUE (*(l.data (l.front ())) == 1);
    ASSERT_TRUE (*(l.data (l.back ())) == 1);
    ASSERT_TRUE (! static_cast < bool > (l.next (h1)));
  }

  // Remove solitary element
  {
    minilib::list < int32_t > l;
    l.attach_counter (minilib::addressof (ctr));
    auto h1 = l.push_back (99);

    l.remove (h1);
    ASSERT_TRUE (l.size () == 0);
    ASSERT_TRUE (! static_cast < bool > (l.front ()));
    ASSERT_TRUE (! static_cast < bool > (l.back ()));
  }

  return true;
}

// 7. Clear and repopulate
constexpr bool test_clear_and_repopulate (minilib::counter & ctr) {
  minilib::list < int32_t > l;
  l.attach_counter (minilib::addressof (ctr));
  l.clear (); // Clear on empty list
  ASSERT_TRUE (l.size () == 0);

  l.push_back (10);
  l.push_back (20);
  l.push_back (30);
  ASSERT_TRUE (l.size () == 3);

  l.clear ();
  ASSERT_TRUE (l.size () == 0);
  ASSERT_TRUE (! static_cast < bool > (l.front ()));
  ASSERT_TRUE (! static_cast < bool > (l.back ()));

  // Repopulate after clear
  l.push_back (100);
  l.push_front (50);
  ASSERT_TRUE (l.size () == 2);
  ASSERT_TRUE (*(l.data (l.front ())) == 50);
  ASSERT_TRUE (*(l.data (l.back ())) == 100);

  return true;
}

// 8. Lifecycle tracking
constexpr bool test_lifecycle_tracking (minilib::counter & ctr) {
  int32_t destroyed = 0;
  {
    minilib::list < ConstexprTracker > l;
    l.attach_counter (minilib::addressof (ctr));
    l.emplace_back (1, 10, minilib::addressof (destroyed));
    l.emplace_front (0, 0, minilib::addressof (destroyed));
    l.emplace_back (2, 20, minilib::addressof (destroyed));
    ASSERT_TRUE (l.size () == 3);
    ASSERT_TRUE (destroyed == 0);

    // Pop front
    l.pop_front ();
    ASSERT_TRUE (destroyed == 1);
    ASSERT_TRUE (l.size () == 2);

    // Pop back
    l.pop_back ();
    ASSERT_TRUE (destroyed == 2);
    ASSERT_TRUE (l.size () == 1);

    // Clear
    l.clear ();
    ASSERT_TRUE (destroyed == 3);
    ASSERT_TRUE (l.size () == 0);
  }
  ASSERT_TRUE (destroyed == 3);

  // Destruction on scope exit
  destroyed = 0;
  {
    minilib::list < ConstexprTracker > l;
    l.attach_counter (minilib::addressof (ctr));
    l.emplace_back (1, 10, minilib::addressof (destroyed));
    l.emplace_back (2, 20, minilib::addressof (destroyed));
    ASSERT_TRUE (destroyed == 0);
  }
  ASSERT_TRUE (destroyed == 2);

  return true;
}

// 9. Copy constructor
constexpr bool test_copy_ctor (minilib::counter & ctr) {
  // Empty copy (copy constructor automatically attaches, no re-attach)
  {
    minilib::list < int32_t > l1;
    l1.attach_counter (minilib::addressof (ctr));
    minilib::list < int32_t > l2 (l1);
    ASSERT_TRUE (l2.size () == 0);
    ASSERT_TRUE (! static_cast < bool > (l2.front ()));
    ASSERT_TRUE (! static_cast < bool > (l2.back ()));
  }

  // Non-empty copy
  {
    minilib::list < int32_t > l1;
    l1.attach_counter (minilib::addressof (ctr));
    l1.push_back (1);
    l1.push_back (2);
    l1.push_back (3);

    minilib::list < int32_t > l2 (l1);
    ASSERT_TRUE (l2.size () == 3);
    ASSERT_TRUE (*(l2.data (l2.front ())) == 1);
    ASSERT_TRUE (*(l2.data (l2.back ())) == 3);

    // Deep copy verification
    *(l1.data (l1.front ())) = 999;
    ASSERT_TRUE (*(l1.data (l1.front ())) == 999);
    ASSERT_TRUE (*(l2.data (l2.front ())) == 1);
  }

  // Non-trivial copy
  {
    minilib::list < NonTrivial > l1;
    l1.attach_counter (minilib::addressof (ctr));
    l1.emplace_back (1, 10);
    l1.emplace_back (2, 20);

    minilib::list < NonTrivial > l2 (l1);
    ASSERT_TRUE (l2.size () == 2);
    // Copy constructor adds 100 to id
    ASSERT_TRUE (l2.data (l2.front ())->id == 101 && l2.data (l2.front ())->val == 10);
    ASSERT_TRUE (l2.data (l2.back ())->id == 102 && l2.data (l2.back ())->val == 20);
  }

  return true;
}

// 10. Copy assignment
constexpr bool test_copy_assignment (minilib::counter & ctr) {
  // Self assignment
  {
    minilib::list < int32_t > l;
    l.attach_counter (minilib::addressof (ctr));
    l.push_back (1);
    l.push_back (2);
    l = l;
    ASSERT_TRUE (l.size () == 2);
    ASSERT_TRUE (*(l.data (l.front ())) == 1);
    ASSERT_TRUE (*(l.data (l.back ())) == 2);
  }

  // Assign empty to empty
  {
    minilib::list < int32_t > l1;
    l1.attach_counter (minilib::addressof (ctr));
    minilib::list < int32_t > l2;
    l2.attach_counter (minilib::addressof (ctr));
    l1 = l2;
    ASSERT_TRUE (l1.size () == 0);
  }

  // Assign non-empty to empty
  {
    minilib::list < int32_t > l1;
    l1.attach_counter (minilib::addressof (ctr));
    minilib::list < int32_t > l2;
    l2.attach_counter (minilib::addressof (ctr));
    l2.push_back (10);
    l2.push_back (20);
    l1 = l2;
    ASSERT_TRUE (l1.size () == 2);
    ASSERT_TRUE (*(l1.data (l1.front ())) == 10);
    ASSERT_TRUE (*(l1.data (l1.back ())) == 20);
  }

  // Assign empty to non-empty
  {
    minilib::list < int32_t > l1;
    l1.attach_counter (minilib::addressof (ctr));
    l1.push_back (1);
    l1.push_back (2);
    minilib::list < int32_t > l2;
    l2.attach_counter (minilib::addressof (ctr));
    l1 = l2;
    ASSERT_TRUE (l1.size () == 0);
    ASSERT_TRUE (! static_cast < bool > (l1.front ()));
    ASSERT_TRUE (! static_cast < bool > (l1.back ()));
  }

  // Assign same size
  {
    minilib::list < int32_t > l1;
    l1.attach_counter (minilib::addressof (ctr));
    l1.push_back (1);
    l1.push_back (2);
    minilib::list < int32_t > l2;
    l2.attach_counter (minilib::addressof (ctr));
    l2.push_back (100);
    l2.push_back (200);
    l1 = l2;
    ASSERT_TRUE (l1.size () == 2);
    ASSERT_TRUE (*(l1.data (l1.front ())) == 100);
    ASSERT_TRUE (*(l1.data (l1.back ())) == 200);
  }

  // Assign smaller to larger (shrinking): tests destruction of excess elements
  {
    int32_t destroyed = 0;
    {
      minilib::list < ConstexprTracker > l1;
      l1.attach_counter (minilib::addressof (ctr));
      l1.emplace_back (1, 10, minilib::addressof (destroyed));
      l1.emplace_back (2, 20, minilib::addressof (destroyed));
      l1.emplace_back (3, 30, minilib::addressof (destroyed));

      minilib::list < ConstexprTracker > l2;
      l2.attach_counter (minilib::addressof (ctr));
      l2.emplace_back (4, 40, minilib::addressof (destroyed));

      int32_t destruct_before = destroyed;
      l1 = l2; // l1 shrinks from 3 elements to 1 element; 2 elements must be destroyed!
      ASSERT_TRUE (l1.size () == 1);
      ASSERT_TRUE (destroyed - destruct_before == 2);
      ASSERT_TRUE (l1.data (l1.front ())->val == 40);
    }
  }

  // Assign larger to smaller (growing)
  {
    minilib::list < int32_t > l1;
    l1.attach_counter (minilib::addressof (ctr));
    l1.push_back (1);
    minilib::list < int32_t > l2;
    l2.attach_counter (minilib::addressof (ctr));
    l2.push_back (10);
    l2.push_back (20);
    l2.push_back (30);
    l1 = l2;
    ASSERT_TRUE (l1.size () == 3);
    auto it = l1.front ();
    ASSERT_TRUE (*(l1.data (it)) == 10);
    it = l1.next (it);
    ASSERT_TRUE (*(l1.data (it)) == 20);
    it = l1.next (it);
    ASSERT_TRUE (*(l1.data (it)) == 30);
  }

  return true;
}

// 11. Move constructor
constexpr bool test_move_ctor (minilib::counter & ctr) {
  // Empty move (move constructor automatically attaches, no re-attach)
  {
    minilib::list < int32_t > l1;
    l1.attach_counter (minilib::addressof (ctr));
    minilib::list < int32_t > l2 (minilib::move (l1));
    ASSERT_TRUE (l2.size () == 0);
    ASSERT_TRUE (l1.size () == 0);
  }

  // Non-empty move
  {
    minilib::list < int32_t > l1;
    l1.attach_counter (minilib::addressof (ctr));
    auto h1 = l1.push_back (10);
    auto h2 = l1.push_back (20);

    minilib::list < int32_t > l2 (minilib::move (l1));
    ASSERT_TRUE (l2.size () == 2);
    ASSERT_TRUE (*(l2.data (l2.front ())) == 10);
    ASSERT_TRUE (*(l2.data (l2.back ())) == 20);

    // Moved-from container is empty
    ASSERT_TRUE (l1.size () == 0);
    ASSERT_TRUE (! static_cast < bool > (l1.front ()));
    ASSERT_TRUE (! static_cast < bool > (l1.back ()));

    // Handles created before move are now valid on l2
    ASSERT_TRUE (*(l2.data (h1)) == 10);
    ASSERT_TRUE (*(l2.data (h2)) == 20);
  }

  return true;
}

// 12. Move assignment
constexpr bool test_move_assignment (minilib::counter & ctr) {
  // Self assignment
  {
    minilib::list < int32_t > l;
    l.attach_counter (minilib::addressof (ctr));
    l.push_back (42);
    l = minilib::move (l);
    ASSERT_TRUE (l.size () == 1);
    ASSERT_TRUE (*(l.data (l.front ())) == 42);
  }

  // Move into empty
  {
    minilib::list < int32_t > l1;
    l1.attach_counter (minilib::addressof (ctr));
    minilib::list < int32_t > l2;
    l2.attach_counter (minilib::addressof (ctr));
    auto h = l2.push_back (77);
    l1 = minilib::move (l2);
    ASSERT_TRUE (l1.size () == 1);
    ASSERT_TRUE (*(l1.data (l1.front ())) == 77);
    ASSERT_TRUE (l2.size () == 0);
    ASSERT_TRUE (*(l1.data (h)) == 77);
  }

  // Move into non-empty: clears existing elements
  {
    int32_t destroyed = 0;
    {
      minilib::list < ConstexprTracker > l1;
      l1.attach_counter (minilib::addressof (ctr));
      l1.emplace_back (1, 10, minilib::addressof (destroyed));
      l1.emplace_back (2, 20, minilib::addressof (destroyed));

      minilib::list < ConstexprTracker > l2;
      l2.attach_counter (minilib::addressof (ctr));
      l2.emplace_back (3, 30, minilib::addressof (destroyed));

      l1 = minilib::move (l2);
      ASSERT_TRUE (l1.size () == 1);
      ASSERT_TRUE (l2.size () == 0);
      ASSERT_TRUE (destroyed == 2);
      ASSERT_TRUE (l1.data (l1.front ())->val == 30);
    }
    ASSERT_TRUE (destroyed == 3);
  }

  return true;
}

// 13. Move-only types
constexpr bool test_move_only (minilib::counter & ctr) {
  minilib::list < MoveOnly > l;
  l.attach_counter (minilib::addressof (ctr));
  l.push_back (MoveOnly (10));
  l.push_front (MoveOnly (5));
  l.emplace_back (20);
  l.emplace_front (1);

  ASSERT_TRUE (l.size () == 4);
  ASSERT_TRUE (l.data (l.front ())->val == 1);
  ASSERT_TRUE (l.data (l.back ())->val == 20);

  auto h_mid = l.next (l.front ()); // node with 5
  l.insert_before (h_mid, MoveOnly (3));
  l.insert_after (h_mid, MoveOnly (7));
  l.emplace_before (h_mid, 4);
  l.emplace_after (h_mid, 6);
  ASSERT_TRUE (l.size () == 8);

  l.pop_front ();
  l.pop_back ();
  ASSERT_TRUE (l.size () == 6);

  auto h_rem = l.front ();
  l.remove (h_rem);
  ASSERT_TRUE (l.size () == 5);

  // Move construction and assignment
  minilib::list < MoveOnly > l2 (minilib::move (l));
  ASSERT_TRUE (l2.size () == 5);
  ASSERT_TRUE (l.size () == 0);

  minilib::list < MoveOnly > l3;
  l3.attach_counter (minilib::addressof (ctr));
  l3 = minilib::move (l2);
  ASSERT_TRUE (l3.size () == 5);
  ASSERT_TRUE (l2.size () == 0);

  return true;
}

// 14. Handle semantics and tombstone lifecycle
constexpr bool test_handle_semantics_and_tombstone (minilib::counter & ctr) {
  minilib::list < int32_t > l;
  l.attach_counter (minilib::addressof (ctr));
  auto h1 = l.push_back (100);
  auto h2 = l.push_back (200);

  // Boolean conversion
  ASSERT_TRUE (static_cast < bool > (h1));
  ASSERT_TRUE (static_cast < bool > (h2));
  minilib::list < int32_t > :: handle_type h_null;
  ASSERT_TRUE (! static_cast < bool > (h_null));

  // get_id
  ASSERT_TRUE (h1.get_id () == h2.get_id ());

  // Copy construction of handle
  auto h1_copy (h1);
  ASSERT_TRUE (static_cast < bool > (h1_copy));
  ASSERT_TRUE (*(l.data (h1_copy)) == 100);

  // Copy assignment of handle
  minilib::list < int32_t > :: handle_type h1_assign;
  h1_assign = h1;
  ASSERT_TRUE (static_cast < bool > (h1_assign));
  ASSERT_TRUE (*(l.data (h1_assign)) == 100);

  // Move construction of handle
  auto h1_moved (minilib::move (h1_copy));
  ASSERT_TRUE (! static_cast < bool > (h1_copy));
  ASSERT_TRUE (static_cast < bool > (h1_moved));
  ASSERT_TRUE (*(l.data (h1_moved)) == 100);

  // Move assignment of handle
  minilib::list < int32_t > :: handle_type h1_move_assign;
  h1_move_assign = minilib::move (h1_assign);
  ASSERT_TRUE (! static_cast < bool > (h1_assign));
  ASSERT_TRUE (static_cast < bool > (h1_move_assign));
  ASSERT_TRUE (*(l.data (h1_move_assign)) == 100);

  // Reset to nullptr
  h1_moved = nullptr;
  ASSERT_TRUE (! static_cast < bool > (h1_moved));
  h1_move_assign = nullptr;
  ASSERT_TRUE (! static_cast < bool > (h1_move_assign));

  // Tombstone lifecycle: remove node while external handle is alive
  {
    minilib::list < int32_t > lt;
    lt.attach_counter (minilib::addressof (ctr));
    auto ht = lt.push_back (555);
    ASSERT_TRUE (static_cast < bool > (ht));
    ASSERT_TRUE (*(lt.data (ht)) == 555);

    lt.pop_front (); // Node removed from list; becomes tombstone because ht is alive
    ASSERT_TRUE (lt.size () == 0);
    // As a tombstone, operator bool returns false
    ASSERT_TRUE (! static_cast < bool > (ht));

    // Destroy handle; tombstone storage is released
    ht = nullptr;
    ASSERT_TRUE (! static_cast < bool > (ht));
  }

  return true;
}

// 15. Types with private destructor (inform_destruct)
constexpr bool test_private_destructor (minilib::counter & ctr) {
  minilib::list < Probe > l;
  l.attach_counter (minilib::addressof (ctr));

  auto h1 = l.emplace_back_null ();
  construct_probe (l.data (h1), 10);

  auto h0 = l.emplace_front_null ();
  construct_probe (l.data (h0), 5);

  auto h2 = l.emplace_after_null (h1);
  construct_probe (l.data (h2), 20);

  auto h_mid = l.emplace_before_null (h1);
  construct_probe (l.data (h_mid), 8);

  ASSERT_TRUE (l.size () == 4);
  ASSERT_TRUE (l.data (h0)->val == 5);
  ASSERT_TRUE (l.data (h_mid)->val == 8);
  ASSERT_TRUE (l.data (h1)->val == 10);
  ASSERT_TRUE (l.data (h2)->val == 20);

  // Manually destroy each element and inform container
  destroy_probe (l.data (h0));
  l.inform_destruct (h0);

  destroy_probe (l.data (h2));
  l.inform_destruct (h2);

  destroy_probe (l.data (h_mid));
  l.inform_destruct (h_mid);

  destroy_probe (l.data (h1));
  l.inform_destruct (h1);

  ASSERT_TRUE (l.size () == 0);
  return true;
}

// 16. Handle comparison and total ordering (compare_three_way)
constexpr bool test_handle_comparison_total_order (minilib::counter & ctr) {
  minilib::list < int32_t > l;
  l.attach_counter (minilib::addressof (ctr));
  auto h1 = l.push_back (10);
  auto h2 = l.push_back (20);
  auto h3 = l.push_back (30);
  minilib::list < int32_t > :: handle_type h_null;
  minilib::list < int32_t > :: handle_type h_null2;

  // 1. Reflexivity: a <=> a is equal
  ASSERT_TRUE (minilib::compare_three_way::operator () (h1, h1) == minilib::order_result::equal ());
  ASSERT_TRUE (minilib::compare_three_way::operator () (h2, h2) == minilib::order_result::equal ());
  ASSERT_TRUE (minilib::compare_three_way::operator () (h3, h3) == minilib::order_result::equal ());
  ASSERT_TRUE (minilib::compare_three_way::operator () (h_null, h_null) == minilib::order_result::equal ());
  ASSERT_TRUE (minilib::compare_three_way::operator () (h_null, h_null2) == minilib::order_result::equal ());

  // Handle copies compare equal
  auto h1_copy = h1;
  ASSERT_TRUE (minilib::compare_three_way::operator () (h1, h1_copy) == minilib::order_result::equal ());

  // Null is strictly less than any valid handle
  ASSERT_TRUE (minilib::compare_three_way::operator () (h_null, h1) < 0);
  ASSERT_TRUE (minilib::compare_three_way::operator () (h1, h_null) > 0);

  // Distinct nodes are never equal
  ASSERT_TRUE (minilib::compare_three_way::operator () (h1, h2) != minilib::order_result::equal ());
  ASSERT_TRUE (minilib::compare_three_way::operator () (h2, h3) != minilib::order_result::equal ());
  ASSERT_TRUE (minilib::compare_three_way::operator () (h1, h3) != minilib::order_result::equal ());

  // 2. Antisymmetry & Totality: for any pair (a, b), exactly one of (<, ==, >) holds and reversing reverses the sign
  auto check_pair = [] (const auto & a, const auto & b) constexpr -> bool {
    auto ab = minilib::compare_three_way::operator () (a, b);
    auto ba = minilib::compare_three_way::operator () (b, a);
    if (ab == minilib::order_result::equal ()) {
      return ba == minilib::order_result::equal ();
    } else if (ab < 0) {
      return ba > 0;
    } else {
      return ba < 0;
    }
  };

  ASSERT_TRUE (check_pair (h1, h2));
  ASSERT_TRUE (check_pair (h2, h3));
  ASSERT_TRUE (check_pair (h1, h3));
  ASSERT_TRUE (check_pair (h_null, h1));
  ASSERT_TRUE (check_pair (h_null, h2));
  ASSERT_TRUE (check_pair (h_null, h3));

  // 3. Transitivity: for all triplets (a, b, c), if a < b and b < c then a < c
  auto check_trans = [] (const auto & a, const auto & b, const auto & c) constexpr -> bool {
    auto ab = minilib::compare_three_way::operator () (a, b);
    auto bc = minilib::compare_three_way::operator () (b, c);
    auto ac = minilib::compare_three_way::operator () (a, c);
    if (ab < 0 && bc < 0 && !(ac < 0)) return false;
    if (ab > 0 && bc > 0 && !(ac > 0)) return false;
    if (ab == minilib::order_result::equal () && bc == minilib::order_result::equal () && !(ac == minilib::order_result::equal ())) return false;
    return true;
  };

  const minilib::list < int32_t > :: handle_type handles[4] = { h_null, h1, h2, h3 };
  for (size_t i = 0; i < 4; i++) {
    for (size_t j = 0; j < 4; j++) {
      for (size_t k = 0; k < 4; k++) {
        ASSERT_TRUE (check_trans (handles[i], handles[j], handles[k]));
      }
    }
  }

  return true;
}

constexpr bool test_all () {
  minilib::counter ctr;
  ASSERT_TRUE (test_default_ctor (ctr));
  ASSERT_TRUE (test_push_and_emplace_front_back (ctr));
  ASSERT_TRUE (test_insert_and_emplace_before_after (ctr));
  ASSERT_TRUE (test_traversal_and_data_access (ctr));
  ASSERT_TRUE (test_pop_front_and_pop_back (ctr));
  ASSERT_TRUE (test_remove_by_handle (ctr));
  ASSERT_TRUE (test_clear_and_repopulate (ctr));
  ASSERT_TRUE (test_lifecycle_tracking (ctr));
  ASSERT_TRUE (test_copy_ctor (ctr));
  ASSERT_TRUE (test_copy_assignment (ctr));
  ASSERT_TRUE (test_move_ctor (ctr));
  ASSERT_TRUE (test_move_assignment (ctr));
  ASSERT_TRUE (test_move_only (ctr));
  ASSERT_TRUE (test_handle_semantics_and_tombstone (ctr));
  ASSERT_TRUE (test_private_destructor (ctr));
  ASSERT_TRUE (test_handle_comparison_total_order (ctr));
  return true;
}

constexpr bool passed = test_all ();
static_assert (passed);

void main ([[maybe_unused]] void * sp) {
  if (! passed) exit (1);
  exit (0);
}
