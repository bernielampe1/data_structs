#ifndef __UTILS_H__
#define __UTILS_H__

unsigned floor_log2(unsigned v) {
  unsigned r = -1;
  while (v) { v >>= 1; r++; }
  return r;
}

#endif // __UTILS_H__
