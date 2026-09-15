// Tests for Array<T>. Prints one line per check and exits nonzero on the
// first failure. Build and run from tests/:  make arrayTest && ./arrayTest

#include "Array.h"
#include <algorithm> // std::sort
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

// Ordering for std::sort on Array<obj>.
bool operator<(const obj &a, const obj &b) { return a._n < b._n; }

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

// The counter typedef must be 64 bits wide (u64 from types.h).
static_assert(sizeof(Array<obj>::size_type) == 8,
              "Array::size_type must be 64 bits");

int main() {
  // Construction and size.
  Array<obj> empty_;
  Array<obj> zero(0);
  Array<obj> a(3);
  Array<obj> filled(obj(7), 3);

  CHECK(empty_.size() == 0);
  CHECK(empty_.empty());
  CHECK(zero.size() == 0);
  CHECK(a.size() == 3);
  CHECK(!a.empty());
  CHECK(filled.size() == 3);

  // Elements are value-initialized: an Array<obj> starts zeroed.
  CHECK(a[0]._n == 0 && a[1]._n == 0 && a[2]._n == 0);

  // Fill ctor copies the value into every element.
  CHECK(filled[0]._n == 7 && filled[1]._n == 7 && filled[2]._n == 7);

  // Copy construction is deep.
  Array<obj> copy(filled);
  CHECK(copy.size() == 3);
  CHECK(copy[1]._n == 7);
  copy[1] = obj(9);
  CHECK(filled[1]._n == 7);

  // Copy assignment between arrays of different sizes: the destination
  // is reallocated to the source's size (regression: old operator[]
  // wrote past a buffer sized for the old size).
  Array<obj> small(1);
  Array<obj> big(100);
  for (int i = 0; i < 100; ++i)
    big[i] = obj(i);

  small = big;
  CHECK(small.size() == 100);
  CHECK(small[99]._n == 99);

  // ...and in the other direction.
  big = filled;
  CHECK(big.size() == 3);
  CHECK(big[2]._n == 7);

  // Self-assignment is a no-op.
  filled = *&filled;
  CHECK(filled.size() == 3);
  CHECK(filled[0]._n == 7);

  // Move construction steals and empties the source.
  Array<obj> moved(std::move(filled));
  CHECK(moved.size() == 3);
  CHECK(moved[0]._n == 7);
  CHECK(filled.size() == 0);
  CHECK(filled.empty());

  // Move assignment releases the destination's old buffer, even when
  // that buffer has size 0 (regression: old code skipped the delete
  // when _n == 0, leaking clear()'s zero-length allocation).
  Array<obj> dst(5);
  dst.clear();
  CHECK(dst.size() == 0); // but dst still owns a buffer here
  dst = std::move(moved);
  CHECK(dst.size() == 3);
  CHECK(dst[0]._n == 7);
  CHECK(moved.size() == 0);

  // A moved-from array remains usable.
  moved.resize(2);
  moved[0] = obj(4);
  moved[1] = obj(5);
  CHECK(moved.size() == 2 && moved[0]._n == 4 && moved[1]._n == 5);

  // swap exchanges contents in constant time.
  moved.swap(dst);
  CHECK(moved.size() == 3 && moved[0]._n == 7);
  CHECK(dst.size() == 2 && dst[0]._n == 4);

  // resize keeps the common prefix and value-initializes the rest.
  Array<obj> r(3);
  r[0] = obj(1);
  r[1] = obj(2);
  r[2] = obj(3);
  r.resize(5);
  CHECK(r.size() == 5);
  CHECK(r[0]._n == 1 && r[1]._n == 2 && r[2]._n == 3);
  CHECK(r[3]._n == 0 && r[4]._n == 0);

  // Shrinking keeps the first n elements.
  r.resize(2);
  CHECK(r.size() == 2 && r[0]._n == 1 && r[1]._n == 2);

  // resize(0) is equivalent to clear().
  r.resize(0);
  CHECK(r.size() == 0 && r.empty());

  // clear() releases memory; a cleared array is reusable.
  Array<obj> c(8);
  c.clear();
  CHECK(c.size() == 0 && c.empty());
  c.resize(2);
  c[0] = obj(6);
  CHECK(c.size() == 2 && c[0]._n == 6);

  // data() exposes the buffer; the const overload works on const arrays.
  Array<obj> d(4);
  const Array<obj> &cd = d;
  d.data()[0] = obj(11);
  CHECK(cd.data()[0]._n == 11);
  CHECK(cd[0]._n == 11);

  // Const access: operator[], empty(), size() all work on a const Array.
  CHECK(cd.size() == 4);
  CHECK(!cd.empty());
  CHECK(cd[0]._n == 11);

  // Iterators: forward and backward traversal, reading and writing.
  Array<obj> it(5);
  for (int i = 0; i < 5; ++i)
    it[i] = obj(i);

  int seen = 0, sum = 0;
  for (Array<obj>::iterator i = it.begin(); i != it.end(); ++i) {
    ++seen;
    sum += i->_n;
    *i = obj(i->_n * 2); // iterator writes reach the array
  }
  CHECK(seen == 5);
  CHECK(sum == 0 + 1 + 2 + 3 + 4);
  CHECK(it[4]._n == 8);

  // Post-decrement walks back from end().
  Array<obj>::iterator i = it.end();
  --i;
  CHECK(i->_n == 8);
  i -= 3;
  CHECK(i->_n == 2);
  i += 2;
  CHECK(i->_n == 6);

  // const_iterator reads a const Array.
  const Array<obj> &cit = it;
  seen = 0, sum = 0;
  for (Array<obj>::const_iterator ci = cit.begin(); ci != cit.end(); ci++) {
    ++seen;
    sum += ci->_n;
  }
  CHECK(seen == 5);
  CHECK(sum == 0 + 2 + 4 + 6 + 8);

  // operator<< prints all elements separated by ", ".
  cout << "print: " << it << endl;
  ostringstream oss;
  oss << it;
  CHECK(oss.str() == "0, 2, 4, 6, 8");

  // Empty arrays print nothing and iterate zero times.
  ostringstream ess;
  ess << Array<obj>();
  CHECK(ess.str() == "");


  // STL algorithms accept the iterators (random-access conformance).
  Array<obj> s(6);
  int svals[] = {5, 3, 9, 1, 7, 3};
  for (int i = 0; i < 6; ++i)
    s[i] = obj(svals[i]);
  std::sort(s.begin(), s.end());
  bool sortedOK = s[0]._n == 1 && s[1]._n == 3 && s[2]._n == 3 &&
                  s[3]._n == 5 && s[4]._n == 7 && s[5]._n == 9;
  CHECK(sortedOK);

  // Iterator arithmetic: distance, offset from either side, subscript.
  CHECK(s.end() - s.begin() == 6);
  CHECK((s.begin() + 2)->_n == 3);
  CHECK((2 + s.begin())->_n == 3);
  CHECK(s.begin()[4]._n == 7);
  CHECK(s.end()[-1]._n == 9);


  // A copy assignment whose element copy throws mid-fill must leave the
  // destination untouched (strong exception safety) and leak nothing.
  {
    int copies = 0;
    struct noisy {
      int v;
      int *copies;
      noisy() : v(0), copies(0) {}
      noisy(int *c) : v(0), copies(c) {}
      noisy(const noisy &o) : v(o.v), copies(o.copies) {}
      noisy &operator=(const noisy &o) {
        if (o.copies && ++*o.copies == 3)
          throw "boom";
        v = o.v;
        return *this;
      }
    };
    Array<noisy> src(4), dst(4);
    for (int i = 0; i < 4; ++i) {
      src[i].copies = &copies; // counter rides in the SOURCE elements
      src[i].v = i;            // ...so copies from them are counted
    }
    for (int i = 0; i < 4; ++i)
      dst[i].v = 100 + i; // old contents to verify preservation
    bool threw = false;
    try {
      dst = src;
    } catch (const char *) {
      threw = true;
    }
    CHECK(threw);
    CHECK(copies == 3); // third assignment threw
    CHECK(dst.size() == 4); // destination untouched by the failure
    CHECK(dst[0].v == 100 && dst[3].v == 103);
  }


  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
