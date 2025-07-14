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
int32_t strcmp (const char * l, const char * r);
size_t strnlen (const char * s, size_t n);
char * strncpy (char * restrict d, const char * restrict s, size_t n);
int32_t strncmp (const char * sl, const char * sr, size_t n);
void * memset (void * dest, uint32_t c, size_t n);
void * memcpy (void * restrict dest, const void * restrict src, size_t n);
void * memmove (void * dest, const void * src, size_t n);
int32_t memcmp (const void * vl, const void * vr, size_t n);
uint64_t safe_memcmp (const void * vl, const void * vr, size_t n);
void cond_memcpy (uint8_t cond, void * restrict vd, const void * restrict vs, size_t n);
void * memxor (void * restrict dest, const void * restrict src, size_t n);

#ifdef __cplusplus
}
#undef restrict
#endif

#endif
