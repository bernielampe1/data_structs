#pragma once

#include <memory>
#include <ostream>

#include "Exception.h"
#include "Vec.h"
#include "types.h"

// Matrix<T>: a simple dense row-major matrix, not optimized.
//
// A rule-of-five implementation (see README.md): the destructor, copy
// and move construction, and copy and move assignment are all defined.
// Moves are noexcept and leave the source empty and reusable.
//
// operator+, -, *, / between matrices are ELEMENT-WISE (Hadamard
// products); dot() is the true matrix/matrix-vector product. All
// binary/compound matrix operators require equal shapes and throw
// Exception otherwise; dot() requires compatible shapes.
//
// operator[] and get() index the flat buffer unchecked (same contract
// as std::vector's operator[]); at(r, c) is bounds-checked and throws.
//
// eye(d) is a static d x d identity factory.
//
// Element requirements: numeric T (integers work for arithmetic but
// not for inverse/solve, which divide).
template <typename T> class Matrix {
private:
  T *_data;         // row-major order
  u32 _rows, _cols; // matrix dimensions

public:
  Matrix() : _data(0), _rows(0), _cols(0) {}

  Matrix(const u32 r, const u32 c) : _data(0), _rows(r), _cols(c) {
    init(r, c);
  }

  // r x c from a flat row-major array of r * c values.
  Matrix(const T m[], const u32 r, const u32 c)
      : _data(0), _rows(r), _cols(c) {
    std::unique_ptr<T[]> temp(new T[r * c]());
    for (u32 i = 0; i < r * c; i++)
      temp[i] = m[i];
    _data = temp.release();
  }

  // Deep copy: the new buffer is filled under an RAII guard, so a
  // throwing element copy leaks nothing.
  Matrix(const Matrix<T> &m) : _data(0), _rows(m._rows), _cols(m._cols) {
    std::unique_ptr<T[]> guard(new T[_rows * _cols]());
    for (u32 i = 0; i < _rows * _cols; i++)
      guard[i] = m._data[i];
    _data = guard.release();
  }

  // Steal m's buffer; m left empty and reusable.
  Matrix(Matrix<T> &&m) noexcept
      : _data(m._data), _rows(m._rows), _cols(m._cols) {
    m._data = 0;
    m._rows = m._cols = 0;
  }

  ~Matrix() { delete[] _data; }

  // Deep copy: the new buffer is built and filled before the old one is
  // freed, so a failed allocation leaves *this untouched.
  Matrix<T> &operator=(const Matrix<T> &m) {
    if (this == &m)
      return *this;

    std::unique_ptr<T[]> temp(new T[m._rows * m._cols]());
    for (u32 i = 0; i < m._rows * m._cols; i++)
      temp[i] = m._data[i];

    delete[] _data;
    _data = temp.release();
    _rows = m._rows;
    _cols = m._cols;

    return *this;
  }

  // Steal m's buffer (freeing ours, unconditionally).
  Matrix<T> &operator=(Matrix<T> &&m) noexcept {
    if (this != &m) {
      delete[] _data;
      _data = m._data;
      _rows = m._rows;
      _cols = m._cols;

      m._data = 0;
      m._rows = m._cols = 0;
    }

    return *this;
  }

  // Constant-time exchange of both buffers.
  void swap(Matrix<T> &m) {
    std::swap(_data, m._data);
    std::swap(_rows, m._rows);
    std::swap(_cols, m._cols);
  }

  // Reinitializes to r x c, every element v (default 0), releasing any
  // previous storage. The new buffer is allocated before the old is
  // freed, so a failed allocation leaves *this untouched.
  void init(const u32 r, const u32 c, const T v = 0) {
    std::unique_ptr<T[]> temp(new T[r * c]());

    delete[] _data;
    _data = temp.release();
    _rows = r;
    _cols = c;

    for (u32 i = 0; i < _rows * _cols; i++)
      _data[i] = v;
  }

  // Releases all memory; the matrix becomes empty and reusable.
  void clear() {
    delete[] _data;
    _rows = _cols = 0;
    _data = 0;
  }

  /* accessors */
  u32 rows() const { return (_rows); }

  u32 cols() const { return (_cols); }

  u32 size() const { return (_rows * _cols); } // element count

  bool empty() const { return (_rows == 0 || _cols == 0); }

  T &get(const u32 r, const u32 c) { return (_data[r * _cols + c]); } // unchecked

  const T &get(const u32 r, const u32 c) const { return (_data[r * _cols + c]); }

  T &operator[](const u32 i) { return (_data[i]); }             // unchecked

  const T &operator[](const u32 i) const { return (_data[i]); } // unchecked read

  // Bounds-checked access: throws Exception when (r, c) is outside.
  T &at(const u32 r, const u32 c) {
    if (r >= _rows || c >= _cols)
      throw Exception("Matrix::at: coordinates outside the matrix");
    return _data[r * _cols + c];
  }

  const T &at(const u32 r, const u32 c) const {
    if (r >= _rows || c >= _cols)
      throw Exception("Matrix::at: coordinates outside the matrix");
    return _data[r * _cols + c];
  }

  /* d x d identity */
  static Matrix<T> eye(const u32 d) {
    Matrix<T> m(d, d);
    for (u32 i = 0; i < d; i++)
      m._data[i * d + i] = T(1);
    return m;
  }

  /* Matrix scalar operations */
  Matrix<T> operator+(const T &c) const;
  Matrix<T> &operator+=(const T &c);

  Matrix<T> operator-(const T &c) const;
  Matrix<T> &operator-=(const T &c);

  Matrix<T> operator*(const T &c) const;
  Matrix<T> &operator*=(const T &c);

  Matrix<T> operator/(const T &c) const;
  Matrix<T> &operator/=(const T &c);

  /* Simple composition operators (ELEMENT-WISE, shape-checked) */
  Matrix<T> operator+(const Matrix<T> &m) const;
  Matrix<T> &operator+=(const Matrix<T> &m);

  Matrix<T> operator-(const Matrix<T> &m) const;
  Matrix<T> &operator-=(const Matrix<T> &m);

  Matrix<T> operator*(const Matrix<T> &m) const;
  Matrix<T> &operator*=(const Matrix<T> &m);

  Matrix<T> operator/(const Matrix<T> &m) const;
  Matrix<T> &operator/=(const Matrix<T> &m);

  /* More advanced composition operations */
  Matrix<T> transpose() const;

  Vec<T> diag() const;

  Vec<T> dot(const Vec<T> &v) const;

  Matrix<T> dot(const Matrix<T> &m) const;

  // perform LUP decomp
  Matrix<T> decompLUP(Vec<u32> &P) const;

  // return cofactor matrix
  Matrix<T> cofactor(const u32 &rp, const u32 &cp) const;

  // return adjoint matrix
  Matrix<T> adjoint() const;

  // compute determinant using cofactors
  double determinant_1() const;

  // compute determinant using LUP decomp
  double determinant_2() const;

  // solve using kramer's method
  Vec<T> solve_1(const Vec<T> &b) const;

  // solve using LUP decomp
  Vec<T> solve_2(const Vec<T> &b) const;

  // invert using adjoint matrix
  Matrix<T> inverse_1() const;

  // invert using LUP decomp
  Matrix<T> inverse_2() const;
};

// Prints the elements row by row, ", " separated, one newline per row,
// with no trailing separator. Written to os (the old version wrongly
// wrote to std::cout).
template<typename T>
std::ostream &operator<<(std::ostream &os, const Matrix<T> &m) {
  for (u32 r = 0; r < m.rows(); r++) {
    for (u32 c = 0; c < m.cols(); c++) {
      if (c > 0)
        os << ", ";
      os << m.get(r, c);
    }
    os << "\n";
  }

  return os;
}

// Constant-time exchange (lets the std::swap idiom find the member).
template<typename T> void swap(Matrix<T> &a, Matrix<T> &b) { a.swap(b); }

#include "Matrix.inl"
