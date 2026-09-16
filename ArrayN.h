#pragma once

#include <cstddef>
#include <memory>
#include <ostream>
#include <utility>

#include "Exception.h"
#include "types.h"

// ArrayN<T, Rank>: an N-dimensional dense array over a flat contiguous
// buffer with row-major strides -- the last README todo ("N-dimensional
// Array").
//
// Layout: element (i0, i1, ..., i{R-1}) lives at flat offset
// i0*s0 + i1*s1 + ... where s{R-1} = 1, s{k} = s{k+1} * dim{k+1}. This
// is exactly how a Matrix flattens and how C multi-dimensional arrays
// work; the strides are exposed because offset arithmetic on a linear
// buffer is the concept this class exists to teach.
//
// Rule-of-five implementation (see README.md): the destructor, copy and
// move construction, and copy and move assignment are all defined.
// Moves are noexcept and leave the source empty and reusable.
//
// operator[](indices) with a std::initializer_list indexes one element
// (unchecked); at(indices) is bounds-checked and throws Exception.
// flat(i) addresses the linear buffer directly. Every dimension must
// be >= 1 (a Rank-dimensional product of empty dimensions has no
// elements; construct ArrayN<T, R>(0) only through the default ctor).
//
// Element requirements: T must be default-constructible and
// copy-assignable (allocations are value-initialized and filled by
// assignment).
template <typename T, unsigned Rank> class ArrayN {
private:
  static_assert(Rank >= 1, "ArrayN requires at least one dimension");

  T *_data;          // flat buffer, row-major; null when size() == 0
  u64 _dims[Rank];   // per-dimension element counts (>= 1)
  u64 _strides[Rank];// per-dimension flat offsets: stride{k} =
                     // stride{k+1} * dim{k+1}; stride{R-1} = 1
  u64 _n;            // total element count == product of dims

  // Recomputes strides from dims.
  void computeStrides() {
    _strides[Rank - 1] = 1;
    for (unsigned k = Rank - 1; k > 0; k--)
      _strides[k - 1] = _strides[k] * _dims[k];
  }

  u64 offset(const std::initializer_list<u64> &indices) const {
    u64 off = 0;
    unsigned k = 0;
    for (u64 i : indices) {
      off += i * _strides[k];
      k++;
    }
    return off;
  }

  void validateShape(const std::initializer_list<u64> &dims) const {
    if (dims.size() != Rank)
      throw Exception("ArrayN: initializer list must name every dimension");
  }

public:
  typedef u64 size_type;

  // Fresh dims: the buffer holds prod(dims) zeros.
  ArrayN(const std::initializer_list<u64> &dims)
      : _data(0), _n(0) {
    validateShape(dims);

    unsigned k = 0;
    for (u64 d : dims) {
      if (d == 0)
        throw Exception("ArrayN: dimensions must be positive");
      _dims[k++] = d;
    }

    computeStrides();

    _n = 1;
    for (k = 0; k < Rank; k++)
      _n *= _dims[k];

    std::unique_ptr<T[]> fresh(new T[_n]());
    _data = fresh.release();
  }

  // Copy: same dims, same strides, same elements.
  ArrayN(const ArrayN &o)
      : _data(0), _dims{}, _strides{}, _n(o._n) {
    for (unsigned k = 0; k < Rank; k++) {
      _dims[k] = o._dims[k];
      _strides[k] = o._strides[k];
    }

    std::unique_ptr<T[]> fresh(new T[_n]());
    for (u64 i = 0; i < _n; i++)
      fresh[i] = o._data[i];
    _data = fresh.release();
  }

  // Steal o's buffer; o left empty (0x... shape but internal state
  // consistent: the default ctor state).
  ArrayN(ArrayN &&o) noexcept : _data(o._data), _n(o._n) {
    for (unsigned k = 0; k < Rank; k++) {
      _dims[k] = o._dims[k];
      _strides[k] = o._strides[k];
      o._dims[k] = 0;
      o._strides[k] = 0;
    }
    _data = o._data;
    _n = o._n;
    o._data = 0;
    o._n = 0;
  }

  ~ArrayN() { delete[] _data; }

  // Copy assign: built before the old is freed (strong safety).
  ArrayN &operator=(const ArrayN &o) {
    if (this == &o)
      return *this;

    std::unique_ptr<T[]> fresh(new T[o._n]());
    for (u64 i = 0; i < o._n; i++)
      fresh[i] = o._data[i];

    delete[] _data;
    _data = fresh.release();
    _n = o._n;
    for (unsigned k = 0; k < Rank; k++) {
      _dims[k] = o._dims[k];
      _strides[k] = o._strides[k];
    }

    return *this;
  }

  // Steal o's buffer (freeing ours, unconditionally).
  ArrayN &operator=(ArrayN &&o) noexcept {
    if (this != &o) {
      delete[] _data;
      _data = o._data;
      _n = o._n;
      for (unsigned k = 0; k < Rank; k++) {
        _dims[k] = o._dims[k];
        _strides[k] = o._strides[k];
        o._dims[k] = 0;
        o._strides[k] = 0;
      }
      o._data = 0;
      o._n = 0;
    }
    return *this;
  }

  // Constant-time exchange of both buffers.
  void swap(ArrayN &o) {
    std::swap(_data, o._data);
    std::swap(_n, o._n);
    for (unsigned k = 0; k < Rank; k++) {
      std::swap(_dims[k], o._dims[k]);
      std::swap(_strides[k], o._strides[k]);
    }
  }

  // Per-dimension counts and strides (out params, house style has no
  // std::array dependency here).
  void shape(u64 *dimsOut, u64 *stridesOut) const {
    for (unsigned k = 0; k < Rank; k++) {
      dimsOut[k] = _dims[k];
      stridesOut[k] = _strides[k];
    }
  }

  u64 dim(unsigned k) const { // elements along axis k
    return _dims[k];
  }

  u64 stride(unsigned k) const { // flat offset step along axis k
    return _strides[k];
  }

  size_type size() const { return _n; } // total element count

  bool empty() const { return _n == 0; }

  // Unchecked element access by multi-index.
  T &operator[](const std::initializer_list<u64> &indices) {
    return _data[offset(indices)];
  }

  const T &operator[](const std::initializer_list<u64> &indices) const {
    return _data[offset(indices)];
  }

  // Bounds-checked access: throws Exception when the index list is not
  // Rank long or any coordinate is out of its dimension.
  T &at(const std::initializer_list<u64> &indices) {
    if (indices.size() != Rank)
      throw Exception("ArrayN::at: index list must name every dimension");

    u64 off = 0;
    unsigned k = 0;
    for (u64 i : indices) {
      if (i >= _dims[k])
        throw Exception("ArrayN::at: coordinate outside its dimension");
      off += i * _strides[k];
      k++;
    }
    return _data[off];
  }

  const T &at(const std::initializer_list<u64> &indices) const {
    if (indices.size() != Rank)
      throw Exception("ArrayN::at: index list must name every dimension");

    u64 off = 0;
    unsigned k = 0;
    for (u64 i : indices) {
      if (i >= _dims[k])
        throw Exception("ArrayN::at: coordinate outside its dimension");
      off += i * _strides[k];
      k++;
    }
    return _data[off];
  }

  // Direct linear addressing (0 <= i < size()): the row-major buffer
  // is the data structure; this is how you walk it coherently.
  T &flat(u64 i) { return _data[i]; }
  const T &flat(u64 i) const { return _data[i]; }
};

// Row-major array-on-a-line printer (2-D example: one line per row)).
template <typename T, unsigned Rank>
std::ostream &operator<<(std::ostream &os, const ArrayN<T, Rank> &a) {
  os << "ArrayN<" << Rank << "D> {";
  for (u64 i = 0; i < a.size(); i++) {
    if (i > 0)
      os << ", ";
    os << a.flat(i);
  }
  os << "}";
  return os;
}

// Constant-time exchange (lets the std::swap idiom find the member).
template <typename T, unsigned Rank>
void swap(ArrayN<T, Rank> &a, ArrayN<T, Rank> &b) {
  a.swap(b);
}
