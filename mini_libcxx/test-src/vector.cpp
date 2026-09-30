#include <stdint.h>
#include <tls.h>
#include <memory.h>
#include <exit.h>
#include <vector.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

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

// 1. Default constructor
static void test_default_ctor () {
  minilib::vector < int32_t > v;
  TEST_CHECK (v.size () == 0);
  TEST_CHECK (v.capacity () == 0);
  TEST_CHECK (v.data () == nullptr);
  TEST_CHECK (!v);
  TEST_CHECK (!static_cast < bool > (v));
}

// 2. Copy constructor
static void test_copy_ctor () {
  // Empty copy
  {
    minilib::vector < int32_t > v1;
    minilib::vector < int32_t > v2 (v1);
    TEST_CHECK (v2.size () == 0);
    TEST_CHECK (v2.capacity () == 0);
    TEST_CHECK (v2.data () == nullptr);
  }
  // Non-empty trivial copy (tests memcpy branch)
  {
    minilib::vector < int32_t > v1;
    v1.push_back (1);
    v1.push_back (2);
    v1.push_back (3);
    minilib::vector < int32_t > v2 (v1);
    TEST_CHECK (v2.size () == 3);
    TEST_CHECK (v2.capacity () == 3);
    TEST_CHECK (v2[0] == 1 && v2[1] == 2 && v2[2] == 3);
    // Deep copy verification
    v1[0] = 99;
    TEST_CHECK (v2[0] == 1);
  }
  // Non-empty non-trivial copy
  {
    reset_tracker ();
    {
      minilib::vector < Tracker > v1;
      v1.emplace_back (1, 10);
      v1.emplace_back (2, 20);
      minilib::vector < Tracker > v2 (v1);
      TEST_CHECK (v2.size () == 2);
      TEST_CHECK (v2[0].val == 10 && v2[1].val == 20);
      TEST_CHECK (v2[0].id == 101 && v2[1].id == 102);
      TEST_CHECK (g_active_instances == 4);
    }
    TEST_CHECK (g_active_instances == 0);
  }
}

// 3. Move constructor
static void test_move_ctor () {
  // Empty move
  {
    minilib::vector < int32_t > v1;
    minilib::vector < int32_t > v2 (minilib::move (v1));
    TEST_CHECK (v2.size () == 0 && v2.capacity () == 0 && v2.data () == nullptr);
    TEST_CHECK (v1.size () == 0 && v1.capacity () == 0 && v1.data () == nullptr);
  }
  // Non-empty move
  {
    minilib::vector < int32_t > v1;
    v1.push_back (10);
    v1.push_back (20);
    size_t old_cap = v1.capacity ();
    int32_t * old_data = v1.data ();

    minilib::vector < int32_t > v2 (minilib::move (v1));
    TEST_CHECK (v2.size () == 2);
    TEST_CHECK (v2.capacity () == old_cap);
    TEST_CHECK (v2.data () == old_data);
    TEST_CHECK (v2[0] == 10 && v2[1] == 20);

    TEST_CHECK (v1.size () == 0);
    TEST_CHECK (v1.capacity () == 0);
    TEST_CHECK (v1.data () == nullptr);
  }
}

// 4. Copy assignment
static void test_copy_assignment () {
  // Self assignment
  {
    minilib::vector < int32_t > v;
    v.push_back (1);
    v.push_back (2);
    v = v;
    TEST_CHECK (v.size () == 2);
    TEST_CHECK (v[0] == 1 && v[1] == 2);
  }
  // other.len <= alloc_len (no reallocation)
  // Case 1.1: trivially copyable (memcpy), other.len == 0
  {
    minilib::vector < int32_t > v1;
    v1.push_back (1);
    v1.push_back (2);
    minilib::vector < int32_t > v2;
    v1 = v2;
    TEST_CHECK (v1.size () == 0);
  }
  // Case 1.1: trivially copyable, len < other.len <= alloc_len
  {
    minilib::vector < int32_t > v1;
    v1.reserve (10);
    v1.push_back (1);
    minilib::vector < int32_t > v2;
    v2.push_back (10);
    v2.push_back (20);
    v2.push_back (30);
    v1 = v2;
    TEST_CHECK (v1.size () == 3);
    TEST_CHECK (v1[0] == 10 && v1[1] == 20 && v1[2] == 30);
  }
  // Case 1.1: trivially copyable, len > other.len
  {
    minilib::vector < int32_t > v1;
    v1.push_back (1);
    v1.push_back (2);
    v1.push_back (3);
    minilib::vector < int32_t > v2;
    v2.push_back (100);
    v1 = v2;
    TEST_CHECK (v1.size () == 1);
    TEST_CHECK (v1[0] == 100);
  }
  // Case 2.1: trivially copyable, other.len > alloc_len (reallocation memcpy)
  {
    minilib::vector < int32_t > v1;
    v1.push_back (1);
    minilib::vector < int32_t > v2;
    for (int32_t i = 0; i < 20; i++) v2.push_back (i);
    v1 = v2;
    TEST_CHECK (v1.size () == 20);
    for (int32_t i = 0; i < 20; i++) TEST_CHECK (v1[i] == i);
  }
  // Case 1.2: non-trivially copyable copy assignment
  {
    reset_tracker ();
    {
      minilib::vector < Tracker > v1;
      v1.reserve (8);
      v1.emplace_back (1, 10);
      minilib::vector < Tracker > v2;
      v2.emplace_back (2, 20);
      v2.emplace_back (3, 30);
      v1 = v2;
      TEST_CHECK (v1.size () == 2);
      TEST_CHECK (v1[0].val == 20 && v1[1].val == 30);
    }
    TEST_CHECK (g_active_instances == 0);
  }
}

// 5. Move assignment
static void test_move_assignment () {
  // Self assignment
  {
    minilib::vector < int32_t > v;
    v.push_back (1);
    v = minilib::move (v);
    TEST_CHECK (v.size () == 1 && v[0] == 1);
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
    TEST_CHECK (v1.size () == 3);
    TEST_CHECK (v1.capacity () == old_cap);
    TEST_CHECK (v1.data () == old_data);
    TEST_CHECK (v1[0] == 10 && v1[1] == 20 && v1[2] == 30);

    TEST_CHECK (v2.size () == 0 && v2.capacity () == 0 && v2.data () == nullptr);
  }
}

// 6. Clear and Storage (reserve, resize_storage, clear)
static void test_clear_and_storage () {
  minilib::vector < int32_t > v;
  v.reserve (12);
  TEST_CHECK (v.capacity () >= 12);
  TEST_CHECK (v.size () == 0);
  TEST_CHECK (v.data () != nullptr);

  size_t cap = v.capacity ();
  v.reserve (5);
  TEST_CHECK (v.capacity () == cap);

  for (int32_t i = 0; i < 5; i++) v.push_back (i * 10);
  TEST_CHECK (v.size () == 5);

  v.clear ();
  TEST_CHECK (v.size () == 0);
  TEST_CHECK (v.capacity () == cap);
  TEST_CHECK (!v);

  v.clear (); // no-op
  TEST_CHECK (v.size () == 0);

  // resize_storage expand (trivially copyable memcpy)
  v.push_back (100);
  v.push_back (200);
  v.resize_storage (30);
  TEST_CHECK (v.capacity () == 30);
  TEST_CHECK (v.size () == 2);
  TEST_CHECK (v[0] == 100 && v[1] == 200);

  // resize_storage shrink with truncation
  v.push_back (300);
  v.push_back (400);
  v.resize_storage (3);
  TEST_CHECK (v.capacity () == 3);
  TEST_CHECK (v.size () == 3);
  TEST_CHECK (v[0] == 100 && v[1] == 200 && v[2] == 300);

  // resize_storage to 0
  v.resize_storage (0);
  TEST_CHECK (v.size () == 0);
  TEST_CHECK (v.capacity () == 0);
  TEST_CHECK (v.data () == nullptr);

  // resize_storage for non-trivial type
  reset_tracker ();
  {
    minilib::vector < Tracker > vt;
    vt.emplace_back (1, 10);
    vt.emplace_back (2, 20);
    vt.resize_storage (16);
    TEST_CHECK (vt.size () == 2);
    TEST_CHECK (vt.capacity () == 16);
    TEST_CHECK (vt[0].val == 10 && vt[1].val == 20);
    TEST_CHECK (g_active_instances == 2);

    vt.resize_storage (1); // shrink to 1
    TEST_CHECK (vt.size () == 1);
    TEST_CHECK (vt[0].val == 10);
    TEST_CHECK (g_active_instances == 1);
  }
  TEST_CHECK (g_active_instances == 0);
}

// 7. Push back (lvalue, rvalue, self-reference, growth)
static void test_push_back () {
  minilib::vector < int32_t > v;
  int32_t x = 10;
  v.push_back (x); // lvalue
  v.push_back (20); // rvalue
  TEST_CHECK (v.size () == 2);
  TEST_CHECK (v.capacity () >= 8);
  TEST_CHECK (v[0] == 10 && v[1] == 20);

  while (v.size () < v.capacity ()) {
    v.push_back (static_cast < int32_t > (v.size ()));
  }
  size_t full_cap = v.capacity ();
  TEST_CHECK (v.size () == full_cap);

  // Self-push_back edge case
  v.push_back (v[0]);
  TEST_CHECK (v.size () == full_cap + 1);
  TEST_CHECK (v.capacity () > full_cap);
  TEST_CHECK (v[full_cap] == 10);
}

// 8. Push back many
static void test_push_back_many () {
  minilib::vector < int32_t > v;
  v.push_back (1);
  v.push_back_many (2, 0); // count == 0
  TEST_CHECK (v.size () == 1);

  v.push_back_many (7, 3);
  TEST_CHECK (v.size () == 4);
  TEST_CHECK (v[0] == 1 && v[1] == 7 && v[2] == 7 && v[3] == 7);

  v.push_back_many (9, 20);
  TEST_CHECK (v.size () == 24);
  for (size_t i = 4; i < 24; i++) TEST_CHECK (v[i] == 9);

  // self-reference with expansion
  v.push_back_many (v[0], 50);
  TEST_CHECK (v.size () == 74);
  for (size_t i = 24; i < 74; i++) TEST_CHECK (v[i] == 1);
}

// 9. Emplace back and default
static void test_emplace_back () {
  minilib::vector < Point > v;
  v.emplace_back (3, 4);
  v.emplace_back (5, 6);
  TEST_CHECK (v.size () == 2);
  TEST_CHECK (v[0].x == 3 && v[0].y == 4);
  TEST_CHECK (v[1].x == 5 && v[1].y == 6);

  minilib::vector < DefaultInitProbe > vp;
  vp.emplace_back_default ();
  TEST_CHECK (vp.size () == 1);
  TEST_CHECK (vp[0].val == 12345);

  vp.emplace_back_many (3, 777);
  TEST_CHECK (vp.size () == 4);
  TEST_CHECK (vp[1].val == 777 && vp[2].val == 777 && vp[3].val == 777);

  vp.emplace_back_many_default (2);
  TEST_CHECK (vp.size () == 6);
  TEST_CHECK (vp[4].val == 12345 && vp[5].val == 12345);
}

// 10. Inform emplace / pop back many
static void test_inform_methods () {
  minilib::vector < int32_t > v;
  v.reserve (10);
  minilib::construct_at (v.data () + v.size (), 101);
  minilib::construct_at (v.data () + v.size () + 1, 102);
  minilib::construct_at (v.data () + v.size () + 2, 103);
  v.inform_emplace_back_many (3);

  TEST_CHECK (v.size () == 3);
  TEST_CHECK (v[0] == 101 && v[1] == 102 && v[2] == 103);

  minilib::destroy_at (v.data () + v.size () - 1);
  minilib::destroy_at (v.data () + v.size () - 2);
  v.inform_pop_back_many (2);

  TEST_CHECK (v.size () == 1);
  TEST_CHECK (v[0] == 101);
}

// 11. Generate
static void test_generate () {
  minilib::vector < int32_t > v;
  v.push_back (999);
  v.generate (4, [](size_t i) { return static_cast < int32_t > ((i + 1) * 10); });
  TEST_CHECK (v.size () == 5);
  TEST_CHECK (v[0] == 999);
  TEST_CHECK (v[1] == 10 && v[2] == 20 && v[3] == 30 && v[4] == 40);

  minilib::vector < Point > vp;
  vp.generate (3, [](size_t i) { return static_cast < int32_t > (i); },
                  [](size_t i) { return static_cast < int32_t > (i + 100); });
  TEST_CHECK (vp.size () == 3);
  TEST_CHECK (vp[0].x == 0 && vp[0].y == 100);
  TEST_CHECK (vp[1].x == 1 && vp[1].y == 101);
  TEST_CHECK (vp[2].x == 2 && vp[2].y == 102);
}

// 12. Pop back
static void test_pop_back () {
  minilib::vector < int32_t > v;
  v.push_back (10);
  v.push_back (20);
  v.push_back (30);
  v.push_back (40);
  v.push_back (50);

  v.pop_back ();
  TEST_CHECK (v.size () == 4);
  TEST_CHECK (v[3] == 40);

  v.pop_back_many (0);
  TEST_CHECK (v.size () == 4);

  // tests trivially destructible pop_back_many branch (len -= n)
  v.pop_back_many (2);
  TEST_CHECK (v.size () == 2);
  TEST_CHECK (v[0] == 10 && v[1] == 20);

  v.pop_back_many (2);
  TEST_CHECK (v.size () == 0);

  // Non-trivial pop_back_many (tests loop with destroy_at)
  reset_tracker ();
  {
    minilib::vector < Tracker > vt;
    vt.emplace_back (1, 10);
    vt.emplace_back (2, 20);
    vt.emplace_back (3, 30);
    TEST_CHECK (g_active_instances == 3);
    vt.pop_back ();
    TEST_CHECK (vt.size () == 2);
    TEST_CHECK (g_active_instances == 2);
    vt.pop_back_many (2);
    TEST_CHECK (vt.size () == 0);
    TEST_CHECK (g_active_instances == 0);
  }
  TEST_CHECK (g_active_instances == 0);
}

// 13. Element access and const correctness
static void test_element_access () {
  minilib::vector < int32_t > v;
  v.push_back (11);
  v.push_back (22);

  v[0] = 111;
  TEST_CHECK (v[0] == 111);
  *v.data () = 112;
  TEST_CHECK (v[0] == 112);

  const minilib::vector < int32_t > & cv = v;
  TEST_CHECK (cv.size () == 2);
  TEST_CHECK (cv[0] == 112 && cv[1] == 22);
  TEST_CHECK (cv.data ()[0] == 112);
  TEST_CHECK (static_cast < bool > (cv));
}

// 14. Shift by
static void test_shift_by () {
  // Trivially copyable type (tests __builtin_memmove branch)
  {
    minilib::vector < int32_t > v;
    v.push_back (1);
    v.push_back (2);
    v.push_back (3);

    // shift_by at end (index == len)
    v.shift_by (3, 2);
    minilib::construct_at (v.data () + 3, 4);
    minilib::construct_at (v.data () + 4, 5);
    TEST_CHECK (v.size () == 5);
    TEST_CHECK (v[0] == 1 && v[1] == 2 && v[2] == 3 && v[3] == 4 && v[4] == 5);

    // shift_by with shift == 0
    v.shift_by (1, 0);
    TEST_CHECK (v.size () == 5);

    // shift_by in middle (exercises memmove)
    v.shift_by (1, 2);
    v[1] = 88;
    v[2] = 99;
    TEST_CHECK (v.size () == 7);
    TEST_CHECK (v[0] == 1 && v[1] == 88 && v[2] == 99 && v[3] == 2 && v[4] == 3 && v[5] == 4 && v[6] == 5);
  }

  // Non-trivially copyable type (tests loop branch)
  {
    reset_tracker ();
    {
      minilib::vector < Tracker > vt;
      vt.emplace_back (1, 10);
      vt.emplace_back (2, 20);
      vt.emplace_back (3, 30);
      vt.shift_by (1, 1);
      minilib::construct_at < Tracker > (vt.data () + 1, 4, 40);
      TEST_CHECK (vt.size () == 4);
      TEST_CHECK (vt[0].val == 10 && vt[1].val == 40 && vt[2].val == 20 && vt[3].val == 30);
      TEST_CHECK (g_active_instances == 4);
    }
    TEST_CHECK (g_active_instances == 0);
  }
}

// 15. Insert (single element)
static void test_insert () {
  minilib::vector < int32_t > v;
  v.push_back (10);
  v.push_back (30);

  int32_t val20 = 20;
  v.insert_no_invalidate (1, val20); // lvalue
  TEST_CHECK (v.size () == 3);
  TEST_CHECK (v[0] == 10 && v[1] == 20 && v[2] == 30);

  v.insert_no_invalidate (0, 5); // rvalue
  TEST_CHECK (v.size () == 4);
  TEST_CHECK (v[0] == 5 && v[1] == 10 && v[2] == 20 && v[3] == 30);

  v.insert_no_invalidate (4, 40); // at len
  TEST_CHECK (v.size () == 5);
  TEST_CHECK (v[4] == 40);

  // self-reference insert
  v.insert_no_invalidate (2, v[0]);
  TEST_CHECK (v.size () == 6);
  TEST_CHECK (v[2] == 5);
}

// 16. Insert many
static void test_insert_many () {
  minilib::vector < int32_t > v;
  v.push_back (1);
  v.push_back (4);

  v.insert_many_no_invalidate (1, 0, 99); // count == 0
  TEST_CHECK (v.size () == 2);

  // index + count <= old_len
  v.insert_many_no_invalidate (1, 1, 2);
  TEST_CHECK (v.size () == 3);
  TEST_CHECK (v[0] == 1 && v[1] == 2 && v[2] == 4);

  // index + count > old_len
  v.insert_many_no_invalidate (2, 3, 3);
  TEST_CHECK (v.size () == 6);
  TEST_CHECK (v[0] == 1 && v[1] == 2 && v[2] == 3 && v[3] == 3 && v[4] == 3 && v[5] == 4);

  // at back (index == len)
  v.insert_many_no_invalidate (6, 2, 5);
  TEST_CHECK (v.size () == 8);
  TEST_CHECK (v[6] == 5 && v[7] == 5);
}

// 17. Emplace and default at index
static void test_emplace_at_index () {
  minilib::vector < Point > v;
  v.emplace_back (1, 1);
  v.emplace_back (3, 3);

  v.emplace_no_invalidate (1, 2, 2);
  TEST_CHECK (v.size () == 3);
  TEST_CHECK (v[0].x == 1 && v[1].x == 2 && v[2].x == 3);

  v.emplace_no_invalidate (3, 4, 4);
  TEST_CHECK (v.size () == 4);
  TEST_CHECK (v[3].x == 4);

  v.emplace_no_invalidate (0, 0, 0);
  TEST_CHECK (v.size () == 5);
  TEST_CHECK (v[0].x == 0);

  minilib::vector < DefaultInitProbe > vd;
  vd.emplace_back (10);
  vd.emplace_back (30);
  vd.emplace_default (1);
  TEST_CHECK (vd.size () == 3);
  TEST_CHECK (vd[0].val == 10 && vd[1].val == 12345 && vd[2].val == 30);

  minilib::vector < Point > vp;
  vp.emplace_back (1, 1);
  vp.emplace_back (2, 2);
  vp.emplace_back (5, 5);
  vp.emplace_many_no_invalidate (2, 1, 9, 9);
  TEST_CHECK (vp.size () == 4);
  TEST_CHECK (vp[2].x == 9 && vp[3].x == 5);

  vp.emplace_many_no_invalidate (3, 2, 8, 8);
  TEST_CHECK (vp.size () == 6);
  TEST_CHECK (vp[3].x == 8 && vp[4].x == 8 && vp[5].x == 5);

  minilib::vector < DefaultInitProbe > vde;
  vde.emplace_back (1);
  vde.emplace_back (2);
  vde.emplace_many_default (1, 2);
  TEST_CHECK (vde.size () == 4);
  TEST_CHECK (vde[0].val == 1 && vde[1].val == 12345 && vde[2].val == 12345 && vde[3].val == 2);

  minilib::vector < int32_t > vg;
  vg.push_back (1);
  vg.push_back (5);
  vg.generate_at_no_invalidate (1, 3, [](size_t i) { return static_cast < int32_t > (i + 2); });
  TEST_CHECK (vg.size () == 5);
  TEST_CHECK (vg[0] == 1 && vg[1] == 2 && vg[2] == 3 && vg[3] == 4 && vg[4] == 5);
}

// 17b. Insert (may_invalidate)
static void test_insert_may_invalidate () {
  minilib::vector < int32_t > v;
  v.push_back (10);
  v.push_back (30);

  int32_t val20 = 20;
  v.insert_may_invalidate (1, val20); // lvalue
  TEST_CHECK (v.size () == 3);
  TEST_CHECK (v[0] == 10 && v[1] == 20 && v[2] == 30);

  v.insert_may_invalidate (0, 5); // rvalue
  TEST_CHECK (v.size () == 4);
  TEST_CHECK (v[0] == 5 && v[1] == 10 && v[2] == 20 && v[3] == 30);

  v.insert_may_invalidate (4, 40); // at len
  TEST_CHECK (v.size () == 5);
  TEST_CHECK (v[4] == 40);

  // insert causing reallocation
  while (v.size () < v.capacity ()) {
    v.push_back (100);
  }
  size_t full_cap = v.capacity ();
  v.insert_may_invalidate (1, 999);
  TEST_CHECK (v.size () == full_cap + 1);
  TEST_CHECK (v.capacity () > full_cap);
  TEST_CHECK (v[1] == 999);

  // Non-trivial Tracker test
  reset_tracker ();
  {
    minilib::vector < Tracker > vt;
    vt.emplace_back (0, 10);
    vt.emplace_back (2, 30);
    Tracker t (1, 20);
    vt.insert_may_invalidate (1, t);
    TEST_CHECK (vt.size () == 3);
    TEST_CHECK (vt[0].val == 10 && vt[1].val == 20 && vt[2].val == 30);
    TEST_CHECK (g_active_instances == 4);
  }
  TEST_CHECK (g_active_instances == 0);
}

// 17c. Insert many (may_invalidate)
static void test_insert_many_may_invalidate () {
  minilib::vector < int32_t > v;
  v.push_back (1);
  v.push_back (4);

  v.insert_many_may_invalidate (1, 0, 99); // count == 0
  TEST_CHECK (v.size () == 2);

  v.insert_many_may_invalidate (1, 1, 2);
  TEST_CHECK (v.size () == 3);
  TEST_CHECK (v[0] == 1 && v[1] == 2 && v[2] == 4);

  v.insert_many_may_invalidate (2, 3, 3);
  TEST_CHECK (v.size () == 6);
  TEST_CHECK (v[0] == 1 && v[1] == 2 && v[2] == 3 && v[3] == 3 && v[4] == 3 && v[5] == 4);

  v.insert_many_may_invalidate (6, 2, 5);
  TEST_CHECK (v.size () == 8);
  TEST_CHECK (v[6] == 5 && v[7] == 5);

  // insert_many requiring reallocation
  v.insert_many_may_invalidate (1, 20, 77);
  TEST_CHECK (v.size () == 28);
  for (size_t i = 1; i < 21; i++) {
    TEST_CHECK (v[i] == 77);
  }
}

// 17d. Emplace and generate_at (may_invalidate)
static void test_emplace_may_invalidate_at_index () {
  minilib::vector < Point > v;
  v.emplace_back (1, 1);
  v.emplace_back (3, 3);

  v.emplace_may_invalidate (1, 2, 2);
  TEST_CHECK (v.size () == 3);
  TEST_CHECK (v[0].x == 1 && v[1].x == 2 && v[2].x == 3);

  v.emplace_may_invalidate (3, 4, 4);
  TEST_CHECK (v.size () == 4);
  TEST_CHECK (v[3].x == 4);

  v.emplace_may_invalidate (0, 0, 0);
  TEST_CHECK (v.size () == 5);
  TEST_CHECK (v[0].x == 0);

  // emplace_may_invalidate requiring reallocation
  while (v.size () < v.capacity ()) {
    v.emplace_back (9, 9);
  }
  size_t full_cap = v.capacity ();
  v.emplace_may_invalidate (2, 88, 88);
  TEST_CHECK (v.size () == full_cap + 1);
  TEST_CHECK (v.capacity () > full_cap);
  TEST_CHECK (v[2].x == 88 && v[2].y == 88);

  minilib::vector < Point > vp;
  vp.emplace_back (1, 1);
  vp.emplace_back (5, 5);
  vp.emplace_many_may_invalidate (1, 0, 9, 9);
  TEST_CHECK (vp.size () == 2);

  vp.emplace_many_may_invalidate (1, 2, 2, 2);
  TEST_CHECK (vp.size () == 4);
  TEST_CHECK (vp[0].x == 1 && vp[1].x == 2 && vp[2].x == 2 && vp[3].x == 5);

  vp.emplace_many_may_invalidate (2, 20, 7, 7);
  TEST_CHECK (vp.size () == 24);
  for (size_t i = 2; i < 22; i++) {
    TEST_CHECK (vp[i].x == 7 && vp[i].y == 7);
  }

  minilib::vector < int32_t > vg;
  vg.push_back (1);
  vg.push_back (5);
  vg.generate_at_may_invalidate (1, 0, [](size_t i) { return static_cast < int32_t > (i); });
  TEST_CHECK (vg.size () == 2);

  vg.generate_at_may_invalidate (1, 3, [](size_t i) { return static_cast < int32_t > (i + 2); });
  TEST_CHECK (vg.size () == 5);
  TEST_CHECK (vg[0] == 1 && vg[1] == 2 && vg[2] == 3 && vg[3] == 4 && vg[4] == 5);

  vg.generate_at_may_invalidate (2, 20, [](size_t i) { return static_cast < int32_t > (i + 50); });
  TEST_CHECK (vg.size () == 25);
  for (size_t i = 2; i < 22; i++) {
    TEST_CHECK (vg[i] == static_cast < int32_t > (i - 2 + 50));
  }
}

// 18. Erase (trivially copyable and non-trivial)
static void test_erase () {
  // Trivially copyable (__builtin_memmove branch)
  {
    minilib::vector < int32_t > v;
    for (int32_t i = 0; i < 6; i++) v.push_back (i);

    v.erase (2); // erase single in middle -> [0, 1, 3, 4, 5]
    TEST_CHECK (v.size () == 5);
    TEST_CHECK (v[0] == 0 && v[1] == 1 && v[2] == 3 && v[3] == 4 && v[4] == 5);

    v.erase (0); // erase single at front -> [1, 3, 4, 5]
    TEST_CHECK (v.size () == 4);
    TEST_CHECK (v[0] == 1 && v[1] == 3 && v[2] == 4 && v[3] == 5);

    v.erase (3); // erase single at back -> [1, 3, 4]
    TEST_CHECK (v.size () == 3);
    TEST_CHECK (v[0] == 1 && v[1] == 3 && v[2] == 4);

    v.erase (1, 0); // count == 0
    TEST_CHECK (v.size () == 3);

    v.push_back (5);
    v.push_back (6);
    // [1, 3, 4, 5, 6], erase index 1 count 2 -> [1, 5, 6]
    v.erase (1, 2);
    TEST_CHECK (v.size () == 3);
    TEST_CHECK (v[0] == 1 && v[1] == 5 && v[2] == 6);

    v.erase (0, 3); // erase all
    TEST_CHECK (v.size () == 0);
  }

  // Non-trivially copyable erase (move assign / loop branch)
  {
    reset_tracker ();
    {
      minilib::vector < Tracker > vt;
      vt.emplace_back (0, 10);
      vt.emplace_back (1, 20);
      vt.emplace_back (2, 30);
      vt.emplace_back (3, 40);
      TEST_CHECK (g_active_instances == 4);

      vt.erase (1); // erase index 1 (val 20) -> [10, 30, 40]
      TEST_CHECK (vt.size () == 3);
      TEST_CHECK (vt[0].val == 10 && vt[1].val == 30 && vt[2].val == 40);
      TEST_CHECK (g_active_instances == 3);

      vt.erase (1, 2); // erase remaining 30 and 40 -> [10]
      TEST_CHECK (vt.size () == 1);
      TEST_CHECK (vt[0].val == 10);
      TEST_CHECK (g_active_instances == 1);
    }
    TEST_CHECK (g_active_instances == 0);
  }
}

// 19. Move-only type support
static void test_move_only () {
  minilib::vector < MoveOnly > v;
  v.push_back (MoveOnly (10));
  v.emplace_back (20);
  v.insert_no_invalidate (1, MoveOnly (15));
  v.emplace_no_invalidate (0, 5);
  v.insert_may_invalidate (2, MoveOnly (12));
  v.emplace_may_invalidate (1, 7);
  TEST_CHECK (v.size () == 6);
  TEST_CHECK (v[0].val == 5 && v[1].val == 7 && v[2].val == 10 && v[3].val == 12 && v[4].val == 15 && v[5].val == 20);

  v.pop_back ();
  v.pop_back ();
  v.pop_back ();
  v.pop_back ();
  TEST_CHECK (v.size () == 2);
  TEST_CHECK (v[0].val == 5 && v[1].val == 7);

  v.erase (1);
  TEST_CHECK (v.size () == 1);
  TEST_CHECK (v[0].val == 5);
  v.push_back (MoveOnly (15));
  TEST_CHECK (v.size () == 2);
  TEST_CHECK (v[0].val == 5 && v[1].val == 15);

  minilib::vector < MoveOnly > v2 (minilib::move (v));
  TEST_CHECK (v2.size () == 2 && v.size () == 0);
  TEST_CHECK (v2[0].val == 5 && v2[1].val == 15);

  minilib::vector < MoveOnly > v3;
  v3 = minilib::move (v2);
  TEST_CHECK (v3.size () == 2 && v2.size () == 0);
  TEST_CHECK (v3[0].val == 5 && v3[1].val == 15);

  v3.reserve (20);
  TEST_CHECK (v3.capacity () >= 20);
  TEST_CHECK (v3[0].val == 5 && v3[1].val == 15);

  v3.clear ();
  TEST_CHECK (v3.size () == 0);
}

// 20. Non-trivial lifecycle verification with Tracker
static void test_tracker_lifecycle () {
  reset_tracker ();
  {
    minilib::vector < Tracker > vt;
    for (int32_t i = 0; i < 10; i++) {
      vt.emplace_back (i, i * 100);
    }
    TEST_CHECK (vt.size () == 10);
    TEST_CHECK (g_active_instances == 10);

    // Copy to another vector
    {
      minilib::vector < Tracker > vt2 (vt);
      TEST_CHECK (vt2.size () == 10);
      TEST_CHECK (g_active_instances == 20);
    }
    TEST_CHECK (g_active_instances == 10);

    // Clear
    vt.clear ();
    TEST_CHECK (vt.size () == 0);
    TEST_CHECK (g_active_instances == 0);
  }
  TEST_CHECK (g_active_instances == 0);
}

void main ([[maybe_unused]] void * sp) {
  main_tls.malloc_arena = &main_arena;
  set_thread_pointer (&main_tls);
  malloc_init ();

  test_default_ctor ();
  test_copy_ctor ();
  test_move_ctor ();
  test_copy_assignment ();
  test_move_assignment ();
  test_clear_and_storage ();
  test_push_back ();
  test_push_back_many ();
  test_emplace_back ();
  test_inform_methods ();
  test_generate ();
  test_pop_back ();
  test_element_access ();
  test_shift_by ();
  test_insert ();
  test_insert_may_invalidate ();
  test_insert_many ();
  test_insert_many_may_invalidate ();
  test_emplace_at_index ();
  test_emplace_may_invalidate_at_index ();
  test_erase ();
  test_move_only ();
  test_tracker_lifecycle ();

  exit (0);
}
