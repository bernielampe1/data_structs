#pragma once

#include <ostream>
#include <utility>

#include "BinHeap.h"
#include "types.h"

// PriorityQueue<T>: the priority-queue ADT as a thin wrapper over
// BinHeap (MAXHEAP by default: the largest priority pops first).
//
// Weight vs BinHeap: BinHeap exposes the array mechanics (insertion
// by sift, capacity planning, direct root peek/extract) while this
// class exposes the ADT verbs (push, popTop, top, changePriority) and
// tracks entry COUNT semantics -- the classic separation of interface
// from representation for teaching. changePriority() re-sifts in the
// right direction after a caller modifies an element's value, which is
// the operation a bare heap makes awkward and a scheduler needs.
//
// Rules (README): rule-of-five complete by delegation -- the single
// BinHeap member carries all resources, and the wrapper's five
// operations each forward (moves transfer the wrapper's heap wholesale;
// both are noexcept).
//
// T requirements: the BinHeap contract (default-constructible,
// copy-assignable, operator< / operator> for ordering).
template <typename T> class PriorityQueue;
template <typename T>
std::ostream &operator<<(std::ostream &, const PriorityQueue<T> &);

template <typename T> class PriorityQueue {
private:
  BinHeap<T> _heap; // HEAPTYPE's ordering applies (MAXHEAP: max pops)

public:
  typedef u64 size_type;

  PriorityQueue() : _heap() {}

  explicit PriorityQueue(u64 capacity) : _heap(u64(capacity)) {}

  PriorityQueue(const PriorityQueue &o) = default;

  // Steal the wrapped heap; the source becomes a capacity-0 heap (the
  // BinHeap moved-from contract), which reports empty.
  PriorityQueue(PriorityQueue &&o) noexcept : _heap(std::move(o._heap)) {}

  ~PriorityQueue() = default;

  PriorityQueue &operator=(const PriorityQueue &o) = default;

  PriorityQueue &operator=(PriorityQueue &&o) noexcept {
    if (this != &o)
      _heap = std::move(o._heap);
    return *this;
  }

  // Constant-time exchange of both queues.
  void swap(PriorityQueue &o) { _heap.swap(o._heap); }

  size_type size() const { return _heap.size(); }

  bool empty() const { return _heap.empty(); }

  size_type capacity() const { return _heap.capacity(); }

  void clear() { _heap.clear(); }

  // Pushes d with its natural priority (T's own operator<). False
  // when the queue is at capacity (same contract as BinHeap::insert).
  bool push(const T &d) { return _heap.insert(d); }

  // The element that popTop() would remove (a default T when empty).
  T top() const { return _heap.peekroot(); }

  // Removes and returns the highest-priority element (default T when
  // empty). O(log n).
  T popTop() { return _heap.extractroot(); }

  // Reports whether k is present in the queue (linear scan: the heap
  // array holds all entries; presence queries are not the ADT's strong
  // suit and are exposed only for tests).
  bool contains(const T &k) const {
    for (u64 i = 0; i < _heap.size(); i++)
      if (k == _heap[i])
        return true;
    return false;
  }

  // Re-establishes the heap property after the caller changed some
  // element's value: set the element via entryRef(i), mutate it, then
  // call changed() -- or call changePriority(old, nu) to do both in
  // one step (O(n) search + O(log n) re-sift).
  T &entryRef(u64 i) {
    if (i >= _heap.size())
      throw Exception("PriorityQueue: entry index outside the queue");
    return _heap[i];
  }

  void changed(u64 i) {
    if (i >= _heap.size())
      throw Exception("PriorityQueue: entry index outside the queue");

    // value went UP or DOWN: one of the two sifts restores order
    _heap.changedAt(i);
  }

  // Replaces the value at the FIRST position whose entry equals old.
  // Returns false when nothing matched.
  bool changePriority(const T &old, const T &nu) {
    for (u64 i = 0; i < _heap.size(); i++) {
      if (old == _heap[i]) {
        _heap[i] = nu;
        changed(i);
        return true;
      }
    }
    return false;
  }

  // BinHeap-style helpers for teaching the mapping between ADT verbs
  // and heap mechanics.
  const BinHeap<T> &heap() const { return _heap; }

  friend std::ostream &operator<<<>(std::ostream &os,
                                    const PriorityQueue<T> &q);
};

// Prints the heap one level per line (the underlying structure's
// layout; popTop order is the left-to-right maxima, not this order).
template <typename T>
std::ostream &operator<<(std::ostream &os, const PriorityQueue<T> &q) {
  os << q._heap;
  return os;
}

// Constant-time exchange (lets the std::swap idiom find the member).
template <typename T>
void swap(PriorityQueue<T> &a, PriorityQueue<T> &b) {
  a.swap(b);
}
