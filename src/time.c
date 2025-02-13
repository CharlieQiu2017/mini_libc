#include <syscall.h>
#include <syscall_nr.h>
#include <time.h>

int nanosleep (const struct timespec * req, struct timespec * rem) {
  return syscall2 ((long) req, (long) rem, __NR_nanosleep);
}
int clock_nanosleep (clockid_t clockid, int flags, const struct timespec * request, struct timespec * remain) {
  return syscall4 (clockid, flags, (long) request, (long) remain, __NR_clock_nanosleep);
}

int clock_gettime (clockid_t clockid, struct timespec * tp) {
  return syscall2 (clockid, (long) tp, __NR_clock_gettime);
}

int clock_settime (clockid_t clockid, const struct timespec * tp) {
  return syscall2 (clockid, (long) tp, __NR_clock_settime);
}

int clock_getres (clockid_t clockid, struct timespec * res) {
  return syscall2 (clockid, (long) res, __NR_clock_getres);
}

int adjtimex (struct timex * buf) {
  return syscall1 ((long) buf, __NR_adjtimex);
}

int clock_adjtime (clockid_t clockid, struct timex * buf) {
  return syscall2 (clockid, (long) buf, __NR_clock_adjtime);
}

int timerfd_create (clockid_t clockid, int flags) {
  return syscall2 (clockid, flags, __NR_timerfd_create);
}
int timerfd_settime (fd_t fd, int flags, const struct itimerspec * new_value, struct itimerspec * old_value) {
  return syscall4 (fd, flags, (long) new_value, (long) old_value, __NR_timerfd_settime);
}

int timerfd_gettime (fd_t fd, struct itimerspec * curr_value) {
  return syscall2 (fd, (long) curr_value, __NR_timerfd_gettime);
}
