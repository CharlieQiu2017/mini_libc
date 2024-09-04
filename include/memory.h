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

void * mmap (void *addr, size_t len, int prot, int flags, fd_t fd, ssize_t offset);

int munmap (void *addr, size_t len);

void * get_pages (uint64_t num);

void free_pages (void *ptr, uint64_t num);

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

void allocate_block6 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx);

void allocate_block5 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx);

void allocate_block4 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx);

void allocate_block3 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx);

void allocate_block2 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx);

void allocate_block1 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx);

void allocate_block0 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx);

void free_block6 (struct buddy_state *st, uint32_t block_idx);

void free_block5 (struct buddy_state *st, uint32_t block_idx);

void free_block4 (struct buddy_state *st, uint32_t block_idx);

void free_block3 (struct buddy_state *st, uint32_t block_idx);

void free_block2 (struct buddy_state *st, uint32_t block_idx);

void free_block1 (struct buddy_state *st, uint32_t block_idx);

void free_block0 (struct buddy_state *st, uint32_t block_idx);

struct class_area {
  struct buddy_state *buddy_area;
  struct class_area *prev_avail_area, *next_avail_area;
  uint64_t bitmap[32];
  uint64_t avail_num;
  char area[];
};

#define CLASS_AREA_HEADER_SIZE 288

void allocate_slot (uint64_t size, struct class_area **out_class_area, uint32_t *out_idx);

void free_slot (uint64_t size, struct class_area *area, uint32_t idx);

void * malloc (uint64_t size);

void free (void *ptr);

#endif
