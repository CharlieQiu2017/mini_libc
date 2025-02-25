#ifndef MULTIUSER_H
#define MULTIUSER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int pid_t;

pid_t setsid (void);

#ifdef __cplusplus
}
#endif

#endif
