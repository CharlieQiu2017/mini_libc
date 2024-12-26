#ifndef TLS_H
#define TLS_H

#include <stdint.h>

static inline __attribute__((always_inline)) void * get_thread_pointer (void) {
  void * addr;
  asm volatile ("mrs %0, tpidr_el0\n" : "=r" (addr));
  return addr;
}

static inline __attribute__((always_inline)) void set_thread_pointer (void * ptr) {
  asm volatile ("msr tpidr_el0, %[gs]\n" : : [gs] "r" (ptr) : "memory");
}

struct tls_struct {
  uint16_t thread_id;
};

static inline __attribute__((always_inline)) uint16_t get_thread_id (void) {
  return ((struct tls_struct *) get_thread_pointer ()) -> thread_id;
}

#endif
