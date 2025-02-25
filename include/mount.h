#ifndef MOUNT_H
#define MOUNT_H

#ifdef __cplusplus
extern "C" {
#endif

int mount (const char * source, const char * target, const char * filesystemtype, unsigned long mountflags, const void * data);

#ifdef __cplusplus
}
#endif

#endif
