#include <syscall.h>
#include <syscall_nr.h>
#include <net.h>

fd_t socket (int domain, int type, int protocol) {
  return syscall3 (domain, type, protocol, __NR_socket);
}

int bind (fd_t fd, const struct sockaddr * addr, int addrlen) {
  return syscall3 (fd, (long) addr, addrlen, __NR_bind);
}

int connect (fd_t fd, const struct sockaddr * addr, int addrlen) {
  return syscall3 (fd, (long) addr, addrlen, __NR_connect);
}

int accept (fd_t fd, struct sockaddr * addr, int * addrlen) {
  return syscall3 (fd, (long) addr, (long) addrlen, __NR_accept);
}

int accept4 (fd_t fd, struct sockaddr * addr, int * addrlen, int flags) {
  return syscall4 (fd, (long) addr, (long) addrlen, flags, __NR_accept4);
}

int shutdown (fd_t fd, int how) {
  return syscall2 (fd, how, __NR_shutdown);
}

ssize_t send (fd_t fd, const void * buff, size_t len, unsigned int flags) {
  return syscall6 (fd, (long) buff, len, flags, (long) NULL, 0, __NR_sendto);
}

ssize_t sendto (fd_t fd, const void * buff, size_t len, unsigned int flags, const struct sockaddr * addr, int addr_len) {
  return syscall6 (fd, (long) buff, len, flags, (long) addr, addr_len, __NR_sendto);
}

ssize_t recv (fd_t fd, void * ubuf, size_t size, unsigned int flags) {
  return syscall6 (fd, (long) ubuf, size, flags, (long) NULL, 0, __NR_recvfrom);
}

ssize_t recvfrom (fd_t fd, void * ubuf, size_t size, unsigned int flags, struct sockaddr * addr, int * addr_len) {
  return syscall6 (fd, (long) ubuf, size, flags, (long) addr, (long) addr_len, __NR_recvfrom);
}

int getsockopt (fd_t fd, int level, int optname, void * optval, int * optlen) {
  return syscall5 (fd, level, optname, (long) optval, (long) optlen, __NR_getsockopt);
}

int setsockopt (fd_t fd, int level, int optname, const void * optval, int optlen) {
  return syscall5 (fd, level, optname, (long) optval, optlen, __NR_setsockopt);
}
