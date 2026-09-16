// Tests for Matrix<T>. Prints one line per check and exits nonzero on
// the first failure. Build and run from tests/:
//   make matrixTest && ./matrixTest

#include <iostream>
#include <sstream>
#include <utility>

using namespace std;

#include "Matrix.h"

static int failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (cond) {                                                                \
      cout << "ok: " #cond << endl;                                            \
    } else {                                                                   \
      cout << "FAIL: " #cond << " (line " << __LINE__ << ")" << endl;          \
      failures++;                                                              \
    }                                                                          \
  } while (0)

template <typename A, typename B>
bool near(A a, B b, double eps = 1e-6) {
  return double(a > b ? a - b : b - a) <= eps;
}

// Deterministic PRNG so failures are reproducible.
static unsigned seed = 424242424u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Construction, dimensions, element state.
  {
    Matrix<float> dflt;
    CHECK(dflt.empty());
    CHECK(dflt.rows() == 0 && dflt.cols() == 0);

    float a[6] = {1, 2, 3, 4, 5, 6};
    Matrix<float> m(a, 2, 3); // 2 rows x 3 cols, row-major fill
    CHECK(m.rows() == 2 && m.cols() == 3);
    CHECK(m.size() == 6);
    CHECK(!m.empty());
    CHECK(m.get(0, 0) == 1 && m.get(0, 2) == 3);
    CHECK(m.get(1, 0) == 4 && m.get(1, 2) == 6); // row-major layout
    CHECK(m[0] == 1 && m[5] == 6);               // flat indexing

    Matrix<float> z(3, 2);
    bool zeroed = true;
    for (u32 i = 0; i < z.size(); i++)
      if (z[i] != 0)
        zeroed = false;
    CHECK(zeroed); // dimension ctor value-initializes

    Matrix<float> filled(2, 2);
    filled.init(2, 2, 7);
    CHECK(filled[0] == 7 && filled[3] == 7); // init(v) fills
  }

  // at() bounds checking.
  {
    float a[4] = {1, 2, 3, 4};
    Matrix<float> m(a, 2, 2);
    CHECK(m.at(1, 1) == 4);

    bool threw = false;
    try { m.at(2, 0); } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { m.at(0, 2); } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { m.at(2, 2); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // eye(): d x d identity (regression: the old eye() had a no-op body
  // and no return statement at all).
  {
    Matrix<float> e3 = Matrix<float>::eye(3);
    CHECK(e3.rows() == 3 && e3.cols() == 3);
    bool ok = true;
    for (u32 r = 0; r < 3; r++)
      for (u32 c = 0; c < 3; c++)
        if (e3.get(r, c) != (r == c ? 1.0f : 0.0f))
          ok = false;
    CHECK(ok);

    Matrix<float> e1 = Matrix<float>::eye(1);
    CHECK(e1.get(0, 0) == 1);

    Matrix<float> e0 = Matrix<float>::eye(0);
    CHECK(e0.empty());
  }

  // Scalar arithmetic: elementwise, operands untouched.
  {
    float a[4] = {1, 2, 3, 4};
    Matrix<float> m(a, 2, 2);

    Matrix<float> s = m + 10.0f;
    CHECK(s[0] == 11 && s[3] == 14);
    Matrix<float> sub = m - 1.0f;
    CHECK(sub[0] == 0 && sub[3] == 3);
    Matrix<float> mul = m * 2.0f;
    CHECK(mul[0] == 2 && mul[3] == 8);
    Matrix<float> div = m / 4.0f;
    CHECK(near(div[0], 0.25f) && near(div[3], 1.0f));

    Matrix<float> io(m);
    io += 1.0f; io -= 2.0f; io *= 3.0f; io /= 4.0f;
    // ((m + 1) - 2) * 3 / 4 = (m - 1) * 3/4
    CHECK(near(io[0], 0.0f) && near(io[1], 0.75f) &&
          near(io[2], 1.5f) && near(io[3], 2.25f));
    CHECK(m[0] == 1); // operand unchanged
  }

  // Element-wise matrix arithmetic: shape-checked, exact.
  {
    float a[4] = {1, 2, 3, 4};
    float b[4] = {10, 20, 30, 40};
    Matrix<float> m(a, 2, 2), n(b, 2, 2);

    Matrix<float> s = m + n;
    CHECK(s[0] == 11 && s[1] == 22 && s[2] == 33 && s[3] == 44);
    Matrix<float> d = n - m;
    CHECK(d[0] == 9 && d[3] == 36);

    // NOTE: operator* / operator/ are ELEMENT-WISE (Hadamard); dot() is
    // the true product.
    Matrix<float> p = m * n;
    CHECK(p[0] == 10 && p[3] == 160);
    Matrix<float> q = n / m;
    CHECK(q[0] == 10 && q[3] == 10);

    Matrix<float> io(n);
    io += m; io -= m; io *= m; io /= m;
    CHECK(io[0] == 10 && io[3] == 40); // round trip: unchanged
    CHECK(n[0] == 10 && m[0] == 1);    // operands untouched
  }

  // Shape mismatches throw for every element-wise operator (regression:
  // the old versions silently read out of bounds).
  {
    Matrix<float> a(2, 3), b(3, 2);
    bool threw = false;
    try { a + b; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { a - b; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { a * b; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { a / b; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { a += b; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { a -= b; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { a *= b; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { a /= b; } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // transpose() and diag().
  {
    float a[6] = {1, 2, 3, 4, 5, 6};
    Matrix<float> m(a, 2, 3);
    Matrix<float> t = m.transpose();
    CHECK(t.rows() == 3 && t.cols() == 2);
    CHECK(t.get(0, 0) == 1 && t.get(0, 1) == 4);
    CHECK(t.get(2, 1) == 6);
    CHECK(t.transpose().get(0, 0) == 1); // double transpose is identity
    bool same = true;
    for (u32 i = 0; i < 6; i++)
      same = same && (t.transpose()[i] == m[i]);
    CHECK(same);

    Matrix<float> sq(a, 3, 2); // not square
    bool threw = false;
    try { sq.diag(); } catch (Exception &) { threw = true; }
    CHECK(threw); // diag of non-square throws

    Matrix<float> id = Matrix<float>::eye(3);
    Vec<float> d = id.diag();
    CHECK(d.len() == 3 && d[0] == 1 && d[2] == 1);
  }

  // dot(): matrix-vector and matrix-matrix (the true products). The
  // old code accumulated the first row/column into an UNINITIALIZED
  // double; any correct expected value catches it.
  {
    float am[6] = {1, 2, 3, 4, 5, 6};
    Matrix<float> m(am, 2, 3);

    float vv[3] = {1, 1, 1};
    Vec<float> v(vv, 3);
    Vec<float> mv = m.dot(v);
    CHECK(mv.len() == 2);
    CHECK(mv[0] == 6 && mv[1] == 15); // row sums; not garbage
    // n: 3x2 matrix for the matmul below
    float nm[6] = {1, 2, 3, 4, 5, 6};
    Matrix<float> n(nm, 3, 2);
    Matrix<float> mm = m.dot(n);
    CHECK(mm.rows() == 2 && mm.cols() == 2);
    // [1 2 3; 4 5 6] x [1 2; 3 4; 5 6] = [22 28; 49 64]
    CHECK(mm.get(0, 0) == 22 && mm.get(0, 1) == 28);
    CHECK(mm.get(1, 0) == 49 && mm.get(1, 1) == 64);

    // identity is the multiplicative identity of dot()
    Matrix<float> id = Matrix<float>::eye(2);
    Matrix<float> one = Matrix<float>::eye(1);
    Matrix<float> left = id.dot(m); // 2x2 * 2x3 = 2x3
    bool same = true;
    for (u32 i = 0; i < 6; i++)
      same = same && (left[i] == m[i]);
    CHECK(same);

    bool threw = false;
    try {
      Matrix<float> wrong(2, 2);
      m.dot(wrong); // 2x3 dot 2x2: _cols != wrong._rows
    } catch (Exception &) { threw = true; }
    CHECK(threw);

    threw = false;
    try { m.dot(Vec<float>(2)); } catch (Exception &) { threw = true; }
    CHECK(threw); // wrong vector length
  }

  // cofactor() removes the pivot row and column.
  {
    float a[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    Matrix<float> m(a, 3, 3);

    Matrix<float> c00 = m.cofactor(0, 0);
    CHECK(c00.rows() == 2 && c00.cols() == 2);
    CHECK(c00.get(0, 0) == 5 && c00.get(0, 1) == 6);
    CHECK(c00.get(1, 0) == 8 && c00.get(1, 1) == 9);

    Matrix<float> c12 = m.cofactor(1, 2);
    CHECK(c12.get(0, 0) == 1 && c12.get(0, 1) == 2);
    CHECK(c12.get(1, 0) == 7 && c12.get(1, 1) == 8);

    bool threw = false;
    try { m.cofactor(3, 0); } catch (Exception &) { threw = true; }
    CHECK(threw); // pivot row out of range

    threw = false;
    try { m.cofactor(0, 5); } catch (Exception &) { threw = true; }
    CHECK(threw); // pivot col out of range
  }

  // determinant via cofactors and via LUP must agree on known values.
  {
    float a[4] = {1, 2, 3, 4}; // det = -2
    Matrix<float> m2(a, 2, 2);
    CHECK(near(m2.determinant_1(), -2.0));
    CHECK(near(m2.determinant_2(), -2.0));

    float b[9] = {6, 1, 1, 4, -2, 5, 2, 8, 7}; // det = -306
    Matrix<float> m3(b, 3, 3);
    CHECK(near(m3.determinant_1(), -306.0));
  }

  // determinant_1 vs determinant_2 on a random batch of matrices.
  {
    bool allOk = true;
    for (int trial = 0; trial < 50; trial++) {
      const u32 d = 2 + rnd() % 4; // 2..5
      float buf[25];
      for (u32 i = 0; i < d * d; i++)
        buf[i] = float(int(rnd() % 200) - 100) / 4.0f; // -25..25 quarters
      Matrix<float> m(buf, d, d);

      // push each diagonal magnitude past the sum of its row's
      // off-diagonal absolutes: strictly row-diagonal-dominant with
      // the diagonal's original sign, so LUP never finds a zero pivot.
      for (u32 i = 0; i < d; i++) {
        float sgn = m.get(i, i) < 0 ? -1.0f : 1.0f;
        float rest = 0;
        for (u32 j = 0; j < d; j++)
          if (j != i)
            rest += ABS(float(m.get(i, j)));
        m.get(i, i) = sgn * (rest + 25.0f);
      }

      double d1 = m.determinant_1();
      double d2 = m.determinant_2();
      if (ABS(d1 - d2) > 1e-5 * (1.0 + ABS(d1)))
        allOk = false;
    }
    CHECK(allOk);
  }

  // inverse_1 and inverse_2 both satisfy A * A^-1 = I.
  {
    float b[9] = {4, 7, 2, 3, 6, 1, 3, 5, 9};
    Matrix<float> m(b, 3, 3);
    Matrix<float> id = Matrix<float>::eye(3);

    Matrix<float> i1 = m.inverse_1();
    Matrix<float> p1 = m.dot(i1);
    bool ok = true;
    for (u32 r = 0; r < 3 && ok; r++)
      for (u32 c = 0; c < 3; c++)
        if (!near(p1.get(r, c), id.get(r, c), 1e-3))
          ok = false;
    CHECK(ok); // inverse_1 obeys A * inverse = I

    Matrix<float> i2 = m.inverse_2();
    Matrix<float> p2 = m.dot(i2);
    ok = true;
    for (u32 r = 0; r < 3 && ok; r++)
      for (u32 c = 0; c < 3; c++)
        if (!near(p2.get(r, c), id.get(r, c), 1e-3))
          ok = false;
    CHECK(ok); // inverse_2 obeys A * inverse = I

    // inverse_1 and inverse_2 agree with each other
    ok = true;
    for (u32 i = 0; i < 9; i++)
      if (!near(i1[i], i2[i], 1e-3))
        ok = false;
    CHECK(ok);

    // singular matrix throws for both
    float sing[9] = {1, 2, 3, 2, 4, 6, 3, 6, 9};
    Matrix<float> sm(sing, 3, 3);
    bool threw = false;
    try { sm.inverse_1(); } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { sm.inverse_2(); } catch (Exception &) { threw = true; }
    CHECK(threw);

    // non-square (2x3) throws
    float nsbuf[6] = {1, 2, 3, 4, 5, 6};
    Matrix<float> ns(nsbuf, 2, 3);
    threw = false;
    try { ns.inverse_2(); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // solve_1 and solve_2 both recover x with A * x = b.
  {
    float am[4] = {3, 1, 1, 2}; // det = 5, invertible
    Matrix<float> m(am, 2, 2);

    float bv[2] = {9, 8};
    Vec<float> b(bv, 2);

    Vec<float> x1 = m.solve_1(b);
    Vec<float> ref = m.dot(x1);
    CHECK(near(ref[0], 9.0) && near(ref[1], 8.0));

    Vec<float> x2 = m.solve_2(b);
    ref = m.dot(x2);
    CHECK(near(ref[0], 9.0) && near(ref[1], 8.0));

    CHECK(near(x1[0], x2[0]) && near(x1[1], x2[1])); // same solution

    // exact known solution: 3x + y = 9, x + 2y = 8 => x = 2, y = 3
    CHECK(near(x1[0], 2.0) && near(x1[1], 3.0));

    // larger system, 3x3
    float cm[9] = {2, 0, 0, 0, 4, 0, 0, 0, 8};
    Matrix<float> c(cm, 3, 3);
    float cv[3] = {2, 8, 24};
    Vec<float> v(cv, 3);
    Vec<float> cx = c.solve_2(v);
    CHECK(near(cx[0], 1.0) && near(cx[1], 2.0) && near(cx[2], 3.0));
    Vec<float> cx1 = c.solve_1(v);
    CHECK(near(cx1[0], 1.0) && near(cx1[1], 2.0) && near(cx1[2], 3.0));

    // incompatible systems throw
    bool threw = false;
    try { m.solve_1(Vec<float>(3)); } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { m.solve_2(Vec<float>(3)); } catch (Exception &) { threw = true; }
    CHECK(threw);

    // singular system throws (old solve_1 divided det 0 by nothing)
    float sing[9] = {1, 2, 3, 2, 4, 6, 3, 6, 9};
    Matrix<float> sm(sing, 3, 3);
    float sv[3] = {1, 1, 1};
    threw = false;
    try { sm.solve_1(Vec<float>(sv, 3)); } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { sm.solve_2(Vec<float>(sv, 3)); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // decompLUP: pivoting actually permutes; P*A reconstructed equals LU.
  {
    // A matrix that REQUIRES pivoting: a zero in the pivot position.
    float am[4] = {0, 1, 1, 1};
    Matrix<float> m(am, 2, 2);
    Vec<u32> P;
    Matrix<float> lup = m.decompLUP(P);
    // decomposed without dividing by the zero pivot: the run must have
    // pivoted (P records the swap count) rather than crashed.
    CHECK(P[2] - 2 == 1); // exactly one pivot swap
    // reconstruction: rows of Lup = L + U (diag 1 implied for L)
    // L = [1 0; lup[1][0] 1], U = [lup[0][0] lup[0][1]; 0 lup[1][1]]
    // P*A = [1 1; 0 1] (rows swapped)
    bool ok = near(lup.get(0, 0), 1.0) && near(lup.get(0, 1), 1.0) &&
              near(lup.get(1, 0), 0.0) && near(lup.get(1, 1), 1.0);
    CHECK(ok); // L+U of the pivoted system
  }

  // Adjoint: for 2x2 [[a b], [c d]] adj = [[d -b], [-c a]].
  {
    float a[4] = {1, 2, 3, 4};
    Matrix<float> m(a, 2, 2);
    Matrix<float> adj = m.adjoint();
    CHECK(adj.get(0, 0) == 4);  // d
    CHECK(adj.get(0, 1) == -2); // -b  (transposed position!)
    CHECK(adj.get(1, 0) == -3); // -c
    CHECK(adj.get(1, 1) == 1);  // a

    bool threw = false;
    try { Matrix<float>(a, 2, 3).adjoint(); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // Copy construction is deep and independent.
  {
    float a[4] = {1, 2, 3, 4};
    Matrix<float> orig(a, 2, 2);
    Matrix<float> copy(orig);
    CHECK(copy.rows() == 2 && copy.cols() == 2);
    bool same = true;
    for (u32 i = 0; i < 4; i++)
      same = same && (copy[i] == orig[i]);
    CHECK(same);

    copy[0] = 99;
    copy.init(5, 5);
    CHECK(copy.rows() == 5);
    CHECK(orig.rows() == 2); // reinit of the copy left orig alone
    CHECK(orig[0] == 1);

    Matrix<float> assigned(9, 9);
    assigned = orig;
    CHECK(assigned.rows() == 2 && assigned.cols() == 2);
    CHECK(assigned[0] == 1 && assigned[3] == 4);

    Matrix<float> &alias = orig;
    orig = alias; // self-assignment
    CHECK(orig.rows() == 2 && orig[0] == 1);
  }

  // Move construction steals; source is empty and reusable.
  {
    float a[6] = {1, 2, 3, 4, 5, 6};
    Matrix<float> src(a, 2, 3);
    Matrix<float> dst(std::move(src));
    CHECK(dst.rows() == 2 && dst.cols() == 3);
    CHECK(dst[0] == 1 && dst[5] == 6);

    CHECK(src.empty());
    CHECK(src.rows() == 0 && src.cols() == 0);

    src.init(2, 2, 1); // reusable
    CHECK(src[0] == 1 && src[3] == 1);
  }

  // Move assignment frees destination and steals the source.
  {
    float a[4] = {7, 8, 9, 10};
    Matrix<float> src(a, 2, 2);
    Matrix<float> dst(6, 6);
    dst[0] = 55;
    dst = std::move(src);
    CHECK(dst.rows() == 2 && dst.cols() == 2);
    CHECK(dst[0] == 7 && dst[3] == 10);

    CHECK(src.empty());
    src.init(3, 1); // reusable
    CHECK(src.rows() == 3 && src.cols() == 1);
  }

  // Self-move is a no-op.
  {
    float a[4] = {1, 2, 3, 4};
    Matrix<float> m(a, 2, 2);
    Matrix<float> &alias = m;
    m = std::move(alias);
    CHECK(m.rows() == 2 && m.cols() == 2);
    CHECK(m[0] == 1 && m[3] == 4);
  }

  // swap exchanges buffers and dimensions.
  {
    Matrix<float> s1(2, 2), s2(3, 4);
    s1[0] = 1;
    s2[0] = 2;
    s1.swap(s2);
    CHECK(s1.rows() == 3 && s1.cols() == 4);
    CHECK(s2.rows() == 2 && s2.cols() == 2);
    CHECK(s1[0] == 2 && s2[0] == 1);

    // free-swap overload goes through the member
    swap(s1, s2);
    CHECK(s1.rows() == 2 && s1.cols() == 2);
    CHECK(s2.rows() == 3 && s2.cols() == 4);
    CHECK(s1[0] == 1 && s2[0] == 2);
  }

  // clear() empties and stays reusable.
  {
    float a[4] = {1, 2, 3, 4};
    Matrix<float> m(a, 2, 2);
    m.clear();
    CHECK(m.empty());
    CHECK(m.rows() == 0 && m.cols() == 0 && m.size() == 0);

    m.init(2, 2, 5); // reuse
    CHECK(m[0] == 5 && m[3] == 5);
    m.clear();
    m.clear(); // idempotent on empty
    CHECK(m.empty());
  }

  // init() on a populated matrix re-fills with the fill value.
  {
    float a[4] = {1, 2, 3, 4};
    Matrix<float> m(a, 2, 2);
    m.init(3, 2, 9);
    CHECK(m.rows() == 3 && m.cols() == 2);
    bool all = true;
    for (u32 i = 0; i < m.size(); i++)
      if (m[i] != 9)
        all = false;
    CHECK(all);
  }

  // operator<< writes to the passed stream (regression: the old
  // operator<< wrote to std::cout for the row separators) and does not
  // mutate.
  {
    float a[4] = {1, 2, 3, 4};
    Matrix<float> m(a, 2, 2);
    ostringstream os;
    os << m;
    CHECK(os.str() == "1, 2\n3, 4\n");
    CHECK(m.rows() == 2);
  }

  // Randomized stress: random-shape matmul against a naive triple loop.
  {
    bool allOk = true;
    for (int trial = 0; trial < 40; trial++) {
      const u32 R = 1 + rnd() % 6, C = 1 + rnd() % 6, K = 1 + rnd() % 6;
      float am[36], bm[36];
      for (u32 i = 0; i < R * C; i++)
        am[i] = float(rnd() % 20 - 10);
      for (u32 i = 0; i < K * C; i++)
        bm[i] = float(rnd() % 20 - 10);

      Matrix<float> A(am, R, C);
      Matrix<float> B(bm, K, C);
      Matrix<float> AB = A.dot(B.transpose()); // R x K

      // naive reference
      for (u32 r = 0; r < R; r++)
        for (u32 k = 0; k < K; k++) {
          float acc = 0;
          for (u32 c = 0; c < C; c++)
            acc += A.get(r, c) * B.get(k, c);
          if (!near(AB.get(r, k), acc, 1e-3))
            allOk = false;
        }
    }
    CHECK(allOk);
  }

  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
