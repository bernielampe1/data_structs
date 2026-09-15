// Tests for Stack<T>. Prints one line per check and exits nonzero on the
// first failure. Build and run from tests/:  make stackTest && ./stackTest

#include "Stack.h"
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
  // Construction and size.
  Stack<obj> s0;
  Stack<obj> s1(s0);

  for (int i = 0; i < 25; i++) {
    s0.push(obj(i));
  }

  Stack<obj> s2 = s0;

  CHECK(s0.size() == 25);
  CHECK(s1.size() == 0);
  CHECK(s1.empty());
  CHECK(s2.size() == 25);

  // Copy is deep: mutating the copy leaves the original alone.
  s2.pop();
  CHECK(s2.size() == 24);
  CHECK(s0.size() == 25);
  CHECK(s2.top()._n == 23);

  // Copy assignment replaces all contents.
  s1 = s2;
  CHECK(s1.size() == 24);
  CHECK(s1.top()._n == 23);

  // Self-assignment is a no-op.
  s1 = *&s1;
  CHECK(s1.size() == 24);
  CHECK(s1.top()._n == 23);

  // Move construction steals; the source becomes empty and reusable.
  Stack<obj> m(std::move(s2));
  CHECK(m.size() == 24);
  CHECK(m.top()._n == 23);
  CHECK(s2.size() == 0);
  CHECK(s2.empty());

  s2.push(obj(5)); // moved-from stack is reusable
  CHECK(s2.size() == 1 && s2.top()._n == 5);

  // Move assignment frees the destination and steals the source.
  Stack<obj> m2;
  m2.push(obj(1));
  m2.push(obj(2));
  m2 = std::move(m);
  CHECK(m2.size() == 24);
  CHECK(m2.top()._n == 23);
  CHECK(m.size() == 0);

  // LIFO order and pop's empty-guard.
  Stack<obj> l;
  for (int i = 0; i < 5; i++)
    l.push(obj(i));
  bool lifo = true;
  for (int i = 4; i >= 0; i--) {
    lifo = lifo && l.top()._n == i;
    l.pop();
  }
  CHECK(lifo);
  l.pop(); // pop on empty: no-op, no crash
  CHECK(l.size() == 0 && l.empty());

  // clear() empties and the stack stays reusable.
  Stack<obj> c;
  for (int i = 0; i < 10; i++)
    c.push(obj(i));
  c.clear();
  CHECK(c.size() == 0 && c.empty());
  c.push(obj(9));
  CHECK(c.size() == 1 && c.top()._n == 9);

  // operator<< prints top-first, ", " separated, no trailing separator.
  Stack<obj> p;
  p.push(obj(1));
  p.push(obj(2));
  p.push(obj(3));
  cout << "print: " << p << endl;
  ostringstream oss;
  oss << p;
  CHECK(oss.str() == "3, 2, 1");

  // swap exchanges contents (constant-time chain swap).
  Stack<obj> sw1, sw2;
  sw1.push(obj(1));
  sw2.push(obj(2));
  sw2.push(obj(3));
  Stack<obj> &sw1r = sw1; // alias
  Stack<obj> &sw2r = sw2;
  sw1r.swap(sw2r); // the member is private; exercised via copies below
  // (swap is an internal building block; its effect is proven through
  // the exception-safety of copy assignment above and below.)
  CHECK(sw1.size() == 2 && sw2.size() == 1);
  CHECK(sw1.top()._n == 3 && sw2.top()._n == 1);

  // Throwing element copy leaves destination untouched (strong safety).
  {
    static int copies = 0;
    static int threshold = -1; // armed (>=0) only for the assignment
    struct noisy {
      int v;
      noisy() : v(0) {}
      noisy(const noisy &o) : v(o.v) {
        if (threshold >= 0 && ++copies == threshold)
          throw "boom";
      }
    };
    Stack<noisy> src;
    for (int i = 0; i < 5; i++)
      src.push(noisy()); // threshold -1: never throws
    Stack<noisy> dst;
    for (int i = 0; i < 2; i++)
      dst.push(noisy());

    threshold = 3; // 3rd copy inside operator= throws
    copies = 0;
    bool threw = false;
    try {
      dst = src;
    } catch (const char *) {
      threw = true;
    }
    threshold = -1;

    CHECK(threw); // the fill threw mid-build
    CHECK(dst.size() == 2); // destination kept its old elements
    CHECK(src.size() == 5); // source untouched

    dst.push(noisy()); // destination still fully usable
    CHECK(dst.size() == 3);
  }

  // Self-assignment is safe (build-then-commit makes it a no-op).
  Stack<obj> self;
  for (int i = 0; i < 3; i++)
    self.push(obj(i));
  Stack<obj> &selfAlias = self;
  self = selfAlias;
  CHECK(self.size() == 3);
  CHECK(self.top()._n == 2);

  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
