// crt.asm
// Derived from musl-libc arch/aarch64/crt_arch.h

.text
.global _start
.type _start, function
_start:
	mov x29, #0
	mov x30, #0
	mov x0, sp
	and sp, x0, #-16
	b main
