// Tests for qsort(Array<T>&, l, r). Prints one line per check and exits
// nonzero on the first failure. Build and run from tests/:
//   make qsortTest && ./qsortTest

#include "qsort.h"
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
static unsigned seed = 701223551u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Determinism: same element multiset, ANY order, sorts ascending.
  {
    float d[10] = {5.5f, 1, 9, 3, 3, 8, 0, -2, 7, 2};
    Array<float> a(10);
    for (int i = 0; i < 10; i++)
      a[i] = d[i];

    qsort(a, 0, 9);

    bool ascending = true;
    for (int i = 1; i < 10; i++)
      if (a[i - 1] > a[i])
        ascending = false;
    CHECK(ascending);
  }

  // Prints nothing (qsort mutates in place; stdout is not the product).
  {
    ostringstream oss;
    streambuf *old = cout.rdbuf(oss.rdbuf());
    Array<int> a(3);
    a[0] = 2; a[1] = 1; a[2] = 3;
    qsort(a, 0, 2);
    cout.rdbuf(old);
    CHECK(oss.str() == ""); // pure function of the array: no printing
  }

  // Edge cases: empty ranges, singletons, bounds.
  {
    // single element: any (l, l) range is already sorted
    Array<int> one(1);
    one[0] = 42;
    qsort(one, 0, 0);
    CHECK(one[0] == 42);

    // two elements out of order
    Array<int> two(2);
    two[0] = 9; two[1] = 1;
    qsort(two, 0, 1);
    CHECK(two[0] == 1 && two[1] == 9);

    // range subset: sorting [1..3] of a 5-array leaves the rest alone
    Array<int> part(5);
    part[0] = 99; part[1] = 3; part[2] = 1; part[3] = 2; part[4] = 88;
    qsort(part, 1, 3);
    CHECK(part[0] == 99);                 // untouched head
    CHECK(part[4] == 88);                 // untouched tail
    CHECK(part[1] <= part[2] && part[2] <= part[3]); // sorted middle

    // l == r is fine (one-element range)
    Array<int> mid(3);
    mid[0] = 5; mid[1] = 1; mid[2] = 9;
    qsort(mid, 1, 1);
    CHECK(mid[1] == 1);
  }

  // Already-sorted and reverse-sorted inputs (partition stress).
  {
    Array<int> s(50);
    for (int i = 0; i < 50; i++)
      s[i] = i;
    qsort(s, 0, 49);
    bool ok = true;
    for (int i = 0; i < 50; i++)
      if (s[i] != i)
        ok = false;
    CHECK(ok); // ascending input survives

    Array<int> r(50);
    for (int i = 0; i < 50; i++)
      r[i] = 49 - i;
    qsort(r, 0, 49);
    ok = true;
    for (int i = 0; i < 50; i++)
      if (r[i] != i)
        ok = false;
    CHECK(ok); // descending input inverts correctly
  }

  // All-equal elements: the <= partition keeps termination.
  {
    Array<int> eq(20);
    for (int i = 0; i < 20; i++)
      eq[i] = 7;
    qsort(eq, 0, 19);
    bool ok = true;
    for (int i = 0; i < 20; i++)
      if (eq[i] != 7)
        ok = false;
    CHECK(ok);
  }

  // Randomized cross-check against std::sort (the model).
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

      std::sort(ref.begin(), ref.end()); // the model (also via pointers)
      qsort(a, 0, n - 1);

      for (int i = 0; i < n; i++)
        if (a[i] != ref[i])
          allOk = false;
    }
    CHECK(allOk);
  }

  // Duplicates-heavy random inputs.
  {
    bool allOk = true;
    for (int trial = 0; trial < 100; trial++) {
      const int n = 1 + int(rnd() % 100);
      Array<int> a(n);
      Array<int> ref(n);
      for (int i = 0; i < n; i++) {
        int v = int(rnd() % 5); // only 5 values: max duplicates
        a[i] = v;
        ref[i] = v;
      }
      std::sort(ref.begin(), ref.end());
      qsort(a, 0, n - 1);

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
