#include <stddef.h>
#include <new>
#include <exception>

extern "C" void * malloc (size_t size);
extern "C" void * aligned_alloc (size_t alignment, size_t size);
extern "C" void free (void * ptr);

void * operator new (std::size_t size) {
  void * ptr = malloc (size);
  if (ptr == NULL) {
    std::terminate ();
  } else return ptr;
}

void * operator new[] (std::size_t size) {
  void * ptr = malloc (size);
  if (ptr == NULL) {
    std::terminate ();
  } else return ptr;
}

void * operator new (std::size_t size, [[maybe_unused]] const std::nothrow_t& tag) noexcept {
  return malloc (size);
}

void * operator new[] (std::size_t size, [[maybe_unused]] const std::nothrow_t& tag) noexcept {
  return malloc (size);
}

void operator delete (void * ptr) noexcept {
  free (ptr);
}

void operator delete[] (void * ptr) noexcept {
  free (ptr);
}

void operator delete (void * ptr, [[maybe_unused]] std::size_t size) noexcept {
  free (ptr);
}

void operator delete[] (void * ptr, [[maybe_unused]] std::size_t size) noexcept {
  free (ptr);
}

void operator delete(void * ptr, [[maybe_unused]] const std::nothrow_t& tag) noexcept {
  free (ptr);
}

void operator delete[] (void * ptr, [[maybe_unused]] const std::nothrow_t& tag) noexcept {
  free (ptr);
}

void * operator new(std::size_t size, std::align_val_t alignment) {
  void * ptr = aligned_alloc ((size_t) alignment, size);
  if (ptr == NULL) {
    std::terminate ();
  } else return ptr;
}

void * operator new(std::size_t size, std::align_val_t alignment, [[maybe_unused]] const std::nothrow_t& tag) noexcept {
  return aligned_alloc ((size_t) alignment, size);
}

void operator delete(void * ptr, [[maybe_unused]] std::align_val_t alignment) noexcept {
  free (ptr);
}

void operator delete(void * ptr, [[maybe_unused]] std::align_val_t alignment, [[maybe_unused]] const std::nothrow_t& tag) noexcept {
  free (ptr);
}

void * operator new[](std::size_t size, std::align_val_t alignment) {
  void * ptr = aligned_alloc ((size_t) alignment, size);
  if (ptr == NULL) {
    std::terminate ();
  } else return ptr;
}

void * operator new[](std::size_t size, std::align_val_t alignment, [[maybe_unused]] const std::nothrow_t& tag) noexcept {
  return aligned_alloc ((size_t) alignment, size);
}

void operator delete[](void * ptr, [[maybe_unused]] std::align_val_t alignment) noexcept {
  free (ptr);
}

void operator delete[](void * ptr, [[maybe_unused]] std::align_val_t alignment, [[maybe_unused]] const std::nothrow_t&) noexcept {
  free (ptr);
}

void operator delete(void * ptr, [[maybe_unused]] std::size_t size, [[maybe_unused]] std::align_val_t alignment) noexcept {
  free (ptr);
}

void operator delete[](void * ptr, [[maybe_unused]] std::size_t size, [[maybe_unused]] std::align_val_t alignment) noexcept {
  free (ptr);
}
