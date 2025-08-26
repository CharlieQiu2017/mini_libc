#ifndef VDSO_H
#define VDSO_H

#ifdef __cplusplus
extern "C" {
#endif

void interpret_vdso_from_auxv (void * auxv);
void * get_vdso_getrandom_ptr (void);
void * get_vdso_gettimeofday_ptr (void);
void * get_vdso_clock_gettime (void);
void * get_vdso_clock_getres (void);

#ifdef __cplusplus
}
#endif

#endif
