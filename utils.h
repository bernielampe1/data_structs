#pragma once

unsigned floor_log2(unsigned v) {
  unsigned r = -1;
  while (v) { v >>= 1; r++; }
  return r;
}
