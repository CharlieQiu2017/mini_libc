/* io.h
   Adapted from Linux kernel include/uapi/asm-generic/fcntl.h
   Adapted from Linux kernel arch/arm64/include/uapi/asm/fcntl.h
   Adapted from Linux kernel include/uapi/linux/stat.h
   Adapted from Linux kernel include/uapi/linux/fs.h
 */

#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int fd_t;
typedef unsigned short umode_t;

ssize_t read (fd_t fd, void * buf, size_t count);
ssize_t write (fd_t fd, const void * buf, size_t count);
ssize_t pread (fd_t fd, void * buf, size_t count, long offset);
ssize_t pwrite (fd_t fd, const void * buf, size_t count, long offset);

/* The difference between O_SYNC and O_DSYNC is that
   O_DSYNC does not wait for flushing metadata that is not
   necessary for reading the just-written data.
 */

#define O_RDONLY 00000000
#define O_WRONLY 00000001
#define O_RDWR 00000002
#define O_CREAT 00000100
#define O_EXCL 00000200
#define O_NOCTTY 00000400
#define O_TRUNC 00001000
#define O_APPEND 00002000
#define O_NONBLOCK 00004000
#define O_NDELAY O_NONBLOCK
#define O_DSYNC 00010000
#define O_DIRECTORY 00040000
#define O_NOFOLLOW 00100000
#define O_DIRECT 00200000
#define O_NOATIME 01000000
#define O_CLOEXEC 02000000
#define O_SYNC 04010000
#define O_PATH 010000000
#define O_TMPFILE 020040000

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100
#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010
#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001
#define S_ISUID 0004000
#define S_ISGID 0002000
#define S_ISVTX 0001000

#define AT_FDCWD -100

#define F_DUPFD  0
#define F_GETFD  1
#define F_SETFD  2
#define F_GETFL  3
#define F_SETFL  4
#define F_GETLK  5
#define F_SETLK  6
#define F_SETLKW 7

fd_t openat (fd_t dfd, const char * filename, int flags, umode_t mode);
fd_t open (const char * filename, int flags, umode_t mode);
int close (fd_t fd);
fd_t dup (fd_t fd);
int fcntl (fd_t fd, int cmd, unsigned long arg);
ssize_t puts (const char * str);
long lseek (fd_t fd, long offset, int whence);

struct iovec {
  void *iov_base;
  size_t iov_len;
};

#define RWF_HIPRI 0x00000001
#define RWF_DSYNC 0x00000002
#define RWF_SYNC 0x00000004
#define RWF_NOWAIT 0x00000008
#define RWF_APPEND 0x00000010
#define RWF_NOAPPEND 0x00000020
/* RWF_ATOMIC: https://docs.kernel.org/next/filesystems/ext4/atomic_writes.html */
#define RWF_ATOMIC 0x00000040
#define RWF_DONTCACHE 0x00000080

ssize_t readv (int fd, const struct iovec * iov, int iovcnt);
ssize_t writev (int fd, const struct iovec * iov, int iovcnt);
ssize_t preadv (int fd, const struct iovec * iov, int iovcnt, long offset);
ssize_t pwritev (int fd, const struct iovec * iov, int iovcnt, long offset);
ssize_t preadv2 (int fd, const struct iovec * iov, int iovcnt, long offset, int flags);
ssize_t pwritev2 (int fd, const struct iovec * iov, int iovcnt, long offset, int flags);

#ifdef __cplusplus
}
#endif

#endif
