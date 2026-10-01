#include <stdint.h>
#include <tls.h>
#include <memory.h>
#include <exit.h>
#include <map.hpp>
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

static int32_t g_val_active = 0;
static int32_t g_val_construct = 0;
static int32_t g_val_destruct = 0;

static void reset_tracker () {
  g_val_active = 0;
  g_val_construct = 0;
  g_val_destruct = 0;
}

struct ValueTracker {
  int32_t a;
  int32_t b;

  ValueTracker () : a (0), b (0) {
    g_val_active++;
    g_val_construct++;
  }

  ValueTracker (int32_t a_, int32_t b_) : a (a_), b (b_) {
    g_val_active++;
    g_val_construct++;
  }

  ValueTracker (const ValueTracker & o) : a (o.a), b (o.b) {
    g_val_active++;
    g_val_construct++;
  }

  ValueTracker (ValueTracker && o) : a (o.a), b (o.b) {
    g_val_active++;
    g_val_construct++;
    o.a = -1;
    o.b = -1;
  }

  ValueTracker & operator= (const ValueTracker & o) {
    a = o.a;
    b = o.b;
    return *this;
  }

  ValueTracker & operator= (ValueTracker && o) {
    a = o.a;
    b = o.b;
    o.a = -1;
    o.b = -1;
    return *this;
  }

  ~ValueTracker () {
    g_val_active--;
    g_val_destruct++;
    a = -999;
    b = -999;
  }
};

static void test_default_ctor () {
  minilib::map < int32_t, int32_t > m;
  TEST_CHECK (m.empty ());
  TEST_CHECK (m.size () == 0);
  TEST_CHECK (! static_cast < bool > (m.root ()));
  TEST_CHECK (! static_cast < bool > (m.min ()));
  TEST_CHECK (! static_cast < bool > (m.max ()));
}

static void test_initialization_ways () {
  minilib::map < int32_t, ValueTracker > m;

  // 1. insert(key, val)
  ValueTracker v1 (10, 20);
  auto p1 = m.insert (1, v1);
  TEST_CHECK (p1.first == true);
  TEST_CHECK (m.size () == 1);
  TEST_CHECK (m.key (p1.second) == 1);
  TEST_CHECK (m.value (p1.second).a == 10 && m.value (p1.second).b == 20);

  // 2. insert_default(key)
  auto p2 = m.insert_default (2);
  TEST_CHECK (p2.first == true);
  TEST_CHECK (m.size () == 2);
  TEST_CHECK (m.key (p2.second) == 2);
  TEST_CHECK (m.value (p2.second).a == 0 && m.value (p2.second).b == 0);

  // 3. emplace(key, args...) with multiple constructor arguments
  auto p3 = m.emplace (3, 30, 40);
  TEST_CHECK (p3.first == true);
  TEST_CHECK (m.size () == 3);
  TEST_CHECK (m.key (p3.second) == 3);
  TEST_CHECK (m.value (p3.second).a == 30 && m.value (p3.second).b == 40);

  // 4. insert_or_assign
  // Insert new
  auto p4 = m.insert_or_assign (4, ValueTracker (40, 50));
  TEST_CHECK (p4.first == true);
  TEST_CHECK (m.size () == 4);
  TEST_CHECK (m.value (p4.second).a == 40 && m.value (p4.second).b == 50);

  // Assign existing
  auto p4_assign = m.insert_or_assign (4, ValueTracker (400, 500));
  TEST_CHECK (p4_assign.first == false);
  TEST_CHECK (m.size () == 4);
  TEST_CHECK (m.value (p4.second).a == 400 && m.value (p4.second).b == 500);

  // 5. operator[]
  // Access existing
  m[3].a = 999;
  TEST_CHECK (m.value (p3.second).a == 999);

  // Insert default via operator[]
  m[5].a = 55;
  m[5].b = 66;
  TEST_CHECK (m.size () == 5);
  TEST_CHECK (m.at (5).a == 55 && m.at (5).b == 66);
}

static void test_search_and_access () {
  minilib::map < int32_t, int32_t > m;
  m.insert (10, 100);
  m.insert (20, 200);
  m.insert (30, 300);

  // search
  auto h20 = m.search (20);
  TEST_CHECK (static_cast < bool > (h20));
  TEST_CHECK (m.key (h20) == 20);
  TEST_CHECK (m.value (h20) == 200);

  // modify value via value(h)
  m.value (h20) = 250;
  TEST_CHECK (m.value (h20) == 250);
  TEST_CHECK (m.at (20) == 250);

  // contains
  TEST_CHECK (m.contains (10));
  TEST_CHECK (m.contains (20));
  TEST_CHECK (m.contains (30));
  TEST_CHECK (! m.contains (40));

  // search non-existent
  auto h99 = m.search (99);
  TEST_CHECK (! static_cast < bool > (h99));
}

static void test_navigation_and_order () {
  minilib::map < int32_t, int32_t > m;
  int32_t keys[] = { 50, 20, 80, 10, 30, 70, 90 };
  for (int32_t k : keys) {
    m.insert (k, k * 10);
  }
  TEST_CHECK (m.size () == 7);

  // In-order traversal
  int32_t expected_keys[] = { 10, 20, 30, 50, 70, 80, 90 };
  size_t idx = 0;
  for (auto h = m.min (); h; h = m.next (h)) {
    TEST_CHECK (m.key (h) == expected_keys[idx]);
    TEST_CHECK (m.value (h) == expected_keys[idx] * 10);
    idx++;
  }
  TEST_CHECK (idx == 7);

  // Reverse in-order traversal
  idx = 7;
  for (auto h = m.max (); h; h = m.prev (h)) {
    idx--;
    TEST_CHECK (m.key (h) == expected_keys[idx]);
  }
  TEST_CHECK (idx == 0);
}

static void test_remove () {
  minilib::map < int32_t, int32_t > m;
  m.insert (40, 400);
  m.insert (20, 200);
  m.insert (60, 600);
  m.insert (10, 100);
  m.insert (30, 300);

  // Remove by handle
  auto h20 = m.search (20);
  m.remove (h20);
  TEST_CHECK (m.size () == 4);
  TEST_CHECK (! m.contains (20));

  // Remove by key
  TEST_CHECK (m.remove (60));
  TEST_CHECK (m.size () == 3);
  TEST_CHECK (! m.contains (60));

  TEST_CHECK (! m.remove (999));
  TEST_CHECK (m.size () == 3);

  TEST_CHECK (m.remove (40));
  TEST_CHECK (m.remove (10));
  TEST_CHECK (m.remove (30));
  TEST_CHECK (m.empty ());
}

static void test_copy_and_move () {
  minilib::map < int32_t, int32_t > m1;
  m1.insert (1, 10);
  m1.insert (2, 20);
  m1.insert (3, 30);

  // Copy ctor
  minilib::map < int32_t, int32_t > m2 (m1);
  TEST_CHECK (m2.size () == 3);
  TEST_CHECK (m2.at (1) == 10);
  TEST_CHECK (m2.at (2) == 20);
  TEST_CHECK (m2.at (3) == 30);

  // Modify m2
  m2.at (2) = 222;
  TEST_CHECK (m1.at (2) == 20);
  TEST_CHECK (m2.at (2) == 222);

  // Copy assignment
  minilib::map < int32_t, int32_t > m3;
  m3.insert (9, 99);
  m3 = m1;
  TEST_CHECK (m3.size () == 3);
  TEST_CHECK (m3.at (2) == 20);

  // Move ctor
  minilib::map < int32_t, int32_t > m4 (minilib::move (m3));
  TEST_CHECK (m4.size () == 3);
  TEST_CHECK (m3.empty ());

  // Move assignment
  minilib::map < int32_t, int32_t > m5;
  m5 = minilib::move (m4);
  TEST_CHECK (m5.size () == 3);
  TEST_CHECK (m4.empty ());
}

static void test_lifecycle () {
  reset_tracker ();
  {
    minilib::map < int32_t, ValueTracker > m;
    m.emplace (1, 10, 20);
    m.emplace (2, 30, 40);
    m.emplace (3, 50, 60);
    TEST_CHECK (g_val_active == 3);

    m.remove (2);
    TEST_CHECK (g_val_active == 2);

    m.clear ();
    TEST_CHECK (g_val_active == 0);
  }
  TEST_CHECK (g_val_active == 0);
}

void main ([[maybe_unused]] void * sp) {
  main_tls.malloc_arena = &main_arena;
  set_thread_pointer (&main_tls);
  malloc_init ();

  test_default_ctor ();
  test_initialization_ways ();
  test_search_and_access ();
  test_navigation_and_order ();
  test_remove ();
  test_copy_and_move ();
  test_lifecycle ();

  exit (0);
}
