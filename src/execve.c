#include <syscall.h>
#include <syscall_nr.h>

int execve (const char * pathname, char * const argv[], char * const envp[]) {
  return syscall3 ((long) pathname, (long) argv, (long) envp, __NR_execve);
}
