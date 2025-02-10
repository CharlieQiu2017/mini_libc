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

   Our "free-set" implementation follows the "non-linearizable queue"
   used in snmalloc (https://dl.acm.org/doi/pdf/10.1145/3315573.3329980).
   Each thread has a queue of its own. Each queue is a singly linked list
   represented by its head and tail. Only the owner may take elements
   out of this queue, while all other threads add elements (memory regions
   to be freed) into this queue.

   We use the first 8 bytes of the allocated region as the 'next' pointer
   of each queue element. When a thread calls free(), no one should be using
   these bytes anymore.

   To remove an element from the queue, the owner simply sets the head to head.next,
   and returns the previous head. To add an element to the queue, swap tail with
   the new element, and set orig_tail.next to the new element.

   There is a minor caveat with this concurrent queue. The last element of the
   queue cannot be taken out, because some other thread might be writing to the
   'next' field. We resolve this issue by introducing "placeholder" elements, one
   for each thread. When the owner of the thread wants to take out the last element
   of the queue, it inserts the placeholder into the queue. After other threads add
   new elements so that the placeholder is no longer the last element, we take it out.

   Each call to malloc() and free() will also call free_set_clear()
   which clears regions pending to be freed.
 */

static void * free_set_head[LIBC_MAX_THREAD_NUM];
static void * free_set_tail[LIBC_MAX_THREAD_NUM];
static void * free_set_placeholder[LIBC_MAX_THREAD_NUM];

void * malloc (size_t size) {
  uint16_t tid = get_thread_id ();

  if (!size) return NULL;
  if (size >= 1ull << 37) return NULL;

  /* Clear pending regions to be freed */
  free_set_clear ();

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

void * aligned_alloc (size_t alignment, size_t size) {
  uint16_t tid = get_thread_id ();
  if (alignment <= 16) return malloc (size);

  if (!size) return NULL;
  if (size >= 1ull << 37) return NULL;

  unsigned int alignment_log = __builtin_ctzll (alignment);
  if (alignment_log >= 36) return NULL;

  size += alignment;
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

  /* Find k such that (ptr + k) mod alignment = -16 */
  uintptr_t ptr_int = (uintptr_t) ptr;
  uintptr_t ptr_mod = (ptr_int & ((1 << alignment_log) - 1));
  /* Since ptr_int is a multiple of 16,
     we have ptr_mod <= alignment - 16
   */
  ptr_int += (alignment - 16 - ptr_mod);
  ptr = (void *) ptr_int;

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

static void free_set_insert (uint16_t tid, void * elem) {
  void * curr_tail;
  _Bool fail;

  __atomic_store_8 ((void **) elem, (uintptr_t) NULL, __ATOMIC_SEQ_CST);

  /* Swap tail with elem, and store original tail into curr_tail */
  asm volatile (
    "1:\n"
    "\tldxr %[load_reg], [%[tail_ptr_reg]]\n"
    "\tstxr %w[fail_reg], %[new_val_reg], [%[tail_ptr_reg]]\n"
    "\tcbnz %w[fail_reg], 1b\n"
    "\tdmb ish\n"
  : [load_reg] "=&r" (curr_tail), [fail_reg] "=&r" (fail)
  : [tail_ptr_reg] "r" (&free_set_tail[tid]), [new_val_reg] "r" (elem)
  : "memory"
  );

  __atomic_store_8 ((void **) curr_tail, (uintptr_t) elem, __ATOMIC_SEQ_CST);
}

void free_set_clear (void) {
  uint16_t tid = get_thread_id ();
  void * curr_head = free_set_head[tid], * next;

  while (true) {
    next = (void *) __atomic_load_8 ((void **) curr_head, __ATOMIC_SEQ_CST);
    if (next != NULL && curr_head != (void *) &free_set_placeholder[tid]) {
      /* If the current head is not the placeholder, free it */
      void * true_ptr = (void *) (((uintptr_t) curr_head) - 16);
      uint64_t size = __atomic_load_8 ((uint64_t *) true_ptr, __ATOMIC_SEQ_CST) & ((1ull << 37) - 1);
      void * ctx = (void *) __atomic_load_8 ((void **) (((uintptr_t) true_ptr) + 8), __ATOMIC_SEQ_CST);
      free_internal (true_ptr, size, ctx);
      curr_head = next;
    } else {
      /* If the last element is the placeholder, we have reached the end */
      if (curr_head == (void *) &free_set_placeholder[tid]) break;
      /* Otherwise, the placeholder is not in the queue, we insert it to take out the final element */
      free_set_insert (tid, &free_set_placeholder[tid]);
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

  if (alloc_tid == tid || size > 262144) {
    /* Allocations of at least 128 pages are passed directly to mmap, call munmap directly */
    free_internal (true_ptr, size, ctx);
  } else {
    /* Cross-thread deallocation */
    free_set_insert (alloc_tid, ptr);
  }

  free_set_clear ();
}
