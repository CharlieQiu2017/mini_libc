#include <syscall.h>
#include <syscall_nr.h>

void exit (int status) {
  syscall1 (status, __NR_exit);
}
