#pragma once

// Array<T>: a fixed-length, contiguous sequence container.
//
// A rule-of-five implementation (see README.md): the destructor, copy and
// move construction, and copy and move assignment are all defined. Where
// the semantics overlap with std::vector they match it: operator[] is
// unchecked, allocated elements are value-initialized (so an Array<int>
// starts at zero), and iterators are STL-conformant random-access
// iterators usable with the standard algorithms.
//
// Element counts and indices use size_type (u64, see types.h): 64 bits.
//
// Element requirements: T must be default-constructible and
// copy-assignable (a consequence of new T[n]() and assignment-based
// fills). Arrays of move-only types can be built and moved but not
// copied or resized.
//
// Iterator invalidation: resize(), clear(), swap(), and both assignment
// operators free or move the buffer, invalidating every iterator.

#include <cstddef>
#include <memory>
#include <ostream>
#include <utility>

#include "types.h"

template <typename T> class Array {
public:
  typedef u64 size_type; // element counts and indices, up to 64 bits

private:
  T *_data;         // element buffer; nullptr when _n == 0
  size_type _n;     // number of elements

  // Allocates n value-initialized elements under an RAII holder, so a
  // throwing fill leaks nothing.
  static std::unique_ptr<T[]> allocate(size_type n) {
    return std::unique_ptr<T[]>(new T[n]());
  }

public:
  // One class serves both iterator flavors: U is T for iterator and
  // const T for const_iterator. For a contiguous buffer, a wrapped
  // pointer is all an iterator needs.
  template <typename U> class Iterator {
  private:
    U *_curr; // current element; end() is one past the last

    Iterator(U *curr) : _curr(curr) {} // private: only Array builds these

    friend class Array<T>;

  public:
    // Traits that name this a random-access iterator to the STL.
    using value_type = U;
    using difference_type = std::ptrdiff_t;
    using pointer = U *;
    using reference = U &;
    using iterator_category = std::random_access_iterator_tag;

    bool operator==(const Iterator &o) const { return _curr == o._curr; } // same position
    bool operator!=(const Iterator &o) const { return _curr != o._curr; } // different position
    bool operator<(const Iterator &o) const { return _curr < o._curr; } // before o
    bool operator>(const Iterator &o) const { return _curr > o._curr; } // after o
    bool operator<=(const Iterator &o) const { return _curr <= o._curr; } // o or before
    bool operator>=(const Iterator &o) const { return _curr >= o._curr; } // o or after

    Iterator &operator++() { // advance one (pre)
      ++_curr;
      return *this;
    }
    Iterator operator++(int) { // advance one (post)
      Iterator it = *this;
      ++_curr;
      return it;
    }
    Iterator &operator--() { // retreat one (pre)
      --_curr;
      return *this;
    }
    Iterator operator--(int) { // retreat one (post)
      Iterator it = *this;
      --_curr;
      return it;
    }

    Iterator &operator+=(difference_type i) { // advance i elements
      _curr += i;
      return *this;
    }
    Iterator &operator-=(difference_type i) { // retreat i elements
      _curr -= i;
      return *this;
    }

    U &operator*() const { return *_curr; }         // the current element
    U *operator->() const { return _curr; }         // member access on it
    U &operator[](difference_type i) const { return _curr[i]; } // offset i

    Iterator operator+(difference_type i) const { return Iterator(_curr + i); } // this + i
    Iterator operator-(difference_type i) const { return Iterator(_curr - i); } // this - i
    difference_type operator-(const Iterator &o) const { // distance from o
      return _curr - o._curr;
    }

    friend Iterator operator+(difference_type i, const Iterator &it) { // i + it
      return it + i;
    }
  };

  using iterator = Iterator<T>;
  using const_iterator = Iterator<const T>;

  Array() : _data(nullptr), _n(0) {} // empty array

  explicit Array(size_type n) // n value-initialized elements
      : _data(allocate(n).release()), _n(n) {}

  Array(const T &o, size_type n) : _data(new T[n]()), _n(n) { // n copies of o
    for (size_type i = 0; i < _n; ++i)
      _data[i] = o;
  }

  // Deep copy of o. The buffer is held under an RAII guard until the
  // fill completes, so a throwing element copy leaks nothing.
  Array(const Array &o) : _data(allocate(o._n).release()), _n(o._n) {
    std::unique_ptr<T[]> guard(_data);
    for (size_type i = 0; i < _n; ++i)
      _data[i] = o._data[i];
    guard.release();
  }

  Array(Array &&o) noexcept : _data(o._data), _n(o._n) { // steal o's buffer; o left empty
    o._data = nullptr;
    o._n = 0;
  }

  ~Array() { delete[] _data; } // free the buffer

  // Deep copy: the new buffer is built and filled before the old one is
  // freed, so a failed allocation leaves *this untouched.
  Array &operator=(const Array &o) {
    if (this == &o)
      return *this;

    std::unique_ptr<T[]> newData = allocate(o._n); // freed if the fill throws
    for (size_type i = 0; i < o._n; ++i)
      newData[i] = o._data[i];

    delete[] _data;
    _data = newData.release();
    _n = o._n;

    return *this;
  }

  // Steal o's buffer (freeing ours, unconditionally -- delete[] on nullptr
  // is a no-op, and *this may own a buffer while reporting size 0).
  Array &operator=(Array &&o) noexcept {
    if (this != &o) {
      delete[] _data;
      _data = o._data;
      _n = o._n;
      o._data = nullptr;
      o._n = 0;
    }
    return *this;
  }

  // Constant-time exchange of both buffers.
  void swap(Array &o) {
    std::swap(_data, o._data);
    std::swap(_n, o._n);
  }

  // Resize to n, keeping the first min(n, _n) elements; new elements are
  // value-initialized. The new buffer is filled before the old is freed.
  void resize(size_type n) {
    if (n == _n)
      return;

    if (n == 0) { // nothing to keep: release everything
      clear();
      return;
    }

    std::unique_ptr<T[]> newData = allocate(n); // freed if the copy throws
    size_type keep = n < _n ? n : _n;
    for (size_type i = 0; i < keep; ++i)
      newData[i] = _data[i];

    delete[] _data;
    _data = newData.release();
    _n = n;
  }

  // Release all memory; the array becomes empty and reusable.
  void clear() {
    delete[] _data;
    _data = nullptr;
    _n = 0;
  }

  T &operator[](size_type i) { return _data[i]; }             // unchecked access
  const T &operator[](size_type i) const { return _data[i]; } // unchecked read

  T *data() { return _data; }             // direct buffer access
  const T *data() const { return _data; } // direct buffer read

  size_type size() const { return _n; }   // element count
  bool empty() const { return _n == 0; }  // no elements?

  iterator begin() { return iterator(_data); }                   // first element
  const_iterator begin() const { return const_iterator(_data); } // same, const

  iterator end() { return iterator(_data + _n); }                // past the last element
  const_iterator end() const { return const_iterator(_data + _n); } // same, const
};

// Prints the elements separated by ", " with no trailing separator.
template <typename T>
std::ostream &operator<<(std::ostream &os, const Array<T> &a) {
  for (typename Array<T>::size_type i = 0; i < a.size(); ++i) {
    if (i > 0)
      os << ", ";
    os << a[i];
  }
  return os;
}
