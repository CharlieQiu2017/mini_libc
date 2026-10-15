#ifndef HIVE_HPP
#define HIVE_HPP

#include <stdint.h>
#include <tls.h>
#include <exception>
#include <type_traits.hpp>
#include <tagged_ptr.hpp>
#include <raw_dyn_array.hpp>
#include <utility.hpp>
#include <counter.hpp>
#include <pair.hpp>

namespace minilib {

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class hive;

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class hive_handle;

namespace detail {

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class hive_chunk_constexpr;

template < typename T >
using hive_chunk_ptr = minilib::tagged_ptr < minilib::detail::hive_chunk_constexpr < T > >;

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class hive_chunk_constexpr {
public:
  hive_chunk_ptr < T > prev_empty;
  hive_chunk_ptr < T > next_empty;
  hive_chunk_ptr < T > prev_filled;
  hive_chunk_ptr < T > next_filled;
  uint32_t log2_len;
  uint32_t filled_count;
  minilib::raw_dyn_array < uint64_t > bitmap;
  minilib::raw_dyn_array < T > storage;

  constexpr hive_chunk_constexpr ()
    : prev_empty (), next_empty (), prev_filled (), next_filled (),
      log2_len (0), filled_count (0), bitmap (), storage () {}

  hive_chunk_constexpr (const hive_chunk_constexpr&) = delete;
  hive_chunk_constexpr (hive_chunk_constexpr&&) = delete;
  hive_chunk_constexpr& operator= (const hive_chunk_constexpr&) = delete;
  hive_chunk_constexpr& operator= (hive_chunk_constexpr&&) = delete;
  constexpr ~hive_chunk_constexpr () = default;
};

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
struct hive_chunk_runtime {
  hive_chunk_ptr < T > prev_empty;
  hive_chunk_ptr < T > next_empty;
  hive_chunk_ptr < T > prev_filled;
  hive_chunk_ptr < T > next_filled;
  uint32_t log2_len;
  uint32_t filled_count;
};

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
struct hive_runtime {
  size_t len;
  size_t container_id;
  hive_chunk_ptr < T > empty_head;
  hive_chunk_ptr < T > filled_head;
  hive_chunk_ptr < T > filled_tail;
  uint32_t next_log2_len;
};

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
struct hive_constexpr {
  struct hive_runtime < T > runtime;
  minilib::counter local_counter;
  minilib::counter * global_counter;
};

constexpr uint32_t count_trailing_zeros_u64 (uint64_t val) {
  if (val == 0) return 64;
  return __builtin_ctzll (val);
}

constexpr uint32_t count_leading_zeros_u64 (uint64_t val) {
  if (val == 0) return 64;
  return __builtin_clzll (val);
}

}

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class hive_handle {
  minilib::counted_ref < minilib::detail::hive_chunk_constexpr < T > > ref_;
  uint32_t offset_;

public:
  constexpr hive_handle () : ref_ (), offset_ (0) {}

  constexpr hive_handle (const minilib::counted_ref < minilib::detail::hive_chunk_constexpr < T > > & ref, uint32_t offset)
    : ref_ (ref), offset_ (offset) {}

  constexpr hive_handle (const hive_handle& other) = default;
  constexpr hive_handle (hive_handle&& other) = default;
  constexpr hive_handle& operator= (const hive_handle& other) = default;
  constexpr hive_handle& operator= (hive_handle&& other) = default;

  constexpr hive_handle& operator= (const std::nullptr_t&) {
    ref_ = nullptr;
    offset_ = 0;
    return *this;
  }

  constexpr ~hive_handle () = default;

  constexpr explicit operator bool () const {
    return static_cast < bool > (ref_);
  }

  constexpr uint64_t get_id () const {
    return ref_.get_id ();
  }

  constexpr bool check (uint64_t id) const {
    return ref_.check (id);
  }

  constexpr uint32_t offset () const {
    return offset_;
  }

  constexpr const minilib::counted_ref < minilib::detail::hive_chunk_constexpr < T > > & ref () const {
    return ref_;
  }

  constexpr minilib::counted_ref < minilib::detail::hive_chunk_constexpr < T > > & ref () {
    return ref_;
  }

  constexpr bool operator== (const hive_handle& other) const {
    if (! ref_ && ! other.ref_) return true;
    return (ref_ == other.ref_) && (offset_ == other.offset_);
  }

  friend class hive < T >;
};

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class hive {
public:
  using handle_type = minilib::hive_handle < T >;
  using value_type = T;
  using size_type = size_t;

private:
  using chunk_ptr = minilib::detail::hive_chunk_ptr < T >;

  union {
    minilib::detail::hive_runtime < T > runtime_;
    minilib::detail::hive_constexpr < T > * constexpr_;
  };

  constexpr minilib::detail::hive_runtime < T > * get_runtime () {
    if consteval {
      return minilib::addressof (constexpr_->runtime);
    } else {
      return minilib::addressof (runtime_);
    }
  }

  constexpr const minilib::detail::hive_runtime < T > * get_runtime () const {
    if consteval {
      return minilib::addressof (constexpr_->runtime);
    } else {
      return minilib::addressof (runtime_);
    }
  }

  constexpr uint64_t get_local_counter () {
    if consteval {
      return constexpr_->local_counter.get ();
    } else {
      return 0;
    }
  }

  static constexpr chunk_ptr get_prev_empty (chunk_ptr chunk) {
    if consteval {
      return chunk.data_unsafe ()->prev_empty;
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      return rt->prev_empty;
    }
  }

  static constexpr void set_prev_empty (chunk_ptr chunk, chunk_ptr val) {
    if consteval {
      chunk.data_unsafe ()->prev_empty = val;
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      rt->prev_empty = val;
    }
  }

  static constexpr chunk_ptr get_next_empty (chunk_ptr chunk) {
    if consteval {
      return chunk.data_unsafe ()->next_empty;
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      return rt->next_empty;
    }
  }

  static constexpr void set_next_empty (chunk_ptr chunk, chunk_ptr val) {
    if consteval {
      chunk.data_unsafe ()->next_empty = val;
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      rt->next_empty = val;
    }
  }

  static constexpr chunk_ptr get_prev_filled (chunk_ptr chunk) {
    if consteval {
      return chunk.data_unsafe ()->prev_filled;
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      return rt->prev_filled;
    }
  }

  static constexpr void set_prev_filled (chunk_ptr chunk, chunk_ptr val) {
    if consteval {
      chunk.data_unsafe ()->prev_filled = val;
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      rt->prev_filled = val;
    }
  }

  static constexpr chunk_ptr get_next_filled (chunk_ptr chunk) {
    if consteval {
      return chunk.data_unsafe ()->next_filled;
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      return rt->next_filled;
    }
  }

  static constexpr void set_next_filled (chunk_ptr chunk, chunk_ptr val) {
    if consteval {
      chunk.data_unsafe ()->next_filled = val;
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      rt->next_filled = val;
    }
  }

  static constexpr uint32_t get_log2_len (chunk_ptr chunk) {
    if consteval {
      return chunk.data_unsafe ()->log2_len;
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      return rt->log2_len;
    }
  }

  static constexpr uint32_t get_capacity (chunk_ptr chunk) {
    return 1u << get_log2_len (chunk);
  }

  static constexpr uint32_t get_filled_count (chunk_ptr chunk) {
    if consteval {
      return chunk.data_unsafe ()->filled_count;
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      return rt->filled_count;
    }
  }

  static constexpr void set_filled_count (chunk_ptr chunk, uint32_t val) {
    if consteval {
      chunk.data_unsafe ()->filled_count = val;
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      rt->filled_count = val;
    }
  }

  static constexpr uint64_t get_bitmap_word (chunk_ptr chunk, size_t word_idx) {
    if consteval {
      return chunk.data_unsafe ()->bitmap[word_idx];
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      uint64_t * bm = reinterpret_cast < uint64_t * > (reinterpret_cast < char * > (rt) + sizeof (minilib::detail::hive_chunk_runtime < T >));
      return bm[word_idx];
    }
  }

  static constexpr void set_bitmap_word (chunk_ptr chunk, size_t word_idx, uint64_t val) {
    if consteval {
      chunk.data_unsafe ()->bitmap[word_idx] = val;
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      uint64_t * bm = reinterpret_cast < uint64_t * > (reinterpret_cast < char * > (rt) + sizeof (minilib::detail::hive_chunk_runtime < T >));
      bm[word_idx] = val;
    }
  }

  static constexpr bool is_slot_filled (chunk_ptr chunk, uint32_t slot) {
    size_t w = slot >> 6;
    size_t bit = slot & 63;
    return (get_bitmap_word (chunk, w) & (1ull << bit)) != 0;
  }

  static constexpr void set_slot_filled (chunk_ptr chunk, uint32_t slot, bool filled) {
    size_t w = slot >> 6;
    size_t bit = slot & 63;
    uint64_t val = get_bitmap_word (chunk, w);
    if (filled) {
      val |= (1ull << bit);
    } else {
      val &= ~(1ull << bit);
    }
    set_bitmap_word (chunk, w, val);
  }

  static constexpr T * get_slot_ptr (chunk_ptr chunk, uint32_t slot) {
    if consteval {
      return chunk.data_unsafe ()->storage.data () + slot;
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      size_t cap = 1ull << rt->log2_len;
      size_t bm_words = (cap + 63) >> 6;
      size_t raw_offset = sizeof (minilib::detail::hive_chunk_runtime < T >) + bm_words * sizeof (uint64_t);
      size_t storage_offset = (raw_offset + alignof (T) - 1) & ~(alignof (T) - 1);
      T * storage = reinterpret_cast < T * > (reinterpret_cast < char * > (rt) + storage_offset);
      return storage + slot;
    }
  }

  static constexpr uint32_t find_first_empty_slot (chunk_ptr chunk) {
    uint32_t cap = get_capacity (chunk);
    uint32_t num_words = (cap + 63) >> 6;
    for (uint32_t w = 0; w < num_words; ++w) {
      uint64_t val = get_bitmap_word (chunk, w);
      if (w == num_words - 1 && (cap & 63) != 0) {
        val |= (~0ull << (cap & 63));
      }
      if (~val != 0ull) {
        uint32_t bit = minilib::detail::count_trailing_zeros_u64 (~val);
        uint32_t slot = (w << 6) | bit;
        if (slot < cap) return slot;
      }
    }
    return cap;
  }

  static constexpr uint32_t find_first_filled_slot (chunk_ptr chunk) {
    uint32_t cap = get_capacity (chunk);
    uint32_t num_words = (cap + 63) >> 6;
    for (uint32_t w = 0; w < num_words; ++w) {
      uint64_t val = get_bitmap_word (chunk, w);
      if (w == num_words - 1 && (cap & 63) != 0) {
        val &= (1ull << (cap & 63)) - 1;
      }
      if (val != 0ull) {
        uint32_t bit = minilib::detail::count_trailing_zeros_u64 (val);
        uint32_t slot = (w << 6) | bit;
        if (slot < cap) return slot;
      }
    }
    return cap;
  }

  static constexpr uint32_t find_last_filled_slot (chunk_ptr chunk) {
    uint32_t cap = get_capacity (chunk);
    uint32_t num_words = (cap + 63) >> 6;
    for (int32_t w = static_cast < int32_t > (num_words) - 1; w >= 0; --w) {
      uint64_t val = get_bitmap_word (chunk, static_cast < uint32_t > (w));
      if (static_cast < uint32_t > (w) == num_words - 1 && (cap & 63) != 0) {
        val &= (1ull << (cap & 63)) - 1;
      }
      if (val != 0ull) {
        uint32_t bit = 63 - minilib::detail::count_leading_zeros_u64 (val);
        return (static_cast < uint32_t > (w) << 6) | bit;
      }
    }
    return cap;
  }

  static constexpr uint32_t find_next_filled_slot (chunk_ptr chunk, uint32_t cur_slot) {
    uint32_t cap = get_capacity (chunk);
    if (cur_slot + 1 >= cap) return cap;
    uint32_t next_slot = cur_slot + 1;
    uint32_t start_word = next_slot >> 6;
    uint32_t start_bit = next_slot & 63;
    uint32_t num_words = (cap + 63) >> 6;

    for (uint32_t w = start_word; w < num_words; ++w) {
      uint64_t val = get_bitmap_word (chunk, w);
      if (w == start_word && start_bit > 0) {
        val &= (~0ull << start_bit);
      }
      if (w == num_words - 1 && (cap & 63) != 0) {
        val &= (1ull << (cap & 63)) - 1;
      }
      if (val != 0ull) {
        uint32_t bit = minilib::detail::count_trailing_zeros_u64 (val);
        uint32_t found = (w << 6) | bit;
        if (found < cap) return found;
      }
    }
    return cap;
  }

  static constexpr uint32_t find_prev_filled_slot (chunk_ptr chunk, uint32_t cur_slot) {
    if (cur_slot == 0) return get_capacity (chunk);
    uint32_t target_slot = cur_slot - 1;
    uint32_t cap = get_capacity (chunk);
    uint32_t start_word = target_slot >> 6;
    uint32_t start_bit = target_slot & 63;
    uint32_t num_words = (cap + 63) >> 6;

    for (int32_t w = static_cast < int32_t > (start_word); w >= 0; --w) {
      uint64_t val = get_bitmap_word (chunk, static_cast < uint32_t > (w));
      if (static_cast < uint32_t > (w) == start_word && start_bit < 63) {
        val &= (1ull << (start_bit + 1)) - 1;
      }
      if (static_cast < uint32_t > (w) == num_words - 1 && (cap & 63) != 0) {
        val &= (1ull << (cap & 63)) - 1;
      }
      if (val != 0ull) {
        uint32_t bit = 63 - minilib::detail::count_leading_zeros_u64 (val);
        return (static_cast < uint32_t > (w) << 6) | bit;
      }
    }
    return cap;
  }

  constexpr chunk_ptr create_chunk (uint32_t log2_len) {
    auto runtime = get_runtime ();
    uint64_t local_id = get_local_counter ();
    size_t cap = 1ull << log2_len;
    size_t bm_words = (cap + 63) >> 6;
    size_t raw_offset = sizeof (minilib::detail::hive_chunk_runtime < T >) + bm_words * sizeof (uint64_t);
    size_t storage_offset = (raw_offset + alignof (T) - 1) & ~(alignof (T) - 1);
    size_t total_size = storage_offset + cap * sizeof (T);
    size_t total_alignment = alignof (T) > 8 ? alignof (T) : 8;

    auto chunk = chunk_ptr::allocate (runtime->container_id, local_id, total_alignment, total_size);

    if consteval {
      minilib::construct_at < minilib::detail::hive_chunk_constexpr < T > > (chunk.data_unsafe ());
      auto cptr = chunk.data_unsafe ();
      cptr->log2_len = log2_len;
      cptr->filled_count = 0;
      cptr->prev_empty = nullptr;
      cptr->next_empty = nullptr;
      cptr->prev_filled = nullptr;
      cptr->next_filled = nullptr;
      cptr->bitmap.allocate (bm_words);
      for (size_t i = 0; i < bm_words; ++i) {
        cptr->bitmap.construct_at (i, 0ull);
      }
      cptr->storage.allocate (cap);
    } else {
      auto rt = reinterpret_cast < minilib::detail::hive_chunk_runtime < T > * > (chunk.data_unsafe ());
      rt->prev_empty = nullptr;
      rt->next_empty = nullptr;
      rt->prev_filled = nullptr;
      rt->next_filled = nullptr;
      rt->log2_len = log2_len;
      rt->filled_count = 0;
      uint64_t * bm = reinterpret_cast < uint64_t * > (reinterpret_cast < char * > (rt) + sizeof (minilib::detail::hive_chunk_runtime < T >));
      for (size_t i = 0; i < bm_words; ++i) {
        bm[i] = 0ull;
      }
    }
    return chunk;
  }

  constexpr void destroy_chunk (chunk_ptr chunk) {
    if consteval {
      auto cptr = chunk.data_unsafe ();
      cptr->bitmap.deallocate ();
      cptr->storage.deallocate ();
      minilib::destroy_at < minilib::detail::hive_chunk_constexpr < T > > (cptr);
      chunk_ptr::deallocate (chunk);
    } else {
      chunk_ptr::deallocate (chunk);
    }
  }

  constexpr void add_to_empty_list (chunk_ptr chunk) {
    auto runtime = get_runtime ();
    set_next_empty (chunk, runtime->empty_head);
    set_prev_empty (chunk, chunk_ptr ());
    if (runtime->empty_head) {
      set_prev_empty (runtime->empty_head, chunk);
    }
    runtime->empty_head = chunk;
  }

  constexpr void remove_from_empty_list (chunk_ptr chunk) {
    auto runtime = get_runtime ();
    auto prev = get_prev_empty (chunk);
    auto next = get_next_empty (chunk);
    if (prev) {
      set_next_empty (prev, next);
    } else {
      runtime->empty_head = next;
    }
    if (next) {
      set_prev_empty (next, prev);
    }
    set_prev_empty (chunk, chunk_ptr ());
    set_next_empty (chunk, chunk_ptr ());
  }

  constexpr void add_to_filled_list (chunk_ptr chunk) {
    auto runtime = get_runtime ();
    set_next_filled (chunk, chunk_ptr ());
    set_prev_filled (chunk, runtime->filled_tail);
    if (runtime->filled_tail) {
      set_next_filled (runtime->filled_tail, chunk);
    } else {
      runtime->filled_head = chunk;
    }
    runtime->filled_tail = chunk;
  }

  constexpr void remove_from_filled_list (chunk_ptr chunk) {
    auto runtime = get_runtime ();
    auto prev = get_prev_filled (chunk);
    auto next = get_next_filled (chunk);
    if (prev) {
      set_next_filled (prev, next);
    } else {
      runtime->filled_head = next;
    }
    if (next) {
      set_prev_filled (next, prev);
    } else {
      runtime->filled_tail = prev;
    }
    set_prev_filled (chunk, chunk_ptr ());
    set_next_filled (chunk, chunk_ptr ());
  }

  constexpr minilib::pair < chunk_ptr, uint32_t > allocate_slot () {
    auto runtime = get_runtime ();
    chunk_ptr chunk = runtime->empty_head;
    if (! chunk) {
      uint32_t log2_len = runtime->next_log2_len;
      if (runtime->next_log2_len < 16) {
        runtime->next_log2_len++;
      }
      chunk = create_chunk (log2_len);
      add_to_empty_list (chunk);
    }

    uint32_t slot = find_first_empty_slot (chunk);
    set_slot_filled (chunk, slot, true);
    uint32_t fc = get_filled_count (chunk) + 1;
    set_filled_count (chunk, fc);
    runtime->len++;

    if (fc == 1) {
      add_to_filled_list (chunk);
    }
    if (fc == get_capacity (chunk)) {
      remove_from_empty_list (chunk);
    }

    return minilib::pair < chunk_ptr, uint32_t > (chunk, slot);
  }

  constexpr void remove_slot (chunk_ptr chunk, uint32_t slot) {
    auto runtime = get_runtime ();
    set_slot_filled (chunk, slot, false);
    uint32_t fc = get_filled_count (chunk) - 1;
    set_filled_count (chunk, fc);
    runtime->len--;

    uint32_t cap = get_capacity (chunk);
    if (fc == cap - 1) {
      add_to_empty_list (chunk);
    }

    if (fc == 0) {
      remove_from_filled_list (chunk);
      remove_from_empty_list (chunk);
      destroy_chunk (chunk);
    }
  }

public:
  constexpr hive () {
    if consteval {
      constexpr_ = minilib::allocator < minilib::detail::hive_constexpr < T > > :: allocate (1);
      minilib::construct_at < minilib::detail::hive_constexpr < T > > (constexpr_);
      constexpr_->runtime.len = 0;
      constexpr_->runtime.container_id = 0;
      constexpr_->runtime.empty_head = nullptr;
      constexpr_->runtime.filled_head = nullptr;
      constexpr_->runtime.filled_tail = nullptr;
      constexpr_->runtime.next_log2_len = 3;
      constexpr_->global_counter = nullptr;
    } else {
      minilib::construct_at < minilib::detail::hive_runtime < T > > (minilib::addressof (runtime_));
      runtime_.len = 0;
      runtime_.container_id = get_tls_counter ();
      runtime_.empty_head = nullptr;
      runtime_.filled_head = nullptr;
      runtime_.filled_tail = nullptr;
      runtime_.next_log2_len = 3;
    }
  }

  constexpr void attach_counter (minilib::counter * ctr) {
    if consteval {
      if (constexpr_->global_counter != nullptr) std::terminate ();
      if (ctr != nullptr) {
        constexpr_->runtime.container_id = ctr->get ();
      }
      constexpr_->global_counter = ctr;
    } else {
      minilib::do_not_call_this ();
    }
  }

  constexpr ~hive () {
    clear ();
    if consteval {
      minilib::destroy_at < minilib::detail::hive_constexpr < T > > (constexpr_);
      minilib::allocator < minilib::detail::hive_constexpr < T > > :: deallocate (constexpr_, 1);
    }
  }

  constexpr hive (const hive& other) requires (minilib::is_copy_constructible_v < T >) {
    if consteval {
      constexpr_ = minilib::allocator < minilib::detail::hive_constexpr < T > > :: allocate (1);
      minilib::construct_at < minilib::detail::hive_constexpr < T > > (constexpr_);
      constexpr_->runtime.len = 0;
      constexpr_->runtime.empty_head = nullptr;
      constexpr_->runtime.filled_head = nullptr;
      constexpr_->runtime.filled_tail = nullptr;
      constexpr_->runtime.next_log2_len = 3;
      constexpr_->global_counter = other.constexpr_->global_counter;
      if (constexpr_->global_counter != nullptr) {
        constexpr_->runtime.container_id = constexpr_->global_counter->get ();
      } else {
        constexpr_->runtime.container_id = 0;
      }
    } else {
      minilib::construct_at < minilib::detail::hive_runtime < T > > (minilib::addressof (runtime_));
      runtime_.len = 0;
      runtime_.container_id = get_tls_counter ();
      runtime_.empty_head = nullptr;
      runtime_.filled_head = nullptr;
      runtime_.filled_tail = nullptr;
      runtime_.next_log2_len = 3;
    }

    for (auto h = other.front (); static_cast < bool > (h); h = other.next_unsafe (h)) {
      push_back (*(other.data_unsafe (h)));
    }
  }

  constexpr hive (hive&& other) {
    if consteval {
      constexpr_ = minilib::allocator < minilib::detail::hive_constexpr < T > > :: allocate (1);
      minilib::construct_at < minilib::detail::hive_constexpr < T > > (constexpr_);
      constexpr_->runtime = other.constexpr_->runtime;
      constexpr_->global_counter = other.constexpr_->global_counter;
      constexpr_->local_counter = other.constexpr_->local_counter;
      other.constexpr_->runtime.empty_head = nullptr;
      other.constexpr_->runtime.filled_head = nullptr;
      other.constexpr_->runtime.filled_tail = nullptr;
      other.constexpr_->runtime.len = 0;
      other.constexpr_->runtime.next_log2_len = 3;
      other.constexpr_->local_counter = minilib::counter ();
      if (constexpr_->global_counter != nullptr) {
        other.constexpr_->runtime.container_id = constexpr_->global_counter->get ();
      } else {
        other.constexpr_->runtime.container_id = 0;
      }
    } else {
      minilib::construct_at < minilib::detail::hive_runtime < T > > (minilib::addressof (runtime_));
      runtime_ = other.runtime_;
      other.runtime_.empty_head = nullptr;
      other.runtime_.filled_head = nullptr;
      other.runtime_.filled_tail = nullptr;
      other.runtime_.len = 0;
      other.runtime_.next_log2_len = 3;
      other.runtime_.container_id = get_tls_counter ();
    }
  }

  constexpr hive& operator= (const hive& other) requires (minilib::is_copy_constructible_v < T >) {
    if (this == minilib::addressof (other)) return *this;
    clear ();
    for (auto h = other.front (); static_cast < bool > (h); h = other.next_unsafe (h)) {
      push_back (*(other.data_unsafe (h)));
    }
    return *this;
  }

  constexpr hive& operator= (hive&& other) {
    if (this == minilib::addressof (other)) return *this;
    clear ();
    if consteval {
      constexpr_->runtime = other.constexpr_->runtime;
      constexpr_->global_counter = other.constexpr_->global_counter;
      constexpr_->local_counter = other.constexpr_->local_counter;
      other.constexpr_->runtime.empty_head = nullptr;
      other.constexpr_->runtime.filled_head = nullptr;
      other.constexpr_->runtime.filled_tail = nullptr;
      other.constexpr_->runtime.len = 0;
      other.constexpr_->runtime.next_log2_len = 3;
      other.constexpr_->local_counter = minilib::counter ();
      if (constexpr_->global_counter != nullptr) {
        other.constexpr_->runtime.container_id = constexpr_->global_counter->get ();
      } else {
        other.constexpr_->runtime.container_id = 0;
      }
    } else {
      runtime_ = other.runtime_;
      other.runtime_.empty_head = nullptr;
      other.runtime_.filled_head = nullptr;
      other.runtime_.filled_tail = nullptr;
      other.runtime_.len = 0;
      other.runtime_.next_log2_len = 3;
      other.runtime_.container_id = get_tls_counter ();
    }
    return *this;
  }

  constexpr void set_next_log2_len (uint32_t next_log2_len) {
    auto runtime = get_runtime ();
    runtime->next_log2_len = next_log2_len;
  }

  constexpr handle_type emplace_null () {
    auto pair = allocate_slot ();
    auto chunk = pair.first;
    auto slot = pair.second;
    handle_type h;
    chunk.create_counted_ref (h.ref_);
    h.offset_ = slot;
    return h;
  }

  constexpr handle_type emplace_default () requires (minilib::is_truly_default_constructible_v < T >) {
    auto pair = allocate_slot ();
    auto chunk = pair.first;
    auto slot = pair.second;
    handle_type h;
    chunk.create_counted_ref (h.ref_);
    h.offset_ = slot;
    if consteval {
      chunk.data_unsafe ()->storage.default_construct_at (slot);
    } else {
      T * tp = get_slot_ptr (chunk, slot);
      minilib::default_construct_at < T > (tp);
    }
    return h;
  }

  template < typename... Args >
  requires (minilib::is_constructible_v < T, Args&&... >)
  constexpr handle_type emplace (Args&&... args) {
    auto pair = allocate_slot ();
    auto chunk = pair.first;
    auto slot = pair.second;
    handle_type h;
    chunk.create_counted_ref (h.ref_);
    h.offset_ = slot;
    if consteval {
      chunk.data_unsafe ()->storage.construct_at (slot, minilib::forward < Args > (args)...);
    } else {
      T * tp = get_slot_ptr (chunk, slot);
      minilib::construct_at < T > (tp, minilib::forward < Args > (args)...);
    }
    return h;
  }

  constexpr handle_type insert (const T& val) requires (minilib::is_copy_constructible_v < T >) {
    return emplace (val);
  }

  constexpr handle_type insert (T&& val) requires (minilib::is_move_constructible_v < T >) {
    return emplace (minilib::move (val));
  }

  constexpr handle_type push_back (const T& val) requires (minilib::is_copy_constructible_v < T >) {
    return emplace (val);
  }

  constexpr handle_type push_back (T&& val) requires (minilib::is_move_constructible_v < T >) {
    return emplace (minilib::move (val));
  }

  constexpr void remove (const handle_type& h) requires (minilib::is_destructible_v < T >) {
    if (! check (h)) std::terminate ();
    auto p = chunk_ptr::from_counted_ref (h.ref_);
    if consteval {
      p.data_unsafe ()->storage.destroy_at (h.offset_);
    } else {
      T * tp = get_slot_ptr (p, h.offset_);
      minilib::destroy_at < T > (tp);
    }
    remove_slot (p, h.offset_);
  }

  constexpr void erase (const handle_type& h) requires (minilib::is_destructible_v < T >) {
    remove (h);
  }

  constexpr void inform_destruct (const handle_type& h) {
    if (! check (h)) std::terminate ();
    auto p = chunk_ptr::from_counted_ref (h.ref_);
    remove_slot (p, h.offset_);
  }

  constexpr void clear () {
    auto runtime = get_runtime ();
    if constexpr (minilib::is_destructible_v < T >) {
      while (runtime->filled_head) {
        auto chunk = runtime->filled_head;
        uint32_t cap = get_capacity (chunk);
        uint32_t num_words = (cap + 63) >> 6;
        for (uint32_t w = 0; w < num_words; ++w) {
          uint64_t val = get_bitmap_word (chunk, w);
          if (w == num_words - 1 && (cap & 63) != 0) {
            val &= (1ull << (cap & 63)) - 1;
          }
          while (val != 0ull) {
            uint32_t bit = minilib::detail::count_trailing_zeros_u64 (val);
            uint32_t slot = (w << 6) | bit;
            if consteval {
              chunk.data_unsafe ()->storage.destroy_at (slot);
            } else {
              T * tp = get_slot_ptr (chunk, slot);
              minilib::destroy_at < T > (tp);
            }
            val &= val - 1;
          }
        }
        remove_from_filled_list (chunk);
        if (get_filled_count (chunk) < cap) {
          remove_from_empty_list (chunk);
        }
        destroy_chunk (chunk);
      }
      while (runtime->empty_head) {
        auto chunk = runtime->empty_head;
        remove_from_empty_list (chunk);
        destroy_chunk (chunk);
      }
      runtime->len = 0;
      runtime->next_log2_len = 3;
    } else {
      if (runtime->len != 0) {
        std::terminate ();
      }
    }
  }

  constexpr void pop_front () requires (minilib::is_destructible_v < T >) {
    auto h = front ();
    if (! h) std::terminate ();
    remove (h);
  }

  constexpr void pop_back () requires (minilib::is_destructible_v < T >) {
    auto h = back ();
    if (! h) std::terminate ();
    remove (h);
  }

  constexpr bool check (const handle_type& h) const {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) return false;
    auto p = chunk_ptr::from_counted_ref (h.ref_);
    if (h.offset_ >= get_capacity (p)) return false;
    return is_slot_filled (p, h.offset_);
  }

  constexpr T * data (const handle_type& h) {
    if (! check (h)) std::terminate ();
    auto p = chunk_ptr::from_counted_ref (h.ref_);
    return get_slot_ptr (p, h.offset_);
  }

  constexpr const T * data (const handle_type& h) const {
    if (! check (h)) std::terminate ();
    auto p = chunk_ptr::from_counted_ref (h.ref_);
    return get_slot_ptr (p, h.offset_);
  }

  constexpr T * data_unsafe (const handle_type& h) {
    auto p = chunk_ptr::from_counted_ref (h.ref_);
    return get_slot_ptr (p, h.offset_);
  }

  constexpr const T * data_unsafe (const handle_type& h) const {
    auto p = chunk_ptr::from_counted_ref (h.ref_);
    return get_slot_ptr (p, h.offset_);
  }

  constexpr T& get (const handle_type& h) {
    return *data (h);
  }

  constexpr const T& get (const handle_type& h) const {
    return *data (h);
  }

  constexpr T& operator[] (const handle_type& h) {
    return *data (h);
  }

  constexpr const T& operator[] (const handle_type& h) const {
    return *data (h);
  }

  constexpr handle_type front () const {
    auto runtime = get_runtime ();
    auto curr_chunk = runtime->filled_head;
    while (curr_chunk) {
      uint32_t slot = find_first_filled_slot (curr_chunk);
      if (slot < get_capacity (curr_chunk)) {
        handle_type h;
        curr_chunk.create_counted_ref (h.ref_);
        h.offset_ = slot;
        return h;
      }
      curr_chunk = get_next_filled (curr_chunk);
    }
    return handle_type ();
  }

  constexpr handle_type back () const {
    auto runtime = get_runtime ();
    auto curr_chunk = runtime->filled_tail;
    while (curr_chunk) {
      uint32_t slot = find_last_filled_slot (curr_chunk);
      if (slot < get_capacity (curr_chunk)) {
        handle_type h;
        curr_chunk.create_counted_ref (h.ref_);
        h.offset_ = slot;
        return h;
      }
      curr_chunk = get_prev_filled (curr_chunk);
    }
    return handle_type ();
  }

  constexpr handle_type next (const handle_type& h) const {
    if (! check (h)) std::terminate ();
    return next_unsafe (h);
  }

  constexpr handle_type next_unsafe (const handle_type& h) const {
    auto p = chunk_ptr::from_counted_ref (h.ref_);
    uint32_t slot = find_next_filled_slot (p, h.offset_);
    if (slot < get_capacity (p)) {
      handle_type nh;
      p.create_counted_ref (nh.ref_);
      nh.offset_ = slot;
      return nh;
    }
    auto curr_chunk = get_next_filled (p);
    while (curr_chunk) {
      uint32_t first_slot = find_first_filled_slot (curr_chunk);
      if (first_slot < get_capacity (curr_chunk)) {
        handle_type nh;
        curr_chunk.create_counted_ref (nh.ref_);
        nh.offset_ = first_slot;
        return nh;
      }
      curr_chunk = get_next_filled (curr_chunk);
    }
    return handle_type ();
  }

  constexpr handle_type prev (const handle_type& h) const {
    if (! check (h)) std::terminate ();
    return prev_unsafe (h);
  }

  constexpr handle_type prev_unsafe (const handle_type& h) const {
    auto p = chunk_ptr::from_counted_ref (h.ref_);
    uint32_t slot = find_prev_filled_slot (p, h.offset_);
    if (slot < get_capacity (p)) {
      handle_type ph;
      p.create_counted_ref (ph.ref_);
      ph.offset_ = slot;
      return ph;
    }
    auto curr_chunk = get_prev_filled (p);
    while (curr_chunk) {
      uint32_t last_slot = find_last_filled_slot (curr_chunk);
      if (last_slot < get_capacity (curr_chunk)) {
        handle_type ph;
        curr_chunk.create_counted_ref (ph.ref_);
        ph.offset_ = last_slot;
        return ph;
      }
      curr_chunk = get_prev_filled (curr_chunk);
    }
    return handle_type ();
  }

  constexpr size_t size () const {
    return get_runtime ()->len;
  }

  constexpr bool empty () const {
    return get_runtime ()->len == 0;
  }

  constexpr size_t capacity () const {
    auto runtime = get_runtime ();
    size_t total = 0;
    auto p = runtime->filled_head;
    while (p) {
      total += get_capacity (p);
      p = get_next_filled (p);
    }
    p = runtime->empty_head;
    while (p) {
      if (get_filled_count (p) == 0) {
        total += get_capacity (p);
      }
      p = get_next_empty (p);
    }
    return total;
  }
};

}

#endif
