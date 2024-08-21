#ifndef IO_H
#define IO_H

#include <stdint.h>

typedef int fd_t;
typedef unsigned short umode_t;

ssize_t read (fd_t fd, void *buf, size_t count);

ssize_t write (fd_t fd, const void *buf, size_t count);

#define O_RDONLY 00000000
#define O_WRONLY 00000001
#define O_RDWR 00000002

#define AT_FDCWD -100

fd_t openat (fd_t dfd, const char *filename, int flags, umode_t mode);

fd_t open (const char *filename, int flags, umode_t mode);

int close (fd_t fd);

fd_t dup (fd_t fd);

ssize_t puts (const char *str);

#endif
