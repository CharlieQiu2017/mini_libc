#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <tls.h>

/* get_class
   Returns size class for a given requested allocation size.
 */
static inline uint64_t get_class (uint64_t size) {
  if (size <= 512) {
    return size + ((- size) & 31);
  } else if (size <= 262144) {
    return 1ull << (64 - __builtin_clzll (size - 1));
  } else {
    return size + ((- size) & 4095);
  }
}

/* When a region of memory is allocated through malloc(),
   the region is prepended with 16 bytes of metadata:

   Let x be the 8-byte value at location -8, and let y be the 8-byte value at location -16.
   The pointer to the arena that made this allocation is (x & ~0x4000000000000007ull).
   The context pointer returned by the allocation function is (y & ~0x07ull).

   The other bits store metadata about the allocation. Let s = (x & 0x07) | ((y & 0x03) << 3).
   If s == 31, then this allocation is made by mmap-alloc.
   If 1 <= s <= 18 then this is a small-class allocation, corresponding to the 18 size classes of small-class-alloc (ordered from small to large).
   If 19 <= s <= 25 then this is a buddy-alloc allocation, corresponding to the 7 size classes of buddy-alloc (ordered from small to large).

   The lowest 3rd bit of y (which is equal to y & 0x04) is used to indicate whether the allocation was made through aligned_tagged_alloc().
   Bit 62 of x (equal to x & 0x4000000000000000) is used to indicate whether this allocation is in the middle of being freed.
   These marks exist to assist debug tools.

   On 64-bit Linux, user-space virtual memory address cannot go beyond TASK_SIZE ((1ull << 47) - 1).
   It follows that neither x nor y can have its bit 63 set.
   When an allocation is made, if metadata is not placed at the beginning of the allocation,
   the first 8 bytes of the allocation will have its bit 63 set, and indicate the offset where the metadata begins.
   This is to assist debug tools that want to scan the entire heap and check on every active allocation.
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

   There is a very subtle problem with this concurrent queue. The last element of the
   queue cannot be taken out, because some other thread might be writing to the
   'next' field. We resolve this issue by introducing "placeholder" elements, one
   for each thread. When the owner of the thread wants to take out the last element
   of the queue, it inserts the placeholder into the queue. After other threads add
   new elements so that the placeholder is no longer the last element, we take it out.

   It is tempting to think that after the owner inserts its placeholder element,
   it can immediately take out residual elements in the queue. This is wrong, because
   the queue is non-linearizable: if there are concurrent callers to free(), it is
   NOT guaranteed that the owner will see its placeholder in the queue immediately.
   After we insert the placeholder, we check head.next. If we still have head.next ==
   nullptr, this means there are concurrent callers to free(), and we handle those
   elements later.

   Each call to malloc() and free() will also call free_set_clear()
   which clears regions pending to be freed.
 */

#define TASK_SIZE ((1ull << 47) - 1)

static inline struct malloc_arena_t * get_thread_malloc_arena (void) {
  return ((struct tls_struct *) get_thread_pointer ()) -> malloc_arena;
}

void malloc_init (void) {
  struct malloc_arena_t * arena = get_thread_malloc_arena ();
  __builtin_memset (arena, 0, sizeof (struct malloc_arena_t));
  arena->buddy_arena.mmap_arena = &arena->mmap_arena;
  arena->small_class_arena.buddy_arena = &arena->buddy_arena;
  arena->free_set_head = &arena->free_set_placeholder;
  arena->free_set_tail = &arena->free_set_placeholder;
}

void * malloc_with_arena (size_t size, struct malloc_arena_t * arena) {
  if (! size) return NULL;
  if (size > TASK_SIZE) return NULL;

  size += 16; // Metadata
  uint64_t class_size = get_class (size);
  if (class_size > TASK_SIZE) return NULL;

  /* Clear pending regions to be freed */
  clear_free_set_of_arena (arena);

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

    ptr = mmap_alloc (class_size, &ctx, &arena->mmap_arena);

  }

  if (ctx == NULL) return NULL;

  /* Encode metadata */
  uintptr_t x, y;
  if (class_size > 262144) {
    x = ((uintptr_t) arena) | 0x07;
    y = ((uintptr_t) ctx) | 0x03;
  } else if (class_size >= 1024) {
    uint32_t s = __builtin_ctzll (class_size) + 7;
    x = ((uintptr_t) arena) | (s & 0x07);
    y = ((uintptr_t) ctx) | ((s >> 3) & 0x03);
  } else {
    uint32_t s = class_size / 32;
    x = ((uintptr_t) arena) | (s & 0x07);
    y = ((uintptr_t) ctx) | ((s >> 3) & 0x03);
  }

  __atomic_store_8 ((uintptr_t *) ptr, y, __ATOMIC_SEQ_CST);
  __atomic_store_8 ((uintptr_t *) (((uintptr_t) ptr) + 8), x, __ATOMIC_SEQ_CST);
  return (void *) (((uintptr_t) ptr) + 16);
}

void * aligned_alloc_with_arena (size_t alignment, size_t size, struct malloc_arena_t * arena) {
  if (alignment <= 16) return malloc_with_arena (size, arena);

  if (! size) return NULL;
  if (size > TASK_SIZE) return NULL;

  unsigned int alignment_log = __builtin_ctzll (alignment);
  if (alignment_log >= 47) return NULL;

  size += alignment;
  uint64_t class_size = get_class (size);
  if (class_size > TASK_SIZE) return NULL;

  clear_free_set_of_arena (arena);

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

    ptr = mmap_alloc (class_size, &ctx, &arena->mmap_arena);

  }

  if (ctx == NULL) return NULL;

  /* Find k such that (ptr + k) mod alignment = -16 */
  uintptr_t ptr_int = (uintptr_t) ptr;
  uintptr_t ptr_mod = (ptr_int & (alignment - 1));
  /* Since both ptr_int and alignment are multiples of 16, we have ptr_mod <= alignment - 16 */
  size_t shift = alignment - 16 - ptr_mod;

  /* If shift != 0, this means metadata is not placed at beginning of allocation, and we mark the shift */
  if (shift != 0) {
    size_t shift_mark = 0x8000000000000000ull | shift;
    /* This mark exists solely for debugging purposes and not read by any code. This store can be relaxed */
    __atomic_store_8 ((size_t *) ptr, shift_mark, __ATOMIC_RELAXED);
  }

  ptr_int += shift;
  ptr = (void *) ptr_int;

  uintptr_t x, y;
  if (class_size > 262144) {
    x = ((uintptr_t) arena) | 0x07;
    y = ((uintptr_t) ctx) | 0x03;
  } else if (class_size >= 1024) {
    uint32_t s = __builtin_ctzll (class_size) + 7;
    x = ((uintptr_t) arena) | (s & 0x07);
    y = ((uintptr_t) ctx) | ((s >> 3) & 0x03);
  } else {
    uint32_t s = class_size / 32;
    x = ((uintptr_t) arena) | (s & 0x07);
    y = ((uintptr_t) ctx) | ((s >> 3) & 0x03);
  }

  __atomic_store_8 ((uintptr_t *) ptr, y, __ATOMIC_SEQ_CST);
  __atomic_store_8 ((uintptr_t *) (((uintptr_t) ptr) + 8), x, __ATOMIC_SEQ_CST);
  return (void *) (((uintptr_t) ptr) + 16);
}

void * aligned_tagged_alloc_with_arena (size_t alignment, size_t size, struct malloc_arena_t * arena) {
  if (! size) return NULL;
  if (size > TASK_SIZE) return NULL;

  if (alignment <= 16) {
    void * ptr = malloc_with_arena (size + 16, arena);
    if (ptr == NULL) return NULL;
    /* Mark this allocation as tagged.
       This store can be relaxed. See comments in free_with_arena().
     */
    uintptr_t y = __atomic_load_8 ((uintptr_t *) (((uintptr_t) ptr) - 16), __ATOMIC_RELAXED);
    __atomic_store_8 ((uint64_t *)(((uintptr_t) ptr) - 16), y | 0x04, __ATOMIC_RELAXED);
    return ptr;
  }

  unsigned int alignment_log = __builtin_ctzll (alignment);
  if (alignment_log >= 47) return NULL;

  size += (alignment + 16);
  uint64_t class_size = get_class (size);
  if (class_size > TASK_SIZE) return NULL;

  clear_free_set_of_arena (arena);

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

    ptr = mmap_alloc (class_size, &ctx, &arena->mmap_arena);

  }

  if (ctx == NULL) return NULL;

  /* Find k such that (ptr + k) mod alignment = -32 */
  uintptr_t ptr_int = (uintptr_t) ptr;
  uintptr_t ptr_mod = (ptr_int & (alignment - 1));
  /* The following code strictly relies on unsigned underflow and alignment being a power of two.
     If ptr_mod <= alignment - 32, then alignment - 32 - ptr_mod < alignment, so ptr_int == alignment - 32 - ptr_mod.
     If ptr_mod > alignment - 32 (the only possibility is ptr_mod == alignment - 16),
     then alignment - 32 - ptr_mod == -16 == 0xFFFFFFFFFFFFFFF0, and taking bitwise AND with (alignment - 1) gives alignment - 16.
   */
  size_t shift = (alignment - 32 - ptr_mod) & (alignment - 1);
  if (shift != 0) {
    size_t shift_mark = 0x8000000000000000ull | shift;
    __atomic_store_8 ((size_t *) ptr, shift_mark, __ATOMIC_RELAXED);
  }

  ptr_int += shift;
  ptr = (void *) ptr_int;

  /* Bit 2 of y is set since this is tagged allocation */
  uintptr_t x, y;
  if (class_size > 262144) {
    x = ((uintptr_t) arena) | 0x07;
    y = ((uintptr_t) ctx) | 0x07;
  } else if (class_size >= 1024) {
    uint32_t s = __builtin_ctzll (class_size) + 7;
    x = ((uintptr_t) arena) | (s & 0x07);
    y = ((uintptr_t) ctx) | ((s >> 3) & 0x03) | 0x04;
  } else {
    uint32_t s = class_size / 32;
    x = ((uintptr_t) arena) | (s & 0x07);
    y = ((uintptr_t) ctx) | ((s >> 3) & 0x03) | 0x04;
  }

  __atomic_store_8 ((uintptr_t *) ptr, y, __ATOMIC_SEQ_CST);
  __atomic_store_8 ((uintptr_t *) (((uintptr_t) ptr) + 8), x, __ATOMIC_SEQ_CST);
  return (void *) (((uintptr_t) ptr) + 16);
}

static void free_with_arena_internal (void * ptr, uintptr_t x, uintptr_t y, struct malloc_arena_t * arena) {
  uint32_t s = (x & 0x07) | ((y & 0x03) << 3);

  /* When this internal function is called, `arena` should be the same arena that made this allocation. */
  void * ctx = (void *)(y & ~0x07ull);

  if (s == 31) {
    mmap_free (ptr, ctx, &arena->mmap_arena);
  } else if (s <= 16) {
    small_free (ptr, ctx, s * 32, &arena->small_class_arena);
  } else if (s == 17) {
    small_free (ptr, ctx, 1024, &arena->small_class_arena);
  } else if (s == 18) {
    small_free (ptr, ctx, 2048, &arena->small_class_arena);
  } else if (s == 19) {
    buddy_free_0 (ptr, ctx, &arena->buddy_arena);
  } else if (s == 20) {
    buddy_free_1 (ptr, ctx, &arena->buddy_arena);
  } else if (s == 21) {
    buddy_free_2 (ptr, ctx, &arena->buddy_arena);
  } else if (s == 22) {
    buddy_free_3 (ptr, ctx, &arena->buddy_arena);
  } else if (s == 23) {
    buddy_free_4 (ptr, ctx, &arena->buddy_arena);
  } else if (s == 24) {
    buddy_free_5 (ptr, ctx, &arena->buddy_arena);
  } else if (s == 25) {
    buddy_free_6 (ptr, ctx, &arena->buddy_arena);
  }
}

static void insert_into_free_set_of_arena (void * elem, struct malloc_arena_t * arena) {
  void * curr_tail;
  _Bool fail;

  __atomic_store_8 ((void **) elem, (uintptr_t) NULL, __ATOMIC_SEQ_CST);

  /* Swap tail with elem, and store original tail into curr_tail */
  __asm__ volatile (
    "1:\n\t"
    "ldxr %[load_reg], [%[tail_ptr_reg]]\n\t"
    "stxr %w[fail_reg], %[new_val_reg], [%[tail_ptr_reg]]\n\t"
    "cbnz %w[fail_reg], 1b\n\t"
    "dmb ish"
  : [load_reg] "=&r" (curr_tail), [fail_reg] "=&r" (fail)
  : [tail_ptr_reg] "r" (&arena->free_set_tail), [new_val_reg] "r" (elem)
  : "memory"
  );

  __atomic_store_8 ((void **) curr_tail, (uintptr_t) elem, __ATOMIC_SEQ_CST);
}

/* This function cannot loop indefinitely.
   The reason is that at any given point, the total number of unreturned allocations made from an arena must be finite.
   If the owner of the arena is stuck in this loop it will not make new allocations.
   Each iteration of the loop is guaranteed to either exit or free a allocation.
   When all allocations made from an arena have been returned, this loop must exit.
 */
void clear_free_set_of_arena (struct malloc_arena_t * arena) {
  void * curr_head = arena->free_set_head, * next;

  /* Assume that the placeholder element has been added to the queue.
     This invariant will be restored at the end.
   */
  _Bool placeholder_in_queue = 1;

  while (1) {
    next = (void *) __atomic_load_8 ((void **) curr_head, __ATOMIC_SEQ_CST);
    if (next != NULL) {
      if (curr_head != (void *) &arena->free_set_placeholder) {
        /* If the current head is not the placeholder, free it */
        /* curr_head is an allocation previously made by this thread.
	   Therefore, the following two loads only need to be relaxed.
	   See also the comments in free_with_arena().
         */
        uintptr_t x = __atomic_load_8 ((uintptr_t *)(((uintptr_t) curr_head) - 8), __ATOMIC_RELAXED);
        uintptr_t y = __atomic_load_8 ((uintptr_t *)(((uintptr_t) curr_head) - 16), __ATOMIC_RELAXED);
        free_with_arena_internal (curr_head, x, y, arena);
        curr_head = next;
      } else {
        /* Otherwise, we have popped the placeholder.
           Set placeholder_in_queue to false.
         */
        placeholder_in_queue = 0;
        curr_head = next;
      }
    } else {
      /* If we reach this point, then next == NULL, and there are three possibilities:
         1. Current head is the placeholder. Then we must have placeholder_in_queue == true.
            In this case we have reached the end, and we return.
         2. Current head is not the placeholder, but placeholder_in_queue == false.
            In this case we re-insert the placeholder and try again.
         3. Current head is not the placeholder, and placeholder_in_queue == true.
            In this case there are concurrent free() callers. We have to return
            and handle the residual elements later.
       */
      if (curr_head == (void *) &arena->free_set_placeholder) break;
      if (placeholder_in_queue) break;

      insert_into_free_set_of_arena (&arena->free_set_placeholder, arena);
      placeholder_in_queue = 1;
    }
  }

  /* There are only two ways to exit the above loop, namely case 1 and 3 in the else branch.
     In either case we have placeholder_in_queue == true, so the assumption at beginning is maintained.
   */

  arena->free_set_head = curr_head;
}

void free_with_arena (void * ptr, struct malloc_arena_t * arena) {
  if (ptr == NULL) return;

  /* ptr need not be previously allocated by this thread.
     Therefore, the following two loads need to be atomic.
     Assuming no double-free, the x value read here should not have the free mark.
   */
  uintptr_t x = __atomic_load_8 ((uintptr_t *)(((uintptr_t) ptr) - 8), __ATOMIC_SEQ_CST);
  uintptr_t y = __atomic_load_8 ((uintptr_t *)(((uintptr_t) ptr) - 16), __ATOMIC_SEQ_CST);

  /* Mark ptr as being freed. This mark exists solely for debugging purpose. */
  /* This store does not need to be atomic.
     When the allocator thread reads this value, it will either see the old value or the new value.
     Since the only difference is in this free mark, it does not matter if it reads the old value or the new value.
   */
  __atomic_store_8 ((uintptr_t *)(((uintptr_t) ptr) - 8), x | 0x4000000000000000ull, __ATOMIC_RELAXED);

  /* x should not have the free mark (see above). Therefore, the arena pointer is simply x & ~0x07ull */
  void * alloc_arena = (void *)(x & ~0x07ull);

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

void * aligned_tagged_alloc (size_t alignment, size_t size) {
  return aligned_tagged_alloc_with_arena (alignment, size, get_thread_malloc_arena ());
}

void free (void * ptr) {
  free_with_arena (ptr, get_thread_malloc_arena ());
}

void clear_free_set (void) {
  clear_free_set_of_arena (get_thread_malloc_arena ());
}
