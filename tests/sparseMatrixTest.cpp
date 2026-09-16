// Tests for SparseMatrix<T>. Prints one line per check and exits
// nonzero on the first failure. Build and run from tests/:
//   make sparseMatrixTest && ./sparseMatrixTest

#include "SparseMatrix.h"
#include <iostream>
#include <sstream>
#include <utility>

using namespace std;

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

// apply()/applied() test helpers.
static int absVal(int x) { return x < 0 ? -x : x; }
static int zeroIfFive(int x) { return x == 5 ? 0 : x; }

// Deterministic PRNG so failures are reproducible.
static unsigned seed = 112358132u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Construction: empty, shaped, dense-array.
  {
    SparseMatrix<float> dflt; // 0x0
    CHECK(dflt.rows() == 0 && dflt.cols() == 0);
    CHECK(dflt.nnz() == 0);
    CHECK(dflt.empty());

    SparseMatrix<float> shaped(4, 7); // all-zero 4x7
    CHECK(shaped.rows() == 4 && shaped.cols() == 7);
    CHECK(shaped.nnz() == 0);
    CHECK(!shaped.empty()); // a shaped zero matrix is not "empty"

    float dense[12] = {0, 1, 0, 2, 0, 0, 3, 0, 0, 0, 4, 0};
    SparseMatrix<float> fromD(dense, 3, 4);
    CHECK(fromD.rows() == 3 && fromD.cols() == 4);
    CHECK(fromD.nnz() == 4); // zeros skipped
    CHECK(fromD.get(0, 1) == 1 && fromD.get(1, 2) == 3);
    CHECK(fromD.get(0, 0) == 0); // absent reads as T()
  }

  // set/get/erase: coordinate storage in sorted order.
  {
    SparseMatrix<int> m(5, 5);
    m.set(2, 3, 10);
    m.set(0, 1, 5);
    m.set(4, 0, 99);
    m.set(2, 3, 20); // replace
    CHECK(m.nnz() == 3);

    // row-major sorted layout, regardless of insertion order
    int first = -1, second = -1, third = -1;
    m.for_each([&](u32 r, u32 c, int v) {
      if (first < 0)
        first = v;
      else if (second < 0)
        second = v;
      else
        third = v;
    });
    CHECK(first == 5 && second == 20 && third == 99); // sorted positions

    CHECK(m.get(2, 3) == 20); // replaced value

    CHECK(m.erase(2, 3));
    CHECK(!m.erase(2, 3)); // already gone
    CHECK(m.nnz() == 2);
    CHECK(m.get(2, 3) == 0); // back to the implicit zero

    m.set(2, 3, 0); // zero CAN be stored (explicit zeros documented)
    CHECK(m.nnz() == 3);
  }

  // Bounds checking on all coordinate-taking methods.
  {
    SparseMatrix<int> m(3, 3);
    bool threw = false;
    try { m.get(3, 0); } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { m.get(0, 3); } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { m.set(3, 3, 1); } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { m.erase(0, 9); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // Shape checks: element-wise ops.
  {
    float d1[6] = {1, 0, 0, 2, 0, 3};
    SparseMatrix<float> a(d1, 2, 3);
    float d2[9] = {1, 0, 1, 0, 1, 0, 1, 0, 1};
    SparseMatrix<float> b(d2, 3, 3);

    bool threw = false;
    try { a + b; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { a - b; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { a * b; } catch (Exception &) { threw = true; }
    CHECK(threw);

    SparseMatrix<float> incompat(2, 3);
    threw = false;
    try { a.dot(incompat); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // Addition: union of stored positions, values combined.
  {
    float d1[6] = {1, 0, 0, 2, 0, 3};
    float d2[6] = {0, 4, 5, 2, 0, 0};
    SparseMatrix<float> a(d1, 2, 3), b(d2, 2, 3);

    SparseMatrix<float> s = a + b;
    // dense reference: [1 4 5; 4 0 3]
    CHECK(s.get(0, 0) == 1 && s.get(0, 1) == 4 && s.get(0, 2) == 5);
    CHECK(s.get(1, 0) == 4 && s.get(1, 1) == 0 && s.get(1, 2) == 3);
    CHECK(s.nnz() == 5);

    // operand shapes/values unchanged
    CHECK(a.get(1, 0) == 2 && b.get(0, 1) == 4);

    // a + zero-matrix is a (with a's entries)
    SparseMatrix<float> z(2, 3);
    CHECK((a + z) == a);
  }

  // Subtraction and negation symmetry.
  {
    float d1[4] = {5, 0, 0, 7};
    float d2[4] = {3, 1, 0, 4};
    SparseMatrix<float> a(d1, 2, 2), b(d2, 2, 2);

    SparseMatrix<float> d = a - b;
    CHECK(d.get(0, 0) == 2 && d.get(0, 1) == -1);
    CHECK(d.get(1, 1) == 3);

    // a - a == zero matrix (empty storage)
    SparseMatrix<float> self = a - a;
    CHECK(self.nnz() == 0);
  }

  // Element-wise product: intersection only.
  {
    float d1[6] = {1, 0, 2, 0, 3, 4};
    float d2[6] = {5, 9, 0, 0, 6, 7};
    SparseMatrix<float> a(d1, 2, 3), b(d2, 2, 3);

    SparseMatrix<float> p = a * b;
    // both-nonzero positions: (0,0)=1*5, (1,1)=3*6, (1,2)=4*7
    CHECK(p.get(0, 0) == 5);
    CHECK(p.get(1, 1) == 18);
    CHECK(p.get(1, 2) == 28);
    CHECK(p.nnz() == 3);
    CHECK(p.get(0, 1) == 0); // (0,1): a stores 0 -> product skips it
  }

  // Scalar arithmetic on stored entries.
  {
    float d1[6] = {1, 0, 0, 2, 0, 3};
    SparseMatrix<float> a(d1, 2, 3);

    SparseMatrix<float> s = a + 10.0f;
    CHECK(s.get(0, 0) == 11 && s.get(1, 0) == 12 && s.get(1, 2) == 13);
    CHECK(s.nnz() == 3); // stored count unchanged by a scalar add

    SparseMatrix<float> sub = a - 1.0f;
    CHECK(sub.get(0, 0) == 0 && sub.get(1, 0) == 1 && sub.get(1, 2) == 2);
    CHECK(sub.nnz() == 2); // (0,0) hit 0 and normalized away

    SparseMatrix<float> mul = a * 3.0f;
    CHECK(mul.get(0, 0) == 3 && mul.get(1, 2) == 9);

    SparseMatrix<float> div = a / 2.0f;
    CHECK(near(div.get(0, 0), 0.5) && near(div.get(1, 2), 1.5));

    CHECK(a.get(0, 0) == 1); // source untouched
  }

  // dot(): true matrix product against the dense Matrix reference.
  {
    float d1[9] = {1, 0, 2, 0, 0, 3, 4, 0, 5};
    SparseMatrix<float> a(d1, 3, 3);
    float d2[9] = {2, 0, 1, 1, 1, 0, 0, 3, 1};
    SparseMatrix<float> b(d2, 3, 3);

    SparseMatrix<float> ab = a.dot(b);

    // dense reference
    Matrix<float> da = a.toDense(), db = b.toDense();
    Matrix<float> ref = da.dot(db);
    Matrix<float> got = ab.toDense();

    bool same = got.rows() == ref.rows() && got.cols() == ref.cols();
    for (u32 r = 0; r < ref.rows() && same; r++)
      for (u32 c = 0; c < ref.cols(); c++)
        if (!near(got.get(r, c), ref.get(r, c)))
          same = false;
    CHECK(same);

    CHECK(ab.rows() == 3 && ab.cols() == 3);
  }

  // dot() with rectangular shapes: (2x3) x (3x2) = 2x2.
  {
    float d1[6] = {1, 0, 2, 0, 1, 0};
    SparseMatrix<float> a(d1, 2, 3);
    float d2[6] = {1, 2, 3, 0, 0, 4};
    SparseMatrix<float> b(d2, 3, 2);

    SparseMatrix<float> ab = a.dot(b);
    CHECK(ab.rows() == 2 && ab.cols() == 2);

    // reference: row0 = [1*1+2*(3? no: B col-major)] -- recomputed:
    //   A row0 = [1, 0, 2]; B rows (as dot targets): B(0,.)=[1,2],
    //   B(1,.)=[3,0], B(2,.)=[0,4]
    //   out(0,0) = 1*1 + 0*3 + 2*0 = 1
    //   out(0,1) = 1*2 + 0*0 + 2*4 = 10
    //   A row1 = [0, 1, 0]:
    //   out(1,0) = 0*1 + 1*3 + 0*0 = 3
    //   out(1,1) = 0*2 + 1*0 + 0*4 = 0 (normalized away)
    CHECK(ab.get(0, 0) == 1 && ab.get(0, 1) == 10);
    CHECK(ab.get(1, 0) == 3 && ab.get(1, 1) == 0);
  }

  // transpose().
  {
    float d1[6] = {1, 0, 2, 3, 0, 4};
    SparseMatrix<float> a(d1, 2, 3);

    SparseMatrix<float> t = a.transpose();
    CHECK(t.rows() == 3 && t.cols() == 2);
    CHECK(t.get(0, 0) == 1); // (0,0) stays
    CHECK(t.get(0, 1) == 3); // (1,0) -> (0,1)
    CHECK(t.get(2, 0) == 2); // (0,2) -> (2,0)
    CHECK(t.get(2, 1) == 4); // (1,2) -> (2,1)
    CHECK(t.nnz() == 4);
    CHECK(t.transpose() == a); // involution
  }

  // fromDense/toDense round trip; explicit-zero handling.
  {
    Matrix<int> dm(3, 3);
    dm.init(3, 3, 0);
    dm.get(1, 1) = 5;
    dm.get(2, 0) = 7;

    SparseMatrix<int> sp = SparseMatrix<int>::fromDense(dm);
    CHECK(sp.nnz() == 2);
    CHECK(sp.get(1, 1) == 5 && sp.get(2, 0) == 7);

    Matrix<int> back = sp.toDense();
    CHECK(back == dm); // Matrix's own equality

    // a stored zero round-trips as STORED via toDense (value equality)
    SparseMatrix<int> z(2, 2);
    z.set(0, 0, 0); // explicit zero
    CHECK(z.nnz() == 1);
    Matrix<int> dz = z.toDense();
    CHECK(dz.get(0, 0) == 0);
  }

  // clear() and reuse; shape survives clear().
  {
    SparseMatrix<int> m(4, 4);
    m.set(1, 1, 9);
    m.clear();
    CHECK(m.nnz() == 0);
    CHECK(m.rows() == 4 && m.cols() == 4); // shape kept
    CHECK(m.get(1, 1) == 0);

    m.set(0, 0, 3); // reusable
    CHECK(m.nnz() == 1 && m.get(0, 0) == 3);
  }

  // operator== / !=.
  {
    float d1[4] = {1, 0, 0, 2};
    SparseMatrix<float> a(d1, 2, 2), b(d1, 2, 2);
    CHECK(a == b);

    SparseMatrix<float> c(2, 2);
    c.set(0, 0, 1); // same nonzeros but 2 missing
    CHECK(a != c);

    SparseMatrix<float> sh(3, 2);
    CHECK(a != sh); // shape mismatch is inequality, not an error
  }

  // Copy construction is deep and independent.
  {
    SparseMatrix<int> a(3, 3);
    a.set(0, 2, 7);
    a.set(2, 0, 8);

    SparseMatrix<int> b(a);
    CHECK(b.nnz() == 2);
    CHECK(b.get(0, 2) == 7 && b.get(2, 0) == 8);
    CHECK(b.rows() == 3 && b.cols() == 3);

    b.set(1, 1, 5);
    b.erase(0, 2);
    CHECK(b.nnz() == 2);
    CHECK(a.nnz() == 2);     // a untouched
    CHECK(a.get(0, 2) == 7); // a still has its entries
  }

  // Copy assignment replaces; self-assign no-op.
  {
    SparseMatrix<int> a(2, 2);
    a.set(0, 0, 1);
    SparseMatrix<int> b(5, 5);
    b.set(3, 3, 9);
    b = a;
    CHECK(b.rows() == 2 && b.cols() == 2); // shape replaced
    CHECK(b.nnz() == 1);
    CHECK(b.get(0, 0) == 1);

    SparseMatrix<int> &alias = a;
    a = alias; // self-assignment
    CHECK(a.nnz() == 1 && a.get(0, 0) == 1);
  }

  // Move construction steals; source empty and reusable.
  {
    SparseMatrix<int> src(3, 3);
    src.set(1, 1, 6);
    SparseMatrix<int> dst(std::move(src));
    CHECK(dst.nnz() == 1 && dst.get(1, 1) == 6);
    CHECK(dst.rows() == 3);

    CHECK(src.rows() == 0 && src.cols() == 0); // fully reset
    CHECK(src.nnz() == 0);
  }

  // (continuation of move-construction block above, corrected) -- the
  // moved-from matrix is 0x0, so set() throws as documented.
  {
    SparseMatrix<int> src(3, 3);
    src.set(1, 1, 6);
    SparseMatrix<int> dst(std::move(src));
    CHECK(src.rows() == 0 && src.cols() == 0);

    bool threw = false;
    try { src.set(0, 0, 1); } catch (Exception &) { threw = true; }
    CHECK(threw); // 0x0 shape: any coordinate is out of bounds
  }

  // Moved-from reuse: re-shape via assignment, then set works.
  {
    SparseMatrix<int> src(3, 3);
    src.set(2, 2, 4);
    SparseMatrix<int> freshShape(5, 5);
    freshShape = std::move(src); // move assignment
    CHECK(freshShape.rows() == 3 && freshShape.get(2, 2) == 4);

    SparseMatrix<int> reshape(7, 7);
    src = reshape;                 // re-shape by copy assign
    src.set(6, 6, 2);
    CHECK(src.get(6, 6) == 2);
  }

  // Self-move is a no-op.
  {
    SparseMatrix<int> m(2, 2);
    m.set(0, 1, 3);
    SparseMatrix<int> &alias = m;
    m = std::move(alias);
    CHECK(m.rows() == 2 && m.get(0, 1) == 3);
  }

  // swap exchanges everything.
  {
    SparseMatrix<int> s1(2, 3), s2(4, 4);
    s1.set(0, 0, 1);
    s2.set(3, 3, 2);
    s1.swap(s2);
    CHECK(s1.rows() == 4 && s1.cols() == 4 && s1.get(3, 3) == 2);
    CHECK(s2.rows() == 2 && s2.cols() == 3 && s2.get(0, 0) == 1);

    swap(s1, s2); // free overload
    CHECK(s1.rows() == 2 && s2.rows() == 4);
  }

  // for_each with for_each-driven recomputation of the dense matrix.
  {
    SparseMatrix<int> m(3, 3);
    m.set(0, 1, 4);
    m.set(2, 2, 6);

    Matrix<int> rec(3, 3);
    rec.init(3, 3, 0);
    m.for_each([&rec](u32 r, u32 c, int v) { rec.get(r, c) = v; });
    CHECK(rec.get(0, 1) == 4 && rec.get(2, 2) == 6);
    CHECK(rec.get(0, 0) == 0 && rec.get(1, 1) == 0); // implicit zeros
  }

  // operator<< smoke test.
  {
    SparseMatrix<int> m(2, 2);
    m.set(1, 0, 3);
    ostringstream oss;
    oss << m;
    CHECK(oss.str().find("Sparse 2x2") != string::npos);
    CHECK(oss.str().find("1,0: 3") != string::npos);
    CHECK(m.nnz() == 1); // printing did not consume
  }

  // Randomized stress: random sparse ops against a dense Matrix model.
  {
    bool allOk = true;
    for (int trial = 0; trial < 30; trial++) {
      const u32 R = 1 + rnd() % 8, C = 1 + rnd() % 8;
      SparseMatrix<double> sp(R, C);
      Matrix<double> refM(R, C);
      refM.init(R, C, 0.0);

      // random sets
      for (int op = 0; op < 60; op++) {
        u32 r = rnd() % R, c = rnd() % C;
        double v = double(int(rnd() % 20) - 10);
        sp.set(r, c, v);
        refM.get(r, c) = v;
      }

      // random erases
      for (int op = 0; op < 20; op++) {
        u32 r = rnd() % R, c = rnd() % C;
        if (sp.erase(r, c))
          refM.get(r, c) = 0.0;
      }

      // element-wise agreement
      for (u32 r = 0; r < R && allOk; r++)
        for (u32 c = 0; c < C; c++)
          if (!near(sp.get(r, c), refM.get(r, c)))
            allOk = false;

      // nnz must match the dense nonzeros (stored zeros count as nnz)
      u32 denseNonzeros = 0;
      for (u32 r = 0; r < R; r++)
        for (u32 c = 0; c < C; c++)
          if (refM.get(r, c) != 0.0)
            denseNonzeros++;
      // nnz = denseNonzeros + (explicit zeros stored via set(.., 0)), so
      // just require nnz >= denseNonzeros
      if (sp.nnz() < denseNonzeros)
        allOk = false;

      // dot against dense reference on random compatible shapes
      const u32 K = 1 + rnd() % 8;
      SparseMatrix<double> other(K, C);
      Matrix<double> refO(K, C);
      refO.init(K, C, 0.0);
      for (int op = 0; op < 30; op++) {
        u32 r = rnd() % K, c = rnd() % C;
        double v = double(int(rnd() % 10) - 5);
        other.set(r, c, v);
        refO.get(r, c) = v;
      }

      SparseMatrix<double> prod = sp.dot(other.transpose());
      Matrix<double> refM2 = refM.dot(refO.transpose());
      if (prod.rows() != R || prod.cols() != K)
        allOk = false;
      for (u32 r = 0; r < R && allOk; r++)
        for (u32 c = 0; c < K; c++)
          if (!near(prod.get(r, c), refM2.get(r, c), 1e-9))
            allOk = false;
    }
    CHECK(allOk);
  }

  // ---- Matrix.h feature parity ----

  // trace(): diagonal sum over stored entries only.
  {
    SparseMatrix<int> m(3, 3);
    m.set(0, 0, 1);
    m.set(1, 1, 2);
    m.set(2, 2, 3);
    m.set(0, 2, 50); // off-diagonal entry must not pollute
    CHECK(m.trace() == 6);

    SparseMatrix<int> ns(2, 3); // non-square
    bool threw = false;
    try { ns.trace(); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // submatrix(): sparse block copy.
  {
    SparseMatrix<int> m(4, 4);
    m.set(0, 0, 1);
    m.set(1, 2, 2); // outside the block below
    m.set(2, 2, 3);
    m.set(3, 3, 4);

    SparseMatrix<int> blk = m.submatrix(2, 1, 2, 2); // rows 2-3, cols 1-2
    CHECK(blk.rows() == 2 && blk.cols() == 2);
    CHECK(blk.get(0, 1) == 3); // was (2,2) -> (0,1)
    CHECK(blk.get(1, 1) == 0); // (3,3) maps to (1,2): outside cols 1-2!
    CHECK(blk.nnz() == 1);

    bool threw = false;
    try { m.submatrix(3, 0, 2, 1); } catch (Exception &) { threw = true; }
    CHECK(threw); // runs past the bottom
  }

  // row()/col() dense extraction with implicit zeros.
  {
    SparseMatrix<int> m(3, 4);
    m.set(1, 0, 9);
    m.set(1, 3, 4);

    Vec<int> r1 = m.row(1);
    CHECK(r1.len() == 4);
    CHECK(r1[0] == 9 && r1[1] == 0 && r1[3] == 4);

    Vec<int> c0 = m.col(0);
    CHECK(c0.len() == 3);
    CHECK(c0[0] == 0 && c0[1] == 9 && c0[2] == 0);

    bool threw = false;
    try { m.row(3); } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { m.col(4); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // setRow()/setCol(): replace with nonzeros-only compression.
  {
    SparseMatrix<int> m(3, 3);
    m.set(1, 0, 9);
    m.set(1, 2, 9);
    m.set(2, 0, 5);

    Vec<int> nr(3);
    nr[0] = 0; nr[1] = 7; nr[2] = 0; // a single nonzero at col 1
    m.setRow(1, nr);
    CHECK(m.nnz() == 2); // old (1,0),(1,2) dropped; kept: (1,1)=7, (2,0)=5
    CHECK(m.get(1, 1) == 7);
    CHECK(m.get(1, 0) == 0); // old entry erased, not explicit zero
    CHECK(m.get(2, 0) == 5); // other row untouched

    // setCol similar
    Vec<int> nc(3);
    nc[2] = 8;
    m.setCol(2, nc);
    // after setRow: entries {(1,1)=7, (2,0)=5}. setCol(2) replaces the
    // column-2 entries (none stored) with {(2,2)=8}: total 3 stored.
    CHECK(m.nnz() == 3);
    CHECK(m.get(0, 2) == 0 && m.get(2, 2) == 8);
    CHECK(m.get(1, 1) == 7 && m.get(2, 0) == 5); // untouched

    bool threw = false;
    try { m.setRow(0, Vec<int>(5)); } catch (Exception &) { threw = true; }
    CHECK(threw); // length mismatch
  }

  // Predicates.
  {
    SparseMatrix<int> id(3, 3);
    id.set(0, 0, 1);
    id.set(1, 1, 1);
    id.set(2, 2, 1);
    CHECK(id.isSquare());
    CHECK(id.isDiagonal());
    CHECK(id.isIdentity());
    CHECK(id.isSymmetric());

    SparseMatrix<int> sym(2, 2);
    sym.set(0, 1, 2);
    sym.set(1, 0, 2); // symmetric off-diagonal pair
    CHECK(sym.isSymmetric());
    CHECK(!sym.isDiagonal());
    CHECK(!sym.isIdentity());

    SparseMatrix<int> d(2, 2);
    d.set(0, 0, 3);
    d.set(1, 1, -1);
    CHECK(d.isDiagonal());
    CHECK(d.isSymmetric());
    CHECK(!d.isIdentity());

    SparseMatrix<int> ns(2, 3);
    CHECK(!ns.isSquare());
    CHECK(!ns.isSymmetric());
    CHECK(!ns.isDiagonal());
    CHECK(!ns.isIdentity());
  }

  // almostEqual over stored/implicit union.
  {
    SparseMatrix<double> a(2, 2);
    a.set(0, 0, 1.0);
    a.set(1, 1, 2.0);
    SparseMatrix<double> b(2, 2);
    b.set(0, 0, 1.0);
    b.set(1, 1, 2.0 + 1e-9); // within eps at an only-in-b position union
    CHECK(a.almostEqual(b, 1e-6));

    SparseMatrix<double> c(2, 2);
    c.set(0, 0, 1.5);
    CHECK(!a.almostEqual(c, 1e-6)); // 0.5 gap

    bool threw = false;
    SparseMatrix<double> sh(3, 2);
    try { a.almostEqual(sh); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // norm(): Frobenius over the stored entries.
  {
    SparseMatrix<double> m(2, 2);
    m.set(0, 0, 3);
    m.set(1, 1, 4);
    CHECK(near(m.norm(), 5.0));

    SparseMatrix<double> e(2, 2);
    CHECK(near(e.norm(), 0.0));
  }

  // rank/rcond/powerIteration delegate through toDense().
  {
    SparseMatrix<double> m(3, 3);
    m.set(0, 0, 5); m.set(1, 1, -3); m.set(2, 2, 2);
    CHECK(m.rank() == 3);
    CHECK(near(m.powerIteration(), 5.0, 1e-6));

    // diagonal(5, 3, 2): cond = 5/2 = 2.5; rcond = 0.4
    CHECK(ABS(m.rcond() - 0.4) < 1e-9);

    // rank-deficient sparse row
    SparseMatrix<double> dep(2, 3);
    dep.set(0, 0, 1); dep.set(0, 1, 2); dep.set(0, 2, 3);
    dep.set(1, 0, 2); dep.set(1, 1, 4); dep.set(1, 2, 6); // = 2*row0
    CHECK(dep.rank() == 1);
  }

  // elementwiseMin/Max with implicit zeros.
  {
    SparseMatrix<int> a(2, 2);
    a.set(0, 0, -5);
    a.set(1, 1, 3);
    SparseMatrix<int> b(2, 2);
    b.set(0, 0, 2);
    b.set(1, 0, 4);

    SparseMatrix<int> mn = a.elementwiseMin(b);
    // (0,0): min(-5, 2) = -5 (stored); (1,0): min(0, 4) = 0 -> implicit
    CHECK(mn.get(0, 0) == -5);
    CHECK(mn.get(1, 0) == 0);
    CHECK(mn.get(1, 1) == 0);
    CHECK(mn.nnz() == 1); // only the negative minimum is stored

    SparseMatrix<int> mx = a.elementwiseMax(b);
    CHECK(mx.get(0, 0) == 2);
    CHECK(mx.get(1, 0) == 4); // 4 beats implicit 0: stored
    CHECK(mx.get(1, 1) == 3); // 3 beats implicit 0: stored
    CHECK(mx.nnz() == 3);
  }

  // apply/applied on stored entries; f(x)==0 removes.
  {
    SparseMatrix<int> m(2, 2);
    m.set(0, 0, 5);
    m.set(1, 1, -5);

    SparseMatrix<int> absApplied = m.applied(absVal);
    CHECK(absApplied.get(0, 0) == 5 && absApplied.get(1, 1) == 5);
    CHECK(m.get(1, 1) == -5); // applied() leaves source

    m.apply(absVal); // in place
    CHECK(m.get(1, 1) == 5);

    // f(x) == 0 removes: applied(zeroAt5) drops (1,1)
    SparseMatrix<int> zt(2, 2);
    zt.set(0, 0, 5);
    SparseMatrix<int> zapped = zt.applied(zeroIfFive);
    CHECK(zapped.nnz() == 0);
  }

  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
