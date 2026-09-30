#include <stdint.h>
#include <tls.h>
#include <memory.h>
#include <exit.h>
#include <tagged_ptr.hpp>
#include <type_traits.hpp>
#include <utility.hpp>

static struct malloc_arena_t main_arena;

static struct tls_struct main_tls = {
  .thread_id = 0,
  .malloc_arena = nullptr,
  .getrandom_opaque_state = nullptr,
  .counter = 0
};

struct TestStruct {
  uint64_t a;
  uint64_t b;
};

void main ([[maybe_unused]] void * sp) {
  main_tls.malloc_arena = &main_arena;
  set_thread_pointer (&main_tls);
  malloc_init ();

  // Test 1: Basic operations, struct access via operator-> and operator*, copy/move
  {
    minilib::tagged_ptr < TestStruct > p = minilib::tagged_ptr < TestStruct > :: allocate (200);
    if (p.get_id () != 200) std::terminate ();
    if (! static_cast < bool > (p)) std::terminate ();
    minilib::construct_at < TestStruct > (p.data ());
    p->a = 10;
    p->b = 20;
    if ((*p).a != 10 || (*p).b != 20) std::terminate ();
    if (p.data ()->a != 10) std::terminate ();

    minilib::tagged_ptr < TestStruct > p_move = minilib::move (p);
    if (p_move->a != 10) std::terminate ();
    minilib::tagged_ptr < TestStruct > p_copy = p_move;
    if (p_copy->b != 20) std::terminate ();

    minilib::tagged_ptr < TestStruct > :: deallocate (p_copy);
  }

  // Test 2: counted_ref copy, move, and tombstone lifecycle
  {
    minilib::tagged_ptr < uint64_t > p = minilib::tagged_ptr < uint64_t > :: allocate (100);
    if (p.get_id () != 100) std::terminate ();
    minilib::construct_at < uint64_t > (p.data ());
    *p = 42;

    minilib::counted_ref < uint64_t > cref1;
    p.create_counted_ref (cref1);
    if (! static_cast < bool > (cref1)) std::terminate ();
    if (cref1.get_id () != 100) std::terminate ();
    if (minilib::tagged_ptr < uint64_t > :: deref_counted_ref (cref1) != 42) std::terminate ();

    // Copy construction
    minilib::counted_ref < uint64_t > cref2 = cref1;
    if (! static_cast < bool > (cref2)) std::terminate ();
    if (cref2.get_id () != 100) std::terminate ();
    if (minilib::tagged_ptr < uint64_t > :: deref_counted_ref (cref2) != 42) std::terminate ();

    // Copy assignment
    minilib::counted_ref < uint64_t > cref3;
    cref3 = cref2;
    if (! static_cast < bool > (cref3)) std::terminate ();

    // Move construction
    minilib::counted_ref < uint64_t > cref4 = minilib::move (cref3);
    if (static_cast < bool > (cref3)) std::terminate ();
    if (! static_cast < bool > (cref4)) std::terminate ();
    if (minilib::tagged_ptr < uint64_t > :: deref_counted_ref (cref4) != 42) std::terminate ();

    // Move assignment
    minilib::counted_ref < uint64_t > cref5;
    cref5 = minilib::move (cref4);
    if (static_cast < bool > (cref4)) std::terminate ();
    if (! static_cast < bool > (cref5)) std::terminate ();

    // Tombstone test: deallocate while cref1, cref2, cref5 are still alive
    minilib::tagged_ptr < uint64_t > :: deallocate (p);
    if (static_cast < bool > (cref1)) std::terminate ();
    if (static_cast < bool > (cref2)) std::terminate ();
    if (static_cast < bool > (cref5)) std::terminate ();

    cref1 = nullptr;
    if (static_cast < bool > (cref1)) std::terminate ();
    cref2 = nullptr;
    if (static_cast < bool > (cref2)) std::terminate ();
    cref5 = nullptr; // Last reference destroyed, frees memory
    if (static_cast < bool > (cref5)) std::terminate ();
  }

  // Test 3: linked_ref copy, move, individual reset, and deallocate nullification
  {
    minilib::tagged_ptr < uint64_t > p2 = minilib::tagged_ptr < uint64_t > :: allocate (300);
    minilib::construct_at < uint64_t > (p2.data ());
    *p2 = 99;

    minilib::linked_ref < uint64_t > lref1;
    p2.create_linked_ref (lref1);
    if (! static_cast < bool > (lref1)) std::terminate ();
    if (lref1.get_id () != 300) std::terminate ();
    if (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref1) != 99) std::terminate ();

    // Copy construction
    minilib::linked_ref < uint64_t > lref2 = lref1;
    minilib::linked_ref < uint64_t > lref3 = lref2;
    if (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref2) != 99) std::terminate ();
    if (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref3) != 99) std::terminate ();

    // Move construction
    minilib::linked_ref < uint64_t > lref4 = minilib::move (lref2);
    if (static_cast < bool > (lref2)) std::terminate ();
    if (! static_cast < bool > (lref4)) std::terminate ();
    if (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref4) != 99) std::terminate ();

    // Middle/head removal
    lref4 = nullptr;
    if (static_cast < bool > (lref4)) std::terminate ();
    if (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref1) != 99) std::terminate ();
    if (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref3) != 99) std::terminate ();

    lref3 = nullptr;
    if (static_cast < bool > (lref3)) std::terminate ();
    if (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref1) != 99) std::terminate ();

    // Copy assignment
    minilib::linked_ref < uint64_t > lref5;
    lref5 = lref1;
    if (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref5) != 99) std::terminate ();

    // Move assignment
    minilib::linked_ref < uint64_t > lref6;
    lref6 = minilib::move (lref5);
    if (static_cast < bool > (lref5)) std::terminate ();
    if (! static_cast < bool > (lref6)) std::terminate ();
    if (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref6) != 99) std::terminate ();

    // Deallocate: crawls list and nullifies all remaining linked refs (lref1, lref6)
    minilib::tagged_ptr < uint64_t > :: deallocate (p2);
    if (static_cast < bool > (lref1)) std::terminate ();
    if (static_cast < bool > (lref6)) std::terminate ();
  }

  // Test 4: Mode switching between counter and linked mode
  {
    minilib::tagged_ptr < uint64_t > p3 = minilib::tagged_ptr < uint64_t > :: allocate (400);
    minilib::construct_at < uint64_t > (p3.data ());
    *p3 = 77;

    // Start in counter mode
    minilib::counted_ref < uint64_t > cref;
    p3.create_counted_ref (cref);
    if (minilib::tagged_ptr < uint64_t > :: deref_counted_ref (cref) != 77) std::terminate ();
    cref = nullptr; // counter back to 0

    // Switch to linked mode
    minilib::linked_ref < uint64_t > lref;
    p3.create_linked_ref (lref);
    if (minilib::tagged_ptr < uint64_t > :: deref_linked_ref (lref) != 77) std::terminate ();
    lref = nullptr; // list back to empty

    // Switch back to counter mode
    minilib::counted_ref < uint64_t > cref2;
    p3.create_counted_ref (cref2);
    if (minilib::tagged_ptr < uint64_t > :: deref_counted_ref (cref2) != 77) std::terminate ();
    cref2 = nullptr;

    minilib::tagged_ptr < uint64_t > :: deallocate (p3);
  }

  exit (0);
}
