#include <stdint.h>
#include <memory.h>
#include <io.h>
#include <syscall.h>
#include <syscall_nr.h>

void * mmap (void * addr, size_t len, int prot, int flags, fd_t fd, ssize_t offset) {
  return (void *) syscall6 ((long) addr, len, prot, flags, fd, offset, __NR_mmap);
}

int munmap (void * addr, size_t len) {
  return syscall2 ((long) addr, len, __NR_munmap);
}

struct mmap_record {
  size_t len; /* If len == 0, this record is not in use */
  void * ptr;
  /* If this record is not in use, prev and next refer to the list of inactive records.
     If this record is in use, they refer to the list of active records.
   */
  struct mmap_record * prev, * next;
};

#define MMAP_RECORDS_PER_GROUP 127

struct mmap_record_group {
  struct mmap_record_group * prev_group, * next_group;
  struct mmap_record records[MMAP_RECORDS_PER_GROUP];
};

_Static_assert (sizeof (struct mmap_record_group) == 4080, "mmap_record_group layout incorrect");

static void allocate_mmap_record_group (struct mmap_arena_t * arena) {
  struct mmap_record_group * new_group = mmap (NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  if (((intptr_t) new_group) < 0) return;
  new_group->next_group = arena->group_list_head;
  if (arena->group_list_head != NULL) arena->group_list_head->prev_group = new_group;
  arena->group_list_head = new_group;

  /* mmap always returns zero pages. Therefore, fields that should be zeroized are already zeroized. */

  for (uint32_t i = 0; i < MMAP_RECORDS_PER_GROUP - 1; i++) {
    new_group->records[i].next = &new_group->records[i + 1];
    new_group->records[i + 1].prev = &new_group->records[i];
  }
  new_group->records[MMAP_RECORDS_PER_GROUP - 1].next = arena->inactive_list_head;
  if (arena->inactive_list_head != NULL) arena->inactive_list_head->prev = &new_group->records[MMAP_RECORDS_PER_GROUP - 1];
  arena->inactive_list_head = &new_group->records[0];
}

/* Since this is an internal interface, we assume the caller ensures len is a multiple of 4096 and len < 1ull << 48. */
void * mmap_alloc (size_t len, void ** ctx_ptr, void * arena_vp) {
  struct mmap_record ** out_mmap_record = (struct mmap_record **) ctx_ptr;
  struct mmap_arena_t * arena = (struct mmap_arena_t *) arena_vp;
  struct mmap_record * st;

  if (arena->inactive_list_head == NULL) allocate_mmap_record_group (arena);
  if (arena->inactive_list_head == NULL) { *out_mmap_record = NULL; return NULL; }

  void * ptr = mmap (NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  if (((intptr_t) ptr) < 0) { *out_mmap_record = NULL; return NULL; }

  st = arena->inactive_list_head;
  if (st->next != NULL) st->next->prev = NULL;
  arena->inactive_list_head = arena->inactive_list_head->next;

  st->len = len;
  st->ptr = ptr;

  st->next = arena->active_list_head;
  if (arena->active_list_head != NULL) arena->active_list_head->prev = st;
  arena->active_list_head = st;

  *out_mmap_record = st;
  return ptr;
}

void mmap_free (__attribute__((unused)) void * ptr, void * ctx, void * arena_vp) {
  struct mmap_arena_t * arena = (struct mmap_arena_t *) arena_vp;
  struct mmap_record * st = (struct mmap_record *) ctx;

  munmap (st->ptr, st->len);

  st->len = 0;
  st->ptr = NULL;
  if (st->next != NULL) st->next->prev = st->prev;
  if (st->prev != NULL) {
    st->prev->next = st->next;
    st->prev = NULL;
  } else {
    arena->active_list_head = st->next;
  }
  st->next = arena->inactive_list_head;
  if (arena->inactive_list_head != NULL) arena->inactive_list_head->prev = st;
  arena->inactive_list_head = st;
}
