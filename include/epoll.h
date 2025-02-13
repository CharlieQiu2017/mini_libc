/* epoll.h
   Adapted from Linux kernel include/uapi/linux/eventpoll.h
 */

#ifndef EPOLL_H
#define EPOLL_H

#include <stdint.h>
#include <io.h>
#include <ioctl.h>

/* Flag for epoll_create1 */

#define EPOLL_CLOEXEC O_CLOEXEC

/* Opcodes for epoll_ctl */

#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3

/* Event masks */

#define EPOLLIN		0x00000001
#define EPOLLPRI	0x00000002
#define EPOLLOUT	0x00000004
#define EPOLLERR	0x00000008
#define EPOLLHUP	0x00000010
#define EPOLLRDHUP	0x00002000
#define EPOLLEXCLUSIVE	(1U << 28)
#define EPOLLWAKEUP	(1U << 29)
#define EPOLLONESHOT	(1U << 30)
#define EPOLLET		(1U << 31)

/* Seems undocumented */

#define EPOLLNVAL	0x00000020
#define EPOLLRDNORM	0x00000040
#define EPOLLRDBAND	0x00000080
#define EPOLLWRNORM	0x00000100
#define EPOLLWRBAND	0x00000200
#define EPOLLMSG	0x00000400

typedef union epoll_data {
  void * ptr;
  fd_t fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event {
  uint32_t events;
  epoll_data_t data;
};

struct epoll_params {
  uint32_t busy_poll_usecs;
  uint16_t busy_poll_budget;
  uint8_t prefer_busy_poll;
  uint8_t __pad;
};

/* ioctl for epoll */

#define EPOLL_IOC_TYPE 0x8A
#define EPIOCSPARAMS _IOW(EPOLL_IOC_TYPE, 0x01, struct epoll_params)
#define EPIOCGPARAMS _IOR(EPOLL_IOC_TYPE, 0x02, struct epoll_params)

int epoll_create (int size);
int epoll_create1 (int flags);
int epoll_ctl (fd_t epfd, int op, fd_t fd, struct epoll_event * event);
int epoll_wait (fd_t epfd, struct epoll_event * events, int maxevents, int timeout);

#endif
