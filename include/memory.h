#ifndef MEMORY_H
#define MEMORY_H

#include <io.h>

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

/* We implement three layers of memory allocator: mmap-alloc, buddy-alloc, and small-alloc.
   Each layer implements two functions:
   void * X_alloc (size_t len, void ** ctx_ptr);
   void X_free (void * ptr, void * ctx, size_t len);

   X_alloc attempts to allocate a block of memory of given length.
   For each layer, the len argument must belong to a predetermined set.
   For mmap-alloc it is any multiple of 4096 smaller than 1 << 48.
   For buddy-alloc it is 4096, 8192, ..., 262144.
   For small-alloc it is 32, 64, 96, ..., 512, 1024, 2048.
   It returns a pointer to the beginning of the allocated region, as well as a context pointer.

   Since the buddy allocator only provides 7 sizes, we provide buddy_alloc_0, ..., buddy_alloc_7 as specialized versions of buddy_alloc.
   buddy_alloc_N allocates (1 << N) pages of memory.
   We also provide buddy_free_N.

   X_free returns the allocated region.
   ctx must be the returned context pointer.
   len must be the length argument used to call alloc.
   ptr can be any address that is within the allocated region.
   For mmap_alloc, the context pointer and returned pointer are identical.
 */

void * mmap_alloc (size_t len, void ** ctx_ptr);

void mmap_free (__attribute__((unused)) void * ptr, void * ctx, size_t len);

struct buddy_state {
  uint8_t in_use;
  uint8_t avail_num[7];
  uint8_t bitmap6:2;
  uint8_t bitmap5:4;
  uint8_t bitmap4;
  uint16_t bitmap3;
  uint32_t bitmap2;
  uint64_t bitmap1;
  uint64_t bitmap0[2];
  struct buddy_state *next_avail_idx6, *prev_avail_idx6;
  struct buddy_state *next_avail_idx5, *prev_avail_idx5;
  struct buddy_state *next_avail_idx4, *prev_avail_idx4;
  struct buddy_state *next_avail_idx3, *prev_avail_idx3;
  struct buddy_state *next_avail_idx2, *prev_avail_idx2;
  struct buddy_state *next_avail_idx1, *prev_avail_idx1;
  struct buddy_state *next_avail_idx0, *prev_avail_idx0;
  struct buddy_state *next_empty_state, *prev_empty_state;
  void *chunk;
};

void * buddy_alloc_6 (void ** ctx_ptr);

void * buddy_alloc_5 (void ** ctx_ptr);

void * buddy_alloc_4 (void ** ctx_ptr);

void * buddy_alloc_3 (void ** ctx_ptr);

void * buddy_alloc_2 (void ** ctx_ptr);

void * buddy_alloc_1 (void ** ctx_ptr);

void * buddy_alloc_0 (void ** ctx_ptr);

void buddy_free_6 (void * ptr, void * ctx);

void buddy_free_5 (void * ptr, void * ctx);

void buddy_free_4 (void * ptr, void * ctx);

void buddy_free_3 (void * ptr, void * ctx);

void buddy_free_2 (void * ptr, void * ctx);

void buddy_free_1 (void * ptr, void * ctx);

void buddy_free_0 (void * ptr, void * ctx);

struct class_area {
  struct buddy_state *buddy_area;
  struct class_area *prev_avail_area, *next_avail_area;
  uint64_t bitmap[32];
  uint64_t avail_num;
  char area[];
};

#define CLASS_AREA_HEADER_SIZE (offsetof (struct class_area, area))
_Static_assert (CLASS_AREA_HEADER_SIZE % 16 == 0, "CLASS_AREA_HEADER_SIZE is not multiple of 16");

void * small_alloc (size_t len, void ** ctx_ptr);

void small_free (void * ptr, void * ctx, size_t len);

void * malloc (size_t len);

void free (void * ptr);

void free_set_clear (void);

#endif
