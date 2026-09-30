#ifndef RBTREE_HPP
#define RBTREE_HPP

#include <stdint.h>
#include <stdlib.h>
#include <new>
#include <exception>
#include <concepts>
#include <type_traits>
#include <utility.hpp>

/* Red-black tree

   Abstract specification:
   (See https://gist.github.com/CharlieQiu2017/53db86b79b98c29481acc5a971ab4f36 for a complete formal verification)

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

   Fixpoint rb_tree_sorted_insert (z : val_t) (tree : rb_tree) : (rb_tree * bool * bool) :=
     match tree with
     | nil => (node z nil nil red, true, true)
     | node x l r c =>
         match T.compare z x with
         | Eq => (node x l r c, true, false)
         | Lt =>
             let (p, inserted) := rb_tree_sorted_insert z l in
             let (l', b) := p in
             if inserted then (rb_insert_rebalance_left x l' r c b, true) else (tree, true, false)
         | Gt =>
             let (p, inserted) := rb_tree_sorted_insert z r in
             let (r', b) := p in
             if inserted then (rb_insert_rebalance_right x l r' c b, true) else (tree, true, false)
         end
     end.

   Fixpoint rb_tree_sorted_delete (z : val_t) (tree : rb_tree) : (rb_tree * bool * bool) :=
     match tree with
     | nil => (nil, true, false)
     | node x l r c =>
         match T.compare z x with
         | Eq => (delete_root tree, true)
         | Lt =>
             let (p, deleted) := rb_tree_sorted_delete z l in
             let (l', b) := p in
             if deleted then (rb_delete_rebalance_left x l' r c b, true) else (tree, true, false)
         | Gt =>
             let (p, deleted) := rb_tree_sorted_delete z r in
             let (r', b) := p in
             if deleted then (rb_delete_rebalance_right x l r' c b, true) else (tree, true, false)
         end
     end.
 */

namespace minilib {

template < typename T >
requires (std::destructible < T >)
class rbtree_node {
public:
  /* These are indices into an arena vector.
     NULL indices are marked by -1ull.
   */
  size_t parent, left, right;

  /* parent == NULL -> RB_NODE_IS_LEFT_CHILD not specified;
     parent != NULL /\ RB_NODE_IS_LEFT_CHILD -> parent->left == this;
     parent != NULL /\ ! RB_NODE_IS_LEFT_CHILD -> parent->right == this;
     left->flags has RB_NODE_IS_LEFT_CHILD set;
     right->flags has RB_NODE_IS_LEFT_CHILD not set;
   */

  static constexpr uint8_t RB_NODE_IS_RED = 0x01;
  static constexpr uint8_t RB_NODE_IS_LEFT_CHILD = 0x02;

  uint8_t flags;

  bool is_red () const { return flags & RB_NODE_IS_RED; }
  bool is_black () const { return ! is_red (); }
  bool is_left_child () const { return flags & RB_NODE_IS_LEFT_CHILD; }
  bool is_right_child () const { return ! is_left_child (); }
  void paint_red () { flags |= RB_NODE_IS_RED; }
  void paint_black () { flags &= ~ RB_NODE_IS_RED; }
  void mark_left_child () { flags |= RB_NODE_IS_LEFT_CHILD; }
  void mark_right_child () { flags &= ~ RB_NODE_IS_LEFT_CHILD; }

  T data;

  template < typename... Args >
  requires (std::constructible_from < T, Args... >)
  rbtree_node (Args&&... args) :
    parent (-1ull), left (-1ull), right (-1ull), flags (0), data (T (minilib::forward < Args > (args)...))
  { }

  void reset () {
    parent = -1ull;
    left = -1ull;
    right = -1ull;
    flags = 0;
  }
};

#endif
