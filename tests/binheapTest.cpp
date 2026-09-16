// Tests for BinHeap<T>. Prints one line per check and exits nonzero on
// the first failure. Build and run from tests/:  make binheapTest && ./binheapTest

#include <iostream>
#include <utility>

using namespace std;

#include "BinHeap.h"

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

struct obj {
  obj() : _n(-1) {}
  obj(const int n) : _n(n) {}
  bool operator<(const obj &rhs) const { return _n < rhs._n; }
  bool operator>(const obj &rhs) const { return _n > rhs._n; }
  bool operator==(const obj &rhs) const { return _n == rhs._n; }
  bool operator!=(const obj &rhs) const { return _n != rhs._n; }

  int _n;
};

ostream &operator<<(ostream &os, const obj &o) {
  os << o._n;
  return (os);
}

// Deterministic PRNG so failures are reproducible.
static unsigned seed = 987654321u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Construction and capacity semantics.
  BinHeap<obj> bh(100);

  CHECK(bh.capacity() == 100);
  CHECK(bh.size() == 0);
  CHECK(bh.empty());
  CHECK(bh.remaining() == 100);

  // Insert and extract: MAXHEAP yields descending order.
  for (int i = 0; i < 100; i++) {
    CHECK(bh.insert(obj(i))); // every insert fits
  }
  CHECK(bh.size() == 100);
  CHECK(!bh.empty());
  CHECK(bh.remaining() == 0);

  bool descending = true;
  int prev = 1 << 30;
  unsigned extracted = 0;
  while (!bh.empty()) {
    obj v = bh.extractroot();
    if (v._n > prev)
      descending = false;
    prev = v._n;
    extracted++;
  }
  CHECK(descending);
  CHECK(extracted == 100);

  // A default-capacity heap is empty and safe.
  BinHeap<obj> de;
  CHECK(de.empty());
  CHECK(de.peekroot()._n == -1); // default obj
  CHECK(de.extractroot()._n == -1);

  // Empty peek/extract are safe on a sized heap too.
  BinHeap<obj> es(10);
  CHECK(es.peekroot()._n == -1);
  CHECK(es.extractroot()._n == -1);
  CHECK(es.empty());

  // Full-heap insert returns false and drops the element.
  BinHeap<obj> full(3);
  CHECK(full.insert(obj(1)));
  CHECK(full.insert(obj(2)));
  CHECK(full.insert(obj(3)));
  CHECK(!full.insert(obj(4))); // full
  CHECK(full.size() == 3);
  CHECK(full.extractroot()._n == 3); // heap intact

  // heapify() bulk-inserts; too-large arrays insert NOTHING.
  {
    BinHeap<obj> h(8);
    obj arr[5] = {obj(3), obj(1), obj(4), obj(1), obj(5)};
    CHECK(h.heapify(arr, 5));
    CHECK(h.size() == 5);

    bool desc = true;
    int p = 1 << 30;
    while (!h.empty()) {
      obj v = h.extractroot();
      if (v._n > p)
        desc = false;
      p = v._n;
    }
    CHECK(desc);

    BinHeap<obj> small(2);
    obj big[3] = {obj(1), obj(2), obj(3)};
    CHECK(!small.heapify(big, 3));
    CHECK(small.size() == 0); // nothing was inserted
  }

  // Randomized stress: many shuffles, all sizes, order must hold and
  // counts must match (regression for the sift-down child-pick bug,
  // where _heapify compared each child to the parent, letting the
  // weaker child overwrite the stronger pick).
  {
    bool allSorted = true;
    for (int trial = 0; trial < 100; trial++) {
      unsigned n = 1 + rnd() % 60;
      BinHeap<obj> h(n);
      for (unsigned i = 0; i < n; i++)
        if (!h.insert(obj(rnd() % 1000)))
          allSorted = false;

      int p = 1 << 30;
      unsigned cnt = 0;
      while (!h.empty()) {
        obj v = h.extractroot();
        if (v._n > p)
          allSorted = false;
        p = v._n;
        cnt++;
      }
      if (cnt != n)
        allSorted = false;
    }
    CHECK(allSorted);
  }

  // Exact power-of-two sizes (level boundaries in operator<<).
  {
    BinHeap<obj> h(128);
    for (int i = 0; i < 128; i++)
      h.insert(obj(i));
    CHECK(h.size() == 128);
    bool desc = true;
    int p = 1 << 30;
    while (!h.empty()) {
      obj v = h.extractroot();
      if (v._n > p)
        desc = false;
      p = v._n;
    }
    CHECK(desc);
  }

  // clear() forgets elements but keeps capacity.
  {
    BinHeap<obj> h(10);
    h.insert(obj(5));
    h.insert(obj(6));
    h.clear();
    CHECK(h.size() == 0);
    CHECK(h.empty());
    CHECK(h.capacity() == 10);
    CHECK(h.remaining() == 10);
    CHECK(h.insert(obj(7)));
    CHECK(h.extractroot()._n == 7);
  }

  // Zero-capacity heap: every operation safe.
  {
    BinHeap<obj> h(0);
    CHECK(!h.insert(obj(1)));
    CHECK(h.size() == 0);
    CHECK(h.peekroot()._n == -1);
    CHECK(h.extractroot()._n == -1);
  }

  // Copy construction is deep.
  {
    BinHeap<obj> a(20);
    for (int i = 0; i < 10; i++)
      a.insert(obj(i));
    BinHeap<obj> b(a);
    CHECK(b.size() == 10);
    CHECK(b.extractroot()._n == 9);
    CHECK(a.size() == 10); // independent: extracting b left a intact
    CHECK(a.extractroot()._n == 9);
  }

  // u64 capacity: a 2^33-slot heap (32 GiB of int) allocates, reports
  // u64-typed counts, and the level printer handles it without 32-bit
  // shift overflow. Small footprint: only [0..3) touched.
  {
    BinHeap<int> big(u64(1) << 33);
    CHECK(big.capacity() == (u64(1) << 33));
    CHECK(big.remaining() == (u64(1) << 33));
    CHECK(big.size() == 0 && big.empty());
    CHECK(big.insert(1) && big.insert(2) && big.insert(3));
    CHECK(big.size() == 3);
    CHECK(big.extractroot() == 3);
    CHECK(big.extractroot() == 2);
    CHECK(big.extractroot() == 1);
    cout << "print:" << endl << big; // empty print after full extraction
  }

  // u64 accessors are addressable across the 32-bit boundary: capacity
  // just past 2^32 validates that none of the arithmetic wraps in u32.
  {
    BinHeap<char> over32(u64(1) + (u64(1) << 32)); // 4 GiB of char
    CHECK(over32.capacity() == u64(1) + (u64(1) << 32));
    CHECK(over32.insert('a'));
    CHECK(over32.size() == 1);
    CHECK(over32.extractroot() == 'a');
  }

  // floor_log2<u64> overload: 2^40 must report 40, not wrap a 32-bit
  // count (regression for the widened heap printer).
  {
    u64 v = u64(1) << 40;
    CHECK(floor_log2(v) == 40);
    CHECK(floor_log2(u64(1) << 63) == 63);
    CHECK(floor_log2(u64(0xFFFFFFFF)) == 31);
    CHECK(floor_log2(u64(1) << 32) == 32);
  }

  // Copy assignment replaces contents (any capacity combination).
  {
    BinHeap<obj> a(20);
    for (int i = 0; i < 6; i++)
      a.insert(obj(i * 3));
    BinHeap<obj> bigger(50);
    bigger.insert(obj(999));
    bigger = a; // shrink
    CHECK(bigger.size() == 6);
    CHECK(bigger.peekroot()._n == 15);

    BinHeap<obj> smaller(2);
    smaller = a; // grow
    CHECK(smaller.size() == 6);
    CHECK(smaller.peekroot()._n == 15);
  }

  // Self-assignment is a no-op.
  {
    BinHeap<obj> a(10);
    a.insert(obj(1));
    a.insert(obj(2));
    BinHeap<obj> &alias = a;
    a = alias;
    CHECK(a.size() == 2);
    CHECK(a.extractroot()._n == 2);
  }

  // Move construction steals; source is empty and reusable.
  {
    BinHeap<obj> a(20);
    for (int i = 0; i < 8; i++)
      a.insert(obj(i));
    BinHeap<obj> m(std::move(a));
    CHECK(m.size() == 8);
    CHECK(a.size() == 0);
    CHECK(a.empty());
    CHECK(a.capacity() == 0); // capacity moved with the buffer
    CHECK(!a.insert(obj(42))); // no capacity: insert refuses

    // The heap becomes reusable again via assignment.
    BinHeap<obj> fresh(4);
    fresh.insert(obj(42));
    a = fresh;
    CHECK(a.capacity() == 4);
    CHECK(a.extractroot()._n == 42);
  }

  // Move assignment frees destination contents and steals the source.
  {
    BinHeap<obj> a(20);
    for (int i = 0; i < 5; i++)
      a.insert(obj(i));
    BinHeap<obj> m2(3);
    m2.insert(obj(999));
    m2 = std::move(a);
    CHECK(m2.size() == 5);
    CHECK(m2.peekroot()._n == 4);
    CHECK(a.size() == 0);
  }

  // swap exchanges contents and capacities.
  {
    BinHeap<obj> s1(4), s2(9);
    s1.insert(obj(1));
    s2.insert(obj(2));
    s2.insert(obj(3));
    s1.swap(s2);
    CHECK(s1.size() == 2 && s2.size() == 1);
    CHECK(s1.capacity() == 9 && s2.capacity() == 4);
    CHECK(s1.peekroot()._n == 3);
  }

  // operator<< prints one level per line; smoke-test a 3-level heap.
  {
    BinHeap<obj> h(10);
    for (int i = 0; i < 4; i++)
      h.insert(obj(i));
    cout << "print:" << endl << h;
    CHECK(h.size() == 4); // printing did not consume anything
  }

  // changedAt() and operator[]: the priority-change flow.
  {
    BinHeap<obj> h(10);
    for (int i = 0; i < 5; i++)
      h.insert(obj(i));
    CHECK(h[0]._n == 4); // slot 0 is the root (max)

    h[2] = obj(50); // slot 2 now beats the root: push it up
    h.changedAt(2);
    CHECK(h.peekroot()._n == 50);
    bool desc = true;
    int p = 1 << 30;
    unsigned cnt = 0;
    while (!h.empty()) {
      obj v = h.extractroot();
      if (v._n > p)
        desc = false;
      p = v._n;
      cnt++;
    }
    CHECK(desc && cnt == 5); // changedAt restored the ordering

    // demotion: root drops below its children. Pre-change slot layout
    // (probe): [40, 30, 10, 0, 20]; after demoting slot 0 to -1 the
    // pops run 30, 20, 10, 0, -1 in descending order.
    BinHeap<obj> h2(10);
    for (int i = 0; i < 5; i++)
      h2.insert(obj(i * 10));
    h2[0] = obj(-1);
    h2.changedAt(0);
    CHECK(h2.peekroot()._n == 30); // the child took over
    CHECK(h2.extractroot()._n == 30);
    CHECK(h2.extractroot()._n == 20);
    CHECK(h2.extractroot()._n == 10);
    CHECK(h2.extractroot()._n == 0);
    CHECK(h2.extractroot()._n == -1); // the demoted element pops last

    // out-of-range changedAt throws
    bool threw = false;
    try { h2.changedAt(99); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
