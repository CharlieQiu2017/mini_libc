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

ssize_t pread (fd_t fd, void * buf, size_t count, long offset) {
  return syscall4 (fd, (long) buf, count, offset, __NR_pread64);
}

ssize_t pwrite (fd_t fd, const void * buf, size_t count, long offset) {
  return syscall4 (fd, (long) buf, count, offset, __NR_pwrite64);
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

long lseek (fd_t fd, long offset, int whence) {
  return syscall3 (fd, offset, whence, __NR_lseek);
}

ssize_t readv (int fd, const struct iovec * iov, int iovcnt) {
  return syscall3 (fd, (long) iov, iovcnt, __NR_readv);
}

ssize_t writev (int fd, const struct iovec * iov, int iovcnt) {
  return syscall3 (fd, (long) iov, iovcnt, __NR_writev);
}

ssize_t preadv (int fd, const struct iovec * iov, int iovcnt, long offset);
ssize_t pwritev (int fd, const struct iovec * iov, int iovcnt, long offset);
ssize_t preadv2 (int fd, const struct iovec * iov, int iovcnt, long offset, int flags);
ssize_t pwritev2 (int fd, const struct iovec * iov, int iovcnt, long offset, int flags);
