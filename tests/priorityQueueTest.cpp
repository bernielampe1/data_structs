// Tests for PriorityQueue<T>. Prints one line per check and exits
// nonzero on the first failure. Build and run from tests/:
//   make priorityQueueTest && ./priorityQueueTest

#include "PriorityQueue.h"
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
static unsigned seed = 1122334455u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Fresh queue.
  {
    PriorityQueue<obj> q;
    CHECK(q.empty());
    CHECK(q.size() == 0);
    CHECK(q.top()._n == -1); // default obj from the empty peek

    q.popTop(); // pop on empty returns the default, no crash
    CHECK(q.empty());
  }

  // MAXHEAP semantics: the largest pops first.
  {
    PriorityQueue<obj> q(10);
    for (int i = 0; i < 8; i++)
      CHECK(q.push(obj(i)));
    CHECK(q.size() == 8);
    CHECK(q.top()._n == 7); // peek without consuming

    bool descending = true;
    int prev = 1 << 30;
    while (!q.empty()) {
      obj v = q.popTop();
      if (v._n > prev)
        descending = false;
      prev = v._n;
    }
    CHECK(descending);
  }

  // Capacity contract: fixed-capacity push refuses when full.
  {
    PriorityQueue<obj> q(3);
    CHECK(q.push(obj(1)));
    CHECK(q.push(obj(2)));
    CHECK(q.push(obj(3)));
    CHECK(!q.push(obj(4))); // full
    CHECK(q.size() == 3);
    CHECK(q.capacity() == 3);
    CHECK(q.popTop()._n == 3);
    CHECK(q.push(obj(4))); // a pop freed a slot
  }

  // Duplicate priorities: equal elements pop in SOME order, all pop.
  {
    PriorityQueue<obj> q(10);
    for (int i = 0; i < 6; i++)
      q.push(obj(3)); // all equal
    CHECK(q.size() == 6);
    unsigned popped = 0;
    while (!q.empty()) {
      CHECK(q.popTop()._n == 3);
      popped++;
    }
    CHECK(popped == 6);
  }

  // changePriority: promotion to the top and demotion to the bottom.
  {
    PriorityQueue<obj> q(10);
    for (int i = 0; i < 5; i++)
      q.push(obj(i));

    // promote 1 to 99: it should sit on top, everything else intact
    CHECK(q.changePriority(obj(1), obj(99)));
    CHECK(q.top()._n == 99);
    CHECK(q.size() == 5);
    CHECK(!q.changePriority(obj(1), obj(1))); // old value no longer here

    // drain: 99 first, then 4 3 2 0 in order
    CHECK(q.popTop()._n == 99);
    bool order = true;
    obj v0 = q.popTop(); // 4
    if (v0._n != 4)
      order = false;
    q.popTop(); // 3
    q.popTop(); // 2
    obj last = q.popTop(); // 0
    CHECK(order && last._n == 0);

    // demote the top
    PriorityQueue<obj> q2(10);
    for (int i = 0; i < 5; i++)
      q2.push(obj(i * 10));
    CHECK(q2.changePriority(obj(40), obj(-5))); // top demoted
    CHECK(q2.top()._n == 30); // the child took over
    CHECK(q2.popTop()._n == 30);
    CHECK(q2.popTop()._n == 20);
    CHECK(q2.popTop()._n == 10);
    CHECK(q2.popTop()._n == 0);
    CHECK(q2.popTop()._n == -5); // demoted element pops last
  }

  // entryRef + changed: the two-phase flow.
  {
    PriorityQueue<obj> q(10);
    for (int i = 0; i < 4; i++)
      q.push(obj(i));

    q.entryRef(1) = obj(77); // mutate in place
    q.changed(1);
    CHECK(q.top()._n == 77);

    bool threw = false;
    try { q.entryRef(99); } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { q.changed(99); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // contains: linear membership (exposed for tests).
  {
    PriorityQueue<obj> q(10);
    q.push(obj(1));
    q.push(obj(2));
    CHECK(q.contains(obj(2)));
    CHECK(!q.contains(obj(9)));
    q.popTop();
    CHECK(!q.contains(obj(2)));
  }

  // clear() empties; reusable.
  {
    PriorityQueue<obj> q(10);
    q.push(obj(5));
    q.clear();
    CHECK(q.empty());
    CHECK(q.capacity() == 10); // slots kept
    CHECK(q.push(obj(6)));
    CHECK(q.popTop()._n == 6);
  }

  // The heap member is inspectable: one level per line.
  {
    PriorityQueue<obj> q(10);
    for (int i = 0; i < 3; i++)
      q.push(obj(i));
    ostringstream oss;
    oss << q;
    CHECK(oss.str().find("\n") != string::npos); // level separators
    CHECK(q.size() == 3);
  }

  // Copy construction deep; assignment replaces; self-assign.
  {
    PriorityQueue<obj> a(10);
    for (int i = 0; i < 4; i++)
      a.push(obj(i));
    PriorityQueue<obj> b(a);
    CHECK(b.size() == 4);
    CHECK(b.popTop()._n == 3);
    CHECK(a.size() == 4); // source untouched

    PriorityQueue<obj> c(20);
    c.push(obj(50));
    c = a; // replace (shrink to a's capacity)
    CHECK(c.size() == 4);
    CHECK(c.top()._n == 3);

    PriorityQueue<obj> &alias = a;
    a = alias;
    CHECK(a.size() == 4);
  }

  // Move construction steals; source empty and reusable-after-assign.
  {
    PriorityQueue<obj> a(8);
    for (int i = 0; i < 4; i++)
      a.push(obj(i));
    PriorityQueue<obj> m(std::move(a));
    CHECK(m.size() == 4);
    CHECK(m.top()._n == 3);

    CHECK(a.size() == 0);
    CHECK(a.capacity() == 0); // BinHeap moved-from contract
  }

  // Move assignment; self-move no-op.
  {
    PriorityQueue<obj> a(8);
    a.push(obj(1));
    PriorityQueue<obj> b(4);
    b.push(obj(9));
    b = std::move(a);
    CHECK(b.size() == 1 && b.top()._n == 1);
    CHECK(a.size() == 0);

    PriorityQueue<obj> &alias = a;
    a = std::move(alias); // self-move: guarded in BinHeap move-assign
    CHECK(a.size() == 0); // empty anyway post-move
  }

  // swap exchanges.
  {
    PriorityQueue<obj> s1(5), s2(9);
    s1.push(obj(1));
    s2.push(obj(2));
    s2.push(obj(3));
    s1.swap(s2);
    CHECK(s1.size() == 2 && s2.size() == 1);
    CHECK(s1.capacity() == 9 && s2.capacity() == 5); // capacities moved too
    CHECK(s1.top()._n == 3);
  }

  // Randomized stress: ADT pop order must produce descending sequence;
  // changePriority churn keeps ordering valid.
  {
    bool allOk = true;
    for (int trial = 0; trial < 100; trial++) {
      const unsigned n = 1 + rnd() % 60;
      PriorityQueue<obj> q(n);
      for (unsigned i = 0; i < n; i++)
        if (!q.push(obj(int(rnd() % 1000))))
          allOk = false;

      // random priority churn on surviving elements
      for (unsigned churn = 0; churn < 10 && q.size() > 0; churn++) {
        int oldV = int(rnd() % 1000);
        int nuV = int(rnd() % 1000);
        q.changePriority(obj(oldV), obj(nuV)); // best effort
      }

      int prev = 1 << 30;
      unsigned cnt = 0;
      while (!q.empty()) {
        obj v = q.popTop();
        if (v._n > prev)
          allOk = false;
        prev = v._n;
        cnt++;
      }
      if (cnt != n)
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
