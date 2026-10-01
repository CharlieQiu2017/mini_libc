#ifndef SET_HPP
#define SET_HPP

#include <stdint.h>
#include <exception>
#include <type_traits.hpp>
#include <utility.hpp>
#include <compare.hpp>
#include <rbtree.hpp>
#include <pair.hpp>
#include <counter.hpp>

namespace minilib {

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T > && minilib::three_way_comparable < T >)
class set {
public:
  using value_type = T;
  using size_type = size_t;
  using tree_type = minilib::rbtree < T >;
  using handle_type = typename tree_type::handle_type;

private:
  tree_type tree_;
  size_t size_;

  static constexpr bool same_handle (const handle_type& a, const handle_type& b) {
    return minilib::compare_three_way::operator () (a, b) == 0;
  }

public:
  constexpr set () : tree_ (), size_ (0) {}

  constexpr void attach_counter (minilib::counter * ctr) {
    tree_.attach_counter (ctr);
  }

  constexpr set (const set& other) requires (minilib::is_copy_constructible_v < T >)
    : tree_ (other.tree_), size_ (other.size_) {}

  constexpr set (set&& other)
    : tree_ (minilib::move (other.tree_)), size_ (other.size_) {
    other.size_ = 0;
  }

  constexpr set& operator= (const set& other) requires (minilib::is_copy_constructible_v < T >) {
    if (this == minilib::addressof (other)) return *this;
    tree_ = other.tree_;
    size_ = other.size_;
    return *this;
  }

  constexpr set& operator= (set&& other) {
    if (this == minilib::addressof (other)) return *this;
    tree_ = minilib::move (other.tree_);
    size_ = other.size_;
    other.size_ = 0;
    return *this;
  }

  constexpr ~set () = default;

  constexpr size_t size () const { return size_; }
  constexpr bool empty () const { return size_ == 0; }

  constexpr void clear () {
    tree_.clear ();
    size_ = 0;
  }

  /* Read-only access to node value */
  constexpr const T * data (const handle_type& h) const {
    return tree_.data (h);
  }

  constexpr const T& get (const handle_type& h) const {
    return *tree_.data (h);
  }

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
      if (! l) return curr;
      curr = l;
    }
  }

  constexpr handle_type max () const {
    auto curr = tree_.root ();
    if (! curr.is_not_null ()) return curr;
    while (true) {
      auto r = tree_.right_unsafe (curr);
      if (! r) return curr;
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
      p = tree_.parent_unsafe (p);
    }
    return p;
  }

  /* Search */
  template < typename K >
  requires (minilib::three_way_comparable_with < K, T >)
  constexpr handle_type search (const K& val) const {
    auto curr = tree_.root ();
    while (curr.is_not_null ()) {
      auto res = minilib::compare_three_way::operator () (val, *tree_.data_unsafe (curr));
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
  requires (minilib::three_way_comparable_with < K, T >)
  constexpr bool contains (const K& val) const {
    return static_cast < bool > (search (val));
  }

  /* Insertion */
  constexpr minilib::pair < bool, handle_type > insert (const T& val) requires (minilib::is_copy_constructible_v < T >) {
    if (empty ()) {
      handle_type h = tree_.emplace_root (val);
      size_ = 1;
      return minilib::pair < bool, handle_type > (true, h);
    }
    auto curr = tree_.root ();
    while (true) {
      auto res = minilib::compare_three_way::operator () (val, *tree_.data_unsafe (curr));
      if (res == 0) {
        return minilib::pair < bool, handle_type > (false, curr);
      }
      if (res < 0) {
        auto nxt = tree_.left_unsafe (curr);
        if (nxt.is_not_null ()) {
          curr = nxt;
        } else {
          handle_type h = tree_.emplace_left (curr, val);
          ++size_;
          return minilib::pair < bool, handle_type > (true, h);
        }
      } else {
        auto nxt = tree_.right_unsafe (curr);
        if (nxt.is_not_null ()) {
          curr = nxt;
        } else {
          handle_type h = tree_.emplace_right (curr, val);
          ++size_;
          return minilib::pair < bool, handle_type > (true, h);
        }
      }
    }
  }

  constexpr minilib::pair < bool, handle_type > insert (T&& val) requires (minilib::is_move_constructible_v < T >) {
    if (empty ()) {
      handle_type h = tree_.emplace_root (minilib::move (val));
      size_ = 1;
      return minilib::pair < bool, handle_type > (true, h);
    }
    auto curr = tree_.root ();
    while (true) {
      auto res = minilib::compare_three_way::operator () (val, *tree_.data_unsafe (curr));
      if (res == 0) {
        return minilib::pair < bool, handle_type > (false, curr);
      }
      if (res < 0) {
        auto nxt = tree_.left_unsafe (curr);
        if (nxt.is_not_null ()) {
          curr = nxt;
        } else {
          handle_type h = tree_.emplace_left (curr, minilib::move (val));
          ++size_;
          return minilib::pair < bool, handle_type > (true, h);
        }
      } else {
        auto nxt = tree_.right_unsafe (curr);
        if (nxt.is_not_null ()) {
          curr = nxt;
        } else {
          handle_type h = tree_.emplace_right (curr, minilib::move (val));
          ++size_;
          return minilib::pair < bool, handle_type > (true, h);
        }
      }
    }
  }

  /* Removal */
  constexpr void remove (const handle_type& h) requires (minilib::is_destructible_v < T >) {
    tree_.remove (h);
    --size_;
  }

  template < typename K >
  requires (minilib::three_way_comparable_with < K, T > && minilib::is_destructible_v < T >)
  constexpr bool remove (const K& val) {
    auto h = search (val);
    if (h) {
      remove (h);
      return true;
    }
    return false;
  }
};

}

#endif
