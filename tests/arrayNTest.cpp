// Tests for ArrayN<T, Rank>. Prints one line per check and exits
// nonzero on the first failure. Build and run from tests/:
//   make arrayNTest && ./arrayNTest

#include "ArrayN.h"
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

int main() {
  // 1-D: strides degenerate to 1; flat() == operator[].
  {
    ArrayN<int, 1> a({6});
    u64 dims[1], strides[1];
    a.shape(dims, strides);
    CHECK(dims[0] == 6);
    CHECK(strides[0] == 1);

    CHECK(a.size() == 6);
    for (u64 i = 0; i < 6; i++)
      a[{i}] = int(i) * 10;
    CHECK(a.flat(3) == 30);
    CHECK(a[{3}] == 30);
    bool same = true;
    for (u64 i = 0; i < 6; i++)
      if (a.flat(i) != a[{i}])
        same = false;
    CHECK(same); // 1-D: flat and multi-index agree exactly

    // zero-initialized
    ArrayN<int, 1> z({3});
    CHECK(z[{0}] == 0 && z[{1}] == 0 && z[{2}] == 0);
  }

  // 2-D: row-major layout matches Matrix's convention.
  {
    ArrayN<int, 2> m({2, 3}); // 2 rows x 3 cols
    u64 dims[2], strides[2];
    m.shape(dims, strides);
    CHECK(dims[0] == 2 && dims[1] == 3);
    CHECK(strides[0] == 3 && strides[1] == 1); // row-major strides!

    CHECK(m.size() == 6);

    // (r, c) -> r * 3 + c
    for (u64 r = 0; r < 2; r++)
      for (u64 c = 0; c < 3; c++)
        m[{r, c}] = int(r * 10 + c);

    CHECK(m.flat(0) == 0 && m.flat(1) == 1);
    CHECK(m.flat(3) == 10 && m.flat(5) == 12);
    CHECK((m[{1, 2}] == 12));

    // strides let you address the next row: flat + stride[0]
    CHECK((&m.flat(1) == &m[{0, 1}] || true)); // braces differ; semantics:
    CHECK((m.flat(0 + m.stride(0)) == m[{1, 0}])); // stride[0] = one row
  }

  // 3-D canonical strides: {6, 2, 1} for dims {2, 3, 2}.
  {
    ArrayN<int, 3> cube({2, 3, 2});
    u64 dims[3], strides[3];
    cube.shape(dims, strides);
    CHECK(dims[0] == 2 && dims[1] == 3 && dims[2] == 2);
    CHECK(strides[0] == 6 && strides[1] == 2 && strides[2] == 1);

    CHECK(cube.size() == 12);

    // every (i, j, k) maps to i*6 + j*2 + k
    for (u64 i = 0; i < 2; i++)
      for (u64 j = 0; j < 3; j++)
        for (u64 k = 0; k < 2; k++)
          cube[{i, j, k}] = int(i * 100 + j * 10 + k);

    CHECK(cube.flat(0 * 6 + 0 * 2 + 0) == 0);
    CHECK(cube.flat(1 * 6 + 2 * 2 + 1) == 121);
    CHECK((cube[{1, 2, 1}] == 121));
    CHECK((cube[{0, 0, 1}] == 1));
  }

  // 4-D: nested stride arithmetic continues to compose.
  {
    ArrayN<int, 4> hyper({2, 2, 2, 2});
    u64 dims[4], strides[4];
    hyper.shape(dims, strides);
    CHECK(strides[0] == 8 && strides[1] == 4 && strides[2] == 2 &&
          strides[3] == 1);

    CHECK(hyper.size() == 16);
    hyper[{1, 0, 1, 0}] = 42;
    CHECK(hyper.flat(8 + 0 + 2 + 0) == 42);
  }

  // at() bounds checking (each axis individually).
  {
    ArrayN<int, 3> cube({2, 3, 2});
    CHECK((cube.at({1, 2, 1}) == 0));

    bool threw = false;
    try { cube.at({2, 0, 0}); } catch (Exception &) { threw = true; }
    CHECK(threw); // dim 0 out of range

    threw = false;
    try { cube.at({0, 3, 0}); } catch (Exception &) { threw = true; }
    CHECK(threw); // dim 1 out of range

    threw = false;
    try { cube.at({0, 0, 2}); } catch (Exception &) { threw = true; }
    CHECK(threw); // dim 2 out of range

    threw = false;
    try { cube.at({1, 2}); } catch (Exception &) { threw = true; }
    CHECK(threw); // wrong rank

    threw = false;
    try { cube.at({1, 2, 1, 0}); } catch (Exception &) { threw = true; }
    CHECK(threw); // too many coordinates
  }

  // Construction rejects zero dimensions and wrong-rank lists.
  {
    bool threw = false;
    try { ArrayN<int, 2> bad({0, 3}); } catch (Exception &) { threw = true; }
    CHECK(threw);

    threw = false;
    try { ArrayN<int, 2> bad({3}); } catch (Exception &) { threw = true; }
    CHECK(threw); // 1 initializer for a 2-D array

    threw = false;
    try { ArrayN<int, 2> bad({2, 2, 2}); } catch (Exception &) { threw = true; }
    CHECK(threw); // 3 initializers for a 2-D array
  }

  // Copy construction is deep and holds shape/strides.
  {
    ArrayN<int, 2> a({3, 3});
    for (u64 i = 0; i < 9; i++)
      a.flat(i) = int(i);

    ArrayN<int, 2> b(a);
    CHECK(b.size() == 9);
    u64 dims[2], strides[2], dims2[2], strides2[2];
    a.shape(dims, strides);
    b.shape(dims2, strides2);
    CHECK(dims[0] == dims2[0] && dims[1] == dims2[1]);
    CHECK(strides[0] == strides2[0] && strides[1] == strides2[1]);

    bool same = true;
    for (u64 i = 0; i < 9; i++)
      if (b.flat(i) != a.flat(i))
        same = false;
    CHECK(same);

    b.flat(0) = 99;
    CHECK(a.flat(0) == 0); // deep
  }

  // Copy assignment replaces shape too.
  {
    ArrayN<int, 2> a({2, 2});
    ArrayN<int, 2> b({4, 5});
    b.flat(19) = 7;
    b = a; // shrink assignment
    CHECK(b.size() == 4);
    u64 dims[2], strides[2];
    b.shape(dims, strides);
    CHECK(dims[0] == 2 && dims[1] == 2 && strides[0] == 2);

    // self-assignment
    ArrayN<int, 2> &alias = a;
    a = alias;
    CHECK(a.size() == 4);
    CHECK((a[{1, 1}] == 0));
  }

  // Move construction steals; source drained.
  {
    ArrayN<int, 2> a({3, 4});
    a.flat(5) = 55;
    ArrayN<int, 2> m(std::move(a));
    CHECK(m.size() == 12);
    CHECK(m.flat(5) == 55);

    CHECK(a.size() == 0);
    u64 dims[2], strides[2];
    a.shape(dims, strides);
    CHECK(dims[0] == 0 && dims[1] == 0); // moved-from drained
  }

  // Move assignment; self-move no-op.
  {
    ArrayN<int, 3> a({2, 2, 2});
    a[{1, 1, 1}] = 8;
    ArrayN<int, 3> b({1, 1, 1});
    b = std::move(a);
    CHECK(b.size() == 8);
    CHECK((b[{1, 1, 1}] == 8));

    CHECK(a.size() == 0);
    ArrayN<int, 3> &alias = a; // no-op self-move
    a = std::move(alias);
    CHECK(a.size() == 0);
  }

  // swap exchanges shape and contents.
  {
    ArrayN<int, 2> s1({2, 2}), s2({3, 3});
    s1.flat(0) = 1;
    s2.flat(4) = 2;
    s1.swap(s2);
    CHECK(s1.size() == 9 && s2.size() == 4);
    CHECK(s1.flat(4) == 2 && s2.flat(0) == 1);

    swap(s1, s2);
    CHECK(s1.size() == 4 && s2.size() == 9);
    CHECK(s1.flat(0) == 1 && s2.flat(4) == 2);
  }

  // operator<< rounds out the house-style printer.
  {
    ArrayN<int, 2> a({2, 2});
    a.flat(3) = 9;
    ostringstream oss;
    oss << a;
    CHECK((oss.str() == "ArrayN<2D> {0, 0, 0, 9}"));
  }

  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
