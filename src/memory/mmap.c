#include <stdint.h>
#include <memory.h>
#include <io.h>
#include <syscall.h>
#include <syscall_nr.h>

void * mmap (void * addr, size_t len, int prot, int flags, fd_t fd, ssize_t offset) {
  return (void *) syscall6 ((long) addr, len, prot, flags, fd, offset, __NR_mmap);
}

int munmap (void * addr, size_t len) {
  return syscall2 ((long) addr, len, __NR_munmap);
}

void * mmap_alloc (size_t len, void ** ctx_ptr) {
  len = (((len - 1) >> 12) + 1) << 12;
  void * ptr = mmap (NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  if (((intptr_t) ptr) < 0) return NULL; else { *ctx_ptr = ptr; return ptr; }
}

void mmap_free (__attribute__((unused)) void * ptr, void * ctx, size_t len) {
  len = (((len - 1) >> 12) + 1) << 12;
  munmap (ctx, len);
}
