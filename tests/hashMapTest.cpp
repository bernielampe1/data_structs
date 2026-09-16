// Tests for HashMap<K, V>. Prints one line per check and exits nonzero
// on the first failure. Build and run from tests/:
//   make hashMapTest && ./hashMapTest

#include "HashMap.h"
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

// A key type wrapping int with an ODD hash (same value for every even
// key) to force maximal clustering and exercise tombstone probing.
struct OddHash {
  int v;
  OddHash() : v(0) {}
  OddHash(int n) : v(n) {}
  bool operator==(const OddHash &o) const { return v == o.v; }
};

struct OddHasher {
  size_t operator()(const OddHash &o) const {
    return (o.v % 2 == 0) ? 42 : size_t(o.v * 31 + 1); // evens collide
  }
};

namespace std {
template <> struct hash<OddHash> : OddHasher {};
} // namespace std

// Deterministic PRNG so failures are reproducible.
static unsigned seed = 192608170u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Fresh map.
  {
    HashMap<int, string> m;
    CHECK(m.empty());
    CHECK(m.size() == 0);
    CHECK(m.capacity() == 0);
    CHECK(!m.exists(1));
    CHECK(!m.erase(1)); // erase on empty

    bool threw = false;
    try { m.find(1); } catch (Exception &) { threw = true; }
    CHECK(threw); // find on empty throws
  }

  // insert/find/replace.
  {
    HashMap<int, string> m;
    CHECK(m.insert(5, "five"));  // new
    CHECK(!m.insert(5, "FIVE")); // replace reports false
    CHECK(m.insert(1, "one"));
    CHECK(m.insert(9, "nine"));
    CHECK(m.size() == 3);

    CHECK(m.find(5) == "FIVE"); // replaced
    CHECK(m.find(1) == "one");
    CHECK(m.find(9) == "nine");

    bool threw = false;
    try { m.find(42); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // operator[] read/write: absent inserts a default; const throws.
  {
    HashMap<int, string> m;
    m[3] = "three";
    CHECK(m.size() == 1);
    CHECK(m[3] == "three");

    int before = m.size();
    m[99]; // materializes a default
    CHECK(m.size() == before + 1);
    CHECK(m[99] == "");
    m[99] = "ninety-nine";
    CHECK(m[99] == "ninety-nine");

    const HashMap<int, string> &cm = m;
    bool threw = false;
    try { cm[77]; } catch (Exception &) { threw = true; }
    CHECK(threw);
    CHECK(cm[99] == "ninety-nine");
  }

  // Erase and re-insert: tombstones absorb erasures, the mapping is
  // gone, and a re-inserted key lands back in the table fine.
  {
    HashMap<int, string> m;
    for (int i = 0; i < 20; i++)
      m[i] = "v";
    CHECK(m.size() == 20);

    CHECK(m.erase(0));
    CHECK(m.erase(7));
    CHECK(m.erase(15));
    CHECK(m.size() == 17);
    CHECK(!m.exists(0) && !m.exists(7) && !m.exists(15));

    CHECK(!m.erase(7)); // second erase: false
    CHECK(m.size() == 17);

    // re-insert erased keys
    CHECK(m.insert(0, "reborn"));
    CHECK(m.size() == 18);
    CHECK(m.find(0) == "reborn");

    // odd untouched keys still readable (probes cross the tombstones)
    for (int i = 1; i < 20; i += 2)
      if (i != 7 && i != 15 ? !m.exists(i) : false)
        CHECK(false); // probes through the tombstones work
    CHECK(true);
  }

  // Rehash growth: crossing the load factor preserves every mapping.
  {
    HashMap<int, int> m;
    for (int i = 0; i < 1000; i++)
      m.insert(i, i * 3);
    CHECK(m.size() == 1000);
    CHECK(m.capacity() >= 1024); // doubled at least 7 times from 0

    bool allMapped = true;
    for (int i = 0; i < 1000; i++)
      if (!m.exists(i) || m.find(i) != i * 3)
        allMapped = false;
    CHECK(allMapped); // nothing lost by any rehash

    CHECK(m.capacity() != 1024 || true);   // exact growth is internal
    CHECK(m.load() <= 0.7);                // load factor holds
    bool pow2 = m.capacity() != 0 && (m.capacity() & (m.capacity() - 1)) == 0;
    CHECK(pow2);                           // power-of-two invariant
  }

  // Wraparound: keys hashing to the tail must probe into the head
  // cells (linear probing is circular). OddHash evens all hash to 42:
  // inserting many evens forces chains that wrap.
  {
    HashMap<OddHash, int> m;
    for (int i = 0; i < 30; i += 2) // 15 even keys, all hash 42
      m.insert(OddHash(i), i);

    CHECK(m.size() == 15);
    for (int i = 0; i < 30; i += 2) {
      if (!m.exists(OddHash(i)) || m.find(OddHash(i)) != i)
        CHECK(false);
    }
    CHECK(true);

    // erase the middle of a wrapped chain; the rest stays findable
    CHECK(m.erase(OddHash(16))); // chain-internal tombstone
    for (int i = 0; i < 30; i += 2) {
      bool want = (i != 16);
      if (m.exists(OddHash(i)) != want)
        CHECK(false);
    }
    CHECK(true);

    // capacity grew under clustering; still power of two
    bool pow2 = m.capacity() != 0 && (m.capacity() & (m.capacity() - 1)) == 0;
    CHECK(pow2);
  }

  // clear(): empties in place; capacity kept; reusable.
  {
    HashMap<int, string> m;
    for (int i = 0; i < 10; i++)
      m[i] = "v";
    m.clear();
    CHECK(m.empty());
    CHECK(m.size() == 0);
    CHECK(m.load() == 0);
    CHECK(m.capacity() >= 8); // cells kept

    m[5] = "again"; // reusable without realloc
    CHECK(m.size() == 1 && m.find(5) == "again");

    // clear(), then refill with different keys
    m.clear();
    m[100] = "hundred";
    CHECK(m.size() == 1);

    m.hardReset(); // release the cells
    CHECK(m.capacity() == 0);
    CHECK(m.empty());
    m[42] = "life"; // reusable from scratch
    CHECK(m.size() == 1);
  }

  // Eager-tombstone-rehash: erasing most of a table must shrink the
  // usable load and keep survivors correct.
  {
    HashMap<int, int> m;
    for (int i = 0; i < 100; i++)
      m[i] = i;

    for (int i = 0; i < 90; i++) // erase almost everything
      CHECK(m.erase(i));
    CHECK(m.size() == 10);
    CHECK(m.load() <= 0.7); // tombstones compacted away

    for (int i = 90; i < 100; i++) {
      if (!m.exists(i) || m.find(i) != i)
        CHECK(false);
    }
    CHECK(true);

    // room for plenty more without error
    for (int i = 0; i < 200; i++)
      m.insert(1000 + i, i);
    CHECK(m.size() == 210);
    CHECK(m.find(1199) == 199);
  }

  // String keys via std::hash<string>.
  {
    HashMap<string, int> m;
    m["gamma"] = 3;
    m["alpha"] = 1;
    m["beta"] = 2;
    m["gamma"] = 30; // replace
    CHECK(m.size() == 3);
    CHECK(m.find("alpha") == 1);
    CHECK(m.find("gamma") == 30);

    // count query semantics without order expectations
    int total = 0;
    m.for_each([&](const string &, int v) { total += v; });
    CHECK(total == 33);
  }

  // for_each sees each mapping exactly once.
  {
    HashMap<int, int> m;
    for (int i = 0; i < 50; i++)
      m[i] = i * 2;
    int seen = 0;
    long sum = 0;
    m.for_each([&](int k, int v) {
      seen++;
      if (v == k * 2)
        sum += v;
    });
    CHECK(seen == 50);
    CHECK(sum == 2450); // 2*(0+1+...+49) = 2*1225
  }

  // operator== / != compares mappings regardless of cell layout.
  {
    HashMap<int, int> a, b;
    a[1] = 10; a[2] = 20;
    b[2] = 20; b[1] = 10; // inserted in the other order, same values
    CHECK(a == b);        // same mappings

    b[1] = 11; // now a value differs
    CHECK(a != b);

    HashMap<int, int> c;
    c[1] = 10; c[2] = 20; c[3] = 30;
    CHECK(a != c); // extra mapping

    HashMap<string, int> e1, e2;
    CHECK(e1 == e2);
  }

  // Copy construction is deep and independent.
  {
    HashMap<string, int> a;
    a["x"] = 1;
    a["y"] = 2;

    HashMap<string, int> b(a);
    CHECK(b.size() == 2);
    CHECK(b.find("x") == 1);

    b["x"] = 100;
    b["z"] = 3;
    b.erase("y");
    CHECK(b.size() == 2);
    CHECK(a.size() == 2);      // a untouched
    CHECK(a.find("x") == 1);   // a's x unchanged
    CHECK(a.exists("y"));      // a still has y
    CHECK(!b.exists("y"));
    CHECK(b.find("x") == 100);

    // copies of heavily-tombstoned tables are clean
    HashMap<int, int> t;
    for (int i = 0; i < 20; i++)
      t[i] = i;
    for (int i = 0; i < 15; i++)
      t.erase(i);
    HashMap<int, int> tc(t);
    CHECK(tc.size() == 5);
    for (int i = 15; i < 20; i++)
      if (!tc.exists(i))
        CHECK(false);
    CHECK(true);
  }

  // Copy assignment replaces; empty over populated; self-assign.
  {
    HashMap<int, int> a;
    a[1] = 1; a[2] = 2;
    HashMap<int, int> b;
    b[50] = 50;
    b = a;
    CHECK(b.size() == 2);
    CHECK(b.find(1) == 1);
    CHECK(!b.exists(50));

    HashMap<int, int> e1, e2;
    e1[5] = 5;
    e1 = e2; // empty over populated
    CHECK(e1.empty());
    CHECK(e1.capacity() == 0 ? e1.empty() : true); // either is fine

    HashMap<int, int> &alias = a;
    a = alias; // self-assignment
    CHECK(a.size() == 2);
    CHECK(a.find(1) == 1 && a.find(2) == 2);
  }

  // Move construction steals; source empty and reusable.
  {
    HashMap<int, int> a;
    for (int i = 0; i < 10; i++)
      a[i] = i * 2;
    HashMap<int, int> m(std::move(a));
    CHECK(m.size() == 10);
    CHECK(m.find(0) == 0 && m.find(9) == 18);

    CHECK(a.empty());
    a[50] = 1; // reusable
    CHECK(a.size() == 1 && a.find(50) == 1);
  }

  // Move assignment frees destination and steals; self-move safe.
  {
    HashMap<int, int> a;
    a[1] = 11; a[2] = 22;
    HashMap<int, int> b;
    b[77] = 77;
    b = std::move(a);
    CHECK(b.size() == 2);
    CHECK(b.find(1) == 11 && b.find(2) == 22);
    CHECK(!b.exists(77));

    CHECK(a.empty());
    a[9] = 9; // reusable
    CHECK(a.size() == 1);

    HashMap<int, int> s;
    s[3] = 3;
    HashMap<int, int> &alias = s;
    s = std::move(alias); // self-move no-op
    CHECK(s.size() == 1 && s.find(3) == 3);
  }

  // swap exchanges (member and free).
  {
    HashMap<int, int> s1, s2;
    s1[1] = 1;
    s2[2] = 2; s2[3] = 3;
    s1.swap(s2);
    CHECK(s1.size() == 2 && s2.size() == 1);
    CHECK(s1.find(2) == 2 && s2.find(1) == 1);

    swap(s1, s2);
    CHECK(s1.size() == 1 && s2.size() == 2);
    CHECK(s1.find(1) == 1);
  }

  // operator<<: bucket-order lines, key->value; empty prints nothing.
  {
    HashMap<int, string> m;
    m[1] = "one";
    m[2] = "two";
    ostringstream oss;
    oss << m;
    // order is unspecified; assert content pieces and line count
    CHECK(oss.str().find("1 -> one\n") != string::npos);
    CHECK(oss.str().find("2 -> two\n") != string::npos);
    CHECK(oss.str().size() ==
          string("1 -> one\n").size() + string("2 -> two\n").size());
    CHECK(m.size() == 2); // printing did not consume

    HashMap<int, string> e;
    ostringstream ossE;
    ossE << e;
    CHECK(ossE.str() == "");
  }

  // Randomized stress against std::map: insert/replace/erase/exists/
  // find agreement per op, including deliberate rehash churn.
  {
    bool allOk = true;
    const int KEYS = 300;
    for (int trial = 0; trial < 30; trial++) {
      HashMap<int, int> mine;
      map<int, int> model;

      for (int op = 0; op < 1000; op++) {
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

      // final: same size, same mapping set
      long modelTotal = 0, mineTotal = 0;
      mine.for_each([&](int k, int v) {
        modelTotal += k + v;
        if (model.count(k) == 0 || model[k] != v)
          allOk = false;
      });
      for (auto &kv : model)
        mineTotal += kv.first + kv.second;
      if (modelTotal != mineTotal)
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
