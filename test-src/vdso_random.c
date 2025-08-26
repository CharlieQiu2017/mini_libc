#include <stddef.h>
#include <tls.h>
#include <io.h>
#include <exit.h>
#include <random.h>

static struct tls_struct main_tls = {
  .thread_id = 0
};

void main (void * sp) {
  uint64_t argc = *((uint64_t *) sp);
  char ** argv = (char **) (((uintptr_t) sp) + 8);
  char ** envp = argv + argc + 1;
  char ** auxv = envp;
  while (*auxv != NULL) ++auxv;
  auxv = auxv + 1;

  set_thread_pointer (&main_tls);

  find_vdso_getrandom (auxv);

  char rnd[32];
  getrandom (rnd, 32, 0);

  write (1, rnd, 32);

  exit (0);
}
