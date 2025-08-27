#include <stdint.h>
#include <memory.h>

/* The small-class allocator manages small allocations (smaller than 2048 bytes).
   We request blocks of size 65536 (16 pages) from the buddy allocator, and divide them into small slots in multiples of 32 bytes.
   Small classes are 32, 64, 96, ..., 512, 1024, 2048
 */

struct small_class_block {
  void *buddy_ctx;
  struct small_class_block *prev_avail_block, *next_avail_block;
  uint64_t bitmap[32];
  uint64_t avail_num;
  char block[];
};

#define CLASS_BLOCK_HEADER_SIZE (offsetof (struct small_class_block, block))
_Static_assert (CLASS_BLOCK_HEADER_SIZE % 16 == 0, "CLASS_BLOCK_HEADER_SIZE is not multiple of 16");

#define SMALL_CLASS_IDX_SLOT(block_, idx, size) ((void *) ((&(block_)->block[0]) + (idx) * (size)))
#define SMALL_CLASS_SLOT_IDX(block_, slot, size) ((((uintptr_t) (slot)) - ((uintptr_t) (&(block_)->block[0]))) / (size))

/* allocate_class_block
   Allocate new class block using buddy allocator.
   size must be 32, 64, 96, ..., 512, or 1024, 2048.
   The new block is added to the corresponding list.
   Upon failure, the corresponding list is unmodified.
 */
static void allocate_class_block (uint64_t size, struct small_class_arena_t * arena) {
  struct small_class_block * ptr;
  void * buddy_ctx;

  struct small_class_block ** list_head;
  if (size <= 512) {
    uint32_t cls_idx = size / 32 - 1;
    list_head = &(arena->small_class_avail_lists[cls_idx]);
  } else if (size == 1024) {
    list_head = &arena->class1024_avail_list;
  } else {
    list_head = &arena->class2048_avail_list;
  }

  ptr = buddy_alloc_4 (&buddy_ctx, arena->buddy_arena);
  if (ptr == NULL) return;
  ptr->buddy_ctx = buddy_ctx;
  ptr->prev_avail_block = NULL;

  ptr->next_avail_block = *list_head;
  if (*list_head != NULL) (*list_head)->prev_avail_block = ptr;
  *list_head = ptr;

  uint32_t avail_num = (65536 - CLASS_BLOCK_HEADER_SIZE) / size;
  ptr->avail_num = avail_num;
  for (uint32_t i = 0; i < avail_num / 64; ++i) ptr->bitmap[i] = ~ 0ull;
  uint32_t avail_num_rem = avail_num % 64;
  ptr->bitmap[avail_num / 64] = (1ull << avail_num_rem) - 1;
}

void * small_alloc (size_t len, void ** ctx_ptr, void * arena_vp) {
  struct small_class_block ** out_class_block = (struct small_class_block **) ctx_ptr;
  struct small_class_arena_t * arena = (struct small_class_arena_t *) arena_vp;

  struct small_class_block ** list_head;
  if (len <= 512) {
    uint32_t cls_idx = len / 32 - 1;
    list_head = &(arena->small_class_avail_lists[cls_idx]);
  } else if (len == 1024) {
    list_head = &arena->class1024_avail_list;
  } else {
    list_head = &arena->class2048_avail_list;
  }

  if (*list_head == NULL) allocate_class_block (len, arena);
  if (*list_head == NULL) {
    *out_class_block = NULL;
    return NULL;
  }

  struct small_class_block * block = *list_head;
  for (uint32_t i = 0; i < 32; ++i) {
    if (block->bitmap[i] != 0) {
      uint32_t idx = __builtin_ctzll (block->bitmap[i]);
      block->bitmap[i] &= ~ (1ull << idx);
      *out_class_block = block;
      block->avail_num--;
      if (block->avail_num == 0) {
	if (block->next_avail_block != NULL) block->next_avail_block->prev_avail_block = block->prev_avail_block;
	*list_head = block->next_avail_block;
	block->next_avail_block = NULL;
      }
      return SMALL_CLASS_IDX_SLOT (block, 64 * i + idx, len);
    }
  }

  /* Should not reach here */
  *out_class_block = NULL;
  return NULL;
}

void small_free (void * ptr, void * ctx, size_t len, void * arena_vp) {
  struct small_class_block * block = (struct small_class_block *) ctx;
  struct small_class_arena_t * arena = (struct small_class_arena_t *) arena_vp;
  uint32_t idx = SMALL_CLASS_SLOT_IDX (block, ptr, len);

  struct small_class_block ** list_head;
  if (len <= 512) {
    uint32_t cls_idx = len / 32 - 1;
    list_head = &(arena->small_class_avail_lists[cls_idx]);
  } else if (len == 1024) {
    list_head = &arena->class1024_avail_list;
  } else {
    list_head = &arena->class2048_avail_list;
  }

  block->bitmap[idx / 64] |= (1ull << (idx % 64));
  block->avail_num++;
  if (block->avail_num == 1) {
    block->next_avail_block = *list_head;
    if (*list_head != NULL) (*list_head)->prev_avail_block = block;
    *list_head = block;
  } else if (block->avail_num == (65536 - CLASS_BLOCK_HEADER_SIZE) / len) {
    /* If the current block is the only block of this class with empty slots, do not free it,
       since we anticipate there will be more allocations later.
     */
    if (block->prev_avail_block != NULL || block->next_avail_block != NULL) {
      if (block->prev_avail_block != NULL) block->prev_avail_block->next_avail_block = block->next_avail_block;
      if (block->next_avail_block != NULL) block->next_avail_block->prev_avail_block = block->prev_avail_block;
      if (*list_head == block) *list_head = block->next_avail_block;
      buddy_free_4 (block->buddy_ctx, block, arena->buddy_arena);
    }
  }
}
