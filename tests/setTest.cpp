// Tests for Set<T>. Prints one line per check and exits nonzero on the
// first failure. Build and run from tests/:  make setTest && ./setTest

#include "Set.h"
#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>

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
static unsigned seed = 271828182u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Fresh set.
  {
    Set<int> s;
    CHECK(s.empty());
    CHECK(s.size() == 0);
    CHECK(!s.contains(1));
    CHECK(!s.erase(1)); // erase on empty
    CHECK(s == s);      // self equality
  }

  // insert(): uniqueness, sorted order, return value.
  {
    Set<int> s;
    CHECK(s.insert(5));
    CHECK(s.insert(1));
    CHECK(s.insert(9));
    CHECK(s.insert(5) == false); // duplicate reports false
    CHECK(s.size() == 3);

    // ascending order, deterministic regardless of insert order
    CHECK(s[0] == 1 && s[1] == 5 && s[2] == 9);

    // interleaved duplicates never inflate the count: adding
    // {2, 0, 1} yields {0,1,2,5,9} (1 was already a member)
    s.insert(2); s.insert(0); s.insert(1); s.insert(1); s.insert(2);
    CHECK(s.size() == 5);
    CHECK(s[0] == 0 && s[1] == 1 && s[2] == 2 && s[3] == 5 && s[4] == 9);
  }

  // erase(): exact removal, size tracking, exists patterns.
  {
    Set<int> s;
    for (int i = 0; i < 50; i++)
      s.insert(i);
    CHECK(s.size() == 50);

    CHECK(s.erase(0));
    CHECK(s.erase(49));
    CHECK(!s.erase(0)); // already gone
    CHECK(s.size() == 48);

    // erase the evens (2..48: twenty-four evens)
    for (int i = 2; i < 50; i += 2)
      CHECK(s.erase(i));
    CHECK(s.size() == 24);

    bool onlyOdds = true;
    for (int i = 0; i < 50; i++)
      if (i % 2 == 0 && s.contains(i))
        onlyOdds = false;
    CHECK(onlyOdds);

    // drain the odds (49 was already erased above)
    for (int i = 1; i < 49; i += 2)
      CHECK(s.erase(i));
    CHECK(s.empty());
    s.insert(7); // reusable
    CHECK(s.size() == 1 && s.contains(7));
  }

  // Growth: many inserts exercise doubling; order and membership hold.
  {
    Set<int> s;
    for (int i = 0; i < 1000; i++)
      CHECK(s.insert(i));
    CHECK(s.insert(500) == false); // already there
    CHECK(s.size() == 1000);

    bool sorted = true;
    for (size_t i = 1; i < s.size(); i++)
      if (!(s[i - 1] < s[i]))
        sorted = false;
    CHECK(sorted);

    for (int i = 0; i < 1000; i++)
      if (!s.contains(i))
        CHECK(false);
    CHECK(true);

    // reverse insert order produces the same sorted set
    Set<int> r;
    for (int i = 999; i >= 0; i--)
      r.insert(i);
    CHECK(r == s); // insertion order does not matter
  }

  // at() bounds checking; operator[] unchecked contract.
  {
    Set<int> s;
    s.insert(1);
    s.insert(2);
    CHECK(s.at(0) == 1);
    CHECK(s.at(1) == 2);

    bool threw = false;
    try { s.at(2); } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { s.at((size_t)-1); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // setUnion: overlap, disjoint, subset, superset cases.
  {
    Set<int> a, b;
    a.insert(1); a.insert(2); a.insert(3);
    b.insert(3); b.insert(4); b.insert(5);

    Set<int> u = a.setUnion(b);
    CHECK(u.size() == 5);
    CHECK(u[0] == 1 && u[1] == 2 && u[2] == 3 && u[3] == 4 && u[4] == 5);

    Set<int> disjoint1, disjoint2;
    disjoint1.insert(1);
    disjoint2.insert(2);
    CHECK(disjoint1.setUnion(disjoint2).size() == 2);

    // subset union equals the superset
    CHECK(a.setUnion(b).setUnion(Set<int>()) ==
          (a.setUnion(b))); // union with empty is identity
    auto ab = a.setUnion(b);
    CHECK(ab.setUnion(a) == ab); // absorbing duplicate
  }

  // setIntersection.
  {
    Set<int> a, b;
    a.insert(1); a.insert(2); a.insert(3);
    b.insert(3); b.insert(4); b.insert(5);

    Set<int> i = a.setIntersection(b);
    CHECK(i.size() == 1 && i[0] == 3);

    // disjoint: empty intersection
    Set<int> c, d;
    c.insert(1);
    d.insert(2);
    CHECK(c.setIntersection(d).empty());

    // with self is self
    CHECK(a.setIntersection(a) == a);

    // commutativity
    CHECK(a.setIntersection(b) == b.setIntersection(a));
  }

  // setDifference.
  {
    Set<int> a, b;
    a.insert(1); a.insert(2); a.insert(3);
    b.insert(3); b.insert(4); b.insert(5);

    Set<int> d1 = a.setDifference(b); // in a, not in b
    CHECK(d1.size() == 2 && d1[0] == 1 && d1[1] == 2);

    Set<int> d2 = b.setDifference(a);
    CHECK(d2.size() == 2 && d2[0] == 4 && d2[1] == 5);

    // self-difference is empty
    CHECK(a.setDifference(a).empty());

    // a - b + b = union (when b's extras don't overlap a)
    CHECK(a.setDifference(b).setUnion(b) == a.setUnion(b));
  }

  // isSubsetOf.
  {
    Set<int> a, b, c;
    a.insert(1); a.insert(2);
    b.insert(1); b.insert(2); b.insert(3);
    c.insert(1); c.insert(9);

    CHECK(a.isSubsetOf(b));
    CHECK(!b.isSubsetOf(a));
    CHECK(!c.isSubsetOf(b));  // 9 not in b
    CHECK(b.isSubsetOf(b));   // reflexive
    CHECK(a.isSubsetOf(a));

    Set<int> emptySet;
    CHECK(emptySet.isSubsetOf(a)); // empty set is a subset of anything
    CHECK(emptySet.isSubsetOf(emptySet));
    CHECK(!a.isSubsetOf(emptySet));
  }

  // operator== / operator!=.
  {
    Set<int> a, b;
    a.insert(1); a.insert(2);
    b.insert(2); b.insert(1);
    CHECK(a == b); // order of insertion irrelevant

    Set<int> c;
    c.insert(1); c.insert(3);
    CHECK(a != c);

    Set<int> bigger;
    bigger.insert(1); bigger.insert(2); bigger.insert(3);
    CHECK(a != bigger);
  }

  // Copy construction is deep and independent.
  {
    Set<string> a;
    a.insert("pear");
    a.insert("apple");
    a.insert("orange");
    Set<string> b(a);
    CHECK(b.size() == 3);
    CHECK(b[0] == "apple" && b[2] == "pear"); // sorted

    b.erase("apple");
    b.insert("banana");
    CHECK(a.size() == 3);        // a untouched
    CHECK(a.contains("apple")); // and still has apple
    CHECK(b[0] == "banana");
  }

  // Copy assignment replaces; empty-to-empty safe.
  {
    Set<int> a;
    a.insert(1); a.insert(2); a.insert(3);
    Set<int> b;
    b.insert(99);
    b = a;
    CHECK(b.size() == 3);
    CHECK(b[0] == 1);
    CHECK(!b.contains(99));

    Set<int> e;
    e = a;
    CHECK(e == a);

    e.clear();
    Set<int> f;
    f = e; // assign empty over empty
    CHECK(f.empty());

    // self-assignment no-op
    Set<int> &alias = a;
    a = alias;
    CHECK(a.size() == 3);
    CHECK(a[0] == 1);
  }

  // Move construction steals; source empty and reusable.
  {
    Set<int> a;
    for (int i = 0; i < 10; i++)
      a.insert(i);
    Set<int> m(std::move(a));
    CHECK(m.size() == 10);
    CHECK(m[0] == 0 && m[9] == 9);

    CHECK(a.empty());
    a.insert(50); // reusable
    CHECK(a.size() == 1 && a.contains(50));
  }

  // Move assignment frees destination and steals; self-move safe.
  {
    Set<int> a;
    a.insert(1); a.insert(2);
    Set<int> b;
    b.insert(66);
    b = std::move(a);
    CHECK(b.size() == 2);
    CHECK(b[0] == 1 && b[1] == 2);
    CHECK(!b.contains(66));

    CHECK(a.empty());
    a.insert(9); // reusable
    CHECK(a.size() == 1);

    Set<int> s;
    s.insert(3);
    Set<int> &alias = s;
    s = std::move(alias); // self-move no-op
    CHECK(s.size() == 1 && s.contains(3));
  }

  // swap exchanges (member and free).
  {
    Set<int> s1, s2;
    s1.insert(1);
    s2.insert(2); s2.insert(3);
    s1.swap(s2);
    CHECK(s1.size() == 2 && s2.size() == 1);
    CHECK(s1[0] == 2);

    swap(s1, s2);
    CHECK(s1.size() == 1 && s2.size() == 2);
    CHECK(s1[0] == 1);
  }

  // clear() empties and stays reusable.
  {
    Set<int> c;
    for (int i = 0; i < 10; i++)
      c.insert(i);
    c.clear();
    CHECK(c.empty());
    CHECK(!c.contains(5));
    c.insert(5); // reusable
    CHECK(c.size() == 1);
    c.clear();
    c.clear(); // idempotent
    CHECK(c.empty());
  }

  // Iterator pair works with std algorithms (contiguous, sorted).
  {
    Set<int> s;
    s.insert(4); s.insert(2); s.insert(7); s.insert(1);

    CHECK(std::find(s.begin(), s.end(), 7) == s.begin() + 3);
    CHECK(std::find(s.begin(), s.end(), 5) == s.end());

    // std::accumulate equivalent by hand
    long sum = 0;
    for (int *it = s.begin(); it != s.end(); ++it)
      sum += *it;
    CHECK(sum == 14); // 1+2+4+7

    // range insert from another container
    Set<int> t;
    t.insert(s.begin(), s.end());
    CHECK(t == s);

    Set<int> u;
    u.insert(s.begin(), s.begin() + 2); // partial
    CHECK(u.size() == 2 && u[0] == 1 && u[1] == 2);
  }

  // operator<< prints mathematical set notation.
  {
    Set<int> s;
    s.insert(2); s.insert(1);
    ostringstream oss;
    oss << s;
    CHECK(oss.str() == "{ 1, 2 }");

    Set<int> e;
    ostringstream ossE;
    ossE << e;
    CHECK(ossE.str() == "{}");

    Set<string> w;
    w.insert("hello");
    ostringstream ossW;
    ossW << w;
    CHECK(ossW.str() == "{ hello }");
  }

  // Randomized stress: Set vs std::set model through random
  // insert/erase with union/intersection/difference cross-checks.
  {
    bool allOk = true;
    for (int trial = 0; trial < 30; trial++) {
      Set<int> mine;
      set<int> model;
      const int UNIVERSE = 200;

      for (int op = 0; op < 600; op++) {
        int k = int(rnd() % UNIVERSE);
        int roll = int(rnd() % 3);
        if (roll == 0) {
          bool added = mine.insert(k);
          bool modelAdded = model.insert(k).second;
          if (added != modelAdded)
            allOk = false;
        } else if (roll == 1) {
          bool removed = mine.erase(k);
          bool modelRemoved = model.erase(k) > 0;
          if (removed != modelRemoved)
            allOk = false;
        } else {
          if (mine.contains(k) != (model.count(k) > 0))
            allOk = false;
        }
        if (mine.size() != model.size())
          allOk = false;
      }

      // final: exact elementwise comparison against the model
      bool same = true;
      auto it = model.begin();
      for (size_t idx = 0; idx < mine.size(); ++idx, ++it)
        if (it == model.end() || mine[idx] != *it)
          same = false;
      if (it != model.end() || !same)
        allOk = false;

      // set-operation identity: U = (A-B) u (A^B) u (B-A)
      Set<int> subA = mine.setDifference(Set<int>()); // copy of mine
      Set<int> modelSet;
      for (std::set<int>::const_iterator mit = model.begin();
           mit != model.end(); ++mit)
        modelSet.insert(*mit);

      Set<int> diffAndInterAndBack =
          modelSet.setDifference(mine).setUnion(
              mine.setIntersection(modelSet)).setUnion(
                  mine.setDifference(modelSet));
      Set<int> full = mine.setUnion(modelSet);
      if (!(diffAndInterAndBack == full))
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
