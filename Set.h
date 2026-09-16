#pragma once

#include <cstddef>
#include <memory>
#include <ostream>
#include <utility>

#include "Exception.h"
#include "types.h"

template <typename T> class Set;
template <typename T>
std::ostream &operator<<(std::ostream &, const Set<T> &);

// Set<T>: an ordered set of unique elements in a sorted dynamic array.
//
// Iteration order is ascending by operator< (deterministic, unlike a
// hash set). Backing store doubles on demand; insert/erase shift
// elements O(n) in exchange for cache-friendly storage and O(log n)
// lookup -- the same trade std::vector-based sorted sets make.
//
// Rule-of-five implementation (see README.md): the destructor, copy and
// move construction, and copy and move assignment are all defined.
// Moves are noexcept and leave the source empty and reusable.
//
// insert() reports whether the element was new (false for a duplicate,
// like std::set::insert's bool); erase() reports whether anything was
// removed; contains() is the membership test. operator[] indexes the
// internal sorted order unchecked; at() is bounds-checked and throws
// Exception.
//
// Element requirements: T must be default-constructible (growth
// assigns through uninitialized memory element by element), and
// copy-assignable with a strict weak ordering via operator< (plus
// operator== for exact set comparisons).
//
// Iterator invalidation: any insert/erase/clear/assignment may move
// elements or reallocate, invalidating every iterator.
template <typename T> class Set {
private:
  T *_data;       // element buffer; null when empty
  u64 _n;         // stored element count
  u64 _capacity;  // allocated slot count

  // Binary search for v. Returns true and sets idx to the element's
  // position when present; returns false and sets idx to the position
  // where v belongs (the insertion point) either way.
  bool locate(const T &v, u64 &idx) const {
    u64 lo = 0, hi = _n;
    while (lo < hi) {
      u64 mid = lo + (hi - lo) / 2;
      if (_data[mid] < v)
        lo = mid + 1;
      else
        hi = mid;
    }
    idx = lo;
    return idx < _n && !(_data[idx] < v) && !(_data[idx] > v);
  }

  // Ensures capacity >= want, allocating a doubled buffer under an
  // RAII guard and moving the existing elements over.
  void grow(u64 want) {
    u64 newCap = _capacity ? _capacity : 8;
    while (newCap < want)
      newCap *= 2;

    std::unique_ptr<T[]> fresh(new T[newCap]());
    for (u64 i = 0; i < _n; i++)
      fresh[i] = _data[i];

    delete[] _data;
    _data = fresh.release();
    _capacity = newCap;
  }

public:
  typedef u64 size_type; // element counts and indices, up to 64 bits
  typedef T *iterator;   // contiguous buffer: raw pointers suffice
  typedef const T *const_iterator;

  Set() : _data(0), _n(0), _capacity(0) {}

  // Deep copy: same elements, same order.
  Set(const Set &o) : _data(0), _n(o._n), _capacity(o._n) {
    if (o._n) {
      std::unique_ptr<T[]> fresh(new T[o._n]());
      for (u64 i = 0; i < o._n; i++)
        fresh[i] = o._data[i];
      _data = fresh.release();
    }
  }

  // Steal o's buffer; o left empty and reusable.
  Set(Set &&o) noexcept : _data(o._data), _n(o._n), _capacity(o._capacity) {
    o._data = 0;
    o._n = 0;
    o._capacity = 0;
  }

  ~Set() { delete[] _data; }

  // Deep copy: the new buffer is built and filled before the old one is
  // freed, so a failed allocation leaves *this untouched.
  Set &operator=(const Set &o) {
    if (this == &o)
      return *this;

    std::unique_ptr<T[]> fresh;
    if (o._n) {
      fresh.reset(new T[o._n]());
      for (u64 i = 0; i < o._n; i++)
        fresh[i] = o._data[i];
    }

    delete[] _data;
    _data = fresh.release();
    _n = o._n;
    _capacity = o._n;

    return *this;
  }

  // Steal o's buffer (freeing ours, unconditionally).
  Set &operator=(Set &&o) noexcept {
    if (this != &o) {
      delete[] _data;
      _data = o._data;
      _n = o._n;
      _capacity = o._capacity;
      o._data = 0;
      o._n = 0;
      o._capacity = 0;
    }
    return *this;
  }

  // Constant-time exchange of both buffers.
  void swap(Set &o) {
    std::swap(_data, o._data);
    std::swap(_n, o._n);
    std::swap(_capacity, o._capacity);
  }

  size_type size() const { return _n; } // element count

  bool empty() const { return _n == 0; } // no elements?

  size_type capacity() const { return _capacity; } // allocated slots

  // Inserts v. True when v was new; false when v was already present
  // (the set is then unchanged: uniqueness is the invariant).
  bool insert(const T &v) {
    u64 idx;
    if (locate(v, idx))
      return false;

    if (_n == _capacity)
      grow(_n + 1);

    // shift the tail [idx, _n) right by one, from the back
    for (u64 i = _n; i > idx; i--)
      _data[i] = _data[i - 1];
    _data[idx] = v;
    _n++;

    return true;
  }

  // Inserts every element of [first, last).
  template <typename Iter> void insert(Iter first, Iter last) {
    for (; first != last; ++first)
      insert(*first);
  }

  // Removes v when present, reporting whether anything was erased.
  bool erase(const T &v) {
    u64 idx;
    if (!locate(v, idx))
      return false;

    for (u64 i = idx; i + 1 < _n; i++)
      _data[i] = _data[i + 1];
    _n--;

    return true;
  }

  // True when v is a member.
  bool contains(const T &v) const {
    u64 idx;
    return locate(v, idx);
  }

  // Releases all memory; the set becomes empty and reusable.
  void clear() {
    delete[] _data;
    _data = 0;
    _n = _capacity = 0;
  }

  T &operator[](size_type i) { return _data[i]; }             // unchecked

  const T &operator[](size_type i) const { return _data[i]; } // unchecked read

  // Bounds-checked access: throws Exception when i is out of range.
  const T &at(size_type i) const {
    if (i >= _n)
      throw Exception("Set::at: index outside the set");
    return _data[i];
  }

  iterator begin() { return _data; }
  const_iterator begin() const { return _data; }

  iterator end() { return _data + _n; }
  const_iterator end() const { return _data + _n; }

  // ---- set operations ----

  // Elements in either *this or other (classic sorted merge).
  Set setUnion(const Set &other) const {
    Set out;
    u64 i = 0, j = 0;
    while (i < _n || j < other._n) {
      const T *pick;
      if (i >= _n)
        pick = &other._data[j++];
      else if (j >= other._n)
        pick = &_data[i++];
      else if (other._data[j] < _data[i])
        pick = &other._data[j++];
      else if (_data[i] < other._data[j])
        pick = &_data[i++];
      else {
        pick = &_data[i]; // equal: take once, advance both
        i++;
        j++;
      }
      out.insert(*pick); // insert() dedups, so the equal case is safe
    }
    return out;
  }

  // Elements present in BOTH sets.
  Set setIntersection(const Set &other) const {
    Set out;
    u64 i = 0, j = 0;
    while (i < _n && j < other._n) {
      if (_data[i] < other._data[j])
        i++;
      else if (other._data[j] < _data[i])
        j++;
      else {
        out.insert(_data[i]); // equal: in both
        i++;
        j++;
      }
    }
    return out;
  }

  // Elements in *this but not in other.
  Set setDifference(const Set &other) const {
    Set out;
    u64 i = 0, j = 0;
    while (i < _n) {
      if (j >= other._n || _data[i] < other._data[j]) {
        out.insert(_data[i++]);
      } else if (other._data[j] < _data[i]) {
        j++;
      } else { // equal: in both, skip
        i++;
        j++;
      }
    }
    return out;
  }

  // True when every element of *this is also in other.
  bool isSubsetOf(const Set &other) const {
    if (_n > other._n)
      return false;

    u64 i = 0, j = 0;
    while (i < _n) {
      if (j >= other._n)
        return false;
      if (other._data[j] < _data[i]) {
        j++;
      } else if (_data[i] < other._data[j]) {
        return false; // my element missing from other
      } else {
        i++;
        j++;
      }
    }
    return true;
  }

  // True when both sets hold exactly the same elements.
  bool operator==(const Set &o) const {
    if (_n != o._n)
      return false;
    for (u64 i = 0; i < _n; i++)
      if (_data[i] < o._data[i] || o._data[i] < _data[i])
        return false;
    return true;
  }

  bool operator!=(const Set &o) const { return !(*this == o); }

  friend std::ostream &operator<<<>(std::ostream &os, const Set<T> &s);
};

// Prints the elements ascending, "{ ", separated by ", ", " }" at the
// end (mathematical set notation; the empty set prints "{ }").
template <typename T>
std::ostream &operator<<(std::ostream &os, const Set<T> &s) {
  os << "{";
  for (typename Set<T>::size_type i = 0; i < s.size(); ++i) {
    os << (i == 0 ? " " : ", ");
    os << s[i];
  }
  if (s.size() > 0)
    os << " ";
  os << "}";
  return os;
}

// Constant-time exchange (lets the std::swap idiom find the member).
template <typename T> void swap(Set<T> &a, Set<T> &b) { a.swap(b); }
