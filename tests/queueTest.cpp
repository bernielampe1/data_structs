// Tests for Queue<T>. Prints one line per check and exits nonzero on
// the first failure. Build and run from tests/:  make queueTest && ./queueTest

#include "Queue.h"
#include <iostream>
#include <sstream>
#include <utility>

using namespace std;

struct obj {
  obj() : _n(0) {}
  obj(const int n) : _n(n) {}

  int _n;
};

ostream &operator<<(ostream &os, const obj &o) {
  os << o._n;
  return (os);
}

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
  // Fresh queue.
  {
    Queue<obj> q;
    CHECK(q.size() == 0);
    CHECK(q.empty());
  }

  // FIFO order: the element pushed FIRST is popped FIRST (this is the
  // contract regression: the old code's push/pop order made front()
  // return the NEWEST element -- LIFO semantics under a queue's name).
  {
    Queue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);

    CHECK(q.front() == 1); // oldest, next to pop
    CHECK(q.back() == 3);  // newest

    bool fifo = true;
    int expect[3] = {1, 2, 3};
    for (int i = 0; i < 3; i++) {
      if (q.front() != expect[i])
        fifo = false;
      q.pop();
    }
    CHECK(fifo);
    CHECK(q.empty());
  }

  // Interleaved push/pop keeps FIFO across cycles.
  {
    Queue<int> q;
    q.push(1);
    q.push(2);
    q.pop();
    q.push(3);
    q.pop();
    CHECK(q.front() == 3); // 1 popped, then 2 popped; 3 remains
    q.pop();
    CHECK(q.empty());

    // full empty/refill cycle
    q.push(7);
    CHECK(q.size() == 1);
    CHECK(q.front() == 7 && q.back() == 7);
    q.pop();
    q.push(8);
    q.push(9);
    CHECK(q.front() == 8 && q.back() == 9);
  }

  // pop on empty is a safe no-op.
  {
    Queue<int> q;
    q.pop();
    CHECK(q.empty());
    q.push(1);
    q.pop();
    q.pop(); // second pop on now-empty
    CHECK(q.empty());
  }

  // Copy construction is deep and preserves order.
  {
    Queue<int> a;
    for (int i = 0; i < 10; i++)
      a.push(i);
    Queue<int> b(a);
    CHECK(b.size() == 10);
    CHECK(b.front() == 0 && b.back() == 9);

    bool same = true;
    for (int i = 0; i < 10; i++) {
      if (b.front() != i)
        same = false;
      b.pop();
    }
    CHECK(same);

    // draining b left a intact
    CHECK(a.size() == 10);
    CHECK(a.front() == 0);

    b.push(99);
    CHECK(a.back() == 9); // no aliasing after pushes in b
  }

  // Copy assignment replaces contents in order.
  {
    Queue<int> a;
    for (int i = 0; i < 5; i++)
      a.push(i);
    Queue<int> b;
    b.push(77);
    b.push(88);
    b = a;
    CHECK(b.size() == 5);
    CHECK(b.front() == 0 && b.back() == 4);
    CHECK(a.size() == 5); // source intact

    // self-assignment is a no-op
    Queue<int> &alias = a;
    a = alias;
    CHECK(a.size() == 5);
    CHECK(a.front() == 0 && a.back() == 4);
  }

  // Move construction steals; source empty and reusable.
  {
    Queue<int> a;
    for (int i = 0; i < 8; i++)
      a.push(i);
    Queue<int> m(std::move(a));
    CHECK(m.size() == 8);
    CHECK(m.front() == 0 && m.back() == 7);

    CHECK(a.size() == 0);
    CHECK(a.empty());
    a.push(5); // reusable
    CHECK(a.size() == 1 && a.front() == 5);
  }

  // Move assignment frees destination and steals.
  {
    Queue<int> a;
    a.push(1);
    a.push(2);
    Queue<int> b;
    b.push(66);
    b = std::move(a);
    CHECK(b.size() == 2);
    CHECK(b.front() == 1 && b.back() == 2);
    CHECK(a.empty());
    a.push(9); // reusable
    CHECK(a.size() == 1 && a.back() == 9);
  }

  // Self-move is a no-op.
  {
    Queue<int> a;
    a.push(1);
    Queue<int> &alias = a;
    a = std::move(alias);
    CHECK(a.size() == 1);
    CHECK(a.front() == 1);
  }

  // swap exchanges contents.
  {
    Queue<int> s1, s2;
    s1.push(1);
    s2.push(2);
    s2.push(3);
    s1.swap(s2);
    CHECK(s1.size() == 2 && s2.size() == 1);
    CHECK(s1.front() == 2 && s1.back() == 3);
    CHECK(s2.front() == 1);
  }

  // clear() empties and the queue stays reusable.
  {
    Queue<int> c;
    for (int i = 0; i < 10; i++)
      c.push(i);
    c.clear();
    CHECK(c.size() == 0 && c.empty());
    c.push(9);
    CHECK(c.size() == 1 && c.front() == 9);
    c.clear();
    c.clear(); // idempotent on empty
    CHECK(c.empty());
  }

  // operator<< prints front to back (oldest first), ", " separated,
  // with NO trailing separator (regression: the old printer emitted a
  // trailing ", " and traversed back-to-front).
  {
    Queue<int> p;
    p.push(1);
    p.push(2);
    p.push(3);
    ostringstream oss;
    oss << p;
    CHECK(oss.str() == "1, 2, 3");
    CHECK(p.size() == 3); // printing did not consume

    Queue<int> e;
    ostringstream ossE;
    ossE << e;
    CHECK(ossE.str() == ""); // empty queue prints nothing
  }

  // Order under stress: 1000 pushes with periodic pops must preserve
  // FIFO exactly.
  {
    Queue<int> q;
    int expect = 0;
    bool ok = true;
    for (int i = 0; i < 1000; i++) {
      q.push(i);
      if (i % 7 == 3) {
        if (q.front() != expect)
          ok = false;
        q.pop();
        expect++;
      }
    }
    CHECK(ok);
    while (!q.empty()) {
      if (q.front() != expect)
        ok = false;
      q.pop();
      expect++;
    }
    CHECK(ok);
    CHECK(expect == 1000);
  }

  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
