#include <stddef.h>
#include <syscall.h>
#include <syscall_nr.h>
#include <errno.h>
#include <epoll.h>

int epoll_create (int size) {
  if (size <= 0) return -EINVAL;
  return epoll_create1 (0);
}

int epoll_create1 (int flags) {
  return syscall1 (flags, __NR_epoll_create1);
}

int epoll_ctl (fd_t epfd, int op, fd_t fd, struct epoll_event *event) {
  return syscall4 (epfd, op, fd, (long) event, __NR_epoll_ctl);
}

int epoll_wait (fd_t epfd, struct epoll_event *events, int maxevents, int timeout) {
  return syscall5 (epfd, (long) events, maxevents, timeout, (long) NULL, __NR_epoll_pwait);
}
