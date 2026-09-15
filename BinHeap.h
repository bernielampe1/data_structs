#pragma once

#include <ostream>
#include <utility>

#include "types.h"
#include "utils.h"

// Compile-time heap ordering: MINHEAP extracts the smallest element
// first, MAXHEAP the largest.
#define MINHEAP 0
#define MAXHEAP 1
#define HEAPTYPE MAXHEAP

#if HEAPTYPE == MINHEAP
#define CMPDIR <
#elif HEAPTYPE == MAXHEAP
#define CMPDIR >
#endif

template <typename T> class BinHeap;

template <typename T>
std::ostream &operator<<(std::ostream &, const BinHeap<T> &);

// BinHeap<T>: a fixed-capacity binary heap over a contiguous array.
//
// Rule-of-five-default (see README.md): all five special operations are
// defined. Moves are noexcept and leave the source empty and reusable.
//
// capacity() is the allocated slot count; size() is how many elements
// are currently stored. Counts and slot indices use u64 (types.h): the
// addressable capacity is the u64 range, not the earlier unsigned/32-bit
// one. insert() fails (returns false) when the heap is full rather than
// growing; peekroot()/extractroot() on an empty heap return a
// default-constructed T.
//
// Element requirements: T must be default-constructible and
// copy-assignable (the array is allocated uninitialized and filled by
// assignment).
template <typename T> class BinHeap {
private:
  T *_data;     // slot buffer; null when unallocated
  u64 _n;       // capacity (slots allocated)
  u64 _last;    // one past the last stored element

  static u64 parent(u64 i) { return (i - 1) / 2; } // slot i's parent

  static u64 left(u64 i) { return 2 * i + 1; } // left child slot

  static u64 right(u64 i) { return 2 * i + 2; } // right child slot

  void swap(u64 a, u64 b) { // exchange two stored elements
    T temp = _data[a];
    _data[a] = _data[b];
    _data[b] = temp;
  }

  // Sifts the element at i down until the heap property holds. The
  // child chosen is the one that beats the OTHER child too (not merely
  // the parent); otherwise a valid child can be skipped.
  void siftDown(u64 i) {
    while (true) {
      u64 l = left(i);
      u64 r = right(i);
      u64 best = i;

      if (l < _last && _data[l] CMPDIR _data[best])
        best = l;
      if (r < _last && _data[r] CMPDIR _data[best])
        best = r;

      if (best == i)
        return;

      swap(i, best);
      i = best;
    }
  }

  // Sifts the element at i up toward the root.
  void siftUp(u64 i) {
    while (i > 0) {
      u64 p = parent(i);
      if (!(_data[i] CMPDIR _data[p]))
        return;
      swap(i, p);
      i = p;
    }
  }

public:
  explicit BinHeap(u64 capacity)
      : _data(capacity ? new T[capacity] : 0), _n(capacity), _last(0) {}

  BinHeap() : _data(0), _n(0), _last(0) {}

  // Deep copy: same capacity, same elements.
  BinHeap(const BinHeap &o)
      : _data(o._n ? new T[o._n] : 0), _n(o._n), _last(o._last) {
    for (u64 i = 0; i < _last; i++)
      _data[i] = o._data[i];
  }

  // Steal o's buffer AND capacity; o left a default heap (capacity 0).
  // A moved-from BinHeap is therefore not reusable without being
  // reconstructed or assigned to (fixed-capacity semantics: the
  // capacity is part of the resource that moved).
  BinHeap(BinHeap &&o) noexcept
      : _data(o._data), _n(o._n), _last(o._last) {
    o._data = 0;
    o._n = 0;
    o._last = 0;
  }

  ~BinHeap() { delete[] _data; }

  // Deep copy: the new buffer is built and filled before the old one is
  // freed, so a failed allocation leaves *this untouched.
  BinHeap &operator=(const BinHeap &o) {
    if (this == &o)
      return *this;

    T *newData = o._n ? new T[o._n] : 0;
    for (u64 i = 0; i < o._last; i++)
      newData[i] = o._data[i];

    delete[] _data;
    _data = newData;
    _n = o._n;
    _last = o._last;

    return *this;
  }

  // Steal o's buffer (freeing ours, unconditionally).
  BinHeap &operator=(BinHeap &&o) noexcept {
    if (this != &o) {
      delete[] _data;
      _data = o._data;
      _n = o._n;
      _last = o._last;
      o._data = 0;
      o._n = 0;
      o._last = 0;
    }
    return *this;
  }

  // Constant-time exchange of both buffers.
  void swap(BinHeap &o) {
    std::swap(_data, o._data);
    std::swap(_n, o._n);
    std::swap(_last, o._last);
  }

  u64 size() const { return (_last); }      // stored element count

  u64 remaining() const { return (_n - _last); } // free slots

  u64 capacity() const { return (_n); }     // allocated slot count

  bool empty() const { return (_last == 0); }

  void clear() { _last = 0; } // forget all elements (slots stay allocated)

  // Stores elem, restoring the heap property. False when full.
  bool insert(const T &elem) {
    if (_last >= _n)
      return false;

    _data[_last++] = elem;
    siftUp(_last - 1);

    return true;
  }

  // Inserts arry[0..n) one by one. False (inserting nothing) when the
  // array cannot fit in the remaining slots.
  bool heapify(const T arry[], u64 n) {
    if (n > remaining())
      return false;

    for (u64 i = 0; i < n; i++)
      insert(arry[i]);

    return true;
  }

  // The root element (a default T when empty). Returned BY VALUE from
  // extract; peekroot on an empty heap has nothing to refer to.
  T peekroot() const { return (empty() ? T() : _data[0]); }

  // Removes and returns the root (a default T when empty).
  T extractroot() {
    if (empty())
      return T();

    T root = _data[0];
    _last--;

    if (_last > 0) {
      _data[0] = _data[_last];
      siftDown(0);
    }

    return root;
  }

  friend std::ostream &operator<<<>(std::ostream &os, const BinHeap<T> &rhs);
};

// Prints the heap one tree level per line. Level counter arithmetic uses
// u64 so levels past 31 (heap sizes beyond 2^31) do not shift a 32-bit 1.
template <typename T>
std::ostream &operator<<(std::ostream &os, const BinHeap<T> &rhs) {
  if (!rhs.empty()) {
    u64 levels = floor_log2(rhs.size()) + 1;
    u64 i = 0;

    for (u64 l = 0; l < levels; l++) {
      for (u64 j = 0; j < (u64(1) << l); j++) {
        if (i < rhs.size())
          os << rhs._data[i++] << " ";
        else
          break;
      }
      os << std::endl;
    }
  }

  return (os);
}
