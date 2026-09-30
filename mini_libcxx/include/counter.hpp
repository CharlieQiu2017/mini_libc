/* A simple counter */

#ifndef COUNTER_HPP
#define COUNTER_HPP

#include <stdint.h>

namespace minilib {

class counter {
  uint64_t ctr;

public:
  constexpr counter () : ctr (0) { }
  constexpr counter (const counter&) = default;
  constexpr counter (counter&&) = default;
  constexpr counter& operator= (const counter&) = default;
  constexpr counter& operator= (counter&&) = default;
  constexpr ~counter () = default;

  constexpr uint64_t get () { uint64_t r = ctr++; return r; }
};

}

#endif
