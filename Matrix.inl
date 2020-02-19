template <typename T> Matrix<T> Matrix<T>::operator+(const T &c) const {
  Matrix<T> temp(_rows, _cols);
  for (u32 i = 0; i < _rows * _cols; i++)
    temp._data[i] = _data[i] + c;
  return temp;
}

template <typename T> Matrix<T> &Matrix<T>::operator+=(const T &c) {
  for (u32 i = 0; i < _rows * _cols; i++)
    _data[i] += c;
  return *this;
}

template <typename T> Matrix<T> Matrix<T>::operator-(const T &c) const {
  Matrix<T> temp(_rows, _cols);
  for (u32 i = 0; i < _rows * _cols; i++)
    temp._data[i] = _data[i] - c;
  return temp;
}

template <typename T> Matrix<T> &Matrix<T>::operator-=(const T &c) {
  for (u32 i = 0; i < _rows * _cols; i++)
    _data[i] -= c;
  return *this;
}

template <typename T> Matrix<T> Matrix<T>::operator*(const T &c) const {
  Matrix<T> temp(_rows, _cols);
  for (u32 i = 0; i < _rows * _cols; i++)
    temp._data[i] = _data[i] * c;
  return temp;
}

template <typename T> Matrix<T> &Matrix<T>::operator*=(const T &c) {
  for (u32 i = 0; i < _rows * _cols; i++)
    _data[i] *= c;
  return *this;
}

template <typename T> Matrix<T> Matrix<T>::operator/(const T &c) const {
  Matrix<T> temp(_rows, _cols);
  for (u32 i = 0; i < _rows * _cols; i++)
    temp._data[i] = _data[i] / c;
  return temp;
}

template <typename T> Matrix<T> &Matrix<T>::operator/=(const T &c) {
  for (u32 i = 0; i < _rows * _cols; i++)
    _data[i] /= c;
  return *this;
}

template <typename T> Matrix<T> Matrix<T>::operator+(const Matrix<T> &m) const {
  Matrix<T> temp(m._rows, m._cols);
  for (u32 i = 0; i < m._rows * m._cols; i++)
    temp._data[i] = _data[i] + m._data[i];
  return temp;
}

template <typename T> Matrix<T> &Matrix<T>::operator+=(const Matrix<T> &m) {
  for (u32 i = 0; i < m._rows * m._cols; i++)
    _data[i] += m._data[i];
  return *this;
}

template <typename T> Matrix<T> Matrix<T>::operator-(const Matrix<T> &m) const {
  Matrix<T> temp(m._rows, m._cols);
  for (u32 i = 0; i < m._rows * m._cols; i++)
    temp._data[i] = _data[i] - m._data[i];
  return temp;
}

template <typename T> Matrix<T> &Matrix<T>::operator-=(const Matrix<T> &m) {
  for (u32 i = 0; i < m._rows * m._cols; i++)
    _data[i] -= m._data[i];
  return *this;
}

template <typename T> Matrix<T> Matrix<T>::operator*(const Matrix<T> &m) const {
  Matrix<T> temp(m._rows, m._cols);
  for (u32 i = 0; i < m._rows * m._cols; i++)
    temp._data[i] = _data[i] * m._data[i];
  return temp;
}

template <typename T> Matrix<T> &Matrix<T>::operator*=(const Matrix<T> &m) {
  for (u32 i = 0; i < m._rows * m._cols; i++)
    _data[i] *= m._data[i];
  return *this;
}

template <typename T> Matrix<T> Matrix<T>::operator/(const Matrix<T> &m) const {
  Matrix<T> temp(m._rows, m._cols);
  for (u32 i = 0; i < m._rows * m._cols; i++)
    temp._data[i] = _data[i] / m._data[i];
  return temp;
}

template <typename T> Matrix<T> &Matrix<T>::operator/=(const Matrix<T> &m) {
  for (u32 i = 0; i < m._rows * m._cols; i++)
    _data[i] /= m._data[i];
  return *this;
}

template <typename T> Vec<T> Matrix<T>::dot(const Vec<T> &v) const {
  if (v.len() != _cols)
    throw Exception("matrix-vector are not compatible");

  double s;
  Vec<T> temp(_rows);
  for (u32 r = 0; r < _rows; r++, s = 0) {
    for (u32 c = 0; c < _cols; c++)
      s += v[c] * _data[r * _cols + c];
    temp[r] = (T)s;
  }

  return temp;
}

template <typename T> Matrix<T> Matrix<T>::dot(const Matrix<T> &m) const {
  if (_cols != m._rows)
    throw Exception("matrices are not compatible");

  double s;
  Matrix<T> temp(_rows, m._cols);
  for (u32 r = 0; r < _rows; r++) {
    for (u32 c = 0; c < m._cols; c++, s = 0) {
      for (u32 i = 0; i < _cols; i++) {
        s += _data[r * _cols + i] * m._data[i * m._cols + c];
      }
      temp._data[r * m._cols + c] = (T)s;
    }
  }

  return temp;
}

template <typename T> Matrix<T> Matrix<T>::transpose() const {
  Matrix<T> temp(_cols, _rows);
  for (u32 r = 0; r < _rows; r++) {
    for (u32 c = 0; c < _cols; c++) {
      temp._data[c * _rows + r] = _data[r * _cols + c];
    }
  }
  return temp;
}

template <typename T> Vec<T> Matrix<T>::diag() const {
  if (_rows != _cols)
    throw Exception("matrix is not square");

  Vec<T> v(_rows);
  for (u32 i = 0; i < _cols; i++)
    v[i] = _data[i * _cols + i];
  return v;
}

template <typename T>
Matrix<T> Matrix<T>::cofactor(const u32 &rp, const u32 &cp) const {
  if (_rows <= 1 || _cols <= 1)
    throw Exception("cannot create cofactor matrix");

  u32 i = 0;
  u32 j = 0;
  Matrix<T> cof(_rows - 1, _cols - 1);
  for (u32 r = 0; r < _rows; r++) {
    for (u32 c = 0; c < _cols; c++) {
      if (r != rp && c != cp) {
        cof._data[i * cof._cols + j++] = _data[r * _cols + c];
        if (j == cof._cols) {
          j = 0;
          i++;
        }
      }
    }
  }

  return cof;
}

/* Returns both L-E and U matrix as A = (L-E)+U such as P*A = L*U
 * The permutation matrix is not stored as a matrix, but in vector P of size N+1
 * contains column indexes where permutation matrix has "1" s.t. P[N] = S+N and 
 * the det(P) = (-1)^S. */
template<typename T>
Matrix<T> Matrix<T>::decompLUP(Vec<u32> &P) const {
    s32 i, j, k, imax;
    double maxA, absA;

    if (_rows != _cols)
      throw Exception("cannot perform LU decomp on non-square matrix");

    // create copy
    Matrix<T> temp = *this;

    P.init(_rows+1);
    for(i = 0; i <= _rows; i++) P[i] = i;

    for(i = 0; i < _rows; i++) {
        maxA = 0;
        imax = i;

        for(k = i; k < _rows; k++) {
            if ((absA = ABS(temp._data[k * _cols + i])) > maxA) {
                maxA = absA;
                imax = k;
            }
        }

        if (maxA < tol)
            throw Exception("matrix is degenerate");

        if (imax != i) {
            // pivoting P
            j = P[i];
            P[i] = P[imax];
            P[imax] = j;

            // pivot rows
            for(int c = 0; c < _cols; c++) {
                // swap
                T t = temp._data[i * _cols + c];
                temp._data[i * _cols + c] = temp._data[imax * _cols + c];;
                temp._data[imax * _cols + c] = t;
            }

            // count pivot
            P[_rows]++;
        }

        for(j = i + 1; j < _rows; j++) {
            temp._data[j * _cols + i] /= temp._data[i * _cols + i];

            for(k = i + 1; k < _rows; k++)
                temp._data[j * _cols + k] -= temp._data[j * _cols + i] *
                                                                temp._data[i * _cols + k];
        }
    }

    return temp;
}

template <typename T> Matrix<T> Matrix<T>::adjoint() const {
  s32 sign = 1;
  Matrix<T> adj(_rows, _cols);
  for (u32 r = 0; r < _rows; r++) {
    for (u32 c = 0; c < _cols; c++) {
      Matrix<T> cof = cofactor(r, c);
      sign = ((r + c) % 2) ? 1 : -1;
      adj._data[c * _cols + r] = sign * cof.determinant_1();
    }
  }

  return adj;
}

template <typename T> double Matrix<T>::determinant_1() const {
  if (_rows != _cols)
    throw Exception("cannot compute determinant of non-square matrix");

  double det = 0;
  s32 sign = 1;
  if (_rows == 1) return _data[0];

  for(u32 f = 0; f < _cols; f++) {
    Matrix<T> temp = cofactor(0, f);
    det += sign * _data[0 * _cols + f] * temp.determinant_1();
    sign = -sign;
  }

  return det;
}

template <typename T> Matrix<T> Matrix<T>::inverse_1() const {
  if (_rows != _cols)
    throw Exception("cannot compute inverse of non-square matrix");

  T det = determinant_1();
  if (abs(det) < tol)
    throw Exception("determinant is zero for inverse operation");

  return (adjoint() /= det) * -1;
}

template <typename T> Vec<T> Matrix<T>::solve_1(const Vec<T> &b) const {
  // solve Ax = b with kramer's rule
  if (_rows != b.len())
    throw Exception("the matrix-vector system is not compatible");

  if (_rows != _cols)
    throw Exception("cannot solve for non-square matrix");

  // init sol.
  Vec<T> x(_cols);

  // use cramers rule x_i = det(A_i) / det(A)
  Matrix<T> temp = *this;
  for (u32 i = 0; i < _cols; i++) {
    for (u32 r = 0; r < _rows; r++)
      temp._data[r * _cols + i] = b[r]; // new column
    x[i] = temp.determinant_1();          // compute det ration
    for (u32 r = 0; r < _rows; r++)
      temp._data[r * _cols + i] = _data[r * _cols + i];
  }
  x /= determinant_1();

  return x;
}

template <typename T> double Matrix<T>::determinant_2() const {
  if (_rows != _cols)
    throw Exception("cannot compute determinant of non-square matrix");

  Vec<u32> P;
  Matrix<T> lup = decompLUP(P);

  double det = lup._data[0];
  for(u32 i = 1; i < _rows; i++) det *= lup._data[i * lup._cols + i];

  if ((P[_rows] - _rows) % 2 != 0) det *= -1;

  return det;
}

template <typename T> Matrix<T> Matrix<T>::inverse_2() const {
  if (_rows != _cols)
    throw Exception("cannot compute inverse of non-square matrix");

  Vec<u32> P;
  Matrix<T> lup = decompLUP(P);
  Matrix<T> inv(_rows, _cols);

  for(s32 j = 0; j < _rows; j++) {
    for(s32 i = 0; i < _cols; i++) { 
      inv._data[i * _cols + j] = (P[i] == j) ? 1 : 0;

      for(s32 k = 0; k < i; k++)
        inv._data[i * _cols + j] -= lup._data[i * _cols + k] * inv._data[k * _cols + j];
    }

    for(s32 i = _rows - 1; i >= 0; i--) {
      for(s32 k = i + 1; k < _rows; k++)
          inv._data[i * _cols + j] -= lup._data[i * _cols + k] * inv._data[k * _cols + j];

      inv._data[i * _cols + j] /= lup._data[i * _cols + i];
    }
  }

  return inv;
}

template <typename T> Vec<T> Matrix<T>::solve_2(const Vec<T> &b) const {
  if (_rows != b.len())
    throw Exception("the matrix-vector system is not compatible");

  if (_rows != _cols)
    throw Exception("cannot compute inverse of non-square matrix");

  Vec<u32> P;
  Matrix<T> lup = decompLUP(P);

  Vec<T> x(b.len());
  for(u32 i = 0; i < _rows; i++) {
    x[i] = b[P[i]];

    for (u32 k = 0; k < i; k++)
        x[i] -= lup._data[i * _cols + k] * x[k];
  }

  for(s32 i = _rows - 1; i >= 0; i--) {
    for(s32 k = i + 1; k < _rows; k++)
      x[i] -= lup._data[i * _cols + k] * x[k];

    x[i] /= lup._data[i * _cols + i];
  }

  return x;
}
