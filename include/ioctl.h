/* ioctl.h
   Adapted from Linux kernel include/uapi/asm-generic/ioctl.h
   Adapted from Linux kernel include/uapi/asm-generic/ioctls.h
 */

#ifndef IOCTL_H
#define IOCTL_H

#include <io.h>

#ifdef __cplusplus
extern "C" {
#endif

int ioctl (fd_t fd, unsigned long request, void * argp);

/* Hard-coded ioctl commands */

#define TIOCSCTTY 0x540E
#define TIOCNOTTY 0x5422

/* Encoded commands */

#define _IOC_NRBITS	8
#define _IOC_TYPEBITS	8
#define _IOC_SIZEBITS	14
#define _IOC_DIRBITS	2

#define _IOC_NRMASK	((1 << _IOC_NRBITS) - 1)
#define _IOC_TYPEMASK	((1 << _IOC_TYPEBITS) - 1)
#define _IOC_SIZEMASK	((1 << _IOC_SIZEBITS) - 1)
#define _IOC_DIRMASK	((1 << _IOC_DIRBITS) - 1)

#define _IOC_NRSHIFT	0
#define _IOC_TYPESHIFT	(_IOC_NRSHIFT + _IOC_NRBITS)
#define _IOC_SIZESHIFT	(_IOC_TYPESHIFT + _IOC_TYPEBITS)
#define _IOC_DIRSHIFT	(_IOC_SIZESHIFT + _IOC_SIZEBITS)

#define _IOC_NONE	0U
#define _IOC_WRITE	1U
#define _IOC_READ	2U

#define _IOC(dir,type,nr,size) \
	(((dir)  << _IOC_DIRSHIFT) | \
	 ((type) << _IOC_TYPESHIFT) | \
	 ((nr)   << _IOC_NRSHIFT) | \
	 ((size) << _IOC_SIZESHIFT))

#define _IO(type,nr)			_IOC(_IOC_NONE,(type),(nr),0)
#define _IOR(type,nr,argtype)		_IOC(_IOC_READ,(type),(nr),(sizeof(argtype)))
#define _IOW(type,nr,argtype)		_IOC(_IOC_WRITE,(type),(nr),(sizeof(argtype)))
#define _IOWR(type,nr,argtype)		_IOC(_IOC_READ|_IOC_WRITE,(type),(nr),(sizeof(argtype)))

#ifdef __cplusplus
}
#endif

#endif
