#ifndef EXECVE_H
#define EXECVE_H

#ifdef __cplusplus
extern "C" {
#endif

int execve (const char * pathname, char * const argv[], char * const envp[]);

#ifdef __cplusplus
}
#endif

#endif
