#include <stdint.h>
#include <exit.h>
#include <tagged_ptr.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

#define ASSERT_TRUE(cond) do { if (!(cond)) return false; } while (0)

struct TestStruct {
  uint64_t a;
  uint64_t b;
};

constexpr bool test_tagged_ptr () {
  // Test 1: Basic operations, struct access via operator-> and operator*, copy/move
  {
    minilib::tagged_ptr < TestStruct > p = minilib::tagged_ptr < TestStruct > :: allocate (200);
    ASSERT_TRUE (p.get_id () == 200);
    ASSERT_TRUE (static_cast < bool > (p));
    minilib::construct_at < TestStruct > (p.data ());
    p->a = 10;
    p->b = 20;
    ASSERT_TRUE ((*p).a == 10);
    ASSERT_TRUE ((*p).b == 20);
    ASSERT_TRUE (p.data ()->a == 10);

    minilib::tagged_ptr < TestStruct > p_move = minilib::move (p);
    ASSERT_TRUE (p_move->a == 10);
    minilib::tagged_ptr < TestStruct > p_copy = p_move;
    ASSERT_TRUE (p_copy->b == 20);

    minilib::tagged_ptr < TestStruct > :: deallocate (p_copy);
  }

  // Test 2: counted_ref copy, move, and tombstone lifecycle
  {
    minilib::tagged_ptr < uint64_t > p = minilib::tagged_ptr < uint64_t > :: allocate (100);
    ASSERT_TRUE (p.get_id () == 100);
    minilib::construct_at < uint64_t > (p.data ());
    *p = 42;

    minilib::counted_ref < uint64_t > cref1;
    p.create_counted_ref (cref1);
    ASSERT_TRUE (static_cast < bool > (cref1));
    ASSERT_TRUE (cref1.get_id () == 100);
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_counted_ref (cref1) == 42);

    // Copy construction
    minilib::counted_ref < uint64_t > cref2 = cref1;
    ASSERT_TRUE (static_cast < bool > (cref2));
    ASSERT_TRUE (cref2.get_id () == 100);
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_counted_ref (cref2) == 42);

    // Copy assignment
    minilib::counted_ref < uint64_t > cref3;
    cref3 = cref2;
    ASSERT_TRUE (static_cast < bool > (cref3));

    // Move construction
    minilib::counted_ref < uint64_t > cref4 = minilib::move (cref3);
    ASSERT_TRUE (! static_cast < bool > (cref3));
    ASSERT_TRUE (static_cast < bool > (cref4));
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_counted_ref (cref4) == 42);

    // Move assignment
    minilib::counted_ref < uint64_t > cref5;
    cref5 = minilib::move (cref4);
    ASSERT_TRUE (! static_cast < bool > (cref4));
    ASSERT_TRUE (static_cast < bool > (cref5));

    // Tombstone test: deallocate while cref1, cref2, cref5 are still alive
    minilib::tagged_ptr < uint64_t > :: deallocate (p);
    ASSERT_TRUE (! static_cast < bool > (cref1));
    ASSERT_TRUE (! static_cast < bool > (cref2));
    ASSERT_TRUE (! static_cast < bool > (cref5));

    cref1 = nullptr;
    ASSERT_TRUE (! static_cast < bool > (cref1));
    cref2 = nullptr;
    ASSERT_TRUE (! static_cast < bool > (cref2));
    cref5 = nullptr; // Last reference destroyed, frees memory
    ASSERT_TRUE (! static_cast < bool > (cref5));
  }

  // Test 3: linked_ref copy, move, individual reset, and deallocate nullification
  {
    minilib::tagged_ptr < uint64_t > p2 = minilib::tagged_ptr < uint64_t > :: allocate (300);
    minilib::construct_at < uint64_t > (p2.data ());
    *p2 = 99;

    minilib::linked_ref < uint64_t > lref1;
    p2.create_linked_ref (lref1);
    ASSERT_TRUE (static_cast < bool > (lref1));
    ASSERT_TRUE (lref1.get_id () == 300);
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref1) == 99);

    // Copy construction
    minilib::linked_ref < uint64_t > lref2 = lref1;
    minilib::linked_ref < uint64_t > lref3 = lref2;
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref2) == 99);
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref3) == 99);

    // Move construction
    minilib::linked_ref < uint64_t > lref4 = minilib::move (lref2);
    ASSERT_TRUE (! static_cast < bool > (lref2));
    ASSERT_TRUE (static_cast < bool > (lref4));
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref4) == 99);

    // Middle/head removal
    lref4 = nullptr;
    ASSERT_TRUE (! static_cast < bool > (lref4));
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref1) == 99);
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref3) == 99);

    lref3 = nullptr;
    ASSERT_TRUE (! static_cast < bool > (lref3));
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref1) == 99);

    // Copy assignment
    minilib::linked_ref < uint64_t > lref5;
    lref5 = lref1;
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref5) == 99);

    // Move assignment
    minilib::linked_ref < uint64_t > lref6;
    lref6 = minilib::move (lref5);
    ASSERT_TRUE (! static_cast < bool > (lref5));
    ASSERT_TRUE (static_cast < bool > (lref6));
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref6) == 99);

    // Deallocate: crawls list and nullifies all remaining linked refs (lref1, lref6)
    minilib::tagged_ptr < uint64_t > :: deallocate (p2);
    ASSERT_TRUE (! static_cast < bool > (lref1));
    ASSERT_TRUE (! static_cast < bool > (lref6));
  }

  // Test 4: Mode switching between counter and linked mode
  {
    minilib::tagged_ptr < uint64_t > p3 = minilib::tagged_ptr < uint64_t > :: allocate (400);
    minilib::construct_at < uint64_t > (p3.data ());
    *p3 = 77;

    // Start in counter mode
    minilib::counted_ref < uint64_t > cref;
    p3.create_counted_ref (cref);
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_counted_ref (cref) == 77);
    cref = nullptr; // counter back to 0

    // Switch to linked mode
    minilib::linked_ref < uint64_t > lref;
    p3.create_linked_ref (lref);
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref) == 77);
    lref = nullptr; // list back to empty

    // Switch back to counter mode
    minilib::counted_ref < uint64_t > cref2;
    p3.create_counted_ref (cref2);
    ASSERT_TRUE (minilib::tagged_ptr < uint64_t > :: deref_counted_ref (cref2) == 77);
    cref2 = nullptr;

    minilib::tagged_ptr < uint64_t > :: deallocate (p3);
  }

  return true;
}

constexpr bool all_passed = test_tagged_ptr ();

void main ([[maybe_unused]] void * sp) {
  if (! all_passed) exit (1);
  exit (0);
}
