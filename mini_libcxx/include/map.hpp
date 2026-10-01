#ifndef MAP_HPP
#define MAP_HPP

#include <stdint.h>
#include <exception>
#include <type_traits.hpp>
#include <raw_array.hpp>
#include <utility.hpp>
#include <compare.hpp>
#include <rbtree.hpp>
#include <pair.hpp>
#include <counter.hpp>

namespace minilib {

namespace detail {

struct map_in_place_val_t { explicit map_in_place_val_t () = default; };
inline constexpr map_in_place_val_t map_in_place_val {};

struct map_in_place_default_val_t { explicit map_in_place_default_val_t () = default; };
inline constexpr map_in_place_default_val_t map_in_place_default_val {};

template < typename Key, typename Value >
requires (std::is_object_v < Key > && ! std::is_array_v < Key > && ! std::is_const_v < Key > && ! std::is_volatile_v < Key >
          && std::is_object_v < Value > && ! std::is_array_v < Value > && ! std::is_const_v < Value > && ! std::is_volatile_v < Value >)
struct map_pair {
  minilib::pair < minilib::raw_array < Key, 1 >, minilib::raw_array < Value, 1 > > storage;

  constexpr map_pair () requires (minilib::is_truly_default_constructible_v < Key > && minilib::is_truly_default_constructible_v < Value >)
    : storage () {
    storage.first.default_construct_at (0);
    storage.second.default_construct_at (0);
  }

  template < typename K, typename... Args >
  requires (minilib::is_constructible_v < Key, K&& > && minilib::is_constructible_v < Value, Args&&... >)
  constexpr map_pair (map_in_place_val_t, K&& k, Args&&... args)
    : storage () {
    storage.first.construct_at (0, minilib::forward < K > (k));
    storage.second.construct_at (0, minilib::forward < Args > (args)...);
  }

  template < typename K >
  requires (minilib::is_constructible_v < Key, K&& > && minilib::is_truly_default_constructible_v < Value >)
  constexpr map_pair (map_in_place_default_val_t, K&& k)
    : storage () {
    storage.first.construct_at (0, minilib::forward < K > (k));
    storage.second.default_construct_at (0);
  }

  template < typename K, typename V >
  requires (minilib::is_constructible_v < Key, K&& > && minilib::is_constructible_v < Value, V&& >)
  constexpr map_pair (K&& k, V&& v)
    : storage () {
    storage.first.construct_at (0, minilib::forward < K > (k));
    storage.second.construct_at (0, minilib::forward < V > (v));
  }

  constexpr map_pair (const minilib::pair < Key, Value >& p) requires (minilib::is_copy_constructible_v < Key > && minilib::is_copy_constructible_v < Value >)
    : storage () {
    storage.first.construct_at (0, p.first);
    storage.second.construct_at (0, p.second);
  }

  constexpr map_pair (minilib::pair < Key, Value >&& p) requires (minilib::is_move_constructible_v < Key > && minilib::is_move_constructible_v < Value >)
    : storage () {
    storage.first.construct_at (0, minilib::move (p.first));
    storage.second.construct_at (0, minilib::move (p.second));
  }

  constexpr map_pair (const map_pair& other) requires (minilib::is_copy_constructible_v < Key > && minilib::is_copy_constructible_v < Value >)
    : storage () {
    storage.first.construct_at (0, *other.storage.first.data ());
    storage.second.construct_at (0, *other.storage.second.data ());
  }

  constexpr map_pair (map_pair&& other) requires (minilib::is_move_constructible_v < Key > && minilib::is_move_constructible_v < Value >)
    : storage () {
    storage.first.construct_at (0, minilib::move (*other.storage.first.data ()));
    storage.second.construct_at (0, minilib::move (*other.storage.second.data ()));
  }

  constexpr map_pair& operator= (const map_pair& other) requires (minilib::is_copy_constructible_v < Key > && minilib::is_copy_constructible_v < Value >) {
    if (this == minilib::addressof (other)) return *this;
    if constexpr (minilib::is_copy_assignable_v < Key >) {
      *storage.first.data () = *other.storage.first.data ();
    } else {
      storage.first.destroy_at (0);
      storage.first.construct_at (0, *other.storage.first.data ());
    }
    if constexpr (minilib::is_copy_assignable_v < Value >) {
      *storage.second.data () = *other.storage.second.data ();
    } else {
      storage.second.destroy_at (0);
      storage.second.construct_at (0, *other.storage.second.data ());
    }
    return *this;
  }

  constexpr map_pair& operator= (map_pair&& other) requires (minilib::is_move_constructible_v < Key > && minilib::is_move_constructible_v < Value >) {
    if (this == minilib::addressof (other)) return *this;
    if constexpr (minilib::is_move_assignable_v < Key >) {
      *storage.first.data () = minilib::move (*other.storage.first.data ());
    } else {
      storage.first.destroy_at (0);
      storage.first.construct_at (0, minilib::move (*other.storage.first.data ()));
    }
    if constexpr (minilib::is_move_assignable_v < Value >) {
      *storage.second.data () = minilib::move (*other.storage.second.data ());
    } else {
      storage.second.destroy_at (0);
      storage.second.construct_at (0, minilib::move (*other.storage.second.data ()));
    }
    return *this;
  }

  constexpr ~map_pair () {
    if constexpr (minilib::is_destructible_v < Value >) {
      storage.second.destroy_at (0);
    }
    if constexpr (minilib::is_destructible_v < Key >) {
      storage.first.destroy_at (0);
    }
  }

  constexpr Key& key () { return *storage.first.data (); }
  constexpr const Key& key () const { return *storage.first.data (); }
  constexpr Value& value () { return *storage.second.data (); }
  constexpr const Value& value () const { return *storage.second.data (); }
};

}

template < typename Key, typename Value >
requires (std::is_object_v < Key > && ! std::is_array_v < Key > && ! std::is_const_v < Key > && ! std::is_volatile_v < Key >
          && minilib::three_way_comparable < Key >
          && std::is_object_v < Value > && ! std::is_array_v < Value > && ! std::is_const_v < Value > && ! std::is_volatile_v < Value >)
class map {
public:
  using key_type = Key;
  using mapped_type = Value;
  using size_type = size_t;
  using node_pair_type = minilib::detail::map_pair < Key, Value >;
  using tree_type = minilib::rbtree < node_pair_type >;
  using handle_type = typename tree_type::handle_type;

private:
  tree_type tree_;
  size_t size_;

  static constexpr bool same_handle (const handle_type& a, const handle_type& b) {
    return minilib::compare_three_way::operator () (a, b) == 0;
  }

public:
  constexpr map () : tree_ (), size_ (0) {}

  constexpr void attach_counter (minilib::counter * ctr) {
    tree_.attach_counter (ctr);
  }

  constexpr map (const map& other) requires (minilib::is_copy_constructible_v < Key > && minilib::is_copy_constructible_v < Value >)
    : tree_ (other.tree_), size_ (other.size_) {}

  constexpr map (map&& other)
    : tree_ (minilib::move (other.tree_)), size_ (other.size_) {
    other.size_ = 0;
  }

  constexpr map& operator= (const map& other) requires (minilib::is_copy_constructible_v < Key > && minilib::is_copy_constructible_v < Value >) {
    if (this == minilib::addressof (other)) return *this;
    tree_ = other.tree_;
    size_ = other.size_;
    return *this;
  }

  constexpr map& operator= (map&& other) {
    if (this == minilib::addressof (other)) return *this;
    tree_ = minilib::move (other.tree_);
    size_ = other.size_;
    other.size_ = 0;
    return *this;
  }

  constexpr ~map () = default;

  constexpr size_t size () const { return size_; }
  constexpr bool empty () const { return size_ == 0; }

  constexpr void clear () {
    tree_.clear ();
    size_ = 0;
  }

  /* Node accessors */
  constexpr const Key& key (const handle_type& h) const {
    return tree_.data (h)->key ();
  }

  constexpr const Key * key_ptr (const handle_type& h) const {
    return minilib::addressof (tree_.data (h)->key ());
  }

  constexpr Value& value (const handle_type& h) {
    return tree_.data (h)->value ();
  }

  constexpr const Value& value (const handle_type& h) const {
    return tree_.data (h)->value ();
  }

  constexpr Value * value_ptr (const handle_type& h) {
    return minilib::addressof (tree_.data (h)->value ());
  }

  constexpr const Value * value_ptr (const handle_type& h) const {
    return minilib::addressof (tree_.data (h)->value ());
  }

  /* Navigation helpers */
  constexpr handle_type root () const { return tree_.root (); }
  constexpr handle_type left (const handle_type& h) const { return tree_.left (h); }
  constexpr handle_type right (const handle_type& h) const { return tree_.right (h); }
  constexpr handle_type parent (const handle_type& h) const { return tree_.parent (h); }

  constexpr handle_type min () const {
    auto curr = tree_.root ();
    if (! curr) return curr;
    while (true) {
      auto l = tree_.left (curr);
      if (! l) return curr;
      curr = l;
    }
  }

  constexpr handle_type max () const {
    auto curr = tree_.root ();
    if (! curr) return curr;
    while (true) {
      auto r = tree_.right (curr);
      if (! r) return curr;
      curr = r;
    }
  }

  constexpr handle_type next (const handle_type& h) const {
    if (! h) return handle_type ();
    auto r = tree_.right (h);
    if (r) {
      auto curr = r;
      while (true) {
        auto l = tree_.left (curr);
        if (! l) return curr;
        curr = l;
      }
    }
    auto curr = h;
    auto p = tree_.parent (curr);
    while (p && same_handle (tree_.right (p), curr)) {
      curr = p;
      p = tree_.parent (p);
    }
    return p;
  }

  constexpr handle_type prev (const handle_type& h) const {
    if (! h) return handle_type ();
    auto l = tree_.left (h);
    if (l) {
      auto curr = l;
      while (true) {
        auto r = tree_.right (curr);
        if (! r) return curr;
        curr = r;
      }
    }
    auto curr = h;
    auto p = tree_.parent (curr);
    while (p && same_handle (tree_.left (p), curr)) {
      curr = p;
      p = tree_.parent (p);
    }
    return p;
  }

  /* Search */
  template < typename K >
  requires (minilib::three_way_comparable_with < K, Key >)
  constexpr handle_type search (const K& k) const {
    auto curr = tree_.root ();
    while (curr) {
      auto res = minilib::compare_three_way::operator () (k, tree_.data (curr)->key ());
      if (res == 0) return curr;
      if (res < 0) {
        curr = tree_.left (curr);
      } else {
        curr = tree_.right (curr);
      }
    }
    return handle_type ();
  }

  template < typename K >
  requires (minilib::three_way_comparable_with < K, Key >)
  constexpr bool contains (const K& k) const {
    return static_cast < bool > (search (k));
  }

  /* Value lookup */
  template < typename K >
  requires (minilib::three_way_comparable_with < K, Key >)
  constexpr Value& at (const K& k) {
    auto h = search (k);
    if (! h) std::terminate ();
    return value (h);
  }

  template < typename K >
  requires (minilib::three_way_comparable_with < K, Key >)
  constexpr const Value& at (const K& k) const {
    auto h = search (k);
    if (! h) std::terminate ();
    return value (h);
  }

  /* Insertion / emplace */
  template < typename K, typename... Args >
  requires (minilib::is_constructible_v < Key, K&& > && minilib::is_constructible_v < Value, Args&&... >)
  constexpr minilib::pair < bool, handle_type > emplace (K&& k, Args&&... args) {
    if (empty ()) {
      handle_type h = tree_.emplace_root (minilib::detail::map_in_place_val, minilib::forward < K > (k), minilib::forward < Args > (args)...);
      size_ = 1;
      return minilib::pair < bool, handle_type > (true, h);
    }
    auto curr = tree_.root ();
    while (true) {
      auto res = minilib::compare_three_way::operator () (k, tree_.data (curr)->key ());
      if (res == 0) {
        return minilib::pair < bool, handle_type > (false, curr);
      }
      if (res < 0) {
        auto nxt = tree_.left (curr);
        if (nxt) {
          curr = nxt;
        } else {
          handle_type h = tree_.emplace_left (curr, minilib::detail::map_in_place_val, minilib::forward < K > (k), minilib::forward < Args > (args)...);
          ++size_;
          return minilib::pair < bool, handle_type > (true, h);
        }
      } else {
        auto nxt = tree_.right (curr);
        if (nxt) {
          curr = nxt;
        } else {
          handle_type h = tree_.emplace_right (curr, minilib::detail::map_in_place_val, minilib::forward < K > (k), minilib::forward < Args > (args)...);
          ++size_;
          return minilib::pair < bool, handle_type > (true, h);
        }
      }
    }
  }

  template < typename K >
  requires (minilib::is_constructible_v < Key, K&& > && minilib::is_truly_default_constructible_v < Value >)
  constexpr minilib::pair < bool, handle_type > insert_default (K&& k) {
    if (empty ()) {
      handle_type h = tree_.emplace_root (minilib::detail::map_in_place_default_val, minilib::forward < K > (k));
      size_ = 1;
      return minilib::pair < bool, handle_type > (true, h);
    }
    auto curr = tree_.root ();
    while (true) {
      auto res = minilib::compare_three_way::operator () (k, tree_.data (curr)->key ());
      if (res == 0) {
        return minilib::pair < bool, handle_type > (false, curr);
      }
      if (res < 0) {
        auto nxt = tree_.left (curr);
        if (nxt) {
          curr = nxt;
        } else {
          handle_type h = tree_.emplace_left (curr, minilib::detail::map_in_place_default_val, minilib::forward < K > (k));
          ++size_;
          return minilib::pair < bool, handle_type > (true, h);
        }
      } else {
        auto nxt = tree_.right (curr);
        if (nxt) {
          curr = nxt;
        } else {
          handle_type h = tree_.emplace_right (curr, minilib::detail::map_in_place_default_val, minilib::forward < K > (k));
          ++size_;
          return minilib::pair < bool, handle_type > (true, h);
        }
      }
    }
  }

  template < typename K, typename V >
  requires (minilib::is_constructible_v < Key, K&& > && minilib::is_constructible_v < Value, V&& >)
  constexpr minilib::pair < bool, handle_type > insert (K&& k, V&& v) {
    return emplace (minilib::forward < K > (k), minilib::forward < V > (v));
  }

  constexpr minilib::pair < bool, handle_type > insert (const minilib::pair < Key, Value >& p)
    requires (minilib::is_copy_constructible_v < Key > && minilib::is_copy_constructible_v < Value >) {
    return emplace (p.first, p.second);
  }

  constexpr minilib::pair < bool, handle_type > insert (minilib::pair < Key, Value >&& p)
    requires (minilib::is_move_constructible_v < Key > && minilib::is_move_constructible_v < Value >) {
    return emplace (minilib::move (p.first), minilib::move (p.second));
  }

  template < typename K, typename M >
  requires (minilib::is_constructible_v < Key, K&& > && minilib::is_constructible_v < Value, M&& > && requires (Value& v, M&& m) { v = minilib::forward < M > (m); })
  constexpr minilib::pair < bool, handle_type > insert_or_assign (K&& k, M&& m) {
    auto res = emplace (minilib::forward < K > (k), minilib::forward < M > (m));
    if (! res.first) {
      value (res.second) = minilib::forward < M > (m);
    }
    return res;
  }

  template < typename K >
  requires (minilib::is_constructible_v < Key, K&& > && minilib::is_truly_default_constructible_v < Value >)
  constexpr Value& operator[] (K&& k) {
    auto res = insert_default (minilib::forward < K > (k));
    return value (res.second);
  }

  /* Removal */
  constexpr void remove (const handle_type& h) requires (minilib::is_destructible_v < Key > && minilib::is_destructible_v < Value >) {
    tree_.remove (h);
    --size_;
  }

  template < typename K >
  requires (minilib::three_way_comparable_with < K, Key > && minilib::is_destructible_v < Key > && minilib::is_destructible_v < Value >)
  constexpr bool remove (const K& k) {
    auto h = search (k);
    if (h) {
      remove (h);
      return true;
    }
    return false;
  }
};

}

#endif
