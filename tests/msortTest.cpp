// Tests for msort(Array<T>&, l, r). Prints one line per check and exits
// nonzero on the first failure. Build and run from tests/:
//   make msortTest && ./msortTest

#include "msort.h"
#include "Array.h"
#include <algorithm>
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

// Deterministic PRNG so failures are reproducible.
static unsigned seed = 447210447u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Ascending sort of arbitrary data.
  {
    float d[10] = {5.5f, 1, 9, 3, 3, 8, 0, -2, 7, 2};
    Array<float> a(10);
    for (int i = 0; i < 10; i++)
      a[i] = d[i];

    msort(a, 0, 9);

    bool ascending = true;
    for (int i = 1; i < 10; i++)
      if (a[i - 1] > a[i])
        ascending = false;
    CHECK(ascending);
  }

  // Stability equivalent: equal keys keep input relative order (the
  // merge takes from the LEFT run on ties -- a[m] <= a[k] picks left).
  {
    struct Tagged {
      int key;
      int seq;
      bool operator<=(const Tagged &o) const { return key <= o.key; }
      bool operator>(const Tagged &o) const { return key > o.key; }
    };
    Array<Tagged> a(6);
    a[0] = {1, 0}; a[1] = {2, 1}; a[2] = {1, 2};
    a[3] = {2, 3}; a[4] = {1, 4}; a[5] = {2, 5};

    msort(a, 0, 5);

    // stable: within key 1, seqs 0,2,4 in order; within 2: 1,3,5
    bool stable = a[0].seq == 0 && a[1].seq == 2 && a[2].seq == 4 &&
                  a[3].seq == 1 && a[4].seq == 3 && a[5].seq == 5;
    CHECK(stable);
  }

  // Edge cases: singletons, subset ranges.
  {
    Array<int> one(1);
    one[0] = 42;
    msort(one, 0, 0);
    CHECK(one[0] == 42);

    Array<int> part(5);
    part[0] = 99; part[1] = 3; part[2] = 1; part[3] = 2; part[4] = 88;
    msort(part, 1, 3);
    CHECK(part[0] == 99); // untouched head
    CHECK(part[4] == 88); // untouched tail
    CHECK(part[1] <= part[2] && part[2] <= part[3]);
  }

  // Reverse-sorted input.
  {
    Array<int> r(50);
    for (int i = 0; i < 50; i++)
      r[i] = 49 - i;
    msort(r, 0, 49);
    bool ok = true;
    for (int i = 0; i < 50; i++)
      if (r[i] != i)
        ok = false;
    CHECK(ok);
  }

  // Randomized cross-check against std::sort.
  {
    bool allOk = true;
    for (int trial = 0; trial < 200; trial++) {
      const int n = 1 + int(rnd() % 200);
      Array<int> a(n);
      Array<int> ref(n);
      for (int i = 0; i < n; i++) {
        int v = int(rnd() % 1000) - 500;
        a[i] = v;
        ref[i] = v;
      }

      std::sort(ref.begin(), ref.end());
      msort(a, 0, n - 1);

      for (int i = 0; i < n; i++)
        if (a[i] != ref[i])
          allOk = false;
    }
    CHECK(allOk);
  }

  // Size coverage incl. odd sizes (the (l+r)/2 mid split).
  {
    bool allOk = true;
    for (int n = 1; n <= 64; n++) {
      Array<int> a(n), ref(n);
      for (int i = 0; i < n; i++) {
        int v = int(rnd() % 100);
        a[i] = v;
        ref[i] = v;
      }
      std::sort(ref.begin(), ref.end());
      msort(a, 0, n - 1);
      for (int i = 0; i < n; i++)
        if (a[i] != ref[i])
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
