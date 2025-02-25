/* random.h
   Adapted from Linux kernel include/uapi/linux/random.h
 */

#ifndef RANDOM_H
#define RANDOM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Flags for getrandom(2)
 *
 * GRND_NONBLOCK	Don't block and return EAGAIN instead
 * GRND_RANDOM		No effect
 * GRND_INSECURE	Return non-cryptographic random bytes
 */
#define GRND_NONBLOCK	0x0001
#define GRND_RANDOM	0x0002
#define GRND_INSECURE	0x0004

ssize_t getrandom (void * buf, size_t buflen, unsigned int flags);

#ifdef __cplusplus
}
#endif

#endif
