#ifndef HASHMAP_HPP
#define HASHMAP_HPP

#include <stdint.h>
#include <random.h>
#include <exception>
#include <type_traits.hpp>
#include <utility.hpp>
#include <compare.hpp>
#include <hash.hpp>
#include <pair.hpp>
#include <vector.hpp>
#include <raw_dyn_array.hpp>
#include <hive.hpp>
#include <counter.hpp>

namespace minilib {

template < typename Key, typename Value >
requires (std::is_object_v < Key > && ! std::is_array_v < Key > && ! std::is_const_v < Key > && ! std::is_volatile_v < Key >
          && minilib::is_destructible_v < Key > && minilib::is_copy_constructible_v < Key >
          && minilib::hashable < Key > && minilib::equality_comparable < Key >
          && std::is_object_v < Value > && ! std::is_array_v < Value > && ! std::is_const_v < Value > && ! std::is_volatile_v < Value >)
class hashmap {
public:
  using key_type = Key;
  using mapped_type = Value;
  using size_type = size_t;
  using handle_type = typename minilib::hive < Value > :: handle_type;
  using slot_type = minilib::pair < Key, handle_type >;

  static constexpr size_t GROUP_SIZE = 8;
  static constexpr size_t NOT_FOUND = static_cast < size_t > (-1);
  static constexpr uint8_t CTRL_EMPTY = 0x00;
  static constexpr uint8_t CTRL_TOMBSTONE = 0x7F;
  static constexpr size_t REHASH_STEP_COUNT = 4;

private:
  struct table_t {
    unsigned char random_key[32];
    minilib::vector < unsigned char > ctrl;
    minilib::raw_dyn_array < slot_type > slots;
    size_t capacity;
    size_t size;
    size_t tombstones;

    constexpr table_t ()
      : random_key {0}, ctrl (), slots (), capacity (0), size (0), tombstones (0) { }

    table_t (const table_t&) = delete;
    table_t& operator= (const table_t&) = delete;

    constexpr table_t (table_t&& other)
      : ctrl (minilib::move (other.ctrl)),
        slots (minilib::move (other.slots)),
        capacity (other.capacity),
        size (other.size),
        tombstones (other.tombstones) {
      if consteval {
	for (size_t i = 0; i < 32; ++i) random_key[i] = other.random_key[i];
      } else {
	__builtin_memcpy (random_key, other.random_key, 32);
      }
      other.capacity = 0;
      other.size = 0;
      other.tombstones = 0;
    }

    constexpr table_t& operator= (table_t&& other) {
      if (this == minilib::addressof (other)) return *this;
      ctrl = minilib::move (other.ctrl);
      slots = minilib::move (other.slots);
      capacity = other.capacity;
      size = other.size;
      tombstones = other.tombstones;
      for (size_t i = 0; i < 32; ++i) random_key[i] = other.random_key[i];
      other.capacity = 0;
      other.size = 0;
      other.tombstones = 0;
      return *this;
    }

    constexpr ~table_t () {
      slots.deallocate ();
    }
  };

  table_t tables_[2];
  minilib::hive < Value > hive_;
  bool rehashing_;
  size_t rehash_idx_;

  static constexpr void refresh_random_key (unsigned char * key) {
    if consteval {
      for (size_t i = 0; i < 32; ++i) key[i] = 0;
    } else {
      if (getrandom (key, 32, 0) != 32) {
        std::terminate ();
      }
    }
  }

  static constexpr void init_table (table_t& tbl, size_t cap) {
#ifdef CONFIG_DEFENSIVE
    if (__builtin_mul_overflow_p (sizeof (slot_type), cap, 0ull)) std::terminate ();
#endif
    tbl.capacity = cap;
    tbl.size = 0;
    tbl.tombstones = 0;
    tbl.ctrl.clear ();
    tbl.ctrl.push_back_many (CTRL_EMPTY, cap);
    tbl.slots.allocate (cap);
    refresh_random_key (tbl.random_key);
  }

  static constexpr void clear_table (table_t& tbl) {
    if (tbl.capacity == 0) return;
    for (size_t i = 0; i < tbl.capacity; ++i) {
      if ((tbl.ctrl[i] & 0x80) != 0) {
        tbl.slots.destroy_at (i);
        tbl.ctrl[i] = CTRL_EMPTY;
      }
    }
    tbl.size = 0;
    tbl.tombstones = 0;
  }

  template < typename K >
  static constexpr size_t find_slot (const table_t& tbl, const K& key, uint64_t hash_val) {
    if (tbl.capacity == 0) return NOT_FOUND;
    size_t num_groups = tbl.capacity / GROUP_SIZE;
    uint8_t target = 0x80 | static_cast < uint8_t > (hash_val & 0x7FULL);
    size_t g = (hash_val >> 7) & (num_groups - 1);
    size_t probe_step = 1;

    for (size_t probe = 0; probe < num_groups; ++probe) {
      size_t base = g * GROUP_SIZE;
      bool has_empty = false;

      if consteval {
        for (size_t i = 0; i < GROUP_SIZE; ++i) {
          uint8_t c = tbl.ctrl[base + i];
          if (c == target) {
            if (tbl.slots[base + i].first == key) {
              return base + i;
            }
          } else if (c == CTRL_EMPTY) {
            has_empty = true;
          }
        }
      } else {
        uint64_t ctrl8;
        __builtin_memcpy (&ctrl8, tbl.ctrl.data () + base, 8);

        uint64_t match_mask = 0x0101010101010101ULL * target;
        uint64_t x = ctrl8 ^ match_mask;
        uint64_t matches = (x - 0x0101010101010101ULL) & ~x & 0x8080808080808080ULL;

        while (matches != 0) {
          int bit = __builtin_ctzll (matches);
          int offset = bit >> 3;
          if (tbl.slots[base + offset].first == key) {
            return base + offset;
          }
          matches &= matches - 1;
        }

        uint64_t empty_mask = (ctrl8 - 0x0101010101010101ULL) & ~ctrl8 & 0x8080808080808080ULL;
        if (empty_mask != 0) {
          has_empty = true;
        }
      }

      if (has_empty) {
        return NOT_FOUND;
      }

      g = (g + probe_step) & (num_groups - 1);
      probe_step++;
    }

    return NOT_FOUND;
  }

  template < typename K >
  static constexpr size_t find_insert_slot (table_t& tbl, const K& key, uint64_t hash_val) {
    size_t num_groups = tbl.capacity / GROUP_SIZE;
    uint8_t target = 0x80 | static_cast < uint8_t > (hash_val & 0x7FULL);
    size_t g = (hash_val >> 7) & (num_groups - 1);
    size_t probe_step = 1;
    size_t first_avail_slot = NOT_FOUND;

    for (size_t probe = 0; probe < num_groups; ++probe) {
      size_t base = g * GROUP_SIZE;
      bool has_empty = false;

      if consteval {
        for (size_t i = 0; i < GROUP_SIZE; ++i) {
          uint8_t c = tbl.ctrl[base + i];
          if (c == target) {
            if (tbl.slots[base + i].first == key) {
              return base + i;
            }
          } else if (c == CTRL_TOMBSTONE) {
            if (first_avail_slot == NOT_FOUND) {
              first_avail_slot = base + i;
            }
          } else if (c == CTRL_EMPTY) {
            has_empty = true;
            if (first_avail_slot == NOT_FOUND) {
              first_avail_slot = base + i;
            }
          }
        }
      } else {
        uint64_t ctrl8;
        __builtin_memcpy (&ctrl8, tbl.ctrl.data () + base, 8);

        uint64_t match_mask = 0x0101010101010101ULL * target;
        uint64_t x = ctrl8 ^ match_mask;
        uint64_t matches = (x - 0x0101010101010101ULL) & ~x & 0x8080808080808080ULL;

        while (matches != 0) {
          int bit = __builtin_ctzll (matches);
          int offset = bit >> 3;
          if (tbl.slots[base + offset].first == key) {
            return base + offset;
          }
          matches &= matches - 1;
        }

        uint64_t avail = ~ctrl8 & 0x8080808080808080ULL;
        if (avail != 0 && first_avail_slot == NOT_FOUND) {
          int bit = __builtin_ctzll (avail);
          int offset = bit >> 3;
          first_avail_slot = base + offset;
        }

        uint64_t empty_mask = (ctrl8 - 0x0101010101010101ULL) & ~ctrl8 & 0x8080808080808080ULL;
        if (empty_mask != 0) {
          has_empty = true;
        }
      }

      if (has_empty) {
        return first_avail_slot;
      }

      g = (g + probe_step) & (num_groups - 1);
      probe_step++;
    }

    return first_avail_slot;
  }

  constexpr void start_rehash () {
    if (rehashing_) return;
    size_t new_cap = tables_[0].capacity * 2;
    if (tables_[0].size <= tables_[0].capacity / 4 && tables_[0].capacity > 8) {
      new_cap = tables_[0].capacity / 2;
    } else if (tables_[0].size * 2 <= tables_[0].capacity) {
      new_cap = tables_[0].capacity;
    }
    if (new_cap < 8) new_cap = 8;

    init_table (tables_[1], new_cap);
    rehashing_ = true;
    rehash_idx_ = 0;
  }

  constexpr void finalize_rehash () {
    if (tables_[0].size > 0) {
      for (size_t i = rehash_idx_; i < tables_[0].capacity; ++i) {
        if ((tables_[0].ctrl[i] & 0x80) != 0) {
          Key& k = tables_[0].slots[i].first;
          handle_type h = tables_[0].slots[i].second;
          uint64_t hash_val = minilib::hash::operator () (tables_[1].random_key, k);
          size_t dest_slot = find_insert_slot (tables_[1], k, hash_val);
          if (tables_[1].ctrl[dest_slot] == CTRL_TOMBSTONE) {
            tables_[1].tombstones--;
          }
          tables_[1].slots.construct_at (dest_slot, minilib::move (k), h);
          tables_[1].ctrl[dest_slot] = 0x80 | static_cast < uint8_t > (hash_val & 0x7FULL);
          tables_[1].size++;

          tables_[0].slots.destroy_at (i);
          tables_[0].ctrl[i] = CTRL_TOMBSTONE;
        }
      }
      tables_[0].size = 0;
    }

    tables_[0].slots.deallocate ();
    tables_[0].ctrl.clear ();
    tables_[0] = minilib::move (tables_[1]);

    rehashing_ = false;
    rehash_idx_ = 0;
  }

  constexpr void ensure_capacity_for_insert () {
    if (tables_[0].capacity == 0) {
      init_table (tables_[0], 8);
    } else if (! rehashing_) {
      if ((tables_[0].size + tables_[0].tombstones + 1) * 8 > tables_[0].capacity * 7) {
        start_rehash ();
        step_rehash (REHASH_STEP_COUNT);
      }
    } else {
      if ((tables_[1].size + tables_[1].tombstones + 1) * 8 > tables_[1].capacity * 7) {
        finalize_rehash ();
        start_rehash ();
        step_rehash (REHASH_STEP_COUNT);
      }
    }
  }

  constexpr bool erase_slot_matching_handle (const handle_type& h) {
    for (size_t i = 0; i < tables_[0].capacity; ++i) {
      if ((tables_[0].ctrl[i] & 0x80) != 0 && tables_[0].slots[i].second == h) {
        tables_[0].slots.destroy_at (i);
        tables_[0].ctrl[i] = CTRL_TOMBSTONE;
        tables_[0].size--;
        tables_[0].tombstones++;
        return true;
      }
    }
    if (rehashing_) {
      for (size_t i = 0; i < tables_[1].capacity; ++i) {
        if ((tables_[1].ctrl[i] & 0x80) != 0 && tables_[1].slots[i].second == h) {
          tables_[1].slots.destroy_at (i);
          tables_[1].ctrl[i] = CTRL_TOMBSTONE;
          tables_[1].size--;
          tables_[1].tombstones++;
          return true;
        }
      }
    }
    return false;
  }

  constexpr void check_shrink () {
    if (rehashing_) return;
    if (tables_[0].capacity > 8 && tables_[0].size <= tables_[0].capacity / 4) {
      start_rehash ();
      step_rehash (REHASH_STEP_COUNT);
    }
  }

public:
  constexpr hashmap ()
    : tables_ (), hive_ (), rehashing_ (false), rehash_idx_ (0) {
    refresh_random_key (tables_[0].random_key);
  }

  constexpr void attach_counter (minilib::counter * ctr) {
    hive_.attach_counter (ctr);
  }

  constexpr hashmap (const hashmap& other)
  requires (minilib::is_copy_constructible_v < Key > && minilib::is_copy_constructible_v < Value >)
    : tables_ (), hive_ (), rehashing_ (false), rehash_idx_ (0)
  {
    refresh_random_key (tables_[0].random_key);
    if (other.empty ()) return;
    size_t target_cap = 8;
    while (target_cap * 7 < other.size () * 8) {
      target_cap *= 2;
    }
    init_table (tables_[0], target_cap);
    for (size_t i = 0; i < other.tables_[0].capacity; ++i) {
      if ((other.tables_[0].ctrl[i] & 0x80) != 0) {
        insert (other.tables_[0].slots[i].first, other.hive_.get (other.tables_[0].slots[i].second));
      }
    }
    if (other.rehashing_) {
      for (size_t i = 0; i < other.tables_[1].capacity; ++i) {
        if ((other.tables_[1].ctrl[i] & 0x80) != 0) {
          insert (other.tables_[1].slots[i].first, other.hive_.get (other.tables_[1].slots[i].second));
        }
      }
    }
  }

  constexpr hashmap (hashmap&& other)
    : tables_ (), hive_ (minilib::move (other.hive_)),
      rehashing_ (other.rehashing_),
      rehash_idx_ (other.rehash_idx_)
  {
    tables_[0] = minilib::move (other.tables_[0]);
    tables_[1] = minilib::move (other.tables_[1]);
    other.rehashing_ = false;
    other.rehash_idx_ = 0;
  }

  constexpr hashmap& operator= (const hashmap& other)
  requires (minilib::is_copy_constructible_v < Key > && minilib::is_copy_constructible_v < Value >) {
    if (this == minilib::addressof (other)) return *this;
    clear ();
    for (size_t i = 0; i < other.tables_[0].capacity; ++i) {
      if ((other.tables_[0].ctrl[i] & 0x80) != 0) {
        insert (other.tables_[0].slots[i].first, other.hive_.get (other.tables_[0].slots[i].second));
      }
    }
    if (other.rehashing_) {
      for (size_t i = 0; i < other.tables_[1].capacity; ++i) {
        if ((other.tables_[1].ctrl[i] & 0x80) != 0) {
          insert (other.tables_[1].slots[i].first, other.hive_.get (other.tables_[1].slots[i].second));
        }
      }
    }
    return *this;
  }

  constexpr hashmap& operator= (hashmap&& other) {
    if (this == minilib::addressof (other)) return *this;
    clear ();
    tables_[0] = minilib::move (other.tables_[0]);
    tables_[1] = minilib::move (other.tables_[1]);
    hive_ = minilib::move (other.hive_);
    rehashing_ = other.rehashing_;
    rehash_idx_ = other.rehash_idx_;
    other.rehashing_ = false;
    other.rehash_idx_ = 0;
    return *this;
  }

  constexpr ~hashmap () {
    clear_table (tables_[0]);
    if (rehashing_) {
      clear_table (tables_[1]);
    }
  }

  constexpr size_t size () const { return hive_.size (); }
  constexpr bool empty () const { return hive_.empty (); }
  constexpr size_t capacity () const {
    return rehashing_ ? tables_[1].capacity : tables_[0].capacity;
  }

  constexpr void step_rehash (size_t n = REHASH_STEP_COUNT) {
    if (! rehashing_) return;
    size_t moved = 0;
    while (rehash_idx_ < tables_[0].capacity && moved < n) {
      if ((tables_[0].ctrl[rehash_idx_] & 0x80) != 0) {
        Key& k = tables_[0].slots[rehash_idx_].first;
        handle_type h = tables_[0].slots[rehash_idx_].second;

        uint64_t hash_val = minilib::hash::operator () (tables_[1].random_key, k);
        size_t dest_slot = find_insert_slot (tables_[1], k, hash_val);
        if (tables_[1].ctrl[dest_slot] == CTRL_TOMBSTONE) {
          tables_[1].tombstones--;
        }
        tables_[1].slots.construct_at (dest_slot, minilib::move (k), h);
        tables_[1].ctrl[dest_slot] = 0x80 | static_cast < uint8_t > (hash_val & 0x7FULL);
        tables_[1].size++;

        tables_[0].slots.destroy_at (rehash_idx_);
        tables_[0].ctrl[rehash_idx_] = CTRL_TOMBSTONE;
        tables_[0].size--;
        moved++;
      }
      rehash_idx_++;
    }

    if (rehash_idx_ >= tables_[0].capacity || tables_[0].size == 0) {
      finalize_rehash ();
    }
  }

  constexpr void finish_rehash () {
    while (rehashing_) {
      step_rehash (tables_[0].capacity);
    }
  }

  constexpr bool is_rehashing () const { return rehashing_; }

  constexpr const unsigned char * random_key () const {
    return rehashing_ ? tables_[1].random_key : tables_[0].random_key;
  }

  constexpr const minilib::vector < unsigned char > & ctrl () const {
    return rehashing_ ? tables_[1].ctrl : tables_[0].ctrl;
  }

  constexpr const minilib::hive < Value > & hive () const { return hive_; }
  constexpr minilib::hive < Value > & hive () { return hive_; }

  template < typename K >
  requires (minilib::hashable < K > && requires (const K& k, const Key& key_obj) { k == key_obj; })
  constexpr handle_type search (const K& k) const {
    if (tables_[0].capacity == 0) return handle_type ();
    if (rehashing_) {
      uint64_t h1 = minilib::hash::operator () (tables_[1].random_key, k);
      size_t s1 = find_slot (tables_[1], k, h1);
      if (s1 != NOT_FOUND) return tables_[1].slots[s1].second;
    }
    uint64_t h0 = minilib::hash::operator () (tables_[0].random_key, k);
    size_t s0 = find_slot (tables_[0], k, h0);
    if (s0 != NOT_FOUND) return tables_[0].slots[s0].second;
    return handle_type ();
  }

  template < typename K >
  requires (minilib::hashable < K > && requires (const K& k, const Key& key_obj) { k == key_obj; })
  constexpr bool contains (const K& k) const {
    return static_cast < bool > (search (k));
  }

  template < typename K >
  requires (minilib::hashable < K > && requires (const K& k, const Key& key_obj) { k == key_obj; })
  constexpr Value& at (const K& k) {
    auto h = search (k);
    if (! static_cast < bool > (h)) std::terminate ();
    return hive_.get (h);
  }

  template < typename K >
  requires (minilib::hashable < K > && requires (const K& k, const Key& key_obj) { k == key_obj; })
  constexpr const Value& at (const K& k) const {
    auto h = search (k);
    if (! static_cast < bool > (h)) std::terminate ();
    return hive_.get (h);
  }

  constexpr const Key& key (const handle_type& h) const {
    for (size_t i = 0; i < tables_[0].capacity; ++i) {
      if ((tables_[0].ctrl[i] & 0x80) != 0 && tables_[0].slots[i].second == h) {
        return tables_[0].slots[i].first;
      }
    }
    if (rehashing_) {
      for (size_t i = 0; i < tables_[1].capacity; ++i) {
        if ((tables_[1].ctrl[i] & 0x80) != 0 && tables_[1].slots[i].second == h) {
          return tables_[1].slots[i].first;
        }
      }
    }
    std::terminate ();
  }

  constexpr Value& value (const handle_type& h) {
    return hive_.get (h);
  }

  constexpr const Value& value (const handle_type& h) const {
    return hive_.get (h);
  }

  constexpr Value * value_ptr (const handle_type& h) {
    return hive_.data (h);
  }

  constexpr const Value * value_ptr (const handle_type& h) const {
    return hive_.data (h);
  }

  constexpr Value * data (const handle_type& h) {
    return hive_.data (h);
  }

  constexpr const Value * data (const handle_type& h) const {
    return hive_.data (h);
  }

  constexpr Value& get (const handle_type& h) {
    return hive_.get (h);
  }

  constexpr const Value& get (const handle_type& h) const {
    return hive_.get (h);
  }

  constexpr handle_type front () const {
    return hive_.front ();
  }

  constexpr handle_type next (const handle_type& h) const {
    return hive_.next (h);
  }

  constexpr void clear () {
    clear_table (tables_[0]);
    if (rehashing_) {
      clear_table (tables_[1]);
      tables_[1].slots.deallocate ();
      tables_[1].ctrl.clear ();
      tables_[1].capacity = 0;
      rehashing_ = false;
      rehash_idx_ = 0;
    }
    hive_.clear ();
  }

  template < typename K, typename... Args >
  requires (minilib::is_constructible_v < Key, K&& > && minilib::is_constructible_v < Value, Args&&... >)
  constexpr minilib::pair < bool, handle_type > emplace (K&& k, Args&&... args) {
    step_rehash (REHASH_STEP_COUNT);
    auto existing = search (k);
    if (static_cast < bool > (existing)) {
      return minilib::pair < bool, handle_type > (false, existing);
    }
    ensure_capacity_for_insert ();
    table_t& target = rehashing_ ? tables_[1] : tables_[0];
    handle_type h = hive_.emplace (minilib::forward < Args > (args)...);
    uint64_t hash_val = minilib::hash::operator () (target.random_key, k);
    size_t slot = find_insert_slot (target, k, hash_val);
    if (target.ctrl[slot] == CTRL_TOMBSTONE) {
      target.tombstones--;
    }
    target.slots.construct_at (slot, minilib::forward < K > (k), h);
    target.ctrl[slot] = 0x80 | static_cast < uint8_t > (hash_val & 0x7FULL);
    target.size++;
    return minilib::pair < bool, handle_type > (true, h);
  }

  template < typename K, typename V >
  requires (minilib::is_constructible_v < Key, K&& > && minilib::is_constructible_v < Value, V&& >)
  constexpr minilib::pair < bool, handle_type > insert (K&& k, V&& v) {
    return emplace (minilib::forward < K > (k), minilib::forward < V > (v));
  }

  constexpr minilib::pair < bool, handle_type > insert (const minilib::pair < Key, Value > & p)
  requires (minilib::is_copy_constructible_v < Key > && minilib::is_copy_constructible_v < Value >) {
    return emplace (p.first, p.second);
  }

  constexpr minilib::pair < bool, handle_type > insert (minilib::pair < Key, Value > && p)
  requires (minilib::is_move_constructible_v < Key > && minilib::is_move_constructible_v < Value >) {
    return emplace (minilib::move (p.first), minilib::move (p.second));
  }

  template < typename K >
  requires (minilib::is_constructible_v < Key, K&& > && minilib::is_truly_default_constructible_v < Value >)
  constexpr minilib::pair < bool, handle_type > insert_default (K&& k) {
    step_rehash (REHASH_STEP_COUNT);
    auto existing = search (k);
    if (static_cast < bool > (existing)) {
      return minilib::pair < bool, handle_type > (false, existing);
    }
    ensure_capacity_for_insert ();
    table_t& target = rehashing_ ? tables_[1] : tables_[0];
    handle_type h = hive_.emplace_default ();
    uint64_t hash_val = minilib::hash::operator () (target.random_key, k);
    size_t slot = find_insert_slot (target, k, hash_val);
    if (target.ctrl[slot] == CTRL_TOMBSTONE) {
      target.tombstones--;
    }
    target.slots.construct_at (slot, minilib::forward < K > (k), h);
    target.ctrl[slot] = 0x80 | static_cast < uint8_t > (hash_val & 0x7FULL);
    target.size++;
    return minilib::pair < bool, handle_type > (true, h);
  }

  template < typename K, typename M >
  requires (minilib::is_constructible_v < Key, K&& > && minilib::is_constructible_v < Value, M&& > && requires (Value& v, M&& m) { v = minilib::forward < M > (m); })
  constexpr minilib::pair < bool, handle_type > insert_or_assign (K&& k, M&& m) {
    step_rehash (REHASH_STEP_COUNT);
    auto existing = search (k);
    if (static_cast < bool > (existing)) {
      hive_.get (existing) = minilib::forward < M > (m);
      return minilib::pair < bool, handle_type > (false, existing);
    }
    return emplace (minilib::forward < K > (k), minilib::forward < M > (m));
  }

  template < typename K >
  requires (minilib::is_constructible_v < Key, K&& > && minilib::is_truly_default_constructible_v < Value >)
  constexpr Value& operator[] (K&& k) {
    auto res = insert_default (minilib::forward < K > (k));
    return hive_.get (res.second);
  }

  template < typename K >
  requires (minilib::is_destructible_v < Value > && minilib::hashable < K > && requires (const K& k, const Key& key_obj) { k == key_obj; })
  constexpr bool remove (const K& k) {
    if (tables_[0].capacity == 0) return false;
    step_rehash (REHASH_STEP_COUNT);
    if (rehashing_) {
      uint64_t h1 = minilib::hash::operator () (tables_[1].random_key, k);
      size_t s1 = find_slot (tables_[1], k, h1);
      if (s1 != NOT_FOUND) {
        handle_type h = tables_[1].slots[s1].second;
        tables_[1].slots.destroy_at (s1);
        tables_[1].ctrl[s1] = CTRL_TOMBSTONE;
        tables_[1].size--;
        tables_[1].tombstones++;
        hive_.remove (h);
        check_shrink ();
        return true;
      }
    }
    uint64_t h0 = minilib::hash::operator () (tables_[0].random_key, k);
    size_t s0 = find_slot (tables_[0], k, h0);
    if (s0 != NOT_FOUND) {
      handle_type h = tables_[0].slots[s0].second;
      tables_[0].slots.destroy_at (s0);
      tables_[0].ctrl[s0] = CTRL_TOMBSTONE;
      tables_[0].size--;
      tables_[0].tombstones++;
      hive_.remove (h);
      check_shrink ();
      return true;
    }
    return false;
  }

  template < typename K >
  requires (minilib::is_destructible_v < Value > && minilib::hashable < K > && requires (const K& k, const Key& key_obj) { k == key_obj; })
  constexpr bool erase (const K& k) {
    return remove (k);
  }

  constexpr void remove (const handle_type& h) requires (minilib::is_destructible_v < Value >) {
    if (! static_cast < bool > (h)) return;
    step_rehash (REHASH_STEP_COUNT);
    if (erase_slot_matching_handle (h)) {
      hive_.remove (h);
      check_shrink ();
    } else {
      std::terminate ();
    }
  }

  constexpr void erase (const handle_type& h) requires (minilib::is_destructible_v < Value >) {
    remove (h);
  }

  constexpr void inform_destruct (const handle_type& h) {
    if (! static_cast < bool > (h)) return;
    step_rehash (REHASH_STEP_COUNT);
    if (erase_slot_matching_handle (h)) {
      hive_.inform_destruct (h);
      check_shrink ();
    } else {
      std::terminate ();
    }
  }
};

}

#endif
