#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <tls.h>
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

   The state of each chunk is recorded using a `struct buddy_state`.
   This struct contains the following fields:
   uint8_t in_use; // Whether this buddy_state record is in use
   uint8_t avail_num[7]; // Number of available blocks of each order
   uint8_t bitmap6:2; // Bitmap for available order 6 blocks
   uint8_t bitmap5:4; // Bitmap for available order 5 blocks
   uint8_t bitmap4; // Bitmap for available order 4 blocks
   uint16_t bitmap3;
   uint32_t bitmap2;
   uint64_t bitmap1;
   uint64_t bitmap0[2];
   struct buddy_state *next_avail_idx6, *prev_avail_idx6; // Linked list of chunks with available order 6 blocks
   struct buddy_state *next_avail_idx5, *prev_avail_idx5; // Linked list of chunks with available order 5 blocks
   ...
   struct buddy_state *next_avail_idx0, *prev_avail_idx0; // Linked list of chunks with available order 0 blocks
   struct buddy_state *next_empty_state, *prev_empty_state; // Linked list of not-in-use buddy_state records
   void *chunk; // Pointer to the chunk being managed

   The size of each buddy_state record is 176 bytes.
   buddy_state records are allocated in groups of 372 as a single struct buddy_state_area.
   Each buddy_state_area is allocated by mmap'ing 16 pages.
   We don't automatically garbage-collect buddy_state_area, but can do so upon request.
 */

#define BUDDY_RECORDS_PER_AREA ((65536 - 16) / sizeof (struct buddy_state))
#define BUDDY_BLOCK_IDX(st, block, order) ((((uintptr_t) (block)) - ((uintptr_t) (st)->chunk)) >> (12 + order))
#define BUDDY_IDX_BLOCK(st, idx, order) ((void *) (((uintptr_t) (st)->chunk) + ((idx) << (12 + order))))

struct buddy_state_area {
  struct buddy_state_area *prev_area;
  struct buddy_state_area *next_area;
  struct buddy_state state_records[BUDDY_RECORDS_PER_AREA];
};

struct buddy_tls_t {
  struct buddy_state_area *area_list_head;
  struct buddy_state *empty_list_head;
  struct buddy_state *avail6_list_head;
  struct buddy_state *avail5_list_head;
  struct buddy_state *avail4_list_head;
  struct buddy_state *avail3_list_head;
  struct buddy_state *avail2_list_head;
  struct buddy_state *avail1_list_head;
  struct buddy_state *avail0_list_head;
  uint32_t chunk_num;
};

static struct buddy_tls_t buddy_tls[LIBC_MAX_THREAD_NUM];

/* allocate_buddy_state_area
   Allocate new struct buddy_state_area.
   Add newly allocated buddy_state records to the empty record list.
   Returns nothing. If failed, empty_list_head remains unchanged.
 */

static void allocate_buddy_state_area (void) {
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];

  void * mmap_ctx_ptr;
  struct buddy_state_area * new_area = mmap_alloc (16 << 12, &mmap_ctx_ptr);
  if (new_area == NULL) return;

  new_area->next_area = tls->area_list_head;
  if (tls->area_list_head != NULL) tls->area_list_head->prev_area = new_area;
  tls->area_list_head = new_area;

  for (uint32_t i = 1; i < BUDDY_RECORDS_PER_AREA - 1; ++i) {
    new_area->state_records[i].next_empty_state = &(new_area->state_records[i + 1]);
    new_area->state_records[i].prev_empty_state = &(new_area->state_records[i - 1]);
  }
  new_area->state_records[0].next_empty_state = &(new_area->state_records[1]);
  new_area->state_records[BUDDY_RECORDS_PER_AREA - 1].prev_empty_state = &(new_area->state_records[BUDDY_RECORDS_PER_AREA - 2]);
  new_area->state_records[BUDDY_RECORDS_PER_AREA - 1].next_empty_state = tls->empty_list_head;
  if (tls->empty_list_head != NULL) tls->empty_list_head->prev_empty_state = &(new_area->state_records[BUDDY_RECORDS_PER_AREA - 1]);
  tls->empty_list_head = &(new_area->state_records[0]);
}

/* allocate_buddy_state
   Allocate a new not-in-use struct buddy_state and mark it as in-use.
   Returns ptr to buddy_state; returns NULL upon failure.
 */

static struct buddy_state * allocate_buddy_state (void) {
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];

  if (tls->empty_list_head == NULL) allocate_buddy_state_area ();
  if (tls->empty_list_head == NULL) return NULL;

  struct buddy_state * new_state = tls->empty_list_head;
  tls->empty_list_head = new_state->next_empty_state;
  if (tls->empty_list_head != NULL) tls->empty_list_head->prev_empty_state = NULL;
  new_state->next_empty_state = NULL;
  new_state->in_use = 1;

  return new_state;
}

/* free_buddy_state
   Deallocate a single buddy_state record.
   The caller is responsible for freeing the associated chunk area, as well as removing the record from all linked lists.
   Returns nothing.
 */

static void free_buddy_state (struct buddy_state *ptr) {
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];

  memset (ptr, 0, sizeof (struct buddy_state));
  ptr->next_empty_state = tls->empty_list_head;
  if (tls->empty_list_head != NULL) tls->empty_list_head->prev_empty_state = ptr;
  tls->empty_list_head = ptr;
}

/* allocate_chunk_and_block6
   Allocate a new chunk, and a new block of order 6 from the chunk.
   Returns ptr to buddy_state of new chunk.
   The new block has index 0 within the new chunk.
   Returns NULL upon failure.
 */

static struct buddy_state * allocate_chunk_and_block6 (void) {
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];

  struct buddy_state * new_state = allocate_buddy_state ();
  if (new_state == NULL) return NULL;

  void * mmap_ctx_ptr;
  new_state->chunk = mmap_alloc (128 << 12, &mmap_ctx_ptr);
  if (new_state->chunk == NULL) {
    free_buddy_state (new_state);
    return NULL;
  }

  new_state->bitmap6 = 2;
  new_state->avail_num[6] = 1;
  new_state->next_avail_idx6 = tls->avail6_list_head;
  if (tls->avail6_list_head != NULL) tls->avail6_list_head->prev_avail_idx6 = new_state;
  tls->avail6_list_head = new_state;
  tls->chunk_num++;

  return new_state;
}

/* buddy_alloc_6
   Allocate a new block of order 6.
   Outputs both buddy_state ptr and block ptr.
   out_buddy_state is set to NULL upon failure.
 */

void * buddy_alloc_6 (void ** ctx_ptr) {
  struct buddy_state ** out_buddy_state = (struct buddy_state **) ctx_ptr;
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st;

  if (tls->avail6_list_head != NULL) {

    st = tls->avail6_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap6);
    st->bitmap6 &= ~ (1ull << idx);
    st->avail_num[6]--;

    if (st->avail_num[6] == 0) {
      if (st->next_avail_idx6 != NULL) st->next_avail_idx6->prev_avail_idx6 = NULL;
      st->next_avail_idx6 = NULL;
      tls->avail6_list_head = st->next_avail_idx6;
    }

    *out_buddy_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 6);

  } else {

    st = allocate_chunk_and_block6 ();
    if (st != NULL) {
      *out_buddy_state = st;
      return st->chunk;
    } else {
      *out_buddy_state = NULL;
      return NULL;
    }

  }
}

void * buddy_alloc_5 (void ** ctx_ptr) {
  struct buddy_state ** out_buddy_state = (struct buddy_state **) ctx_ptr;
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st;

  if (tls->avail5_list_head != NULL) {

    st = tls->avail5_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap5);
    st->bitmap5 &= ~ (1ull << idx);
    st->avail_num[5]--;

    if (st->avail_num[5] == 0) {
      if (st->next_avail_idx5 != NULL) st->next_avail_idx5->prev_avail_idx5 = NULL;
      st->next_avail_idx5 = NULL;
      tls->avail5_list_head = st->next_avail_idx5;
    }

    *out_buddy_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 5);

  } else {

    void * block = buddy_alloc_6 ((void **) &st);
    if (st != NULL) {
      uint32_t idx = BUDDY_BLOCK_IDX (st, block, 6);
      st->avail_num[5]++;
      st->bitmap5 |= (1ull << (2 * idx + 1));
      if (st->avail_num[5] == 1) {
	st->next_avail_idx5 = tls->avail5_list_head;
	if (tls->avail5_list_head != NULL) tls->avail5_list_head->prev_avail_idx5 = st;
	tls->avail5_list_head = st;
      }
      *out_buddy_state = st;
      return block;
    } else {
      *out_buddy_state = NULL;
      return NULL;
    }

  }
}

void * buddy_alloc_4 (void ** ctx_ptr) {
  struct buddy_state ** out_buddy_state = (struct buddy_state **) ctx_ptr;
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st;

  if (tls->avail4_list_head != NULL) {

    st = tls->avail4_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap4);
    st->bitmap4 &= ~ (1ull << idx);
    st->avail_num[4]--;

    if (st->avail_num[4] == 0) {
      if (st->next_avail_idx4 != NULL) st->next_avail_idx4->prev_avail_idx4 = NULL;
      st->next_avail_idx4 = NULL;
      tls->avail4_list_head = st->next_avail_idx4;
    }

    *out_buddy_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 4);

  } else {

    void * block = buddy_alloc_5 ((void **) &st);
    uint32_t idx = BUDDY_BLOCK_IDX (st, block, 5);
    if (st != NULL) {
      st->avail_num[4]++;
      st->bitmap4 |= (1ull << (2 * idx + 1));
      if (st->avail_num[4] == 1) {
	st->next_avail_idx4 = tls->avail4_list_head;
	if (tls->avail4_list_head != NULL) tls->avail4_list_head->prev_avail_idx4 = st;
	tls->avail4_list_head = st;
      }
      *out_buddy_state = st;
      return block;
    } else {
      *out_buddy_state = NULL;
      return NULL;
    }

  }
}

void * buddy_alloc_3 (void ** ctx_ptr) {
  struct buddy_state ** out_buddy_state = (struct buddy_state **) ctx_ptr;
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st;

  if (tls->avail3_list_head != NULL) {

    st = tls->avail3_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap3);
    st->bitmap3 &= ~ (1ull << idx);
    st->avail_num[3]--;

    if (st->avail_num[3] == 0) {
      if (st->next_avail_idx3 != NULL) st->next_avail_idx3->prev_avail_idx3 = NULL;
      st->next_avail_idx3 = NULL;
      tls->avail3_list_head = st->next_avail_idx3;
    }

    *out_buddy_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 3);

  } else {

    void * block = buddy_alloc_4 ((void **) &st);
    if (st != NULL) {
      uint32_t idx = BUDDY_BLOCK_IDX (st, block, 4);
      st->avail_num[3]++;
      st->bitmap3 |= (1ull << (2 * idx + 1));
      if (st->avail_num[3] == 1) {
	st->next_avail_idx3 = tls->avail3_list_head;
	if (tls->avail3_list_head != NULL) tls->avail3_list_head->prev_avail_idx3 = st;
	tls->avail3_list_head = st;
      }
      *out_buddy_state = st;
      return block;
    } else {
      *out_buddy_state = NULL;
      return NULL;
    }

  }
}

void * buddy_alloc_2 (void ** ctx_ptr) {
  struct buddy_state ** out_buddy_state = (struct buddy_state **) ctx_ptr;
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st;

  if (tls->avail2_list_head != NULL) {

    st = tls->avail2_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap2);
    st->bitmap2 &= ~ (1ull << idx);
    st->avail_num[2]--;

    if (st->avail_num[2] == 0) {
      if (st->next_avail_idx2 != NULL) st->next_avail_idx2->prev_avail_idx2 = NULL;
      st->next_avail_idx2 = NULL;
      tls->avail2_list_head = st->next_avail_idx2;
    }

    *out_buddy_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 2);

  } else {

    void * block = buddy_alloc_3 ((void **) &st);
    if (st != NULL) {
      uint32_t idx = BUDDY_BLOCK_IDX (st, block, 3);
      st->avail_num[2]++;
      st->bitmap2 |= (1ull << (2 * idx + 1));
      if (st->avail_num[2] == 1) {
	st->next_avail_idx2 = tls->avail2_list_head;
	if (tls->avail2_list_head != NULL) tls->avail2_list_head->prev_avail_idx2 = st;
	tls->avail2_list_head = st;
      }
      *out_buddy_state = st;
      return block;
    } else {
      *out_buddy_state = NULL;
      return NULL;
    }

  }
}

void * buddy_alloc_1 (void ** ctx_ptr) {
  struct buddy_state ** out_buddy_state = (struct buddy_state **) ctx_ptr;
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st;

  if (tls->avail1_list_head != NULL) {

    st = tls->avail1_list_head;
    uint32_t idx = __builtin_ctzll (st->bitmap1);
    st->bitmap1 &= ~ (1ull << idx);
    st->avail_num[1]--;

    if (st->avail_num[1] == 0) {
      if (st->next_avail_idx1 != NULL) st->next_avail_idx1->prev_avail_idx1 = NULL;
      st->next_avail_idx1 = NULL;
      tls->avail1_list_head = st->next_avail_idx1;
    }

    *out_buddy_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 1);

  } else {

    void * block = buddy_alloc_2 ((void **) &st);
    if (st != NULL) {
      uint32_t idx = BUDDY_BLOCK_IDX (st, block, 2);
      st->avail_num[1]++;
      st->bitmap1 |= (1ull << (2 * idx + 1));
      if (st->avail_num[1] == 1) {
	st->next_avail_idx1 = tls->avail1_list_head;
	if (tls->avail1_list_head != NULL) tls->avail1_list_head->prev_avail_idx1 = st;
	tls->avail1_list_head = st;
      }
      *out_buddy_state = st;
      return block;
    } else {
      *out_buddy_state = NULL;
      return NULL;
    }

  }
}

void * buddy_alloc_0 (void ** ctx_ptr) {
  struct buddy_state ** out_buddy_state = (struct buddy_state **) ctx_ptr;
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st;

  if (tls->avail0_list_head != NULL) {

    st = tls->avail0_list_head;
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
      tls->avail0_list_head = st->next_avail_idx0;
    }

    *out_buddy_state = st;
    return BUDDY_IDX_BLOCK (st, idx, 0);

  } else {

    void * block = buddy_alloc_1 ((void **) &st);
    if (st != NULL) {
      uint32_t idx = BUDDY_BLOCK_IDX (st, block, 1);
      st->avail_num[0]++;
      if (idx < 32) {
	st->bitmap0[0] |= (1ull << (2 * idx + 1));
      } else {
	st->bitmap0[1] |= (1ull << (2 * (idx - 32) + 1));
      }
      if (st->avail_num[0] == 1) {
	st->next_avail_idx0 = tls->avail0_list_head;
	if (tls->avail0_list_head != NULL) tls->avail0_list_head->prev_avail_idx0 = st;
	tls->avail0_list_head = st;
      }
      *out_buddy_state = st;
      return block;
    } else {
      *out_buddy_state = NULL;
      return NULL;
    }

  }
}

void buddy_free_6 (void * ptr, void * ctx) {
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st = (struct buddy_state *) ctx;
  uint32_t block_idx = BUDDY_BLOCK_IDX (st, ptr, 6);

  uint32_t buddy = block_idx ^ 1;
  if ((st->bitmap6 & (1ull << buddy)) && tls->chunk_num > LIBC_KEEP_CHUNK_NUM) {
    /* At this point, no smaller blocks should be available */
    if (st->prev_avail_idx6 != NULL) st->prev_avail_idx6->next_avail_idx6 = st->next_avail_idx6;
    if (st->next_avail_idx6 != NULL) st->next_avail_idx6->prev_avail_idx6 = st->prev_avail_idx6;
    if (tls->avail6_list_head == st) tls->avail6_list_head = st->next_avail_idx6;
    mmap_free (st->chunk, st->chunk, 128 << 12);
    free_buddy_state (st);
    tls->chunk_num--;
  } else {
    st->bitmap6 |= (1ull << block_idx);
    st->avail_num[6]++;
    if (st->avail_num[6] == 1) {
      st->next_avail_idx6 = tls->avail6_list_head;
      if (tls->avail6_list_head != NULL) tls->avail6_list_head->prev_avail_idx6 = st;
      tls->avail6_list_head = st;
    }
  }
}

void buddy_free_5 (void * ptr, void * ctx) {
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st = (struct buddy_state *) ctx;
  uint32_t block_idx = BUDDY_BLOCK_IDX (st, ptr, 5);

  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap5 & (1ull << buddy)) {
    st->bitmap5 &= ~ (1ull << buddy);
    st->avail_num[5]--;
    if (st->avail_num[5] == 0) {
      if (st->prev_avail_idx5 != NULL) st->prev_avail_idx5->next_avail_idx5 = st->next_avail_idx5;
      if (st->next_avail_idx5 != NULL) st->next_avail_idx5->prev_avail_idx5 = st->prev_avail_idx5;
      if (tls->avail5_list_head == st) tls->avail5_list_head = st->next_avail_idx5;
      st->prev_avail_idx5 = NULL;
      st->next_avail_idx5 = NULL;
    }
    buddy_free_6 (BUDDY_IDX_BLOCK (st, block_idx >> 1, 6), st);
  } else {
    st->bitmap5 |= (1ull << block_idx);
    st->avail_num[5]++;
    if (st->avail_num[5] == 1) {
      st->next_avail_idx5 = tls->avail5_list_head;
      if (tls->avail5_list_head != NULL) tls->avail5_list_head->prev_avail_idx5 = st;
      tls->avail5_list_head = st;
    }
  }
}

void buddy_free_4 (void * ptr, void * ctx) {
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st = (struct buddy_state *) ctx;
  uint32_t block_idx = BUDDY_BLOCK_IDX (st, ptr, 4);

  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap4 & (1ull << buddy)) {
    st->bitmap4 &= ~ (1ull << buddy);
    st->avail_num[4]--;
    if (st->avail_num[4] == 0) {
      if (st->prev_avail_idx4 != NULL) st->prev_avail_idx4->next_avail_idx4 = st->next_avail_idx4;
      if (st->next_avail_idx4 != NULL) st->next_avail_idx4->prev_avail_idx4 = st->prev_avail_idx4;
      if (tls->avail4_list_head == st) tls->avail4_list_head = st->next_avail_idx4;
      st->prev_avail_idx4 = NULL;
      st->next_avail_idx4 = NULL;
    }
    buddy_free_5 (BUDDY_IDX_BLOCK (st, block_idx >> 1, 5), st);
  } else {
    st->bitmap4 |= (1ull << block_idx);
    st->avail_num[4]++;
    if (st->avail_num[4] == 1) {
      st->next_avail_idx4 = tls->avail4_list_head;
      if (tls->avail4_list_head != NULL) tls->avail4_list_head->prev_avail_idx4 = st;
      tls->avail4_list_head = st;
    }
  }
}

void buddy_free_3 (void * ptr, void * ctx) {
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st = (struct buddy_state *) ctx;
  uint32_t block_idx = BUDDY_BLOCK_IDX (st, ptr, 3);

  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap3 & (1ull << buddy)) {
    st->bitmap3 &= ~ (1ull << buddy);
    st->avail_num[3]--;
    if (st->avail_num[3] == 0) {
      if (st->prev_avail_idx3 != NULL) st->prev_avail_idx3->next_avail_idx3 = st->next_avail_idx3;
      if (st->next_avail_idx3 != NULL) st->next_avail_idx3->prev_avail_idx3 = st->prev_avail_idx3;
      if (tls->avail3_list_head == st) tls->avail3_list_head = st->next_avail_idx3;
      st->prev_avail_idx3 = NULL;
      st->next_avail_idx3 = NULL;
    }
    buddy_free_4 (BUDDY_IDX_BLOCK (st, block_idx >> 1, 4), st);
  } else {
    st->bitmap3 |= (1ull << block_idx);
    st->avail_num[3]++;
    if (st->avail_num[3] == 1) {
      st->next_avail_idx3 = tls->avail3_list_head;
      if (tls->avail3_list_head != NULL) tls->avail3_list_head->prev_avail_idx3 = st;
      tls->avail3_list_head = st;
    }
  }
}

void buddy_free_2 (void * ptr, void * ctx) {
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st = (struct buddy_state *) ctx;
  uint32_t block_idx = BUDDY_BLOCK_IDX (st, ptr, 2);

  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap2 & (1ull << buddy)) {
    st->bitmap2 &= ~ (1ull << buddy);
    st->avail_num[2]--;
    if (st->avail_num[2] == 0) {
      if (st->prev_avail_idx2 != NULL) st->prev_avail_idx2->next_avail_idx2 = st->next_avail_idx2;
      if (st->next_avail_idx2 != NULL) st->next_avail_idx2->prev_avail_idx2 = st->prev_avail_idx2;
      if (tls->avail2_list_head == st) tls->avail2_list_head = st->next_avail_idx2;
      st->prev_avail_idx2 = NULL;
      st->next_avail_idx2 = NULL;
    }
    buddy_free_3 (BUDDY_IDX_BLOCK (st, block_idx >> 1, 3), st);
  } else {
    st->bitmap2 |= (1ull << block_idx);
    st->avail_num[2]++;
    if (st->avail_num[2] == 1) {
      st->next_avail_idx2 = tls->avail2_list_head;
      if (tls->avail2_list_head != NULL) tls->avail2_list_head->prev_avail_idx2 = st;
      tls->avail2_list_head = st;
    }
  }
}

void buddy_free_1 (void * ptr, void * ctx) {
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st = (struct buddy_state *) ctx;
  uint32_t block_idx = BUDDY_BLOCK_IDX (st, ptr, 1);

  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap1 & (1ull << buddy)) {
    st->bitmap1 &= ~ (1ull << buddy);
    st->avail_num[1]--;
    if (st->avail_num[1] == 0) {
      if (st->prev_avail_idx1 != NULL) st->prev_avail_idx1->next_avail_idx1 = st->next_avail_idx1;
      if (st->next_avail_idx1 != NULL) st->next_avail_idx1->prev_avail_idx1 = st->prev_avail_idx1;
      if (tls->avail1_list_head == st) tls->avail1_list_head = st->next_avail_idx1;
      st->prev_avail_idx1 = NULL;
      st->next_avail_idx1 = NULL;
    }
    buddy_free_2 (BUDDY_IDX_BLOCK (st, block_idx >> 1, 2), st);
  } else {
    st->bitmap1 |= (1ull << block_idx);
    st->avail_num[1]++;
    if (st->avail_num[1] == 1) {
      st->next_avail_idx1 = tls->avail1_list_head;
      if (tls->avail1_list_head != NULL) tls->avail1_list_head->prev_avail_idx1 = st;
      tls->avail1_list_head = st;
    }
  }
}

void buddy_free_0 (void * ptr, void * ctx) {
  struct buddy_tls_t * tls = &buddy_tls[get_thread_id ()];
  struct buddy_state * st = (struct buddy_state *) ctx;
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
      if (tls->avail0_list_head == st) tls->avail0_list_head = st->next_avail_idx0;
      st->prev_avail_idx0 = NULL;
      st->next_avail_idx0 = NULL;
    }
    buddy_free_1 (BUDDY_IDX_BLOCK (st, block_idx >> 1, 1), st);
  } else {
    if (block_idx < 64) st->bitmap0[0] |= (1ull << block_idx); else st->bitmap0[1] |= (1ull << (block_idx - 64));
    st->avail_num[0]++;
    if (st->avail_num[0] == 1) {
      st->next_avail_idx0 = tls->avail0_list_head;
      if (tls->avail0_list_head != NULL) tls->avail0_list_head->prev_avail_idx0 = st;
      tls->avail0_list_head = st;
    }
  }
}
