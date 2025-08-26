#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <syscall.h>
#include <syscall_nr.h>
#include <vdso.h>
#include <tls.h>
#include <memory.h>
#include <random.h>

typedef ssize_t (* getrandom_func) (void * buffer, size_t len, unsigned int flags, void * opaque_state, size_t opaque_len);

static getrandom_func getrandom_func_ptr = NULL;
static struct vgetrandom_opaque_params getrandom_params;
static void * getrandom_page;

void setup_vdso_getrandom (void) {
  /* Find the getrandom entrypoint */
  getrandom_func_ptr = (getrandom_func) (uintptr_t) get_vdso_getrandom_ptr ();
  if (getrandom_func_ptr == NULL) return;

  /* Get the opaque params */
  getrandom_func_ptr (0, 0, 0, &getrandom_params, ~0ull);

  /* Allocate opaque state space.
     We shall assume that one page is enough for all threads.
     TODO: Implement more robust framework for per-thread state management.
  */
  getrandom_page = mmap (NULL, 4096, getrandom_params.mmap_prot, getrandom_params.mmap_flags | MAP_ANONYMOUS, 0, 0);
  if (getrandom_page == NULL) getrandom_func_ptr = NULL;

  return;
}

ssize_t getrandom_syscall (void * buf, size_t buflen, unsigned int flags) {
  return syscall3 ((long) buf, buflen, flags, __NR_getrandom);
}

ssize_t getrandom (void * buf, size_t buflen, unsigned int flags) {
  if (getrandom_func_ptr) {
    uint16_t tid = get_thread_id ();
    return getrandom_func_ptr (buf, buflen, flags, ((char *) getrandom_page) + getrandom_params.size_of_opaque_state * tid, getrandom_params.size_of_opaque_state);
  }

  return getrandom_syscall (buf, buflen, flags);
}
