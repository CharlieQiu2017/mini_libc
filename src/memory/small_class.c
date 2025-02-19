#include <stdint.h>
#include <memory.h>
#include <tls.h>
#include <config.h>

/* The small-class allocator manages small allocations (smaller than 2048 bytes).
   We request blocks of size 65536 (16 pages) from the buddy allocator, and divide them into small slots in multiples of 32 bytes.
   Small classes are 32, 64, 96, ..., 512, 1024, 2048
 */

struct small_class_tls_t {
  struct class_area *small_class_avail_lists[16];
  struct class_area *class1024_avail_list;
  struct class_area *class2048_avail_list;
};

static struct small_class_tls_t small_class_tls[LIBC_MAX_THREAD_NUM];

#define SMALL_CLASS_IDX_SLOT(area_, idx, size) ((void *) ((&(area_)->area[0]) + (idx) * (size)))
#define SMALL_CLASS_SLOT_IDX(area_, slot, size) ((((uintptr_t) (slot)) - ((uintptr_t) (&(area_)->area[0]))) / (size))

/* allocate_class_area
   Allocate new class area using buddy allocator.
   size must be 32, 64, 96, ..., 512, or 1024, 2048.
   The new area is added to the corresponding list.
   Upon failure, the corresponding list is unmodified.
 */
static void allocate_class_area (uint64_t size) {
  struct small_class_tls_t * tls = &small_class_tls[get_thread_id ()];
  struct class_area * ptr;
  struct buddy_state * buddy_st;

  struct class_area ** list_head;
  if (size <= 512) {
    uint32_t cls_idx = size / 32 - 1;
    list_head = &(tls->small_class_avail_lists[cls_idx]);
  } else if (size == 1024) {
    list_head = &tls->class1024_avail_list;
  } else {
    list_head = &tls->class2048_avail_list;
  }

  ptr = buddy_alloc_4 ((void *) &buddy_st);
  if (ptr == NULL) return;
  ptr->buddy_area = buddy_st;
  ptr->prev_avail_area = NULL;

  ptr->next_avail_area = *list_head;
  if (*list_head != NULL) (*list_head)->prev_avail_area = ptr;
  *list_head = ptr;

  uint32_t avail_num = (65536 - CLASS_AREA_HEADER_SIZE) / size;
  ptr->avail_num = avail_num;
  for (uint32_t i = 0; i < avail_num / 64; ++i) ptr->bitmap[i] = ~ 0ull;
  uint32_t avail_num_rem = avail_num % 64;
  ptr->bitmap[avail_num / 64] = (1ull << avail_num_rem) - 1;
}

void * small_alloc (size_t len, void ** ctx_ptr) {
  struct class_area ** out_class_area = (struct class_area **) ctx_ptr;
  struct small_class_tls_t * tls = &small_class_tls[get_thread_id ()];

  struct class_area ** list_head;
  if (len <= 512) {
    uint32_t cls_idx = len / 32 - 1;
    list_head = &(tls->small_class_avail_lists[cls_idx]);
  } else if (len == 1024) {
    list_head = &tls->class1024_avail_list;
  } else {
    list_head = &tls->class2048_avail_list;
  }

  if (*list_head == NULL) allocate_class_area (len);
  if (*list_head == NULL) {
    *out_class_area = NULL;
    return NULL;
  }

  struct class_area * area = *list_head;
  for (uint32_t i = 0; i < 32; ++i) {
    if (area->bitmap[i] != 0) {
      uint32_t idx = __builtin_ctzll (area->bitmap[i]);
      area->bitmap[i] &= ~ (1ull << idx);
      *out_class_area = area;
      area->avail_num--;
      if (area->avail_num == 0) {
	if (area->next_avail_area != NULL) area->next_avail_area->prev_avail_area = area->prev_avail_area;
	*list_head = area->next_avail_area;
	area->next_avail_area = NULL;
      }
      return SMALL_CLASS_IDX_SLOT (area, 64 * i + idx, len);
    }
  }

  /* Should not reach here */
  *out_class_area = NULL;
  return NULL;
}

void small_free (void * ptr, void * ctx, size_t len) {
  struct class_area * area = (struct class_area *) ctx;
  struct small_class_tls_t * tls = &small_class_tls[get_thread_id ()];
  uint32_t idx = SMALL_CLASS_SLOT_IDX (area, ptr, len);

  struct class_area ** list_head;
  if (len <= 512) {
    uint32_t cls_idx = len / 32 - 1;
    list_head = &(tls->small_class_avail_lists[cls_idx]);
  } else if (len == 1024) {
    list_head = &tls->class1024_avail_list;
  } else {
    list_head = &tls->class2048_avail_list;
  }

  area->bitmap[idx / 64] |= (1ull << (idx % 64));
  area->avail_num++;
  if (area->avail_num == 1) {
    area->next_avail_area = *list_head;
    if (*list_head != NULL) (*list_head)->prev_avail_area = area;
    *list_head = area;
  } else if (area->avail_num == (65536 - CLASS_AREA_HEADER_SIZE) / len) {
    /* If the current area is the only area of this class with empty slots, do not free it,
       since we anticipate there will be more allocations later.
     */
    if (area->prev_avail_area != NULL || area->next_avail_area != NULL) {
      if (area->prev_avail_area != NULL) area->prev_avail_area->next_avail_area = area->next_avail_area;
      if (area->next_avail_area != NULL) area->next_avail_area->prev_avail_area = area->prev_avail_area;
      if (*list_head == area) *list_head = area->next_avail_area;
      buddy_free_4 (area->buddy_area, area);
    }
  }
}
