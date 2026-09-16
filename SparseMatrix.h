#pragma once

#include <cstddef>
#include <memory>
#include <ostream>
#include <utility>

#include "Exception.h"
#include "Matrix.h"
#include "types.h"

template <typename T> class SparseMatrix;
template <typename T>
std::ostream &operator<<(std::ostream &, const SparseMatrix<T> &);

// SparseMatrix<T>: a row-major sparse matrix keeping only nonzero
// entries in a sorted dynamic array of (row, col, value) triples.
//
// Entries are sorted ascending by (row, col) regardless of insertion
// order; the backing array doubles on demand and shifts on
// insert/erase, the same trade the sorted containers (Set, Map) make.
// get() is O(nnz-of-row) after the binary search; iteration touches
// only stored entries, so sparse-friendly algorithms stay proportional
// to nnz, never rows*cols.
//
// Zero semantics: set(r, c, 0) stores an EXPLICIT zero (numeric
// programs sometimes need one; nnz() counts it). Arithmetic results
// are normalized instead: +, -, element-wise *, and dot() drop entries
// whose computed value is zero (cancellation a-a gives the empty
// matrix), so results stay maximally sparse.
//
// Rule-of-five implementation (see README.md): the destructor, copy and
// move construction, and copy and move assignment are all defined.
// Moves are noexcept and leave the source empty and reusable.
//
// get(r, c) returns the stored value or T() when absent.
// set(r, c, v) inserts or replaces. Arithmetic operators (+, -,
// * element-wise, dot for matmul) require compatible shapes and throw
// Exception otherwise. toDense() converts to the dense Matrix<T>.
//
// Matrix.h feature parity: trace/submatrix/row/col/setRow/setCol and
// the isSquare/isSymmetric/isDiagonal/isIdentity predicates, Frobenius
// norm, ==/!=/almostEqual, elementwiseMin/Max, and apply/applied are
// reimplemented sparsely where meaningful. rank/rcond/powerIteration
// delegate to toDense() and Matrix's implementations (bearing Matrix's
// 16x16-working-set cap; documented per function). There is no
// begin()/end(): a sparse container has no contiguous buffer over its
// LOGICAL elements (the implicit zeros), so for_each(r, c, v) is the
// iteration contract.
//
// Element requirements: T must be default-constructible and
// copy-assignable.
template <typename T> class SparseMatrix {
private:
  struct Entry {
    Entry(u32 r, u32 c, const T &v) : _r(r), _c(c), _v(v) {}
    Entry() : _r(0), _c(0), _v() {}

    u32 _r, _c;
    T _v;

    bool before(u32 r, u32 c) const { // lexicographic (row, col)
      return _r < r || (_r == r && _c < c);
    }
    bool samePos(u32 r, u32 c) const { return _r == r && _c == c; }
  };

  Entry *_data;     // ascending by (row, col); null when empty
  u64 _nnz;         // stored entry count
  u64 _capacity;    // allocated slot count
  u32 _rows, _cols; // logical shape (may exceed any stored entry)

  // Binary search for position (r, c). True + idx when stored; false +
  // insertion point otherwise.
  bool locate(u32 r, u32 c, u64 &idx) const {
    u64 lo = 0, hi = _nnz;
    while (lo < hi) {
      u64 mid = lo + (hi - lo) / 2;
      if (_data[mid].before(r, c))
        lo = mid + 1;
      else
        hi = mid;
    }
    idx = lo;
    return idx < _nnz && _data[idx].samePos(r, c);
  }

  void grow(u64 want) {
    u64 newCap = _capacity ? _capacity : 8;
    while (newCap < want)
      newCap *= 2;

    std::unique_ptr<Entry[]> fresh(new Entry[newCap]);
    for (u64 i = 0; i < _nnz; i++)
      fresh[i] = _data[i];

    delete[] _data;
    _data = fresh.release();
    _capacity = newCap;
  }

  // Inserts or replaces (r, c) with v; caller has validated shape.
  void put(u32 r, u32 c, const T &v) {
    u64 idx;
    if (locate(r, c, idx)) {
      _data[idx]._v = v;
      return;
    }

    if (_nnz == _capacity)
      grow(_nnz + 1);

    for (u64 i = _nnz; i > idx; i--)
      _data[i] = _data[i - 1];
    _data[idx] = Entry(r, c, v);
    _nnz++;
  }

  // Raw row-major fill used by toDense() only.
  Matrix<T> dense() const {
    Matrix<T> m(_rows, _cols);
    for (u64 i = 0; i < _nnz; i++)
      m.get(_data[i]._r, _data[i]._c) = _data[i]._v;
    return m;
  }

public:
  typedef u64 size_type; // entry counts, up to 64 bits

  SparseMatrix() : _data(0), _nnz(0), _capacity(0), _rows(0), _cols(0) {}

  // An r x c zero matrix with no stored entries.
  SparseMatrix(const u32 r, const u32 c)
      : _data(0), _nnz(0), _capacity(0), _rows(r), _cols(c) {}

  // r x c built from a dense row-major array; zero entries are SKIPPED
  // (that is what sparse means), so the result holds only nonzeros.
  SparseMatrix(const T m[], const u32 r, const u32 c, const T &zero = T())
      : _data(0), _nnz(0), _capacity(0), _rows(r), _cols(c) {
    for (u32 i = 0; i < r; i++)
      for (u32 j = 0; j < c; j++)
        if (!(m[i * c + j] == zero)) // != via !(==) for generic T
          put(i, j, m[i * c + j]);
  }

  // Deep copy: same shape, same stored entries.
  SparseMatrix(const SparseMatrix &o)
      : _data(0), _nnz(o._nnz), _capacity(o._nnz), _rows(o._rows),
        _cols(o._cols) {
    if (o._nnz) {
      std::unique_ptr<Entry[]> fresh(new Entry[o._nnz]);
      for (u64 i = 0; i < o._nnz; i++)
        fresh[i] = o._data[i];
      _data = fresh.release();
    }
  }

  // Steal o's array; o left empty (shape retained? no: shape part of
  // the resource) and reusable as a fresh 0x0.
  SparseMatrix(SparseMatrix &&o) noexcept
      : _data(o._data), _nnz(o._nnz), _capacity(o._capacity),
        _rows(o._rows), _cols(o._cols) {
    o._data = 0;
    o._nnz = o._capacity = 0;
    o._rows = o._cols = 0;
  }

  ~SparseMatrix() { delete[] _data; }

  // Deep copy: the new array is built and filled before the old one is
  // freed, so a failed allocation leaves *this untouched.
  SparseMatrix &operator=(const SparseMatrix &o) {
    if (this == &o)
      return *this;

    std::unique_ptr<Entry[]> fresh;
    if (o._nnz) {
      fresh.reset(new Entry[o._nnz]);
      for (u64 i = 0; i < o._nnz; i++)
        fresh[i] = o._data[i];
    }

    delete[] _data;
    _data = fresh.release();
    _nnz = o._nnz;
    _capacity = o._nnz;
    _rows = o._rows;
    _cols = o._cols;

    return *this;
  }

  // Steal o's array (freeing ours, unconditionally).
  SparseMatrix &operator=(SparseMatrix &&o) noexcept {
    if (this != &o) {
      delete[] _data;
      _data = o._data;
      _nnz = o._nnz;
      _capacity = o._capacity;
      _rows = o._rows;
      _cols = o._cols;
      o._data = 0;
      o._nnz = o._capacity = 0;
      o._rows = o._cols = 0;
    }
    return *this;
  }

  // Constant-time exchange of both matrices.
  void swap(SparseMatrix &o) {
    std::swap(_data, o._data);
    std::swap(_nnz, o._nnz);
    std::swap(_capacity, o._capacity);
    std::swap(_rows, o._rows);
    std::swap(_cols, o._cols);
  }

  u32 rows() const { return (_rows); }

  u32 cols() const { return (_cols); }

  size_type nnz() const { return _nnz; } // stored (explicit) entry count

  bool empty() const { return _rows == 0 || _cols == 0; }

  // The stored value at (r, c), or T() when absent. Bounds-checked.
  T get(const u32 r, const u32 c) const {
    if (r >= _rows || c >= _cols)
      throw Exception("SparseMatrix::get: coordinates outside the matrix");

    u64 idx;
    if (locate(r, c, idx))
      return _data[idx]._v;
    return T();
  }

  // Inserts or replaces the value at (r, c). Bounds-checked.
  void set(const u32 r, const u32 c, const T &v) {
    if (r >= _rows || c >= _cols)
      throw Exception("SparseMatrix::set: coordinates outside the matrix");
    put(r, c, v);
  }

  // Removes the stored entry at (r, c) when present, reporting whether
  // anything was erased.
  bool erase(const u32 r, const u32 c) {
    if (r >= _rows || c >= _cols)
      throw Exception("SparseMatrix::erase: coordinates outside the matrix");

    u64 idx;
    if (!locate(r, c, idx))
      return false;

    for (u64 i = idx; i + 1 < _nnz; i++)
      _data[i] = _data[i + 1];
    _nnz--;

    return true;
  }

  // Drops every stored entry; shape and reusability are kept.
  void clear() {
    delete[] _data;
    _data = 0;
    _nnz = _capacity = 0;
    // _rows/_cols kept: the logical shape survives a clear()
  }

  // Element-wise + - * (same shape required). Result carries the union
  // of the operands' stored positions.
  SparseMatrix operator+(const SparseMatrix &m) const {
    if (m._rows != _rows || m._cols != _cols)
      throw Exception("sparse addition requires equal shapes");

    SparseMatrix out(_rows, _cols);
    u64 i = 0, j = 0;
    while (i < _nnz || j < m._nnz) {
      if (i >= _nnz) { // only m has entries left
        out.put(m._data[j]._r, m._data[j]._c, m._data[j]._v);
        j++;
      } else if (j >= m._nnz) { // only *this has entries left
        out.put(_data[i]._r, _data[i]._c, _data[i]._v);
        i++;
      } else if (m._data[j].before(_data[i]._r, _data[i]._c)) {
        out.put(m._data[j]._r, m._data[j]._c, m._data[j]._v);
        j++;
      } else if (_data[i].before(m._data[j]._r, m._data[j]._c)) {
        if (!(_data[i]._v == T())) // skip a stored zero: stays zero
          out.put(_data[i]._r, _data[i]._c, _data[i]._v);
        i++;
      } else { // same position: combine; cancelation drops the entry
        T sum = _data[i]._v + m._data[j]._v;
        if (!(sum == T()))
          out.put(_data[i]._r, _data[i]._c, sum);
        i++;
        j++;
      }
    }
    return out;
  }

  SparseMatrix operator-(const SparseMatrix &m) const {
    if (m._rows != _rows || m._cols != _cols)
      throw Exception("sparse subtraction requires equal shapes");

    SparseMatrix out(_rows, _cols);
    u64 i = 0, j = 0;
    while (i < _nnz || j < m._nnz) {
      T neg = -(m._data[j]._v);
      if (i >= _nnz) {
        if (!(neg == T()))
          out.put(m._data[j]._r, m._data[j]._c, neg);
        j++;
      } else if (j >= m._nnz) {
        if (!(_data[i]._v == T()))
          out.put(_data[i]._r, _data[i]._c, _data[i]._v);
        i++;
      } else if (m._data[j].before(_data[i]._r, _data[i]._c)) {
        if (!(neg == T()))
          out.put(m._data[j]._r, m._data[j]._c, neg);
        j++;
      } else if (_data[i].before(m._data[j]._r, m._data[j]._c)) {
        if (!(_data[i]._v == T()))
          out.put(_data[i]._r, _data[i]._c, _data[i]._v);
        i++;
      } else {
        T diff = _data[i]._v - m._data[j]._v;
        if (!(diff == T()))
          out.put(_data[i]._r, _data[i]._c, diff);
        i++;
        j++;
      }
    }
    return out;
  }

  // Element-wise product: nonzero only where BOTH operands store.
  SparseMatrix operator*(const SparseMatrix &m) const {
    if (m._rows != _rows || m._cols != _cols)
      throw Exception("sparse element-wise multiplication requires equal shapes");

    SparseMatrix out(_rows, _cols);
    u64 i = 0, j = 0;
    while (i < _nnz && j < m._nnz) { // intersection walk
      if (_data[i].before(m._data[j]._r, m._data[j]._c))
        i++;
      else if (m._data[j].before(_data[i]._r, _data[i]._c))
        j++;
      else {
        T prod = _data[i]._v * m._data[j]._v;
        if (!(prod == T())) // both nonzero but product may be 0
          out.put(_data[i]._r, _data[i]._c, prod);
        i++;
        j++;
      }
    }
    return out;
  }

  // Scalar arithmetic: touches only stored entries (0 + c etc. would be
  // dense; scalars apply to stored entries alone, documented).
  SparseMatrix operator+(const T &c) const {
    SparseMatrix out(*this);
    u64 w = 0; // write compaction: drop entries hitting zero (policy:
    for (u64 i = 0; i < out._nnz; i++) {  // arithmetic results stay sparse)
      T v = out._data[i]._v + c;
      if (!(v == T()))
        out._data[w++] = Entry(out._data[i]._r, out._data[i]._c, v);
    }
    out._nnz = w;
    return out;
  }

  SparseMatrix operator-(const T &c) const {
    SparseMatrix out(*this);
    u64 w = 0; // write compaction: drop entries hitting zero (policy:
    for (u64 i = 0; i < out._nnz; i++) {  // arithmetic results stay sparse)
      T v = out._data[i]._v - c;
      if (!(v == T()))
        out._data[w++] = Entry(out._data[i]._r, out._data[i]._c, v);
    }
    out._nnz = w;
    return out;
  }

  SparseMatrix operator*(const T &c) const {
    SparseMatrix out(*this);
    u64 w = 0; // write compaction: drop entries hitting zero (policy:
    for (u64 i = 0; i < out._nnz; i++) {  // arithmetic results stay sparse)
      T v = out._data[i]._v * c;
      if (!(v == T()))
        out._data[w++] = Entry(out._data[i]._r, out._data[i]._c, v);
    }
    out._nnz = w;
    return out;
  }

  SparseMatrix operator/(const T &c) const {
    SparseMatrix out(*this);
    u64 w = 0; // write compaction: drop entries hitting zero (policy:
    for (u64 i = 0; i < out._nnz; i++) {  // arithmetic results stay sparse)
      T v = out._data[i]._v / c;
      if (!(v == T()))
        out._data[w++] = Entry(out._data[i]._r, out._data[i]._c, v);
    }
    out._nnz = w;
    return out;
  }

  // True matrix product: O(nnz) per row of the right operand's column
  // lookups; result is a sparse matrix of shape this->rows() x m.cols().
  SparseMatrix dot(const SparseMatrix &m) const {
    if (_cols != m._rows)
      throw Exception("sparse matrices are not compatible for dot");

    SparseMatrix out(_rows, m._cols);

    // iterate over rows of *this; for each stored (r, c1, v1), walk
    // m's row c1 and accumulate into an accumulator map for row r.
    // Dense accumulator per output row keeps the pedagogy simple.
    for (u32 r = 0; r < _rows; r++) {
      // gather this row's range
      u64 start, end;
      rowRange(r, start, end);
      if (start == end)
        continue;

      // accumulate m's contributions for output row r
      for (u64 i = start; i < end; i++) {
        u32 c1 = _data[i]._c;
        const T &v1 = _data[i]._v;

        u64 mstart, mend;
        m.rowRange(c1, mstart, mend);
        for (u64 j = mstart; j < mend; j++) {
          u32 c2 = m._data[j]._c;
          T contrib = v1 * m._data[j]._v;
          if (contrib == T())
            continue; // nothing this term adds
          T cur = out.get(r, c2);        // kept result entries are
          T next = cur + contrib;        // normalize-drop when they hit
          if (next == T())               // exact zero by cancellation
            out.erase(r, c2);
          else
            out.put(r, c2, next);
        }
      }
    }

    return out;
  }

  SparseMatrix transpose() const {
    SparseMatrix out(_cols, _rows);
    for (u64 i = 0; i < _nnz; i++)
      out.put(_data[i]._c, _data[i]._r, _data[i]._v);
    return out; // put() keeps (row, col) sorted
  }

  // Dense conversion (the bridge to Matrix<T>).
  Matrix<T> toDense() const { return dense(); }

  // Builds a sparse matrix from a dense Matrix<T>; zero entries are
  // skipped.
  static SparseMatrix fromDense(const Matrix<T> &m, const T &zero = T()) {
    SparseMatrix out(m.rows(), m.cols());
    for (u32 r = 0; r < m.rows(); r++)
      for (u32 c = 0; c < m.cols(); c++)
        if (!(m.get(r, c) == zero))
          out.put(r, c, m.get(r, c));
    return out;
  }

  // In-order (row-major over stored entries) traversal: f(r, c, v).
  template <typename Func> void for_each(Func f) const {
    for (u64 i = 0; i < _nnz; i++)
      f(_data[i]._r, _data[i]._c, _data[i]._v);
  }


  /* ---- Matrix.h feature parity ---- */

  // sum of the diagonal entries of a square matrix
  T trace() const;

  // contiguous [r0, r0+h) x [c0, c0+w) block, copied sparsely
  SparseMatrix submatrix(const u32 r0, const u32 c0, const u32 h,
                         const u32 w) const;

  // dense row r / column c as Vec<T> (implicit zeros included)
  Vec<T> row(const u32 r) const;
  Vec<T> col(const u32 c) const;

  // replaces row r / column c with v; positions holding zero in v are
  // ERASED (not stored as explicit zeros), keeping the matrix sparse
  void setRow(const u32 r, const Vec<T> &v);
  void setCol(const u32 c, const Vec<T> &v);

  bool isSquare() const;
  bool isSymmetric() const;
  bool isDiagonal() const;
  bool isIdentity() const;

  // exact entry-wise equality (stored entries AND shape)
  bool operator==(const SparseMatrix &o) const;
  bool operator!=(const SparseMatrix &o) const;

  // entry-wise comparison within eps, over the UNION of both stored
  // position sets; a shape mismatch throws
  bool almostEqual(const SparseMatrix &o, double eps = 1e-6) const;

  double norm() const; // Frobenius norm over stored entries

  // rank of the dense equivalent via Matrix::rank (16x16 cap).
  u32 rank() const;

  // inverse condition estimate via Matrix::rcond (16x16 cap).
  double rcond() const;

  // dominant eigenvalue via Matrix::powerIteration (16x16 cap).
  double powerIteration(u32 maxIter = 1000, double eps = 1e-9) const;

  // element-wise min/max over the union of stored positions (a
  // position stored by only one operand competes against 0)
  SparseMatrix elementwiseMin(const SparseMatrix &o) const;
  SparseMatrix elementwiseMax(const SparseMatrix &o) const;

  // applies f to STORED entries only, in place / out of place; note an
  // f(x) == 0 REMOVES the entry (results stay maximally sparse)
  void apply(T (*f)(T));
  SparseMatrix applied(T (*f)(T)) const;

  friend std::ostream &operator<<<>(std::ostream &os,
                                    const SparseMatrix<T> &m);

private:
  // [start, end) range of the stored entries of row r.
  void rowRange(u32 r, u64 &start, u64 &end) const {
    locate(r, 0, start); // first entry with row >= r
    end = start;
    while (end < _nnz && _data[end]._r == r)
      end++;
  }
};

/* ---- Matrix.h feature parity: definitions ---- */

template <typename T> T SparseMatrix<T>::trace() const {
  if (_rows != _cols)
    throw Exception("trace of non-square matrix");

  T s = T(0);
  for (u64 i = 0; i < _nnz; i++)
    if (_data[i]._r == _data[i]._c)
      s += _data[i]._v;
  return s;
}

template <typename T>
SparseMatrix<T> SparseMatrix<T>::submatrix(const u32 r0, const u32 c0,
                                           const u32 h, const u32 w) const {
  if (h == 0 || w == 0 || r0 + h > _rows || c0 + w > _cols)
    throw Exception("submatrix outside the matrix");

  SparseMatrix out(h, w);
  for (u64 i = 0; i < _nnz; i++) {
    if (_data[i]._r >= r0 && _data[i]._r < r0 + h && _data[i]._c >= c0 &&
        _data[i]._c < c0 + w)
      out.put(_data[i]._r - r0, _data[i]._c - c0, _data[i]._v);
  }
  return out;
}

template <typename T> Vec<T> SparseMatrix<T>::row(const u32 r) const {
  if (r >= _rows)
    throw Exception("row index outside the matrix");

  Vec<T> v(_cols);
  for (u64 i = 0; i < _nnz; i++)
    if (_data[i]._r == r)
      v[_data[i]._c] = _data[i]._v;
  return v;
}

template <typename T> Vec<T> SparseMatrix<T>::col(const u32 c) const {
  if (c >= _cols)
    throw Exception("column index outside the matrix");

  Vec<T> v(_rows);
  for (u64 i = 0; i < _nnz; i++)
    if (_data[i]._c == c)
      v[_data[i]._r] = _data[i]._v;
  return v;
}

template <typename T> void SparseMatrix<T>::setRow(const u32 r, const Vec<T> &v) {
  if (r >= _rows)
    throw Exception("row index outside the matrix");
  if (v.len() != _cols)
    throw Exception("setRow vector length mismatch");

  // drop this row's stored entries, then insert v's nonzeros
  u64 w = 0; // compaction over the whole array
  for (u64 i = 0; i < _nnz; i++)
    if (_data[i]._r != r)
      _data[w++] = _data[i];
  _nnz = w;

  for (u32 c = 0; c < _cols; c++)
    if (!(v[c] == T()))
      put(r, c, v[c]);
}

template <typename T> void SparseMatrix<T>::setCol(const u32 c, const Vec<T> &v) {
  if (c >= _cols)
    throw Exception("column index outside the matrix");
  if (v.len() != _rows)
    throw Exception("setCol vector length mismatch");

  u64 w = 0;
  for (u64 i = 0; i < _nnz; i++)
    if (_data[i]._c != c)
      _data[w++] = _data[i];
  _nnz = w;

  for (u32 r = 0; r < _rows; r++)
    if (!(v[r] == T()))
      put(r, c, v[r]);
}

template <typename T> bool SparseMatrix<T>::isSquare() const {
  return _rows == _cols;
}

template <typename T> bool SparseMatrix<T>::isSymmetric() const {
  if (!isSquare())
    return false;

  for (u64 i = 0; i < _nnz; i++) {
    if (!(_data[i]._v == get(_data[i]._c, _data[i]._r)))
      return false; // transpose mismatch (absent counterpart = T())
  }
  return true;
}

template <typename T> bool SparseMatrix<T>::isDiagonal() const {
  if (!isSquare())
    return false;
  for (u64 i = 0; i < _nnz; i++)
    if (_data[i]._r != _data[i]._c)
      return false;
  return true;
}

template <typename T> bool SparseMatrix<T>::isIdentity() const {
  if (!isSquare())
    return false;

  SparseMatrix id(_rows, _cols);
  for (u32 i = 0; i < _rows; i++)
    id.put(i, i, T(1));
  return *this == id;
}

template <typename T> bool SparseMatrix<T>::operator==(const SparseMatrix &o) const {
  if (_rows != o._rows || _cols != o._cols || _nnz != o._nnz)
    return false;
  for (u64 i = 0; i < _nnz; i++) {
    if (_data[i]._r != o._data[i]._r || _data[i]._c != o._data[i]._c)
      return false;
    if (_data[i]._v < o._data[i]._v || o._data[i]._v < _data[i]._v)
      return false;
  }
  return true;
}

template <typename T> bool SparseMatrix<T>::operator!=(const SparseMatrix &o) const {
  return !(*this == o);
}

template <typename T>
bool SparseMatrix<T>::almostEqual(const SparseMatrix &o, double eps) const {
  if (_rows != o._rows || _cols != o._cols)
    throw Exception("almostEqual requires equal shapes");

  // walk the union of stored positions via merge
  u64 i = 0, j = 0;
  while (i < _nnz || j < o._nnz) {
    T mine = T(), theirs = T();
    bool bothSame = false;
    if (i >= _nnz) {
      theirs = o._data[j]._v;
      j++;
    } else if (j >= o._nnz) {
      mine = _data[i]._v;
      i++;
    } else if (o._data[j].before(_data[i]._r, _data[i]._c)) {
      theirs = o._data[j]._v;
      j++;
    } else if (_data[i].before(o._data[j]._r, o._data[j]._c)) {
      mine = _data[i]._v;
      i++;
    } else {
      mine = _data[i]._v;
      theirs = o._data[j]._v;
      bothSame = true;
    }
    if (bothSame) {
      i++;
      j++;
    }

    double diff = double(mine > theirs ? mine - theirs : theirs - mine);
    if (diff > eps)
      return false;
  }
  return true;
}

template <typename T> double SparseMatrix<T>::norm() const {
  double s = 0;
  for (u64 i = 0; i < _nnz; i++)
    s += double(_data[i]._v) * double(_data[i]._v);
  return sqrt(s);
}

template <typename T> u32 SparseMatrix<T>::rank() const {
  return toDense().rank();
}

template <typename T> double SparseMatrix<T>::rcond() const {
  return toDense().rcond();
}

template <typename T>
double SparseMatrix<T>::powerIteration(u32 maxIter, double eps) const {
  return toDense().powerIteration(maxIter, eps);
}

template <typename T>
SparseMatrix<T> SparseMatrix<T>::elementwiseMin(const SparseMatrix &o) const {
  if (o._rows != _rows || o._cols != _cols)
    throw Exception("elementwiseMin requires equal shapes");

  // implicit zero competes: a position stored by ONE side takes that
  // value only when it is NEGATIVE (a positive value's min with an
  // implicit 0 is 0, which stays implicit). Both stored: plain min.
  SparseMatrix out(_rows, _cols);
  u64 i = 0, j = 0;
  while (i < _nnz || j < o._nnz) {
    if (i >= _nnz) {
      if (o._data[j]._v < T())
        out.put(o._data[j]._r, o._data[j]._c, o._data[j]._v);
      j++;
    } else if (j >= o._nnz) {
      if (_data[i]._v < T())
        out.put(_data[i]._r, _data[i]._c, _data[i]._v);
      i++;
    } else if (o._data[j].before(_data[i]._r, _data[i]._c)) {
      if (o._data[j]._v < T())
        out.put(o._data[j]._r, o._data[j]._c, o._data[j]._v);
      j++;
    } else if (_data[i].before(o._data[j]._r, o._data[j]._c)) {
      if (_data[i]._v < T())
        out.put(_data[i]._r, _data[i]._c, _data[i]._v);
      i++;
    } else {
      T mn = _data[i]._v < o._data[j]._v ? _data[i]._v : o._data[j]._v;
      if (!(mn == T()))
        out.put(_data[i]._r, _data[i]._c, mn);
      i++;
      j++;
    }
  }
  return out;
}

template <typename T>
SparseMatrix<T> SparseMatrix<T>::elementwiseMax(const SparseMatrix &o) const {
  if (o._rows != _rows || o._cols != _cols)
    throw Exception("elementwiseMax requires equal shapes");

  // implicit zero competes: a position stored by ONE side takes that
  // value only when it is POSITIVE. Both stored: plain max, dropped
  // when it computes to zero.
  SparseMatrix out(_rows, _cols);
  u64 i = 0, j = 0;
  while (i < _nnz || j < o._nnz) {
    if (i >= _nnz) {
      if (!(o._data[j]._v < T()))
        out.put(o._data[j]._r, o._data[j]._c, o._data[j]._v);
      j++;
    } else if (j >= o._nnz) {
      if (!(_data[i]._v < T()))
        out.put(_data[i]._r, _data[i]._c, _data[i]._v);
      i++;
    } else if (o._data[j].before(_data[i]._r, _data[i]._c)) {
      if (!(o._data[j]._v < T()))
        out.put(o._data[j]._r, o._data[j]._c, o._data[j]._v);
      j++;
    } else if (_data[i].before(o._data[j]._r, o._data[j]._c)) {
      if (!(_data[i]._v < T()))
        out.put(_data[i]._r, _data[i]._c, _data[i]._v);
      i++;
    } else {
      T mx = _data[i]._v < o._data[j]._v ? o._data[j]._v : _data[i]._v;
      if (!(mx == T()))
        out.put(_data[i]._r, _data[i]._c, mx);
      i++;
      j++;
    }
  }
  return out;
}

template <typename T> void SparseMatrix<T>::apply(T (*f)(T)) {
  u64 w = 0;
  for (u64 i = 0; i < _nnz; i++) {
    T v = f(_data[i]._v);
    if (!(v == T()))
      _data[w++] = Entry(_data[i]._r, _data[i]._c, v);
  }
  _nnz = w;
}

template <typename T>
SparseMatrix<T> SparseMatrix<T>::applied(T (*f)(T)) const {
  SparseMatrix out(*this);
  out.apply(f);
  return out;
}

// Prints the shape, nnz, and every stored entry as "r,c: v" lines.
template <typename T>
std::ostream &operator<<(std::ostream &os, const SparseMatrix<T> &m) {
  os << "Sparse " << m.rows() << "x" << m.cols() << ", nnz=" << m.nnz()
     << "\n";
  m.for_each([&os](u32 r, u32 c, const T &v) {
    os << "  " << r << "," << c << ": " << v << "\n";
  });
  return os;
}

// Constant-time exchange (lets the std::swap idiom find the member).
template <typename T>
void swap(SparseMatrix<T> &a, SparseMatrix<T> &b) {
  a.swap(b);
}
