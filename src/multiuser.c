#include <syscall.h>
#include <syscall_nr.h>
#include <multiuser.h>

int setregid (gid_t rgid, gid_t egid) {
  return syscall2 (rgid, egid, __NR_setregid);
}

int setgid (gid_t gid) {
  return syscall1 (gid, __NR_setgid);
}

int setreuid (uid_t ruid, uid_t euid) {
  return syscall2 (ruid, euid, __NR_setreuid);
}

int setuid (uid_t uid) {
  return syscall1 (uid, __NR_setuid);
}

int setresuid (uid_t ruid, uid_t euid, uid_t suid) {
  return syscall3 (ruid, euid, suid, __NR_setresuid);
}

int getresuid (uid_t * ruid, uid_t * euid, uid_t * suid) {
  return syscall3 ((long) ruid, (long) euid, (long) suid, __NR_getresuid);
}

int setresgid (gid_t rgid, gid_t egid, gid_t sgid) {
  return syscall3 (rgid, egid, sgid, __NR_setresgid);
}

int getresgid (gid_t * rgid, gid_t * egid, gid_t * sgid) {
  return syscall3 ((long) rgid, (long) egid, (long) sgid, __NR_getresgid);
}

int setfsuid (uid_t uid) {
  return syscall1 (uid, __NR_setfsuid);
}

int setfsgid (gid_t gid) {
  return syscall1 (gid, __NR_setfsgid);
}

int setpgid (pid_t pid, pid_t pgid) {
  return syscall2 (pid, pgid, __NR_setpgid);
}

pid_t getpgid (pid_t pid) {
  return syscall1 (pid, __NR_getpgid);
}

pid_t getsid (pid_t pid) {
  return syscall1 (pid, __NR_getsid);
}

pid_t setsid (void) {
  return syscall0 (__NR_setsid);
}

int getgroups (int gidsetsize, gid_t * grouplist) {
  return syscall2 (gidsetsize, (long) grouplist, __NR_getgroups);
}

int setgroups (int gidsetsize, gid_t * grouplist) {
  return syscall2 (gidsetsize, (long) grouplist, __NR_setgroups);
}

pid_t getpid (void) {
  return syscall0 (__NR_getpid);
}

pid_t getppid (void) {
  return syscall0 (__NR_getppid);
}

uid_t getuid (void) {
  return syscall0 (__NR_getuid);
}

uid_t geteuid (void) {
  return syscall0 (__NR_geteuid);
}

gid_t getgid (void) {
  return syscall0 (__NR_getgid);
}

gid_t getegid (void) {
  return syscall0 (__NR_getegid);
}

pid_t gettid (void) {
  return syscall0 (__NR_gettid);
}
