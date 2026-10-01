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

template < typename Key, typename Value >
requires (std::is_object_v < Key > && ! std::is_array_v < Key > && ! std::is_const_v < Key > && ! std::is_volatile_v < Key >
          && minilib::three_way_comparable < Key >
          && std::is_object_v < Value > && ! std::is_array_v < Value > && ! std::is_const_v < Value > && ! std::is_volatile_v < Value >)
class map_pair {
public:
  minilib::pair < minilib::raw_array < Key, 1 >, minilib::raw_array < Value, 1 > > storage;

  constexpr Key& key () { return *(storage.first.data ()); }
  constexpr const Key& key () const { return *(storage.first.data ()); }
  constexpr Value& value () { return *(storage.second.data ()); }
  constexpr const Value& value () const { return *(storage.second.data ()); }
  constexpr Key * key_ptr () { return storage.first.data (); }
  constexpr const Key * key_ptr () const { return storage.first.data (); }
  constexpr Value * value_ptr () { return storage.second.data (); }
  constexpr const Value * value_ptr () const { return storage.second.data (); }

  friend constexpr minilib::order_result compare_three_way (const map_pair& a, const map_pair& b) {
    return minilib::compare_three_way::operator() (*(a.storage.first.data ()), *(b.storage.first.data ()));
  }
};

}

/* We cannot provide non-const access to the key part. However, we do need to destruct keys in the destructor.
   Therefore, we require Key to be publicly destructible. We don't see how to support private destructor keys.
 */
template < typename Key, typename Value >
requires (std::is_object_v < Key > && ! std::is_array_v < Key > && ! std::is_const_v < Key > && ! std::is_volatile_v < Key >
          && minilib::is_destructible_v < Key > && minilib::three_way_comparable < Key >
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
    return tree_.data (h)->key_ptr ();
  }

  constexpr Value& value (const handle_type& h) {
    return tree_.data (h)->value ();
  }

  constexpr const Value& value (const handle_type& h) const {
    return tree_.data (h)->value ();
  }

  constexpr Value * value_ptr (const handle_type& h) {
    return tree_.data (h)->value_ptr ();
  }

  constexpr const Value * value_ptr (const handle_type& h) const {
    return tree_.data (h)->value_ptr ();
  }

private:
  /* Unsafe node accessors */
  constexpr const Key& key_unsafe (const handle_type& h) const {
    return tree_.data_unsafe (h)->key ();
  }

  constexpr const Key * key_ptr_unsafe (const handle_type& h) const {
    return tree_.data_unsafe (h)->key_ptr ();
  }

  constexpr Value& value_unsafe (const handle_type& h) {
    return tree_.data_unsafe (h)->value ();
  }

  constexpr const Value& value_unsafe (const handle_type& h) const {
    return tree_.data_unsafe (h)->value ();
  }

  constexpr Value * value_ptr_unsafe (const handle_type& h) {
    return tree_.data_unsafe (h)->value_ptr ();
  }

  constexpr const Value * value_ptr_unsafe (const handle_type& h) const {
    return tree_.data_unsafe (h)->value_ptr ();
  }

public:
  /* Navigation helpers */
  constexpr handle_type root () const { return tree_.root (); }
  constexpr handle_type left (const handle_type& h) const { return tree_.left (h); }
  constexpr handle_type right (const handle_type& h) const { return tree_.right (h); }
  constexpr handle_type parent (const handle_type& h) const { return tree_.parent (h); }

  constexpr handle_type min () const {
    auto curr = tree_.root ();
    if (! curr.is_not_null ()) return curr;
    while (true) {
      auto l = tree_.left_unsafe (curr);
      if (! l.is_not_null ()) return curr;
      curr = l;
    }
  }

  constexpr handle_type max () const {
    auto curr = tree_.root ();
    if (! curr.is_not_null ()) return curr;
    while (true) {
      auto r = tree_.right_unsafe (curr);
      if (! r.is_not_null ()) return curr;
      curr = r;
    }
  }

  constexpr handle_type next (const handle_type& h) const {
    uint32_t check = tree_.check_maybe_null (h);
    if (check == 0) return handle_type ();
    if (check == 1) std::terminate ();
    auto r = tree_.right_unsafe (h);
    if (r.is_not_null ()) {
      auto curr = r;
      while (true) {
        auto l = tree_.left_unsafe (curr);
        if (! l.is_not_null ()) return curr;
        curr = l;
      }
    }
    auto curr = h;
    auto p = tree_.parent_unsafe (curr);
    while (p.is_not_null () && same_handle (tree_.right_unsafe (p), curr)) {
      curr = p;
      p = tree_.parent_unsafe (p);
    }
    return p;
  }

  constexpr handle_type prev (const handle_type& h) const {
    uint32_t check = tree_.check_maybe_null (h);
    if (check == 0) return handle_type ();
    if (check == 1) std::terminate ();
    auto l = tree_.left_unsafe (h);
    if (l.is_not_null ()) {
      auto curr = l;
      while (true) {
        auto r = tree_.right_unsafe (curr);
        if (! r.is_not_null ()) return curr;
        curr = r;
      }
    }
    auto curr = h;
    auto p = tree_.parent_unsafe (curr);
    while (p.is_not_null () && same_handle (tree_.left_unsafe (p), curr)) {
      curr = p;
      p = tree_.parent (p);
    }
    return p;
  }

  /* Search */
  template < typename K >
  requires (minilib::three_way_comparable_with < const K&, const Key& >)
  constexpr handle_type search (const K& k) const {
    auto curr = tree_.root ();
    while (curr.is_not_null ()) {
      auto res = minilib::compare_three_way::operator () (k, tree_.data_unsafe (curr)->key ());
      if (res == 0) return curr;
      if (res < 0) {
        curr = tree_.left_unsafe (curr);
      } else {
        curr = tree_.right_unsafe (curr);
      }
    }
    return handle_type ();
  }

  template < typename K >
  requires (minilib::three_way_comparable_with < const K&, const Key& >)
  constexpr bool contains (const K& k) const {
    return search (k).is_not_null ();
  }

  /* Value lookup */
  template < typename K >
  requires (minilib::three_way_comparable_with < const K&, const Key& >)
  constexpr Value& at (const K& k) {
    auto h = search (k);
    if (! h.is_not_null ()) std::terminate ();
    return value_unsafe (h);
  }

  template < typename K >
  requires (minilib::three_way_comparable_with < const K&, const Key& >)
  constexpr const Value& at (const K& k) const {
    auto h = search (k);
    if (! h.is_not_null ()) std::terminate ();
    return value_unsafe (h);
  }

  /* Insertion / emplace */

  /* DANGEROUS: This function is deliberately left public to allow cases where the caller manually manages the lifetime of Value. */
  template < typename K >
  requires (minilib::three_way_comparable_with < K&&, const Key& > && minilib::is_constructible_v < Key, K&& >)
  constexpr minilib::pair < bool, handle_type > emplace_null (K&& k) {
    if (empty ()) {
      handle_type h = tree_.emplace_root ();
      auto p = tree_.data (h);
      minilib::construct_at < Key > (p->key_ptr (), minilib::forward < K&& > (k));
      size_ = 1;
      return minilib::pair < bool, handle_type > (true, h);
    }
    auto curr = tree_.root ();
    while (true) {
      auto res = minilib::compare_three_way::operator () (minilib::forward < K&& > (k), minilib::forward < const Key& > (tree_.data (curr)->key ()));
      if (res == 0) {
        return minilib::pair < bool, handle_type > (false, curr);
      }
      if (res < 0) {
        auto nxt = tree_.left_unsafe (curr);
        if (nxt.is_not_null ()) {
          curr = nxt;
        } else {
          handle_type h = tree_.emplace_left (curr);
	  auto p = tree_.data (h);
	  minilib::construct_at < Key > (p->key_ptr (), minilib::forward < K&& > (k));
          ++size_;
          return minilib::pair < bool, handle_type > (true, h);
        }
      } else {
        auto nxt = tree_.right_unsafe (curr);
        if (nxt.is_not_null ()) {
          curr = nxt;
        } else {
          handle_type h = tree_.emplace_right (curr);
	  auto p = tree_.data (h);
	  minilib::construct_at < Key > (p->key_ptr (), minilib::forward < K&& > (k));
          ++size_;
          return minilib::pair < bool, handle_type > (true, h);
        }
      }
    }
  }

  template < typename K, typename... Args >
  requires (minilib::three_way_comparable_with < K&&, const Key& > && minilib::is_constructible_v < Key, K&& > && minilib::is_constructible_v < Value, Args&&... >)
  constexpr minilib::pair < bool, handle_type > emplace (K&& k, Args&&... args) {
    auto res = emplace_null (minilib::forward < K&& > (k));
    if (! res.first) return res;
    auto p = tree_.data_unsafe (res.second);
    minilib::construct_at < Value > (p->value_ptr (), minilib::forward < Args&& > (args)...);
    return res;
  }

  template < typename K >
  requires (minilib::three_way_comparable_with < K&&, const Key& > && minilib::is_constructible_v < Key, K&& > && minilib::is_truly_default_constructible_v < Value >)
  constexpr minilib::pair < bool, handle_type > insert_default (K&& k) {
    auto res = emplace_null (minilib::forward < K&& > (k));
    if (! res.first) return res;
    auto p = tree_.data_unsafe (res.second);
    minilib::default_construct_at < Value > (p->value_ptr ());
    return res;
  }

  template < typename K, typename V >
  requires (minilib::three_way_comparable_with < K&&, const Key& > && minilib::is_constructible_v < Key, K&& > && minilib::is_constructible_v < Value, V&& >)
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

  template < typename K, typename M >
  requires (minilib::three_way_comparable_with < K&&, const Key& > && minilib::is_constructible_v < Key, K&& > && minilib::is_constructible_v < Value, M&& > && requires (Value& v, M&& m) { v = minilib::forward < M > (m); })
  constexpr minilib::pair < bool, handle_type > insert_or_assign (K&& k, M&& m) {
    auto res = emplace (minilib::forward < K > (k), minilib::forward < M > (m));
    if (! res.first) {
      value_unsafe (res.second) = minilib::forward < M > (m);
    }
    return res;
  }

  template < typename K >
  requires (minilib::three_way_comparable_with < K&&, const Key& > && minilib::is_constructible_v < Key, K&& > && minilib::is_truly_default_constructible_v < Value >)
  constexpr Value& operator[] (K&& k) {
    auto res = insert_default (minilib::forward < K > (k));
    return value_unsafe (res.second);
  }

  /* Removal */
  constexpr void remove (const handle_type& h) requires (minilib::is_destructible_v < Key >) {
    auto p = tree_.data (h);
    minilib::destroy_at < Key > (p->key_ptr ());
    minilib::destroy_at < Value > (p->value_ptr ());
    tree_.remove (h);
    --size_;
  }

  /* For callers who have destructed Value manually */
  constexpr void inform_destruct (const handle_type& h) requires (minilib::is_destructible_v < Key >) {
    auto p = tree_.data (h);
    minilib::destroy_at < Key > (p->key_ptr ());
    tree_.remove (h);
    --size_;
  }

  template < typename K >
  requires (minilib::three_way_comparable_with < const K&, const Key& > && minilib::is_destructible_v < Key > && minilib::is_destructible_v < Value >)
  constexpr bool remove (const K& k) {
    auto h = search (k);
    if (h.is_not_null ()) {
      remove (h);
      return true;
    }
    return false;
  }
};

}

#endif
