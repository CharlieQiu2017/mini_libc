#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
#define restrict
extern "C" {
#endif

size_t strlen (const char * str);
char * strcpy (char * restrict d, const char * restrict s);
void * memset (void * dest, uint32_t c, size_t n);
void * memcpy (void * restrict dest, const void * restrict src, size_t n);
void * memmove (void * dest, const void * src, size_t n);
int memcmp (const void * vl, const void * vr, size_t n);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif
