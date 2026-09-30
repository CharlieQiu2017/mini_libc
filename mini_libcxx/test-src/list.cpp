#include <stdint.h>
#include <tls.h>
#include <memory.h>
#include <exit.h>
#include <list.hpp>
#include <type_traits.hpp>
#include <utility.hpp>
#include <compare.hpp>

#define TEST_CHECK(cond) do { if (!(cond)) exit (1); } while (0)

static struct malloc_arena_t main_arena;

static struct tls_struct main_tls = {
  .thread_id = 0,
  .malloc_arena = nullptr,
  .getrandom_opaque_state = nullptr,
  .counter = 0
};

struct Point {
  int32_t x;
  int32_t y;
  Point () : x (0), y (0) {}
  Point (int32_t x_, int32_t y_) : x (x_), y (y_) {}
};

struct DefaultInitProbe {
  int32_t val;
  DefaultInitProbe () : val (12345) {}
  DefaultInitProbe (int32_t v) : val (v) {}
};

static int32_t g_active_instances = 0;
static int32_t g_construct_count = 0;
static int32_t g_copy_construct_count = 0;
static int32_t g_move_construct_count = 0;
static int32_t g_copy_assign_count = 0;
static int32_t g_move_assign_count = 0;
static int32_t g_destruct_count = 0;

static void reset_tracker () {
  g_active_instances = 0;
  g_construct_count = 0;
  g_copy_construct_count = 0;
  g_move_construct_count = 0;
  g_copy_assign_count = 0;
  g_move_assign_count = 0;
  g_destruct_count = 0;
}

struct Tracker {
  int32_t id;
  int32_t val;

  Tracker () : id (0), val (42) {
    g_active_instances++;
    g_construct_count++;
  }

  Tracker (int32_t i, int32_t v) : id (i), val (v) {
    g_active_instances++;
    g_construct_count++;
  }

  Tracker (const Tracker & o) : id (o.id + 100), val (o.val) {
    g_active_instances++;
    g_copy_construct_count++;
  }

  Tracker (Tracker && o) : id (o.id), val (o.val) {
    o.id = -1;
    o.val = -1;
    g_active_instances++;
    g_move_construct_count++;
  }

  Tracker & operator= (const Tracker & o) {
    id = o.id + 100;
    val = o.val;
    g_copy_assign_count++;
    return *this;
  }

  Tracker & operator= (Tracker && o) {
    id = o.id;
    val = o.val;
    o.id = -1;
    o.val = -1;
    g_move_assign_count++;
    return *this;
  }

  ~Tracker () {
    id = -999;
    val = -999;
    g_active_instances--;
    g_destruct_count++;
  }
};

struct MoveOnly {
  int32_t val;
  MoveOnly () : val (0) {}
  explicit MoveOnly (int32_t v) : val (v) {}
  MoveOnly (const MoveOnly &) = delete;
  MoveOnly & operator= (const MoveOnly &) = delete;
  MoveOnly (MoveOnly && o) : val (o.val) { o.val = -1; }
  MoveOnly & operator= (MoveOnly && o) {
    val = o.val;
    o.val = -1;
    return *this;
  }
  ~MoveOnly () {}
};

struct Probe {
  int32_t val;
  Probe () : val (42) {}
  explicit Probe (int32_t v) : val (v) {}

  friend void destroy_probe (Probe * p) { p->~Probe (); }
  friend void construct_probe (Probe * p, int32_t v) {
    ::new (static_cast < void * > (p)) Probe (v);
  }

private:
  ~Probe () {}
};

// 1. Default constructor and empty state
static void test_default_ctor () {
  minilib::list < int32_t > l;
  TEST_CHECK (l.size () == 0);
  TEST_CHECK (! static_cast < bool > (l.front ()));
  TEST_CHECK (! static_cast < bool > (l.back ()));
}

// 2. Push & emplace front and back
static void test_push_and_emplace_front_back () {
  minilib::list < int32_t > l;

  // push_back
  auto h1 = l.push_back (10);
  TEST_CHECK (l.size () == 1);
  TEST_CHECK (*(l.data (h1)) == 10);
  TEST_CHECK (*(l.data (l.front ())) == 10);
  TEST_CHECK (*(l.data (l.back ())) == 10);

  int32_t val20 = 20;
  auto h2 = l.push_back (val20); // const ref overload
  TEST_CHECK (l.size () == 2);
  TEST_CHECK (*(l.data (h2)) == 20);
  TEST_CHECK (*(l.data (l.back ())) == 20);

  // push_front
  auto h0 = l.push_front (0);
  TEST_CHECK (l.size () == 3);
  TEST_CHECK (*(l.data (h0)) == 0);
  TEST_CHECK (*(l.data (l.front ())) == 0);

  int32_t val_neg = -10;
  auto h_neg = l.push_front (val_neg);
  TEST_CHECK (l.size () == 4);
  TEST_CHECK (*(l.data (h_neg)) == -10);
  TEST_CHECK (*(l.data (l.front ())) == -10);

  // emplace_front and emplace_back with non-trivial constructor
  minilib::list < Point > lp;
  auto hp1 = lp.emplace_back (1, 2);
  TEST_CHECK (lp.size () == 1);
  TEST_CHECK (lp.data (hp1)->x == 1 && lp.data (hp1)->y == 2);

  auto hp0 = lp.emplace_front (3, 4);
  TEST_CHECK (lp.size () == 2);
  TEST_CHECK (lp.data (hp0)->x == 3 && lp.data (hp0)->y == 4);
  TEST_CHECK (lp.data (lp.front ())->x == 3);
  TEST_CHECK (lp.data (lp.back ())->x == 1);

  // emplace_default
  minilib::list < DefaultInitProbe > ld;
  auto hd1 = ld.emplace_back_default ();
  TEST_CHECK (ld.data (hd1)->val == 12345);
  auto hd0 = ld.emplace_front_default ();
  TEST_CHECK (ld.data (hd0)->val == 12345);
  TEST_CHECK (ld.size () == 2);

  // emplace_null
  minilib::list < int32_t > ln;
  auto hn1 = ln.emplace_back_null ();
  minilib::construct_at < int32_t > (ln.data (hn1), 99);
  TEST_CHECK (*(ln.data (hn1)) == 99);

  auto hn0 = ln.emplace_front_null ();
  minilib::construct_at < int32_t > (ln.data (hn0), 88);
  TEST_CHECK (*(ln.data (hn0)) == 88);
  TEST_CHECK (ln.size () == 2);
  TEST_CHECK (*(ln.data (ln.front ())) == 88);
  TEST_CHECK (*(ln.data (ln.back ())) == 99);
}

// 3. Insert and emplace before & after
static void test_insert_and_emplace_before_after () {
  minilib::list < int32_t > l;
  auto h2 = l.push_back (20);

  // insert_before on head -> new head
  auto h1 = l.insert_before (h2, 10);
  TEST_CHECK (l.size () == 2);
  TEST_CHECK (*(l.data (l.front ())) == 10);
  TEST_CHECK (*(l.data (h1)) == 10);

  // insert_after on tail -> new tail
  auto h4 = l.insert_after (h2, 40);
  TEST_CHECK (l.size () == 3);
  TEST_CHECK (*(l.data (l.back ())) == 40);

  // insert_before in middle
  int32_t v30 = 30;
  auto h3 = l.insert_before (h4, v30);
  TEST_CHECK (l.size () == 4);
  TEST_CHECK (*(l.data (h3)) == 30);

  // insert_after in middle
  auto h25 = l.insert_after (h2, 25);
  TEST_CHECK (l.size () == 5);
  TEST_CHECK (*(l.data (h25)) == 25);

  // Verify order: 10, 20, 25, 30, 40
  auto cur = l.front ();
  TEST_CHECK (*(l.data (cur)) == 10);
  cur = l.next (cur);
  TEST_CHECK (*(l.data (cur)) == 20);
  cur = l.next (cur);
  TEST_CHECK (*(l.data (cur)) == 25);
  cur = l.next (cur);
  TEST_CHECK (*(l.data (cur)) == 30);
  cur = l.next (cur);
  TEST_CHECK (*(l.data (cur)) == 40);
  cur = l.next (cur);
  TEST_CHECK (! static_cast < bool > (cur));

  // emplace_before and emplace_after with multi-arg Point
  minilib::list < Point > lp;
  auto hp2 = lp.push_back (Point (2, 2));
  auto hp1 = lp.emplace_before (hp2, 1, 1);
  auto hp3 = lp.emplace_after (hp2, 3, 3);
  TEST_CHECK (lp.size () == 3);
  TEST_CHECK (lp.data (hp1)->x == 1);
  TEST_CHECK (lp.data (hp2)->x == 2);
  TEST_CHECK (lp.data (hp3)->x == 3);

  // emplace_before_default and emplace_after_default
  minilib::list < DefaultInitProbe > ld;
  auto hd1 = ld.push_back (DefaultInitProbe (1));
  auto hd0 = ld.emplace_before_default (hd1);
  auto hd2 = ld.emplace_after_default (hd1);
  TEST_CHECK (ld.size () == 3);
  TEST_CHECK (ld.data (hd0)->val == 12345);
  TEST_CHECK (ld.data (hd1)->val == 1);
  TEST_CHECK (ld.data (hd2)->val == 12345);

  // emplace_before_null and emplace_after_null
  minilib::list < int32_t > ln;
  auto hmid = ln.push_back (50);
  auto hpre = ln.emplace_before_null (hmid);
  minilib::construct_at < int32_t > (ln.data (hpre), 40);
  auto hpost = ln.emplace_after_null (hmid);
  minilib::construct_at < int32_t > (ln.data (hpost), 60);
  TEST_CHECK (ln.size () == 3);
  TEST_CHECK (*(ln.data (hpre)) == 40);
  TEST_CHECK (*(ln.data (hmid)) == 50);
  TEST_CHECK (*(ln.data (hpost)) == 60);
}

// 4. Traversal and data access (non-const and const)
static void test_traversal_and_data_access () {
  minilib::list < int32_t > l;
  for (int32_t i = 1; i <= 5; i++) {
    l.push_back (i * 10);
  }

  // Forward traversal
  int32_t expected = 10;
  auto it = l.front ();
  while (it) {
    TEST_CHECK (*(l.data (it)) == expected);
    expected += 10;
    it = l.next (it);
  }
  TEST_CHECK (expected == 60);

  // Backward traversal
  expected = 50;
  it = l.back ();
  while (it) {
    TEST_CHECK (*(l.data (it)) == expected);
    expected -= 10;
    it = l.prev (it);
  }
  TEST_CHECK (expected == 0);

  // Mutating elements through non-const data()
  it = l.front ();
  *(l.data (it)) = 100;
  TEST_CHECK (*(l.data (l.front ())) == 100);

  // Const list access
  const minilib::list < int32_t > & cl = l;
  TEST_CHECK (cl.size () == 5);
  auto ch = cl.front ();
  const int32_t * val_ptr = cl.data (ch);
  TEST_CHECK (*val_ptr == 100);

  auto ch_next = cl.next (ch);
  TEST_CHECK (*(cl.data (ch_next)) == 20);
  auto ch_prev = cl.prev (ch_next);
  TEST_CHECK (*(cl.data (ch_prev)) == 100);
  TEST_CHECK (! static_cast < bool > (cl.prev (ch)));
  TEST_CHECK (! static_cast < bool > (cl.next (cl.back ())));
}

// 5. Pop front and pop back
static void test_pop_front_and_pop_back () {
  minilib::list < int32_t > l;
  l.push_back (1);
  l.push_back (2);
  l.push_back (3);

  // pop_front
  l.pop_front ();
  TEST_CHECK (l.size () == 2);
  TEST_CHECK (*(l.data (l.front ())) == 2);
  TEST_CHECK (*(l.data (l.back ())) == 3);

  // pop_back
  l.pop_back ();
  TEST_CHECK (l.size () == 1);
  TEST_CHECK (*(l.data (l.front ())) == 2);
  TEST_CHECK (*(l.data (l.back ())) == 2);

  // Pop last remaining element
  l.pop_front ();
  TEST_CHECK (l.size () == 0);
  TEST_CHECK (! static_cast < bool > (l.front ()));
  TEST_CHECK (! static_cast < bool > (l.back ()));

  // Test single-element pop_back
  l.push_back (42);
  TEST_CHECK (l.size () == 1);
  l.pop_back ();
  TEST_CHECK (l.size () == 0);
  TEST_CHECK (! static_cast < bool > (l.front ()));
  TEST_CHECK (! static_cast < bool > (l.back ()));
}

// 6. Remove node by handle
static void test_remove_by_handle () {
  // Remove middle element
  {
    minilib::list < int32_t > l;
    auto h1 = l.push_back (1);
    auto h2 = l.push_back (2);
    auto h3 = l.push_back (3);

    l.remove (h2);
    TEST_CHECK (l.size () == 2);
    TEST_CHECK (*(l.data (l.front ())) == 1);
    TEST_CHECK (*(l.data (l.back ())) == 3);
    TEST_CHECK (*(l.data (l.next (h1))) == 3);
    TEST_CHECK (*(l.data (l.prev (h3))) == 1);
  }

  // Remove head element
  {
    minilib::list < int32_t > l;
    auto h1 = l.push_back (1);
    auto h2 = l.push_back (2);

    l.remove (h1);
    TEST_CHECK (l.size () == 1);
    TEST_CHECK (*(l.data (l.front ())) == 2);
    TEST_CHECK (*(l.data (l.back ())) == 2);
    TEST_CHECK (! static_cast < bool > (l.prev (h2)));
  }

  // Remove tail element
  {
    minilib::list < int32_t > l;
    auto h1 = l.push_back (1);
    auto h2 = l.push_back (2);

    l.remove (h2);
    TEST_CHECK (l.size () == 1);
    TEST_CHECK (*(l.data (l.front ())) == 1);
    TEST_CHECK (*(l.data (l.back ())) == 1);
    TEST_CHECK (! static_cast < bool > (l.next (h1)));
  }

  // Remove solitary element
  {
    minilib::list < int32_t > l;
    auto h1 = l.push_back (99);

    l.remove (h1);
    TEST_CHECK (l.size () == 0);
    TEST_CHECK (! static_cast < bool > (l.front ()));
    TEST_CHECK (! static_cast < bool > (l.back ()));
  }
}

// 7. Clear and repopulate
static void test_clear_and_repopulate () {
  minilib::list < int32_t > l;
  l.clear (); // Clear on empty list
  TEST_CHECK (l.size () == 0);

  l.push_back (10);
  l.push_back (20);
  l.push_back (30);
  TEST_CHECK (l.size () == 3);

  l.clear ();
  TEST_CHECK (l.size () == 0);
  TEST_CHECK (! static_cast < bool > (l.front ()));
  TEST_CHECK (! static_cast < bool > (l.back ()));

  // Repopulate after clear
  l.push_back (100);
  l.push_front (50);
  TEST_CHECK (l.size () == 2);
  TEST_CHECK (*(l.data (l.front ())) == 50);
  TEST_CHECK (*(l.data (l.back ())) == 100);
}

// 8. Lifecycle tracking (destruction, moves, copies)
static void test_lifecycle_tracking () {
  reset_tracker ();
  {
    minilib::list < Tracker > l;
    l.emplace_back (1, 10);
    l.emplace_front (0, 0);
    l.emplace_back (2, 20);
    TEST_CHECK (g_active_instances == 3);
    TEST_CHECK (g_construct_count == 3);

    // Pop front
    l.pop_front ();
    TEST_CHECK (g_active_instances == 2);
    TEST_CHECK (g_destruct_count == 1);

    // Pop back
    l.pop_back ();
    TEST_CHECK (g_active_instances == 1);
    TEST_CHECK (g_destruct_count == 2);

    // Clear
    l.clear ();
    TEST_CHECK (g_active_instances == 0);
    TEST_CHECK (g_destruct_count == 3);
  }
  TEST_CHECK (g_active_instances == 0);

  // Destruction on list destruction
  reset_tracker ();
  {
    minilib::list < Tracker > l;
    l.emplace_back (1, 10);
    l.emplace_back (2, 20);
    TEST_CHECK (g_active_instances == 2);
  }
  TEST_CHECK (g_active_instances == 0);
  TEST_CHECK (g_destruct_count == 2);
}

// 9. Copy constructor
static void test_copy_ctor () {
  // Empty copy
  {
    minilib::list < int32_t > l1;
    minilib::list < int32_t > l2 (l1);
    TEST_CHECK (l2.size () == 0);
    TEST_CHECK (! static_cast < bool > (l2.front ()));
    TEST_CHECK (! static_cast < bool > (l2.back ()));
  }

  // Non-empty copy
  {
    minilib::list < int32_t > l1;
    l1.push_back (1);
    l1.push_back (2);
    l1.push_back (3);

    minilib::list < int32_t > l2 (l1);
    TEST_CHECK (l2.size () == 3);
    TEST_CHECK (*(l2.data (l2.front ())) == 1);
    TEST_CHECK (*(l2.data (l2.back ())) == 3);

    // Deep copy verification
    *(l1.data (l1.front ())) = 999;
    TEST_CHECK (*(l1.data (l1.front ())) == 999);
    TEST_CHECK (*(l2.data (l2.front ())) == 1);
  }
}

// 10. Copy assignment
static void test_copy_assignment () {
  // Self assignment
  {
    minilib::list < int32_t > l;
    l.push_back (1);
    l.push_back (2);
    l = l;
    TEST_CHECK (l.size () == 2);
    TEST_CHECK (*(l.data (l.front ())) == 1);
    TEST_CHECK (*(l.data (l.back ())) == 2);
  }

  // Assign empty to empty
  {
    minilib::list < int32_t > l1;
    minilib::list < int32_t > l2;
    l1 = l2;
    TEST_CHECK (l1.size () == 0);
  }

  // Assign non-empty to empty
  {
    minilib::list < int32_t > l1;
    minilib::list < int32_t > l2;
    l2.push_back (10);
    l2.push_back (20);
    l1 = l2;
    TEST_CHECK (l1.size () == 2);
    TEST_CHECK (*(l1.data (l1.front ())) == 10);
    TEST_CHECK (*(l1.data (l1.back ())) == 20);
  }

  // Assign empty to non-empty
  {
    minilib::list < int32_t > l1;
    l1.push_back (1);
    l1.push_back (2);
    minilib::list < int32_t > l2;
    l1 = l2;
    TEST_CHECK (l1.size () == 0);
    TEST_CHECK (! static_cast < bool > (l1.front ()));
    TEST_CHECK (! static_cast < bool > (l1.back ()));
  }

  // Assign same size
  {
    minilib::list < int32_t > l1;
    l1.push_back (1);
    l1.push_back (2);
    minilib::list < int32_t > l2;
    l2.push_back (100);
    l2.push_back (200);
    l1 = l2;
    TEST_CHECK (l1.size () == 2);
    TEST_CHECK (*(l1.data (l1.front ())) == 100);
    TEST_CHECK (*(l1.data (l1.back ())) == 200);
  }

  // Assign smaller to larger (len > other.len): tests destruction of excess elements
  {
    reset_tracker ();
    {
      minilib::list < Tracker > l1;
      l1.emplace_back (1, 10);
      l1.emplace_back (2, 20);
      l1.emplace_back (3, 30);
      TEST_CHECK (g_active_instances == 3);

      minilib::list < Tracker > l2;
      l2.emplace_back (4, 40);
      TEST_CHECK (g_active_instances == 4);

      int32_t destruct_before = g_destruct_count;
      l1 = l2; // l1 shrinks from 3 elements to 1 element; 2 elements must be destroyed!
      TEST_CHECK (l1.size () == 1);
      TEST_CHECK (g_destruct_count - destruct_before == 2);
      TEST_CHECK (g_active_instances == 2);
      TEST_CHECK (l1.data (l1.front ())->val == 40);
    }
    TEST_CHECK (g_active_instances == 0);
  }

  // Assign larger to smaller (len < other.len)
  {
    minilib::list < int32_t > l1;
    l1.push_back (1);
    minilib::list < int32_t > l2;
    l2.push_back (10);
    l2.push_back (20);
    l2.push_back (30);
    l1 = l2;
    TEST_CHECK (l1.size () == 3);
    auto it = l1.front ();
    TEST_CHECK (*(l1.data (it)) == 10);
    it = l1.next (it);
    TEST_CHECK (*(l1.data (it)) == 20);
    it = l1.next (it);
    TEST_CHECK (*(l1.data (it)) == 30);
  }
}

// 11. Move constructor
static void test_move_ctor () {
  // Empty move
  {
    minilib::list < int32_t > l1;
    minilib::list < int32_t > l2 (minilib::move (l1));
    TEST_CHECK (l2.size () == 0);
    TEST_CHECK (l1.size () == 0);
  }

  // Non-empty move
  {
    minilib::list < int32_t > l1;
    auto h1 = l1.push_back (10);
    auto h2 = l1.push_back (20);

    minilib::list < int32_t > l2 (minilib::move (l1));
    TEST_CHECK (l2.size () == 2);
    TEST_CHECK (*(l2.data (l2.front ())) == 10);
    TEST_CHECK (*(l2.data (l2.back ())) == 20);

    // Moved-from container is empty
    TEST_CHECK (l1.size () == 0);
    TEST_CHECK (! static_cast < bool > (l1.front ()));
    TEST_CHECK (! static_cast < bool > (l1.back ()));

    // Handles created before move are now valid on l2
    TEST_CHECK (*(l2.data (h1)) == 10);
    TEST_CHECK (*(l2.data (h2)) == 20);
  }
}

// 12. Move assignment
static void test_move_assignment () {
  // Self assignment
  {
    minilib::list < int32_t > l;
    l.push_back (42);
    l = minilib::move (l);
    TEST_CHECK (l.size () == 1);
    TEST_CHECK (*(l.data (l.front ())) == 42);
  }

  // Move into empty
  {
    minilib::list < int32_t > l1;
    minilib::list < int32_t > l2;
    auto h = l2.push_back (77);
    l1 = minilib::move (l2);
    TEST_CHECK (l1.size () == 1);
    TEST_CHECK (*(l1.data (l1.front ())) == 77);
    TEST_CHECK (l2.size () == 0);
    TEST_CHECK (*(l1.data (h)) == 77);
  }

  // Move into non-empty: clears existing elements
  {
    reset_tracker ();
    {
      minilib::list < Tracker > l1;
      l1.emplace_back (1, 10);
      l1.emplace_back (2, 20);
      TEST_CHECK (g_active_instances == 2);

      minilib::list < Tracker > l2;
      l2.emplace_back (3, 30);
      TEST_CHECK (g_active_instances == 3);

      l1 = minilib::move (l2);
      TEST_CHECK (l1.size () == 1);
      TEST_CHECK (l2.size () == 0);
      TEST_CHECK (g_active_instances == 1);
      TEST_CHECK (l1.data (l1.front ())->val == 30);
    }
    TEST_CHECK (g_active_instances == 0);
  }
}

// 13. Move-only types
static void test_move_only () {
  minilib::list < MoveOnly > l;
  l.push_back (MoveOnly (10));
  l.push_front (MoveOnly (5));
  l.emplace_back (20);
  l.emplace_front (1);

  TEST_CHECK (l.size () == 4);
  TEST_CHECK (l.data (l.front ())->val == 1);
  TEST_CHECK (l.data (l.back ())->val == 20);

  auto h_mid = l.next (l.front ()); // node with 5
  l.insert_before (h_mid, MoveOnly (3));
  l.insert_after (h_mid, MoveOnly (7));
  l.emplace_before (h_mid, 4);
  l.emplace_after (h_mid, 6);
  TEST_CHECK (l.size () == 8);

  l.pop_front ();
  l.pop_back ();
  TEST_CHECK (l.size () == 6);

  auto h_rem = l.front ();
  l.remove (h_rem);
  TEST_CHECK (l.size () == 5);

  // Move construction and assignment
  minilib::list < MoveOnly > l2 (minilib::move (l));
  TEST_CHECK (l2.size () == 5);
  TEST_CHECK (l.size () == 0);

  minilib::list < MoveOnly > l3;
  l3 = minilib::move (l2);
  TEST_CHECK (l3.size () == 5);
  TEST_CHECK (l2.size () == 0);
}

// 14. Handle semantics, comparisons, and tombstone lifecycle
static void test_handle_semantics_and_tombstone () {
  minilib::list < int32_t > l;
  auto h1 = l.push_back (100);
  auto h2 = l.push_back (200);

  // Boolean conversion
  TEST_CHECK (static_cast < bool > (h1));
  TEST_CHECK (static_cast < bool > (h2));
  minilib::list < int32_t > :: handle_type h_null;
  TEST_CHECK (! static_cast < bool > (h_null));

  // get_id
  TEST_CHECK (h1.get_id () == h2.get_id ());

  // Copy construction of handle
  auto h1_copy (h1);
  TEST_CHECK (static_cast < bool > (h1_copy));
  TEST_CHECK (*(l.data (h1_copy)) == 100);

  // Copy assignment of handle
  minilib::list < int32_t > :: handle_type h1_assign;
  h1_assign = h1;
  TEST_CHECK (static_cast < bool > (h1_assign));
  TEST_CHECK (*(l.data (h1_assign)) == 100);

  // Move construction of handle
  auto h1_moved (minilib::move (h1_copy));
  TEST_CHECK (! static_cast < bool > (h1_copy));
  TEST_CHECK (static_cast < bool > (h1_moved));
  TEST_CHECK (*(l.data (h1_moved)) == 100);

  // Move assignment of handle
  minilib::list < int32_t > :: handle_type h1_move_assign;
  h1_move_assign = minilib::move (h1_assign);
  TEST_CHECK (! static_cast < bool > (h1_assign));
  TEST_CHECK (static_cast < bool > (h1_move_assign));
  TEST_CHECK (*(l.data (h1_move_assign)) == 100);

  // Reset to nullptr
  h1_moved = nullptr;
  TEST_CHECK (! static_cast < bool > (h1_moved));
  h1_move_assign = nullptr;
  TEST_CHECK (! static_cast < bool > (h1_move_assign));

  // Tombstone lifecycle: remove node while external handle is alive
  {
    minilib::list < int32_t > lt;
    auto ht = lt.push_back (555);
    TEST_CHECK (static_cast < bool > (ht));
    TEST_CHECK (*(lt.data (ht)) == 555);

    lt.pop_front (); // Node removed from list; becomes tombstone because ht is alive
    TEST_CHECK (lt.size () == 0);
    // As a tombstone, operator bool returns false
    TEST_CHECK (! static_cast < bool > (ht));

    // Destroy handle; tombstone storage is released
    ht = nullptr;
    TEST_CHECK (! static_cast < bool > (ht));
  }
}

// 15. Types with private destructor (inform_destruct)
static void test_private_destructor () {
  minilib::list < Probe > l;
  auto h1 = l.emplace_back_null ();
  construct_probe (l.data (h1), 10);

  auto h0 = l.emplace_front_null ();
  construct_probe (l.data (h0), 5);

  auto h2 = l.emplace_after_null (h1);
  construct_probe (l.data (h2), 20);

  auto h_mid = l.emplace_before_null (h1);
  construct_probe (l.data (h_mid), 8);

  TEST_CHECK (l.size () == 4);
  TEST_CHECK (l.data (h0)->val == 5);
  TEST_CHECK (l.data (h_mid)->val == 8);
  TEST_CHECK (l.data (h1)->val == 10);
  TEST_CHECK (l.data (h2)->val == 20);

  // Manually destroy each element and inform container
  destroy_probe (l.data (h0));
  l.inform_destruct (h0);

  destroy_probe (l.data (h2));
  l.inform_destruct (h2);

  destroy_probe (l.data (h_mid));
  l.inform_destruct (h_mid);

  destroy_probe (l.data (h1));
  l.inform_destruct (h1);

  TEST_CHECK (l.size () == 0);
  // Empty list destructor completes safely without invoking deleted destructor
}

// 16. Handle comparison and total ordering (compare_three_way)
static void test_handle_comparison_total_order () {
  minilib::list < int32_t > l;
  auto h1 = l.push_back (10);
  auto h2 = l.push_back (20);
  auto h3 = l.push_back (30);
  minilib::list < int32_t > :: handle_type h_null;
  minilib::list < int32_t > :: handle_type h_null2;

  // 1. Reflexivity: a <=> a is equal
  TEST_CHECK (minilib::compare_three_way::operator () (h1, h1) == minilib::order_result::equal ());
  TEST_CHECK (minilib::compare_three_way::operator () (h2, h2) == minilib::order_result::equal ());
  TEST_CHECK (minilib::compare_three_way::operator () (h3, h3) == minilib::order_result::equal ());
  TEST_CHECK (minilib::compare_three_way::operator () (h_null, h_null) == minilib::order_result::equal ());
  TEST_CHECK (minilib::compare_three_way::operator () (h_null, h_null2) == minilib::order_result::equal ());

  // Handle copies compare equal
  auto h1_copy = h1;
  TEST_CHECK (minilib::compare_three_way::operator () (h1, h1_copy) == minilib::order_result::equal ());

  // Null is strictly less than any valid handle
  TEST_CHECK (minilib::compare_three_way::operator () (h_null, h1) < 0);
  TEST_CHECK (minilib::compare_three_way::operator () (h1, h_null) > 0);

  // Distinct nodes are never equal
  TEST_CHECK (minilib::compare_three_way::operator () (h1, h2) != minilib::order_result::equal ());
  TEST_CHECK (minilib::compare_three_way::operator () (h2, h3) != minilib::order_result::equal ());
  TEST_CHECK (minilib::compare_three_way::operator () (h1, h3) != minilib::order_result::equal ());

  // 2. Antisymmetry & Totality: for any pair (a, b), exactly one of (<, ==, >) holds and reversing reverses the sign
  auto check_pair = [] (const auto & a, const auto & b) {
    auto ab = minilib::compare_three_way::operator () (a, b);
    auto ba = minilib::compare_three_way::operator () (b, a);
    if (ab == minilib::order_result::equal ()) {
      TEST_CHECK (ba == minilib::order_result::equal ());
    } else if (ab < 0) {
      TEST_CHECK (ba > 0);
    } else {
      TEST_CHECK (ba < 0);
    }
  };

  check_pair (h1, h2);
  check_pair (h2, h3);
  check_pair (h1, h3);
  check_pair (h_null, h1);
  check_pair (h_null, h2);
  check_pair (h_null, h3);

  // 3. Transitivity: for all triplets (a, b, c), if a < b and b < c then a < c
  auto check_trans = [] (const auto & a, const auto & b, const auto & c) {
    auto ab = minilib::compare_three_way::operator () (a, b);
    auto bc = minilib::compare_three_way::operator () (b, c);
    auto ac = minilib::compare_three_way::operator () (a, c);
    if (ab < 0 && bc < 0) {
      TEST_CHECK (ac < 0);
    }
    if (ab > 0 && bc > 0) {
      TEST_CHECK (ac > 0);
    }
    if (ab == minilib::order_result::equal () && bc == minilib::order_result::equal ()) {
      TEST_CHECK (ac == minilib::order_result::equal ());
    }
  };

  const minilib::list < int32_t > :: handle_type handles[4] = { h_null, h1, h2, h3 };
  for (size_t i = 0; i < 4; i++) {
    for (size_t j = 0; j < 4; j++) {
      for (size_t k = 0; k < 4; k++) {
        check_trans (handles[i], handles[j], handles[k]);
      }
    }
  }
}

void main ([[maybe_unused]] void * sp) {
  main_tls.malloc_arena = &main_arena;
  set_thread_pointer (&main_tls);
  malloc_init ();

  test_default_ctor ();
  test_push_and_emplace_front_back ();
  test_insert_and_emplace_before_after ();
  test_traversal_and_data_access ();
  test_pop_front_and_pop_back ();
  test_remove_by_handle ();
  test_clear_and_repopulate ();
  test_lifecycle_tracking ();
  test_copy_ctor ();
  test_copy_assignment ();
  test_move_ctor ();
  test_move_assignment ();
  test_move_only ();
  test_handle_semantics_and_tombstone ();
  test_private_destructor ();
  test_handle_comparison_total_order ();

  exit (0);
}
