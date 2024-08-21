#include <syscall.h>
#include <syscall_nr.h>
#include <multiuser.h>

pid_t setsid (void) {
  return syscall0 (__NR_setsid);
}
