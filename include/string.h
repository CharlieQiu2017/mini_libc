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
/* The standard signature of memset() is void * memset (void * dest, int c, size_t n) which is confusing.
   C standard says: The memset function copies the value of c **(converted to an unsigned char)** into
   each of the first n characters of the object pointed to by s.
   Therefore we simply use uint8_t as the type for c.
 */
void * memset (void * dest, uint8_t c, size_t n);
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
