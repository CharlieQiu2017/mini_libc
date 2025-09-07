/* multiuser.h
   Adapted from Linux kernel include/linux/syscalls.h
 */

#ifndef MULTIUSER_H
#define MULTIUSER_H

#include <multiuser_types.h>

#ifdef __cplusplus
extern "C" {
#endif

int setregid (gid_t rgid, gid_t egid);
int setgid (gid_t gid);
int setreuid (uid_t ruid, uid_t euid);
int setuid (uid_t uid);
int setresuid (uid_t ruid, uid_t euid, uid_t suid);
int getresuid (uid_t * ruid, uid_t * euid, uid_t * suid);
int setresgid (gid_t rgid, gid_t egid, gid_t sgid);
int getresgid (gid_t * rgid, gid_t * egid, gid_t * sgid);
int setfsuid (uid_t uid);
int setfsgid (gid_t gid);
int setpgid (pid_t pid, pid_t pgid);
pid_t getpgid (pid_t pid);
pid_t getsid (pid_t pid);
pid_t setsid (void);
int getgroups (int gidsetsize, gid_t * grouplist);
int setgroups (int gidsetsize, gid_t * grouplist);
pid_t getpid (void);
pid_t getppid (void);
uid_t getuid (void);
uid_t geteuid (void);
gid_t getgid (void);
gid_t getegid (void);
pid_t gettid (void);

#ifdef __cplusplus
}
#endif

#endif
