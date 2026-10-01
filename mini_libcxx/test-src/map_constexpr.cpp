#include <stdint.h>
#include <exit.h>
#include <counter.hpp>
#include <map.hpp>
#include <type_traits.hpp>
#include <utility.hpp>
#include <compare.hpp>

#define ASSERT_TRUE(cond) do { if (!(cond)) return false; } while (0)

struct ConstexprTracker {
  int32_t a;
  int32_t b;
  int32_t * destruct_count;

  constexpr ConstexprTracker () : a (0), b (0), destruct_count (nullptr) {}
  constexpr ConstexprTracker (int32_t a_, int32_t b_, int32_t * dc = nullptr) : a (a_), b (b_), destruct_count (dc) {}
  constexpr ConstexprTracker (const ConstexprTracker & o) : a (o.a), b (o.b), destruct_count (o.destruct_count) {}
  constexpr ConstexprTracker (ConstexprTracker && o) : a (o.a), b (o.b), destruct_count (o.destruct_count) {
    o.a = -1;
    o.b = -1;
  }
  constexpr ConstexprTracker & operator= (const ConstexprTracker & o) {
    a = o.a;
    b = o.b;
    destruct_count = o.destruct_count;
    return *this;
  }
  constexpr ConstexprTracker & operator= (ConstexprTracker && o) {
    a = o.a;
    b = o.b;
    destruct_count = o.destruct_count;
    o.a = -1;
    o.b = -1;
    return *this;
  }
  constexpr ~ConstexprTracker () {
    if (destruct_count) (*destruct_count)++;
    a = -999;
    b = -999;
  }
};

constexpr bool test_default_ctor (minilib::counter & ctr) {
  minilib::map < int32_t, int32_t > m;
  m.attach_counter (minilib::addressof (ctr));
  ASSERT_TRUE (m.empty ());
  ASSERT_TRUE (m.size () == 0);
  ASSERT_TRUE (! static_cast < bool > (m.root ()));
  ASSERT_TRUE (! static_cast < bool > (m.min ()));
  ASSERT_TRUE (! static_cast < bool > (m.max ()));
  return true;
}

constexpr bool test_initialization_ways (minilib::counter & ctr) {
  minilib::map < int32_t, ConstexprTracker > m;
  m.attach_counter (minilib::addressof (ctr));

  // 1. insert(key, val)
  ConstexprTracker v1 (10, 20);
  auto p1 = m.insert (1, v1);
  ASSERT_TRUE (p1.first == true);
  ASSERT_TRUE (m.size () == 1);
  ASSERT_TRUE (m.key (p1.second) == 1);
  ASSERT_TRUE (m.value (p1.second).a == 10 && m.value (p1.second).b == 20);

  // 2. insert_default(key)
  auto p2 = m.insert_default (2);
  ASSERT_TRUE (p2.first == true);
  ASSERT_TRUE (m.size () == 2);
  ASSERT_TRUE (m.key (p2.second) == 2);
  ASSERT_TRUE (m.value (p2.second).a == 0 && m.value (p2.second).b == 0);

  // 3. emplace(key, args...)
  auto p3 = m.emplace (3, 30, 40);
  ASSERT_TRUE (p3.first == true);
  ASSERT_TRUE (m.size () == 3);
  ASSERT_TRUE (m.key (p3.second) == 3);
  ASSERT_TRUE (m.value (p3.second).a == 30 && m.value (p3.second).b == 40);

  // 4. insert_or_assign
  auto p4 = m.insert_or_assign (4, ConstexprTracker (40, 50));
  ASSERT_TRUE (p4.first == true);
  ASSERT_TRUE (m.size () == 4);
  ASSERT_TRUE (m.value (p4.second).a == 40 && m.value (p4.second).b == 50);

  auto p4_assign = m.insert_or_assign (4, ConstexprTracker (400, 500));
  ASSERT_TRUE (p4_assign.first == false);
  ASSERT_TRUE (m.size () == 4);
  ASSERT_TRUE (m.value (p4.second).a == 400 && m.value (p4.second).b == 500);

  // 5. operator[]
  m[3].a = 999;
  ASSERT_TRUE (m.value (p3.second).a == 999);

  m[5].a = 55;
  m[5].b = 66;
  ASSERT_TRUE (m.size () == 5);
  ASSERT_TRUE (m.at (5).a == 55 && m.at (5).b == 66);

  return true;
}

constexpr bool test_search_and_access (minilib::counter & ctr) {
  minilib::map < int32_t, int32_t > m;
  m.attach_counter (minilib::addressof (ctr));
  m.insert (10, 100);
  m.insert (20, 200);
  m.insert (30, 300);

  auto h20 = m.search (20);
  ASSERT_TRUE (static_cast < bool > (h20));
  ASSERT_TRUE (m.key (h20) == 20);
  ASSERT_TRUE (m.value (h20) == 200);

  m.value (h20) = 250;
  ASSERT_TRUE (m.value (h20) == 250);
  ASSERT_TRUE (m.at (20) == 250);

  ASSERT_TRUE (m.contains (10));
  ASSERT_TRUE (m.contains (20));
  ASSERT_TRUE (m.contains (30));
  ASSERT_TRUE (! m.contains (40));

  auto h99 = m.search (99);
  ASSERT_TRUE (! static_cast < bool > (h99));
  return true;
}

constexpr bool test_navigation_and_order (minilib::counter & ctr) {
  minilib::map < int32_t, int32_t > m;
  m.attach_counter (minilib::addressof (ctr));
  int32_t keys[] = { 50, 20, 80, 10, 30, 70, 90 };
  for (int32_t k : keys) {
    m.insert (k, k * 10);
  }
  ASSERT_TRUE (m.size () == 7);

  int32_t expected_keys[] = { 10, 20, 30, 50, 70, 80, 90 };
  size_t idx = 0;
  for (auto h = m.min (); h; h = m.next (h)) {
    ASSERT_TRUE (m.key (h) == expected_keys[idx]);
    ASSERT_TRUE (m.value (h) == expected_keys[idx] * 10);
    idx++;
  }
  ASSERT_TRUE (idx == 7);

  idx = 7;
  for (auto h = m.max (); h; h = m.prev (h)) {
    idx--;
    ASSERT_TRUE (m.key (h) == expected_keys[idx]);
  }
  ASSERT_TRUE (idx == 0);
  return true;
}

constexpr bool test_remove (minilib::counter & ctr) {
  minilib::map < int32_t, int32_t > m;
  m.attach_counter (minilib::addressof (ctr));
  m.insert (40, 400);
  m.insert (20, 200);
  m.insert (60, 600);
  m.insert (10, 100);
  m.insert (30, 300);

  auto h20 = m.search (20);
  m.remove (h20);
  ASSERT_TRUE (m.size () == 4);
  ASSERT_TRUE (! m.contains (20));

  ASSERT_TRUE (m.remove (60));
  ASSERT_TRUE (m.size () == 3);
  ASSERT_TRUE (! m.contains (60));

  ASSERT_TRUE (! m.remove (999));
  ASSERT_TRUE (m.size () == 3);

  ASSERT_TRUE (m.remove (40));
  ASSERT_TRUE (m.remove (10));
  ASSERT_TRUE (m.remove (30));
  ASSERT_TRUE (m.empty ());
  return true;
}

constexpr bool test_copy_move (minilib::counter & ctr) {
  minilib::map < int32_t, int32_t > m1;
  m1.attach_counter (minilib::addressof (ctr));
  m1.insert (1, 10);
  m1.insert (2, 20);
  m1.insert (3, 30);

  minilib::map < int32_t, int32_t > m2 (m1);
  ASSERT_TRUE (m2.size () == 3);
  ASSERT_TRUE (m2.at (1) == 10);
  ASSERT_TRUE (m2.at (2) == 20);
  ASSERT_TRUE (m2.at (3) == 30);

  m2.at (2) = 222;
  ASSERT_TRUE (m1.at (2) == 20);
  ASSERT_TRUE (m2.at (2) == 222);

  minilib::map < int32_t, int32_t > m3;
  m3.attach_counter (minilib::addressof (ctr));
  m3.insert (9, 99);
  m3 = m1;
  ASSERT_TRUE (m3.size () == 3);
  ASSERT_TRUE (m3.at (2) == 20);

  minilib::map < int32_t, int32_t > m4 (minilib::move (m3));
  ASSERT_TRUE (m4.size () == 3);
  ASSERT_TRUE (m3.empty ());

  minilib::map < int32_t, int32_t > m5;
  m5.attach_counter (minilib::addressof (ctr));
  m5 = minilib::move (m4);
  ASSERT_TRUE (m5.size () == 3);
  ASSERT_TRUE (m4.empty ());
  return true;
}

constexpr bool test_lifecycle (minilib::counter & ctr) {
  int32_t dc = 0;
  {
    minilib::map < int32_t, ConstexprTracker > m;
    m.attach_counter (minilib::addressof (ctr));
    m.emplace (1, 10, 20, minilib::addressof (dc));
    m.emplace (2, 30, 40, minilib::addressof (dc));
    m.emplace (3, 50, 60, minilib::addressof (dc));
    m.remove (2);
    ASSERT_TRUE (m.size () == 2);
  }
  ASSERT_TRUE (dc > 0);
  return true;
}

constexpr bool test_all () {
  minilib::counter ctr;
  ASSERT_TRUE (test_default_ctor (ctr));
  ASSERT_TRUE (test_initialization_ways (ctr));
  ASSERT_TRUE (test_search_and_access (ctr));
  ASSERT_TRUE (test_navigation_and_order (ctr));
  ASSERT_TRUE (test_remove (ctr));
  ASSERT_TRUE (test_copy_move (ctr));
  ASSERT_TRUE (test_lifecycle (ctr));
  return true;
}

constexpr bool passed = test_all ();
static_assert (passed);

void main ([[maybe_unused]] void * sp) {
  if (! passed) exit (1);
  exit (0);
}
