#include <stdint.h>
#include <stdbool.h>
#include <memory.h>
#include <config.h>
#include <tls.h>

/* get_class
   Returns size class for a given requested allocation size.
 */
static inline uint64_t get_class (uint64_t size) {
  if (size <= 512) {
    return size + ((- size) & 32);
  } else if (size <= 262144) {
    return 1ull << (64 - __builtin_clzll (size - 1));
  } else {
    return size + ((- size) & 4096);
  }
}

/* When a region of memory is allocated through malloc(),
   the region is prepended with 16 bytes of metadata:
   Location -16 stores the "true size" of allocation,
   which is requested size + metadata + extra space for alignment.
   We shall assume the total allocation size is always smaller than
   128GiB, hence fitting into 37 bits. The remaining 27 bits are
   used to store the ID of the thread that made this allocation.
   Location -8 stores the context pointer returned by the allocator.
 */

/* When a region is allocated and freed by the same thread,
   the underlying allocator is called directly.
   Otherwise, it is put into a "free-set", waiting for the thread
   that originally made the allocation to free it.

   For each pair of threads, we use a singly linked lists to represent
   the "free-set". To insert an item, first load the current head of
   the list. Set the 'next' field of the item to the current head.
   If current head is NULL, simply set the current head to the item.
   If it is not NULL, do a compare-and-swap to set the current head
   to the item. The only possiblity where the CAS can fail is when
   the allocating thread exchanged the head with NULL. In that case,
   simply set 'next' of current item to NULL and set the current head
   to the item.

   The 'next' field of the linked list is stored at the first 8 bytes of
   the allocated region. After one calls free() no one should be using
   these bytes anymore.

   Each call to malloc() and free() will also call clear_free_set()
   which clears regions pending to be freed.
 */

static void * free_set_head[LIBC_MAX_THREAD_NUM][LIBC_MAX_THREAD_NUM];

void * malloc (size_t size) {
  uint16_t tid = get_thread_id ();

  if (!size) return NULL;
  if (size >= 1ull << 37) return NULL;

  /* Clear pending regions to be freed */
  clear_free_set ();

  size += 16; // Metadata
  uint64_t class_size = get_class (size);
  if (class_size >= 1ull << 37) return NULL;

  void * ptr = NULL, * ctx = NULL;
  if (class_size <= 2048) {

    ptr = small_alloc (class_size, &ctx);

  } else if (class_size <= 262144) {

    if (class_size == 4096) {
      ptr = buddy_alloc_0 (&ctx);
    } else if (class_size == 8192) {
      ptr = buddy_alloc_1 (&ctx);
    } else if (class_size == 16384) {
      ptr = buddy_alloc_2 (&ctx);
    } else if (class_size == 32768) {
      ptr = buddy_alloc_3 (&ctx);
    } else if (class_size == 65536) {
      ptr = buddy_alloc_4 (&ctx);
    } else if (class_size == 131072) {
      ptr = buddy_alloc_5 (&ctx);
    } else if (class_size == 262144) {
      ptr = buddy_alloc_6 (&ctx);
    }

  } else {

    ptr = mmap_alloc (class_size, &ctx);

  }

  if (ctx == NULL) return NULL;
  __atomic_store_8 ((uint64_t *) ptr, size | (((uint64_t) tid) << 38), __ATOMIC_SEQ_CST);
  __atomic_store_8 ((void **) (((uintptr_t) ptr) + 8), (uintptr_t) ctx, __ATOMIC_SEQ_CST);
  return (void *) (((uintptr_t) ptr) + 16);
}

static void free_internal (void * true_ptr, uint64_t size, void * ctx) {
  uint64_t class_size = get_class (size);

  if (class_size <= 2048) {

    small_free (true_ptr, ctx, class_size);

  } else if (class_size <= 262144) {

    if (class_size == 4096) {
      buddy_free_0 (true_ptr, ctx);
    } else if (class_size == 8192) {
      buddy_free_1 (true_ptr, ctx);
    } else if (class_size == 16384) {
      buddy_free_2 (true_ptr, ctx);
    } else if (class_size == 32768) {
      buddy_free_3 (true_ptr, ctx);
    } else if (class_size == 65536) {
      buddy_free_4 (true_ptr, ctx);
    } else if (class_size == 131072) {
      buddy_free_5 (true_ptr, ctx);
    } else if (class_size == 262144) {
      buddy_free_6 (true_ptr, ctx);
    }

  } else {

    mmap_free (true_ptr, ctx, class_size);

  }

}

void clear_free_set (void) {
  uint16_t tid = get_thread_id ();
  if (tid >= LIBC_MAX_THREAD_NUM) return; // Quench GCC warning

  for (uint16_t i = 0; i < LIBC_MAX_THREAD_NUM; ++i) {
    if (i == tid) continue;

    /* Swap free_set_head with NULL */
    void * curr_head;
    _Bool fail;
    __asm volatile (
      "1:\n"
      "\tldxr %[load_reg], [%[ptr_reg]]\n"
      "\tcbz %[load_reg], 1f\n"
      "\tstlxr %w[fail_reg], %[zero_reg], [%[ptr_reg]]\n"
      "\tcbnz %w[fail_reg], 1b\n"
      "\tb 2f\n"
      "\t1:\n"
      "\tclrex\n"
      "\t2:\n"
      "\tdmb ish\n"
      : [load_reg] "=&r" (curr_head), [fail_reg] "=&r" (fail)
      : [ptr_reg] "r" (&free_set_head[tid][i]), [zero_reg] "r" (0)
      : "memory"
    );

    while (curr_head != NULL) {
      void * next = (void *) __atomic_load_8 ((void **) curr_head, __ATOMIC_SEQ_CST);
      void * true_ptr = (void *) (((uintptr_t) curr_head) - 16);
      uint64_t size = __atomic_load_8 ((uint64_t *) true_ptr, __ATOMIC_SEQ_CST) & ((1ull << 37) - 1);
      void * ctx = (void *) __atomic_load_8 ((void **) (((uintptr_t) true_ptr) + 8), __ATOMIC_SEQ_CST);
      free_internal (true_ptr, size, ctx);
      curr_head = next;
    }
  }
}

void free (void * ptr) {
  if (ptr == NULL) return;

  void * true_ptr = (void *) (((uintptr_t) ptr) - 16);
  uint64_t size_and_tid = __atomic_load_8 ((uint64_t *) true_ptr, __ATOMIC_SEQ_CST);
  void * ctx = (void *) __atomic_load_8 ((void **) (((uintptr_t) true_ptr) + 8), __ATOMIC_SEQ_CST);
  uint64_t size = size_and_tid & ((1ull << 37) - 1);
  uint16_t alloc_tid = (uint16_t) (size_and_tid >> 38);
  uint16_t tid = get_thread_id ();

  /* Allocations of at least 128 pages are passed directly to mmap, call munmap directly */
  if (alloc_tid == tid || size > 262144) { free_internal (true_ptr, size, ctx); return; }

  /* Cross-thread deallocation */
  void * curr_head = (void *) __atomic_load_8 ((void **) &free_set_head[alloc_tid][tid], __ATOMIC_SEQ_CST);

  if (curr_head == NULL) {
    /* When curr_head is NULL, no other thread will modify it.
       Hence directly set it to the region to be freed.
     */

    __atomic_store_8 ((void **) ptr, (uintptr_t) NULL, __ATOMIC_SEQ_CST);
    __atomic_store_8 ((void **) &free_set_head[alloc_tid][tid], (uintptr_t) ptr, __ATOMIC_SEQ_CST);
  } else {
    /* When curr_head is non-NULL, the allocating thread may swap it with
       NULL at any given time. Hence use LL/SC to update the list head.
     */

    do {
      _Bool fail = 1;
      __atomic_store_8 ((void **) ptr, (uintptr_t) curr_head, __ATOMIC_SEQ_CST);
      __asm volatile (
	"1:\n"
	"\tldxr %[load_reg], [%[ptr_reg]]\n"
	"\tcmp %[load_reg], %[orig_val_reg]\n"
	"\tb.ne 1f\n"
	"\tstlxr %w[fail_reg], %[new_val_reg], [%[ptr_reg]]\n"
	"\tcbnz %w[fail_reg], 1b\n"
	"\tb 2f\n"
	"\t1:\n"
	"\tclrex\n"
	"\t2:\n"
	"\tdmb ish\n"
	: [load_reg] "=&r" (curr_head), [fail_reg] "=&r" (fail)
	: [ptr_reg] "r" (&free_set_head[alloc_tid][tid]), [orig_val_reg] "r" (curr_head), [new_val_reg] "r" (ptr)
	: "memory"
      );
      if (!fail) break;
    } while (true);
  }

  clear_free_set ();
}
