#include <stdint.h>
#include <exit.h>
#include <counter.hpp>
#include <rbtree.hpp>
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

// 1. Default constructor
constexpr bool test_default_ctor (minilib::counter & ctr) {
  minilib::rbtree < int32_t > tree;
  tree.attach_counter (minilib::addressof (ctr));
  ASSERT_TRUE (! static_cast < bool > (tree.root ()));
  return true;
}

// 2. Emplace root
constexpr bool test_emplace_root (minilib::counter & ctr) {
  {
    minilib::rbtree < int32_t > tree;
    tree.attach_counter (minilib::addressof (ctr));
    auto r = tree.emplace_root (42);
    ASSERT_TRUE (static_cast < bool > (r));
    ASSERT_TRUE (*(tree.data (r)) == 42);
    ASSERT_TRUE (*(tree.data (tree.root ())) == 42);
    ASSERT_TRUE (! static_cast < bool > (tree.left (r)));
    ASSERT_TRUE (! static_cast < bool > (tree.right (r)));
    ASSERT_TRUE (! static_cast < bool > (tree.parent (r)));
  }
  {
    minilib::rbtree < DefaultInitProbe > tree;
    tree.attach_counter (minilib::addressof (ctr));
    auto r = tree.emplace_root_default ();
    ASSERT_TRUE (tree.data (r)->val == 12345);
  }
  {
    minilib::rbtree < Point > tree;
    tree.attach_counter (minilib::addressof (ctr));
    auto r = tree.emplace_root (10, 20);
    ASSERT_TRUE (tree.data (r)->x == 10 && tree.data (r)->y == 20);
  }
  {
    minilib::rbtree < int32_t > tree;
    tree.attach_counter (minilib::addressof (ctr));
    auto r = tree.emplace_root_null ();
    minilib::construct_at < int32_t > (tree.data (r), 99);
    ASSERT_TRUE (*(tree.data (tree.root ())) == 99);
  }
  return true;
}

// 3. Emplace left and right
constexpr bool test_emplace_left_right (minilib::counter & ctr) {
  minilib::rbtree < int32_t > tree;
  tree.attach_counter (minilib::addressof (ctr));
  auto r = tree.emplace_root (10);
  auto l = tree.emplace_left (r, 5);
  auto right_child = tree.emplace_right (r, 15);

  ASSERT_TRUE (*(tree.data (tree.left (r))) == 5);
  ASSERT_TRUE (*(tree.data (tree.right (r))) == 15);
  ASSERT_TRUE (*(tree.data (tree.parent (l))) == *(tree.data (r)));
  ASSERT_TRUE (*(tree.data (tree.parent (right_child))) == *(tree.data (r)));

  return true;
}

// 4. Insertion rotations
constexpr bool test_insert_rotations (minilib::counter & ctr) {
  // LL rotation
  {
    minilib::rbtree < int32_t > tree;
    tree.attach_counter (minilib::addressof (ctr));
    auto r = tree.emplace_root (30);
    auto n20 = tree.emplace_left (r, 20);
    tree.emplace_left (n20, 10);
    auto root = tree.root ();
    ASSERT_TRUE (*(tree.data (root)) == 20);
    ASSERT_TRUE (*(tree.data (tree.left (root))) == 10);
    ASSERT_TRUE (*(tree.data (tree.right (root))) == 30);
  }
  // LR rotation
  {
    minilib::rbtree < int32_t > tree;
    tree.attach_counter (minilib::addressof (ctr));
    auto r = tree.emplace_root (30);
    auto n10 = tree.emplace_left (r, 10);
    tree.emplace_right (n10, 20);
    auto root = tree.root ();
    ASSERT_TRUE (*(tree.data (root)) == 20);
    ASSERT_TRUE (*(tree.data (tree.left (root))) == 10);
    ASSERT_TRUE (*(tree.data (tree.right (root))) == 30);
  }
  // RR rotation
  {
    minilib::rbtree < int32_t > tree;
    tree.attach_counter (minilib::addressof (ctr));
    auto r = tree.emplace_root (10);
    auto n20 = tree.emplace_right (r, 20);
    tree.emplace_right (n20, 30);
    auto root = tree.root ();
    ASSERT_TRUE (*(tree.data (root)) == 20);
    ASSERT_TRUE (*(tree.data (tree.left (root))) == 10);
    ASSERT_TRUE (*(tree.data (tree.right (root))) == 30);
  }
  // RL rotation
  {
    minilib::rbtree < int32_t > tree;
    tree.attach_counter (minilib::addressof (ctr));
    auto r = tree.emplace_root (10);
    auto n30 = tree.emplace_right (r, 30);
    tree.emplace_left (n30, 20);
    auto root = tree.root ();
    ASSERT_TRUE (*(tree.data (root)) == 20);
    ASSERT_TRUE (*(tree.data (tree.left (root))) == 10);
    ASSERT_TRUE (*(tree.data (tree.right (root))) == 30);
  }
  return true;
}

// 5. Remove
constexpr bool test_remove (minilib::counter & ctr) {
  // Remove single root
  {
    minilib::rbtree < int32_t > tree;
    tree.attach_counter (minilib::addressof (ctr));
    auto r = tree.emplace_root (42);
    tree.remove (r);
    ASSERT_TRUE (! static_cast < bool > (tree.root ()));
  }
  // Remove leaf
  {
    minilib::rbtree < int32_t > tree;
    tree.attach_counter (minilib::addressof (ctr));
    auto r = tree.emplace_root (20);
    auto l = tree.emplace_left (r, 10);
    tree.emplace_right (r, 30);
    tree.remove (l);
    ASSERT_TRUE (! static_cast < bool > (tree.left (tree.root ())));
    ASSERT_TRUE (*(tree.data (tree.right (tree.root ()))) == 30);
  }
  // Remove node with children
  {
    minilib::rbtree < int32_t > tree;
    tree.attach_counter (minilib::addressof (ctr));
    auto r = tree.emplace_root (20);
    tree.emplace_left (r, 10);
    tree.emplace_right (r, 30);
    tree.remove (r);
    auto new_root = tree.root ();
    ASSERT_TRUE (static_cast < bool > (new_root));
    ASSERT_TRUE (*(tree.data (new_root)) == 30);
    ASSERT_TRUE (*(tree.data (tree.left (new_root))) == 10);
  }
  // Remove with successor deeper in right subtree
  {
    minilib::rbtree < int32_t > tree;
    tree.attach_counter (minilib::addressof (ctr));
    auto r = tree.emplace_root (20);
    tree.emplace_left (r, 10);
    auto right_child = tree.emplace_right (r, 40);
    tree.emplace_left (right_child, 30);
    tree.emplace_right (right_child, 50);

    tree.remove (r);
    auto new_root = tree.root ();
    ASSERT_TRUE (*(tree.data (new_root)) == 30);
    ASSERT_TRUE (*(tree.data (tree.left (new_root))) == 10);
    ASSERT_TRUE (*(tree.data (tree.right (new_root))) == 40);
  }
  return true;
}

// 6. Private destructor
constexpr bool test_inform_destruct (minilib::counter & ctr) {
  minilib::rbtree < Probe > tree;
  tree.attach_counter (minilib::addressof (ctr));
  auto r = tree.emplace_root_null ();
  construct_probe (tree.data (r), 100);
  ASSERT_TRUE (tree.data (r)->val == 100);

  destroy_probe (tree.data (r));
  tree.inform_destruct (r);
  ASSERT_TRUE (! static_cast < bool > (tree.root ()));
  return true;
}

// 7. Clear
constexpr bool test_clear (minilib::counter & ctr) {
  minilib::rbtree < int32_t > tree;
  tree.attach_counter (minilib::addressof (ctr));
  tree.clear ();
  ASSERT_TRUE (! static_cast < bool > (tree.root ()));

  auto r = tree.emplace_root (20);
  tree.emplace_left (r, 10);
  tree.emplace_right (r, 30);
  tree.clear ();
  ASSERT_TRUE (! static_cast < bool > (tree.root ()));

  auto r2 = tree.emplace_root (50);
  ASSERT_TRUE (*(tree.data (r2)) == 50);
  return true;
}

// 8. Lifecycle tracking
constexpr bool test_lifecycle (minilib::counter & ctr) {
  int32_t destruct_count = 0;
  {
    minilib::rbtree < ConstexprTracker > tree;
    tree.attach_counter (minilib::addressof (ctr));
    auto r = tree.emplace_root (1, 10, minilib::addressof (destruct_count));
    auto l = tree.emplace_left (r, 2, 20, minilib::addressof (destruct_count));
    tree.emplace_right (r, 3, 30, minilib::addressof (destruct_count));

    tree.remove (l);
    ASSERT_TRUE (destruct_count == 1);
  }
  ASSERT_TRUE (destruct_count == 3);
  return true;
}

// 9. Copy constructor
constexpr bool test_copy_ctor (minilib::counter & ctr) {
  minilib::rbtree < int32_t > tree1;
  tree1.attach_counter (minilib::addressof (ctr));
  auto r = tree1.emplace_root (20);
  auto l = tree1.emplace_left (r, 10);
  tree1.emplace_right (r, 30);
  tree1.emplace_left (l, 5);

  minilib::rbtree < int32_t > tree2 (tree1);
  auto r2 = tree2.root ();
  auto r1 = tree1.root ();
  ASSERT_TRUE (*(tree2.data (r2)) == *(tree1.data (r1)));
  ASSERT_TRUE (*(tree2.data (tree2.left (r2))) == *(tree1.data (tree1.left (r1))));
  ASSERT_TRUE (*(tree2.data (tree2.right (r2))) == *(tree1.data (tree1.right (r1))));
  ASSERT_TRUE (*(tree2.data (tree2.left (tree2.left (r2)))) == *(tree1.data (tree1.left (tree1.left (r1)))));

  *(tree2.data (r2)) = 999;
  ASSERT_TRUE (*(tree1.data (tree1.root ())) != 999);
  ASSERT_TRUE (*(tree2.data (tree2.root ())) == 999);
  return true;
}

// 10. Copy assignment
constexpr bool test_copy_assignment (minilib::counter & ctr) {
  minilib::rbtree < int32_t > tree1;
  tree1.attach_counter (minilib::addressof (ctr));
  auto r = tree1.emplace_root (20);
  tree1.emplace_left (r, 10);
  tree1.emplace_right (r, 30);

  minilib::rbtree < int32_t > tree2;
  tree2.attach_counter (minilib::addressof (ctr));
  auto r2 = tree2.emplace_root (50);
  tree2.emplace_left (r2, 40);

  tree2 = tree1;
  auto new_r2 = tree2.root ();
  ASSERT_TRUE (*(tree2.data (new_r2)) == 20);
  ASSERT_TRUE (*(tree2.data (tree2.left (new_r2))) == 10);
  ASSERT_TRUE (*(tree2.data (tree2.right (new_r2))) == 30);

  tree2 = tree2;
  ASSERT_TRUE (*(tree2.data (tree2.root ())) == 20);

  minilib::rbtree < int32_t > empty_tree;
  empty_tree.attach_counter (minilib::addressof (ctr));
  tree2 = empty_tree;
  ASSERT_TRUE (! static_cast < bool > (tree2.root ()));
  return true;
}

// 11. Move constructor and assignment
constexpr bool test_move_ctor_assignment (minilib::counter & ctr) {
  minilib::rbtree < int32_t > tree1;
  tree1.attach_counter (minilib::addressof (ctr));
  auto r = tree1.emplace_root (20);
  tree1.emplace_left (r, 10);

  minilib::rbtree < int32_t > tree2 (minilib::move (tree1));
  ASSERT_TRUE (! static_cast < bool > (tree1.root ()));
  ASSERT_TRUE (static_cast < bool > (tree2.root ()));
  ASSERT_TRUE (*(tree2.data (tree2.root ())) == 20);

  minilib::rbtree < int32_t > tree3;
  tree3.attach_counter (minilib::addressof (ctr));
  tree3 = minilib::move (tree2);
  ASSERT_TRUE (! static_cast < bool > (tree2.root ()));
  ASSERT_TRUE (static_cast < bool > (tree3.root ()));
  ASSERT_TRUE (*(tree3.data (tree3.root ())) == 20);
  return true;
}

// 12. Move-only
constexpr bool test_move_only (minilib::counter & ctr) {
  minilib::rbtree < MoveOnly > tree;
  tree.attach_counter (minilib::addressof (ctr));
  auto r = tree.emplace_root (MoveOnly (10));
  auto l = tree.emplace_left (r, MoveOnly (5));
  ASSERT_TRUE (tree.data (r)->val == 10);
  ASSERT_TRUE (tree.data (l)->val == 5);

  minilib::rbtree < MoveOnly > tree_moved (minilib::move (tree));
  ASSERT_TRUE (tree_moved.data (tree_moved.root ())->val == 10);
  return true;
}

// 13. Handle semantics
constexpr bool test_handle_semantics (minilib::counter & ctr) {
  minilib::rbtree < int32_t > tree;
  tree.attach_counter (minilib::addressof (ctr));
  auto r = tree.emplace_root (42);
  minilib::rbtree < int32_t > :: handle_type h = r;

  tree.remove (r);
  ASSERT_TRUE (! static_cast < bool > (tree.root ()));
  ASSERT_TRUE (! static_cast < bool > (h));
  return true;
}

constexpr bool test_all () {
  minilib::counter ctr;
  ASSERT_TRUE (test_default_ctor (ctr));
  ASSERT_TRUE (test_emplace_root (ctr));
  ASSERT_TRUE (test_emplace_left_right (ctr));
  ASSERT_TRUE (test_insert_rotations (ctr));
  ASSERT_TRUE (test_remove (ctr));
  ASSERT_TRUE (test_inform_destruct (ctr));
  ASSERT_TRUE (test_clear (ctr));
  ASSERT_TRUE (test_lifecycle (ctr));
  ASSERT_TRUE (test_copy_ctor (ctr));
  ASSERT_TRUE (test_copy_assignment (ctr));
  ASSERT_TRUE (test_move_ctor_assignment (ctr));
  ASSERT_TRUE (test_move_only (ctr));
  ASSERT_TRUE (test_handle_semantics (ctr));
  return true;
}

constexpr bool passed = test_all ();
static_assert (passed);

void main ([[maybe_unused]] void * sp) {
  if (! passed) exit (1);
  exit (0);
}
