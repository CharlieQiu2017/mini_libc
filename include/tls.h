#ifndef TLS_H
#define TLS_H

#include <stdint.h>

static inline __attribute__((always_inline)) void * get_thread_pointer (void) {
  void * addr;
  __asm__ volatile ("mrs %0, tpidr_el0" : "=r" (addr));
  return addr;
}

static inline __attribute__((always_inline)) void set_thread_pointer (void * ptr) {
  __asm__ volatile ("msr tpidr_el0, %[gs]" : : [gs] "r" (ptr) : "memory");
}

struct malloc_arena_t;

struct tls_struct {
  uint16_t thread_id;

  /* Memory allocator structures */
  struct malloc_arena_t * malloc_arena;

  /* Random number generator data structure, unused for the moment */
  void * getrandom_opaque_state;

  /* A 50-bit counter that is used to provide unique ID for objects (used in C++) */
  uint64_t counter;
};

static inline __attribute__((always_inline)) uint16_t get_thread_id (void) {
  return ((struct tls_struct *) get_thread_pointer ()) -> thread_id;
}

static inline __attribute__((always_inline)) uint64_t fetch_inc_counter (void) {
  struct tls_struct * tls = (struct tls_struct *) get_thread_pointer ();
  uint64_t ctr = tls->counter++;
  return (ctr & ((1ull << 50) - 1)) | (((uint64_t) tls->thread_id) << 50);
}

#endif
