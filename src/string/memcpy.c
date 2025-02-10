/* memcpy.c
   Derived from musl-libc src/string/memcpy.c
 */

#include <stddef.h>
#include <string.h>
#include <stdint.h>

void * memcpy (void * restrict dest, const void * restrict src, size_t n) {
  unsigned char * d = dest;
  const unsigned char * s = src;
  typedef uint32_t __attribute__((__may_alias__)) u32;
  uint32_t w, x;

  /* Align src to 4-byte boundary */
  for (/* empty */; (uintptr_t) s % 4 && n; n--) *d++ = *s++;

  /* If dst is also aligned to 4-byte boundary,
     then simply copy 4 bytes at once.
   */
  if ((uintptr_t) d % 4 == 0) {
    for (/* empty */; n >= 16; s += 16, d += 16, n -= 16) {
      *(u32 *)(d + 0) = *(u32 *)(s + 0);
      *(u32 *)(d + 4) = *(u32 *)(s + 4);
      *(u32 *)(d + 8) = *(u32 *)(s + 8);
      *(u32 *)(d + 12) = *(u32 *)(s + 12);
    }

    /* Handle remaining tail */
    if (n & 8) {
      *(u32 *)(d + 0) = *(u32 *)(s + 0);
      *(u32 *)(d + 4) = *(u32 *)(s + 4);
      d += 8; s += 8;
    }
    if (n & 4) {
      *(u32 *)(d + 0) = *(u32 *)(s + 0);
      d += 4; s += 4;
    }
    if (n & 2) {
      *d++ = *s++; *d++ = *s++;
    }
    if (n & 1) {
      *d = *s;
    }
    return dest;
  }

  /* AArch64 is little-endian,
     so ">>" corresponds to moving bytes to lower addresses.
   */

  if (n >= 32) {
    switch ((uintptr_t) d % 4) {
    case 1:
      w = *(u32 *) s;
      *d++ = *s++;
      *d++ = *s++;
      *d++ = *s++;
      n -= 3;
      for (/* empty */; n >= 17; s += 16, d += 16, n -= 16) {
	x = *(u32 *)(s + 1);
	*(u32 *)(d + 0) = (w >> 24) | (x << 8);
	w = *(u32 *)(s + 5);
	*(u32 *)(d + 4) = (x >> 24) | (w << 8);
	x = *(u32 *)(s + 9);
	*(u32 *)(d + 8) = (w >> 24) | (x << 8);
	w = *(u32 *)(s + 13);
	*(u32 *)(d + 12) = (x >> 24) | (w << 8);
      }
      break;
    case 2:
      w = *(u32 *) s;
      *d++ = *s++;
      *d++ = *s++;
      n -= 2;
      for (/* empty */; n >= 18; s += 16, d += 16, n -= 16) {
	x = *(u32 *)(s + 2);
	*(u32 *)(d + 0) = (w >> 16) | (x << 16);
	w = *(u32 *)(s + 6);
	*(u32 *)(d + 4) = (x >> 16) | (w << 16);
	x = *(u32 *)(s + 10);
	*(u32 *)(d + 8) = (w >> 16) | (x << 16);
	w = *(u32 *)(s + 14);
	*(u32 *)(d + 12) = (x >> 16) | (w << 16);
      }
      break;
    case 3:
      w = *(u32 *)s;
      *d++ = *s++;
      n -= 1;
      for (/* empty */; n >= 19; s += 16, d += 16, n -= 16) {
	x = *(u32 *)(s + 3);
	*(u32 *)(d + 0) = (w >> 8) | (x << 24);
	w = *(u32 *)(s + 7);
	*(u32 *)(d + 4) = (x >> 8) | (w << 24);
	x = *(u32 *)(s + 11);
	*(u32 *)(d + 8) = (w >> 8) | (x << 24);
	w = *(u32 *)(s + 15);
	*(u32 *)(d + 12) = (x >> 8) | (w << 24);
      }
      break;
    }
  }

  /* Handle remaining tail */
  if (n & 16) {
    *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
    *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
    *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
    *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
  }
  if (n & 8) {
    *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
    *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
  }
  if (n & 4) {
    *d++ = *s++; *d++ = *s++; *d++ = *s++; *d++ = *s++;
  }
  if (n & 2) {
    *d++ = *s++; *d++ = *s++;
  }
  if (n & 1) {
    *d = *s;
  }
  return dest;
}
