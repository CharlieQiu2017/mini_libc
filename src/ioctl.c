#include <ioctl.h>
#include <syscall.h>
#include <syscall_nr.h>

int ioctl (fd_t fd, unsigned long request, void *argp) {
  return syscall3 (fd, request, (long) argp, __NR_ioctl);
}
