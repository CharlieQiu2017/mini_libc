/* syscall.h
   Derived from musl-libc arch/aarch64/syscall_arch.h
 */

#ifndef SYSCALL_H
#define SYSCALL_H

static inline long syscall0 (long number) {
  register long _ret __asm__ ("x0");
  register long _num __asm__ ("x8") = number;

  __asm__ volatile (
    "svc 0\n"
  : "=r"(_ret)
  : "r" (_num)
  : "memory", "cc"
  );

  return _ret;
}

static inline long syscall1 (long arg1, long number) {
  register long _arg1 __asm__ ("x0") = arg1;
  register long _num __asm__ ("x8") = number;

  __asm__ volatile (
    "svc 0\n"
  : "=r"(_arg1)
  : "0"(_arg1), "r"(_num)
  : "memory", "cc"
  );

  return _arg1;
}

static inline long syscall2 (long arg1, long arg2, long number) {
  register long _arg1 __asm__ ("x0") = arg1;
  register long _arg2 __asm__ ("x1") = arg2;
  register long _num __asm__ ("x8") = number;

  __asm__ volatile (
    "svc 0\n"
  : "=r"(_arg1)
  : "0"(_arg1), "r"(_arg2), "r"(_num)
  : "memory", "cc"
  );

  return _arg1;
}

static inline long syscall3 (long arg1, long arg2, long arg3, long number) {
  register long _arg1 __asm__ ("x0") = arg1;
  register long _arg2 __asm__ ("x1") = arg2;
  register long _arg3 __asm__ ("x2") = arg3;
  register long _num __asm__ ("x8") = number;

  __asm__ volatile (
    "svc 0\n"
  : "=r"(_arg1)
  : "0"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_num)
  : "memory", "cc"
  );

  return _arg1;
}

static inline long syscall4 (long arg1, long arg2, long arg3, long arg4, long number) {
  register long _arg1 __asm__ ("x0") = arg1;
  register long _arg2 __asm__ ("x1") = arg2;
  register long _arg3 __asm__ ("x2") = arg3;
  register long _arg4 __asm__ ("x3") = arg4;
  register long _num __asm__ ("x8") = number;

  __asm__ volatile (
    "svc 0\n"
  : "=r"(_arg1)
  : "0"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_num)
  : "memory", "cc"
  );

  return _arg1;
}

static inline long syscall5 (long arg1, long arg2, long arg3, long arg4, long arg5, long number) {
  register long _arg1 __asm__ ("x0") = arg1;
  register long _arg2 __asm__ ("x1") = arg2;
  register long _arg3 __asm__ ("x2") = arg3;
  register long _arg4 __asm__ ("x3") = arg4;
  register long _arg5 __asm__ ("x4") = arg5;
  register long _num __asm__ ("x8") = number;

  __asm__ volatile (
    "svc 0\n"
  : "=r"(_arg1)
  : "0"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), "r"(_num)
  : "memory", "cc"
  );

  return _arg1;
}

static inline long syscall6 (long arg1, long arg2, long arg3, long arg4, long arg5, long arg6, long number) {
  register long _arg1 __asm__ ("x0") = arg1;
  register long _arg2 __asm__ ("x1") = arg2;
  register long _arg3 __asm__ ("x2") = arg3;
  register long _arg4 __asm__ ("x3") = arg4;
  register long _arg5 __asm__ ("x4") = arg5;
  register long _arg6 __asm__ ("x5") = arg6;
  register long _num __asm__ ("x8") = number;

  __asm__ volatile (
    "svc 0\n"
  : "=r"(_arg1)
  : "0"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), "r"(_arg6), "r"(_num)
  : "memory", "cc"
  );

  return _arg1;
}

#endif
