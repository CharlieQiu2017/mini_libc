#include <stdint.h>
#include <exit.h>
#include <counter.hpp>
#include <set.hpp>
#include <type_traits.hpp>
#include <utility.hpp>
#include <compare.hpp>

#define ASSERT_TRUE(cond) do { if (!(cond)) return false; } while (0)

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

  friend constexpr minilib::order_result compare_three_way (const ConstexprTracker& a, const ConstexprTracker& b) {
    return minilib::compare_three_way::operator () (a.val, b.val);
  }
};

constexpr bool test_default_ctor (minilib::counter & ctr) {
  minilib::set < int32_t > s;
  s.attach_counter (minilib::addressof (ctr));
  ASSERT_TRUE (s.empty ());
  ASSERT_TRUE (s.size () == 0);
  ASSERT_TRUE (! static_cast < bool > (s.root ()));
  ASSERT_TRUE (! static_cast < bool > (s.min ()));
  ASSERT_TRUE (! static_cast < bool > (s.max ()));
  return true;
}

constexpr bool test_insert_search (minilib::counter & ctr) {
  minilib::set < int32_t > s;
  s.attach_counter (minilib::addressof (ctr));

  auto p1 = s.insert (20);
  ASSERT_TRUE (p1.first == true);
  ASSERT_TRUE (static_cast < bool > (p1.second));
  ASSERT_TRUE (s.size () == 1);
  ASSERT_TRUE (s.get (p1.second) == 20);
  ASSERT_TRUE (*s.data (p1.second) == 20);

  // Duplicate insert
  auto p2 = s.insert (20);
  ASSERT_TRUE (p2.first == false);
  ASSERT_TRUE (s.size () == 1);

  // Multiple inserts
  s.insert (10);
  s.insert (30);
  s.insert (5);
  s.insert (15);
  ASSERT_TRUE (s.size () == 5);

  ASSERT_TRUE (s.contains (20));
  ASSERT_TRUE (s.contains (10));
  ASSERT_TRUE (s.contains (30));
  ASSERT_TRUE (s.contains (5));
  ASSERT_TRUE (s.contains (15));
  ASSERT_TRUE (! s.contains (100));

  auto h15 = s.search (15);
  ASSERT_TRUE (static_cast < bool > (h15));
  ASSERT_TRUE (s.get (h15) == 15);
  return true;
}

constexpr bool test_navigation_and_order (minilib::counter & ctr) {
  minilib::set < int32_t > s;
  s.attach_counter (minilib::addressof (ctr));
  int32_t values[] = { 50, 20, 80, 10, 30, 70, 90, 25, 35 };
  for (int32_t v : values) {
    s.insert (v);
  }
  ASSERT_TRUE (s.size () == 9);

  int32_t expected[] = { 10, 20, 25, 30, 35, 50, 70, 80, 90 };
  size_t idx = 0;
  for (auto h = s.min (); h; h = s.next (h)) {
    ASSERT_TRUE (s.get (h) == expected[idx]);
    idx++;
  }
  ASSERT_TRUE (idx == 9);

  idx = 9;
  for (auto h = s.max (); h; h = s.prev (h)) {
    idx--;
    ASSERT_TRUE (s.get (h) == expected[idx]);
  }
  ASSERT_TRUE (idx == 0);
  return true;
}

constexpr bool test_remove (minilib::counter & ctr) {
  minilib::set < int32_t > s;
  s.attach_counter (minilib::addressof (ctr));
  int32_t values[] = { 40, 20, 60, 10, 30, 50, 70 };
  for (int32_t v : values) {
    s.insert (v);
  }

  auto h30 = s.search (30);
  s.remove (h30);
  ASSERT_TRUE (s.size () == 6);
  ASSERT_TRUE (! s.contains (30));

  ASSERT_TRUE (s.remove (50));
  ASSERT_TRUE (s.size () == 5);
  ASSERT_TRUE (! s.contains (50));

  ASSERT_TRUE (! s.remove (999));
  ASSERT_TRUE (s.size () == 5);

  ASSERT_TRUE (s.remove (40));
  ASSERT_TRUE (s.remove (20));
  ASSERT_TRUE (s.remove (60));
  ASSERT_TRUE (s.remove (10));
  ASSERT_TRUE (s.remove (70));
  ASSERT_TRUE (s.empty ());
  return true;
}

constexpr bool test_clear (minilib::counter & ctr) {
  minilib::set < int32_t > s;
  s.attach_counter (minilib::addressof (ctr));
  for (int32_t i = 0; i < 15; ++i) {
    s.insert (i);
  }
  ASSERT_TRUE (s.size () == 15);
  s.clear ();
  ASSERT_TRUE (s.empty ());
  ASSERT_TRUE (s.size () == 0);
  return true;
}

constexpr bool test_copy_move (minilib::counter & ctr) {
  minilib::set < int32_t > s1;
  s1.attach_counter (minilib::addressof (ctr));
  s1.insert (10);
  s1.insert (20);
  s1.insert (30);

  minilib::set < int32_t > s2 (s1);
  ASSERT_TRUE (s2.size () == 3);
  ASSERT_TRUE (s2.contains (10));
  ASSERT_TRUE (s2.contains (20));
  ASSERT_TRUE (s2.contains (30));

  minilib::set < int32_t > s3 (minilib::move (s2));
  ASSERT_TRUE (s3.size () == 3);
  ASSERT_TRUE (s2.empty ());

  minilib::set < int32_t > s4;
  s4.attach_counter (minilib::addressof (ctr));
  s4 = s3;
  ASSERT_TRUE (s4.size () == 3);

  minilib::set < int32_t > s5;
  s5.attach_counter (minilib::addressof (ctr));
  s5 = minilib::move (s4);
  ASSERT_TRUE (s5.size () == 3);
  ASSERT_TRUE (s4.empty ());
  return true;
}

constexpr bool test_lifecycle (minilib::counter & ctr) {
  int32_t dc = 0;
  {
    minilib::set < ConstexprTracker > s;
    s.attach_counter (minilib::addressof (ctr));
    s.insert (ConstexprTracker (1, 10, minilib::addressof (dc)));
    s.insert (ConstexprTracker (2, 20, minilib::addressof (dc)));
    s.insert (ConstexprTracker (3, 30, minilib::addressof (dc)));
    s.remove (ConstexprTracker (0, 20));
    ASSERT_TRUE (s.size () == 2);
  }
  // All elements should have been destroyed properly
  ASSERT_TRUE (dc > 0);
  return true;
}

constexpr bool test_all () {
  minilib::counter ctr;
  ASSERT_TRUE (test_default_ctor (ctr));
  ASSERT_TRUE (test_insert_search (ctr));
  ASSERT_TRUE (test_navigation_and_order (ctr));
  ASSERT_TRUE (test_remove (ctr));
  ASSERT_TRUE (test_clear (ctr));
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
