#include <syscall.h>
#include <syscall_nr.h>

__attribute__((noreturn)) void exit (int status) {
  syscall1 (status, __NR_exit);
  __builtin_unreachable ();
}
