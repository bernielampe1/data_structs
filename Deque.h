#pragma once

#include <cstddef>
#include <ostream>
#include <utility>

#include "Exception.h"
#include "types.h"

template <typename T> class Deque;
template <typename T>
std::ostream &operator<<(std::ostream &, const Deque<T> &);

// Deque<T>: a double-ended queue over a circular (ring) buffer.
//
// push_front/push_back and pop_front/pop_back are all O(1) amortized:
// elements live in a power-of-two-ish slot array and both logical ends
// wrap modulo the allocated count. When the ring fills, the buffer
// re-centers and doubles. This is the container Stack (one end) and
// Queue (both ends, one in one out) specialize; comparing its memory
// layout with theirs is the pedagogical point.
//
// Rule-of-five implementation (see README.md): the destructor, copy and
// move construction, and copy and move assignment are all defined.
// Moves are noexcept and leave the source empty and reusable.
//
// front()/back() reference the two ends (UB when empty, same contract
// as std::deque). operator[]/at() index LOGICAL order (the ring is
// interior; at() is bounds-checked and throws Exception).
//
// Element requirements: T must be default-constructible (growth
// allocates value-initialized slots) and copy-assignable.
template <typename T> class Deque {
private:
  T *_slots;   // ring buffer; null when capacity is 0
  u64 _cap;    // allocated slots
  u64 _front;  // slot index of the first (logical) element
  u64 _n;      // element count

  // Re-centers the ring in a doubled buffer: logical order becomes
  // contiguous from slot 0 and both ends regain free space.
  void grow() {
    u64 newCap = _cap ? _cap * 2 : 8;

    std::unique_ptr<T[]> fresh(new T[newCap]());
    for (u64 i = 0; i < _n; i++)
      fresh[i] = _slots[(_front + i) % _cap];

    delete[] _slots;
    _slots = fresh.release();
    _cap = newCap;
    _front = 0;
  }

public:
  typedef u64 size_type;

  Deque() : _slots(0), _cap(0), _front(0), _n(0) {}

  // Deep copy: same elements, contiguous layout.
  Deque(const Deque &o) : _slots(0), _cap(o._n ? o._n : 0), _front(0),
                          _n(o._n) {
    if (o._n) {
      std::unique_ptr<T[]> fresh(new T[o._n]());
      for (u64 i = 0; i < o._n; i++)
        fresh[i] = o._slots[(o._front + i) % o._cap];
      _slots = fresh.release();
      _cap = o._n;
    }
  }

  // Steal o's ring; o left empty and reusable.
  Deque(Deque &&o) noexcept
      : _slots(o._slots), _cap(o._cap), _front(o._front), _n(o._n) {
    o._slots = 0;
    o._cap = 0;
    o._front = 0;
    o._n = 0;
  }

  ~Deque() { delete[] _slots; }

  // Deep copy: the new ring is built and filled before the old is
  // freed, so a failed allocation leaves *this untouched.
  Deque &operator=(const Deque &o) {
    if (this == &o)
      return *this;

    std::unique_ptr<T[]> fresh;
    if (o._n) {
      fresh.reset(new T[o._n]());
      for (u64 i = 0; i < o._n; i++)
        fresh[i] = o._slots[(o._front + i) % o._cap];
    }

    delete[] _slots;
    _slots = fresh.release();
    _cap = o._n;
    _front = 0;
    _n = o._n;

    return *this;
  }

  // Steal o's ring (freeing ours, unconditionally).
  Deque &operator=(Deque &&o) noexcept {
    if (this != &o) {
      delete[] _slots;
      _slots = o._slots;
      _cap = o._cap;
      _front = o._front;
      _n = o._n;
      o._slots = 0;
      o._cap = 0;
      o._front = 0;
      o._n = 0;
    }
    return *this;
  }

  // Constant-time exchange of both rings.
  void swap(Deque &o) {
    std::swap(_slots, o._slots);
    std::swap(_cap, o._cap);
    std::swap(_front, o._front);
    std::swap(_n, o._n);
  }

  size_type size() const { return _n; } // element count

  bool empty() const { return _n == 0; }

  size_type capacity() const { return _cap; } // allocated slots

  void clear() { // forget all elements (slots stay allocated)
    _n = 0;
    _front = 0;
  }

  // Inserts d at the FRONT (it becomes the oldest-position element,
  // the one pop_front() removes).
  void push_front(const T &d) {
    if (_n == _cap)
      grow();

    _front = (_front + _cap - 1) % _cap; // wrap backwards
    _slots[_front] = d;
    _n++;
  }

  // Inserts d at the BACK (the one pop_back() removes).
  void push_back(const T &d) {
    if (_n == _cap)
      grow();

    _slots[(_front + _n) % _cap] = d;
    _n++;
  }

  // Removes the front element (no-op when empty).
  void pop_front() {
    if (empty())
      return;
    _front = (_front + 1) % _cap;
    _n--;
    if (_n == 0)
      _front = 0;
  }

  // Removes the back element (no-op when empty).
  void pop_back() {
    if (empty())
      return;
    _n--;
    if (_n == 0)
      _front = 0;
  }

  T &front() { return _slots[_front]; }              // UB when empty

  const T &front() const { return _slots[_front]; }  // UB when empty

  T &back() { return _slots[(_front + _n - 1) % _cap]; }              // UB

  const T &back() const { return _slots[(_front + _n - 1) % _cap]; }  // UB

  // Logical-order indexing (unchecked): operator[](0) is front().
  T &operator[](size_type i) { return _slots[(_front + i) % _cap]; }

  const T &operator[](size_type i) const {
    return _slots[(_front + i) % _cap];
  }

  // Bounds-checked access: throws Exception when i >= size().
  const T &at(size_type i) const {
    if (i >= _n)
      throw Exception("Deque::at: index outside the deque");
    return _slots[(_front + i) % _cap];
  }

  friend std::ostream &operator<<<>(std::ostream &os, const Deque<T> &d);
};

// Prints the elements front to back, ", " separated, no trailing
// separator (List's printer format).
template <typename T>
std::ostream &operator<<(std::ostream &os, const Deque<T> &d) {
  bool first = true;
  for (u64 i = 0; i < d._n; i++) {
    if (!first)
      os << ", ";
    os << d._slots[(d._front + i) % d._cap];
    first = false;
  }
  return os;
}

// Constant-time exchange (lets the std::swap idiom find the member).
template <typename T> void swap(Deque<T> &a, Deque<T> &b) { a.swap(b); }
