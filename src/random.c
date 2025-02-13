#include <syscall.h>
#include <syscall_nr.h>
#include <random.h>

ssize_t getrandom (void * buf, size_t buflen, unsigned int flags) {
  return syscall3 ((long) buf, buflen, flags, __NR_getrandom);
}
