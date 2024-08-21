# Mini C Library for AArch64

For demonstrating various features of the Linux kernel it is useful to have a C library that gets as little in the way between the application and the kernel as possible.
Therefore I choose to build a trimmed-down version of [musl-libc](https://musl.libc.org/).
It is highly experimental and intended only for static linking.
It is deliberately not compatible with standard C library.
We only target AArch64 since that is the platform I intend to do kernel experiments upon.

## Building

1. Install `aarch64-none-elf` toolchain from https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads.
2. Modify `Makefile` so it correctly points to the installed toolchain.
3. Invoke `make` to build without optimization. Invoke `make optimize=1` to build with optimization `-Os`.

Output files:
* `crt.o`: Initialization object, link it into every executable.
* `libc.a`: C library without PIE support.
* `libc_pic.a`: C library with PIE support.

## TODO

* Implement `malloc`, `realloc`, and `free`.
* Implement formatted I/O, but we'll probably not use the variadic `printf` API.
