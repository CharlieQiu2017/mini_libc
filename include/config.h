#ifndef LIBC_CONFIG_H
#define LIBC_CONFIG_H

/* Length of each cache line on this platform */
#define LIBC_CACHE_LINE_LEN 64

/* Maximum number of concurrent threads in this program */
#define LIBC_MAX_THREAD_NUM 1

/* Minimum number of chunks to keep always available for each allocator */
#define LIBC_KEEP_CHUNK_NUM 1

#endif
