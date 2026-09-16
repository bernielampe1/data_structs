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
  if (m._rows != _rows || m._cols != _cols)
    throw Exception("matrix addition requires equal shapes");

  Matrix<T> temp(_rows, _cols);
  for (u32 i = 0; i < _rows * _cols; i++)
    temp._data[i] = _data[i] + m._data[i];
  return temp;
}

template <typename T> Matrix<T> &Matrix<T>::operator+=(const Matrix<T> &m) {
  if (m._rows != _rows || m._cols != _cols)
    throw Exception("matrix addition requires equal shapes");

  for (u32 i = 0; i < _rows * _cols; i++)
    _data[i] += m._data[i];
  return *this;
}

template <typename T> Matrix<T> Matrix<T>::operator-(const Matrix<T> &m) const {
  if (m._rows != _rows || m._cols != _cols)
    throw Exception("matrix subtraction requires equal shapes");

  Matrix<T> temp(_rows, _cols);
  for (u32 i = 0; i < _rows * _cols; i++)
    temp._data[i] = _data[i] - m._data[i];
  return temp;
}

template <typename T> Matrix<T> &Matrix<T>::operator-=(const Matrix<T> &m) {
  if (m._rows != _rows || m._cols != _cols)
    throw Exception("matrix subtraction requires equal shapes");

  for (u32 i = 0; i < _rows * _cols; i++)
    _data[i] -= m._data[i];
  return *this;
}

// Element-wise product (same shape required). For the true matrix
// product use dot().
template <typename T> Matrix<T> Matrix<T>::operator*(const Matrix<T> &m) const {
  if (m._rows != _rows || m._cols != _cols)
    throw Exception("element-wise matrix multiplication requires equal shapes");

  Matrix<T> temp(_rows, _cols);
  for (u32 i = 0; i < _rows * _cols; i++)
    temp._data[i] = _data[i] * m._data[i];
  return temp;
}

template <typename T> Matrix<T> &Matrix<T>::operator*=(const Matrix<T> &m) {
  if (m._rows != _rows || m._cols != _cols)
    throw Exception("element-wise matrix multiplication requires equal shapes");

  for (u32 i = 0; i < _rows * _cols; i++)
    _data[i] *= m._data[i];
  return *this;
}

template <typename T> Matrix<T> Matrix<T>::operator/(const Matrix<T> &m) const {
  if (m._rows != _rows || m._cols != _cols)
    throw Exception("element-wise matrix division requires equal shapes");

  Matrix<T> temp(_rows, _cols);
  for (u32 i = 0; i < _rows * _cols; i++)
    temp._data[i] = _data[i] / m._data[i];
  return temp;
}

template <typename T> Matrix<T> &Matrix<T>::operator/=(const Matrix<T> &m) {
  if (m._rows != _rows || m._cols != _cols)
    throw Exception("element-wise matrix division requires equal shapes");

  for (u32 i = 0; i < _rows * _cols; i++)
    _data[i] /= m._data[i];
  return *this;
}

template <typename T> Vec<T> Matrix<T>::dot(const Vec<T> &v) const {
  if (v.len() != _cols)
    throw Exception("matrix-vector are not compatible");

  Vec<T> temp(_rows);
  for (u32 r = 0; r < _rows; r++) {
    double s = 0; // was uninitialized on entry (first row accumulated garbage)
    for (u32 c = 0; c < _cols; c++)
      s += double(v[c]) * double(_data[r * _cols + c]);
    temp[r] = (T)s;
  }

  return temp;
}

template <typename T> Matrix<T> Matrix<T>::dot(const Matrix<T> &m) const {
  if (_cols != m._rows)
    throw Exception("matrices are not compatible");

  Matrix<T> temp(_rows, m._cols);
  for (u32 r = 0; r < _rows; r++) {
    for (u32 c = 0; c < m._cols; c++) {
      double s = 0; // was uninitialized on the first column
      for (u32 i = 0; i < _cols; i++)
        s += double(_data[r * _cols + i]) * double(m._data[i * m._cols + c]);
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

/* ---- structural ---- */

template <typename T> T Matrix<T>::trace() const {
  if (_rows != _cols)
    throw Exception("trace of non-square matrix");

  T s = T(0);
  for (u32 i = 0; i < _cols; i++)
    s += _data[i * _cols + i];
  return s;
}

template <typename T>
Matrix<T> Matrix<T>::submatrix(const u32 r0, const u32 c0, const u32 h,
                               const u32 w) const {
  if (h == 0 || w == 0 || r0 + h > _rows || c0 + w > _cols)
    throw Exception("submatrix outside the matrix");

  Matrix<T> temp(h, w);
  for (u32 r = 0; r < h; r++)
    for (u32 c = 0; c < w; c++)
      temp._data[r * w + c] = _data[(r0 + r) * _cols + (c0 + c)];
  return temp;
}

template <typename T> Vec<T> Matrix<T>::row(const u32 r) const {
  if (r >= _rows)
    throw Exception("row index outside the matrix");

  Vec<T> v(_cols);
  for (u32 c = 0; c < _cols; c++)
    v[c] = _data[r * _cols + c];
  return v;
}

template <typename T> Vec<T> Matrix<T>::col(const u32 c) const {
  if (c >= _cols)
    throw Exception("column index outside the matrix");

  Vec<T> v(_rows);
  for (u32 r = 0; r < _rows; r++)
    v[r] = _data[r * _cols + c];
  return v;
}

template <typename T> void Matrix<T>::setRow(const u32 r, const Vec<T> &v) {
  if (r >= _rows)
    throw Exception("row index outside the matrix");
  if (v.len() != _cols)
    throw Exception("setRow vector length mismatch");

  for (u32 c = 0; c < _cols; c++)
    _data[r * _cols + c] = v[c];
}

template <typename T> void Matrix<T>::setCol(const u32 c, const Vec<T> &v) {
  if (c >= _cols)
    throw Exception("column index outside the matrix");
  if (v.len() != _rows)
    throw Exception("setCol vector length mismatch");

  for (u32 r = 0; r < _rows; r++)
    _data[r * _cols + c] = v[r];
}

/* ---- predicates ---- */

template <typename T> bool Matrix<T>::isSquare() const {
  return _rows == _cols;
}

template <typename T> bool Matrix<T>::isSymmetric() const {
  return isSquare() && *this == this->transpose();
}

template <typename T> bool Matrix<T>::isDiagonal() const {
  if (!isSquare())
    return false;

  for (u32 r = 0; r < _rows; r++)
    for (u32 c = 0; c < _cols; c++)
      if (r != c && _data[r * _cols + c] != T(0))
        return false;
  return true;
}

template <typename T> bool Matrix<T>::isIdentity() const {
  return isSquare() && *this == eye(_rows);
}

// Exact element equality. Uses memcmp, which is only valid for plain
// value-semantic element types (arrays of doubles, ints, floats) --
// exactly what Matrix<T> is for. T with padding or exotic comparisons
// must overload this.
template <typename T> bool Matrix<T>::operator==(const Matrix<T> &m) const {
  return _rows == m._rows && _cols == m._cols &&
         memcmp(_data, m._data, _rows * _cols * sizeof(T)) == 0;
}

template <typename T> bool Matrix<T>::operator!=(const Matrix<T> &m) const {
  return !(*this == m);
}

template <typename T>
bool Matrix<T>::almostEqual(const Matrix<T> &m, double eps) const {
  if (m._rows != _rows || m._cols != _cols)
    throw Exception("almostEqual requires equal shapes");

  for (u32 i = 0; i < _rows * _cols; i++)
    if (double(_data[i] > m._data[i] ? _data[i] - m._data[i]
                                     : m._data[i] - _data[i]) > eps)
      return false;
  return true;
}

/* ---- numerical ---- */

// The Frobenius norm: sqrt of the sum of squared entries, accumulated
// in double.
template <typename T> double Matrix<T>::norm() const {
  double s = 0;
  for (u32 i = 0; i < _rows * _cols; i++)
    s += double(_data[i]) * double(_data[i]);
  return sqrt(s);
}

// Rank via LUP: the count of pivots that survived the degeneracy
// threshold. Rank-deficient matrices do NOT throw -- the pivots below
// tol are simply not counted.
template <typename T> u32 Matrix<T>::rank() const {
  if (_rows == 0 || _cols == 0)
    return 0;

  // Gaussian elimination on a double working copy so the tolerance
  // logic never runs in low-precision element arithmetic.
  double work[256]; // supports up to 16x16; larger matrices throw
  if (_rows * _cols > 256)
    throw Exception("rank: matrix larger than 16x16 not supported");

  for (u32 i = 0; i < _rows * _cols; i++)
    work[i] = double(_data[i]);

  u32 r = 0; // filled pivot rows
  for (u32 c = 0; c < _cols && r < _rows; c++) {
    // pivot search
    u32 p = r;
    for (u32 i = r + 1; i < _rows; i++)
      if (ABS(work[i * _cols + c]) > ABS(work[p * _cols + c]))
        p = i;

    if (ABS(work[p * _cols + c]) < tol)
      continue; // column exhausted

    // swap rows r and p
    if (p != r)
      for (u32 j = 0; j < _cols; j++) {
        double t = work[r * _cols + j];
        work[r * _cols + j] = work[p * _cols + j];
        work[p * _cols + j] = t;
      }

    // eliminate below
    for (u32 i = r + 1; i < _rows; i++) {
      double f = work[i * _cols + c] / work[r * _cols + c];
      for (u32 j = c; j < _cols; j++)
        work[i * _cols + j] -= f * work[r * _cols + j];
    }
    r++;
  }

  return r;
}

// Estimate of 1/cond(A) in the infinity norm: ||A||inf * ||A^-1||inf is
// the condition number; this computes its reciprocal so that a value
// near 0 flags near-singularity (the convention of LAPACK's rcond).
// Requires a square, invertible matrix.
template <typename T> double Matrix<T>::rcond() const {
  if (_rows != _cols)
    throw Exception("rcond of non-square matrix");

  // infinity norm of A: max absolute row sum
  double normA = 0;
  for (u32 r = 0; r < _rows; r++) {
    double s = 0;
    for (u32 c = 0; c < _cols; c++)
      s += ABS(double(_data[r * _cols + c]));
    if (s > normA)
      normA = s;
  }

  // infinity norm of A^-1 via the LUP-based inverse
  Matrix<T> inv = inverse_2();
  double normInv = 0;
  for (u32 r = 0; r < _rows; r++) {
    double s = 0;
    for (u32 c = 0; c < _cols; c++)
      s += ABS(double(inv._data[r * _cols + c]));
    if (s > normInv)
      normInv = s;
  }

  if (normInv < tol)
    throw Exception("rcond: inverse unavailable");

  return 1.0 / (normA * normInv);
}

// The dominant eigenvalue (largest magnitude) by power iteration.
// Decomposes the matrix as A^(k+1) x = A^(k) x with a random-ish start;
// the Rayleigh quotient xAx/xx is invariant under the sign flips that
// plague naive power iteration for negative eigenvalues. eps bounds
// the change in the estimate; maxIter caps the sweep count.
template <typename T>
double Matrix<T>::powerIteration(u32 maxIter, double eps) const {
  if (_rows != _cols)
    throw Exception("power iteration on non-square matrix");

  const u32 n = _rows;

  // start vector: all ones (a common choice; for matrices with the
  // dominant eigenvector orthogonal to it the iteration still leaves
  // the subspace via floating point noise).
  Vec<double> x(n);
  for (u32 i = 0; i < n; i++)
    x[i] = 1.0;

  // y = A x, then x <- y/||y||. With x kept unit length the Rayleigh
  // quotient is simply y . x; it is invariant under x -> -x (A x
  // flips with x), so it converges to the SIGNED dominant eigenvalue
  // even when that eigenvalue is negative.
  double lambda = 0;
  for (u32 it = 0; it < maxIter; it++) {
    Vec<double> y(n);
    for (u32 r = 0; r < n; r++) {
      double s = 0;
      for (u32 c = 0; c < n; c++)
        s += double(_data[r * _cols + c]) * x[c];
      y[r] = s;
    }

    double ny = 0;
    for (u32 i = 0; i < n; i++)
      ny += y[i] * y[i];
    ny = sqrt(ny);
    if (ny < 1e-300)
      return 0; // x collapsed: eigenvalue is 0

    double newLambda = 0;
    for (u32 i = 0; i < n; i++)
      newLambda += y[i] * x[i]; // Rayleigh quotient, signed

    for (u32 i = 0; i < n; i++)
      x[i] = y[i] / ny;

    if (ABS(newLambda - lambda) < eps) { // |Δλ| small: converged
      lambda = newLambda;
      break;
    }
    lambda = newLambda;
  }

  return lambda;
}

/* ---- iteration / elementwise ---- */

// STL-style iterators over the flat row-major buffer.
template <typename T> T *Matrix<T>::begin() { return _data; }

template <typename T> T *Matrix<T>::end() { return _data + _rows * _cols; }

template <typename T> const T *Matrix<T>::begin() const { return _data; }

template <typename T> const T *Matrix<T>::end() const {
  return _data + _rows * _cols;
}

template <typename T> void Matrix<T>::apply(T (*f)(T)) {
  for (u32 i = 0; i < _rows * _cols; i++)
    _data[i] = f(_data[i]);
}

template <typename T> Matrix<T> Matrix<T>::applied(T (*f)(T)) const {
  Matrix<T> temp(_rows, _cols);
  for (u32 i = 0; i < _rows * _cols; i++)
    temp._data[i] = f(_data[i]);
  return temp;
}

template <typename T>
Matrix<T> Matrix<T>::elementwiseMin(const Matrix<T> &m) const {
  if (m._rows != _rows || m._cols != _cols)
    throw Exception("elementwiseMin requires equal shapes");

  Matrix<T> temp(_rows, _cols);
  for (u32 i = 0; i < _rows * _cols; i++)
    temp._data[i] = _data[i] < m._data[i] ? _data[i] : m._data[i];
  return temp;
}

template <typename T>
Matrix<T> Matrix<T>::elementwiseMax(const Matrix<T> &m) const {
  if (m._rows != _rows || m._cols != _cols)
    throw Exception("elementwiseMax requires equal shapes");

  Matrix<T> temp(_rows, _cols);
  for (u32 i = 0; i < _rows * _cols; i++)
    temp._data[i] = _data[i] < m._data[i] ? m._data[i] : _data[i];
  return temp;
}

// The (rp, cp) cofactor: *this with row rp and column cp removed.
template <typename T>
Matrix<T> Matrix<T>::cofactor(const u32 &rp, const u32 &cp) const {
  if (_rows <= 1 || _cols <= 1)
    throw Exception("cannot create cofactor matrix");
  if (rp >= _rows || cp >= _cols)
    throw Exception("cofactor pivot outside the matrix");

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
    for(i = 0; i <= s32(_rows); i++) P[i] = u32(i);

    for(i = 0; i < s32(_rows); i++) {
        maxA = 0;
        imax = i;

        for(k = i; k < s32(_rows); k++) {
            if ((absA = ABS(double(temp._data[k * _cols + i]))) > maxA) {
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
            for(s32 c = 0; c < s32(_cols); c++) {
                T t = temp._data[i * _cols + c];
                temp._data[i * _cols + c] = temp._data[imax * _cols + c];
                temp._data[imax * _cols + c] = t;
            }

            // count pivot
            P[_rows]++;
        }

        for(j = i + 1; j < s32(_rows); j++) {
            temp._data[j * _cols + i] /= temp._data[i * _cols + i];

            for(k = i + 1; k < s32(_rows); k++)
                temp._data[j * _cols + k] -= temp._data[j * _cols + i] *
                                                                temp._data[i * _cols + k];
        }
    }

    return temp;
}

// The adjugate: adj[i][j] = cofactor(j, i) * (-1)^(i+j).
//
// Sign convention: the cofactor of position (r, c) is (-1)^(r + c) --
// never 1 for odd sums (the old adjoint() had the sign backwards, which
// inverse_1() then doubled up with a compensating * -1).
template <typename T> Matrix<T> Matrix<T>::adjoint() const {
  if (_rows != _cols)
    throw Exception("adjoint of non-square matrix");

  Matrix<T> adj(_rows, _cols);
  for (u32 r = 0; r < _rows; r++) {
    for (u32 c = 0; c < _cols; c++) {
      Matrix<T> cof = cofactor(r, c);
      s32 sign = ((r + c) % 2) ? -1 : 1;
      adj._data[c * _cols + r] = T(sign) * cof.determinant_1();
    }
  }

  return adj;
}

template <typename T> double Matrix<T>::determinant_1() const {
  if (_rows != _cols)
    throw Exception("cannot compute determinant of non-square matrix");

  if (_rows == 1)
    return double(_data[0]);

  double det = 0;
  s32 sign = 1;
  for (u32 f = 0; f < _cols; f++) {
    Matrix<T> temp = cofactor(0, f);
    det += sign * double(_data[f]) * temp.determinant_1();
    sign = -sign;
  }

  return det;
}

template <typename T> double Matrix<T>::determinant_2() const {
  if (_rows != _cols)
    throw Exception("cannot compute determinant of non-square matrix");

  Vec<u32> P;
  Matrix<T> lup = decompLUP(P);

  double det = double(lup._data[0]);
  for (u32 i = 1; i < _rows; i++)
    det *= double(lup._data[i * lup._cols + i]);

  if ((P[_rows] - _rows) % 2 != 0)
    det *= -1;

  return det;
}

template <typename T> Matrix<T> Matrix<T>::inverse_1() const {
  if (_rows != _cols)
    throw Exception("cannot compute inverse of non-square matrix");

  double det = determinant_1();
  if (ABS(det) < tol)
    throw Exception("determinant is zero for inverse operation");

  // Cramer: inverse = adjoint / det. (The old version divided the
  // adjoint by det AND multiplied by -1, which canceled the adjoint's
  // flipped sign instead of scaling the inverse.)
  Matrix<T> inv = adjoint();
  for (u32 i = 0; i < _rows * _cols; i++)
    inv._data[i] = T(double(inv._data[i]) / det);

  return inv;
}

template <typename T> Vec<T> Matrix<T>::solve_1(const Vec<T> &b) const {
  // solve Ax = b with kramer's rule
  if (_rows != b.len())
    throw Exception("the matrix-vector system is not compatible");

  if (_rows != _cols)
    throw Exception("cannot solve for non-square matrix");

  double detA = determinant_1();
  if (ABS(detA) < tol)
    throw Exception("system is singular for solve_1");

  // init sol.
  Vec<T> x(_cols);

  // use cramers rule x_i = det(A_i) / det(A); the replaced column is
  // restored before the next substitution.
  Matrix<T> temp = *this;
  for (u32 i = 0; i < _cols; i++) {
    for (u32 r = 0; r < _rows; r++)
      temp._data[r * _cols + i] = b[r]; // new column

    x[i] = T(temp.determinant_1() / detA);

    for (u32 r = 0; r < _rows; r++)
      temp._data[r * _cols + i] = _data[r * _cols + i]; // restore
  }

  return x;
}

template <typename T> Matrix<T> Matrix<T>::inverse_2() const {
  if (_rows != _cols)
    throw Exception("cannot compute inverse of non-square matrix");

  Vec<u32> P;
  Matrix<T> lup = decompLUP(P);
  Matrix<T> inv(_rows, _cols);

  // Solve A * X = I column by column via forward/back substitution on
  // the decomposed system; column j of the identity has a 1 in row j.
  for (s32 j = 0; j < s32(_cols); j++) {
    for (s32 i = 0; i < s32(_rows); i++) {
      inv._data[i * _cols + j] = (P[i] == u32(j)) ? T(1) : T(0);

      for (s32 k = 0; k < i; k++)
        inv._data[i * _cols + j] -= lup._data[i * _cols + k] * inv._data[k * _cols + j];
    }

    for (s32 i = s32(_rows) - 1; i >= 0; i--) {
      for (s32 k = i + 1; k < s32(_rows); k++)
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
    throw Exception("cannot solve for non-square matrix");

  Vec<u32> P;
  Matrix<T> lup = decompLUP(P);

  Vec<T> x(b.len());
  for (u32 i = 0; i < _rows; i++) {
    x[i] = b[P[i]];

    for (u32 k = 0; k < i; k++)
      x[i] -= lup._data[i * _cols + k] * x[k];
  }

  for (s32 i = s32(_rows) - 1; i >= 0; i--) {
    for (s32 k = i + 1; k < s32(_rows); k++)
      x[i] -= lup._data[i * _cols + k] * x[k];

    x[i] /= lup._data[i * _cols + i];
  }

  return x;
}
