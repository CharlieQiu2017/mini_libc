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

/* If find_vdso_getrandom() is called during process initialization, and the kernel supports vDSO getrandom,
   then the vDSO version is called. Otherwise, the getrandom syscall is used.
 */
ssize_t getrandom (void * buf, size_t buflen, unsigned int flags);

/* The raw syscall interface */
ssize_t getrandom_syscall (void * buf, size_t buflen, unsigned int flags);

/* The vDSO getrandom opaque params */
struct vgetrandom_opaque_params {
  uint32_t size_of_opaque_state;
  uint32_t mmap_prot;
  uint32_t mmap_flags;
  uint32_t reserved[13];
};

/* Given pointer to the auxv array, find entrypoint of the vDSO getrandom function. */
/* This function should only be called once during process initialization. */
void find_vdso_getrandom (void * auxv);

#ifdef __cplusplus
}
#endif

#endif
