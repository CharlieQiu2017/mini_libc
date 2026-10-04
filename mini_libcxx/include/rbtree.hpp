/* Red-black tree

   Formal specification:

   Inductive rb_color :=
   | black
   | red.

   Inductive rb_tree :=
   | nil
   | node (x : val_t) (l : rb_tree) (r : rb_tree) (c : rb_color).

   Definition rb_insert_rebalance_left x l' r c (is_bh_equal : bool) : rb_tree * bool :=
     match c with
     | black =>
         if is_bh_equal then
           (node x l' r black, true)
         else
           match l' with
           | nil => (* Impossible *) (nil, true)
           | node l'_x l'_l l'_r _ => (* l'_c must be black *)
               match rb_tree_color l'_l with
               | black =>
                   match rb_tree_color l'_r with
                   | black =>
                       (node x (node l'_x l'_l l'_r red) r black, true)
                   | red =>
                       match l'_r with
                       | nil => (* Impossible *) (nil, true)
                       | node l'_r_x l'_r_l l'_r_r l'_r_c =>
                           (node l'_r_x (node l'_x l'_l l'_r_l black) (node x l'_r_r r black) red, true)
                       end
                   end
               | red =>
                   match l'_l with
                   | nil => (* Impossible *) (nil, true)
                   | node l'_l_x l'_l_l l'_l_r l'_l_c =>
                       (node l'_x (node l'_l_x l'_l_l l'_l_r black) (node x l'_r r black) red, true)
                   end
               end
           end
     | red =>
         if is_bh_equal then
           (node x l' r black, false)
         else
           match l' with
           | nil => (* Impossible *) (nil, true)
           | node l'_x l'_l l'_r l'_c => (* l'_c must be black *)
               match rb_tree_color l'_l with
               | black =>
                   match rb_tree_color l'_r with
                   | black =>
                       (node x (node l'_x l'_l l'_r red) r black, false)
                   | red =>
                       match l'_r with
                       | nil => (* Impossible *) (nil, true)
                       | node l'_r_x l'_r_l l'_r_r l'_r_c =>
                           (node l'_r_x (node l'_x l'_l l'_r_l red) (node x l'_r_r r red) black, false)
                       end
                   end
               | red =>
                   match rb_tree_color l'_r with
                   | black =>
                       match l'_l with
                       | nil => (* Impossible *) (nil, true)
                       | node l'_l_x l'_l_l l'_l_r l'_l_c =>
                           (node l'_x (node l'_l_x l'_l_l l'_l_r red) (node x l'_r r red) black, false)
                       end
                   | red => (* Impossible *) (nil, true)
                   end
               end
           end
     end.

   Definition rb_insert_rebalance_right x l r' c (is_bh_equal : bool) : rb_tree * bool :=
     match c with
     | black =>
         if is_bh_equal then
           (node x l r' black, true)
         else
           match r' with
           | nil => (* Impossible *) (nil, true)
           | node r'_x r'_l r'_r r'_c => (* r'_c must be black *)
               match rb_tree_color r'_l with
               | black =>
                   match rb_tree_color r'_r with
                   | black =>
                       (node x l (node r'_x r'_l r'_r red) black, true)
                   | red =>
                       match r'_r with
                       | nil => (* Impossible *) (nil, true)
                       | node r'_r_x r'_r_l r'_r_r r'_r_c =>
                           (node r'_x (node x l r'_l black) (node r'_r_x r'_r_l r'_r_r black) red, true)
                       end
                   end
               | red =>
                   match r'_l with
                   | nil => (* Impossible *) (nil, true)
                   | node r'_l_x r'_l_l r'_l_r r'_l_c =>
                       (node r'_l_x (node x l r'_l_l black) (node r'_x r'_l_r r'_r black) red, true)
                   end
               end
           end
     | red =>
         if is_bh_equal then
           (node x l r' black, false)
         else
           match r' with
           | nil => (* Impossible *) (nil, true)
           | node r'_x r'_l r'_r r'_c => (* r'_c must be black *)
               match rb_tree_color r'_l with
               | black =>
                   match rb_tree_color r'_r with
                   | black =>
                       (node x l (node r'_x r'_l r'_r red) black, false)
                   | red =>
                       match r'_r with
                       | nil => (* Impossible *) (nil, true)
                       | node r'_r_x r'_r_l r'_r_r r'_r_c =>
                           (node r'_x (node x l r'_l red) (node r'_r_x r'_r_l r'_r_r red) black, false)
                       end
                   end
               | red =>
                   match rb_tree_color r'_r with
                   | black =>
                       match r'_l with
                       | nil => (* Impossible *) (nil, true)
                       | node r'_l_x r'_l_l r'_l_r r'_l_c =>
                           (node r'_l_x (node x l r'_l_l red) (node r'_x r'_l_r r'_r red) black, false)
                       end
                   | red => (* Impossible *) (nil, true)
                   end
               end
           end
     end.

   Definition rb_delete_rebalance_left x l' r c (is_bh_equal : bool) : rb_tree * bool :=
     match c with
     | black =>
         if is_bh_equal then
           (node x l' r black, true)
         else
           match rb_tree_color l' with
           | black =>
               match r with
               | nil => (* Impossible *) (nil, true)
               | node r_x r_l r_r r_c =>
                   match r_c with
                   | black =>
                       match r_l with
                       | nil =>
                           (node r_x (node x l' r_l red) r_r black, false)
                       | node r_l_x r_l_l r_l_r r_l_c =>
                           match r_l_c with
                           | black =>
                               (node r_x (node x l' r_l red) r_r black, false)
                           | red =>
                               (node r_l_x (node x l' r_l_l black) (node r_x r_l_r r_r black) red, false)
                           end
                       end
                   | red =>
                       match r_l with
                       | nil => (* Impossible *) (nil, true)
                       | node r_l_x r_l_l r_l_r r_l_c =>
                           match rb_tree_color r_l_l with
                           | black =>
                               (node r_x (node r_l_x (node x l' r_l_l red) r_l_r black) r_r red, false)
                           | red =>
                               match r_l_l with
                               | nil => (* Impossible *) (nil, true)
                               | node r_l_l_x r_l_l_l r_l_l_r r_l_l_c =>
                                   (node r_x (node r_l_l_x (node x l' r_l_l_l black) (node r_l_x r_l_l_r r_l_r black) red) r_r black, true)
                               end
                           end
                       end
                   end
               end
           | red =>
               match l' with
               | nil => (* Impossible *) (nil, true)
               | node l'_x l'_l l'_r l'_c =>
                   (node x (node l'_x l'_l l'_r black) r black, true)
               end
           end
     | red =>
         if is_bh_equal then
           match rb_tree_color l' with
           | black =>
               (node x l' r red, true)
           | red => (* Impossible *) (nil, true)
           end
         else
           match rb_tree_color l' with
           | black =>
               match r with
               | nil => (* Impossible *) (nil, true)
               | node r_x r_l r_r r_c =>
                   match rb_tree_color r_l with
                   | black =>
                       (node r_x (node x l' r_l red) r_r black, true)
                   | red =>
                       match r_l with
                       | nil => (* Impossible *) (nil, true)
                       | node r_l_x r_l_l r_l_r r_l_c =>
                           (node r_l_x (node x l' r_l_l black) (node r_x r_l_r r_r black) red, true)
                       end
                   end
               end
           | red =>
               match l' with
               | nil => (* Impossible *) (nil, true)
               | node l'_x l'_l l'_r l'_c =>
                   (node x (node l'_x l'_l l'_r black) r red, true)
               end
           end
     end.

   Definition rb_delete_rebalance_right x l r' c (is_bh_equal : bool) : rb_tree * bool :=
     match c with
     | black =>
         if is_bh_equal then
           (node x l r' black, true)
         else
           match rb_tree_color r' with
           | black =>
               match l with
               | nil => (* Impossible *) (nil, true)
               | node l_x l_l l_r l_c =>
                   match l_c with
                   | black =>
                       match l_r with
                       | nil =>
                           (node l_x l_l (node x l_r r' red) black, false)
                       | node l_r_x l_r_l l_r_r l_r_c =>
                           match l_r_c with
                           | black =>
                               (node l_x l_l (node x l_r r' red) black, false)
                           | red =>
                               (node l_r_x (node l_x l_l l_r_l black) (node x l_r_r r' black) red, false)
                           end
                       end
                   | red =>
                       match l_r with
                       | nil => (* Impossible *) (nil, true)
                       | node l_r_x l_r_l l_r_r l_r_c =>
                           match rb_tree_color l_r_r with
                           | black =>
                               (node l_x l_l (node l_r_x l_r_l (node x l_r_r r' red) black) red, false)
                           | red =>
                               match l_r_r with
                               | nil => (* Impossible *) (nil, true)
                               | node l_r_r_x l_r_r_l l_r_r_r l_r_r_c =>
                                   (node l_x l_l (node l_r_r_x (node l_r_x l_r_l l_r_r_l black) (node x l_r_r_r r' black) red) black, true)
                               end
                           end
                       end
                   end
               end
           | red =>
               match r' with
               | nil => (* Impossible *) (nil, true)
               | node r'_x r'_l r'_r r'_c =>
                   (node x l (node r'_x r'_l r'_r black) black, true)
               end
           end
     | red =>
         if is_bh_equal then
           match rb_tree_color r' with
           | black =>
               (node x l r' red, true)
           | red => (* Impossible *) (nil, true)
           end
         else
           match rb_tree_color r' with
           | black =>
               match l with
               | nil => (* Impossible *) (nil, true)
               | node l_x l_l l_r l_c =>
                   match rb_tree_color l_r with
                   | black =>
                       (node l_x l_l (node x l_r r' red) black, true)
                   | red =>
                       match l_r with
                       | nil => (* Impossible *) (nil, true)
                       | node l_r_x l_r_l l_r_r l_r_c =>
                           (node l_r_x (node l_x l_l l_r_l black) (node x l_r_r r' black) red, true)
                       end
                   end
               end
           | red =>
               match r' with
               | nil => (* Impossible *) (nil, true)
               | node r'_x r'_l r'_r r'_c =>
                   (node x l (node r'_x r'_l r'_r black) red, true)
               end
           end
     end.

   Fixpoint delete_leftmost (tree : rb_tree) : option (rb_tree * bool * val_t) :=
     match tree with
     | nil => None
     | node x nil r c =>
         Some (r, match c with black => false | red => true end, x)
     | node x l r c =>
         let ret := delete_leftmost l in
         match ret with
         | None => None
         | Some (l', b, v) =>
             let (tree', b') := rb_delete_rebalance_left x l' r c b in
             Some (tree', b', v)
         end
     end.

   Definition delete_root (tree : rb_tree) : (rb_tree * bool) :=
     match tree with
     | nil => (nil, true)
     | node x l nil c =>
       (l, match c with black => false | red => true end)
     | node x l r c =>
       match delete_leftmost r with
       | None => (nil, true)
       | Some (r', b, v) =>
           rb_delete_rebalance_right v l r' c b
       end
     end.

   To attach a leaf node to node X,
   first attach the leaf node and mark it red; then:
   * If it is the left child of X, call (rb_insert_rebalance_left X L' R C true) on X, where L' is the just attched leaf node and C is color of X.
   * If it is the right child of X, call rb_insert_rebalance_right.
   * After the function reorganizes the subtree rooted at X, get the returned bool value, go to parent of X,
     and call rb_insert_rebalance_left or rb_insert_rebalance_right, with is_bh_equal set to the just returned bool value.
     Repeat this process until we reach root.

   To delete a node X, call delete_root on the subtree rooted at X.
   After this function reorganizes the subtree rooted at X, get the returned bool value, go to parent of X,
   and call rb_delete_rebalance_left or rb_delete_rebalance_right, with is_bh_equal set to the just returned bool value.
   Repeat this process until we reach root.
 */

#ifndef RBTREE_HPP
#define RBTREE_HPP

#include <stdint.h>
#include <tls.h>
#include <exception>
#include <type_traits.hpp>
#include <tagged_ptr.hpp>
#include <raw_array.hpp>
#include <utility.hpp>
#include <counter.hpp>
#include <pair.hpp>

namespace minilib {

namespace detail {

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
struct rb_node {
  minilib::tagged_ptr < rb_node > left, right, parent; /* If static_cast < bool > (parent) == false, this is root node */
  size_t flags;

#define RB_IS_LEFT_CHILD 0x01
#define RB_IS_RED 0x02

  minilib::raw_array < T, 1 > storage;
};

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
struct rbtree_runtime {
  size_t container_id;
  minilib::tagged_ptr < minilib::detail::rb_node < T > > root;
};

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
struct rbtree_constexpr {
  struct rbtree_runtime < T > runtime;
  minilib::counter local_counter;
  minilib::counter * global_counter;
};

}

/* We support passing in a destructor function as template parameter. See map.hpp on why this is necessary */
template < typename T, void (*Destructor)(T *) = nullptr >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class rbtree {
  union {
    minilib::detail::rbtree_runtime < T > runtime_;
    minilib::detail::rbtree_constexpr < T > * constexpr_;
  };

public:
  using node_type = minilib::detail::rb_node < T >;
  using ptr_type = minilib::tagged_ptr < node_type >;
  using handle_type = minilib::counted_ref < node_type >;

private:
  constexpr minilib::detail::rbtree_runtime < T > * get_runtime () {
    minilib::detail::rbtree_runtime < T > * runtime = nullptr;
    if consteval {
      runtime = minilib::addressof (constexpr_->runtime);
    } else {
      runtime = minilib::addressof (runtime_);
    }
    return runtime;
  }

  constexpr const minilib::detail::rbtree_runtime < T > * get_runtime () const {
    const minilib::detail::rbtree_runtime < T > * runtime = nullptr;
    if consteval {
      runtime = minilib::addressof (constexpr_->runtime);
    } else {
      runtime = minilib::addressof (runtime_);
    }
    return runtime;
  }

  constexpr uint64_t get_local_counter () {
    if consteval {
      return constexpr_->local_counter.get ();
    } else {
      return 0;
    }
  }

  static constexpr bool is_red (ptr_type p) {
    return static_cast < bool > (p) && ((p.data_unsafe ()->flags & RB_IS_RED) != 0);
  }

  static constexpr bool is_black (ptr_type p) {
    return !static_cast < bool > (p) || ((p.data_unsafe ()->flags & RB_IS_RED) == 0);
  }

  static constexpr bool same_ptr (ptr_type a, ptr_type b) {
    return minilib::compare_three_way::operator () (a, b) == 0;
  }

  static constexpr void set_red (ptr_type p) {
    p.data_unsafe ()->flags |= RB_IS_RED;
  }

  static constexpr void set_black (ptr_type p) {
    p.data_unsafe ()->flags &= ~static_cast < size_t > (RB_IS_RED);
  }

  static constexpr void set_color (ptr_type p, bool red) {
    if (red) set_red (p);
    else set_black (p);
  }

  static constexpr void set_left (ptr_type parent, ptr_type child) {
    parent.data_unsafe ()->left = child;
    if (child) {
      child.data_unsafe ()->parent = parent;
      child.data_unsafe ()->flags |= RB_IS_LEFT_CHILD;
    }
  }

  static constexpr void set_left_no_check (ptr_type parent, ptr_type child) {
    parent.data_unsafe ()->left = child;
    child.data_unsafe ()->parent = parent;
    child.data_unsafe ()->flags |= RB_IS_LEFT_CHILD;
  }

  /* If we know that child is the left child of some other node, then we don't even need to set flags */
  static constexpr void set_left_no_check_no_flag (ptr_type parent, ptr_type child) {
    parent.data_unsafe ()->left = child;
    child.data_unsafe ()->parent = parent;
  }

  static constexpr void set_left (ptr_type parent, const std::nullptr_t&) {
    parent.data_unsafe ()->left = nullptr;
  }

  static constexpr void set_right (ptr_type parent, ptr_type child) {
    parent.data_unsafe ()->right = child;
    if (child) {
      child.data_unsafe ()->parent = parent;
      child.data_unsafe ()->flags &= ~static_cast < size_t > (RB_IS_LEFT_CHILD);
    }
  }

  static constexpr void set_right_no_check (ptr_type parent, ptr_type child) {
    parent.data_unsafe ()->right = child;
    child.data_unsafe ()->parent = parent;
    child.data_unsafe ()->flags &= ~static_cast < size_t > (RB_IS_LEFT_CHILD);
  }

  static constexpr void set_right_no_check_no_flag (ptr_type parent, ptr_type child) {
    parent.data_unsafe ()->right = child;
    child.data_unsafe ()->parent = parent;
  }

  static constexpr void set_right (ptr_type parent, const std::nullptr_t&) {
    parent.data_unsafe ()->right = nullptr;
  }

  constexpr void set_root (ptr_type r) {
    auto runtime = get_runtime ();
    runtime->root = r;
    if (r) {
      r.data_unsafe ()->parent = nullptr;
      r.data_unsafe ()->flags &= ~static_cast < size_t > (RB_IS_LEFT_CHILD);
    }
  }

  constexpr void set_root_no_check (ptr_type r) {
    auto runtime = get_runtime ();
    runtime->root = r;
    r.data_unsafe ()->parent = nullptr;
    r.data_unsafe ()->flags &= ~static_cast < size_t > (RB_IS_LEFT_CHILD);
  }

  constexpr void set_root (const std::nullptr_t&) {
    auto runtime = get_runtime ();
    runtime->root = nullptr;
  }

  constexpr ptr_type allocate_node () {
    auto runtime = get_runtime ();
    auto p = ptr_type::allocate (runtime->container_id, get_local_counter ());
    minilib::construct_at < node_type > (p.data_unsafe ());
    p.data_unsafe ()->left = nullptr;
    p.data_unsafe ()->right = nullptr;
    p.data_unsafe ()->parent = nullptr;
    p.data_unsafe ()->flags = 0;
    return p;
  }

  /* As an optimization, we try to avoid unnecessary recoloring, and avoid unnecessary checks in set_left/set_right */
  static constexpr minilib::pair < ptr_type, bool > do_rb_insert_rebalance_left (ptr_type X, bool is_bh_equal) {
    bool c_red = is_red (X);
    ptr_type L = X.data_unsafe ()->left;
    if (!c_red) {
      /* X is black */
      if (is_bh_equal) {
        /* set_black (X); */
        return {X, true};
      } else {
	/* L is black */
        ptr_type LL = L.data_unsafe ()->left;
        ptr_type LR = L.data_unsafe ()->right;
        if (is_black (LL)) {
	  /* LL is black */
          if (is_black (LR)) {
	    /* LR is black */
            set_red (L);
            /* set_black (X); */
            return {X, true};
          } else {
	    /* LR is red */
            ptr_type LRL = LR.data_unsafe ()->left;
            ptr_type LRR = LR.data_unsafe ()->right;
            set_right (L, LRL);
            set_left (X, LRR);
            /* set_black (L); */
            /* set_black (X); */
            set_left_no_check_no_flag (LR, L);
            set_right_no_check (LR, X);
            /* set_red (LR); */
            return {LR, true};
          }
        } else {
	  /* LL is red */
          set_left (X, LR);
          set_black (LL);
          /* set_black (X); */
          set_right_no_check (L, X);
          set_red (L);
          return {L, true};
        }
      }
    } else {
      /* X is red */
      if (is_bh_equal) {
        set_black (X);
        return {X, false};
      } else {
	/* L is black */
        ptr_type LL = L.data_unsafe ()->left;
        ptr_type LR = L.data_unsafe ()->right;
        if (is_black (LL)) {
	  /* LL is black */
          if (is_black (LR)) {
	    /* LR is black */
            set_red (L);
            set_black (X);
            return {X, false};
          } else {
	    /* LR is red */
            ptr_type LRL = LR.data_unsafe ()->left;
            ptr_type LRR = LR.data_unsafe ()->right;
            set_right (L, LRL);
            set_left (X, LRR);
            set_red (L);
            /* set_red (X); */
            set_left_no_check_no_flag (LR, L);
            set_right_no_check (LR, X);
            set_black (LR);
            return {LR, false};
          }
        } else {
	  /* LL is red */
	  /* LR is black */
          set_left (X, LR);
          /* set_red (LL); */
          /* set_red (X); */
          set_right_no_check (L, X);
          /* set_black (L); */
          return {L, false};
        }
      }
    }
  }

  static constexpr minilib::pair < ptr_type, bool > do_rb_insert_rebalance_right (ptr_type X, bool is_bh_equal) {
    bool c_red = is_red (X);
    ptr_type R = X.data_unsafe ()->right;
    if (!c_red) {
      /* X is black */
      if (is_bh_equal) {
        /* set_black (X); */
        return {X, true};
      } else {
	/* R is black */
        ptr_type RL = R.data_unsafe ()->left;
        ptr_type RR = R.data_unsafe ()->right;
        if (is_black (RL)) {
	  /* RL is black */
          if (is_black (RR)) {
	    /* RR is black */
            set_red (R);
            /* set_black (X); */
            return {X, true};
          } else {
	    /* RR is red */
            set_right (X, RL);
            /* set_black (X); */
            set_black (RR);
            set_left_no_check (R, X);
            set_red (R);
            return {R, true};
          }
        } else {
	  /* RL is red */
          ptr_type RLL = RL.data_unsafe ()->left;
          ptr_type RLR = RL.data_unsafe ()->right;
          set_right (X, RLL);
          set_left (R, RLR);
          /* set_black (X); */
          /* set_black (R); */
          set_left_no_check (RL, X);
          set_right_no_check_no_flag (RL, R);
          /* set_red (RL); */
          return {RL, true};
        }
      }
    } else {
      /* X is red */
      if (is_bh_equal) {
        set_black (X);
        return {X, false};
      } else {
	/* R is black */
        ptr_type RL = R.data_unsafe ()->left;
        ptr_type RR = R.data_unsafe ()->right;
        if (is_black (RL)) {
	  /* RL is black */
          if (is_black (RR)) {
	    /* RR is black */
            set_red (R);
            set_black (X);
            return {X, false};
          } else {
	    /* RR is red */
            set_right (X, RL);
            /* set_red (X); */
            /* set_red (RR); */
            set_left_no_check (R, X);
            /* set_black (R); */
            return {R, false};
          }
        } else {
	  /* RL is red */
	  /* RR is black */
          ptr_type RLL = RL.data_unsafe ()->left;
          ptr_type RLR = RL.data_unsafe ()->right;
          set_right (X, RLL);
          set_left (R, RLR);
          /* set_red (X); */
          set_red (R);
          set_left_no_check (RL, X);
          set_right_no_check_no_flag (RL, R);
          set_black (RL);
          return {RL, false};
        }
      }
    }
  }

  static constexpr minilib::pair < ptr_type, bool > do_rb_delete_rebalance_left (ptr_type X, bool is_bh_equal) {
    bool c_red = is_red (X);
    ptr_type L = X.data_unsafe ()->left;
    ptr_type R = X.data_unsafe ()->right;
    if (!c_red) {
      /* X is black */
      if (is_bh_equal) {
        /* set_black (X); */
        return {X, true};
      } else {
        if (is_red (L)) {
	  /* L is red */
          set_black (L);
          /* set_black (X); */
          return {X, true};
        } else {
	  /* L is black */
          bool r_red = is_red (R);
          ptr_type RL = R.data_unsafe ()->left;
          if (!r_red) {
	    /* R is black */
            if (is_black (RL)) {
	      /* RL is black or nil */
              set_right (X, RL);
              set_red (X);
              /* set_black (R); */
              set_left_no_check (R, X);
              return {R, false};
            } else {
	      /* RL is red */
              ptr_type RLL = RL.data_unsafe ()->left;
              ptr_type RLR = RL.data_unsafe ()->right;
              set_right (X, RLL);
              set_left (R, RLR);
              /* set_black (X); */
              /* set_black (R); */
              set_left_no_check (RL, X);
              set_right_no_check_no_flag (RL, R);
              /* set_red (RL); */
              return {RL, false};
            }
          } else {
	    /* R is red, hence RL is black */
            ptr_type RLL = RL.data_unsafe ()->left;
            ptr_type RLR = RL.data_unsafe ()->right;
            if (is_black (RLL)) {
	      /* RLL is black */
              set_right (X, RLL);
              set_red (X);
              /* set_black (RL); */
              set_left_no_check (RL, X);
              /* set_red (R); */
              return {R, false};
            } else {
	      /* RLL is red */
              ptr_type RLLL = RLL.data_unsafe ()->left;
              ptr_type RLLR = RLL.data_unsafe ()->right;
              set_right (X, RLLL);
              set_left (RL, RLLR);
              /* set_black (X); */
              /* set_black (RL); */
              set_left_no_check (RLL, X);
              set_right_no_check (RLL, RL);
              /* set_red (RLL); */
              set_left_no_check_no_flag (R, RLL);
              set_black (R);
              return {R, true};
            }
          }
        }
      }
    } else {
      /* X is red, hence R is black */
      if (is_bh_equal) {
        /* set_red (X); */
        return {X, true};
      } else {
        if (is_red (L)) {
	  /* L is red */
          set_black (L);
          /* set_red (X); */
          return {X, true};
        } else {
	  /* L is black */
          ptr_type RL = R.data_unsafe ()->left;
          if (is_black (RL)) {
	    /* RL is black */
            set_right (X, RL);
            /* set_red (X); */
            /* set_black (R); */
            set_left_no_check (R, X);
            return {R, true};
          } else {
	    /* RL is red */
            ptr_type RLL = RL.data_unsafe ()->left;
            ptr_type RLR = RL.data_unsafe ()->right;
            set_right (X, RLL);
            set_left (R, RLR);
            set_black (X);
            /* set_black (R); */
            set_left_no_check (RL, X);
            set_right_no_check_no_flag (RL, R);
            /* set_red (RL); */
            return {RL, true};
          }
        }
      }
    }
  }

  static constexpr minilib::pair < ptr_type, bool > do_rb_delete_rebalance_right (ptr_type X, bool is_bh_equal) {
    bool c_red = is_red (X);
    ptr_type L = X.data_unsafe ()->left;
    ptr_type R = X.data_unsafe ()->right;
    if (!c_red) {
      /* X is black */
      if (is_bh_equal) {
        /* set_black (X); */
        return {X, true};
      } else {
        if (is_red (R)) {
	  /* R is red */
          set_black (R);
          /* set_black (X); */
          return {X, true};
        } else {
	  /* R is black */
          bool l_red = is_red (L);
          ptr_type LR = L.data_unsafe ()->right;
          if (!l_red) {
	    /* L is black */
            if (is_black (LR)) {
	      /* LR is black */
              set_left (X, LR);
              set_red (X);
              /* set_black (L); */
              set_right_no_check (L, X);
              return {L, false};
            } else {
	      /* LR is red */
              ptr_type LRL = LR.data_unsafe ()->left;
              ptr_type LRR = LR.data_unsafe ()->right;
              set_right (L, LRL);
              set_left (X, LRR);
              /* set_black (L); */
              /* set_black (X); */
              set_left_no_check_no_flag (LR, L);
              set_right_no_check (LR, X);
              /* set_red (LR); */
              return {LR, false};
            }
          } else {
	    /* L is red, hence LR is black */
            ptr_type LRL = LR.data_unsafe ()->left;
            ptr_type LRR = LR.data_unsafe ()->right;
            if (is_black (LRR)) {
	      /* LRR is black */
              set_left (X, LRR);
              set_red (X);
              /* set_black (LR); */
              set_right_no_check (LR, X);
              /* set_red (L); */
              return {L, false};
            } else {
	      /* LRR is red */
              ptr_type LRRL = LRR.data_unsafe ()->left;
              ptr_type LRRR = LRR.data_unsafe ()->right;
              set_right (LR, LRRL);
              set_left (X, LRRR);
              /* set_black (LR); */
              /* set_black (X); */
              set_left_no_check (LRR, LR);
              set_right_no_check (LRR, X);
              /* set_red (LRR); */
              set_right_no_check_no_flag (L, LRR);
              set_black (L);
              return {L, true};
            }
          }
        }
      }
    } else {
      /* X is red, hence L is black */
      if (is_bh_equal) {
        /* set_red (X); */
        return {X, true};
      } else {
        if (is_red (R)) {
	  /* R is red */
          set_black (R);
          /* set_red (X); */
          return {X, true};
        } else {
	  /* R is black */
          ptr_type LR = L.data_unsafe ()->right;
          if (is_black (LR)) {
	    /* LR is black */
            set_left (X, LR);
            /* set_red (X); */
            /* set_black (L); */
            set_right_no_check (L, X);
            return {L, true};
          } else {
	    /* LR is red */
            ptr_type LRL = LR.data_unsafe ()->left;
            ptr_type LRR = LR.data_unsafe ()->right;
            set_right (L, LRL);
            set_left (X, LRR);
            /* set_black (L); */
            /* set_black (X); */
            set_left_no_check_no_flag (LR, L);
            set_right_no_check (LR, X);
            /* set_red (LR); */
            return {LR, true};
          }
        }
      }
    }
  }

  constexpr void insert_rebalance (ptr_type X, bool is_left) {
    ptr_type curr = X;
    bool is_bh_equal = true;
    while (curr) {
      ptr_type parent = curr.data_unsafe ()->parent;
      bool was_left = (curr.data_unsafe ()->flags & RB_IS_LEFT_CHILD) != 0;
      bool was_red = curr.data_unsafe ()->flags & RB_IS_RED;
      minilib::pair < ptr_type, bool > res;
      if (is_left) {
        res = do_rb_insert_rebalance_left (curr, is_bh_equal);
      } else {
        res = do_rb_insert_rebalance_right (curr, is_bh_equal);
      }
      if (parent) {
        if (was_left) set_left_no_check (parent, res.first);
        else set_right_no_check (parent, res.first);
      } else {
        set_root_no_check (res.first);
      }
      /* If neither color nor black-height of X changed after rebalancing, exit early */
      if (res.second && was_red == static_cast < bool > (res.first.data_unsafe ()->flags & RB_IS_RED)) return;
      curr = parent;
      is_left = was_left;
      is_bh_equal = res.second;
    }
  }

  constexpr void remove_node (ptr_type X) {
    ptr_type parent_of_X = X.data_unsafe ()->parent;
    bool was_X_left = (X.data_unsafe ()->flags & RB_IS_LEFT_CHILD) != 0;

    ptr_type new_sub_root;
    bool final_b;

    if (!X.data_unsafe ()->right) {
      new_sub_root = X.data_unsafe ()->left;
      final_b = is_red (X);
      minilib::destroy_at < node_type > (X.data_unsafe ());
      ptr_type::deallocate (X);
      if (parent_of_X) {
	if (was_X_left) set_left (parent_of_X, new_sub_root);
	else set_right (parent_of_X, new_sub_root);
      } else {
	set_root (new_sub_root);
	return;
      }
    } else {
      ptr_type R = X.data_unsafe ()->right;
      if (!R.data_unsafe ()->left) {
        bool b = is_red (R);
        set_left (R, X.data_unsafe ()->left);
        set_color (R, is_red (X));
        minilib::destroy_at < node_type > (X.data_unsafe ());
        ptr_type::deallocate (X);
        auto res = do_rb_delete_rebalance_right (R, b);
        new_sub_root = res.first;
        final_b = res.second;
        if (parent_of_X) {
          if (was_X_left) set_left_no_check (parent_of_X, new_sub_root);
          else set_right_no_check (parent_of_X, new_sub_root);
        } else {
          set_root_no_check (new_sub_root);
        }
      } else {
        ptr_type curr = R;
        while (curr.data_unsafe ()->left) {
          curr = curr.data_unsafe ()->left;
        }
        ptr_type Y = curr;
        ptr_type P_k = Y.data_unsafe ()->parent;
        bool b = is_red (Y);
        set_left (P_k, Y.data_unsafe ()->right);

        ptr_type p = P_k;
        ptr_type new_R;
        bool b_0;
        while (true) {
          bool is_at_R = same_ptr (p, R);
	  bool was_red = p.data_unsafe ()->flags & RB_IS_RED;
          ptr_type parent_of_p = p.data_unsafe ()->parent;
          auto res = do_rb_delete_rebalance_left (p, b);
          if (!is_at_R) {
            set_left_no_check (parent_of_p, res.first);
            p = parent_of_p;
            b = res.second;
	    if (res.second && was_red == static_cast < bool > (res.first.data_unsafe ()->flags & RB_IS_RED)) {
	      new_R = R;
	      b_0 = true;
	      break;
	    }
          } else {
            new_R = res.first;
            b_0 = res.second;
            break;
          }
        }

        set_left (Y, X.data_unsafe ()->left);
        set_right_no_check (Y, new_R);
        set_color (Y, is_red (X));
        minilib::destroy_at < node_type > (X.data_unsafe ());
        ptr_type::deallocate (X);

        auto res = do_rb_delete_rebalance_right (Y, b_0);
        new_sub_root = res.first;
        final_b = res.second;
        if (parent_of_X) {
          if (was_X_left) set_left_no_check (parent_of_X, new_sub_root);
          else set_right_no_check (parent_of_X, new_sub_root);
        } else {
          set_root_no_check (new_sub_root);
        }
      }
    }

    ptr_type curr = parent_of_X;
    bool is_left = was_X_left;
    bool is_bh_equal = final_b;

    while (curr) {
      ptr_type parent = curr.data_unsafe ()->parent;
      bool was_left = (curr.data_unsafe ()->flags & RB_IS_LEFT_CHILD) != 0;
      bool was_red = curr.data_unsafe ()->flags & RB_IS_RED;
      minilib::pair < ptr_type, bool > res;
      if (is_left) {
        res = do_rb_delete_rebalance_left (curr, is_bh_equal);
      } else {
        res = do_rb_delete_rebalance_right (curr, is_bh_equal);
      }
      if (parent) {
        if (was_left) set_left_no_check (parent, res.first);
        else set_right_no_check (parent, res.first);
      } else {
        set_root_no_check (res.first);
      }
      if (res.second && was_red == static_cast < bool > (res.first.data_unsafe ()->flags & RB_IS_RED)) return;
      curr = parent;
      is_left = was_left;
      is_bh_equal = res.second;
    }
  }

  constexpr void clear_subtree (ptr_type sub) requires (Destructor != nullptr || minilib::is_destructible_v < T >) {
    if (!sub) return;
    ptr_type curr = sub;
    while (true) {
      while (true) {
	ptr_type l = curr.data_unsafe ()->left;
        if (l) { curr = l; continue; }
        ptr_type r = curr.data_unsafe ()->right;
        if (r) { curr = r; continue; }
        break;
      }
      if constexpr (Destructor != nullptr) {
	Destructor (curr.data_unsafe ()->storage.data ());
      } else {
	minilib::destroy_at < T > (curr.data_unsafe ()->storage.data ());
      }
      if (same_ptr (curr, sub)) {
        minilib::destroy_at < node_type > (curr.data_unsafe ());
        ptr_type::deallocate (curr);
        break;
      }
      ptr_type p = curr.data_unsafe ()->parent;
      if ((curr.data_unsafe ()->flags & RB_IS_LEFT_CHILD) != 0) {
        p.data_unsafe ()->left = nullptr;
      } else {
        p.data_unsafe ()->right = nullptr;
      }
      minilib::destroy_at < node_type > (curr.data_unsafe ());
      ptr_type::deallocate (curr);
      curr = p;
    }
  }

  constexpr void copy_tree_from (const rbtree& other) requires (minilib::is_copy_constructible_v < T >) {
    auto other_runtime = other.get_runtime ();
    if (!other_runtime->root) {
      set_root (ptr_type ());
      return;
    }
    ptr_type src_root = other_runtime->root;
    ptr_type dst_root = allocate_node ();
    dst_root.data_unsafe ()->flags = src_root.data_unsafe ()->flags;
    minilib::construct_at < T > (dst_root.data_unsafe ()->storage.data (), *(src_root.data_unsafe ()->storage.data ()));
    set_root (dst_root);

    ptr_type src = src_root;
    ptr_type dst = dst_root;

    while (true) {
      ptr_type l = src.data_unsafe ()->left;
      if (l) {
        ptr_type new_dst = allocate_node ();
        new_dst.data_unsafe ()->flags = l.data_unsafe ()->flags;
        minilib::construct_at < T > (new_dst.data_unsafe ()->storage.data (), *(l.data_unsafe ()->storage.data ()));
        set_left (dst, new_dst);
        src = l;
        dst = new_dst;
        continue;
      }
      ptr_type r = src.data_unsafe ()->right;
      if (r) {
        ptr_type new_dst = allocate_node ();
        new_dst.data_unsafe ()->flags = r.data_unsafe ()->flags;
        minilib::construct_at < T > (new_dst.data_unsafe ()->storage.data (), *(r.data_unsafe ()->storage.data ()));
        set_right (dst, new_dst);
        src = r;
        dst = new_dst;
        continue;
      }

      ptr_type child_src = src;
      ptr_type child_dst = dst;
      bool found_next = false;
      while (child_src.data_unsafe ()->parent) {
        ptr_type p_src = child_src.data_unsafe ()->parent;
        ptr_type p_dst = child_dst.data_unsafe ()->parent;
        if (((child_src.data_unsafe ()->flags & RB_IS_LEFT_CHILD) != 0) && static_cast < bool > (p_src.data_unsafe ()->right)) {
          ptr_type new_dst = allocate_node ();
          new_dst.data_unsafe ()->flags = p_src.data_unsafe ()->right.data_unsafe ()->flags;
          minilib::construct_at < T > (new_dst.data_unsafe ()->storage.data (), *(p_src.data_unsafe ()->right.data_unsafe ()->storage.data ()));
          set_right (p_dst, new_dst);
          src = p_src.data_unsafe ()->right;
          dst = new_dst;
          found_next = true;
          break;
        }
        child_src = p_src;
        child_dst = p_dst;
      }
      if (!found_next) {
        break;
      }
    }
  }

  constexpr ptr_type emplace_root_raw () {
    auto runtime = get_runtime ();
    if (runtime->root) std::terminate ();
    auto p = allocate_node ();
    set_root (p);
    return p;
  }

  constexpr ptr_type emplace_left_raw (const handle_type& h) {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    ptr_type X = ptr_type::from_counted_ref (h);
    if (X.data_unsafe ()->left) std::terminate ();
    auto L = allocate_node ();
    set_left (X, L);
    set_red (L);
    insert_rebalance (X, true);
    return L;
  }

  constexpr ptr_type emplace_right_raw (const handle_type& h) {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    ptr_type X = ptr_type::from_counted_ref (h);
    if (X.data_unsafe ()->right) std::terminate ();
    auto R = allocate_node ();
    set_right (X, R);
    set_red (R);
    insert_rebalance (X, false);
    return R;
  }

public:
  constexpr rbtree () {
    if consteval {
      constexpr_ = minilib::allocator < minilib::detail::rbtree_constexpr < T > > :: allocate (1);
      minilib::construct_at < minilib::detail::rbtree_constexpr < T > > (constexpr_);
      constexpr_->runtime.root = nullptr;
      constexpr_->runtime.container_id = 0;
      constexpr_->global_counter = nullptr;
    } else {
      minilib::construct_at < minilib::detail::rbtree_runtime < T > > (minilib::addressof (runtime_));
      runtime_.root = nullptr;
      runtime_.container_id = get_tls_counter ();
    }
  }

  /* This function should only be called under constexpr, should only be called once, and immediately after default initialization. */
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

  constexpr rbtree (const rbtree& other) requires (minilib::is_copy_constructible_v < T >) {
    if consteval {
      constexpr_ = minilib::allocator < minilib::detail::rbtree_constexpr < T > > :: allocate (1);
      minilib::construct_at < minilib::detail::rbtree_constexpr < T > > (constexpr_);
      constexpr_->runtime.root = nullptr;
      constexpr_->global_counter = other.constexpr_->global_counter;
      if (constexpr_->global_counter != nullptr) {
        constexpr_->runtime.container_id = constexpr_->global_counter->get ();
      } else {
        constexpr_->runtime.container_id = 0;
      }
    } else {
      minilib::construct_at < minilib::detail::rbtree_runtime < T > > (minilib::addressof (runtime_));
      runtime_.root = nullptr;
      runtime_.container_id = get_tls_counter ();
    }
    copy_tree_from (other);
  }

  constexpr rbtree (rbtree&& other) {
    if consteval {
      constexpr_ = minilib::allocator < minilib::detail::rbtree_constexpr < T > > :: allocate (1);
      minilib::construct_at < minilib::detail::rbtree_constexpr < T > > (constexpr_);
      constexpr_->runtime = other.constexpr_->runtime;
      constexpr_->global_counter = other.constexpr_->global_counter;
      constexpr_->local_counter = other.constexpr_->local_counter;
      other.constexpr_->runtime.root = nullptr;
      other.constexpr_->local_counter = minilib::counter ();
      if (constexpr_->global_counter != nullptr) {
        other.constexpr_->runtime.container_id = constexpr_->global_counter->get ();
      } else {
        other.constexpr_->runtime.container_id = 0;
      }
    } else {
      minilib::construct_at < minilib::detail::rbtree_runtime < T > > (minilib::addressof (runtime_));
      runtime_ = other.runtime_;
      other.runtime_.root = nullptr;
      other.runtime_.container_id = get_tls_counter ();
    }
  }

private:
  /* Three helper functions for implementing assignment operator with storage reuse */
  static constexpr void assign_node_data (ptr_type src, ptr_type dst) requires (minilib::is_copy_constructible_v < T >) {
    dst.data_unsafe ()->flags = src.data_unsafe ()->flags;
    T * src_tp = src.data_unsafe ()->storage.data ();
    T * dst_tp = dst.data_unsafe ()->storage.data ();
    if constexpr (minilib::is_copy_assignable_v < T >) {
      *dst_tp = *src_tp;
    } else if constexpr (Destructor != nullptr || minilib::is_destructible_v < T >) {
      if constexpr (Destructor != nullptr) {
	Destructor (dst_tp);
      } else {
	minilib::destroy_at < T > (dst_tp);
      }
      minilib::construct_at < T > (dst_tp, *src_tp);
    } else {
      std::terminate ();
    }
  }

  /* Detach a node from parent when parent is known to be not nullptr */
  static constexpr void detach (ptr_type p) {
    ptr_type parent = p.data_unsafe ()->parent;
    if (p.data_unsafe ()->flags & RB_IS_LEFT_CHILD) {
      set_left (parent, nullptr);
    } else {
      set_right (parent, nullptr);
    }
    p.data_unsafe ()->parent = nullptr;
  }

  /* Find leaf in subtree, assuming p is not nullptr, left child of p is nullptr */
  static constexpr ptr_type find_leaf (ptr_type p) {
    ptr_type c = p.data_unsafe ()->right;
    if (c) {
      p = c;
      while (true) {
	ptr_type l = p.data_unsafe ()->left;
	if (l) { p = l; continue; }
	ptr_type r = p.data_unsafe ()->right;
	if (r) { p = r; continue; }
	return p;
      }
    } else {
      return p;
    }
  }

public:
  constexpr rbtree& operator= (const rbtree& other) requires (minilib::is_copy_constructible_v < T >) {
    if (this == minilib::addressof (other)) return *this;

    auto other_runtime = other.get_runtime ();
    auto runtime = get_runtime ();

    if (!other_runtime->root) {
      clear ();
      return *this;
    }

    if (!runtime->root) {
      copy_tree_from (other);
      return *this;
    }

    /* We want to reuse storage where possible. Therefore, as long as the old tree has at least one node remaining, we take one leaf node at a time. */
    ptr_type old_root = runtime->root;
    ptr_type leaf = old_root;
    while (true) {
      ptr_type l = leaf.data_unsafe ()->left;
      if (l) { leaf = l; continue; }
      ptr_type r = leaf.data_unsafe ()->right;
      if (r) { leaf = r; continue; }
      break;
    }
    /* Hence we maintain the invariant that leaf is either nullptr or a leaf. If it is a right child, then its parent does not have left child */

    ptr_type src = other_runtime->root;
    ptr_type dst = leaf;
    ptr_type p = leaf.data_unsafe ()->parent;
    if (p) {
      detach (leaf);
      assign_node_data (src, dst);
      leaf = find_leaf (p);
    } else {
      assign_node_data (src, dst);
      leaf = nullptr;
    }
    runtime->root = dst;

    while (true) {
      ptr_type l = src.data_unsafe ()->left;
      if (l) {
	if (leaf) {
	  ptr_type p = leaf.data_unsafe ()->parent;
	  if (p) {
	    detach (leaf);
	    assign_node_data (l, leaf);
	    set_left (dst, leaf);
	    src = l;
	    dst = leaf;
	    leaf = find_leaf (p);
	  } else {
	    assign_node_data (l, leaf);
	    set_left (dst, leaf);
	    src = l;
	    dst = leaf;
	    leaf = nullptr;
	  }
	} else {
	  ptr_type new_dst = allocate_node ();
	  new_dst.data_unsafe ()->flags = l.data_unsafe ()->flags;
	  minilib::construct_at < T > (new_dst.data_unsafe ()->storage.data (), *(l.data_unsafe ()->storage.data ()));
	  set_left (dst, new_dst);
	  src = l;
	  dst = new_dst;
	}
	continue;
      }
      ptr_type r = src.data_unsafe ()->right;
      if (r) {
	if (leaf) {
	  ptr_type p = leaf.data_unsafe ()->parent;
	  if (p) {
	    detach (leaf);
	    assign_node_data (r, leaf);
	    set_right (dst, leaf);
	    src = r;
	    dst = leaf;
	    leaf = find_leaf (p);
	  } else {
	    assign_node_data (r, leaf);
	    set_right (dst, leaf);
	    src = r;
	    dst = leaf;
	    leaf = nullptr;
	  }
	} else {
	  ptr_type new_dst = allocate_node ();
	  new_dst.data_unsafe ()->flags = r.data_unsafe ()->flags;
	  minilib::construct_at < T > (new_dst.data_unsafe ()->storage.data (), *(r.data_unsafe ()->storage.data ()));
	  set_right (dst, new_dst);
	  src = r;
	  dst = new_dst;
	}
	continue;
      }

      ptr_type child_src = src;
      ptr_type child_dst = dst;
      bool found_next = false;
      while (child_src.data_unsafe ()->parent) {
        ptr_type p_src = child_src.data_unsafe ()->parent;
        ptr_type p_dst = child_dst.data_unsafe ()->parent;
        if (((child_src.data_unsafe ()->flags & RB_IS_LEFT_CHILD) != 0) && static_cast < bool > (p_src.data_unsafe ()->right)) {
	  if (leaf) {
	    ptr_type p = leaf.data_unsafe ()->parent;
	    if (p) {
	      detach (leaf);
	      assign_node_data (p_src.data_unsafe ()->right, leaf);
	      set_right (p_dst, leaf);
	      src = p_src.data_unsafe ()->right;
	      dst = leaf;
	      leaf = find_leaf (p);
	    } else {
	      assign_node_data (p_src.data_unsafe ()->right, leaf);
	      set_right (p_dst, leaf);
	      src = p_src.data_unsafe ()->right;
	      dst = leaf;
	      leaf = nullptr;
	    }
	  } else {
	    ptr_type new_dst = allocate_node ();
	    new_dst.data_unsafe ()->flags = p_src.data_unsafe ()->right.data_unsafe ()->flags;
	    minilib::construct_at < T > (new_dst.data_unsafe ()->storage.data (), *(p_src.data_unsafe ()->right.data_unsafe ()->storage.data ()));
	    set_right (p_dst, new_dst);
	    src = p_src.data_unsafe ()->right;
	    dst = new_dst;
	  }
	  found_next = true;
	  break;
        }
        child_src = p_src;
        child_dst = p_dst;
      }
      if (!found_next) {
        break;
      }
    }

    /* If leaf is still not nullptr, clear remaining nodes */
    if (leaf) {
      clear_subtree (old_root);
    }

    return *this;
  }

  constexpr rbtree& operator= (rbtree&& other) {
    if (this == minilib::addressof (other)) return *this;
    clear ();
    if consteval {
      constexpr_->runtime = other.constexpr_->runtime;
      constexpr_->global_counter = other.constexpr_->global_counter;
      constexpr_->local_counter = other.constexpr_->local_counter;
      other.constexpr_->runtime.root = nullptr;
      other.constexpr_->local_counter = minilib::counter ();
      if (constexpr_->global_counter != nullptr) {
        other.constexpr_->runtime.container_id = constexpr_->global_counter->get ();
      } else {
        other.constexpr_->runtime.container_id = 0;
      }
    } else {
      runtime_ = other.runtime_;
      other.runtime_.root = nullptr;
      other.runtime_.container_id = get_tls_counter ();
    }
    return *this;
  }

  constexpr ~rbtree () {
    clear ();
    if consteval {
      minilib::destroy_at < minilib::detail::rbtree_constexpr < T > > (constexpr_);
      minilib::allocator < minilib::detail::rbtree_constexpr < T > > :: deallocate (constexpr_, 1);
    }
  }

  constexpr void clear () {
    auto runtime = get_runtime ();
    if constexpr (minilib::is_destructible_v < T >) {
      if (runtime->root) {
        clear_subtree (runtime->root);
        runtime->root = nullptr;
      }
    } else {
      if (runtime->root) {
        std::terminate ();
      }
    }
  }

  constexpr handle_type emplace_root_null () {
    ptr_type p = emplace_root_raw ();
    handle_type h;
    p.create_counted_ref (h);
    return h;
  }

  constexpr handle_type emplace_root_default () requires (minilib::is_truly_default_constructible_v < T >) {
    ptr_type p = emplace_root_raw ();
    handle_type h;
    p.create_counted_ref (h);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::default_construct_at < T > (tp);
    return h;
  }

  template < typename... Args >
  requires (minilib::is_constructible_v < T, Args&&... >)
  constexpr handle_type emplace_root (Args&&... args) {
    ptr_type p = emplace_root_raw ();
    handle_type h;
    p.create_counted_ref (h);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, minilib::forward < Args > (args)...);
    return h;
  }

  constexpr handle_type emplace_left_null (const handle_type& h) {
    ptr_type p = emplace_left_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    return nh;
  }

  constexpr handle_type emplace_left_default (const handle_type& h) requires (minilib::is_truly_default_constructible_v < T >) {
    ptr_type p = emplace_left_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::default_construct_at < T > (tp);
    return nh;
  }

  template < typename... Args >
  requires (minilib::is_constructible_v < T, Args&&... >)
  constexpr handle_type emplace_left (const handle_type& h, Args&&... args) {
    ptr_type p = emplace_left_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, minilib::forward < Args > (args)...);
    return nh;
  }

  constexpr handle_type emplace_right_null (const handle_type& h) {
    ptr_type p = emplace_right_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    return nh;
  }

  constexpr handle_type emplace_right_default (const handle_type& h) requires (minilib::is_truly_default_constructible_v < T >) {
    ptr_type p = emplace_right_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::default_construct_at < T > (tp);
    return nh;
  }

  template < typename... Args >
  requires (minilib::is_constructible_v < T, Args&&... >)
  constexpr handle_type emplace_right (const handle_type& h, Args&&... args) {
    ptr_type p = emplace_right_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, minilib::forward < Args > (args)...);
    return nh;
  }

  constexpr void remove (const handle_type& h) requires (Destructor != nullptr || minilib::is_destructible_v < T >) {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    auto p = ptr_type::from_counted_ref (h);
    T * tp = p.data_unsafe ()->storage.data ();
    if constexpr (Destructor != nullptr) {
      Destructor (tp);
    } else {
      minilib::destroy_at < T > (tp);
    }
    remove_node (p);
  }

  constexpr void inform_destruct (const handle_type& h) {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    auto p = ptr_type::from_counted_ref (h);
    remove_node (p);
  }

  constexpr handle_type root () const {
    auto runtime = get_runtime ();
    handle_type h;
    if (runtime->root) {
      runtime->root.create_counted_ref (h);
    }
    return h;
  }

  constexpr handle_type left (const handle_type& h) const {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    handle_type res;
    auto p = ptr_type::from_counted_ref (h).data_unsafe ()->left;
    if (p) {
      p.create_counted_ref (res);
    }
    return res;
  }

  constexpr handle_type left_unsafe (const handle_type& h) const {
    handle_type res;
    auto p = ptr_type::from_counted_ref (h).data_unsafe ()->left;
    if (p) {
      p.create_counted_ref (res);
    }
    return res;
  }

  constexpr handle_type right (const handle_type& h) const {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    handle_type res;
    auto p = ptr_type::from_counted_ref (h).data_unsafe ()->right;
    if (p) {
      p.create_counted_ref (res);
    }
    return res;
  }

  constexpr handle_type right_unsafe (const handle_type& h) const {
    handle_type res;
    auto p = ptr_type::from_counted_ref (h).data_unsafe ()->right;
    if (p) {
      p.create_counted_ref (res);
    }
    return res;
  }

  constexpr handle_type parent (const handle_type& h) const {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    handle_type res;
    auto p = ptr_type::from_counted_ref (h).data_unsafe ()->parent;
    if (p) {
      p.create_counted_ref (res);
    }
    return res;
  }

  constexpr handle_type parent_unsafe (const handle_type& h) const {
    handle_type res;
    auto p = ptr_type::from_counted_ref (h).data_unsafe ()->parent;
    if (p) {
      p.create_counted_ref (res);
    }
    return res;
  }

  constexpr T * data (const handle_type& h) {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    return ptr_type::from_counted_ref (h).data_unsafe ()->storage.data ();
  }

  constexpr const T * data (const handle_type& h) const {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    return ptr_type::from_counted_ref (h).data_unsafe ()->storage.data ();
  }

  constexpr T * data_unsafe (const handle_type& h) {
    return ptr_type::from_counted_ref (h).data_unsafe ()->storage.data ();
  }

  constexpr const T * data_unsafe (const handle_type& h) const {
    return ptr_type::from_counted_ref (h).data_unsafe ()->storage.data ();
  }

  /* Check handle validity */
  constexpr bool check (const handle_type& h) const {
    auto runtime = get_runtime ();
    return h.check (runtime->container_id);
  }

  constexpr uint32_t check_maybe_null (const handle_type& h) const {
    auto runtime = get_runtime ();
    return h.check_maybe_null (runtime->container_id);
  }
};

}

#endif
