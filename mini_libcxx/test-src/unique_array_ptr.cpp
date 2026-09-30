#include <tls.h>
#include <memory.h>
#include <exit.h>
#include <unique_ptr.hpp>
#include <utility.hpp>

using namespace minilib;

static struct malloc_arena_t main_arena;

static struct tls_struct main_tls = {
  .thread_id = 0,
  .malloc_arena = nullptr,
  .getrandom_opaque_state = nullptr
};

void main ([[maybe_unused]] void * sp) {
  main_tls.malloc_arena = &main_arena;
  set_thread_pointer (&main_tls);
  malloc_init ();

  unique_array_ptr < int > p;
  p.construct (3);
  if (p.size () != 3) exit (1);
  p.destruct ();
  p.construct (3, 4);
  if (p.size () != 3) exit (1);
  for (size_t i = 0; i < 3; i++) if (p[i] != 4) exit (1);
  p[1] = 5;

  unique_array_ptr < int > q (move (p));
  if (q.size () != 3) exit (1);
  if (p.size () != 0) exit (1);
  if (q[1] != 5) exit (1);

  exit (0);
}
