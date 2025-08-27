#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <memory.h>
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

   Let x be the 8-byte value at location -8, and let y be the 8-byte value at location -16.
   First take the lowest 3 bits of x and y, let s = (x & 0x07) | ((y & 0x07) << 3).

   If s == 63, then this allocation is made by mmap, and its size is (x & ~0x07).
   The beginning address of the allocation is (y & ~0x07).
   To free this allocation we call munmap directly.

   Otherwise, the allocation is made through either buddy-alloc or small-class-alloc.
   In both cases, the arena pointer is stored in (x & ~0x07), and the context pointer is stored in (y & ~0x07).

   If 1 <= s <= 18 then this is a small-class allocation, corresponding to the 18 size classes of small-class-alloc (ordered from small to large).
   If 19 <= s <= 25 then this is a buddy-alloc allocation, corresponding to the 7 size classes of buddy-alloc (ordered from small to large).
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

static inline struct malloc_arena_t * get_thread_malloc_arena (void) {
  return ((struct tls_struct *) get_thread_pointer ()) -> malloc_arena;
}

void malloc_init (void) {
  struct malloc_arena_t * arena = get_thread_malloc_arena ();
  __builtin_memset (arena, 0, sizeof (struct malloc_arena_t));
  arena->small_class_arena.buddy_arena = &arena->buddy_arena;
  arena->free_set_head = &arena->free_set_placeholder;
  arena->free_set_tail = &arena->free_set_placeholder;
}

void * malloc_with_arena (size_t size, struct malloc_arena_t * arena) {
  if (!size) return NULL;
  if (size >= 1ull << 37) return NULL;

  /* Clear pending regions to be freed */
  clear_free_set_of_arena (arena);

  size += 16; // Metadata
  uint64_t class_size = get_class (size);
  if (class_size >= 1ull << 37) return NULL;

  void * ptr = NULL, * ctx = NULL;
  if (class_size <= 2048) {

    ptr = small_alloc (class_size, &ctx, &arena->small_class_arena);

  } else if (class_size <= 262144) {

    if (class_size == 4096) {
      ptr = buddy_alloc_0 (&ctx, &arena->buddy_arena);
    } else if (class_size == 8192) {
      ptr = buddy_alloc_1 (&ctx, &arena->buddy_arena);
    } else if (class_size == 16384) {
      ptr = buddy_alloc_2 (&ctx, &arena->buddy_arena);
    } else if (class_size == 32768) {
      ptr = buddy_alloc_3 (&ctx, &arena->buddy_arena);
    } else if (class_size == 65536) {
      ptr = buddy_alloc_4 (&ctx, &arena->buddy_arena);
    } else if (class_size == 131072) {
      ptr = buddy_alloc_5 (&ctx, &arena->buddy_arena);
    } else if (class_size == 262144) {
      ptr = buddy_alloc_6 (&ctx, &arena->buddy_arena);
    }

  } else {

    ptr = mmap_alloc (class_size, &ctx);

  }

  if (ctx == NULL) return NULL;

  /* Encode metadata */
  uintptr_t x, y;
  if (class_size > 262144) {
    x = ((uintptr_t) class_size) | 0x07;
    y = ((uintptr_t) ctx) | 0x07;
  } else if (class_size >= 1024) {
    uint32_t s = __builtin_ctzll (class_size) + 7;
    x = ((uintptr_t) arena) | (s & 0x07);
    y = ((uintptr_t) ctx) | ((s >> 3) & 0x07);
  } else {
    uint32_t s = class_size / 32;
    x = ((uintptr_t) arena) | (s & 0x07);
    y = ((uintptr_t) ctx) | ((s >> 3) & 0x07);
  }

  __atomic_store_8 ((uintptr_t *) ptr, y, __ATOMIC_SEQ_CST);
  __atomic_store_8 ((uintptr_t *) (((uintptr_t) ptr) + 8), x, __ATOMIC_SEQ_CST);
  return (void *) (((uintptr_t) ptr) + 16);
}

void * aligned_alloc_with_arena (size_t alignment, size_t size, struct malloc_arena_t * arena) {
  if (alignment <= 16) return malloc_with_arena (size, arena);

  if (!size) return NULL;
  if (size >= 1ull << 37) return NULL;

  clear_free_set_of_arena (arena);

  unsigned int alignment_log = __builtin_ctzll (alignment);
  if (alignment_log >= 36) return NULL;

  size += alignment;
  uint64_t class_size = get_class (size);
  if (class_size >= 1ull << 37) return NULL;

  void * ptr = NULL, * ctx = NULL;
  if (class_size <= 2048) {

    ptr = small_alloc (class_size, &ctx, &arena->small_class_arena);

  } else if (class_size <= 262144) {

    if (class_size == 4096) {
      ptr = buddy_alloc_0 (&ctx, &arena->buddy_arena);
    } else if (class_size == 8192) {
      ptr = buddy_alloc_1 (&ctx, &arena->buddy_arena);
    } else if (class_size == 16384) {
      ptr = buddy_alloc_2 (&ctx, &arena->buddy_arena);
    } else if (class_size == 32768) {
      ptr = buddy_alloc_3 (&ctx, &arena->buddy_arena);
    } else if (class_size == 65536) {
      ptr = buddy_alloc_4 (&ctx, &arena->buddy_arena);
    } else if (class_size == 131072) {
      ptr = buddy_alloc_5 (&ctx, &arena->buddy_arena);
    } else if (class_size == 262144) {
      ptr = buddy_alloc_6 (&ctx, &arena->buddy_arena);
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

  uintptr_t x, y;
  if (class_size > 262144) {
    x = ((uintptr_t) class_size) | 0x07;
    y = ((uintptr_t) ctx) | 0x07;
  } else if (class_size >= 1024) {
    uint32_t s = __builtin_ctzll (class_size) + 7;
    x = ((uintptr_t) arena) | (s & 0x07);
    y = ((uintptr_t) ctx) | ((s >> 3) & 0x07);
  } else {
    uint32_t s = class_size / 32;
    x = ((uintptr_t) arena) | (s & 0x07);
    y = ((uintptr_t) ctx) | ((s >> 3) & 0x07);
  }

  __atomic_store_8 ((uintptr_t *) ptr, y, __ATOMIC_SEQ_CST);
  __atomic_store_8 ((uintptr_t *) (((uintptr_t) ptr) + 8), x, __ATOMIC_SEQ_CST);
  return (void *) (((uintptr_t) ptr) + 16);
}

static void free_with_arena_internal (void * ptr, uintptr_t x, uintptr_t y, struct malloc_arena_t * arena) {
  uint32_t s = (x & 0x07) | ((y & 0x07) << 3);

  if (s == 63) {
    uint64_t size = x & ~0x07;
    void * ctx = (void *)(y & ~0x07);
    mmap_free (ptr, ctx, size);
    return;
  }

  /* When this internal function is called, `arena` should be the same arena that made this allocation. */
  void * ctx = (void *)(y & ~0x07);

  if (s <= 16) {
    small_free (ptr, ctx, s * 32, arena);
  } else if (s == 17) {
    small_free (ptr, ctx, 1024, arena);
  } else if (s == 18) {
    small_free (ptr, ctx, 2048, arena);
  } else if (s == 19) {
    buddy_free_0 (ptr, ctx, arena);
  } else if (s == 20) {
    buddy_free_1 (ptr, ctx, arena);
  } else if (s == 21) {
    buddy_free_2 (ptr, ctx, arena);
  } else if (s == 22) {
    buddy_free_3 (ptr, ctx, arena);
  } else if (s == 23) {
    buddy_free_4 (ptr, ctx, arena);
  } else if (s == 24) {
    buddy_free_5 (ptr, ctx, arena);
  } else if (s == 25) {
    buddy_free_6 (ptr, ctx, arena);
  }
}

static void insert_into_free_set_of_arena (void * elem, struct malloc_arena_t * arena) {
  void * curr_tail;
  _Bool fail;

  __atomic_store_8 ((void **) elem, (uintptr_t) NULL, __ATOMIC_SEQ_CST);

  /* Swap tail with elem, and store original tail into curr_tail */
  __asm__ volatile (
    "1:\n"
    "\tldxr %[load_reg], [%[tail_ptr_reg]]\n"
    "\tstxr %w[fail_reg], %[new_val_reg], [%[tail_ptr_reg]]\n"
    "\tcbnz %w[fail_reg], 1b\n"
    "\tdmb ish\n"
  : [load_reg] "=&r" (curr_tail), [fail_reg] "=&r" (fail)
  : [tail_ptr_reg] "r" (&arena->free_set_tail), [new_val_reg] "r" (elem)
  : "memory"
  );

  __atomic_store_8 ((void **) curr_tail, (uintptr_t) elem, __ATOMIC_SEQ_CST);
}

void clear_free_set_of_arena (struct malloc_arena_t * arena) {
  void * curr_head = arena->free_set_head, * next;

  while (true) {
    next = (void *) __atomic_load_8 ((void **) curr_head, __ATOMIC_SEQ_CST);
    if (next != NULL && curr_head != (void *) &arena->free_set_placeholder) {
      /* If the current head is not the placeholder, free it */
      /* curr_head is an allocation previously made by this thread.
	 Therefore, the following to loads do not need to be atomic.
       */
      uintptr_t x = *(uintptr_t *)(((uintptr_t) curr_head) - 8);
      uintptr_t y = *(uintptr_t *)(((uintptr_t) curr_head) - 16);
      free_with_arena_internal (curr_head, x, y, arena);
      curr_head = next;
    } else {
      /* If the last element is the placeholder, we have reached the end */
      if (curr_head == (void *) &arena->free_set_placeholder) break;
      /* Otherwise, the placeholder is not in the queue, we insert it to take out the final element */
      insert_into_free_set_of_arena (&arena->free_set_placeholder, arena);
    }
  }
}

void free_with_arena (void * ptr, struct malloc_arena_t * arena) {
  if (ptr == NULL) return;

  /* ptr need not be previously allocated by this thread.
     Therefore, the following two loads need to be atomic.
   */
  uintptr_t x = __atomic_load_8 ((uintptr_t *)(((uintptr_t) ptr) - 8), __ATOMIC_SEQ_CST);
  uintptr_t y = __atomic_load_8 ((uintptr_t *)(((uintptr_t) ptr) - 16), __ATOMIC_SEQ_CST);
  uint32_t s = (x & 0x07) | ((y & 0x07) << 3);

  /* If allocation is made by mmap, free directly */
  if (s == 63) {
    free_with_arena_internal (ptr, x, y, arena);
    clear_free_set_of_arena (arena);
    return;
  }

  void * alloc_arena = (void *)(x & ~0x07);

  if (alloc_arena == arena) {
    free_with_arena_internal (ptr, x, y, arena);
  } else {
    /* Cross-thread deallocation */
    insert_into_free_set_of_arena (ptr, alloc_arena);
  }

  clear_free_set_of_arena (arena);
}

void * malloc (size_t len) {
  return malloc_with_arena (len, get_thread_malloc_arena ());
}

void * aligned_alloc (size_t alignment, size_t size) {
  return aligned_alloc_with_arena (alignment, size, get_thread_malloc_arena ());
}

void free (void * ptr) {
  free_with_arena (ptr, get_thread_malloc_arena ());
}

void clear_free_set (void) {
  clear_free_set_of_arena (get_thread_malloc_arena ());
}
