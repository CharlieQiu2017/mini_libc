#include <stdint.h>
#include <string.h>
#include <memory.h>

/* The buddy allocator manages memory chunks of size 512KiB (128 pages).
   It allocates memory blocks of sizes 1, 2, 4, ..., 64 pages.

   Initially, each chunk is considered as a single block of 128 pages.
   Blocks of 128 pages can be divided into two blocks of 64 pages.
   These blocks can be further divided, until we reach blocks of 1 page.

   Within each chunk, each block can be identified by the pair (order, idx).
   The length of the block is (1 << order) pages, and the index of the first page of the block is (length * idx).
   Hence 0 <= idx < 128.

   If all blocks within a chunk are freed, the chunk is returned to the OS.

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

struct buddy_state_area {
  struct buddy_state_area *prev_area;
  struct buddy_state_area *next_area;
  struct buddy_state state_records[BUDDY_RECORDS_PER_AREA];
};

static struct buddy_state_area *area_list_head;
static struct buddy_state *empty_list_head;
static struct buddy_state *avail6_list_head;
static struct buddy_state *avail5_list_head;
static struct buddy_state *avail4_list_head;
static struct buddy_state *avail3_list_head;
static struct buddy_state *avail2_list_head;
static struct buddy_state *avail1_list_head;
static struct buddy_state *avail0_list_head;

/* allocate_buddy_state_area
   Allocate new struct buddy_state_area.
   Add newly allocated buddy_state records to the empty record list.
   Returns nothing. If failed, empty_list_head remains unchanged.
 */

static void allocate_buddy_state_area (void) {
  struct buddy_state_area *new_area = get_pages (16);
  if (new_area == NULL) return;

  new_area->next_area = area_list_head;
  if (area_list_head != NULL) area_list_head->prev_area = new_area;
  area_list_head = new_area;

  for (uint32_t i = 1; i < BUDDY_RECORDS_PER_AREA - 1; ++i) {
    new_area->state_records[i].next_empty_state = &(new_area->state_records[i + 1]);
    new_area->state_records[i].prev_empty_state = &(new_area->state_records[i - 1]);
  }
  new_area->state_records[0].next_empty_state = &(new_area->state_records[1]);
  new_area->state_records[BUDDY_RECORDS_PER_AREA - 1].prev_empty_state = &(new_area->state_records[BUDDY_RECORDS_PER_AREA - 2]);
  new_area->state_records[BUDDY_RECORDS_PER_AREA - 1].next_empty_state = empty_list_head;
  if (empty_list_head != NULL) empty_list_head->prev_empty_state = &(new_area->state_records[BUDDY_RECORDS_PER_AREA - 1]);
  empty_list_head = &(new_area->state_records[0]);
}

/* allocate_buddy_state
   Allocate a new not-in-use struct buddy_state and mark it as in-use.
   Returns ptr to buddy_state; returns NULL upon failure.
 */

static struct buddy_state * allocate_buddy_state (void) {
  if (empty_list_head == NULL) allocate_buddy_state_area ();
  if (empty_list_head == NULL) return NULL;

  struct buddy_state *new_state = empty_list_head;
  empty_list_head = new_state->next_empty_state;
  if (empty_list_head != NULL) empty_list_head->prev_empty_state = NULL;
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
  memset (ptr, 0, sizeof (struct buddy_state));
  ptr->next_empty_state = empty_list_head;
  if (empty_list_head != NULL) empty_list_head->prev_empty_state = ptr;
  empty_list_head = ptr;
}

/* allocate_chunk_and_block6
   Allocate a new chunk, and a new block of order 6 from the chunk.
   Returns ptr to buddy_state of new chunk.
   The new block has index 0 within the new chunk.
   Returns NULL upon failure.
 */

static struct buddy_state * allocate_chunk_and_block6 (void) {
  struct buddy_state *new_state = allocate_buddy_state ();
  if (new_state == NULL) return NULL;

  new_state->chunk = get_pages (128);
  if (new_state->chunk == NULL) {
    free_buddy_state (new_state);
    return NULL;
  }

  new_state->bitmap6 = 2;
  new_state->avail_num[6] = 1;
  new_state->next_avail_idx6 = avail6_list_head;
  if (avail6_list_head != NULL) avail6_list_head->prev_avail_idx6 = new_state;
  avail6_list_head = new_state;

  return new_state;
}

/* allocate_block6
   Allocate a new block of order 6.
   Outputs both buddy_state ptr and block idx.
   out_buddy_state is set to NULL upon failure.
 */

void allocate_block6 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx) {
  struct buddy_state *st;

  if (avail6_list_head != NULL) {

    st = avail6_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap6);
    st->bitmap6 &= ~ (1ull << idx);
    st->avail_num[6]--;

    if (st->avail_num[6] == 0) {
      if (st->next_avail_idx6 != NULL) st->next_avail_idx6->prev_avail_idx6 = NULL;
      st->next_avail_idx6 = NULL;
      avail6_list_head = st->next_avail_idx6;
    }

    *out_buddy_state = st;
    *out_block_idx = idx;

  } else {

    st = allocate_chunk_and_block6 ();
    if (st != NULL) {
      *out_buddy_state = st;
      *out_block_idx = 0;
      return;
    } else {
      *out_buddy_state = NULL;
      return;
    }

  }
}

void allocate_block5 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx) {
  struct buddy_state *st;

  if (avail5_list_head != NULL) {

    st = avail5_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap5);
    st->bitmap5 &= ~ (1ull << idx);
    st->avail_num[5]--;

    if (st->avail_num[5] == 0) {
      if (st->next_avail_idx5 != NULL) st->next_avail_idx5->prev_avail_idx5 = NULL;
      st->next_avail_idx5 = NULL;
      avail5_list_head = st->next_avail_idx5;
    }

    *out_buddy_state = st;
    *out_block_idx = idx;

  } else {

    uint32_t idx;
    allocate_block6 (&st, &idx);
    if (st != NULL) {
      st->avail_num[5]++;
      st->bitmap5 |= (1ull << (2 * idx + 1));
      if (st->avail_num[5] == 1) {
	st->next_avail_idx5 = avail5_list_head;
	if (avail5_list_head != NULL) avail5_list_head->prev_avail_idx5 = st;
	avail5_list_head = st;
      }
      *out_buddy_state = st;
      *out_block_idx = 2 * idx;
      return;
    } else {
      *out_buddy_state = NULL;
      return;
    }

  }
}

void allocate_block4 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx) {
  struct buddy_state *st;

  if (avail4_list_head != NULL) {

    st = avail4_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap4);
    st->bitmap4 &= ~ (1ull << idx);
    st->avail_num[4]--;

    if (st->avail_num[4] == 0) {
      if (st->next_avail_idx4 != NULL) st->next_avail_idx4->prev_avail_idx4 = NULL;
      st->next_avail_idx4 = NULL;
      avail4_list_head = st->next_avail_idx4;
    }

    *out_buddy_state = st;
    *out_block_idx = idx;

  } else {

    uint32_t idx;
    allocate_block5 (&st, &idx);
    if (st != NULL) {
      st->avail_num[4]++;
      st->bitmap4 |= (1ull << (2 * idx + 1));
      if (st->avail_num[4] == 1) {
	st->next_avail_idx4 = avail4_list_head;
	if (avail4_list_head != NULL) avail4_list_head->prev_avail_idx4 = st;
	avail4_list_head = st;
      }
      *out_buddy_state = st;
      *out_block_idx = 2 * idx;
      return;
    } else {
      *out_buddy_state = NULL;
      return;
    }

  }
}

void allocate_block3 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx) {
  struct buddy_state *st;

  if (avail3_list_head != NULL) {

    st = avail3_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap3);
    st->bitmap3 &= ~ (1ull << idx);
    st->avail_num[3]--;

    if (st->avail_num[3] == 0) {
      if (st->next_avail_idx3 != NULL) st->next_avail_idx3->prev_avail_idx3 = NULL;
      st->next_avail_idx3 = NULL;
      avail3_list_head = st->next_avail_idx3;
    }

    *out_buddy_state = st;
    *out_block_idx = idx;

  } else {

    uint32_t idx;
    allocate_block4 (&st, &idx);
    if (st != NULL) {
      st->avail_num[3]++;
      st->bitmap3 |= (1ull << (2 * idx + 1));
      if (st->avail_num[3] == 1) {
	st->next_avail_idx3 = avail3_list_head;
	if (avail3_list_head != NULL) avail3_list_head->prev_avail_idx3 = st;
	avail3_list_head = st;
      }
      *out_buddy_state = st;
      *out_block_idx = 2 * idx;
      return;
    } else {
      *out_buddy_state = NULL;
      return;
    }

  }
}

void allocate_block2 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx) {
  struct buddy_state *st;

  if (avail2_list_head != NULL) {

    st = avail2_list_head;
    uint32_t idx = __builtin_ctz (st->bitmap2);
    st->bitmap2 &= ~ (1ull << idx);
    st->avail_num[2]--;

    if (st->avail_num[2] == 0) {
      if (st->next_avail_idx2 != NULL) st->next_avail_idx2->prev_avail_idx2 = NULL;
      st->next_avail_idx2 = NULL;
      avail2_list_head = st->next_avail_idx2;
    }

    *out_buddy_state = st;
    *out_block_idx = idx;

  } else {

    uint32_t idx;
    allocate_block3 (&st, &idx);
    if (st != NULL) {
      st->avail_num[2]++;
      st->bitmap2 |= (1ull << (2 * idx + 1));
      if (st->avail_num[2] == 1) {
	st->next_avail_idx2 = avail2_list_head;
	if (avail2_list_head != NULL) avail2_list_head->prev_avail_idx2 = st;
	avail2_list_head = st;
      }
      *out_buddy_state = st;
      *out_block_idx = 2 * idx;
      return;
    } else {
      *out_buddy_state = NULL;
      return;
    }

  }
}

void allocate_block1 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx) {
  struct buddy_state *st;

  if (avail1_list_head != NULL) {

    st = avail1_list_head;
    uint32_t idx = __builtin_ctzll (st->bitmap1);
    st->bitmap1 &= ~ (1ull << idx);
    st->avail_num[1]--;

    if (st->avail_num[1] == 0) {
      if (st->next_avail_idx1 != NULL) st->next_avail_idx1->prev_avail_idx1 = NULL;
      st->next_avail_idx1 = NULL;
      avail1_list_head = st->next_avail_idx1;
    }

    *out_buddy_state = st;
    *out_block_idx = idx;

  } else {

    uint32_t idx;
    allocate_block2 (&st, &idx);
    if (st != NULL) {
      st->avail_num[1]++;
      st->bitmap1 |= (1ull << (2 * idx + 1));
      if (st->avail_num[1] == 1) {
	st->next_avail_idx1 = avail1_list_head;
	if (avail1_list_head != NULL) avail1_list_head->prev_avail_idx1 = st;
	avail1_list_head = st;
      }
      *out_buddy_state = st;
      *out_block_idx = 2 * idx;
      return;
    } else {
      *out_buddy_state = NULL;
      return;
    }

  }
}

void allocate_block0 (struct buddy_state **out_buddy_state, uint32_t *out_block_idx) {
  struct buddy_state *st;

  if (avail0_list_head != NULL) {

    st = avail0_list_head;
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
      avail0_list_head = st->next_avail_idx0;
    }

    *out_buddy_state = st;
    *out_block_idx = idx;

  } else {

    uint32_t idx;
    allocate_block1 (&st, &idx);
    if (st != NULL) {
      st->avail_num[0]++;
      if (idx < 32) {
	st->bitmap0[0] |= (1ull << (2 * idx + 1));
      } else {
	st->bitmap0[1] |= (1ull << (2 * (idx - 32) + 1));
      }
      if (st->avail_num[0] == 1) {
	st->next_avail_idx0 = avail0_list_head;
	if (avail0_list_head != NULL) avail0_list_head->prev_avail_idx0 = st;
	avail0_list_head = st;
      }
      *out_buddy_state = st;
      *out_block_idx = 2 * idx;
      return;
    } else {
      *out_buddy_state = NULL;
      return;
    }

  }
}

void free_block6 (struct buddy_state *st, uint32_t block_idx) {
  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap6 & (1ull << buddy)) {
    /* At this point, no smaller blocks should be available */
    if (st->prev_avail_idx6 != NULL) st->prev_avail_idx6->next_avail_idx6 = st->next_avail_idx6;
    if (st->next_avail_idx6 != NULL) st->next_avail_idx6->prev_avail_idx6 = st->prev_avail_idx6;
    if (avail6_list_head == st) avail6_list_head = st->next_avail_idx6;
    free_pages (st->chunk, 128);
    free_buddy_state (st);
  } else {
    st->bitmap6 |= (1ull << block_idx);
    st->avail_num[6]++;
    if (st->avail_num[6] == 1) {
      st->next_avail_idx6 = avail6_list_head;
      if (avail6_list_head != NULL) avail6_list_head->prev_avail_idx6 = st;
      avail6_list_head = st;
    }
  }
}

void free_block5 (struct buddy_state *st, uint32_t block_idx) {
  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap5 & (1ull << buddy)) {
    st->bitmap5 &= ~ (1ull << buddy);
    st->avail_num[5]--;
    if (st->avail_num[5] == 0) {
      if (st->prev_avail_idx5 != NULL) st->prev_avail_idx5->next_avail_idx5 = st->next_avail_idx5;
      if (st->next_avail_idx5 != NULL) st->next_avail_idx5->prev_avail_idx5 = st->prev_avail_idx5;
      if (avail5_list_head == st) avail5_list_head = st->next_avail_idx5;
      st->prev_avail_idx5 = NULL;
      st->next_avail_idx5 = NULL;
    }
    free_block6 (st, block_idx >> 1);
  } else {
    st->bitmap5 |= (1ull << block_idx);
    st->avail_num[5]++;
    if (st->avail_num[5] == 1) {
      st->next_avail_idx5 = avail5_list_head;
      if (avail5_list_head != NULL) avail5_list_head->prev_avail_idx5 = st;
      avail5_list_head = st;
    }
  }
}

void free_block4 (struct buddy_state *st, uint32_t block_idx) {
  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap4 & (1ull << buddy)) {
    st->bitmap4 &= ~ (1ull << buddy);
    st->avail_num[4]--;
    if (st->avail_num[4] == 0) {
      if (st->prev_avail_idx4 != NULL) st->prev_avail_idx4->next_avail_idx4 = st->next_avail_idx4;
      if (st->next_avail_idx4 != NULL) st->next_avail_idx4->prev_avail_idx4 = st->prev_avail_idx4;
      if (avail4_list_head == st) avail4_list_head = st->next_avail_idx4;
      st->prev_avail_idx4 = NULL;
      st->next_avail_idx4 = NULL;
    }
    free_block5 (st, block_idx >> 1);
  } else {
    st->bitmap4 |= (1ull << block_idx);
    st->avail_num[4]++;
    if (st->avail_num[4] == 1) {
      st->next_avail_idx4 = avail4_list_head;
      if (avail4_list_head != NULL) avail4_list_head->prev_avail_idx4 = st;
      avail4_list_head = st;
    }
  }
}

void free_block3 (struct buddy_state *st, uint32_t block_idx) {
  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap3 & (1ull << buddy)) {
    st->bitmap3 &= ~ (1ull << buddy);
    st->avail_num[3]--;
    if (st->avail_num[3] == 0) {
      if (st->prev_avail_idx3 != NULL) st->prev_avail_idx3->next_avail_idx3 = st->next_avail_idx3;
      if (st->next_avail_idx3 != NULL) st->next_avail_idx3->prev_avail_idx3 = st->prev_avail_idx3;
      if (avail3_list_head == st) avail3_list_head = st->next_avail_idx3;
      st->prev_avail_idx3 = NULL;
      st->next_avail_idx3 = NULL;
    }
    free_block4 (st, block_idx >> 1);
  } else {
    st->bitmap3 |= (1ull << block_idx);
    st->avail_num[3]++;
    if (st->avail_num[3] == 1) {
      st->next_avail_idx3 = avail3_list_head;
      if (avail3_list_head != NULL) avail3_list_head->prev_avail_idx3 = st;
      avail3_list_head = st;
    }
  }
}

void free_block2 (struct buddy_state *st, uint32_t block_idx) {
  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap2 & (1ull << buddy)) {
    st->bitmap2 &= ~ (1ull << buddy);
    st->avail_num[2]--;
    if (st->avail_num[2] == 0) {
      if (st->prev_avail_idx2 != NULL) st->prev_avail_idx2->next_avail_idx2 = st->next_avail_idx2;
      if (st->next_avail_idx2 != NULL) st->next_avail_idx2->prev_avail_idx2 = st->prev_avail_idx2;
      if (avail2_list_head == st) avail2_list_head = st->next_avail_idx2;
      st->prev_avail_idx2 = NULL;
      st->next_avail_idx2 = NULL;
    }
    free_block3 (st, block_idx >> 1);
  } else {
    st->bitmap2 |= (1ull << block_idx);
    st->avail_num[2]++;
    if (st->avail_num[2] == 1) {
      st->next_avail_idx2 = avail2_list_head;
      if (avail2_list_head != NULL) avail2_list_head->prev_avail_idx2 = st;
      avail2_list_head = st;
    }
  }
}

void free_block1 (struct buddy_state *st, uint32_t block_idx) {
  uint32_t buddy = block_idx ^ 1;
  if (st->bitmap1 & (1ull << buddy)) {
    st->bitmap1 &= ~ (1ull << buddy);
    st->avail_num[1]--;
    if (st->avail_num[1] == 0) {
      if (st->prev_avail_idx1 != NULL) st->prev_avail_idx1->next_avail_idx1 = st->next_avail_idx1;
      if (st->next_avail_idx1 != NULL) st->next_avail_idx1->prev_avail_idx1 = st->prev_avail_idx1;
      if (avail1_list_head == st) avail1_list_head = st->next_avail_idx1;
      st->prev_avail_idx1 = NULL;
      st->next_avail_idx1 = NULL;
    }
    free_block2 (st, block_idx >> 1);
  } else {
    st->bitmap1 |= (1ull << block_idx);
    st->avail_num[1]++;
    if (st->avail_num[1] == 1) {
      st->next_avail_idx1 = avail1_list_head;
      if (avail1_list_head != NULL) avail1_list_head->prev_avail_idx1 = st;
      avail1_list_head = st;
    }
  }
}

void free_block0 (struct buddy_state *st, uint32_t block_idx) {
  uint32_t buddy = block_idx ^ 1;
  uint32_t merge_cond0 = block_idx < 64 && (st->bitmap0[0] & (1ull << buddy));
  uint32_t merge_cond1 = block_idx >= 64 && (st->bitmap0[1] & (1ull << (buddy - 64)));
  if (merge_cond0 || merge_cond1) {
    if (merge_cond0) st->bitmap0[0] &= ~ (1ull << buddy); else st->bitmap0[1] &= ~ (1ull << (buddy - 64));
    st->avail_num[0]--;
    if (st->avail_num[0] == 0) {
      if (st->prev_avail_idx0 != NULL) st->prev_avail_idx0->next_avail_idx0 = st->next_avail_idx0;
      if (st->next_avail_idx0 != NULL) st->next_avail_idx0->prev_avail_idx0 = st->prev_avail_idx0;
      if (avail0_list_head == st) avail0_list_head = st->next_avail_idx0;
      st->prev_avail_idx0 = NULL;
      st->next_avail_idx0 = NULL;
    }
    free_block1 (st, block_idx >> 1);
  } else {
    if (block_idx < 64) st->bitmap0[0] |= (1ull << block_idx); else st->bitmap0[1] |= (1ull << (block_idx - 64));
    st->avail_num[0]++;
    if (st->avail_num[0] == 1) {
      st->next_avail_idx0 = avail0_list_head;
      if (avail0_list_head != NULL) avail0_list_head->prev_avail_idx0 = st;
      avail0_list_head = st;
    }
  }
}
