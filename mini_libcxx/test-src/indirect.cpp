/* Test indirect.hpp under runtime */

#include <stdint.h>
#include <tls.h>
#include <memory.h>
#include <exit.h>
#include <indirect.hpp>
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
static int32_t g_last_destructed_id = -1;
static int32_t g_destruct_order_count = 0;
static int32_t g_destruct_order[16];

static void reset_tracker () {
  g_active_instances = 0;
  g_last_destructed_id = -1;
  g_destruct_order_count = 0;
  for (size_t i = 0; i < 16; i++) g_destruct_order[i] = -1;
}

struct Tracker {
  int32_t id;
  int32_t val;

  Tracker () : id (0), val (42) {
    g_active_instances++;
  }

  Tracker (int32_t i, int32_t v) : id (i), val (v) {
    g_active_instances++;
  }

  Tracker (const Tracker & o) : id (o.id + 100), val (o.val) {
    g_active_instances++;
  }

  Tracker (Tracker && o) : id (o.id), val (o.val) {
    o.id = -1;
    o.val = -1;
    g_active_instances++;
  }

  Tracker & operator= (const Tracker & o) {
    id = o.id + 100;
    val = o.val;
    return *this;
  }

  Tracker & operator= (Tracker && o) {
    id = o.id;
    val = o.val;
    o.id = -1;
    o.val = -1;
    return *this;
  }

  ~Tracker () {
    g_last_destructed_id = id;
    if (g_destruct_order_count < 16) {
      g_destruct_order[g_destruct_order_count++] = id;
    }
    id = -999;
    val = -999;
    g_active_instances--;
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

// ==========================================
// Tests for minilib::indirect<T> (Runtime)
// ==========================================

static void test_indirect_default_ctor () {
  minilib::indirect < int32_t > ind;
  TEST_CHECK (!ind);
  TEST_CHECK (!static_cast < bool > (ind));
  TEST_CHECK (ind.data () == nullptr);
}

static void test_indirect_construct () {
  minilib::indirect < int32_t > ind1;
  ind1.construct (42);
  TEST_CHECK (static_cast < bool > (ind1));
  TEST_CHECK (ind1.data () != nullptr);
  TEST_CHECK (*ind1 == 42);

  minilib::indirect < Point > ind2;
  ind2.construct (10, 20);
  TEST_CHECK (ind2->x == 10 && ind2->y == 20);
  TEST_CHECK ((*ind2).x == 10 && (*ind2).y == 20);

  minilib::indirect < DefaultInitProbe > ind3;
  ind3.construct_default ();
  TEST_CHECK (ind3->val == 12345);

  minilib::indirect < int32_t > ind4;
  ind4.construct_null ();
  minilib::construct_at (ind4.data (), 999);
  TEST_CHECK (*ind4 == 999);
}

static void test_indirect_copy () {
  // copy empty
  {
    minilib::indirect < int32_t > ind1;
    minilib::indirect < int32_t > ind2 (ind1);
    TEST_CHECK (!ind2);
  }
  // copy populated (trivially copyable)
  {
    minilib::indirect < int32_t > ind1;
    ind1.construct (100);
    minilib::indirect < int32_t > ind2 (ind1);
    TEST_CHECK (static_cast < bool > (ind2));
    TEST_CHECK (*ind2 == 100);
    TEST_CHECK (ind1.data () != ind2.data ());
    *ind1 = 200;
    TEST_CHECK (*ind2 == 100); // deep copy
  }
  // copy populated (non-trivial tracker)
  reset_tracker ();
  {
    minilib::indirect < Tracker > ind1;
    ind1.construct (1, 55);
    TEST_CHECK (g_active_instances == 1);
    {
      minilib::indirect < Tracker > ind2 (ind1);
      TEST_CHECK (g_active_instances == 2);
      TEST_CHECK (ind2->id == 101 && ind2->val == 55);
    }
    TEST_CHECK (g_active_instances == 1);
  }
  TEST_CHECK (g_active_instances == 0);

  // copy assignment: self
  {
    minilib::indirect < int32_t > ind;
    ind.construct (77);
    ind = ind;
    TEST_CHECK (*ind == 77);
  }
  // copy assignment: non-empty to non-empty
  {
    minilib::indirect < int32_t > ind1;
    ind1.construct (10);
    minilib::indirect < int32_t > ind2;
    ind2.construct (20);
    ind1 = ind2;
    TEST_CHECK (*ind1 == 20);
    TEST_CHECK (ind1.data () != ind2.data ());
  }
  // copy assignment: empty to non-empty
  {
    minilib::indirect < int32_t > ind1;
    ind1.construct (10);
    minilib::indirect < int32_t > ind2;
    ind1 = ind2;
    TEST_CHECK (!ind1);
  }
}

static void test_indirect_move () {
  // move empty
  {
    minilib::indirect < int32_t > ind1;
    minilib::indirect < int32_t > ind2 (minilib::move (ind1));
    TEST_CHECK (!ind1 && !ind2);
  }
  // move populated
  {
    minilib::indirect < int32_t > ind1;
    ind1.construct (500);
    int32_t * original_ptr = ind1.data ();
    minilib::indirect < int32_t > ind2 (minilib::move (ind1));
    TEST_CHECK (!ind1);
    TEST_CHECK (ind2.data () == original_ptr);
    TEST_CHECK (*ind2 == 500);
  }
  // move assignment: self
  {
    minilib::indirect < int32_t > ind;
    ind.construct (88);
    ind = minilib::move (ind);
    TEST_CHECK (*ind == 88);
  }
  // move assignment: populated to populated
  {
    minilib::indirect < int32_t > ind1;
    ind1.construct (10);
    minilib::indirect < int32_t > ind2;
    ind2.construct (20);
    int32_t * ptr2 = ind2.data ();
    ind1 = minilib::move (ind2);
    TEST_CHECK (ind1.data () == ptr2);
    TEST_CHECK (*ind1 == 20);
    TEST_CHECK (!ind2);
  }
  // move-only type
  {
    minilib::indirect < MoveOnly > ind1;
    ind1.construct (777);
    minilib::indirect < MoveOnly > ind2 (minilib::move (ind1));
    TEST_CHECK (!ind1);
    TEST_CHECK (ind2->val == 777);
  }
}

static void test_indirect_destruct () {
  reset_tracker ();
  {
    minilib::indirect < Tracker > ind;
    ind.construct (9, 99);
    TEST_CHECK (g_active_instances == 1);
    ind.destruct ();
    TEST_CHECK (g_active_instances == 0);
    TEST_CHECK (!ind);
    TEST_CHECK (ind.data () == nullptr);
    // safe to destruct again
    ind.destruct ();
    TEST_CHECK (g_active_instances == 0);
  }
  TEST_CHECK (g_active_instances == 0);
}

static void test_indirect_const_correctness () {
  minilib::indirect < Point > ind;
  ind.construct (3, 4);

  ind->x = 30;
  *ind = Point (33, 44);
  TEST_CHECK (ind->x == 33 && ind->y == 44);

  const minilib::indirect < Point > & cind = ind;
  TEST_CHECK (cind->x == 33 && cind->y == 44);
  TEST_CHECK ((*cind).x == 33);
  TEST_CHECK (cind.data ()->x == 33);
  TEST_CHECK (static_cast < bool > (cind));
}

// ==========================================
// Tests for minilib::indirect_array<T> (Runtime)
// ==========================================

static void test_indirect_array_default_ctor () {
  minilib::indirect_array < int32_t > arr;
  TEST_CHECK (arr.size () == 0);
  TEST_CHECK (arr.data () == nullptr);
  TEST_CHECK (!arr);
  TEST_CHECK (!static_cast < bool > (arr));
}

static void test_indirect_array_construct () {
  minilib::indirect_array < int32_t > arr1;
  arr1.construct (4, 10);
  TEST_CHECK (arr1.size () == 4);
  TEST_CHECK (static_cast < bool > (arr1));
  TEST_CHECK (arr1.data () != nullptr);
  for (size_t i = 0; i < 4; i++) {
    TEST_CHECK (arr1[i] == 10);
  }

  // construct 0 elements
  minilib::indirect_array < int32_t > arr0;
  arr0.construct (0, 10);
  TEST_CHECK (arr0.size () == 0);
  TEST_CHECK (arr0.data () == nullptr);
  TEST_CHECK (!arr0);

  minilib::indirect_array < Point > arr2;
  arr2.construct (3, 5, 6);
  TEST_CHECK (arr2.size () == 3);
  for (size_t i = 0; i < 3; i++) {
    TEST_CHECK (arr2[i].x == 5 && arr2[i].y == 6);
  }

  minilib::indirect_array < DefaultInitProbe > arr3;
  arr3.construct_default (2);
  TEST_CHECK (arr3.size () == 2);
  TEST_CHECK (arr3[0].val == 12345 && arr3[1].val == 12345);

  minilib::indirect_array < int32_t > arr4;
  arr4.generate (5, [](size_t i) { return static_cast < int32_t > (i * 10); });
  TEST_CHECK (arr4.size () == 5);
  for (size_t i = 0; i < 5; i++) {
    TEST_CHECK (arr4[i] == static_cast < int32_t > (i * 10));
  }

  minilib::indirect_array < int32_t > arr5;
  arr5.construct_null (3);
  minilib::construct_at (arr5.data (), 101);
  minilib::construct_at (arr5.data () + 1, 102);
  minilib::construct_at (arr5.data () + 2, 103);
  TEST_CHECK (arr5.size () == 3);
  TEST_CHECK (arr5[0] == 101 && arr5[1] == 102 && arr5[2] == 103);
}

static void test_indirect_array_copy () {
  // copy empty
  {
    minilib::indirect_array < int32_t > arr1;
    minilib::indirect_array < int32_t > arr2 (arr1);
    TEST_CHECK (arr2.size () == 0 && arr2.data () == nullptr);
  }
  // copy populated (trivially copyable exercises __builtin_memcpy branch)
  {
    minilib::indirect_array < int32_t > arr1;
    arr1.generate (4, [](size_t i) { return static_cast < int32_t > (i + 1); });
    minilib::indirect_array < int32_t > arr2 (arr1);
    TEST_CHECK (arr2.size () == 4);
    for (size_t i = 0; i < 4; i++) TEST_CHECK (arr2[i] == static_cast < int32_t > (i + 1));
    arr1[0] = 999;
    TEST_CHECK (arr2[0] == 1);
  }
  // copy populated (non-trivial tracker)
  reset_tracker ();
  {
    minilib::indirect_array < Tracker > arr1;
    arr1.construct (2, 1, 42);
    TEST_CHECK (g_active_instances == 2);
    {
      minilib::indirect_array < Tracker > arr2 (arr1);
      TEST_CHECK (g_active_instances == 4);
      TEST_CHECK (arr2[0].id == 101 && arr2[0].val == 42);
      TEST_CHECK (arr2[1].id == 101 && arr2[1].val == 42);
    }
    TEST_CHECK (g_active_instances == 2);
  }
  TEST_CHECK (g_active_instances == 0);

  // copy assignment: self
  {
    minilib::indirect_array < int32_t > arr;
    arr.construct (3, 7);
    arr = arr;
    TEST_CHECK (arr.size () == 3 && arr[0] == 7);
  }
  // copy assignment: populated to populated (memcpy branch)
  {
    minilib::indirect_array < int32_t > arr1;
    arr1.construct (2, 11);
    minilib::indirect_array < int32_t > arr2;
    arr2.construct (4, 22);
    arr1 = arr2;
    TEST_CHECK (arr1.size () == 4);
    for (size_t i = 0; i < 4; i++) TEST_CHECK (arr1[i] == 22);
  }
  // copy assignment: empty to populated
  {
    minilib::indirect_array < int32_t > arr1;
    arr1.construct (3, 99);
    minilib::indirect_array < int32_t > arr2;
    arr1 = arr2;
    TEST_CHECK (arr1.size () == 0 && arr1.data () == nullptr);
  }
}

static void test_indirect_array_move () {
  // move empty
  {
    minilib::indirect_array < int32_t > arr1;
    minilib::indirect_array < int32_t > arr2 (minilib::move (arr1));
    TEST_CHECK (arr1.size () == 0 && arr2.size () == 0);
  }
  // move populated
  {
    minilib::indirect_array < int32_t > arr1;
    arr1.construct (3, 55);
    int32_t * original_ptr = arr1.data ();
    minilib::indirect_array < int32_t > arr2 (minilib::move (arr1));
    TEST_CHECK (arr1.size () == 0 && arr1.data () == nullptr);
    TEST_CHECK (arr2.size () == 3 && arr2.data () == original_ptr);
    TEST_CHECK (arr2[0] == 55 && arr2[1] == 55 && arr2[2] == 55);
  }
  // move assignment: self
  {
    minilib::indirect_array < int32_t > arr;
    arr.construct (2, 33);
    arr = minilib::move (arr);
    TEST_CHECK (arr.size () == 2 && arr[0] == 33);
  }
  // move assignment: populated to populated
  {
    minilib::indirect_array < int32_t > arr1;
    arr1.construct (2, 1);
    minilib::indirect_array < int32_t > arr2;
    arr2.construct (3, 2);
    int32_t * ptr2 = arr2.data ();
    arr1 = minilib::move (arr2);
    TEST_CHECK (arr1.size () == 3 && arr1.data () == ptr2);
    TEST_CHECK (arr2.size () == 0 && arr2.data () == nullptr);
  }
  // move-only array
  {
    minilib::indirect_array < MoveOnly > arr1;
    arr1.generate (3, [](size_t i) { return MoveOnly (static_cast < int32_t > (i + 10)); });
    minilib::indirect_array < MoveOnly > arr2 (minilib::move (arr1));
    TEST_CHECK (arr1.size () == 0 && arr2.size () == 3);
    TEST_CHECK (arr2[0].val == 10 && arr2[1].val == 11 && arr2[2].val == 12);
  }
}

static void test_indirect_array_destruct () {
  reset_tracker ();
  {
    minilib::indirect_array < Tracker > arr;
    arr.construct_null (3);
    minilib::construct_at (arr.data (), 0, 10);
    minilib::construct_at (arr.data () + 1, 1, 20);
    minilib::construct_at (arr.data () + 2, 2, 30);
    TEST_CHECK (g_active_instances == 3);

    arr.destruct ();
    TEST_CHECK (g_active_instances == 0);
    TEST_CHECK (arr.size () == 0 && arr.data () == nullptr);

    // Verify reverse order destruction: 2, then 1, then 0
    TEST_CHECK (g_destruct_order_count == 3);
    TEST_CHECK (g_destruct_order[0] == 2);
    TEST_CHECK (g_destruct_order[1] == 1);
    TEST_CHECK (g_destruct_order[2] == 0);

    // safe to destruct again
    arr.destruct ();
    TEST_CHECK (g_active_instances == 0);
  }
  TEST_CHECK (g_active_instances == 0);
}

static void test_indirect_array_const_correctness () {
  minilib::indirect_array < int32_t > arr;
  arr.construct (3, 10);
  arr[1] = 20;

  const minilib::indirect_array < int32_t > & carr = arr;
  TEST_CHECK (carr.size () == 3);
  TEST_CHECK (carr[0] == 10 && carr[1] == 20 && carr[2] == 10);
  TEST_CHECK (carr.data () == arr.data ());
  TEST_CHECK (static_cast < bool > (carr));
}

void main ([[maybe_unused]] void * sp) {
  main_tls.malloc_arena = &main_arena;
  set_thread_pointer (&main_tls);
  malloc_init ();

  test_indirect_default_ctor ();
  test_indirect_construct ();
  test_indirect_copy ();
  test_indirect_move ();
  test_indirect_destruct ();
  test_indirect_const_correctness ();

  test_indirect_array_default_ctor ();
  test_indirect_array_construct ();
  test_indirect_array_copy ();
  test_indirect_array_move ();
  test_indirect_array_destruct ();
  test_indirect_array_const_correctness ();

  exit (0);
}
