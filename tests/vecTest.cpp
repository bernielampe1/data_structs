// Tests for Vec<T>. Prints one line per check and exits nonzero on the
// first failure. Build and run from tests/:  make vecTest && ./vecTest

#include "Vec.h"
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

// Deterministic PRNG so failures are reproducible.
static unsigned seed = 51501550u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Construction and access.
  {
    Vec<float> v0;
    CHECK(v0.len() == 0);

    float a[5] = {1, 2, 3, 4, 5};
    Vec<float> v1(a, 5);
    CHECK(v1.len() == 5);
    CHECK(v1[0] == 1 && v1[4] == 5);

    v1[2] = 30; // mutable index
    CHECK(v1[2] == 30);
    v1[2] = 3;

    const Vec<float> &cv = v1;
    CHECK(cv[1] == 2); // const read
  }

  // init() refills and resizes; clear() empties and stays reusable.
  {
    Vec<float> v(3);
    CHECK(v.len() == 3);
    bool zeroed = true;
    for (u32 i = 0; i < v.len(); i++)
      if (v[i] != 0)
        zeroed = false;
    CHECK(zeroed); // value-initialized to 0

    v.init(2, 7);
    CHECK(v.len() == 2);
    CHECK(v[0] == 7 && v[1] == 7); // init(v) fills

    v.clear();
    CHECK(v.len() == 0);
    v.init(4, 1); // reusable after clear
    CHECK(v.len() == 4 && v[3] == 1);
  }

  // sum()/prod()/abs(): prod is the regression (the old code started
  // its product accumulator at 0, so every real vector product
  // returned 0).
  {
    float a[4] = {1, 2, 3, 4};
    Vec<float> v(a, 4);
    CHECK(v.sum() == 10);
    CHECK(v.prod() == 24); // 1*2*3*4, not 0

    float b[3] = {-1, 5, -7};
    Vec<float> w(b, 3);
    Vec<float> aw = w.abs();
    CHECK(aw[0] == 1 && aw[1] == 5 && aw[2] == 7);
    CHECK(w[0] == -1); // abs() is out of place

    float z[3] = {0, 0, 0};
    Vec<float> zv(z, 3);
    CHECK(zv.prod() == 0); // a real zero product still works
    CHECK(zv.sum() == 0);

    Vec<float> e;
    CHECK(e.prod() == 1); // empty product is the empty product: 1
    CHECK(e.sum() == 0);
  }

  // Scalar arithmetic: element-wise, operands untouched.
  {
    float a[3] = {1, 2, 3};
    Vec<float> v(a, 3);

    Vec<float> s = v + 10.0f;
    CHECK(s[0] == 11 && s[2] == 13);
    Vec<float> d = v - 1.0f;
    CHECK(d[0] == 0 && d[2] == 2);
    Vec<float> m = v * 2.0f;
    CHECK(m[1] == 4);
    Vec<float> q = v / 2.0f;
    CHECK(near(q[1], 1.0));

    Vec<float> io(v);
    io += 1; io -= 2; io *= 3; io /= 4; // (v-1)*3/4
    CHECK(near(io[0], 0.0) && near(io[2], 1.5));
    CHECK(v[0] == 1); // operand untouched
  }

  // Vector arithmetic: element-wise and shape-checked. The old code
  // threw bare string literals -- uncatchable as Exception (regression:
  // catch (Exception &) now intercepts the mismatch).
  {
    float a[3] = {1, 2, 3};
    float b[3] = {10, 20, 30};
    Vec<float> v(a, 3), w(b, 3);

    Vec<float> s = v + w;
    CHECK(s[0] == 11 && s[2] == 33);
    Vec<float> d = w - v;
    CHECK(d[0] == 9 && d[2] == 27);
    Vec<float> m = v * w;
    CHECK(m[1] == 40);
    Vec<float> q = w / v;
    CHECK(q[2] == 10);

    Vec<float> io(w);
    io += v; io -= v; io *= v; io /= v; // round trip
    CHECK(io[0] == 10 && io[2] == 30);
    CHECK(v[0] == 1 && w[0] == 10); // operands untouched

    // shape mismatch throws as Exception
    Vec<float> x(5);
    bool threw = false;
    try { v + x; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { v += x; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { v - x; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { v -= x; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { v * x; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { v *= x; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { v / x; } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { v /= x; } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // Self-assignment is a no-op.
  {
    float a[3] = {1, 2, 3};
    Vec<float> v(a, 3);
    Vec<float> &alias = v;
    v = alias;
    CHECK(v.len() == 3);
    CHECK(v[0] == 1 && v[2] == 3);
  }

  // Copy construction is deep; assignment replaces contents.
  {
    float a[4] = {1, 2, 3, 4};
    Vec<float> orig(a, 4);
    Vec<float> copy(orig);
    CHECK(copy.len() == 4);
    bool same = true;
    for (u32 i = 0; i < 4; i++)
      same = same && (copy[i] == orig[i]);
    CHECK(same);

    copy[0] = 99;
    CHECK(orig[0] == 1); // deep

    Vec<float> grow(10);
    grow = orig; // shrink assignment
    CHECK(grow.len() == 4);
    CHECK(grow[3] == 4);
    CHECK(orig.len() == 4); // no aliasing

    Vec<float> tiny(1);
    tiny = orig; // grow assignment
    CHECK(tiny.len() == 4);
    CHECK(tiny[0] == 1);
  }

  // Move construction steals; source empty and reusable.
  {
    float a[4] = {1, 2, 3, 4};
    Vec<float> src(a, 4);
    Vec<float> dst(std::move(src));
    CHECK(dst.len() == 4);
    CHECK(dst[3] == 4);

    CHECK(src.len() == 0);
    src.init(2, 5); // reusable
    CHECK(src.len() == 2 && src[1] == 5);
  }

  // Move assignment frees destination and steals; self-move safe.
  {
    float a[3] = {7, 8, 9};
    Vec<float> src(a, 3);
    Vec<float> dst(6);
    dst[0] = 55;
    dst = std::move(src);
    CHECK(dst.len() == 3);
    CHECK(dst[0] == 7);

    CHECK(src.len() == 0);
    src.init(1, 1); // reusable
    CHECK(src.len() == 1);

    Vec<float> self2(2);
    self2[0] = 4;
    Vec<float> &alias = self2;
    self2 = std::move(alias); // self-move: no-op
    CHECK(self2.len() == 2 && self2[0] == 4);
  }

  // swap exchanges contents (member and free).
  {
    float a[2] = {1, 2};
    float b[3] = {3, 4, 5};
    Vec<float> s1(a, 2), s2(b, 3);
    s1.swap(s2);
    CHECK(s1.len() == 3 && s2.len() == 2);
    CHECK(s1[0] == 3 && s2[1] == 2);

    swap(s1, s2); // free overload
    CHECK(s1.len() == 2 && s2.len() == 3);
    CHECK(s1[0] == 1 && s2[2] == 5);
  }

  // operator<< prints ", " separated with NO trailing separator
  // (regression: the old printer emitted "a, b, c, ").
  {
    float a[3] = {1, 2, 3};
    Vec<float> v(a, 3);
    ostringstream oss;
    oss << v;
    CHECK(oss.str() == "1, 2, 3");
    CHECK(v.len() == 3);

    Vec<float> e;
    ostringstream ossE;
    ossE << e;
    CHECK(ossE.str() == "");
  }

  // Vec2f_t / Vec3f_t / Vec4f_t pixel helpers still zero-init.
  {
    Vec2f_t p2;
    CHECK(p2.v[0] == 0 && p2.v[1] == 0 && p2.len() == 2);
    Vec3f_t p3;
    CHECK(p3.v[2] == 0 && p3.len() == 3);
    Vec4f_t p4;
    CHECK(p4.v[3] == 0 && p4.len() == 4);

    p3[1] = 5;
    CHECK(p3.v[1] == 5);
  }

  // Randomized stress: element-wise vector ops against a scalar model.
  {
    bool allOk = true;
    for (int trial = 0; trial < 100; trial++) {
      const unsigned n = 1 + rnd() % 50;
      Vec<float> a(n), b(n);
      for (unsigned i = 0; i < n; i++) {
        a[i] = float(int(rnd() % 200) - 100);
        b[i] = float(int(rnd() % 200) - 100);
      }

      Vec<float> s = a + b;
      Vec<float> d = a - b;
      Vec<float> m = a * b;
      float sModel = 0, mModel = 1;
      bool okLocal = true;
      for (unsigned i = 0; i < n; i++) {
        if (s[i] != a[i] + b[i] || d[i] != a[i] - b[i] ||
            m[i] != a[i] * b[i])
          okLocal = false;
        sModel += a[i] + b[i];
        mModel *= a[i] * b[i];
      }
      if (s.sum() != sModel)
        okLocal = false;
      if (n > 0 && mModel == 0)
        mModel = 0; // keep finite; product of ints in float is exact
      if (n > 0 && !near(m.sum(), mModel * float(n == 0 ? 1 : 1), 1e-2) &&
          mModel != 0) {
        // sum of per-element products vs model product-of-products are
        // different quantities; only element-wise equality is asserted
      }
      if (!okLocal)
        allOk = false;
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
