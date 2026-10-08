#include <stdint.h>
#include <tls.h>
#include <memory.h>
#include <exit.h>
#include <rbtree.hpp>
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

struct PrivateDestructProbe {
  int32_t val;
  PrivateDestructProbe () : val (42) {}
  explicit PrivateDestructProbe (int32_t v) : val (v) {}

  friend void destroy_probe (PrivateDestructProbe * p) { p->~PrivateDestructProbe (); }

private:
  ~PrivateDestructProbe () {}
};

// 1. Default constructor and empty state
static void test_default_ctor () {
  minilib::rbtree < int32_t > tree;
  TEST_CHECK (! static_cast < bool > (tree.root ()));
}

// 2. Emplace root (null, default, args)
static void test_emplace_root () {
  {
    minilib::rbtree < int32_t > tree;
    auto r = tree.emplace_root (42);
    TEST_CHECK (static_cast < bool > (r));
    TEST_CHECK (*(tree.data (r)) == 42);
    TEST_CHECK (*(tree.data (tree.root ())) == 42);
    TEST_CHECK (! static_cast < bool > (tree.left (r)));
    TEST_CHECK (! static_cast < bool > (tree.right (r)));
    TEST_CHECK (! static_cast < bool > (tree.parent (r)));
  }
  {
    minilib::rbtree < DefaultInitProbe > tree;
    auto r = tree.emplace_root_default ();
    TEST_CHECK (tree.data (r)->val == 12345);
  }
  {
    minilib::rbtree < Point > tree;
    auto r = tree.emplace_root (10, 20);
    TEST_CHECK (tree.data (r)->x == 10 && tree.data (r)->y == 20);
  }
  {
    minilib::rbtree < int32_t > tree;
    auto r = tree.emplace_root_null ();
    *(tree.data (r)) = 99;
    TEST_CHECK (*(tree.data (tree.root ())) == 99);
  }
}

// 3. Emplace left and right children and rebalancing
static void test_emplace_left_right () {
  minilib::rbtree < int32_t > tree;
  auto r = tree.emplace_root (10);
  auto l = tree.emplace_left (r, 5);
  auto right_child = tree.emplace_right (r, 15);

  TEST_CHECK (*(tree.data (tree.left (r))) == 5);
  TEST_CHECK (*(tree.data (tree.right (r))) == 15);
  TEST_CHECK (*(tree.data (tree.parent (l))) == *(tree.data (r)));
  TEST_CHECK (*(tree.data (tree.parent (right_child))) == *(tree.data (r)));

  // Emplace default and null on children
  minilib::rbtree < DefaultInitProbe > tree_def;
  auto rd = tree_def.emplace_root_default ();
  auto ld = tree_def.emplace_left_default (rd);
  auto rrd = tree_def.emplace_right_default (rd);
  TEST_CHECK (tree_def.data (ld)->val == 12345);
  TEST_CHECK (tree_def.data (rrd)->val == 12345);

  minilib::rbtree < int32_t > tree_null;
  auto rn = tree_null.emplace_root_null ();
  *(tree_null.data (rn)) = 1;
  auto ln = tree_null.emplace_left_null (rn);
  *(tree_null.data (ln)) = 2;
  auto rrn = tree_null.emplace_right_null (rn);
  *(tree_null.data (rrn)) = 3;
  TEST_CHECK (*(tree_null.data (tree_null.left (rn))) == 2);
  TEST_CHECK (*(tree_null.data (tree_null.right (rn))) == 3);
}

// 4. Test rebalancing on insertion (LL, LR, RR, RL)
static void test_insert_rotations () {
  // LL rotation
  {
    minilib::rbtree < int32_t > tree;
    auto r = tree.emplace_root (30);
    auto n20 = tree.emplace_left (r, 20);
    auto n10 = tree.emplace_left (n20, 10);
    // After inserting 10 as left of 20, 20 should become root, left 10, right 30
    auto root = tree.root ();
    TEST_CHECK (*(tree.data (root)) == 20);
    TEST_CHECK (*(tree.data (tree.left (root))) == 10);
    TEST_CHECK (*(tree.data (tree.right (root))) == 30);
  }
  // LR rotation
  {
    minilib::rbtree < int32_t > tree;
    auto r = tree.emplace_root (30);
    auto n10 = tree.emplace_left (r, 10);
    auto n20 = tree.emplace_right (n10, 20);
    auto root = tree.root ();
    TEST_CHECK (*(tree.data (root)) == 20);
    TEST_CHECK (*(tree.data (tree.left (root))) == 10);
    TEST_CHECK (*(tree.data (tree.right (root))) == 30);
  }
  // RR rotation
  {
    minilib::rbtree < int32_t > tree;
    auto r = tree.emplace_root (10);
    auto n20 = tree.emplace_right (r, 20);
    auto n30 = tree.emplace_right (n20, 30);
    auto root = tree.root ();
    TEST_CHECK (*(tree.data (root)) == 20);
    TEST_CHECK (*(tree.data (tree.left (root))) == 10);
    TEST_CHECK (*(tree.data (tree.right (root))) == 30);
  }
  // RL rotation
  {
    minilib::rbtree < int32_t > tree;
    auto r = tree.emplace_root (10);
    auto n30 = tree.emplace_right (r, 30);
    auto n20 = tree.emplace_left (n30, 20);
    auto root = tree.root ();
    TEST_CHECK (*(tree.data (root)) == 20);
    TEST_CHECK (*(tree.data (tree.left (root))) == 10);
    TEST_CHECK (*(tree.data (tree.right (root))) == 30);
  }
}

// 5. Remove node by handle
static void test_remove () {
  // Remove single root
  {
    minilib::rbtree < int32_t > tree;
    auto r = tree.emplace_root (42);
    tree.remove (r);
    TEST_CHECK (! static_cast < bool > (tree.root ()));
  }
  // Remove leaf
  {
    minilib::rbtree < int32_t > tree;
    auto r = tree.emplace_root (20);
    auto l = tree.emplace_left (r, 10);
    auto right_child = tree.emplace_right (r, 30);
    tree.remove (l);
    TEST_CHECK (! static_cast < bool > (tree.left (tree.root ())));
    TEST_CHECK (*(tree.data (tree.right (tree.root ()))) == 30);
  }
  // Remove node with children
  {
    minilib::rbtree < int32_t > tree;
    auto r = tree.emplace_root (20);
    auto l = tree.emplace_left (r, 10);
    auto right_child = tree.emplace_right (r, 30);
    tree.remove (r);
    auto new_root = tree.root ();
    TEST_CHECK (static_cast < bool > (new_root));
    TEST_CHECK (*(tree.data (new_root)) == 30);
    TEST_CHECK (*(tree.data (tree.left (new_root))) == 10);
  }
  // Remove with successor deeper in right subtree
  {
    minilib::rbtree < int32_t > tree;
    auto r = tree.emplace_root (20);
    auto l = tree.emplace_left (r, 10);
    auto right_child = tree.emplace_right (r, 40);
    auto rl = tree.emplace_left (right_child, 30);
    auto rr = tree.emplace_right (right_child, 50);

    tree.remove (r); // successor should be 30
    auto new_root = tree.root ();
    TEST_CHECK (*(tree.data (new_root)) == 30);
    TEST_CHECK (*(tree.data (tree.left (new_root))) == 10);
    TEST_CHECK (*(tree.data (tree.right (new_root))) == 40);
  }
}

// 6. Private destructor and inform_destruct
static void test_inform_destruct () {
  minilib::rbtree < PrivateDestructProbe > tree;
  auto r = tree.emplace_root_null ();
  PrivateDestructProbe * p = tree.data (r);
  ::new (static_cast < void * > (p)) PrivateDestructProbe (100);
  TEST_CHECK (tree.data (r)->val == 100);

  destroy_probe (tree.data (r));
  tree.inform_destruct (r);
  TEST_CHECK (! static_cast < bool > (tree.root ()));
}

// 7. Clear
static void test_clear () {
  minilib::rbtree < int32_t > tree;
  tree.clear ();
  TEST_CHECK (! static_cast < bool > (tree.root ()));

  auto r = tree.emplace_root (20);
  tree.emplace_left (r, 10);
  tree.emplace_right (r, 30);
  tree.clear ();
  TEST_CHECK (! static_cast < bool > (tree.root ()));

  // Re-populate after clear
  auto r2 = tree.emplace_root (50);
  TEST_CHECK (*(tree.data (r2)) == 50);
}

// 8. Lifecycle tracking
static void test_lifecycle () {
  reset_tracker ();
  {
    minilib::rbtree < Tracker > tree;
    auto r = tree.emplace_root (1, 10);
    auto l = tree.emplace_left (r, 2, 20);
    auto right_child = tree.emplace_right (r, 3, 30);
    TEST_CHECK (g_active_instances == 3);

    tree.remove (l);
    TEST_CHECK (g_active_instances == 2);
    TEST_CHECK (g_destruct_count == 1);
  }
  TEST_CHECK (g_active_instances == 0);
  TEST_CHECK (g_destruct_count == 3);
}

// 9. Copy constructor
static void test_copy_ctor () {
  minilib::rbtree < int32_t > tree1;
  auto r = tree1.emplace_root (20);
  auto l = tree1.emplace_left (r, 10);
  tree1.emplace_right (r, 30);
  tree1.emplace_left (l, 5);

  minilib::rbtree < int32_t > tree2 (tree1);
  auto r2 = tree2.root ();
  auto r1 = tree1.root ();
  TEST_CHECK (*(tree2.data (r2)) == *(tree1.data (r1)));
  TEST_CHECK (*(tree2.data (tree2.left (r2))) == *(tree1.data (tree1.left (r1))));
  TEST_CHECK (*(tree2.data (tree2.right (r2))) == *(tree1.data (tree1.right (r1))));
  TEST_CHECK (*(tree2.data (tree2.left (tree2.left (r2)))) == *(tree1.data (tree1.left (tree1.left (r1)))));

  // Modifying tree2 does not affect tree1
  *(tree2.data (r2)) = 999;
  TEST_CHECK (*(tree1.data (tree1.root ())) != 999);
  TEST_CHECK (*(tree2.data (tree2.root ())) == 999);
}

// 10. Copy assignment
static void test_copy_assignment () {
  minilib::rbtree < int32_t > tree1;
  auto r = tree1.emplace_root (20);
  tree1.emplace_left (r, 10);
  tree1.emplace_right (r, 30);

  minilib::rbtree < int32_t > tree2;
  auto r2 = tree2.emplace_root (50);
  tree2.emplace_left (r2, 40);

  // Assign tree1 to tree2 (storage reuse)
  tree2 = tree1;
  auto new_r2 = tree2.root ();
  TEST_CHECK (*(tree2.data (new_r2)) == 20);
  TEST_CHECK (*(tree2.data (tree2.left (new_r2))) == 10);
  TEST_CHECK (*(tree2.data (tree2.right (new_r2))) == 30);

  // Self assignment
  tree2 = tree2;
  TEST_CHECK (*(tree2.data (tree2.root ())) == 20);

  // Assign empty tree
  minilib::rbtree < int32_t > empty_tree;
  tree2 = empty_tree;
  TEST_CHECK (! static_cast < bool > (tree2.root ()));
}

// 11. Move constructor and move assignment
static void test_move_ctor_assignment () {
  minilib::rbtree < int32_t > tree1;
  auto r = tree1.emplace_root (20);
  tree1.emplace_left (r, 10);

  // Move ctor
  minilib::rbtree < int32_t > tree2 (minilib::move (tree1));
  TEST_CHECK (! static_cast < bool > (tree1.root ()));
  TEST_CHECK (static_cast < bool > (tree2.root ()));
  TEST_CHECK (*(tree2.data (tree2.root ())) == 20);

  // Move assignment
  minilib::rbtree < int32_t > tree3;
  tree3 = minilib::move (tree2);
  TEST_CHECK (! static_cast < bool > (tree2.root ()));
  TEST_CHECK (static_cast < bool > (tree3.root ()));
  TEST_CHECK (*(tree3.data (tree3.root ())) == 20);
}

// 12. Move-only type
static void test_move_only () {
  minilib::rbtree < MoveOnly > tree;
  auto r = tree.emplace_root (MoveOnly (10));
  auto l = tree.emplace_left (r, MoveOnly (5));
  TEST_CHECK (tree.data (r)->val == 10);
  TEST_CHECK (tree.data (l)->val == 5);

  minilib::rbtree < MoveOnly > tree_moved (minilib::move (tree));
  TEST_CHECK (tree_moved.data (tree_moved.root ())->val == 10);
}

// 13. Handle semantics and tombstone
static void test_handle_semantics () {
  minilib::rbtree < int32_t > tree;
  auto r = tree.emplace_root (42);
  minilib::rbtree < int32_t > :: handle_type h = r;

  tree.remove (r);
  TEST_CHECK (! static_cast < bool > (tree.root ()));
  // h is a tombstone handle, operator bool() should return false
  TEST_CHECK (! static_cast < bool > (h));
}

void main ([[maybe_unused]] void * sp) {
  main_tls.malloc_arena = &main_arena;
  set_thread_pointer (&main_tls);
  malloc_init ();

  test_default_ctor ();
  test_emplace_root ();
  test_emplace_left_right ();
  test_insert_rotations ();
  test_remove ();
  test_inform_destruct ();
  test_clear ();
  test_lifecycle ();
  test_copy_ctor ();
  test_copy_assignment ();
  test_move_ctor_assignment ();
  test_move_only ();
  test_handle_semantics ();

  exit (0);
}
