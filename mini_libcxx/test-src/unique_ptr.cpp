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

  unique_ptr < int > p;
  p.construct ();
  if (!p) exit (1);
  p.destruct ();
  if (p) exit (1);
  p.construct (3);
  if (*p != 3) exit (1);
  *p = 5;
  if (*p != 5) exit (1);

  unique_ptr < int > q (move (p));
  if (p) exit (1);
  if (*q != 5) exit (1);
  p = move (q);
  if (q) exit (1);
  if (*p != 5) exit (1);

  exit (0);
}
