#ifndef _BHL_QSORT_H_
#define _BHL_QSORT_H_

#include "Array.h"

template <typename T> void swap(T &a0, T &a1) {
  T t = a0;
  a0 = a1;
  a1 = t;
}

template <typename T> int partition(Array<T> &a, int l, int r) {
  T x = a[r];

  int j = l;
  for (int i = l; i < r; i++) {
    if (a[i] <= x) {
      if (i != j)
        swap(a[i], a[j]);
      j++;
    }
  }

  a[r] = a[j];
  a[j] = x;

  return (j);
}

template <typename T> void qsort(Array<T> &a, int l, int r) {
  if (l < r) {
    int p = partition(a, l, r);
    qsort(a, l, p - 1);
    qsort(a, p + 1, r);
  }
}

#endif // _BHL_QSORT_H_
