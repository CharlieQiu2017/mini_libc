#include <stdint.h>
#include <memory.h>
#include <io.h>
#include <syscall.h>
#include <syscall_nr.h>

void * mmap (void *addr, size_t len, int prot, int flags, fd_t fd, ssize_t offset) {
  return (void *) syscall6 ((long) addr, len, prot, flags, fd, offset, __NR_mmap);
}

int munmap (void *addr, size_t len) {
  return syscall2 ((long) addr, len, __NR_munmap);
}

/* get_pages
   Request empty pages from the OS.
   Each page is 4096 bytes.
   num should be less than (1 << 36).
   For best performance, try to get pages in large chunks.
   Return NULL upon failure.
 */

void * get_pages (uint64_t num) {
  void * ptr = mmap (NULL, num << 12, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  if (((intptr_t) ptr) < 0) return NULL; else return ptr;
}

/* return_pages
   ptr should be exactly as returned by get_pages and not NULL.
   num should be exactly as used for get_pages.
 */

void free_pages (void *ptr, uint64_t num) {
  munmap (ptr, num << 12);
}
