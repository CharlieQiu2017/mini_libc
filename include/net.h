/* net.h
   Adapted from Linux kernel include/linux/net.h
   Adapted from Linux kernel include/linux/socket.h
   Adapted from Linux kernel include/uapi/linux/in.h
   Adapted from Linux kernel include/uapi/linux/un.h
 */

#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <io.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AF_UNIX 1
#define AF_INET 2

#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOCK_RAW 3

#define SOCK_NONBLOCK O_NONBLOCK
#define SOCK_CLOEXEC O_CLOEXEC

#define IPPROTO_IP 0
#define IPPROTO_ICMP 1
#define IPPROTO_IGMP 2
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define IPPROTO_RAW 255

/* Notice these are in network-order (big-endian) */

#define INADDR_ANY ((unsigned long int) 0x00000000)
#define INADDR_BROADCAST ((unsigned long int) 0xffffffff)
#define INADDR_NONE ((unsigned long int) 0xffffffff)
#define INADDR_DUMMY ((unsigned long int) 0xc0000008)
#define INADDR_LOOPBACK 0x7f000001

#define MSG_OOB		1
#define MSG_PEEK	2
#define MSG_DONTROUTE	4
#define MSG_CTRUNC	8
#define MSG_PROBE	0x10
#define MSG_TRUNC	0x20
#define MSG_DONTWAIT	0x40
#define MSG_EOR         0x80
#define MSG_WAITALL	0x100
#define MSG_CONFIRM	0x800
#define MSG_ERRQUEUE	0x2000
#define MSG_NOSIGNAL	0x4000
#define MSG_MORE	0x8000
#define MSG_WAITFORONE	0x10000
#define MSG_SOCK_DEVMEM 0x2000000
#define MSG_ZEROCOPY	0x4000000
#define MSG_FASTOPEN	0x20000000

#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2

#define SOL_SOCKET 1

typedef unsigned short sa_family_t;

struct sockaddr {
  sa_family_t sa_family;
  /* C++ forbids variable-length array. Make it happy. */
  /* char sa_data[]; */
};

struct sockaddr_un {
  sa_family_t sun_family; /* AF_UNIX */
  char sun_path[108]; /* pathname */
};

struct in_addr {
  /* IP, byte with smallest address corresponds to left most component of IP in dot-decimal notation.
     Hence, on little-endian machines the left most component is placed in the least-significant byte.
   */
  unsigned int s_addr;
};

struct sockaddr_in {
  sa_family_t sin_family; /* AF_INET */
  unsigned short sin_port; /* Port number (big endian) */
  struct in_addr sin_addr; /* IP */

  char padding[8];
};

fd_t socket (int domain, int type, int protocol);
int bind (fd_t fd, const struct sockaddr * addr, int addrlen);
int connect (fd_t fd, const struct sockaddr * addr, int addrlen);
int accept (fd_t fd, struct sockaddr * addr, int * addrlen);
int accept4 (fd_t fd, struct sockaddr * addr, int * addrlen, int flags);
int shutdown (fd_t fd, int how);
ssize_t send (fd_t fd, const void * buff, size_t len, unsigned int flags);
ssize_t sendto (fd_t fd, const void * buff, size_t len, unsigned int flags, const struct sockaddr * addr, int addr_len);
ssize_t recv (fd_t fd, void * ubuf, size_t size, unsigned int flags);
ssize_t recvfrom (fd_t fd, void * ubuf, size_t size, unsigned int flags, struct sockaddr * addr, int * addr_len);
int getsockopt (fd_t fd, int level, int optname, void * optval, int * optlen);
int setsockopt (fd_t fd, int level, int optname, const void * optval, int optlen);

static inline uint16_t htons (uint16_t hostshort) { return __builtin_bswap16 (hostshort); }
static inline uint16_t ntohs (uint16_t netshort) { return __builtin_bswap16 (netshort); }
static inline uint32_t htonl (uint32_t hostlong) { return __builtin_bswap32 (hostlong); }
static inline uint32_t ntohl (uint32_t netlong) { return __builtin_bswap32 (netlong); }

#ifdef __cplusplus
}
#endif

#endif
