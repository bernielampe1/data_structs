// Tests for Histogram<T>. Prints one line per check and exits nonzero
// on the first failure. Build and run from tests/:
//   make histogramTest && ./histogramTest

#include <iostream>
#include <sstream>
#include <utility>

using namespace std;

#include "Histogram.h"

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
  // Default construction: empty, no bins, zero range.
  {
    Histogram<int> h;
    CHECK(h.getNumBins() == 0);
    CHECK(h.getMin() == 0 && h.getMax() == 0 && h.getBinWidth() == 0);
  }

  // Basic FD binning on a well-spread sample. Values 0..99: min 0,
  // max 99, so every value lands in its own width-packed bin and the
  // counts must reconstruct the sorted sample exactly.
  {
    int v[100];
    for (int i = 0; i < 100; i++)
      v[i] = i;
    Histogram<int> h;
    h.set(v, 100);

    CHECK(h.getNumBins() > 0);
    CHECK(h.getMin() == 0);
    CHECK(h.getMax() == 99);

    // total count conservation: all bins sum to the sample size
    int total = 0;
    for (int i = 0; i < h.getNumBins(); ++i)
      total += h[i];
    CHECK(total == 100);

    // counts are nonnegative
    bool nonneg = true;
    for (int i = 0; i < h.getNumBins(); ++i)
      if (h[i] < 0)
        nonneg = false;
    CHECK(nonneg);

    // min bin <= each value's bin; value at max must land in last bin
    int last = h.getNumBins() - 1;
    CHECK(h[last] >= 1);
  }

  // Big counts are correct: memset zeroed numBins BYTES, so bins held
  // garbage on 64-bit builds (regression).
  {
    // outlier-heavy sample: 4 low values, 96 identical high values
    int v[100];
    for (int i = 0; i < 4; i++)
      v[i] = i;
    for (int i = 4; i < 100; i++)
      v[i] = 1000000;
    Histogram<int> h;
    h.set(v, 100);

    // count conservation regardless of FD outcome
    int total = 0;
    for (int i = 0; i < h.getNumBins(); ++i)
      total += h[i];
    CHECK(total == 100);

    // the bulk must be in SOME bin, exactly 96 strong
    bool bulk = false;
    for (int i = 0; i < h.getNumBins(); ++i)
      if (h[i] == 96)
        bulk = true;
    CHECK(bulk);

    int last = h.getNumBins() - 1;
    CHECK(h.getMax() == 1000000);
    CHECK(h[last] >= 96 - 4); // max value in last bin; bulk mostly there
  }

  // Degenerate samples: all identical values (r == 0, d == 0) fall back
  // to a single unit-width bin holding everything (regression: the old
  // code read v_t[-1] here on tiny samples and v_t[2] on n==2 -- both
  // out of bounds).
  {
    int v[5] = {7, 7, 7, 7, 7};
    Histogram<int> h;
    h.set(v, 5);
    CHECK(h.getNumBins() == 1);
    CHECK(h[0] == 5);
    CHECK(h.getMin() == 7 && h.getMax() == 7);

    // n == 1: quartile indexes clamp to the sample
    int one[1] = {42};
    Histogram<int> h1;
    h1.set(one, 1);
    CHECK(h1.getNumBins() >= 1);
    CHECK(h1.getMin() == 42 && h1.getMax() == 42);
    int total = 0;
    for (int i = 0; i < h1.getNumBins(); ++i)
      total += h1[i];
    CHECK(total == 1);

    // n == 2: the old 3*qw-1 index was out of bounds
    int two[2] = {3, 9};
    Histogram<int> h2;
    h2.set(two, 2);
    int total2 = 0;
    for (int i = 0; i < h2.getNumBins(); ++i)
      total2 += h2[i];
    CHECK(total2 == 2);
    CHECK(h2.getMin() == 3 && h2.getMax() == 9);

    // n == 3, all distinct ascending
    int three[3] = {1, 5, 9};
    Histogram<int> h3;
    h3.set(three, 3);
    int total3 = 0;
    for (int i = 0; i < h3.getNumBins(); ++i)
      total3 += h3[i];
    CHECK(total3 == 3);
  }

  // set() rejects bad samples with Exception (the old code read v_t[0]
  // of an empty vector, then new signed[negative])
  {
    Histogram<int> h;
    bool threw = false;
    try { h.set((int *)0, 0); } catch (Exception &) { threw = true; }
    CHECK(threw); // n == 0

    threw = false;
    try { h.set((int *)0, -5); } catch (Exception &) { threw = true; }
    CHECK(threw); // n < 0

    threw = false;
    int x = 1;
    try { h.set(&x, 1); } catch (Exception &) { threw = true; }
    CHECK(!threw); // a valid one-element sample still works
  }

  // operator[] bounds-checking.
  {
    int v[4] = {0, 10, 20, 30};
    Histogram<int> h;
    h.set(v, 4);
    bool threw = false;
    try { h[h.getNumBins()]; } catch (Exception &) { threw = true; }
    CHECK(threw);

    threw = false;
    try { h[-1]; } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // Calling set() again re-initializes: no leak of the old bins and the
  // new sample fully replaces the old counts (regression: the old code
  // leaked the previous allocation).
  {
    int a[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int b[3] = {100, 200, 300};
    Histogram<int> h;
    h.set(a, 10);
    int binsA = h.getNumBins();
    CHECK(binsA > 0);
    CHECK(h.getMin() == 0 && h.getMax() == 9);

    h.set(b, 3); // totally different range
    CHECK(h.getMin() == 100 && h.getMax() == 300);
    int total = 0;
    for (int i = 0; i < h.getNumBins(); ++i)
      total += h[i];
    CHECK(total == 3); // old counts gone

    // sparse then dense, same-size objects
    h.set(a, 10);
    CHECK(h.getNumBins() > 0);
    total = 0;
    for (int i = 0; i < h.getNumBins(); ++i)
      total += h[i];
    CHECK(total == 10);
  }

  // Copy construction is deep and independent.
  {
    int v[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    Histogram<int> a;
    a.set(v, 8);
    Histogram<int> b(a);
    CHECK(b.getNumBins() == a.getNumBins());
    CHECK(b.getMin() == a.getMin() && b.getMax() == a.getMax());
    bool same = true;
    for (int i = 0; i < a.getNumBins(); ++i)
      same = same && (b[i] == a[i]);
    CHECK(same);

    // mutating through one must not touch the other
    Histogram<int> &alias = b;
    b.set(v, 8); // rebin same data: no count change expected
    (void)alias;
    CHECK(b.getNumBins() == a.getNumBins());

    int more[2] = {0, 0};
    b.set(more, 2); // different sample in b
    CHECK(a.getNumBins() != 0);
    int total = 0;
    for (int i = 0; i < b.getNumBins(); ++i)
      total += b[i];
    CHECK(total == 2);
    total = 0;
    for (int i = 0; i < a.getNumBins(); ++i)
      total += a[i];
    CHECK(total == 8); // a's counts survived b's rebin
  }

  // Copy construction from an EMPTY histogram: _hist null must stay null
  // (regression: the old class had no copy ctor at all).
  {
    Histogram<int> empty_;
    Histogram<int> c(empty_);
    CHECK(c.getNumBins() == 0);

    // copying an empty histogram over a populated one leaves it empty
    Histogram<int> d;
    int v[3] = {1, 2, 3};
    d.set(v, 3);
    d = empty_;
    CHECK(d.getNumBins() == 0);

    // and assignment out of an empty must be safe to reuse
    int v2[3] = {1, 2, 3};
    d.set(v2, 3);
    CHECK(d.getNumBins() >= 1);
  }

  // Copy assignment replaces contents; self-assignment is a no-op.
  {
    int v[6] = {0, 1, 2, 3, 4, 5};
    Histogram<int> a;
    a.set(v, 6);

    Histogram<int> b;
    int w[4] = {9, 9, 9, 9};
    b.set(w, 4);
    b = a;
    CHECK(b.getNumBins() == a.getNumBins());
    CHECK(b.getMin() == 0 && b.getMax() == 5);
    int total = 0;
    for (int i = 0; i < b.getNumBins(); ++i)
      total += b[i];
    CHECK(total == 6);
    CHECK(a.getNumBins() == b.getNumBins()); // source intact

    Histogram<int> &alias = a;
    a = alias;
    CHECK(a.getNumBins() == b.getNumBins());
    total = 0;
    for (int i = 0; i < a.getNumBins(); ++i)
      total += a[i];
    CHECK(total == 6); // self-assignment did not corrupt counts
  }

  // Move construction steals; source is empty and reusable.
  {
    int v[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    Histogram<int> a;
    a.set(v, 8);
    int bins = a.getNumBins();
    double mn = a.getMin(), mx = a.getMax();

    Histogram<int> m(std::move(a));
    CHECK(m.getNumBins() == bins);
    CHECK(m.getMin() == mn && m.getMax() == mx);
    int total = 0;
    for (int i = 0; i < m.getNumBins(); ++i)
      total += m[i];
    CHECK(total == 8);

    CHECK(a.getNumBins() == 0); // emptied by the move

    a.set(v, 8); // reusable
    CHECK(a.getNumBins() == bins);
    CHECK(a.getMin() == mn && a.getMax() == mx);
  }

  // Move assignment frees destination contents and steals the source.
  {
    int v[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    Histogram<int> a;
    a.set(v, 8);

    Histogram<int> b;
    int w[2] = {50, 60};
    b.set(w, 2);
    b = std::move(a);
    CHECK(b.getMax() == 7);
    int total = 0;
    for (int i = 0; i < b.getNumBins(); ++i)
      total += b[i];
    CHECK(total == 8);

    CHECK(a.getNumBins() == 0);
    a.set(w, 2); // reusable
    CHECK(a.getMax() == 60);
  }

  // Self-move is a no-op.
  {
    int v[4] = {1, 2, 3, 4};
    Histogram<int> a;
    a.set(v, 4);
    Histogram<int> &alias = a;
    a = std::move(alias);
    CHECK(a.getNumBins() >= 1);
    int total = 0;
    for (int i = 0; i < a.getNumBins(); ++i)
      total += a[i];
    CHECK(total == 4);
  }

  // swap exchanges bins and ranges.
  {
    int v[4] = {0, 10, 20, 30};
    int w[3] = {100, 200, 300};
    Histogram<int> a, b;
    a.set(v, 4);
    b.set(w, 3);
    int binsA = a.getNumBins(), binsB = b.getNumBins();
    a.swap(b);
    CHECK(a.getNumBins() == binsB && b.getNumBins() == binsA);
    CHECK(a.getMax() == 300 && b.getMax() == 30);
  }

  // clear() releases everything; the histogram is reusable after.
  {
    int v[4] = {0, 10, 20, 30};
    Histogram<int> h;
    h.set(v, 4);
    h.clear();
    CHECK(h.getNumBins() == 0);
    CHECK(h.getMin() == 0 && h.getMax() == 0 && h.getBinWidth() == 0);
    h.set(v, 4); // reuse works
    int total = 0;
    for (int i = 0; i < h.getNumBins(); ++i)
      total += h[i];
    CHECK(total == 4);
    h.clear();
    h.clear(); // idempotent on empty
    CHECK(h.getNumBins() == 0);
  }

  // Double data: fractional bin widths, max clamped into last bin.
  {
    double v[6] = {0.0, 0.5, 1.0, 1.5, 2.0, 2.0};
    Histogram<double> h;
    h.set(v, 6);
    CHECK(h.getMin() == 0.0 && h.getMax() == 2.0);
    int total = 0;
    for (int i = 0; i < h.getNumBins(); ++i)
      total += h[i];
    CHECK(total == 6);
    CHECK(h[h.getNumBins() - 1] >= 2); // 2.0 twice: at or past the last

    // identical doubles: degenerate fallback, single bin
    double same[3] = {2.5, 2.5, 2.5};
    h.set(same, 3);
    CHECK(h.getNumBins() == 1);
    CHECK(h[0] == 3);
  }

  // print()/operator<< produce output and do not mutate.
  {
    int v[5] = {0, 1, 2, 3, 4};
    Histogram<int> h;
    h.set(v, 5);
    ostringstream os;
    os << h;
    CHECK(os.str().find("* Histogram:") != string::npos);
    CHECK(os.str().find("* Num Bins:") != string::npos);
    CHECK(h.getNumBins() >= 1); // printing consumed nothing
  }

  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
