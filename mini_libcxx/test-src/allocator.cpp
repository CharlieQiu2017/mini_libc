#include <stdint.h>
#include <tls.h>
#include <memory.h>
#include <exit.h>
#include <utility.hpp>

using namespace minilib;

static struct malloc_arena_t main_arena;

static struct tls_struct main_tls = {
  .thread_id = 0,
  .malloc_arena = nullptr,
  .getrandom_opaque_state = nullptr,
  .counter = 0
};

void main ([[maybe_unused]] void * sp) {
  main_tls.malloc_arena = &main_arena;
  set_thread_pointer (&main_tls);
  malloc_init ();

  uint32_t * arr = minilib::allocator < uint32_t > :: allocate (3);
  arr[0] = 1;
  arr[1] = 2;
  arr[2] = 3;
  if (arr[0] != 1 || arr[1] != 2 || arr[2] != 3) exit (1);

  exit (0);
}
