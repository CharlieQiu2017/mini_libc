/* net.h
   Adapted from Linux kernel include/uapi/asm-generic/posix_types.h
   Adapted from Linux kernel include/linux/net.h
   Adapted from Linux kernel include/linux/socket.h
   Adapted from Linux kernel include/uapi/linux/socket.h
   Adapted from Linux kernel include/uapi/linux/in.h
   Adapted from Linux kernel include/uapi/linux/un.h
   Adapted from Linux kernel include/uapi/linux/if_ether.h
   Adapted from Linux kernel include/uapi/linux/tcp.h
   Adapted from Linux kernel include/uapi/linux/udp.h
   Adapted from Linux kernel include/uapi/linux/net_tstamp.h
   Adapted from Linux kernel include/uapi/linux/errqueue.h
   Adapted from Linux man pages
 */

/* The most important types of sockets in Linux are:
   TCP socket,
   UDP socket,
   Unix stream socket,
   Unix datagram socket,
   Unix seqpacket socket.

   For the difference between Unix datagram and seqpacket sockets,
   see https://stackoverflow.com/questions/10104082/unix-socket-sock-seqpacket-vs-sock-dgram.
   For seqpacket sockets, a record can be sent using one or more output operations and received using one or more input operations,
   but a single operation never transfers parts of more than one record.
   Record boundaries are visible to the receiver via the MSG_EOR flag in the received message flags returned by the recvmsg() function.

   In general we only consider the socket types above.

   There are also some minor types of sockets:
   Packet socket (necessary for implementing DHCP client due to a final confirmation step),
   Netlink socket (necessary for network device configuration),
   XDP (Express Data Path).
 */

#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <multiuser_types.h>
#include <io.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AF_UNIX 1
#define AF_INET 2
#define AF_PACKET 17

#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOCK_RAW 3

#define SOCK_NONBLOCK O_NONBLOCK
#define SOCK_CLOEXEC O_CLOEXEC

/* Protocols of the IPv4 family */

#define IPPROTO_IP 0
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define IPPROTO_RAW 255

/* Special IP addresses */
/* Since AArch64 is little-endian, the left-most component of IP is placed in the last byte.
   Use htons() to correct it.
 */

#define INADDR_ANY ((unsigned long int) 0x00000000)
#define INADDR_BROADCAST ((unsigned long int) 0xffffffff)
#define INADDR_NONE ((unsigned long int) 0xffffffff)
#define INADDR_DUMMY ((unsigned long int) 0xc0000008)
#define INADDR_LOOPBACK 0x7f000001

/* Ethernet protocols */

#define ETH_P_ARP 0x0806

/* Flags of send, recv, recvmsg */

/* MSG_OOB: When set in recv() on TCP socket: This flag requests receipt of out-of-band data that would not be received in the normal data stream.
   When set in send() on TCP socket: the last byte of the data is considered the "urgent byte".
   When set in returned messages: indicate that expedited or out-of-band data was received.
 */
/* TCP has an "urgent pointer" mechanism that has been traditionally interpreted as an out-of-band mechanism.
   This interpretation is incorrect, but for interoperability reasons everyone chose to keep the incorrect behavior.
   Due to ambiguities in the semantics of the urgent pointer,
   as well as inconsistencies in the handling of it by various OSes,
   this mechanism is now deprecated.
   See https://datatracker.ietf.org/doc/html/rfc6093.

   Each TCP packet may contain an "urgent flag" and an "urgent pointer".
   If the urgent flag is set, then the urgent pointer contains the sequence number of the byte after the "urgent byte".
   (This behavior is incorrect per RFC 1122, but that is how every implementation went,
   and so RFC 6093 updates RFC 1122 by specifying the de facto interpretation is correct.)
   Normally, the urgent pointer should point to a byte that is after the first byte contained in this packet.
   If it points to a byte that the kernel has already received, the kernel ignores the urgent pointer.
   (See net/ipv4/tcp_input.c, tcp_check_urg() function).
   If the urgent pointer points to a byte that is beyond the last byte of the packet,
   the kernel records the urgent pointer, but the urgent buffer is set to TCP_URG_NOTYET.
   When the packet containing the sequence number of the urgent pointer minus one is received,
   the urgent buffer gets filled with the byte before the urgent pointer.
   If a new urgent pointer (beyond the current urgent pointer)
   is received before the current urgent byte is read (or even received),
   the current urgent pointer is discarded, and the byte it points to is treated like normal data.

   When a new urgent pointer (beyond the current urgent pointers) is received,
   the kernel sends a signal SIGURG to the user application.
   Normally this signal is simply ignored (see signal(7)).
   When an urgent byte becomes available for read, select() and poll() will report POLLPRI on the socket,
   and epoll() will report EPOLLPRI.

   If an urgent byte is available for read, and there are still unread normal bytes before that byte,
   recv() will read bytes up to the byte just before the urgent byte.
   If the user application initiates recv() again, it returns data in the stream that are after the urgent byte.
   The only way to receive the urgent byte is by calling recv() with the MSG_OOB flag.
   This can be done even before receiving the data before the urgent byte.
   Hence it is called an "out-of-band" mechanism (at least according to the BSD interpretation).

   If the user application is going to initiate recv() that will read past the urgent byte
   (i.e. there is no remaining normal data before that byte),
   but the urgent pointer is updated before the user application calls recv(), then:
   1. If there is some normal data between the two urgent pointers, then the previous urgent byte will be read like normal data.
   2. If the two pointers are consecutive, the kernel has some special handling so that the previous urgent byte is lost.
   (See net/ipv4/tcp_input.c, tcp_check_urg() function. The comments state this behavior is wrong.)

   Like normal data, the urgent byte can only be read once, unless MSG_PEEK is used.

   If the urgent pointer is updated when the current urgent byte is not read yet,
   and the user application has already read past the current urgent byte,
   the current urgent byte is lost.

   There is also a socket option SO_OOBINLINE that requests the kernel to treat the urgent byte like normal data.
 */
/* Out-of-band data is also supported for Unix sockets since 2021, but mostly undocumented.
   It is said that Oracle products need this feature.
   If a Unix datagram is sent with MSG_OOB, the last byte of it is considered "out-of-band".
   See commit 314001f ("af_unix: Add OOB support"),
   and kernel config CONFIG_AF_UNIX_OOB (https://cateee.net/lkddb/web-lkddb/AF_UNIX_OOB.html).
   See also https://googleprojectzero.blogspot.com/2025/08/from-chrome-renderer-code-exec-to-kernel.html,
   https://lore.kernel.org/netdev/bef45d8e-35b7-42e4-bf6c-768da5b6d8f2@oracle.com/.
 */

#define MSG_OOB 1

/* MSG_PEEK: This flag causes the receive operation to return data from the beginning of the receive queue without removing that data from the queue. */
#define MSG_PEEK 2
/* MSG_DONTROUTE: UDP. Don't use a gateway to send out the packet, send to hosts only on directly connected networks.
   Don't know what effect it has on TCP.
   https://stackoverflow.com/questions/14471509/neither-socket-option-so-dontroute-nor-send-flags-msg-dontroute-works
 */
#define MSG_DONTROUTE 4
/* MSG_CTRUNC: Indicates that some control data was discarded due to lack of space in the buffer for ancillary data. */
#define MSG_CTRUNC 8
/* MSG_PROBE: Do not send. Only probe path f.e. for MTU. Seems undocumented. Perhaps UDP only.
 */
#define MSG_PROBE 0x10
/* MSG_TRUNC: For recv() only.
   For TCP sockets, causes a specified number of bytes to be discarded rather than returned in user-provided buffer. See tcp(7).
   If MSG_OOB is also set, the bytes before the urgent byte are discarded.
   For UDP and Unix datagram sockets, return the actual length of of the packet, even when it is longer than the passed buffer.
   See also the ioctl command FIONREAD.
   When set in a returned message: indicates that the trailing portion of a datagram was discarded
   because the datagram was larger than the buffer supplied.
 */
#define MSG_TRUNC 0x20
/* MSG_DONTWAIT: The operation should not block. It returns EAGAIN when it would block. */
#define MSG_DONTWAIT 0x40
/* MSG_EOR: When set in send() on SOCK_SEQPACKET sockets: terminates a record.
   When set in received msg: indicates end-of-record; the data returned completed a record (generally used with sockets of type SOCK_SEQPACKET). */
#define MSG_EOR 0x80
/* MSG_WAITALL: For recv() on stream sockets only.
   This flag requests that the operation block until the full request is satisfied.
 */
#define MSG_WAITALL 0x100
/* MSG_CONFIRM: Tell the link layer that forward progress happened.
   The kernel maintains an ARP table. An ARP entry is considered stale after some time.
   If the upper layer application is able to confirm that the mapping is still correct,
   it can use this flag to inform the kernel not to delete the ARP entry.
   Valid only on SOCK_DGRAM and SOCK_RAW sockets and currently implemented only for IPv4 and IPv6.
 */
#define MSG_CONFIRM 0x800
/* MSG_ERRQUEUE: This flag specifies that queued errors should be received from the socket error queue.
   Use IP_RECVERR socket option to enable this for UDP.
   For TCP: Note that TCP has no error queue; MSG_ERRQUEUE is not permitted on SOCK_STREAM sockets (see ip(7)).
   IP_RECVERR is valid for TCP, but all errors are returned by socket function return or SO_ERROR only.
   Unix sockets do not have error queue.
 */
#define MSG_ERRQUEUE 0x2000
/* MSG_NOSIGNAL: For both send() and recv() on stream sockets.
   Don't generate a SIGPIPE signal if the peer on a stream-oriented socket has closed the connection.
 */
#define MSG_NOSIGNAL 0x4000
/* MSG_MORE: The caller has more data to send.
   This flag is used with TCP sockets to obtain the same effect as the TCP_CORK socket option.
   This flag is not supported by UNIX domain sockets (See unix(7)).
 */
#define MSG_MORE 0x8000
/* MSG_WAITFORONE: Only for recvmmsg(). Turns on MSG_DONTWAIT after the first message has been received. */
#define MSG_WAITFORONE 0x10000
/* MSG_SOCK_DEVMEM: See https://docs.kernel.org/networking/devmem.html. */
#define MSG_SOCK_DEVMEM 0x2000000
/* MSG_ZEROCOPY: TCP and UDP only. See https://docs.kernel.org/networking/msg_zerocopy.html. */
#define MSG_ZEROCOPY 0x4000000
/* MSG_FASTOPEN: TCP only. See https://datatracker.ietf.org/doc/html/rfc7413. */
#define MSG_FASTOPEN 0x20000000
/* MSG_CMSG_CLOEXEC: For recvmsg() only.
   If a file descriptor is received via an ancillary message
   (only Unix sockets support file descriptor passing vis SCM_RIGHTS),
   set the CLOEXEC flag on that file descriptor.
 */
#define MSG_CMSG_CLOEXEC 0x40000000

/* Structures related to MSG_ERRQUEUE */
struct sock_ee_data_rfc4884 {
  uint16_t len;
  uint8_t flags;
  uint8_t reserved;
};

struct sock_extended_err {
  uint32_t ee_errno;	
  uint8_t ee_origin;
  uint8_t ee_type;
  uint8_t ee_code;
  uint8_t ee_pad;
  uint32_t ee_info;
  union	{
    uint32_t ee_data;
    struct sock_ee_data_rfc4884 ee_rfc4884;
  };
};

#define SO_EE_ORIGIN_NONE 0
#define SO_EE_ORIGIN_LOCAL 1
#define SO_EE_ORIGIN_ICMP 2
#define SO_EE_ORIGIN_ICMP6 3
#define SO_EE_ORIGIN_TXSTATUS 4
#define SO_EE_ORIGIN_ZEROCOPY 5
#define SO_EE_ORIGIN_TXTIME 6
#define SO_EE_ORIGIN_TIMESTAMPING SO_EE_ORIGIN_TXSTATUS
#define SO_EE_OFFENDER(ee) ((struct sockaddr*)((ee)+1))
#define SO_EE_CODE_ZEROCOPY_COPIED 1
#define SO_EE_CODE_TXTIME_INVALID_PARAM 1
#define SO_EE_CODE_TXTIME_MISSED 2
#define SO_EE_RFC4884_FLAG_INVALID 1

/* Flags of shutdown() */

#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2

/* Socket options */

#define SOL_SOCKET 1

/* SO_DEBUG: This option toggles recording of debugging information in the underlying protocol modules.
   Seems mostly unused by the kernel. It likely exists only for BSD compatibility.
 */
#define SO_DEBUG 1
/* SO_REUSEADDR: TCP only. Allow reusing a local address even if it is in TIME_WAIT state.
   However, it cannot be used to reuse an address if there is an active listener.
   See also SO_REUSEPORT below.
 */
#define SO_REUSEADDR 2
/* SO_TYPE: For getsockopt() only. Tell whether the socket is SOCK_STREAM, SOCK_DGRAM, or SOCK_RAW. */
#define SO_TYPE 3
/* SO_ERROR: For getsockopt() only. Retrieve the error code of a previous operation, and clear the error.
   Internally, it is related to the sk_err and sk_soft_err fields of a socket.
   Therefore, grep for these fields along the codepath to see where the error occurred.
 */
#define SO_ERROR 4
/* SO_DONTROUTE: Similar to MSG_DONTROUTE. */
#define SO_DONTROUTE 5
/* SO_BROADCAST: UDP only. Whether to allow sending packets to a broadcast address. */
#define SO_BROADCAST 6
/* SO_SNDBUF: Sets or gets the maximum socket send buffer in bytes. */
#define SO_SNDBUF 7
/* SO_RCVBUF: Sets or gets the maximum socket receive buffer in bytes. */
#define SO_RCVBUF 8
/* SO_SNDBUFFORCE: Using this socket option, a privileged (CAP_NET_ADMIN) process can perform the same task as SO_SNDBUF,
   but the rmem_max limit can be overridden.
 */
#define SO_SNDBUFFORCE 32
/* SO_RCVBUFFORCE: See SO_SNDBUFFORCE above */
#define SO_RCVBUFFORCE 33
/* SO_KEEPALIVE: TCP only. Enable sending of keep-alive messages on connection-oriented sockets. */
#define SO_KEEPALIVE 9
/* SO_OOBINLINE: See MSG_OOB above. */
#define SO_OOBINLINE 10
/* SO_NO_CHECK: Seems unused on Linux. It is supposed to disable checksum checking. */
#define SO_NO_CHECK 11
/* SO_PRIORITY: Set the protocol-defined priority for all packets to be sent on this socket.
   Linux uses this value to order the networking queues.
   See: https://stackoverflow.com/questions/37998404/what-is-the-effect-of-setting-a-linux-socket-high-priority.
 */
#define SO_PRIORITY 12

/* SO_LINGER: If linger is enabled, a close() or shutdown() call will not return until all queued messages
   for the socket have been successfully sent or the linger timeout has been reached.
   This applies to all kinds of sockets (TCP, UDP, and Unix).

   There is another use of SO_LINGER specific to TCP.
   If the linger time is set to 0, then upon close() the system will not initiate the normal TCP shutdown procedure.
   Instead, the system sends RST.
   See https://stackoverflow.com/questions/3757289/when-is-tcp-option-so-linger-0-required.
 */
struct linger {
  int l_onoff;
  int l_linger;
};

#define SO_LINGER 13

/* SO_BSDCOMPAT: Currently unused on Linux. */
#define SO_BSDCOMPAT 14
/* SO_REUSEPORT: For TCP and UDP. Permits multiple AF_INET or AF_INET6 sockets to be bound to an identical socket address.
   See https://lwn.net/Articles/542629/.
   Unix sockets are not supported. https://stackoverflow.com/questions/23742368/can-so-reuseport-be-used-on-unix-domain-sockets
 */
#define SO_REUSEPORT 15

/* SO_PASSCRED: Unix socket only.
   Enabling this socket option causes receipt of the
   credentials of the sending process in an SCM_CREDENTIALS
   ancillary message in each subsequently received message.
 */
struct ucred {
  pid_t pid;
  uid_t uid;
  gid_t gid;
};

#define SO_PASSCRED 16

/* SO_PEERCRED: Unix socket only. Retrieve the credentials of the peer process. */
#define SO_PEERCRED 17
/* SO_RCVLOWAT, SO_SNDLOWAT:
   Specify the minimum number of bytes in the buffer until the
   socket layer will pass the data to the protocol
   (SO_SNDLOWAT) or the user on receiving (SO_RCVLOWAT).
   These two values are initialized to 1.  SO_SNDLOWAT is not
   changeable on Linux (setsockopt(2) fails with the error
   ENOPROTOOPT).  SO_RCVLOWAT is changeable only since Linux
   2.4. A read on the socket will block until SO_RCVLOWAT bytes are available.
   The syscalls select(), poll(), epoll() will indicate a socket as readable
   only if SO_RCVLOWAT bytes are available.

   SO_RCVLOWAT applies to stream-oriented socket only.
 */
#define SO_RCVLOWAT 18
#define SO_SNDLOWAT 19
/* SO_RCVTIMEO_OLD, SO_SNDTIMEO_OLD:
   Legacy versions of SO_RCVTIMEO, SO_SNDTIMEO that use 32-bit struct timeval.
 */
#define SO_RCVTIMEO_OLD 20
#define SO_SNDTIMEO_OLD 21

/* Security levels - as per NRL IPv6 - don't actually do anything */
#define SO_SECURITY_AUTHENTICATION 22
#define SO_SECURITY_ENCRYPTION_TRANSPORT 23
#define SO_SECURITY_ENCRYPTION_NETWORK 24

/* SO_BINDTODEVICE: TCP and UDP. Bind a socket to a particular network interface.
   This interface is "asymmetrical": for setsockopt() one provides the name of the interface,
   but for getsockopt() one gets the index of the interface.
   See also SO_BINDTOIFINDEX below.
 */
#define SO_BINDTODEVICE 25
/* SO_ATTACH_FILTER: Attach classic BPF filter. Deprecated.
   See: https://docs.kernel.org/bpf/classic_vs_extended.html.
 */
#define SO_ATTACH_FILTER 26
/* SO_DETACH_FILTER: Detach classic or eBPF filter. */
#define SO_DETACH_FILTER 27
#define SO_GET_FILTER SO_ATTACH_FILTER
/* SO_PEERNAME: For getsockopt() only. Seems to be an undocumented version of getpeername(). */
#define SO_PEERNAME 28
/* SO_ACCEPTCONN: For getsockopt() only.
   Returns a value indicating whether or not this socket has
   been marked to accept connections with listen(2).
   Note that listen() is only for stream and seqpacket sockets.
 */
#define SO_ACCEPTCONN 30
/* SO_PEERSEC: Return the security context of the peer socket connected to this socket.
   Unix socket only. This is related to Linux Security Modules (LSM).
 */
#define SO_PEERSEC 31
/* SO_PASSSEC: Enables receiving of the SELinux security label of the peer
   socket in an ancillary message of type SCM_SECURITY.
   Unix socket only. This is related to Linux Security Modules (LSM).
 */
#define SO_PASSSEC 34
/* SO_MARK: Set a mark (a 32-bit unsigned integer) on each packet sent through this socket.
   Marks can be used by iptable, ebpf to manage packets.
   Since eBPF also works on Unix sockets, it is possible to set this for all kinds of sockets (TCP, UDP, Unix).
 */
#define SO_MARK 36
/* SO_PROTOCOL: For getsockopt() only. Retrieve the protocol (e.g. IPPROTO_TCP) of a socket. */
#define SO_PROTOCOL 38
/* SO_DOMAIN: For getsockopt() only. Retrieve the address family (AF_UNIX or AF_INET) of a socket. */
#define SO_DOMAIN 39
/* SO_RXQ_OVFL: UDP only.
   Indicates that an unsigned 32-bit value ancillary message
   (cmsg) should be attached to received skbs indicating the
   number of packets dropped by the socket since its creation.
 */
#define SO_RXQ_OVFL 40
/* SO_WIFI_STATUS: Undocumented. Kernel comment: "push wifi status to userspace."
   Seems to generate an ancillary message with type SCM_WIFI_STATUS.
   See net/socket.c, function __sock_recv_wifi_status().
 */
#define SO_WIFI_STATUS 41
#define SCM_WIFI_STATUS SO_WIFI_STATUS
/* SO_PEEK_OFF: This option, which is currently supported only for unix(7)
   sockets, sets the value of the "peek offset" for the
   recv(2) system call when used with MSG_PEEK flag.
   See socket(7) for more details.

   It seems that this option is now also supported by TCP and UDP.
 */
#define SO_PEEK_OFF 42
/* SO_NOFCS: "Tell NIC not to do the Ethernet FCS. Will use last 4 bytes of packet sent from user-space instead."
   FCS is Frame Check Sequence, a checksum at the end of Ethernet frames.
   This options seems to be only used on raw sockets on Ethernet interfaces.
 */
#define SO_NOFCS 43
/* SO_LOCK_FILTER: When set, this option will prevent changing the filters associated with the socket. */
#define SO_LOCK_FILTER 44
/* SO_SELECT_ERR_QUEUE: Only for backward compatibility. */
#define SO_SELECT_ERR_QUEUE 45
/* SO_BUSY_POLL: Sets the approximate time in microseconds to busy poll on a blocking receive when there is no data.
   Busy-polling is only for TCP, UDP, and XDP.
 */
#define SO_BUSY_POLL 46
/* SO_MAX_PACING_RATE: This option is related to the Fair Queue packet scheduler.
   See https://man7.org/linux/man-pages/man8/tc-fq.8.html.
   Like all traffic control features, this is TCP and UDP only.
 */
#define SO_MAX_PACING_RATE 47
/* SO_BPF_EXTENSIONS: Obtain information about which BPF extensions are supported by the current kernel.
   For getsockopt() only. Seems currently mostly unused.
 */
#define SO_BPF_EXTENSIONS 48
/* SO_INCOMING_CPU: TCP and UDP. Sets or gets the CPU affinity of a socket.
   It is related to the SO_REUSEPORT feature.
   If multiple sockets are waiting on the same address, we can let each thread handle one socket,
   and bind both the thread and the socket to the same CPU.
 */
#define SO_INCOMING_CPU 49
/* SO_ATTACH_BPF: Attach eBPF filter. */
#define SO_ATTACH_BPF 50
#define SO_DETACH_BPF SO_DETACH_FILTER
/* SO_ATTACH_REUSEPORT_CBPF, SO_ATTACH_REUSEPORT_EBPF:
   For use with the SO_REUSEPORT option, these options allow
   the user to set a classic BPF (SO_ATTACH_REUSEPORT_CBPF) or
   an extended BPF (SO_ATTACH_REUSEPORT_EBPF) program which
   defines how packets are assigned to the sockets in the
   reuseport group (that is, all sockets which have
   SO_REUSEPORT set and are using the same local address to
   receive packets).
 */
#define SO_ATTACH_REUSEPORT_CBPF 51
#define SO_ATTACH_REUSEPORT_EBPF 52
/* SO_CNX_ADVICE: For setsockopt() only.
   The purpose is to allow an application to give feedback to the kernel about
   the quality of the network path for a connected socket.
   https://patchwork.ozlabs.org/project/netdev/patch/1456336972-3024417-1-git-send-email-tom@herbertland.com/
 */
#define SO_CNX_ADVICE 53
/* SO_MEMINFO: For getsockopt() only.
   Allows reading of SK_MEMINFO_VARS via socket option. This way an
   application can get all meminfo related information in single socket
   option call instead of multiple calls.
   Each entry in the enum below is returned as a 32-bit unsigned integer.
   Hence, prepare a buffer of sufficient size.
   https://patchwork.ozlabs.org/project/netdev/patch/1490037723-6475-1-git-send-email-johunt@akamai.com/
   See also https://unix.stackexchange.com/questions/33855/kernel-socket-structure-and-tcp-diag.
 */
enum {
  SK_MEMINFO_RMEM_ALLOC, /* Allocated receive buffer size */
  SK_MEMINFO_RCVBUF, /* Receive buffer size limit */
  SK_MEMINFO_WMEM_ALLOC, /* Allocated send buffer size */
  SK_MEMINFO_SNDBUF, /* Send buffer size limit */
  SK_MEMINFO_FWD_ALLOC, /* man ss(8) says: the memory allocated by the socket as cache, but not used for receiving/sending packet yet. */
  SK_MEMINFO_WMEM_QUEUED, /* Amount of data in the send queue */
  SK_MEMINFO_OPTMEM, /* Amount of memory for storing socket options and other data */
  SK_MEMINFO_BACKLOG, /* man ss(8) says: The memory used for the sk backlog queue. */
  SK_MEMINFO_DROPS, /* Number of packets dropped. See SO_RXQ_OVFL */

  SK_MEMINFO_VARS,
};

#define SO_MEMINFO 55

/* SO_INCOMING_NAPI_ID: Returns a system-level unique ID called NAPI ID that is
   associated with a RX queue on which the last packet
   associated with that socket is received.
   https://docs.kernel.org/networking/napi.html
 */
#define SO_INCOMING_NAPI_ID 56
/* SO_COOKIE: Obtain the socket cookie.
   A cookie is a global unique identifier for a socket generated by the kernel.
   See: https://docs.ebpf.io/linux/helper-function/bpf_get_socket_cookie/,
   https://blog.cloudflare.com/a-story-about-af-xdp-network-namespaces-and-a-cookie/.
 */
#define SO_COOKIE 57
/* SO_PEERGROUPS: Unix socket only.
   This adds the new getsockopt(2) option SO_PEERGROUPS on SOL_SOCKET to
   retrieve the auxiliary groups of the remote peer. It is designed to
   naturally extend SO_PEERCRED.
   https://patchwork.ozlabs.org/project/netdev/patch/20170621084715.17772-1-dh.herrmann@gmail.com/
 */
#define SO_PEERGROUPS 59
/* SO_ZEROCOPY: TCP and UDP. Signal to the kernel that we intend to use MSG_ZEROCOPY on this socket.
   See https://www.kernel.org/doc/html/latest/networking/msg_zerocopy.html for why this is necessary.
 */
#define SO_ZEROCOPY 60
/* SO_TXTIME: This option is related to the Earliest TxTime queue discipline.
   See https://man7.org/linux/man-pages/man8/tc-etf.8.html.
 */
#define SO_TXTIME 61
#define SCM_TXTIME SO_TXTIME
/* SO_BINDTOIFINDEX: Bind the socket to a network interface, specified using the index of that interface.
   https://patchwork.ozlabs.org/project/netdev/patch/5086A7FD.3020503@parallels.com/
 */
#define SO_BINDTOIFINDEX 62
/* SO_RCV_TIMEO_NEW, SO_SNDTIMEO_NEW: Timeout for send() and recv(). Uses 64-bit struct timeval. */
#define SO_RCVTIMEO_NEW 66
#define SO_SNDTIMEO_NEW 67
#define SO_RCVTIMEO SO_RCVTIMEO_NEW
#define SO_SNDTIMEO SO_SNDTIMEO_NEW
/* SO_DETACH_REUSEPORT_BPF: See SO_ATTACH_REUSEPORT_BPF above. */
#define SO_DETACH_REUSEPORT_BPF 68
/* SO_PREFER_BUSY_POLL: Preferred busy-polling.
   Seems to be limited to XDP.
   https://lwn.net/Articles/837010/
 */
#define SO_PREFER_BUSY_POLL 69
/* SO_BUSY_POLL_BUDGET: Busy-polling budget.
   https://docs.kernel.org/networking/napi.html
 */
#define SO_BUSY_POLL_BUDGET 70
/* SO_NETNS_COOKIE: Get the cookie of the networking namespace associated with this socket.
   Note that Unix sockets are not networking namespace-aware.
   They are filesystem-based.
   See SO_COOKIE above.
 */
#define SO_NETNS_COOKIE 71

/* SO_BUF_LOCK: Lock or unlock the size of the send/receive buffers.
   This has no effect on Unix sockets because the kernel never automatically increases the size of buffers for Unix sockets.
   If sending a datagram causes the buffer to overflow, the sender blocks.
   https://patchwork.ozlabs.org/project/sparclinux/patch/20210730105406.318726-1-ptikhomirov@virtuozzo.com/
 */
#define SOCK_SNDBUF_LOCK 1
#define SOCK_RCVBUF_LOCK 2

#define SO_BUF_LOCK 72

/* SO_RESERVE_MEM: This socket option provides a mechanism for users to reserve a certain amount of memory for the socket to use.
   https://patchwork.kernel.org/project/netdevbpf/cover/20210929172513.3930074-1-weiwan@google.com/
 */
#define SO_RESERVE_MEM 73

/* SO_TXREHASH: TCP only. Controls the "hash rethink" behavior.
   Google AI says, "hash rethink" is a mechanism for avoiding denial-of-service attacks.
 */
#define SO_TXREHASH 74

#define SOCK_TXREHASH_DEFAULT 255
#define SOCK_TXREHASH_DISABLED 0
#define SOCK_TXREHASH_ENABLED 1

/* SO_RCVMARK: UDP only. Receive packet mark via ancillary data.
   Note that the packet mark is not sent over the wire.
   This mark can only be set by local eBPF programs processing the packet.
 */
#define SO_RCVMARK 75
/* SO_PASSPIDFD: Unix socket. When sending messages provide a pidfd to the peer via ancillary message. */
#define SO_PASSPIDFD 76
/* SO_PEERPIDFD: For getsockopt() only. Unix socket only. Receive a pidfd to the peer. */
#define SO_PEERPIDFD 77
/* SO_DEVMEM_*: TCP only. See https://docs.kernel.org/networking/devmem.html. */
#define SO_DEVMEM_LINEAR 78
#define SCM_DEVMEM_LINEAR SO_DEVMEM_LINEAR
#define SO_DEVMEM_DMABUF 79
#define SCM_DEVMEM_DMABUF SO_DEVMEM_DMABUF
#define SO_DEVMEM_DONTNEED 80
/* SO_RCVPRIORITY: UDP only. Receive the priority of a packet (SO_PRIORITY) via ancillary message.
   Note that the socket option SO_PRIORITY is only related to sending a packet.
   The priority of a received packet can be modified by traffic control algorithms,
   or bpf programs via __sk_buff->priority.
 */
#define SO_RCVPRIORITY 82
/* SO_PASSRIGHTS: Disable Unix socket file descriptor passing.
   See https://lwn.net/ml/all/20250508013021.79654-7-kuniyu@amazon.com/ for why this is needed.
 */
#define SO_PASSRIGHTS 83

/* Options related to timestamping.
   SO_TIMESTAMP and SO_TIMESTAMPNS are implemented for UDP and Unix datagram sockets only.
   They only provide timestamps for received datagrams.
   SO_TIMESTAMPING allows timestamping both sent and received packets, and also supports TCP.
   For sent packets, the timestamps are retrieved via MSG_ERRQUEUE.
   See https://docs.kernel.org/networking/timestamping.html.
 */
#define SO_TIMESTAMP_OLD 29
#define SO_TIMESTAMPNS_OLD 35
#define SO_TIMESTAMPING_OLD 37
#define SO_TIMESTAMP_NEW 63
#define SO_TIMESTAMPNS_NEW 64
#define SO_TIMESTAMPING_NEW 65
#define SCM_TIMESTAMPING_OPT_STATS 54
#define SCM_TIMESTAMPING_PKTINFO 58
#define SCM_TS_OPT_ID 81
#define SO_TIMESTAMP SO_TIMESTAMP_NEW
#define SO_TIMESTAMPNS SO_TIMESTAMPNS_NEW
#define SO_TIMESTAMPING SO_TIMESTAMPING_NEW
#define SO_RCVTIMEO SO_RCVTIMEO_NEW
#define SO_SNDTIMEO SO_SNDTIMEO_NEW
#define SCM_TIMESTAMP SO_TIMESTAMP
#define SCM_TIMESTAMPNS SO_TIMESTAMPNS
#define SCM_TIMESTAMPING SO_TIMESTAMPING

enum {
  SOF_TIMESTAMPING_TX_HARDWARE = (1<<0),
  SOF_TIMESTAMPING_TX_SOFTWARE = (1<<1),
  SOF_TIMESTAMPING_RX_HARDWARE = (1<<2),
  SOF_TIMESTAMPING_RX_SOFTWARE = (1<<3),
  SOF_TIMESTAMPING_SOFTWARE = (1<<4),
  SOF_TIMESTAMPING_SYS_HARDWARE = (1<<5),
  SOF_TIMESTAMPING_RAW_HARDWARE = (1<<6),
  SOF_TIMESTAMPING_OPT_ID = (1<<7),
  SOF_TIMESTAMPING_TX_SCHED = (1<<8),
  SOF_TIMESTAMPING_TX_ACK = (1<<9),
  SOF_TIMESTAMPING_OPT_CMSG = (1<<10),
  SOF_TIMESTAMPING_OPT_TSONLY = (1<<11),
  SOF_TIMESTAMPING_OPT_STATS = (1<<12),
  SOF_TIMESTAMPING_OPT_PKTINFO = (1<<13),
  SOF_TIMESTAMPING_OPT_TX_SWHW = (1<<14),
  SOF_TIMESTAMPING_BIND_PHC = (1 << 15),
  SOF_TIMESTAMPING_OPT_ID_TCP = (1 << 16),
  SOF_TIMESTAMPING_OPT_RX_FILTER = (1 << 17),
  SOF_TIMESTAMPING_TX_COMPLETION = (1 << 18),

  SOF_TIMESTAMPING_LAST = SOF_TIMESTAMPING_TX_COMPLETION,
  SOF_TIMESTAMPING_MASK = ((SOF_TIMESTAMPING_LAST - 1) | SOF_TIMESTAMPING_LAST)
};

enum {
  SCM_TSTAMP_SND, /* driver passed skb to NIC, or HW */
  SCM_TSTAMP_SCHED, /* data entered the packet scheduler */
  SCM_TSTAMP_ACK, /* data acknowledged by peer */
  SCM_TSTAMP_COMPLETION, /* packet tx completion */
};

/* IP options use the option level IPPROTO_IP */

#define IP_TOS 1
#define IP_TTL 2
#define IP_HDRINCL 3
#define IP_OPTIONS 4
#define IP_ROUTER_ALERT 5
#define IP_RECVOPTS 6
#define IP_RETOPTS 7
#define IP_PKTINFO 8
#define IP_PKTOPTIONS 9
#define IP_MTU_DISCOVER 10
#define IP_RECVERR 11
#define IP_RECVTTL 12
#define	IP_RECVTOS 13
#define IP_MTU 14
#define IP_FREEBIND 15
#define IP_IPSEC_POLICY 16
#define IP_XFRM_POLICY 17
#define IP_PASSSEC 18
#define IP_TRANSPARENT 19
#define IP_ORIGDSTADDR 20
#define IP_MINTTL 21
#define IP_NODEFRAG 22
#define IP_CHECKSUM 23
#define IP_BIND_ADDRESS_NO_PORT 24
#define IP_RECVFRAGSIZE 25
#define IP_RECVERR_RFC4884 26
#define IP_MULTICAST_IF 32
#define IP_MULTICAST_TTL 33
#define IP_MULTICAST_LOOP 34
#define IP_ADD_MEMBERSHIP 35
#define IP_DROP_MEMBERSHIP 36
#define IP_UNBLOCK_SOURCE 37
#define IP_BLOCK_SOURCE 38
#define IP_ADD_SOURCE_MEMBERSHIP 39
#define IP_DROP_SOURCE_MEMBERSHIP 40
#define IP_MSFILTER 41
#define MCAST_JOIN_GROUP 42
#define MCAST_BLOCK_SOURCE 43
#define MCAST_UNBLOCK_SOURCE 44
#define MCAST_LEAVE_GROUP 45
#define MCAST_JOIN_SOURCE_GROUP 46
#define MCAST_LEAVE_SOURCE_GROUP 47
#define MCAST_MSFILTER 48
#define IP_MULTICAST_ALL 49
#define IP_UNICAST_IF 50
#define IP_LOCAL_PORT_RANGE 51
#define IP_PROTOCOL 52

/* IP_MTU_DISCOVER values */
#define IP_PMTUDISC_DONT 0 /* Never send DF frames */
#define IP_PMTUDISC_WANT 1 /* Use per route hints */
#define IP_PMTUDISC_DO 2 /* Always DF */
#define IP_PMTUDISC_PROBE 3 /* Ignore dst pmtu */
/* Always use interface mtu (ignores dst pmtu) but don't set DF flag.
 * Also incoming ICMP frag_needed notifications will be ignored on
 * this socket to prevent accepting spoofed ones.
 */
#define IP_PMTUDISC_INTERFACE 4
/* weaker version of IP_PMTUDISC_INTERFACE, which allows packets to get
 * fragmented if they exceed the interface mtu
 */
#define IP_PMTUDISC_OMIT 5

/* TCP options use the option level IPPROTO_TCP */

#define TCP_NODELAY 1 /* Turn off Nagle's algorithm. */
#define TCP_MAXSEG 2 /* Limit MSS */
#define TCP_CORK 3 /* Never send partially complete segments */
#define TCP_KEEPIDLE 4 /* Start keeplives after this period */
#define TCP_KEEPINTVL 5 /* Interval between keepalives */
#define TCP_KEEPCNT 6 /* Number of keepalives before death */
#define TCP_SYNCNT 7 /* Number of SYN retransmits */
#define TCP_LINGER2 8 /* Life time of orphaned FIN-WAIT-2 state */
#define TCP_DEFER_ACCEPT 9 /* Wake up listener only when data arrive */
#define TCP_WINDOW_CLAMP 10 /* Bound advertised window */
#define TCP_INFO 11 /* Information about this connection. */
#define TCP_QUICKACK 12 /* Block/reenable quick acks */
#define TCP_CONGESTION 13 /* Congestion control algorithm */
#define TCP_MD5SIG 14 /* TCP MD5 Signature (RFC2385) */
#define TCP_THIN_LINEAR_TIMEOUTS 16 /* Use linear timeouts for thin streams*/
#define TCP_THIN_DUPACK 17 /* Fast retrans. after 1 dupack */
#define TCP_USER_TIMEOUT 18 /* How long for loss retry before timeout */
#define TCP_REPAIR 19 /* TCP sock is under repair right now */
#define TCP_REPAIR_QUEUE 20
#define TCP_QUEUE_SEQ 21
#define TCP_REPAIR_OPTIONS 22
#define TCP_FASTOPEN 23 /* Enable FastOpen on listeners */
#define TCP_TIMESTAMP 24
#define TCP_NOTSENT_LOWAT 25 /* limit number of unsent bytes in write queue */
#define TCP_CC_INFO 26 /* Get Congestion Control (optional) info */
#define TCP_SAVE_SYN 27 /* Record SYN headers for new connections */
#define TCP_SAVED_SYN 28 /* Get SYN headers recorded for connection */
#define TCP_REPAIR_WINDOW 29 /* Get/set window parameters */
#define TCP_FASTOPEN_CONNECT 30 /* Attempt FastOpen with connect */
#define TCP_ULP 31 /* Attach a ULP to a TCP connection */
#define TCP_MD5SIG_EXT 32 /* TCP MD5 Signature with extensions */
#define TCP_FASTOPEN_KEY 33 /* Set the key for Fast Open (cookie) */
#define TCP_FASTOPEN_NO_COOKIE 34 /* Enable TFO without a TFO cookie */
#define TCP_ZEROCOPY_RECEIVE 35
#define TCP_INQ 36 /* Notify bytes available to read as a cmsg on read */
#define TCP_CM_INQ TCP_INQ
#define TCP_TX_DELAY 37 /* delay outgoing packets by XX usec */
#define TCP_AO_ADD_KEY 38 /* Add/Set MKT */
#define TCP_AO_DEL_KEY 39 /* Delete MKT */
#define TCP_AO_INFO 40 /* Set/list TCP-AO per-socket options */
#define TCP_AO_GET_KEYS 41 /* List MKT(s) */
#define TCP_AO_REPAIR 42 /* Get/Set SNEs and ISNs */
#define TCP_IS_MPTCP 43 /* Is MPTCP being used? */
#define TCP_RTO_MAX_MS 44 /* max rto time in ms */
#define TCP_RTO_MIN_US 45 /* min rto time in us */
#define TCP_DELACK_MAX_US 46 /* max delayed ack time in us */

/* TCP repair, see https://lwn.net/Articles/495304/. */
#define TCP_REPAIR_ON 1
#define TCP_REPAIR_OFF 0
#define TCP_REPAIR_OFF_NO_WP -1 /* Turn off without window probes */

/* UDP options use the option level IPPROTO_UDP */
#define UDP_CORK 1 /* Never send partially complete segments */
#define UDP_ENCAP 100 /* Set the socket to accept encapsulated packets */
#define UDP_NO_CHECK6_TX 101 /* Disable sending checksum for UDP6X */
#define UDP_NO_CHECK6_RX 102 /* Disable accepting checksum for UDP6 */
#define UDP_SEGMENT 103 /* Set GSO segmentation size */
#define UDP_GRO 104 /* This socket can receive UDP GRO packets */

typedef unsigned short sa_family_t;

struct sockaddr {
  sa_family_t sa_family;
  /* C++ forbids variable-length array. Make it happy. */
  char sa_data[1];
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

struct msghdr {
  void *msg_name; /* ptr to socket address structure */
  int msg_namelen; /* size of socket address structure */
  void *msg_iov; /* scatter/gather array, this is supposed to be struct iovec. Include uio.h. */
  size_t msg_iovlen; /* # elements in msg_iov */
  void *msg_control; /* ancillary data */
  size_t msg_controllen; /* ancillary data buffer length */
  unsigned int msg_flags; /* flags on received message */
};

struct mmsghdr {
  struct msghdr msg_hdr;
  unsigned int msg_len;
};

struct cmsghdr {
  size_t cmsg_len; /* data byte count, including hdr */
  int cmsg_level; /* originating protocol */
  int cmsg_type; /* protocol-specific type */
};

fd_t socket (int domain, int type, int protocol);
int socketpair (int domain, int type, int protocol, fd_t * sv);
int bind (fd_t fd, const struct sockaddr * addr, int addrlen);
int connect (fd_t fd, const struct sockaddr * addr, int addrlen);
int accept (fd_t fd, struct sockaddr * addr, int * addrlen);
int accept4 (fd_t fd, struct sockaddr * addr, int * addrlen, int flags);
int shutdown (fd_t fd, int how);
ssize_t send (fd_t fd, const void * buff, size_t len, unsigned int flags);
ssize_t sendto (fd_t fd, const void * buff, size_t len, unsigned int flags, const struct sockaddr * addr, int addr_len);
ssize_t sendmsg (fd_t fd, const struct msghdr * msg, int flags);
int sendmmsg (fd_t fd, struct mmsghdr * msgvec, unsigned int vlen, int flags);
ssize_t recv (fd_t fd, void * ubuf, size_t size, unsigned int flags);
ssize_t recvfrom (fd_t fd, void * ubuf, size_t size, unsigned int flags, struct sockaddr * addr, int * addr_len);
ssize_t recvmsg (fd_t fd, struct msghdr * msg, int flags);
int recvmmsg (fd_t fd, struct mmsghdr * msgvec, unsigned int vlen, int flags, void * timeout); /* timeout should be struct timespec */
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
