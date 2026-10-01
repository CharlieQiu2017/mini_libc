#include <stdint.h>
#include <tls.h>
#include <memory.h>
#include <exit.h>
#include <set.hpp>
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

static int32_t g_active_instances = 0;
static int32_t g_construct_count = 0;
static int32_t g_copy_construct_count = 0;
static int32_t g_move_construct_count = 0;
static int32_t g_destruct_count = 0;

static void reset_tracker () {
  g_active_instances = 0;
  g_construct_count = 0;
  g_copy_construct_count = 0;
  g_move_construct_count = 0;
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
    g_active_instances++;
    g_move_construct_count++;
    o.id = -1;
    o.val = -1;
  }

  Tracker & operator= (const Tracker & o) = default;
  Tracker & operator= (Tracker && o) = default;

  ~Tracker () {
    g_active_instances--;
    g_destruct_count++;
    id = -999;
    val = -999;
  }

  friend constexpr minilib::order_result compare_three_way (const Tracker& a, const Tracker& b) {
    return minilib::compare_three_way::operator () (a.val, b.val);
  }
};

struct MoveOnly {
  int32_t val;
  MoveOnly () : val (0) {}
  explicit MoveOnly (int32_t v) : val (v) {}
  MoveOnly (const MoveOnly &) = delete;
  MoveOnly & operator= (const MoveOnly &) = delete;
  MoveOnly (MoveOnly && o) : val (o.val) { o.val = -1; }
  MoveOnly & operator= (MoveOnly && o) { val = o.val; o.val = -1; return *this; }
  ~MoveOnly () = default;

  friend constexpr minilib::order_result compare_three_way (const MoveOnly& a, const MoveOnly& b) {
    return minilib::compare_three_way::operator () (a.val, b.val);
  }
};

static void test_default_ctor () {
  minilib::set < int32_t > s;
  TEST_CHECK (s.empty ());
  TEST_CHECK (s.size () == 0);
  TEST_CHECK (! static_cast < bool > (s.root ()));
  TEST_CHECK (! static_cast < bool > (s.min ()));
  TEST_CHECK (! static_cast < bool > (s.max ()));
}

static void test_insert_and_search () {
  minilib::set < int32_t > s;
  auto p1 = s.insert (20);
  TEST_CHECK (p1.first == true);
  TEST_CHECK (static_cast < bool > (p1.second));
  TEST_CHECK (s.size () == 1);
  TEST_CHECK (s.get (p1.second) == 20);
  TEST_CHECK (*s.data (p1.second) == 20);

  // Duplicate insert
  auto p2 = s.insert (20);
  TEST_CHECK (p2.first == false);
  TEST_CHECK (minilib::compare_three_way::operator () (p2.second, p1.second) == 0);
  TEST_CHECK (s.size () == 1);

  // More inserts
  auto p3 = s.insert (10);
  TEST_CHECK (p3.first == true);
  TEST_CHECK (s.size () == 2);

  auto p4 = s.insert (30);
  TEST_CHECK (p4.first == true);
  TEST_CHECK (s.size () == 3);

  // Search existing
  auto h20 = s.search (20);
  TEST_CHECK (static_cast < bool > (h20));
  TEST_CHECK (s.get (h20) == 20);
  TEST_CHECK (s.contains (20));

  auto h10 = s.search (10);
  TEST_CHECK (static_cast < bool > (h10));
  TEST_CHECK (s.get (h10) == 10);
  TEST_CHECK (s.contains (10));

  auto h30 = s.search (30);
  TEST_CHECK (static_cast < bool > (h30));
  TEST_CHECK (s.get (h30) == 30);
  TEST_CHECK (s.contains (30));

  // Search non-existent
  auto h40 = s.search (40);
  TEST_CHECK (! static_cast < bool > (h40));
  TEST_CHECK (! s.contains (40));

  auto h5 = s.search (5);
  TEST_CHECK (! static_cast < bool > (h5));
  TEST_CHECK (! s.contains (5));
}

static void test_navigation_and_order () {
  minilib::set < int32_t > s;
  int32_t values[] = { 50, 20, 80, 10, 30, 70, 90, 25, 35 };
  for (int32_t v : values) {
    s.insert (v);
  }
  TEST_CHECK (s.size () == 9);

  // In-order traversal using min() and next()
  int32_t expected[] = { 10, 20, 25, 30, 35, 50, 70, 80, 90 };
  size_t idx = 0;
  for (auto h = s.min (); h; h = s.next (h)) {
    TEST_CHECK (s.get (h) == expected[idx]);
    idx++;
  }
  TEST_CHECK (idx == 9);

  // Reverse in-order traversal using max() and prev()
  idx = 9;
  for (auto h = s.max (); h; h = s.prev (h)) {
    idx--;
    TEST_CHECK (s.get (h) == expected[idx]);
  }
  TEST_CHECK (idx == 0);
}

static void test_remove () {
  minilib::set < int32_t > s;
  int32_t values[] = { 40, 20, 60, 10, 30, 50, 70 };
  for (int32_t v : values) {
    s.insert (v);
  }

  // Remove by handle
  auto h30 = s.search (30);
  TEST_CHECK (static_cast < bool > (h30));
  s.remove (h30);
  TEST_CHECK (s.size () == 6);
  TEST_CHECK (! s.contains (30));

  // Remove by value
  bool rem_ok = s.remove (50);
  TEST_CHECK (rem_ok == true);
  TEST_CHECK (s.size () == 5);
  TEST_CHECK (! s.contains (50));

  bool rem_fail = s.remove (999);
  TEST_CHECK (rem_fail == false);
  TEST_CHECK (s.size () == 5);

  // Remove remaining one by one
  TEST_CHECK (s.remove (40));
  TEST_CHECK (s.remove (20));
  TEST_CHECK (s.remove (60));
  TEST_CHECK (s.remove (10));
  TEST_CHECK (s.remove (70));
  TEST_CHECK (s.empty ());
  TEST_CHECK (s.size () == 0);
}

static void test_clear () {
  minilib::set < int32_t > s;
  for (int32_t i = 0; i < 20; ++i) {
    s.insert (i);
  }
  TEST_CHECK (s.size () == 20);
  s.clear ();
  TEST_CHECK (s.empty ());
  TEST_CHECK (s.size () == 0);
  TEST_CHECK (! static_cast < bool > (s.root ()));
}

static void test_copy_and_move () {
  minilib::set < int32_t > s1;
  s1.insert (10);
  s1.insert (20);
  s1.insert (30);

  // Copy ctor
  minilib::set < int32_t > s2 (s1);
  TEST_CHECK (s2.size () == 3);
  TEST_CHECK (s2.contains (10));
  TEST_CHECK (s2.contains (20));
  TEST_CHECK (s2.contains (30));

  // Modify s2
  s2.remove (20);
  TEST_CHECK (! s2.contains (20));
  TEST_CHECK (s1.contains (20));

  // Copy assignment
  minilib::set < int32_t > s3;
  s3.insert (100);
  s3 = s1;
  TEST_CHECK (s3.size () == 3);
  TEST_CHECK (s3.contains (20));

  // Move ctor
  minilib::set < int32_t > s4 (minilib::move (s3));
  TEST_CHECK (s4.size () == 3);
  TEST_CHECK (s3.empty ());

  // Move assignment
  minilib::set < int32_t > s5;
  s5 = minilib::move (s4);
  TEST_CHECK (s5.size () == 3);
  TEST_CHECK (s4.empty ());
}

static void test_move_only () {
  minilib::set < MoveOnly > s;
  s.insert (MoveOnly (10));
  s.insert (MoveOnly (5));
  s.insert (MoveOnly (15));
  TEST_CHECK (s.size () == 3);

  auto h = s.search (MoveOnly (10));
  TEST_CHECK (static_cast < bool > (h));
  TEST_CHECK (s.get (h).val == 10);

  minilib::set < MoveOnly > s2 (minilib::move (s));
  TEST_CHECK (s2.size () == 3);
  TEST_CHECK (s.empty ());
}

static void test_lifecycle () {
  reset_tracker ();
  {
    minilib::set < Tracker > s;
    s.insert (Tracker (1, 10));
    s.insert (Tracker (2, 20));
    s.insert (Tracker (3, 30));
    TEST_CHECK (g_active_instances == 3);

    s.remove (Tracker (0, 20));
    TEST_CHECK (g_active_instances == 2);
  }
  TEST_CHECK (g_active_instances == 0);
}

void main ([[maybe_unused]] void * sp) {
  main_tls.malloc_arena = &main_arena;
  set_thread_pointer (&main_tls);
  malloc_init ();

  test_default_ctor ();
  test_insert_and_search ();
  test_navigation_and_order ();
  test_remove ();
  test_clear ();
  test_copy_and_move ();
  test_move_only ();
  test_lifecycle ();

  exit (0);
}
