/* ioctl.h
   Adapted from Linux kernel include/uapi/asm-generic/ioctl.h
   Adapted from Linux kernel include/uapi/asm-generic/ioctls.h
 */

#ifndef IOCTL_H
#define IOCTL_H

#include <io_types.h>
#include <ioctl_vals.h>

#ifdef __cplusplus
extern "C" {
#endif

int ioctl (fd_t fd, unsigned long request, void * argp);

#ifdef __cplusplus
}
#endif

#endif
