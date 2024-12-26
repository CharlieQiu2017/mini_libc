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

## Threading

We assume that the program consists of a fixed number of threads, known at compile-time.
This number is encoded in the macro `LIBC_MAX_THREAD_NUM` in `config.h`.

Upon creation, each thread (including the main thread) should call `set_thread_pointer()` to set the thread-local pointer.
This pointer points to a `tls_struct` structure, which currently only contains a thread ID number.
The `tls_struct` structure should only contain thread-local data that needs to be globally accessible.
Thread-local data that is only accessed by a single module should be defined within that module.

## Memory Allocation

Memory allocation is implemented in 3 layers.
The first layer is `mmap()`/`munmap()` which requests pages directly from the OS.
The second layer is a buddy allocator which requests "chunks" of 128 pages and allocates them in blocks of 64, 32, ..., 4, 2, or 1 pages.
The third layer is a small object allocator which requests blocks of 16 pages and divides them into slots of 32, 64, 96, ..., 512, 1024, 2048 bytes.
The `malloc()` function chooses one of these allocators based on request size.

Our memory allocator is thread-safe and lock-free.
Each thread is associated with its own memory allocator.
When a thread frees memory allocated by itself (the common case), the underlying allocator is called directly.
When a thread frees memory allocated by other threads, the region is put into a concurrent stack structure.
When the thread that originally made the allocation call `malloc()`/`free()` later,
it retrieves all pending regions from the concurrent stack and completes the `free()` operation.