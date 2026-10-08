/* Red-black tree

   Formal specification:

   Inductive rb_color :=
   | black
   | red.

   Inductive rb_tree :=
   | nil
   | node (x : val_t) (l : rb_tree) (r : rb_tree) (c : rb_color).

   (* rb_insert_rebalance_* are guaranteed to rebalanced correctly as long as the old and new subtrees are related by rb_insert_rel.
      Furthermore, the rebalanced tree is related to the old tree by rb_insert_rel. *)

   Definition rb_insert_rel (tree tree' : rb_tree) :=
     rb_tree_bh tree' = rb_tree_bh tree \/
     rb_tree_bh tree' = S (rb_tree_bh tree) /\
     rb_tree_color tree' = black /\
     match tree' with
     | node _ l r _ => rb_tree_color l = red -> rb_tree_color r = red -> rb_tree_color tree = red
     | _ => True
     end.

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
                           match rb_tree_color r with
                           | black => (node l'_r_x (node l'_x l'_l l'_r_l red) (node x l'_r_r r red) black, true)
                           | red =>
                               match r with
                               | nil => (* Impossible *) (nil, true)
                               | node r_x r_l r_r r_c =>
                                   (node x l' (node r_x r_l r_r black) red, true)
                               end
                           end
                       end
                   end
               | red =>
                   match rb_tree_color l'_r with
                   | black =>
                       match rb_tree_color r with
                       | black => (node l'_x l'_l (node x l'_r r red) black, true)
                       | red =>
                           match r with
                           | nil => (* Impossible *) (nil, true)
                           | node r_x r_l r_r r_c =>
                               (node x l' (node r_x r_l r_r black) red, true)
                           end
                       end
                   | red =>
                       match l'_l with
                       | nil => (nil, true)
                       | node l'_l_x l'_l_l l'_l_r _ => (node l'_x (node l'_l_x l'_l_l l'_l_r black) (node x l'_r r black) red, true)
                       end
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
                           match rb_tree_color l with
                           | black => (node r'_x (node x l r'_l red) r'_r black, true)
                           | red =>
                               match l with
                               | nil => (* Impossible *) (nil, true)
                               | node l_x l_l l_r l_c =>
                                   (node x (node l_x l_l l_r black) r' red, true)
                               end
                           end
                       end
                   end
               | red =>
                   match rb_tree_color r'_r with
                   | black =>
                       match r'_l with
                       | nil => (* Impossible *) (nil, true)
                       | node r'_l_x r'_l_l r'_l_r r'_l_c =>
                           match rb_tree_color l with
                           | black => (node r'_l_x (node x l r'_l_l red) (node r'_x r'_l_r r'_r red) black, true)
                           | red =>
                               match l with
                               | nil => (* Impossible *) (nil, true)
                               | node l_x l_l l_r l_c =>
                                   (node x (node l_x l_l l_r black) r' red, true)
                               end
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
                       match rb_tree_color r_l with
                       | black =>
                           match rb_tree_color r_r with
                           | black => (node x l' (node r_x r_l r_r red) black, false)
                           | red =>
                               match r_r with
                               | nil => (* Impossible *) (nil, true)
                               | node r_r_x r_r_l r_r_r _ =>
                                   (node r_x (node x l' r_l black) (node r_r_x r_r_l r_r_r black) black, true)
                               end
                           end
                       | red =>
                           match r_l with
                           | nil => (* Impossible *) (nil, true)
                           | node r_l_x r_l_l r_l_r r_l_c =>
                               (node r_l_x (node x l' r_l_l black) (node r_x r_l_r r_r black) black, true)
                           end
                       end
                   | red =>
                       match r_l with
                       | nil => (* Impossible *) (nil, true)
                       | node r_l_x r_l_l r_l_r r_l_c =>
                           match rb_tree_color r_l_l with
                           | black =>
                               (node r_x (node r_l_x (node x l' r_l_l red) r_l_r black) r_r black, true)
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
                       match rb_tree_color l_r with
                       | black =>
                           match rb_tree_color l_l with
                           | black => (node x (node l_x l_l l_r red) r' black, false)
                           | red =>
                               match l_l with
                               | nil => (* Impossible *) (nil, true)
                               | node l_l_x l_l_l l_l_r _ =>
                                   (node l_x (node l_l_x l_l_l l_l_r black) (node x l_r r' black) black, true)
                               end
                           end
                       | red =>
                           match l_r with
                           | nil => (* Impossible *) (nil, true)
                           | node l_r_x l_r_l l_r_r l_r_c =>
                               (node l_r_x (node l_x l_l l_r_l black) (node x l_r_r r' black) black, true)
                           end
                       end
                   | red =>
                       match l_r with
                       | nil => (* Impossible *) (nil, true)
                       | node l_r_x l_r_l l_r_r l_r_c =>
                           match rb_tree_color l_r_r with
                           | black =>
                               (node l_x l_l (node l_r_x l_r_l (node x l_r_r r' red) black) black, true)
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

  static constexpr bool is_red_no_check (ptr_type p) {
    return (p.data_unsafe ()->flags & RB_IS_RED) != 0;
  }

  static constexpr bool is_black (ptr_type p) {
    return !static_cast < bool > (p) || ((p.data_unsafe ()->flags & RB_IS_RED) == 0);
  }

  static constexpr void set_red (ptr_type p) {
    p.data_unsafe ()->flags |= RB_IS_RED;
  }

  static constexpr void set_black (ptr_type p) {
    p.data_unsafe ()->flags &= ~static_cast < size_t > (RB_IS_RED);
  }

  static constexpr void set_color (ptr_type p, bool red) {
    p.data_unsafe ()->flags = (p.data_unsafe ()->flags & ~static_cast < size_t > (RB_IS_RED)) | (static_cast < size_t > (red) << 1);
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

  constexpr ptr_type allocate_node (size_t flags) {
    auto runtime = get_runtime ();
    auto p = ptr_type::allocate (runtime->container_id, get_local_counter ());
    minilib::construct_at < node_type > (p.data_unsafe ());
    p.data_unsafe ()->flags = flags;
    return p;
  }

  /* Insert rebalancing functions.
     This function is modeled after rb_insert_rebalance_* in the Rocq spec.
     However, we make the following pruning:
     * We let the function return a uint8_t bitfield.
       Bit 0 indicates the returned is_bh_equal value to be used in the next iteration.
       Bit 1 indicates whether we can stop early.
       The logic is that if we return is_bh_equal == true, and either (1) root of old subtree is red,
       or (2) root of new subtree is black, then we can stop.
     * We first observe that rb_insert_rebalance_* returns is_bh_equal == false only when X is red.
       If X is red then its parent must be black. Therefore, when this function is called with is_bh_equal == false, we assume X is black.
       Conversely, if X is red we assume is_bh_equal == true.
       The branches with red X and is_bh_equal == false are pruned.
     * It follows that rb_insert_rebalance_* returns is_bh_equal == false only if X is red and is_bh_equal == true.
       In this case, the unmodified subtree (l or r) must have black root.
       Suppose the modified subtree (l' or r') has black root, then we should have stopped early at the previous iteration.
       Therefore we may assume the modified subtree (l' or r') must have red root.
       Hence: if we call this function with is_bh_equal == false and black X,
       then we shall assume exactly one grandchild of X in the modified subtree is red.
       The cases where both grandchildren are red or both are black are pruned.
     * Now notice that in the pruned code, all branches that perform rotation return 0x01 | 0x02.
       This justifies the claim that red-black tree insertion requires at most 2 rotations.

     We also try to avoid writing flags where possible.
   */
  static constexpr minilib::pair < ptr_type, uint8_t > do_rb_insert_rebalance_left (ptr_type X, bool is_bh_equal) {
    bool c_red = is_red_no_check (X);
    if (c_red) {
      /* Assume is_bh_equal == true. Just mark X black. */
      set_black (X);
      return {X, 0x00};
    }

    /* Otherwise, X is black */
    if (is_bh_equal) return {X, 0x01 | 0x02};

    /* Now L is assumed to be black and not nil. */

    /* Check if uncle is red */
    ptr_type R = X.data_unsafe ()->right;
    bool uncle_red = is_red (R);
    if (uncle_red) {
      set_black (R);
      set_red (X);
      return {X, 0x01};
    }

    /* Now exactly one grandchild of X in the modified subtree is red, and we check which one */
    ptr_type L = X.data_unsafe ()->left;
    ptr_type LL = L.data_unsafe ()->left;
    ptr_type LR = L.data_unsafe ()->right;
    bool ll_red = is_red (LL);
    if (ll_red) {

      /* (node l'_x l'_l (node x l'_r r red) black, true) */
      set_left (X, LR);
      set_right_no_check (L, X);
      set_red (X);
      return {L, 0x01 | 0x02};

    } else {

      /* (node l'_r_x (node l'_x l'_l l'_r_l red) (node x l'_r_r r red) black, true) */
      ptr_type LRL = LR.data_unsafe ()->left;
      ptr_type LRR = LR.data_unsafe ()->right;
      set_left (X, LRR);
      set_right (L, LRL);
      set_left_no_check_no_flag (LR, L);
      set_right_no_check (LR, X);
      set_red (X);
      set_red (L);
      set_black (LR);
      return {LR, 0x01 | 0x02};

    }
  }

  static constexpr minilib::pair < ptr_type, uint8_t > do_rb_insert_rebalance_right (ptr_type X, bool is_bh_equal) {
    bool c_red = is_red_no_check (X);
    if (c_red) {
      set_black (X);
      return {X, 0x00};
    }

    if (is_bh_equal) return {X, 0x01 | 0x02};

    ptr_type L = X.data_unsafe ()->left;
    bool uncle_red = is_red (L);
    if (uncle_red) {
      set_black (L);
      set_red (X);
      return {X, 0x01};
    }

    ptr_type R = X.data_unsafe ()->right;
    ptr_type RL = R.data_unsafe ()->left;
    ptr_type RR = R.data_unsafe ()->right;
    bool rr_red = is_red (RR);
    if (rr_red) {

      /* (node r'_x (node x l r'_l red) r'_r black, true) */
      set_right (X, RL);
      set_left_no_check (R, X);
      set_red (X);
      return {R, 0x01 | 0x02};

    } else {

      /* (node r'_l_x (node x l r'_l_l red) (node r'_x r'_l_r r'_r red) black, true) */
      ptr_type RLL = RL.data_unsafe ()->left;
      ptr_type RLR = RL.data_unsafe ()->right;
      set_right (X, RLL);
      set_left (R, RLR);
      set_right_no_check_no_flag (RL, R);
      set_left_no_check (RL, X);
      set_red (X);
      set_red (R);
      set_black (RL);
      return {RL, 0x01 | 0x02};

    }
  }

  /* Rebalancing for deletion is more complicated.
     First we observe that almost every branch of rb_delete_rebalance_* can stop immediately.
     The only exception is when X is black, both children of X are black, and the sibling subtree has two black grandchildren.
     This case results in a subtree with black root and is_bh_equal == false, and needs propagation.
     Since this case involves only recoloring, and all other cases stop immediately, we see that red-black tree deletion involves at most 3 rotations.
     It follows we can simply let the caller handle the cases with is_bh_equal == true (which is trivial and only possible on the first iteration).
     Hence we shall always assume is_bh_equal == false.
     The returned bitfield contains only a single bit which is the is_bh_equal value.
     If it is 1, the caller should stop immediately, and only propagate if it is 0.
     Next we observe that, since the modified subtree can have a red root only on the first iteration, and the handling is trivial (just mark it black),
     we can let the caller handle that case. Hence we assume the modified subtree always has black root.
     With these there are still 7 cases to consider in each function.
   */
  static constexpr minilib::pair < ptr_type, uint8_t > do_rb_delete_rebalance_left (ptr_type X) {
    /* Assume is_bh_equal == false */
    /* Assume L is black */
    ptr_type L = X.data_unsafe ()->left;
    bool c_red = is_red_no_check (X);
    if (!c_red) {
      /* X is black */
      /* R cannot be nullptr since L has reduced black-height */
      ptr_type R = X.data_unsafe ()->right;
      bool r_red = is_red_no_check (R);
      ptr_type RL = R.data_unsafe ()->left;
      if (!r_red) {
	/* R is black */
	ptr_type RR = R.data_unsafe ()->right;
	if (is_black (RL)) {
	  /* RL is black */
	  if (is_black (RR)) {
	    /* RR is black */
	    /* (node x l' (node r_x r_l r_r red) black, false) */
	    set_red (R);
	    return {X, 0x00};
	  } else {
	    /* RR is red */
	    /* (node r_x (node x l' r_l black) (node r_r_x r_r_l r_r_r black) black, true) */
	    set_black (RR);
	    set_right (X, RL);
	    set_left_no_check (R, X);
	    return {R, 0x01};
	  }
	} else {
	  /* RL is red */
	  /* (node r_l_x (node x l' r_l_l black) (node r_x r_l_r r_r black) black, true) */
	  ptr_type RLL = RL.data_unsafe ()->left;
	  ptr_type RLR = RL.data_unsafe ()->right;
	  set_right (X, RLL);
	  set_left (R, RLR);
	  set_left_no_check (RL, X);
	  set_right_no_check_no_flag (RL, R);
	  set_black (RL);
	  return {RL, 0x01};
	}
      } else {
	/* R is red, hence RL is black */
	ptr_type RLL = RL.data_unsafe ()->left;
	ptr_type RLR = RL.data_unsafe ()->right;
	if (is_black (RLL)) {
	  /* RLL is black */
	  /* (node r_x (node r_l_x (node x l' r_l_l red) r_l_r black) r_r black, true) */
	  set_right (X, RLL);
	  set_left_no_check (RL, X);
	  set_red (X);
	  set_black (R);
	  return {R, 0x01};
	} else {
	  /* RLL is red */
	  /* (node r_x (node r_l_l_x (node x l' r_l_l_l black) (node r_l_x r_l_l_r r_l_r black) red) r_r black, true) */
	  ptr_type RLLL = RLL.data_unsafe ()->left;
	  ptr_type RLLR = RLL.data_unsafe ()->right;
	  set_right (X, RLLL);
	  set_left (RL, RLLR);
	  set_left_no_check (RLL, X);
	  set_right_no_check (RLL, RL);
	  set_left_no_check_no_flag (R, RLL);
	  set_black (R);
	  return {R, 0x01};
	}
      }
    } else {
      /* X is red, hence R is black */
      ptr_type R = X.data_unsafe ()->right;
      ptr_type RL = R.data_unsafe ()->left;
      if (is_black (RL)) {
	/* RL is black */
	/* (node r_x (node x l' r_l red) r_r black, true) */
	set_right (X, RL);
	set_left_no_check (R, X);
	return {R, 0x01};
      } else {
	/* RL is red */
	/* (node r_l_x (node x l' r_l_l black) (node r_x r_l_r r_r black) red, true) */
	ptr_type RLL = RL.data_unsafe ()->left;
	ptr_type RLR = RL.data_unsafe ()->right;
	set_right (X, RLL);
	set_left (R, RLR);
	set_right_no_check_no_flag (RL, R);
	set_left_no_check (RL, X);
	set_black (X);
	return {RL, 0x01};
      }
    }
  }

  static constexpr minilib::pair < ptr_type, uint8_t > do_rb_delete_rebalance_right (ptr_type X) {
    /* Assume R is black */
    ptr_type R = X.data_unsafe ()->right;
    bool c_red = is_red_no_check (X);
    if (!c_red) {
      /* X is black */
      ptr_type L = X.data_unsafe ()->left;
      bool l_red = is_red_no_check (L);
      ptr_type LR = L.data_unsafe ()->right;
      if (!l_red) {
	/* L is black */
	ptr_type LL = L.data_unsafe ()->left;
	if (is_black (LR)) {
	  /* LR is black */
	  if (is_black (LL)) {
	    /* LL is black */
	    /* (node x (node l_x l_l l_r red) r' black, false) */
	    set_red (L);
	    return {X, 0x00};
	  } else {
	    /* LL is red */
	    /* (node l_x (node l_l_x l_l_l l_l_r black) (node x l_r r' black) black, true) */
	    set_black (LL);
	    set_left (X, LR);
	    set_right_no_check (L, X);
	    return {L, 0x01};
	  }
	} else {
	  /* LR is red */
	  /* (node l_r_x (node l_x l_l l_r_l black) (node x l_r_r r' black) black, true) */
	  ptr_type LRL = LR.data_unsafe ()->left;
	  ptr_type LRR = LR.data_unsafe ()->right;
	  set_right (L, LRL);
	  set_left (X, LRR);
	  set_left_no_check_no_flag (LR, L);
	  set_right_no_check (LR, X);
	  set_black (LR);
	  return {LR, 0x01};
	}
      } else {
	/* L is red, hence LR is black */
	ptr_type LRL = LR.data_unsafe ()->left;
	ptr_type LRR = LR.data_unsafe ()->right;
	if (is_black (LRR)) {
	  /* LRR is black */
	  /* (node l_x l_l (node l_r_x l_r_l (node x l_r_r r' red) black) black, true) */
	  set_left (X, LRR);
	  set_right_no_check (LR, X);
	  set_red (X);
	  set_black (L);
	  return {L, 0x01};
	} else {
	  /* LRR is red */
	  /* (node l_x l_l (node l_r_r_x (node l_r_x l_r_l l_r_r_l black) (node x l_r_r_r r' black) red) black, true) */
	  ptr_type LRRL = LRR.data_unsafe ()->left;
	  ptr_type LRRR = LRR.data_unsafe ()->right;
	  set_right (LR, LRRL);
	  set_left (X, LRRR);
	  set_left_no_check (LRR, LR);
	  set_right_no_check (LRR, X);
	  set_right_no_check_no_flag (L, LRR);
	  set_black (L);
	  return {L, 0x01};
	}
      }
    } else {
      /* X is red, hence L is black */
      ptr_type L = X.data_unsafe ()->left;
      ptr_type LR = L.data_unsafe ()->right;
      if (is_black (LR)) {
	/* LR is black */
	/* (node l_x l_l (node x l_r r' red) black, true) */
	set_left (X, LR);
	set_right_no_check (L, X);
	return {L, 0x01};
      } else {
	/* LR is red */
	/* (node l_r_x (node l_x l_l l_r_l black) (node x l_r_r r' black) red, true) */
	ptr_type LRL = LR.data_unsafe ()->left;
	ptr_type LRR = LR.data_unsafe ()->right;
	set_right (L, LRL);
	set_left (X, LRR);
	set_left_no_check_no_flag (LR, L);
	set_right_no_check (LR, X);
	set_black (X);
	return {LR, 0x01};
      }
    }
  }

  constexpr void insert_rebalance (ptr_type X, bool is_left) {
    ptr_type curr = X;
    bool is_bh_equal = true;
    while (true) {
      ptr_type parent = curr.data_unsafe ()->parent;
      bool was_left = (curr.data_unsafe ()->flags & RB_IS_LEFT_CHILD) != 0;
      minilib::pair < ptr_type, uint8_t > res;
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
	return;
      }
      /* Exit early if the rebalance cannot introduce new violations */
      if (res.second & 0x02) return;
      curr = parent;
      is_left = was_left;
      is_bh_equal = res.second & 0x01;
    }
  }

  constexpr void remove_node (ptr_type X) {
    ptr_type parent_of_X = X.data_unsafe ()->parent;
    ptr_type left_of_X = X.data_unsafe ()->left;
    ptr_type right_of_X = X.data_unsafe ()->right;
    bool was_X_left = (X.data_unsafe ()->flags & RB_IS_LEFT_CHILD) != 0;
    bool was_X_red = is_red_no_check (X);
    minilib::destroy_at < node_type > (X.data_unsafe ());
    ptr_type::deallocate (X);

    ptr_type new_sub_root;

    if (! left_of_X || ! right_of_X) {
      new_sub_root = left_of_X ? left_of_X : right_of_X;
      if (parent_of_X) {
	if (was_X_left) set_left (parent_of_X, new_sub_root);
	else set_right (parent_of_X, new_sub_root);
      } else {
	set_root (new_sub_root);
	return;
      }
      if (was_X_red) return;
      if (is_red (new_sub_root)) {
	set_black (new_sub_root);
	return;
      }
    } else {
      ptr_type R = right_of_X;
      if (! R.data_unsafe ()->left) {
	/* R will be the node that replaces X */
        bool b = is_red_no_check (R);
        set_left_no_check_no_flag (R, left_of_X);
        set_color (R, was_X_red);
	minilib::pair < ptr_type, uint8_t > res = {R, 0x01};
	if (! b && is_red (R.data_unsafe ()->right)) {
	  set_black (R.data_unsafe ()->right);
	  b = true;
	}
        if (! b) res = do_rb_delete_rebalance_right (R);
        new_sub_root = res.first;
        if (parent_of_X) {
          if (was_X_left) set_left_no_check (parent_of_X, new_sub_root);
          else set_right_no_check (parent_of_X, new_sub_root);
        } else {
          set_root_no_check (new_sub_root);
	  return;
        }
	if (res.second) return;
      } else {
        ptr_type curr_p = R, curr = R.data_unsafe ()->left;
        while (curr.data_unsafe ()->left) {
	  curr_p = curr;
          curr = curr.data_unsafe ()->left;
        }
        ptr_type Y = curr; /* Y will be the node that replaces X */
	ptr_type Y_right = Y.data_unsafe ()->right;
        bool b = is_red_no_check (Y);
        set_left (curr_p, Y_right);
        set_left_no_check_no_flag (Y, left_of_X);
	set_color (Y, was_X_red);

	if (! b && is_red (Y_right)) {
	  set_black (Y_right);
	  b = true;
	}

	if (b) {
	  set_right_no_check_no_flag (Y, R);
	  if (parent_of_X) {
	    if (was_X_left) set_left_no_check (parent_of_X, Y);
	    else set_right_no_check (parent_of_X, Y);
	  } else {
	    set_root_no_check (Y);
	  }
	  return;
	}

        ptr_type p = curr_p;
        while (true) {
          bool is_at_R = p == R;
	  ptr_type parent_of_p = p.data_unsafe ()->parent;
          auto res = do_rb_delete_rebalance_left (p);
          if (!is_at_R) {
	    /* if res.second == 0x00, then res.first is just p itself, and there's no need to call set_left */
	    if (res.second) {
	      set_left_no_check (parent_of_p, res.first);
	      set_right_no_check_no_flag (Y, R);
	      if (parent_of_X) {
		if (was_X_left) set_left_no_check (parent_of_X, Y);
		else set_right_no_check (parent_of_X, Y);
	      } else {
		set_root_no_check (Y);
	      }
	      return;
	    }
            p = parent_of_p;
          } else {
	    set_right_no_check (Y, res.first);
	    if (res.second) {
	      if (parent_of_X) {
		if (was_X_left) set_left_no_check (parent_of_X, Y);
		else set_right_no_check (parent_of_X, Y);
	      } else {
		set_root_no_check (Y);
	      }
	      return;
	    }
            break;
          }
        }

        auto res = do_rb_delete_rebalance_right (Y);
        new_sub_root = res.first;
        if (parent_of_X) {
          if (was_X_left) set_left_no_check (parent_of_X, new_sub_root);
          else set_right_no_check (parent_of_X, new_sub_root);
        } else {
          set_root_no_check (new_sub_root);
	  return;
        }
	if (res.second) return;
      }
    }

    ptr_type curr = parent_of_X; /* If we reach this point, then curr != nullptr */
    bool is_left = was_X_left;

    while (true) {
      ptr_type parent = curr.data_unsafe ()->parent;
      bool was_left = (curr.data_unsafe ()->flags & RB_IS_LEFT_CHILD) != 0;
      minilib::pair < ptr_type, uint8_t > res;
      if (is_left) {
        res = do_rb_delete_rebalance_left (curr);
      } else {
        res = do_rb_delete_rebalance_right (curr);
      }
      /* if res.second == 0x00, then res.first is just curr itself */
      if (res.second) {
	if (parent) {
	  if (was_left) set_left_no_check (parent, res.first);
	  else set_right_no_check (parent, res.first);
	} else {
	  set_root_no_check (res.first);
	}
	return;
      }
      curr = parent;
      is_left = was_left;
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
      if (curr == sub) {
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

  /* The caller should check other.root is not nullptr */
  constexpr void copy_tree_from (const rbtree& other) requires (minilib::is_copy_constructible_v < T >) {
    auto runtime = get_runtime ();
    auto other_runtime = other.get_runtime ();

    ptr_type src_root = other_runtime->root;
    ptr_type dst_root = allocate_node (src_root.data_unsafe ()->flags);
    minilib::construct_at < T > (dst_root.data_unsafe ()->storage.data (), *(src_root.data_unsafe ()->storage.data ()));
    runtime->root = dst_root;

    ptr_type src = src_root;
    ptr_type dst = dst_root;

    while (true) {
      ptr_type l = src.data_unsafe ()->left;
      if (l) {
        ptr_type new_dst = allocate_node (l.data_unsafe ()->flags);
        minilib::construct_at < T > (new_dst.data_unsafe ()->storage.data (), *(l.data_unsafe ()->storage.data ()));
        set_left_no_check_no_flag (dst, new_dst);
        src = l;
        dst = new_dst;
        continue;
      }
      ptr_type r = src.data_unsafe ()->right;
      if (r) {
        ptr_type new_dst = allocate_node (r.data_unsafe ()->flags);
        minilib::construct_at < T > (new_dst.data_unsafe ()->storage.data (), *(r.data_unsafe ()->storage.data ()));
        set_right_no_check_no_flag (dst, new_dst);
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
          ptr_type new_dst = allocate_node (p_src.data_unsafe ()->right.data_unsafe ()->flags);
          minilib::construct_at < T > (new_dst.data_unsafe ()->storage.data (), *(p_src.data_unsafe ()->right.data_unsafe ()->storage.data ()));
          set_right_no_check_no_flag (p_dst, new_dst);
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
    auto p = allocate_node (0);
    runtime->root = p;
    return p;
  }

  constexpr ptr_type emplace_left_raw (const handle_type& h) {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    ptr_type X = ptr_type::from_counted_ref (h);
    if (X.data_unsafe ()->left) std::terminate ();
    auto L = allocate_node (RB_IS_RED | RB_IS_LEFT_CHILD);
    X.data_unsafe ()->left = L;
    L.data_unsafe ()->parent = X;
    if (is_red_no_check (X)) insert_rebalance (X, true);
    return L;
  }

  constexpr ptr_type emplace_right_raw (const handle_type& h) {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    ptr_type X = ptr_type::from_counted_ref (h);
    if (X.data_unsafe ()->right) std::terminate ();
    auto R = allocate_node (RB_IS_RED);
    X.data_unsafe ()->right = R;
    R.data_unsafe ()->parent = X;
    if (is_red_no_check (X)) insert_rebalance (X, false);
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
    if (other.get_runtime ()->root) copy_tree_from (other);
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
	  if (p) detach (leaf);
	  assign_node_data (l, leaf);
	  set_left_no_check_no_flag (dst, leaf);
	  src = l;
	  dst = leaf;
	  if (p) leaf = find_leaf (p); else leaf = nullptr;
	} else {
	  ptr_type new_dst = allocate_node (l.data_unsafe ()->flags);
	  minilib::construct_at < T > (new_dst.data_unsafe ()->storage.data (), *(l.data_unsafe ()->storage.data ()));
	  set_left_no_check_no_flag (dst, new_dst);
	  src = l;
	  dst = new_dst;
	}
	continue;
      }
      ptr_type r = src.data_unsafe ()->right;
      if (r) {
	if (leaf) {
	  ptr_type p = leaf.data_unsafe ()->parent;
	  if (p) detach (leaf);
	  assign_node_data (r, leaf);
	  set_right_no_check_no_flag (dst, leaf);
	  src = r;
	  dst = leaf;
	  if (p) leaf = find_leaf (p); else leaf = nullptr;
	} else {
	  ptr_type new_dst = allocate_node (r.data_unsafe ()->flags);
	  minilib::construct_at < T > (new_dst.data_unsafe ()->storage.data (), *(r.data_unsafe ()->storage.data ()));
	  set_right_no_check_no_flag (dst, new_dst);
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
	    if (p) detach (leaf);
	    assign_node_data (p_src.data_unsafe ()->right, leaf);
	    set_right_no_check_no_flag (p_dst, leaf);
	    src = p_src.data_unsafe ()->right;
	    dst = leaf;
	    if (p) leaf = find_leaf (p); else leaf = nullptr;
	  } else {
	    ptr_type new_dst = allocate_node (p_src.data_unsafe ()->right.data_unsafe ()->flags);
	    minilib::construct_at < T > (new_dst.data_unsafe ()->storage.data (), *(p_src.data_unsafe ()->right.data_unsafe ()->storage.data ()));
	    set_right_no_check_no_flag (p_dst, new_dst);
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

#undef RB_IS_LEFT_CHILD
#undef RB_IS_RED

#endif
