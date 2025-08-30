#include <syscall.h>
#include <syscall_nr.h>

__attribute__((noreturn)) void exit (int status) {
  syscall1 (status, __NR_exit);
  __builtin_unreachable ();
}

__attribute__((noreturn)) void exit_group (int status) {
  syscall1 (status, __NR_exit_group);
  __builtin_unreachable ();
}

__attribute__((noreturn)) void abort (void) {
  exit_group (1);
}
