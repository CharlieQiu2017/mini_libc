#ifndef MEMORY_H
#define MEMORY_H

#include <io.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4
#define PROT_SEM 0x8
#define PROT_NONE 0x0
#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02
#define MAP_SHARED_VALIDATE 0x03
#define MAP_FIXED 0x10
#define MAP_ANONYMOUS 0x20
#define MAP_FIXED_NOREPLACE 0x100000

void * mmap (void * addr, size_t len, int prot, int flags, fd_t fd, ssize_t offset);

int munmap (void * addr, size_t len);

/* We implement three layers of memory allocator: mmap-alloc, buddy-alloc, and small-class-alloc.
   Each layer implements two functions:
   void * X_alloc (size_t len, void ** ctx_ptr, void * arena);
   void X_free (void * ptr, void * ctx, size_t len, void * arena);

   arena is a pointer to a per-thread data structure that manages all allocations made by this thread.
   However, since mmap-alloc is stateless, its functions do not have this st_ptr argument.

   X_alloc attempts to allocate a block of memory of given length.
   For each layer, the len argument must belong to a predetermined set.
   For mmap-alloc it is any multiple of 4096 smaller than 1 << 48.
   For buddy-alloc it is 4096, 8192, ..., 262144.
   For small-class-alloc it is 32, 64, 96, ..., 512, 1024, 2048.
   It returns a pointer to the beginning of the allocated region, as well as a context pointer.

   Since the buddy allocator only provides 7 sizes, we provide buddy_alloc_0, ..., buddy_alloc_7 as specialized versions of buddy_alloc.
   buddy_alloc_N allocates (1 << N) pages of memory.
   We also provide buddy_free_N.

   X_free returns the allocated region.
   ctx must be the context pointer that is returned during allocation.
   len must be the length argument used to call alloc.
   arena must be the state pointer used to call alloc.
   ptr can be any address that is within the allocated region.
   For mmap_alloc, the context pointer and returned pointer are identical.
 */

/* Interfaces of mmap-alloc */

void * mmap_alloc (size_t len, void ** ctx_ptr);

void mmap_free (__attribute__((unused)) void * ptr, void * ctx, size_t len);

/* The buddy-alloc arena structure */

struct buddy_chunk_state;
struct buddy_chunk_state_group;

struct buddy_arena_t {
  struct buddy_chunk_state_group *group_list_head;
  struct buddy_chunk_state *empty_list_head;
  struct buddy_chunk_state *avail6_list_head;
  struct buddy_chunk_state *avail5_list_head;
  struct buddy_chunk_state *avail4_list_head;
  struct buddy_chunk_state *avail3_list_head;
  struct buddy_chunk_state *avail2_list_head;
  struct buddy_chunk_state *avail1_list_head;
  struct buddy_chunk_state *avail0_list_head;
  uint32_t chunk_num;
};

/* Interfaces of buddy-alloc */

void * buddy_alloc_6 (void ** ctx_ptr, void * arena);

void * buddy_alloc_5 (void ** ctx_ptr, void * arena);

void * buddy_alloc_4 (void ** ctx_ptr, void * arena);

void * buddy_alloc_3 (void ** ctx_ptr, void * arena);

void * buddy_alloc_2 (void ** ctx_ptr, void * arena);

void * buddy_alloc_1 (void ** ctx_ptr, void * arena);

void * buddy_alloc_0 (void ** ctx_ptr, void * arena);

void buddy_free_6 (void * ptr, void * ctx, void * arena);

void buddy_free_5 (void * ptr, void * ctx, void * arena);

void buddy_free_4 (void * ptr, void * ctx, void * arena);

void buddy_free_3 (void * ptr, void * ctx, void * arena);

void buddy_free_2 (void * ptr, void * ctx, void * arena);

void buddy_free_1 (void * ptr, void * ctx, void * arena);

void buddy_free_0 (void * ptr, void * ctx, void * arena);

/* The small-class-alloc arena structure */

struct small_class_block;

struct small_class_arena_t {
  struct buddy_arena_t *buddy_arena;
  struct small_class_block *small_class_avail_lists[16];
  struct small_class_block *class1024_avail_list;
  struct small_class_block *class2048_avail_list;
};

void * small_alloc (size_t len, void ** ctx_ptr, void * arena);

void small_free (void * ptr, void * ctx, size_t len, void * arena);

/* Per-thread malloc data structure */

struct malloc_arena_t {
  struct buddy_arena_t buddy_arena;
  struct small_class_arena_t small_class_arena;
  void * free_set_head;
  void * free_set_tail;
  void * free_set_placeholder;
};

void * malloc_with_arena (size_t size, struct malloc_arena_t * arena);

void * aligned_alloc_with_arena (size_t alignment, size_t size, struct malloc_arena_t * arena);

void free_with_arena (void * ptr, struct malloc_arena_t * arena);

void clear_free_set_of_arena (struct malloc_arena_t * arena);

/* Must be called by each thread upon initialization */
void malloc_init (void);

void * malloc (size_t len);

void * aligned_alloc (size_t alignment, size_t size);

void free (void * ptr);

void clear_free_set (void);

#ifdef __cplusplus
}
#endif

#endif
