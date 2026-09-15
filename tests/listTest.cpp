// Tests for List<T>. Prints one line per check and exits nonzero on the
// first failure. Build and run from tests/:  make listTest && ./listTest

#include "List.h"
#include <algorithm>
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

// Equality for std::find.
bool operator==(const obj &a, const obj &b) { return a._n == b._n; }

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
  List<obj> a0;
  List<obj> a1(a0);
  List<obj> a2(100);
  List<obj> a3(a2);
  List<obj> a4(obj(100), 100);
  List<obj> a5 = a4;

  CHECK(a0.size() == 0);
  CHECK(a0.empty());
  CHECK(a1.size() == 0);
  CHECK(a2.size() == 100);
  CHECK(a3.size() == 100);
  CHECK(a4.size() == 100);
  CHECK(a5.size() == 100);

  // Fill ctor copies the value into every element.
  CHECK(a4.front()._n == 100 && a4.back()._n == 100);

  // Copy construction is deep: erasing a3's first node leaves a2 intact.
  a3.erase(a3.begin());
  CHECK(a3.size() == 99);
  CHECK(a2.size() == 100);

  // Copy assignment between different sizes replaces all contents.
  List<obj> small(1);
  small = a4;
  CHECK(small.size() == 100);
  CHECK(small.front()._n == 100);

  // Self-assignment is a no-op.
  a4 = *&a4;
  CHECK(a4.size() == 100);
  CHECK(a4.front()._n == 100);

  // clear() empties every list; cleared lists stay reusable (the
  // sequence that used to crash when the sentinel kept pointing at
  // freed nodes).
  a0.clear();
  a1.clear();
  a2.clear();
  a3.clear();
  a4.clear();
  a5.clear();

  CHECK(a0.size() == 0 && a0.empty());
  CHECK(a1.size() == 0 && a1.empty());
  CHECK(a2.size() == 0 && a2.empty());
  CHECK(a3.size() == 0 && a3.empty());
  CHECK(a4.size() == 0 && a4.empty());
  CHECK(a5.size() == 0 && a5.empty());

  a0.push_back(1);
  CHECK(a0.size() == 1 && a0.front()._n == 1 && a0.back()._n == 1);
  a0.clear();

  // Move construction steals; source becomes empty and reusable.
  List<obj> m4(obj(42), 4);
  List<obj> m(std::move(m4));
  CHECK(m.size() == 4);
  CHECK(m.front()._n == 42 && m.back()._n == 42);
  CHECK(m4.size() == 0 && m4.empty());

  m4.push_back(9); // moved-from list is reusable
  CHECK(m4.size() == 1 && m4.front()._n == 9);

  // Move assignment frees destination contents and steals the source.
  List<obj> m2(10);
  m2 = std::move(m);
  CHECK(m2.size() == 4);
  CHECK(m2.front()._n == 42);
  CHECK(m.size() == 0 && m.empty());

  // resize grows with zero-filled elements and shrinks from the back.
  a0.resize(999);
  CHECK(a0.size() == 999);
  a0.resize(99);
  CHECK(a0.size() == 99);
  a0.clear();

  // push_back/pop_back and back().
  for (int i = 0; i < 99; i++) {
    a0.push_back(i);
  }
  CHECK(a0.size() == 99);
  CHECK(a0.back()._n == 98);

  a0.clear();
  CHECK(a0.size() == 0 && a0.empty());

  a0.resize(50);
  CHECK(a0.size() == 50);

  for (int i = 0; i < 25; i++) {
    a0.pop_back();
  }
  CHECK(a0.size() == 25);

  a0.push_back(10000);
  CHECK(a0.back()._n == 10000);
  CHECK(a0.size() == 26);

  // push_front works on non-empty AND empty lists.
  for (int i = 0; i < 25; i++) {
    a0.push_front(i);
  }
  CHECK(a0.size() == 51);
  CHECK(a0.front()._n == 24);

  for (int i = 0; i < 25; i++) {
    a0.pop_front();
  }
  CHECK(a0.size() == 26);
  CHECK(a0.back()._n == 10000); // the 10000 sits at the back

  a0.push_front(10000);
  CHECK(a0.front()._n == 10000);
  CHECK(a0.size() == 27);

  a0.clear();
  a0.push_front(5); // push_front on empty list
  CHECK(a0.size() == 1 && a0.front()._n == 5);

  // insert: on an empty list, in the middle, at begin, at end.
  List<obj> e;
  e.insert(e.begin(), 5);
  CHECK(e.size() == 1 && e.front()._n == 5); // used to segfault

  List<obj> ins;
  for (int i = 0; i < 27; i++)
    ins.push_back(i);
  List<obj>::iterator it = ins.begin();
  for (int i = 0; i < 15; i++)
    it++;
  ins.insert(it, 5555); // inserts BEFORE it (STL contract)
  CHECK(ins.size() == 28);

  ins.insert(ins.begin(), -1); // new front
  CHECK(ins.front()._n == -1);

  ins.insert(ins.end(), 9999); // new back
  CHECK(ins.back()._n == 9999);
  CHECK(ins.size() == 30);

  // erase: returns an iterator to the element that followed.
  List<obj> er;
  er.push_back(1);
  er.push_back(2);
  er.push_back(3);
  List<obj>::iterator ex = er.begin();
  ++ex; // element 2
  List<obj>::iterator after = er.erase(ex);
  CHECK(er.size() == 2);
  CHECK(after->_n == 3); // following element

  List<obj>::iterator fb = er.erase(er.begin());
  CHECK(fb->_n == 3); // erase(begin()) returns the new front
  CHECK(er.size() == 1);

  er.erase(er.end()); // erase(end()) is a no-op
  CHECK(er.size() == 1);

  er.push_back(4);
  CHECK(er.back()._n == 4); // ring consistent after erases

  // Iterators: forward and backward walk, STL algorithms.
  List<obj> walk;
  for (int i = 0; i < 5; i++)
    walk.push_back(i * 2);
  int seen = 0, sum = 0;
  for (List<obj>::iterator wi = walk.begin(); wi != walk.end(); wi++) {
    seen++;
    sum += wi->_n;
  }
  CHECK(seen == 5);
  CHECK(sum == 0 + 2 + 4 + 6 + 8);

  // --end() names the last element (ring property).
  List<obj>::iterator last = walk.end();
  --last;
  CHECK(last->_n == 8);

  // std::find and std::reverse accept the iterators.
  List<obj>::iterator f = std::find(walk.begin(), walk.end(), obj(4));
  CHECK(f != walk.end() && f->_n == 4);
  std::reverse(walk.begin(), walk.end());
  CHECK(walk.front()._n == 8 && walk.back()._n == 0);

  // Iterators of different lists never compare equal.
  List<obj> other;
  other.push_back(0);
  CHECK(!(walk.begin() == other.begin()));

  // swap exchanges contents in constant time (member and free).
  List<obj> sa, sb;
  sa.push_back(1);
  sb.push_back(2);
  sb.push_back(3);
  sa.swap(sb);
  CHECK(sa.size() == 2 && sb.size() == 1);
  CHECK(sa.front()._n == 2 && sb.front()._n == 1);
  swap(sa, sb);
  CHECK(sa.size() == 1 && sb.size() == 2);

  // operator<< prints front-to-back, ", " separated, no trailing sep.
  ostringstream oss;
  List<obj> pr;
  pr.push_back(0);
  pr.push_back(2);
  pr.push_back(4);
  oss << pr;
  CHECK(oss.str() == "0, 2, 4");

  // Empty list prints nothing; begin() == end() on empty.
  ostringstream ess;
  ess << List<obj>();
  CHECK(ess.str() == "");
  List<obj> emp;
  CHECK(emp.begin() == emp.end());

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
    List<noisy> src_l;
    for (int i = 0; i < 4; i++)
      src_l.push_back(noisy()); // threshold -1: never throws
    List<noisy> dst_l;
    for (int i = 0; i < 2; i++)
      dst_l.push_back(noisy());

    threshold = 3; // 3rd copy inside operator= throws
    copies = 0;
    bool threw = false;
    try {
      dst_l = src_l;
    } catch (const char *) {
      threw = true;
    }
    threshold = -1;

    CHECK(threw); // the fill threw mid-build
    CHECK(dst_l.size() == 2); // destination kept its old elements
    CHECK(src_l.size() == 4); // source untouched

    dst_l.push_back(noisy()); // destination still fully usable
    CHECK(dst_l.size() == 3);
  }

  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
