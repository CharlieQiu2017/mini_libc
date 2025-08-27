#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <config.h>

/* The buddy allocator manages memory chunks of size 512KiB (128 pages).
   It allocates memory blocks of sizes 1, 2, 4, ..., 64 pages.

   Initially, each chunk is considered as a single block of 128 pages.
   Blocks of 128 pages can be divided into two blocks of 64 pages.
   These blocks can be further divided, until we reach blocks of 1 page.

   Within each chunk, each block can be identified by the pair (order, idx).
   The length of the block is (1 << order) pages, and the index of the first page of the block is (length * idx).
   Hence 0 <= idx < 128.

   If all blocks within a chunk are freed, the chunk is returned to the OS.
   We also keep a small number of chunks always available, to avoid frequent syscalls.

   The state of each chunk is recorded using a `struct buddy_chunk_state`.
   This struct contains the following fields:
   uint8_t in_use; // Whether this buddy_chunk_state record is in use
   uint8_t avail_num[7]; // Number of available blocks of each order
   uint8_t bitmap6:2; // Bitmap for available order 6 blocks
   uint8_t bitmap5:4; // Bitmap for available order 5 blocks
   uint8_t bitmap4; // Bitmap for available order 4 blocks
   uint16_t bitmap3;
   uint32_t bitmap2;
   uint64_t bitmap1;
   uint64_t bitmap0[2];
   struct buddy_chunk_state *next_avail_idx6, *prev_avail_idx6; // Linked list of chunks with available order 6 blocks
   struct buddy_chunk_state *next_avail_idx5, *prev_avail_idx5; // Linked list of chunks with available order 5 blocks
   ...
   struct buddy_chunk_state *next_avail_idx0, *prev_avail_idx0; // Linked list of chunks with available order 0 blocks
   struct buddy_chunk_state *next_empty_state, *prev_empty_state; // Linked list of not-in-use buddy_chunk_state records
   void *chunk; // Pointer to the chunk being managed

   The size of each buddy_chunk_state record is 176 bytes.
   buddy_chunk_state records are allocated in groups of 372 as a single struct buddy_chunk_state_group.
   Each buddy_chunk_state_group is allocated by mmap'ing 16 pages.
   We don't automatically garbage-collect buddy_chunk_state_group, but can do so upon request.
 */

#define BUDDY_RECORDS_PER_GROUP ((65536 - 16) / sizeof (struct buddy_chunk_state))
#define BUDDY_BLOCK_IDX(st, block, order) ((((uintptr_t) (block)) - ((uintptr_t) (st)->chunk)) >> (12 + order))
#define BUDDY_IDX_BLOCK(st, idx, order) ((void *) (((uintptr_t) (st)->chunk) + ((idx) << (12 + order))))

struct buddy_chunk_state {
  uint8_t in_use;
  uint8_t avail_num[7];
  uint8_t bitmap6:2;
  uint8_t bitmap5:4;
  uint8_t bitmap4;
  uint16_t bitmap3;
  uint32_t bitmap2;
  uint64_t bitmap1;
  uint64_t bitmap0[2];
  struct buddy_chunk_state *next_avail_idx6, *prev_avail_idx6;
  struct buddy_chunk_state *next_avail_idx5, *prev_avail_idx5;
  struct buddy_chunk_state *next_avail_idx4, *prev_avail_idx4;
  struct buddy_chunk_state *next_avail_idx3, *prev_avail_idx3;
  struct buddy_chunk_state *next_avail_idx2, *prev_avail_idx2;
  struct buddy_chunk_state *next_avail_idx1, *prev_avail_idx1;
  struct buddy_chunk_state *next_avail_idx0, *prev_avail_idx0;
  struct buddy_chunk_state *next_empty_state, *prev_empty_state;
  void *chunk;
};

struct buddy_chunk_state_group {
  struct buddy_chunk_state_group *prev_group;
  struct buddy_chunk_state_group *next_group;
  struct buddy_chunk_state state_records[BUDDY_RECORDS_PER_GROUP];
};

/* allocate_buddy_chunk_state_group
   Allocate new struct buddy_chunk_state_group.
   Add newly allocated buddy_chunk_state records to the empty record list.
   Returns nothing. If failed, empty_list_head remains unchanged.
 */

static void allocate_buddy_chunk_state_group (struct buddy_arena_t * arena) {
  void * mmap_ctx_ptr;
  struct buddy_chunk_state_group * new_group = mmap_alloc (16 << 12, &mmap_ctx_ptr); /* 16 pages */
  if (new_group == NULL) return;

  new_group->next_group = arena->group_list_head;
  if (arena->group_list_head != NULL) arena->group_list_head->prev_group = new_group;
  arena->group_list_head = new_group;

  for (uint32_t i = 1; i < BUDDY_RECORDS_PER_GROUP - 1; ++i) {
    new_group->state_records[i].next_empty_state = &(new_group->state_records[i + 1]);
    new_group->state_records[i].prev_empty_state = &(new_group->state_records[i - 1]);
  }
  new_group->state_records[0].next_empty_state = &(new_group->state_records[1]);
  new_group->state_records[BUDDY_RECORDS_PER_GROUP - 1].prev_empty_state = &(new_group->state_records[BUDDY_RECORDS_PER_GROUP - 2]);
  new_group->state_records[BUDDY_RECORDS_PER_GROUP - 1].next_empty_state = arena->empty_list_head;
  if (arena->empty_list_head != NULL) arena->empty_list_head->prev_empty_state = &(new_group->state_records[BUDDY_RECORDS_PER_GROUP - 1]);
  arena->empty_list_head = &(new_group->state_records[0]);
}

/* allocate_buddy_chunk_state
   Allocate a new not-in-use struct buddy_chunk_state and mark it as in-use.
   Returns ptr to buddy_chunk_state; returns NULL upon failure.
 */

static struct buddy_chunk_state * allocate_buddy_chunk_state (struct buddy_arena_t * arena) {
  if (arena->empty_list_head == NULL) allocate_buddy_chunk_state_group (arena);
  if (arena->empty_list_head == NULL) return NULL;

  struct buddy_chunk_state * new_state = arena->empty_list_head;
  arena->empty_list_head = new_state->next_empty_state;
  if (arena->empty_list_head != NULL) arena->empty_list_head->prev_empty_state = NULL;
  new_state->next_empty_state = NULL;
  new_state->in_use = 1;

  return new_state;
}

/* free_buddy_chunk_state
   Deallocate a single buddy_chunk_state record.
   The caller is responsible for freeing the associated chunk area, as well as removing the record from all linked lists.
   Returns nothing.
 */

static void free_buddy_chunk_state (struct buddy_chunk_state * ptr, struct buddy_arena_t * arena) {
  memset (ptr, 0, sizeof (struct buddy_chunk_state));
  ptr->next_empty_state = arena->empty_list_head;
  if (arena->empty_list_head != NULL) arena->empty_list_head->prev_empty_state = ptr;
  arena->empty_list_head = ptr;
}

/* allocate_chunk_and_block6
   Allocate a new chunk, and a new block of order 6 from the chunk.
   Returns ptr to buddy_chunk_state of new chunk.
   The new block has index 0 within the new chunk.
   Returns NULL upon failure.
 */

static struct buddy_chunk_state * allocate_chunk_and_block6 (struct buddy_arena_t * arena) {
  struct buddy_chunk_state * new_state = allocate_buddy_chunk_state (arena);
  if (new_state == NULL) return NULL;

  void * mmap_ctx_ptr;
  new_state->chunk = mmap_alloc (128 << 12, &mmap_ctx_ptr); /* 128 pages */
  if (new_state->chunk == NULL) {
    free_buddy_chunk_state (new_state, arena);
    return NULL;
  }

  new_state->bitmap6 = 2;
  new_state->avail_num[6] = 1;
  new_state->next_avail_idx6 = arena->avail6_list_head;
  if (arena->avail6_list_head != NULL) arena->avail6_list_head->prev_avail_idx6 = new_state;
  arena->avail6_list_head = new_state;
  arena->chunk_num++;

  return new_state;
}

/* buddy_alloc_6
   Allocate a new block of order 6.
   Outputs both buddy_chunk_state ptr and block ptr.
   out_buddy_chunk_state is set to NULL upon failure.
 */

void * buddy_alloc_6 (void ** ctx_ptr, void * arena_vp) {
  struct buddy_chunk_state ** out_buddy_chunk_state = (struct buddy_chunk_state **) ctx_ptr;
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st;

  if (arena->avail6_list_head != NULL) {

    st = arena->avail6_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap6);
    st->bitmap6 &= ~ (1ull << idx);
    st->avail_num[6]--;

    if (st->avail_num[6] == 0) {
      if (st->next_avail_idx6 != NULL) st->next_avail_idx6->prev_avail_idx6 = NULL;
      st->next_avail_idx6 = NULL;
      arena->avail6_list_head = st->next_avail_idx6;
    }

    *out_buddy_chunk_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 6);

  } else {

    st = allocate_chunk_and_block6 (arena);
    if (st != NULL) {
      *out_buddy_chunk_state = st;
      return st->chunk;
    } else {
      *out_buddy_chunk_state = NULL;
      return NULL;
    }

  }
}

void * buddy_alloc_5 (void ** ctx_ptr, void * arena_vp) {
  struct buddy_chunk_state ** out_buddy_chunk_state = (struct buddy_chunk_state **) ctx_ptr;
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st;

  if (arena->avail5_list_head != NULL) {

    st = arena->avail5_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap5);
    st->bitmap5 &= ~ (1ull << idx);
    st->avail_num[5]--;

    if (st->avail_num[5] == 0) {
      if (st->next_avail_idx5 != NULL) st->next_avail_idx5->prev_avail_idx5 = NULL;
      st->next_avail_idx5 = NULL;
      arena->avail5_list_head = st->next_avail_idx5;
    }

    *out_buddy_chunk_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 5);

  } else {

    void * block = buddy_alloc_6 ((void **) &st, arena);
    if (st != NULL) {
      uint32_t idx = BUDDY_BLOCK_IDX (st, block, 6);
      st->avail_num[5]++;
      st->bitmap5 |= (1ull << (2 * idx + 1));
      if (st->avail_num[5] == 1) {
	st->next_avail_idx5 = arena->avail5_list_head;
	if (arena->avail5_list_head != NULL) arena->avail5_list_head->prev_avail_idx5 = st;
	arena->avail5_list_head = st;
      }
      *out_buddy_chunk_state = st;
      return block;
    } else {
      *out_buddy_chunk_state = NULL;
      return NULL;
    }

  }
}

void * buddy_alloc_4 (void ** ctx_ptr, void * arena_vp) {
  struct buddy_chunk_state ** out_buddy_chunk_state = (struct buddy_chunk_state **) ctx_ptr;
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st;

  if (arena->avail4_list_head != NULL) {

    st = arena->avail4_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap4);
    st->bitmap4 &= ~ (1ull << idx);
    st->avail_num[4]--;

    if (st->avail_num[4] == 0) {
      if (st->next_avail_idx4 != NULL) st->next_avail_idx4->prev_avail_idx4 = NULL;
      st->next_avail_idx4 = NULL;
      arena->avail4_list_head = st->next_avail_idx4;
    }

    *out_buddy_chunk_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 4);

  } else {

    void * block = buddy_alloc_5 ((void **) &st, arena);
    uint32_t idx = BUDDY_BLOCK_IDX (st, block, 5);
    if (st != NULL) {
      st->avail_num[4]++;
      st->bitmap4 |= (1ull << (2 * idx + 1));
      if (st->avail_num[4] == 1) {
	st->next_avail_idx4 = arena->avail4_list_head;
	if (arena->avail4_list_head != NULL) arena->avail4_list_head->prev_avail_idx4 = st;
	arena->avail4_list_head = st;
      }
      *out_buddy_chunk_state = st;
      return block;
    } else {
      *out_buddy_chunk_state = NULL;
      return NULL;
    }

  }
}

void * buddy_alloc_3 (void ** ctx_ptr, void * arena_vp) {
  struct buddy_chunk_state ** out_buddy_chunk_state = (struct buddy_chunk_state **) ctx_ptr;
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st;

  if (arena->avail3_list_head != NULL) {

    st = arena->avail3_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap3);
    st->bitmap3 &= ~ (1ull << idx);
    st->avail_num[3]--;

    if (st->avail_num[3] == 0) {
      if (st->next_avail_idx3 != NULL) st->next_avail_idx3->prev_avail_idx3 = NULL;
      st->next_avail_idx3 = NULL;
      arena->avail3_list_head = st->next_avail_idx3;
    }

    *out_buddy_chunk_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 3);

  } else {

    void * block = buddy_alloc_4 ((void **) &st, arena);
    if (st != NULL) {
      uint32_t idx = BUDDY_BLOCK_IDX (st, block, 4);
      st->avail_num[3]++;
      st->bitmap3 |= (1ull << (2 * idx + 1));
      if (st->avail_num[3] == 1) {
	st->next_avail_idx3 = arena->avail3_list_head;
	if (arena->avail3_list_head != NULL) arena->avail3_list_head->prev_avail_idx3 = st;
	arena->avail3_list_head = st;
      }
      *out_buddy_chunk_state = st;
      return block;
    } else {
      *out_buddy_chunk_state = NULL;
      return NULL;
    }

  }
}

void * buddy_alloc_2 (void ** ctx_ptr, void * arena_vp) {
  struct buddy_chunk_state ** out_buddy_chunk_state = (struct buddy_chunk_state **) ctx_ptr;
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st;

  if (arena->avail2_list_head != NULL) {

    st = arena->avail2_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap2);
    st->bitmap2 &= ~ (1ull << idx);
    st->avail_num[2]--;

    if (st->avail_num[2] == 0) {
      if (st->next_avail_idx2 != NULL) st->next_avail_idx2->prev_avail_idx2 = NULL;
      st->next_avail_idx2 = NULL;
      arena->avail2_list_head = st->next_avail_idx2;
    }

    *out_buddy_chunk_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 2);

  } else {

    void * block = buddy_alloc_3 ((void **) &st, arena);
    if (st != NULL) {
      uint32_t idx = BUDDY_BLOCK_IDX (st, block, 3);
      st->avail_num[2]++;
      st->bitmap2 |= (1ull << (2 * idx + 1));
      if (st->avail_num[2] == 1) {
	st->next_avail_idx2 = arena->avail2_list_head;
	if (arena->avail2_list_head != NULL) arena->avail2_list_head->prev_avail_idx2 = st;
	arena->avail2_list_head = st;
      }
      *out_buddy_chunk_state = st;
      return block;
    } else {
      *out_buddy_chunk_state = NULL;
      return NULL;
    }

  }
}

void * buddy_alloc_1 (void ** ctx_ptr, void * arena_vp) {
  struct buddy_chunk_state ** out_buddy_chunk_state = (struct buddy_chunk_state **) ctx_ptr;
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st;

  if (arena->avail1_list_head != NULL) {

    st = arena->avail1_list_head;
    uint32_t idx = __builtin_ctzll (st->bitmap1);
    st->bitmap1 &= ~ (1ull << idx);
    st->avail_num[1]--;

    if (st->avail_num[1] == 0) {
      if (st->next_avail_idx1 != NULL) st->next_avail_idx1->prev_avail_idx1 = NULL;
      st->next_avail_idx1 = NULL;
      arena->avail1_list_head = st->next_avail_idx1;
    }

    *out_buddy_chunk_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 1);

  } else {

    void * block = buddy_alloc_2 ((void **) &st, arena);
    if (st != NULL) {
      uint32_t idx = BUDDY_BLOCK_IDX (st, block, 2);
      st->avail_num[1]++;
      st->bitmap1 |= (1ull << (2 * idx + 1));
      if (st->avail_num[1] == 1) {
	st->next_avail_idx1 = arena->avail1_list_head;
	if (arena->avail1_list_head != NULL) arena->avail1_list_head->prev_avail_idx1 = st;
	arena->avail1_list_head = st;
      }
      *out_buddy_chunk_state = st;
      return block;
    } else {
      *out_buddy_chunk_state = NULL;
      return NULL;
    }

  }
}

void * buddy_alloc_0 (void ** ctx_ptr, void * arena_vp) {
  struct buddy_chunk_state ** out_buddy_chunk_state = (struct buddy_chunk_state **) ctx_ptr;
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st;

  if (arena->avail0_list_head != NULL) {

    st = arena->avail0_list_head;
    uint32_t idx;
    if (st->bitmap0[0]) {
      idx = __builtin_ctzll (st->bitmap0[0]);
      st->bitmap0[0] &= ~ (1ull << idx);
      st->avail_num[0]--;
    } else {
      idx = __builtin_ctzll (st->bitmap0[1]);
      st->bitmap0[1] &= ~ (1ull << idx);
      st->avail_num[0]--;
      idx += 64;
    }

    if (st->avail_num[0] == 0) {
      if (st->next_avail_idx0 != NULL) st->next_avail_idx0->prev_avail_idx0 = NULL;
      st->next_avail_idx0 = NULL;
      arena->avail0_list_head = st->next_avail_idx0;
    }

    *out_buddy_chunk_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 0);

  } else {

    void * block = buddy_alloc_1 ((void **) &st, arena);
    if (st != NULL) {
      uint32_t idx = BUDDY_BLOCK_IDX (st, block, 1);
      st->avail_num[0]++;
      if (idx < 32) {
	st->bitmap0[0] |= (1ull << (2 * idx + 1));
      } else {
	st->bitmap0[1] |= (1ull << (2 * (idx - 32) + 1));
      }
      if (st->avail_num[0] == 1) {
	st->next_avail_idx0 = arena->avail0_list_head;
	if (arena->avail0_list_head != NULL) arena->avail0_list_head->prev_avail_idx0 = st;
	arena->avail0_list_head = st;
      }
      *out_buddy_chunk_state = st;
      return block;
    } else {
      *out_buddy_chunk_state = NULL;
      return NULL;
    }

  }
}

void buddy_free_6 (void * ptr, void * ctx, void * arena_vp) {
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st = (struct buddy_chunk_state *) ctx;
  uint32_t block_idx = BUDDY_BLOCK_IDX (st, ptr, 6);

  uint32_t buddy = block_idx ^ 1;
  if ((st->bitmap6 & (1ull << buddy)) && arena->chunk_num > LIBC_KEEP_CHUNK_NUM) {
    /* At this point, no smaller blocks should be available */
    if (st->prev_avail_idx6 != NULL) st->prev_avail_idx6->next_avail_idx6 = st->next_avail_idx6;
    if (st->next_avail_idx6 != NULL) st->next_avail_idx6->prev_avail_idx6 = st->prev_avail_idx6;
    if (arena->avail6_list_head == st) arena->avail6_list_head = st->next_avail_idx6;
    mmap_free (st->chunk, st->chunk, 128 << 12);
    free_buddy_chunk_state (st, arena);
    arena->chunk_num--;
  } else {
    st->bitmap6 |= (1ull << block_idx);
    st->avail_num[6]++;
    if (st->avail_num[6] == 1) {
      st->next_avail_idx6 = arena->avail6_list_head;
      if (arena->avail6_list_head != NULL) arena->avail6_list_head->prev_avail_idx6 = st;
      arena->avail6_list_head = st;
    }
  }
}

void buddy_free_5 (void * ptr, void * ctx, void * arena_vp) {
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st = (struct buddy_chunk_state *) ctx;
  uint32_t block_idx = BUDDY_BLOCK_IDX (st, ptr, 5);

  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap5 & (1ull << buddy)) {
    st->bitmap5 &= ~ (1ull << buddy);
    st->avail_num[5]--;
    if (st->avail_num[5] == 0) {
      if (st->prev_avail_idx5 != NULL) st->prev_avail_idx5->next_avail_idx5 = st->next_avail_idx5;
      if (st->next_avail_idx5 != NULL) st->next_avail_idx5->prev_avail_idx5 = st->prev_avail_idx5;
      if (arena->avail5_list_head == st) arena->avail5_list_head = st->next_avail_idx5;
      st->prev_avail_idx5 = NULL;
      st->next_avail_idx5 = NULL;
    }
    buddy_free_6 (BUDDY_IDX_BLOCK (st, block_idx >> 1, 6), st, arena);
  } else {
    st->bitmap5 |= (1ull << block_idx);
    st->avail_num[5]++;
    if (st->avail_num[5] == 1) {
      st->next_avail_idx5 = arena->avail5_list_head;
      if (arena->avail5_list_head != NULL) arena->avail5_list_head->prev_avail_idx5 = st;
      arena->avail5_list_head = st;
    }
  }
}

void buddy_free_4 (void * ptr, void * ctx, void * arena_vp) {
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st = (struct buddy_chunk_state *) ctx;
  uint32_t block_idx = BUDDY_BLOCK_IDX (st, ptr, 4);

  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap4 & (1ull << buddy)) {
    st->bitmap4 &= ~ (1ull << buddy);
    st->avail_num[4]--;
    if (st->avail_num[4] == 0) {
      if (st->prev_avail_idx4 != NULL) st->prev_avail_idx4->next_avail_idx4 = st->next_avail_idx4;
      if (st->next_avail_idx4 != NULL) st->next_avail_idx4->prev_avail_idx4 = st->prev_avail_idx4;
      if (arena->avail4_list_head == st) arena->avail4_list_head = st->next_avail_idx4;
      st->prev_avail_idx4 = NULL;
      st->next_avail_idx4 = NULL;
    }
    buddy_free_5 (BUDDY_IDX_BLOCK (st, block_idx >> 1, 5), st, arena);
  } else {
    st->bitmap4 |= (1ull << block_idx);
    st->avail_num[4]++;
    if (st->avail_num[4] == 1) {
      st->next_avail_idx4 = arena->avail4_list_head;
      if (arena->avail4_list_head != NULL) arena->avail4_list_head->prev_avail_idx4 = st;
      arena->avail4_list_head = st;
    }
  }
}

void buddy_free_3 (void * ptr, void * ctx, void * arena_vp) {
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st = (struct buddy_chunk_state *) ctx;
  uint32_t block_idx = BUDDY_BLOCK_IDX (st, ptr, 3);

  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap3 & (1ull << buddy)) {
    st->bitmap3 &= ~ (1ull << buddy);
    st->avail_num[3]--;
    if (st->avail_num[3] == 0) {
      if (st->prev_avail_idx3 != NULL) st->prev_avail_idx3->next_avail_idx3 = st->next_avail_idx3;
      if (st->next_avail_idx3 != NULL) st->next_avail_idx3->prev_avail_idx3 = st->prev_avail_idx3;
      if (arena->avail3_list_head == st) arena->avail3_list_head = st->next_avail_idx3;
      st->prev_avail_idx3 = NULL;
      st->next_avail_idx3 = NULL;
    }
    buddy_free_4 (BUDDY_IDX_BLOCK (st, block_idx >> 1, 4), st, arena);
  } else {
    st->bitmap3 |= (1ull << block_idx);
    st->avail_num[3]++;
    if (st->avail_num[3] == 1) {
      st->next_avail_idx3 = arena->avail3_list_head;
      if (arena->avail3_list_head != NULL) arena->avail3_list_head->prev_avail_idx3 = st;
      arena->avail3_list_head = st;
    }
  }
}

void buddy_free_2 (void * ptr, void * ctx, void * arena_vp) {
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st = (struct buddy_chunk_state *) ctx;
  uint32_t block_idx = BUDDY_BLOCK_IDX (st, ptr, 2);

  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap2 & (1ull << buddy)) {
    st->bitmap2 &= ~ (1ull << buddy);
    st->avail_num[2]--;
    if (st->avail_num[2] == 0) {
      if (st->prev_avail_idx2 != NULL) st->prev_avail_idx2->next_avail_idx2 = st->next_avail_idx2;
      if (st->next_avail_idx2 != NULL) st->next_avail_idx2->prev_avail_idx2 = st->prev_avail_idx2;
      if (arena->avail2_list_head == st) arena->avail2_list_head = st->next_avail_idx2;
      st->prev_avail_idx2 = NULL;
      st->next_avail_idx2 = NULL;
    }
    buddy_free_3 (BUDDY_IDX_BLOCK (st, block_idx >> 1, 3), st, arena);
  } else {
    st->bitmap2 |= (1ull << block_idx);
    st->avail_num[2]++;
    if (st->avail_num[2] == 1) {
      st->next_avail_idx2 = arena->avail2_list_head;
      if (arena->avail2_list_head != NULL) arena->avail2_list_head->prev_avail_idx2 = st;
      arena->avail2_list_head = st;
    }
  }
}

void buddy_free_1 (void * ptr, void * ctx, void * arena_vp) {
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st = (struct buddy_chunk_state *) ctx;
  uint32_t block_idx = BUDDY_BLOCK_IDX (st, ptr, 1);

  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap1 & (1ull << buddy)) {
    st->bitmap1 &= ~ (1ull << buddy);
    st->avail_num[1]--;
    if (st->avail_num[1] == 0) {
      if (st->prev_avail_idx1 != NULL) st->prev_avail_idx1->next_avail_idx1 = st->next_avail_idx1;
      if (st->next_avail_idx1 != NULL) st->next_avail_idx1->prev_avail_idx1 = st->prev_avail_idx1;
      if (arena->avail1_list_head == st) arena->avail1_list_head = st->next_avail_idx1;
      st->prev_avail_idx1 = NULL;
      st->next_avail_idx1 = NULL;
    }
    buddy_free_2 (BUDDY_IDX_BLOCK (st, block_idx >> 1, 2), st, arena);
  } else {
    st->bitmap1 |= (1ull << block_idx);
    st->avail_num[1]++;
    if (st->avail_num[1] == 1) {
      st->next_avail_idx1 = arena->avail1_list_head;
      if (arena->avail1_list_head != NULL) arena->avail1_list_head->prev_avail_idx1 = st;
      arena->avail1_list_head = st;
    }
  }
}

void buddy_free_0 (void * ptr, void * ctx, void * arena_vp) {
  struct buddy_arena_t * arena = (struct buddy_arena_t *) arena_vp;
  struct buddy_chunk_state * st = (struct buddy_chunk_state *) ctx;
  uint32_t block_idx = BUDDY_BLOCK_IDX (st, ptr, 0);

  uint32_t buddy = block_idx ^ 1;
  uint32_t merge_cond0 = block_idx < 64 && (st->bitmap0[0] & (1ull << buddy));
  uint32_t merge_cond1 = block_idx >= 64 && (st->bitmap0[1] & (1ull << (buddy - 64)));
  if (merge_cond0 || merge_cond1) {
    if (merge_cond0) st->bitmap0[0] &= ~ (1ull << buddy); else st->bitmap0[1] &= ~ (1ull << (buddy - 64));
    st->avail_num[0]--;
    if (st->avail_num[0] == 0) {
      if (st->prev_avail_idx0 != NULL) st->prev_avail_idx0->next_avail_idx0 = st->next_avail_idx0;
      if (st->next_avail_idx0 != NULL) st->next_avail_idx0->prev_avail_idx0 = st->prev_avail_idx0;
      if (arena->avail0_list_head == st) arena->avail0_list_head = st->next_avail_idx0;
      st->prev_avail_idx0 = NULL;
      st->next_avail_idx0 = NULL;
    }
    buddy_free_1 (BUDDY_IDX_BLOCK (st, block_idx >> 1, 1), st, arena);
  } else {
    if (block_idx < 64) st->bitmap0[0] |= (1ull << block_idx); else st->bitmap0[1] |= (1ull << (block_idx - 64));
    st->avail_num[0]++;
    if (st->avail_num[0] == 1) {
      st->next_avail_idx0 = arena->avail0_list_head;
      if (arena->avail0_list_head != NULL) arena->avail0_list_head->prev_avail_idx0 = st;
      arena->avail0_list_head = st;
    }
  }
}
