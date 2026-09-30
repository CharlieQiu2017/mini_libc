/* Double linked list based on tagged pointers
   We only support counted_ref at the moment, but can support linked_ref later.
 */

#ifndef LIST_HPP
#define LIST_HPP

#include <stdint.h>
#include <tls.h>
#include <exception>
#include <type_traits.hpp>
#include <tagged_ptr.hpp>
#include <raw_array.hpp>
#include <utility.hpp>
#include <counter.hpp>

namespace minilib {

namespace detail {

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
struct list_node {
  minilib::tagged_ptr < list_node > prev, next;
  minilib::raw_array < T, 1 > storage;
};

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
struct list_runtime {
  size_t len;
  size_t container_id;
  minilib::tagged_ptr < minilib::detail::list_node < T > > head, tail;
};

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
struct list_constexpr {
  struct list_runtime < T > runtime;
  minilib::counter local_counter;
  minilib::counter * global_counter;
};

}

template < typename T >
requires (std::is_object_v < T > && ! std::is_array_v < T > && ! std::is_const_v < T > && ! std::is_volatile_v < T >)
class list {
  union {
    minilib::detail::list_runtime < T > runtime_;
    minilib::detail::list_constexpr < T > * constexpr_;
  };

public:
  constexpr list () {
    if consteval {
      constexpr_ = minilib::allocator < minilib::detail::list_constexpr < T > > :: allocate (1);
      minilib::construct_at < minilib::detail::list_constexpr < T > > (constexpr_);
      constexpr_->runtime.len = 0;
      /* head, tail are already initialized to nullptr by default constructor */
      constexpr_->runtime.container_id = 0;
      constexpr_->global_counter = nullptr;
    } else {
      minilib::construct_at < minilib::detail::list_runtime < T > > (minilib::addressof (runtime_));
      runtime_.len = 0;
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

  using node_type = minilib::detail::list_node < T >;
  using ptr_type = minilib::tagged_ptr < node_type >;
  using handle_type = minilib::counted_ref < node_type >;

private:
  constexpr minilib::detail::list_runtime < T > * get_runtime () {
    minilib::detail::list_runtime < T > * runtime = nullptr;
    if consteval {
      runtime = minilib::addressof (constexpr_->runtime);
    } else {
      runtime = minilib::addressof (runtime_);
    }
    return runtime;
  }

  constexpr const minilib::detail::list_runtime < T > * get_runtime () const {
    const minilib::detail::list_runtime < T > * runtime = nullptr;
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

  /* Create node when there are no nodes. The caller must have checked this. */
  constexpr ptr_type create_node_empty () {
    auto runtime = get_runtime ();
    auto p = ptr_type::allocate (runtime->container_id, get_local_counter ());
    minilib::construct_at < node_type > (p.data ());
    p->prev = nullptr;
    p->next = nullptr;
    runtime->head = p;
    runtime->tail = p;
    runtime->len = 1;
    return p;
  }

  constexpr ptr_type create_node_before (ptr_type hp) {
    auto runtime = get_runtime ();
    node_type& node = *(hp.data_unsafe ());
    auto p = ptr_type::allocate (runtime->container_id, get_local_counter ());
    auto pn = p.data_unsafe ();
    minilib::construct_at < node_type > (pn);
    pn->prev = node.prev;
    pn->next = hp;
    if (node.prev) {
      node.prev.data_unsafe ()->next = p;
    } else {
      runtime->head = p;
    }
    node.prev = p;
    runtime->len++;
    return p;
  }

  constexpr ptr_type create_node_after (ptr_type hp) {
    auto runtime = get_runtime ();
    node_type& node = *(hp.data_unsafe ());
    auto p = ptr_type::allocate (runtime->container_id, get_local_counter ());
    auto pn = p.data_unsafe ();
    minilib::construct_at < node_type > (pn);
    pn->next = node.next;
    pn->prev = hp;
    if (node.next) {
      node.next.data_unsafe ()->prev = p;
    } else {
      runtime->tail = p;
    }
    node.next = p;
    runtime->len++;
    return p;
  }

  constexpr void remove_node (ptr_type hp) {
    auto runtime = get_runtime ();
    node_type& node = *(hp.data_unsafe ());
    if (node.prev) {
      node.prev.data_unsafe ()->next = node.next;
    } else {
      runtime->head = node.next;
    }
    if (node.next) {
      node.next.data_unsafe ()->prev = node.prev;
    } else {
      runtime->tail = node.prev;
    }
    /* The caller is responsible for calling ~T() on storage */
    minilib::destroy_at < node_type > (minilib::addressof (node));
    ptr_type::deallocate (hp);
    runtime->len--;
  }

  constexpr ptr_type emplace_front_raw () {
    auto runtime = get_runtime ();
    ptr_type p;
    if (runtime->head) {
      p = create_node_before (runtime->head);
    } else {
      p = create_node_empty ();
    }
    return p;
  }

  constexpr ptr_type emplace_back_raw () {
    auto runtime = get_runtime ();
    ptr_type p;
    if (runtime->tail) {
      p = create_node_after (runtime->tail);
    } else {
      p = create_node_empty ();
    }
    return p;
  }

  constexpr ptr_type emplace_before_raw (const handle_type& h) {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    return create_node_before (ptr_type::from_counted_ref (h));
  }

  constexpr ptr_type emplace_after_raw (const handle_type& h) {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    return create_node_after (ptr_type::from_counted_ref (h));
  }

  constexpr void pop_front_unsafe () requires (minilib::is_destructible_v < T >) {
    auto runtime = get_runtime ();
    ptr_type p = runtime->head;
    node_type& node = *(p.data_unsafe ());
    T * tp = node.storage.data ();
    minilib::destroy_at < T > (tp);
    runtime->head = node.next;
    if (node.next) {
      node.next.data_unsafe ()->prev = nullptr;
    } else {
      runtime->tail = nullptr;
    }
    minilib::destroy_at < node_type > (minilib::addressof (node));
    ptr_type::deallocate (p);
    runtime->len--;
  }

  constexpr void pop_back_unsafe () requires (minilib::is_destructible_v < T >) {
    auto runtime = get_runtime ();
    ptr_type p = runtime->tail;
    node_type& node = *(p.data_unsafe ());
    T * tp = node.storage.data ();
    minilib::destroy_at < T > (tp);
    runtime->tail = node.prev;
    if (node.prev) {
      node.prev.data_unsafe ()->next = nullptr;
    } else {
      runtime->head = nullptr;
    }
    minilib::destroy_at < node_type > (minilib::addressof (node));
    ptr_type::deallocate (p);
    runtime->len--;
  }

public:

  /* DANGEROUS: We provide this function to cover cases that construct_at() and default_construct_at() cannot cover.
     The caller MUST immediately use placement new or other means to construct an object at the given storage.
   */
  constexpr handle_type emplace_front_null () {
    ptr_type p = emplace_front_raw ();
    handle_type h;
    p.create_counted_ref (h);
    return h;
  }

  constexpr handle_type emplace_front_default () requires (minilib::is_truly_default_constructible_v < T >) {
    ptr_type p = emplace_front_raw ();
    handle_type h;
    p.create_counted_ref (h);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::default_construct_at < T > (tp);
    return h;
  }

  template < typename... Args >
  requires (minilib::is_constructible_v < T, Args&&... >)
  constexpr handle_type emplace_front (Args&&... args) {
    ptr_type p = emplace_front_raw ();
    handle_type h;
    p.create_counted_ref (h);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, minilib::forward < Args > (args)...);
    return h;
  }

  constexpr handle_type push_front (const T& val) requires (minilib::is_copy_constructible_v < T >) {
    ptr_type p = emplace_front_raw ();
    handle_type h;
    p.create_counted_ref (h);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, val);
    return h;
  }

  constexpr handle_type push_front (T&& val) requires (minilib::is_move_constructible_v < T >) {
    ptr_type p = emplace_front_raw ();
    handle_type h;
    p.create_counted_ref (h);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, minilib::move (val));
    return h;
  }

  constexpr handle_type emplace_back_null () {
    ptr_type p = emplace_back_raw ();
    handle_type h;
    p.create_counted_ref (h);
    return h;
  }

  constexpr handle_type emplace_back_default () requires (minilib::is_truly_default_constructible_v < T >) {
    ptr_type p = emplace_back_raw ();
    handle_type h;
    p.create_counted_ref (h);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::default_construct_at < T > (tp);
    return h;
  }

  template < typename... Args >
  requires (minilib::is_constructible_v < T, Args&&... >)
  constexpr handle_type emplace_back (Args&&... args) {
    ptr_type p = emplace_back_raw ();
    handle_type h;
    p.create_counted_ref (h);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, minilib::forward < Args > (args)...);
    return h;
  }

  constexpr handle_type push_back (const T& val) requires (minilib::is_copy_constructible_v < T >) {
    ptr_type p = emplace_back_raw ();
    handle_type h;
    p.create_counted_ref (h);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, val);
    return h;
  }

  constexpr handle_type push_back (T&& val) requires (minilib::is_move_constructible_v < T >) {
    ptr_type p = emplace_back_raw ();
    handle_type h;
    p.create_counted_ref (h);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, minilib::move (val));
    return h;
  }

  constexpr handle_type emplace_before_null (const handle_type& h) {
    ptr_type p = emplace_before_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    return nh;
  }

  constexpr handle_type emplace_before_default (const handle_type& h) requires (minilib::is_truly_default_constructible_v < T >) {
    ptr_type p = emplace_before_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::default_construct_at < T > (tp);
    return nh;
  }

  template < typename... Args >
  requires (minilib::is_constructible_v < T, Args&&... >)
  constexpr handle_type emplace_before (const handle_type& h, Args&&... args) {
    ptr_type p = emplace_before_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, minilib::forward < Args > (args)...);
    return nh;
  }

  constexpr handle_type insert_before (const handle_type& h, const T& val) requires (minilib::is_copy_constructible_v < T >) {
    ptr_type p = emplace_before_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, val);
    return nh;
  }

  constexpr handle_type insert_before (const handle_type& h, T&& val) requires (minilib::is_move_constructible_v < T >) {
    ptr_type p = emplace_before_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, minilib::move (val));
    return nh;
  }

  constexpr handle_type emplace_after_null (const handle_type& h) {
    ptr_type p = emplace_after_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    return nh;
  }

  constexpr handle_type emplace_after_default (const handle_type& h) requires (minilib::is_truly_default_constructible_v < T >) {
    ptr_type p = emplace_after_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::default_construct_at < T > (tp);
    return nh;
  }

  template < typename... Args >
  requires (minilib::is_constructible_v < T, Args&&... >)
  constexpr handle_type emplace_after (const handle_type& h, Args&&... args) {
    ptr_type p = emplace_after_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, minilib::forward < Args > (args)...);
    return nh;
  }

  constexpr handle_type insert_after (const handle_type& h, const T& val) requires (minilib::is_copy_constructible_v < T >) {
    ptr_type p = emplace_after_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, val);
    return nh;
  }

  constexpr handle_type insert_after (const handle_type& h, T&& val) requires (minilib::is_move_constructible_v < T >) {
    ptr_type p = emplace_after_raw (h);
    handle_type nh;
    p.create_counted_ref (nh);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::construct_at < T > (tp, minilib::move (val));
    return nh;
  }

  constexpr void pop_front () requires (minilib::is_destructible_v < T >) {
    auto runtime = get_runtime ();
    if (runtime->head) {
      pop_front_unsafe ();
    } else {
      std::terminate ();
    }
  }

  constexpr void pop_back () requires (minilib::is_destructible_v < T >) {
    auto runtime = get_runtime ();
    if (runtime->tail) {
      pop_back_unsafe ();
    } else {
      std::terminate ();
    }
  }

  constexpr void remove (const handle_type& h) requires (minilib::is_destructible_v < T >) {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    auto p = ptr_type::from_counted_ref (h);
    T * tp = p.data_unsafe ()->storage.data ();
    minilib::destroy_at < T > (tp);
    remove_node (p);
  }

  /* DANGEROUS: Inform the container that the data in a node has been manually destructed.
     Used for types that have private destructors.
   */
  constexpr void inform_destruct (const handle_type& h) {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    auto p = ptr_type::from_counted_ref (h);
    remove_node (p);
  }

  constexpr void clear () {
    auto runtime = get_runtime ();
    if constexpr (minilib::is_destructible_v < T >) {
      while (runtime->head) {
	pop_front_unsafe ();
      }
    } else {
      if (runtime->head) {
	std::terminate ();
      }
    }
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

  constexpr handle_type front () const {
    auto runtime = get_runtime ();
    handle_type h;
    if (runtime->head) {
      runtime->head.create_counted_ref (h);
    }
    return h;
  }

  constexpr handle_type back () const {
    auto runtime = get_runtime ();
    handle_type h;
    if (runtime->tail) {
      runtime->tail.create_counted_ref (h);
    }
    return h;
  }

  constexpr handle_type prev (const handle_type& h) const {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    handle_type ph;
    auto p = ptr_type::from_counted_ref (h).data_unsafe ()->prev;
    if (p) {
      p.create_counted_ref (ph);
    }
    return ph;
  }

  constexpr handle_type next (const handle_type& h) const {
    auto runtime = get_runtime ();
    if (! h.check (runtime->container_id)) std::terminate ();
    handle_type nh;
    auto p = ptr_type::from_counted_ref (h).data_unsafe ()->next;
    if (p) {
      p.create_counted_ref (nh);
    }
    return nh;
  }

  constexpr size_t size () const {
    auto runtime = get_runtime ();
    return runtime->len;
  }

  constexpr ~list () {
    clear ();
    if consteval {
      minilib::destroy_at < minilib::detail::list_constexpr < T > > (constexpr_);
      minilib::allocator < minilib::detail::list_constexpr < T > > :: deallocate (constexpr_, 1);
    }
  }

  constexpr list (const list& other) requires (minilib::is_copy_constructible_v < T >) {
    if consteval {
      constexpr_ = minilib::allocator < minilib::detail::list_constexpr < T > > :: allocate (1);
      minilib::construct_at < minilib::detail::list_constexpr < T > > (constexpr_);
      constexpr_->runtime.len = 0;
      constexpr_->global_counter = other.constexpr_->global_counter;
      if (constexpr_->global_counter != nullptr) {
	constexpr_->runtime.container_id = constexpr_->global_counter->get ();
      } else {
	constexpr_->runtime.container_id = 0;
      }
    } else {
      minilib::construct_at < minilib::detail::list_runtime < T > > (minilib::addressof (runtime_));
      runtime_.len = 0;
      runtime_.container_id = get_tls_counter ();
    }

    auto other_runtime = other.get_runtime ();
    auto p = other_runtime->head;
    while (p) {
      push_back (*(p.data_unsafe ()->storage.data ()));
      p = p.data_unsafe ()->next;
    }
  }

  constexpr list (list&& other) {
    if consteval {
      constexpr_ = minilib::allocator < minilib::detail::list_constexpr < T > > :: allocate (1);
      minilib::construct_at < minilib::detail::list_constexpr < T > > (constexpr_);
      constexpr_->runtime = other.constexpr_->runtime;
      constexpr_->global_counter = other.constexpr_->global_counter;
      constexpr_->local_counter = other.constexpr_->local_counter;
      other.constexpr_->runtime.head = nullptr;
      other.constexpr_->runtime.tail = nullptr;
      other.constexpr_->runtime.len = 0;
      other.constexpr_->local_counter = minilib::counter ();
      if (constexpr_->global_counter != nullptr) {
	other.constexpr_->runtime.container_id = constexpr_->global_counter->get ();
      } else {
	other.constexpr_->runtime.container_id = 0;
      }
    } else {
      minilib::construct_at < minilib::detail::list_runtime < T > > (minilib::addressof (runtime_));
      runtime_ = other.runtime_;
      other.runtime_.head = nullptr;
      other.runtime_.tail = nullptr;
      other.runtime_.len = 0;
      other.runtime_.container_id = get_tls_counter ();
    }
  }

  constexpr list& operator= (const list& other) requires (minilib::is_copy_constructible_v < T >) {
    if (this == minilib::addressof (other)) return *this;

    auto runtime = get_runtime ();
    auto other_runtime = other.get_runtime ();
    auto p = runtime->head;
    auto q = other_runtime->head;

    while (static_cast < bool > (p) && static_cast < bool > (q)) {
      if constexpr (minilib::is_copy_assignable_v < T >) {
	*(p.data_unsafe ()->storage.data ()) = *(q.data_unsafe ()->storage.data ());
      } else if constexpr (minilib::is_destructible_v < T >) {
	minilib::destroy_at < T > (p.data_unsafe ()->storage.data ());
	minilib::construct_at < T > (p.data_unsafe ()->storage.data (), *(q.data_unsafe ()->storage.data ()));
      } else {
	std::terminate ();
      }
      p = p.data_unsafe ()->next;
      q = q.data_unsafe ()->next;
    }

    while (p) {
      auto np = p.data_unsafe ()->next;
      T * tp = p.data_unsafe ()->storage.data ();
      minilib::destroy_at < T > (tp);
      remove_node (p);
      p = np;
    }

    while (q) {
      push_back (*(q.data_unsafe ()->storage.data ()));
      q = q.data_unsafe ()->next;
    }

    return *this;
  }

  constexpr list& operator= (list&& other) {
    if (this == minilib::addressof (other)) return *this;
    clear ();
    if consteval {
      constexpr_->runtime = other.constexpr_->runtime;
      constexpr_->global_counter = other.constexpr_->global_counter;
      constexpr_->local_counter = other.constexpr_->local_counter;
      other.constexpr_->runtime.head = nullptr;
      other.constexpr_->runtime.tail = nullptr;
      other.constexpr_->runtime.len = 0;
      other.constexpr_->local_counter = minilib::counter ();
      if (constexpr_->global_counter != nullptr) {
	other.constexpr_->runtime.container_id = constexpr_->global_counter->get ();
      } else {
	other.constexpr_->runtime.container_id = 0;
      }
    } else {
      runtime_ = other.runtime_;
      other.runtime_.head = nullptr;
      other.runtime_.tail = nullptr;
      other.runtime_.len = 0;
      other.runtime_.container_id = get_tls_counter ();
    }
    return *this;
  }
};

}

#endif
