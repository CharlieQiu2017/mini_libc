#include <syscall.h>
#include <syscall_nr.h>
#include <string.h>
#include <io.h>

/* sys_read
   count should be at most 0x7ffff000
 */
ssize_t read (fd_t fd, void * buf, size_t count) {
  return syscall3 (fd, (long) buf, count, __NR_read);
}

/* sys_write
   count should be at most 0x7ffff000
 */
ssize_t write (fd_t fd, const void * buf, size_t count) {
  return syscall3 (fd, (long) buf, count, __NR_write);
}

/* sys_openat
   dfd should be either an open file descriptor, or AT_FDCWD
 */
fd_t openat (fd_t dfd, const char * filename, int flags, umode_t mode) {
  return syscall4 (dfd, (long) filename, flags, mode, __NR_openat);
}

fd_t open (const char * filename, int flags, umode_t mode) {
  return openat (AT_FDCWD, filename, flags, mode);
}

int close (fd_t fd) {
  return syscall1 (fd, __NR_close);
}

fd_t dup (fd_t fd) {
  return syscall1 (fd, __NR_dup);
}

/* puts
   strlen(str) should be at most 0x7ffff000
 */
ssize_t puts (const char * str) {
  size_t len = strlen (str);
  ssize_t ret = write (1, str, len);
  return ret;
}
