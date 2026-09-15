#pragma once

#include "types.h"

// floor_log2(v): index of the highest set bit. Callers pass a value that
// fits the unsigned type of the overload chosen.

inline unsigned floor_log2(u32 v) {
  unsigned r = unsigned(-1);
  while (v) { v >>= 1; r++; }
  return r;
}

inline unsigned floor_log2(u64 v) {
  unsigned r = unsigned(-1);
  while (v) { v >>= 1; r++; }
  return r;
}
