// Tests for SkipList<K, V>. Prints one line per check and exits nonzero
// on the first failure. Build and run from tests/:
//   make skipListTest && ./skipListTest
#include <sstream>


#include <iostream>
#include <string>
#include <utility>

using namespace std;

#include "SkipList.h"

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
static unsigned seed = 777777777u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Empty list basics.
  {
    SkipList<int, string> sl;
    CHECK(sl.empty());
    CHECK(sl.size() == 0);
    CHECK(!sl.exists(1));

    bool threw = false;
    try { sl.find(1); } catch (Exception &) { threw = true; }
    CHECK(threw); // find on empty throws

    CHECK(!sl.erase(1)); // erase on empty reports false
  }

  // Insert, find, ordered traversal.
  {
    SkipList<int, string> sl;
    sl.insert(5, "five");
    sl.insert(1, "one");
    sl.insert(3, "three");
    sl.insert(2, "two");
    sl.insert(4, "four");
    CHECK(sl.size() == 5);
    CHECK(!sl.empty());

    CHECK(sl.find(1) == "one");
    CHECK(sl.find(5) == "five");
    CHECK(sl.find(3) == "three");

    bool threw = false;
    try { sl.find(42); } catch (Exception &) { threw = true; }
    CHECK(threw);

    // for_each visits ascending by key.
    bool sorted = true;
    int prev = -1000000;
    sl.for_each([&](int k, const string &) {
      if (k < prev)
        sorted = false;
      prev = k;
    });
    CHECK(sorted);

    bool sawAll = true;
    sl.for_each([&](int k, const string &v) {
      if ((k == 1 && v != "one") || (k == 2 && v != "two") ||
          (k == 3 && v != "three") || (k == 4 && v != "four") ||
          (k == 5 && v != "five"))
        sawAll = false;
    });
    CHECK(sawAll);
  }

  // insert() on an existing key REPLACES the value, keeping size stable.
  {
    SkipList<int, string> sl;
    sl.insert(10, "ten");
    sl.insert(10, "TEN");
    sl.insert(10, "ten again");
    CHECK(sl.size() == 1);
    CHECK(sl.find(10) == "ten again");
  }

  // erase() unlinks correctly (first, middle, last), shrinks size, and
  // falsifies exists afterwards.
  {
    SkipList<int, string> sl;
    for (int i = 0; i < 10; i++)
      sl.insert(i, "x");
    CHECK(sl.size() == 10);

    CHECK(sl.erase(0)); // first
    CHECK(sl.erase(9)); // last
    CHECK(sl.erase(5)); // middle
    CHECK(sl.size() == 7);
    CHECK(!sl.erase(0)); // already gone
    CHECK(!sl.erase(9));
    CHECK(!sl.erase(5));
    CHECK(sl.size() == 7);

    CHECK(!sl.exists(0) && !sl.exists(5) && !sl.exists(9));
    CHECK(sl.exists(1) && sl.exists(2) && sl.exists(8));

    CHECK(sl.erase(1) && sl.erase(2) && sl.erase(3) && sl.erase(4) &&
          sl.erase(6) && sl.erase(7) && sl.erase(8));
    CHECK(sl.empty());
    CHECK(sl.size() == 0);
    // still usable after emptying
    sl.insert(100, "hundred");
    CHECK(sl.size() == 1 && sl.find(100) == "hundred");
  }

  // clear() empties and the list stays usable.
  {
    SkipList<int, string> sl;
    for (int i = 0; i < 20; i++)
      sl.insert(i, "v");
    sl.clear();
    CHECK(sl.empty());
    CHECK(sl.size() == 0);
    CHECK(!sl.exists(0));
    sl.insert(7, "seven"); // still usable
    CHECK(sl.size() == 1);
    CHECK(sl.find(7) == "seven");
  }

  // String keys (ordered by operator<).
  {
    SkipList<string, int> sl;
    sl.insert("delta", 4);
    sl.insert("alpha", 1);
    sl.insert("charlie", 3);
    sl.insert("bravo", 2);
    CHECK(sl.size() == 4);
    CHECK(sl.find("alpha") == 1);

    bool sorted = true;
    string prev;
    bool first = true;
    sl.for_each([&](const string &k, int) {
      if (!first && k < prev)
        sorted = false;
      prev = k;
      first = false;
    });
    CHECK(sorted);
  }

  // Copy construction is deep and independent.
  {
    SkipList<int, string> a;
    for (int i = 0; i < 30; i++)
      a.insert(i * 2, "even");

    SkipList<int, string> b(a);
    CHECK(b.size() == 30);
    CHECK(b.find(28) == "even");
    CHECK(!b.exists(29));

    b.insert(29, "odd");
    b.erase(0);
    CHECK(b.size() == 30);
    CHECK(a.size() == 30); // a untouched
    CHECK(a.find(0) == "even"); // a's 0 still there
    CHECK(b.find(29) == "odd"); // b has its own 29
    CHECK(!a.exists(29));

    // traversal order preserved in the copy
    bool sorted = true;
    int prev = -1;
    b.for_each([&](int k, const string &) {
      if (k != 29 && (k <= prev || prev == -1))
        ; // ordering guard below
      if (k < prev)
        sorted = false;
      prev = k;
    });
    CHECK(sorted);
  }

  // Copy assignment replaces contents and is independent after.
  {
    SkipList<int, string> a;
    for (int i = 0; i < 10; i++)
      a.insert(i, "v");
    SkipList<int, string> b;
    b.insert(999, "stale");
    b = a;
    CHECK(b.size() == 10);
    CHECK(b.find(0) == "v");
    CHECK(!b.exists(999));

    b.insert(10, "ten");
    b.erase(5);
    CHECK(b.size() == 10);
    CHECK(a.size() == 10); // alias-free
    CHECK(!a.exists(10));
    CHECK(a.exists(5));
  }

  // Self-assignment is a no-op.
  {
    SkipList<int, string> a;
    a.insert(1, "one");
    a.insert(2, "two");
    SkipList<int, string> &alias = a;
    a = alias;
    CHECK(a.size() == 2);
    CHECK(a.find(1) == "one");
    CHECK(a.find(2) == "two");
  }

  // Move construction steals; source is empty and reusable.
  {
    SkipList<int, string> a;
    for (int i = 0; i < 15; i++)
      a.insert(i, "v");
    SkipList<int, string> m(std::move(a));
    CHECK(m.size() == 15);
    CHECK(m.find(0) == "v");
    CHECK(m.find(14) == "v");

    CHECK(a.size() == 0);
    CHECK(a.empty());

    a.insert(50, "fifty"); // reusable
    CHECK(a.size() == 1);
    CHECK(a.find(50) == "fifty");
  }

  // Move assignment frees destination contents and steals the source.
  {
    SkipList<int, string> a;
    for (int i = 0; i < 8; i++)
      a.insert(i, "v");
    SkipList<int, string> b;
    b.insert(77, "stale");
    b = std::move(a);
    CHECK(b.size() == 8);
    CHECK(b.find(0) == "v");
    CHECK(!b.exists(77));

    CHECK(a.size() == 0);
    a.insert(9, "nine"); // reusable
    CHECK(a.size() == 1);
    CHECK(a.find(9) == "nine");
  }

  // Self-move is a no-op.
  {
    SkipList<int, string> a;
    a.insert(1, "one");
    SkipList<int, string> &alias = a;
    a = std::move(alias);
    CHECK(a.size() == 1);
    CHECK(a.find(1) == "one");
  }

  // swap exchanges contents.
  {
    SkipList<int, string> s1, s2;
    s1.insert(1, "one");
    s2.insert(2, "two");
    s2.insert(3, "three");
    s1.swap(s2);
    CHECK(s1.size() == 2 && s2.size() == 1);
    CHECK(s1.find(2) == "two" && s2.find(1) == "one");
  }

  // Large-key stress: 2000 keys in, exactly out, all findable, order
  // holds through insert/erase interleaving.
  {
    bool allOk = true;
    const int N = 2000;
    SkipList<int, int> sl;
    for (int i = 0; i < N; i++)
      sl.insert(i, i * 3);
    CHECK(sl.size() == N);

    for (int i = 0; i < N; i += 2) // erase all evens
      if (!sl.erase(i))
        allOk = false;
    if (sl.size() != N / 2)
      allOk = false;

    for (int i = 0; i < N; i++) {
      bool should = (i % 2) == 1;
      if (sl.exists(i) != should)
        allOk = false;
    }

    // erase remaining odds down to empty
    for (int i = 1; i < N; i += 2)
      if (!sl.erase(i))
        allOk = false;
    if (!sl.empty())
      allOk = false;
    CHECK(allOk);
  }

  // Randomized stress against a sorted reference model: random
  // insert/replace/erase/find over a sparse key space, checking exists,
  // find, size, and ascending traversal after each phase.
  {
    bool allOk = true;
    const int KEYS = 300;
    for (int trial = 0; trial < 20; trial++) {
      SkipList<int, int> sl;
      // model: count per key (0 = absent); values tracked separately
      int model[KEYS];
      for (int i = 0; i < KEYS; i++)
        model[i] = -1; // -1: absent
      int live = 0;

      for (int op = 0; op < 1500; op++) {
        int k = static_cast<int>(rnd() % KEYS);
        int roll = static_cast<int>(rnd() % 4);

        if (roll == 0) { // insert or replace
          int v = static_cast<int>(rnd() % 1000000);
          sl.insert(k, v);
          if (model[k] == -1)
            live++;
          model[k] = v;
        } else if (roll == 1) { // erase
          bool erased = sl.erase(k);
          if (erased != (model[k] != -1))
            allOk = false;
          if (model[k] != -1) {
            model[k] = -1;
            live--;
          }
        } else if (roll == 2) { // exists
          if (sl.exists(k) != (model[k] != -1))
            allOk = false;
        } else { // find (or expect a throw)
          if (model[k] != -1) {
            if (sl.find(k) != model[k])
              allOk = false;
          } else {
            bool threw = false;
            try { sl.find(k); } catch (Exception &) { threw = true; }
            if (!threw)
              allOk = false;
          }
        }

        if (static_cast<u64>(live) != sl.size())
          allOk = false;
      }

      // ascending traversal must cover exactly the live keys in order
      bool sorted = true;
      int prev = -1;
      int seen = 0;
      sl.for_each([&](int k, int v) {
        if (k < prev || model[k] == -1 || model[k] != v)
          sorted = false;
        model[k] = -1; // mark seen
        prev = k;
        seen++;
      });
      if (!sorted || seen != live)
        allOk = false;
    }
    CHECK(allOk);
  }

  // operator<< smoke test: contains all pairs, did not mutate.
  {
    SkipList<int, string> sl;
    sl.insert(1, "one");
    sl.insert(2, "two");
    ostringstream os;
    os << sl;
    CHECK(os.str().find("1 -> one") != string::npos);
    CHECK(os.str().find("2 -> two") != string::npos);
    CHECK(sl.size() == 2);
  }

  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
