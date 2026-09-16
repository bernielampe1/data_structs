#pragma once

#include "Array.h"

// tmp is indexed RELATIVE to l (slot 0 holds a[l]): the top-level
// msort allocates only the sub-range's length, so absolute indexes
// past that length would run off the buffer for any call with l > 0
// (the old code wrote tmp[i] with i in [l, r] and overflowed).
template <typename T>
void merge(Array<T> &a, Array<T> &tmp, int l, int m, int r) {
  int i = 0;  // relative to l
  int j = l;  // left run cursor in a
  int k = m + 1; // right run cursor in a

  while (j <= m && k <= r) {
    if (a[j] <= a[k])
      tmp[i++] = a[j++];
    else
      tmp[i++] = a[k++];
  }

  while (j <= m)
    tmp[i++] = a[j++];
  while (k <= r)
    tmp[i++] = a[k++];

  for (i = 0; i <= r - l; i++)
    a[l + i] = tmp[i];
}

template <typename T> void recsort(Array<T> &a, Array<T> &tmp, int l, int r) {
  if (l < r) {
    int m = (l + r) / 2;

    recsort(a, tmp, l, m);
    recsort(a, tmp, m + 1, r);

    merge(a, tmp, l, m, r);
  }
}

template <typename T> void msort(Array<T> &a, int l, int r) {
  Array<T> tmp(r - l + 1);
  recsort(a, tmp, l, r);
}
