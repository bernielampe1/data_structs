// Tests for Map<K, V>. Prints one line per check and exits nonzero on
// the first failure. Build and run from tests/:  make mapTest && ./mapTest

#include "Map.h"
#include <iostream>
#include <map> // std::map as the reference model
#include <sstream>
#include <string>
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
static unsigned seed = 987654321u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Fresh map.
  {
    Map<int, string> m;
    CHECK(m.empty());
    CHECK(m.size() == 0);
    CHECK(!m.exists(1));
    CHECK(!m.erase(1)); // erase on empty

    bool threw = false;
    try { m.find(1); } catch (Exception &) { threw = true; }
    CHECK(threw); // find on empty throws
  }

  // insert()/find(): store, retrieve, replace.
  {
    Map<int, string> m;
    CHECK(m.insert(5, "five"));   // new
    CHECK(!m.insert(5, "FIVE"));  // replace reports false
    CHECK(m.insert(1, "one"));
    CHECK(m.insert(9, "nine"));
    CHECK(m.size() == 3);

    CHECK(m.find(5) == "FIVE"); // replaced value
    CHECK(m.find(1) == "one");
    CHECK(m.find(9) == "nine");

    bool threw = false;
    try { m.find(42); } catch (Exception &) { threw = true; }
    CHECK(threw);

    // writing through find's mutable reference updates the stored value
    m.find(1) = "ONE";
    CHECK(m[1] == "ONE");
  }

  // operator[] read/write: absent key inserts a default; const throws.
  {
    Map<int, string> m;
    m[3] = "three";
    CHECK(m.size() == 1);
    CHECK(m[3] == "three");

    int before = m.size();
    m[99]; // read access materializes a default
    CHECK(m.size() == before + 1);
    CHECK(m[99] == ""); // default string
    m[99] = "ninety-nine";
    CHECK(m[99] == "ninety-nine");

    const Map<int, string> &cm = m;
    bool threw = false;
    try { cm[77]; } catch (Exception &) { threw = true; }
    CHECK(threw); // const operator[] of a missing key throws
    CHECK(cm[99] == "ninety-nine"); // and reads present keys fine
  }

  // erase(): exact removal; size tracks; reusable after emptying.
  {
    Map<int, string> m;
    for (int i = 0; i < 50; i++)
      m.insert(i, "x");
    CHECK(m.size() == 50);

    CHECK(m.erase(0));
    CHECK(m.erase(49));
    CHECK(!m.erase(0)); // already gone
    CHECK(m.size() == 48);

    // drain everything
    for (int i = 1; i < 49; i++)
      CHECK(m.erase(i));
    CHECK(m.empty());
    m.insert(7, "seven"); // reusable
    CHECK(m.size() == 1 && m.find(7) == "seven");
  }

  // Sorted iteration order regardless of insertion order.
  {
    Map<int, int> m;
    for (int i = 99; i >= 0; i--) // descending insert
      m[i] = i * 2;

    CHECK(m.size() == 100);
    bool ascending = true;
    int prev = -1;
    m.for_each([&](int k, int v) {
      if (k <= prev)
        ascending = false;
      if (v != k * 2)
        ascending = false;
      prev = k;
    });
    CHECK(ascending);

    // keyAt/valueAt index the sorted order
    CHECK(m.keyAt(0) == 0 && m.valueAt(0) == 0);
    CHECK(m.keyAt(99) == 99 && m.valueAt(99) == 198);
  }

  // for_each visits exactly the live entries after mutation.
  {
    Map<int, int> m;
    for (int i = 0; i < 20; i++)
      m[i] = i;
    m.erase(3);
    m.erase(7);
    m.erase(13);

    bool skip = true;
    int seen = 0;
    m.for_each([&](int k, int v) {
      if (k == 3 || k == 7 || k == 13)
        skip = false;
      if (v != k)
        skip = false;
      seen++;
    });
    CHECK(skip);
    CHECK(seen == 17);
  }

  // String keys, string values.
  {
    Map<string, int> m;
    m["gamma"] = 3;
    m["alpha"] = 1;
    m["beta"] = 2;
    CHECK(m.size() == 3);
    CHECK(m.find("alpha") == 1);

    bool sorted = true;
    string prev;
    bool first = true;
    m.for_each([&](const string &k, int) {
      if (!first && k <= prev)
        sorted = false;
      prev = k;
      first = false;
    });
    CHECK(sorted); // alphabetical: alpha, beta, gamma
  }

  // Growth: doubling under load keeps order and contents.
  {
    Map<int, int> m;
    for (int i = 0; i < 1000; i++)
      m.insert(i, i * 3);
    CHECK(m.insert(500, 1) == false); // existing key replaced, size same
    CHECK(m.size() == 1000);
    CHECK(m.find(500) == 1); // replaced

    bool ascending = true;
    int prev = -1;
    m.for_each([&](int k, int) {
      if (k <= prev && prev != -1)
        ascending = false;
      prev = k;
    });
    CHECK(ascending);

    for (int i = 0; i < 1000; i++)
      if (!m.exists(i))
        CHECK(false);
    CHECK(true);
  }

  // operator== / != (entry-wise; a replaced value is unequal).
  {
    Map<int, int> a, b;
    a[1] = 10; a[2] = 20;
    b[2] = 20; b[1] = 10;
    CHECK(a == b); // insertion order irrelevant

    b[1] = 11; // same keys, different value
    CHECK(a != b);

    Map<int, int> c;
    c[1] = 10;
    CHECK(a != c); // different key sets

    Map<string, int> e1, e2;
    CHECK(e1 == e2); // empty == empty
  }

  // Copy construction is deep and independent.
  {
    Map<string, int> a;
    a["x"] = 1;
    a["y"] = 2;
    Map<string, int> b(a);
    CHECK(b.size() == 2);
    CHECK(b.find("x") == 1);

    b["x"] = 100;
    b["z"] = 3;
    b.erase("y");
    CHECK(b.size() == 2);
    CHECK(a.size() == 2);       // a untouched
    CHECK(a.find("x") == 1);    // a's x unchanged
    CHECK(a.exists("y"));       // a still has y
    CHECK(!b.exists("y"));      // b lost it
    CHECK(b.find("x") == 100);
  }

  // Copy assignment replaces contents; empty over empty; self-assign.
  {
    Map<int, int> a;
    a[1] = 1; a[2] = 2;
    Map<int, int> b;
    b[50] = 50;
    b = a;
    CHECK(b.size() == 2);
    CHECK(b.find(1) == 1);
    CHECK(!b.exists(50)); // stale key gone

    Map<int, int> e1, e2;
    e1[5] = 5;
    e1 = e2; // assign empty over populated
    CHECK(e1.empty());

    Map<int, int> &alias = a;
    a = alias; // self-assignment
    CHECK(a.size() == 2);
    CHECK(a.find(1) == 1 && a.find(2) == 2);
  }

  // Move construction steals; source empty and reusable.
  {
    Map<int, int> a;
    for (int i = 0; i < 10; i++)
      a[i] = i * 2;
    Map<int, int> m(std::move(a));
    CHECK(m.size() == 10);
    CHECK(m.find(0) == 0 && m.find(9) == 18);

    CHECK(a.empty());
    a[50] = 1; // reusable
    CHECK(a.size() == 1 && a.find(50) == 1);
  }

  // Move assignment frees destination and steals; self-move safe.
  {
    Map<int, int> a;
    a[1] = 11; a[2] = 22;
    Map<int, int> b;
    b[77] = 77;
    b = std::move(a);
    CHECK(b.size() == 2);
    CHECK(b.find(1) == 11 && b.find(2) == 22);
    CHECK(!b.exists(77));

    CHECK(a.empty());
    a[9] = 9; // reusable
    CHECK(a.size() == 1);

    Map<int, int> s;
    s[3] = 3;
    Map<int, int> &alias = s;
    s = std::move(alias); // self-move no-op
    CHECK(s.size() == 1 && s.find(3) == 3);
  }

  // swap exchanges (member and free).
  {
    Map<int, int> s1, s2;
    s1[1] = 1;
    s2[2] = 2; s2[3] = 3;
    s1.swap(s2);
    CHECK(s1.size() == 2 && s2.size() == 1);
    CHECK(s1.find(2) == 2 && s2.find(1) == 1);

    swap(s1, s2);
    CHECK(s1.size() == 1 && s2.size() == 2);
    CHECK(s1.find(1) == 1);
  }

  // clear() empties and stays reusable.
  {
    Map<int, int> c;
    for (int i = 0; i < 10; i++)
      c[i] = i;
    c.clear();
    CHECK(c.empty());
    CHECK(!c.exists(5));
    c[5] = 55; // reusable
    CHECK(c.size() == 1 && c.find(5) == 55);
    c.clear();
    c.clear(); // idempotent
    CHECK(c.empty());
  }

  // operator<< prints "key -> value" lines ascending; empty map prints
  // nothing.
  {
    Map<int, string> m;
    m[1] = "one";
    m[2] = "two";
    ostringstream oss;
    oss << m;
    CHECK(oss.str() == "1 -> one\n2 -> two\n");
    CHECK(m.size() == 2); // printing did not consume

    Map<int, string> e;
    ostringstream ossE;
    ossE << e;
    CHECK(ossE.str() == "");
  }

  // Randomized stress: Map against std::map as the reference model;
  // every insert/replace/erase/exists/find/op-count must agree, and
  // final sorted traversal must match element-wise.
  {
    bool allOk = true;
    for (int trial = 0; trial < 30; trial++) {
      Map<int, int> mine;
      map<int, int> model;
      const int KEYS = 200;

      for (int op = 0; op < 800; op++) {
        int k = int(rnd() % KEYS);
        int roll = int(rnd() % 4);

        if (roll == 0 || roll == 1) { // insert or replace
          int v = int(rnd() % 1000000);
          bool added = mine.insert(k, v);
          auto it = model.find(k);
          bool modelAdded = (it == model.end());
          if (added != modelAdded)
            allOk = false;
          model[k] = v;
        } else if (roll == 2) { // erase
          bool erased = mine.erase(k);
          bool modelErased = model.erase(k) > 0;
          if (erased != modelErased)
            allOk = false;
        } else { // exists + find
          if (mine.exists(k) != (model.count(k) > 0))
            allOk = false;
          if (model.count(k)) {
            if (mine.find(k) != model[k])
              allOk = false;
          }
        }

        if (mine.size() != model.size())
          allOk = false;
      }

      // final traversal: exact ascending match
      bool same = true;
      auto it = model.begin();
      mine.for_each([&](int k, int v) {
        if (it == model.end() || k != it->first || v != it->second)
          same = false;
        else
          ++it;
      });
      if (it != model.end() || !same)
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
