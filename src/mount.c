#include <syscall.h>
#include <syscall_nr.h>

int mount (const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data) {
  return syscall5 ((long) source, (long) target, (long) filesystemtype, mountflags, (long) data, __NR_mount);
}
