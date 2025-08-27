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
Invoke `make debug=1` to build with debug information.

Output files:
* `crt.o`: Initialization object, link it into every executable.
* `libc.a`: C library without PIE support.
* `libc_pic.a`: C library with PIE support.

## Coding style and assumptions on the target

* `char` is 1-byte, `short` is 2-byte, `int` is 4-byte, `long` and `long long` are 8-byte. See the "LP64" data model at https://en.cppreference.com/w/cpp/language/types.html#Integral_types.
* All integral types are in little-endian.
* `char` is `unsigned char`. AArch64 is rather special in this aspect.
On x86 and RISCV `char` is `signed char`.
* In general, we prefer unsigned types over signed types, since signed overflow is undefined behavior.
We do not assume `-fwrapv`.
* Both object pointers and function pointers are 8-byte and can be cast to `uintptr_t` and back.
This is true of most POSIX-compliant systems.
However, insofar as casting `void *` directly to a function pointer is forbidden by the C standard (`gcc` will raise an error with `-pedantic -Werror`),
we cast `void *` first to `uintptr_t` and then to a function pointer.
* In most cases, we prefer the `(u)intX_t` types from `stdint.h`.
The traditional C integral types are used in Linux syscall interfaces, where we follow the Linux header.
* AArch64 SIMD instructions are supported, as well as the AES cryptographic extension.
However, the other cryptographic extensions including SHA2, SHA3 are not supported.
* In some cases we need to implement multiple instances of the same algorithm with different parameters.
This is particularly frequent in the cryptographic library.
While C macros can help avoiding code duplication, it is inconvenient for debugging because one cannot step through each line of a macro.
Therefore, we use the M4 preprocessor to generate source code for the different instances.

## Threading

Currently, we assume that the program consists of a fixed number of threads, known at compile-time.
This number is encoded in the macro `LIBC_MAX_THREAD_NUM` in `config.h`.

Upon creation, each thread (including the main thread) should call `set_thread_pointer()` to set the thread-local pointer.
This pointer points to a `tls_struct` structure, which contains:
* A thread ID;
* A malloc arena;
* An opaque structure for the vDSO random number generator.

The `tls_struct` structure should only contain thread-local data that needs to be globally accessible.

## Memory Allocation

Memory allocation is implemented in 3 layers.
The first layer is `mmap()`/`munmap()` which requests pages directly from the OS.
The second layer is a buddy allocator which requests "chunks" of 128 pages and allocates them in blocks of 64, 32, ..., 4, 2, or 1 pages.
The third layer is a small object allocator which requests blocks of 16 pages and divides them into slots of 32, 64, 96, ..., 512, 1024, 2048 bytes.
The `malloc()` function chooses one of these allocators based on request size.

Our memory allocator is thread-safe and lock-free.
Each thread is associated with its own memory allocator arena.
When a thread frees memory allocated by itself (the common case), the underlying allocator is called directly.
When a thread frees memory allocated by other threads, the region is put into a concurrent stack structure.
When the thread that originally made the allocation call `malloc()`/`free()` later,
it retrieves all pending regions from the concurrent stack and completes the `free()` operation.

## The cryptographic library

Currently, the following cryptographic functions are implemented:
* The SHA-3 hash function and its derivatives specified in SP 800-185.
* The AES symmetric cipher, including the AES-GCM authenticated cipher.
* The NTRU-LPrime key exchange mechanism.
* The SPHINCS+ signature algorithm.
* The UOV signature algorithm.
