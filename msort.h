#pragma once

#include "Array.h"

template <typename T>
void merge(Array<T> &a, Array<T> &tmp, int l, int m, int r) {
  int i = l;
  int j = l;
  int k = m + 1;

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

  for (i = l; i <= r; i++)
    a[i] = tmp[i];
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
