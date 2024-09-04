#include <stdint.h>
#include <memory.h>

/* get_class
   Returns size class for a given requested allocation size.
 */
static uint64_t get_class (uint64_t size) {
  if (size <= 512) {
    return size + ((- size) & 32);
  } else if (size <= 262144) {
    return 1ull << (64 - __builtin_clzll (size - 1));
  } else {
    return size + ((- size) & 4096);
  }
}

void * malloc (uint64_t size) {
  if (!size) return NULL;
  if (size > 1ull << 36) return NULL;

  uint64_t class_size = get_class (size + 16);
  void * ptr;
  if (class_size <= 2048) {

    struct class_area *area;
    uint32_t idx;
    allocate_slot (class_size, &area, &idx);
    if (area == NULL) return NULL;
    ptr = (void *) (((uintptr_t) area) + 288 + class_size * idx);
    *((uint64_t *) ptr) = size;
    *((struct class_area **) (((uintptr_t) ptr) + 8)) = area;
    return (void *) (((uintptr_t) ptr) + 16);

  } else if (class_size <= 262144) {

    struct buddy_state *st = NULL;
    uint32_t idx;
    if (class_size == 4096) {
      allocate_block0 (&st, &idx);
    } else if (class_size == 8192) {
      allocate_block1 (&st, &idx);
    } else if (class_size == 16384) {
      allocate_block2 (&st, &idx);
    } else if (class_size == 32768) {
      allocate_block3 (&st, &idx);
    } else if (class_size == 65536) {
      allocate_block4 (&st, &idx);
    } else if (class_size == 131072) {
      allocate_block5 (&st, &idx);
    } else if (class_size == 262144) {
      allocate_block6 (&st, &idx);
    }

    if (st == NULL) return NULL;
    ptr = (void *) (((uintptr_t) st->chunk) + class_size * idx);
    *((uint64_t *) ptr) = size;
    *((struct buddy_state **) (((uintptr_t) ptr) + 8)) = st;
    return (void *) (((uintptr_t) ptr) + 16);

  } else {

    ptr = get_pages (class_size >> 12);
    if (ptr == NULL) return NULL;
    *((uint64_t *) ptr) = size;
    return (void *) (((uintptr_t) ptr) + 16);

  }
}

void free (void *ptr) {
  if (ptr == NULL) return;

  void *true_ptr = (void *) (((uintptr_t) ptr) - 16);
  uint64_t size = *((uint64_t *) true_ptr);
  uint64_t class_size = get_class (size + 16);

  if (class_size <= 2048) {

    struct class_area *area;
    uint32_t idx;
    area = *((struct class_area **) (((uintptr_t) true_ptr) + 8));
    idx = ((uintptr_t) true_ptr - (((uintptr_t) area) + CLASS_AREA_HEADER_SIZE)) / class_size;
    free_slot (class_size, area, idx);

  } else if (class_size <= 262144) {

    struct buddy_state *st;
    uint32_t idx;
    st = *((struct buddy_state **) (((uintptr_t) true_ptr) + 8));
    idx = ((uintptr_t) true_ptr - ((uintptr_t) st->chunk)) / class_size;

    if (class_size == 4096) {
      free_block0 (st, idx);
    } else if (class_size == 8192) {
      free_block1 (st, idx);
    } else if (class_size == 16384) {
      free_block2 (st, idx);
    } else if (class_size == 32768) {
      free_block3 (st, idx);
    } else if (class_size == 65536) {
      free_block4 (st, idx);
    } else if (class_size == 131072) {
      free_block5 (st, idx);
    } else if (class_size == 262144) {
      free_block6 (st, idx);
    }

  } else {

    free_pages (true_ptr, class_size >> 12);

  }

}
