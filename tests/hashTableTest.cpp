// Tests for HashTable<K, T>. Prints one line per check and exits nonzero
// on the first failure. Build and run from tests/:
//   make hashTableTest && ./hashTableTest

#include <iostream>
#include <string>

using namespace std;

#include "HashTable.h"

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

// djb2. NOTE: deliberately int-returning and allowed to overflow, so the
// table must cope with negative hash values (regression coverage below).
int hashFunc(const string &s) {
  unsigned long v = 5381;
  for (unsigned i = 0; i < s.size(); i++)
    v = ((v << 5) + v) + s[i];
  return static_cast<int>(v);
}

// Multiplies by an odd 32-bit constant; may return negative values.
int intHash(const int &k) { return int(unsigned(k) * 2654435761u); }

// Returns a fixed NEGATIVE value: forces every key into one bucket and
// exercises the negative-modulo guard.
int constHash(const int &k) {
  (void)k;
  return -3;
}

struct obj {
  obj() : _n(-1) {}
  obj(const int n) : _n(n) {}
  bool operator==(const obj &rhs) const { return _n == rhs._n; }

  int _n;
};

ostream &operator<<(ostream &os, const obj &o) {
  os << o._n;
  return (os);
}

// Deterministic PRNG so failures are reproducible.
static unsigned seed = 555555555u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Insert, find, and operator[] basics.
  {
    HashTable<string, float> h(100, hashFunc);
    CHECK(h.empty());
    CHECK(h.size() == 0);

    h.insert("jan", 1);
    h.insert("feb", 2);
    h.insert("mar", 3);
    CHECK(h.size() == 3);
    CHECK(!h.empty());
    CHECK(h.find("jan") == 1);
    CHECK(h["feb"] == 2);
    CHECK(h.find("mar") == 3);

    bool threw = false;
    try { h.find("nope"); } catch (Exception &) { threw = true; }
    CHECK(threw); // find of a missing key throws

    threw = false;
    try {
      const HashTable<string, float> &ch = h;
      ch["nope"];
    } catch (Exception &) {
      threw = true;
    }
    CHECK(threw); // const operator[] of a missing key throws too
  }

  // insert() on an existing key REPLACES, and the returned reference
  // aliases the stored value.
  {
    HashTable<int, obj> h(10, intHash);
    obj &r = h.insert(7, obj(1000));
    r._n = 101; // write through the returned reference
    CHECK(h.find(7)._n == 101);

    h.insert(7, obj(5)); // replace
    CHECK(h.size() == 1);
    CHECK(h.find(7)._n == 5);
    CHECK(r._n == 5); // the old reference still aliases the node
  }

  // operator[] read/write: absent key inserts a default T.
  {
    HashTable<int, obj> h(10, intHash);
    h[3] = obj(30);
    CHECK(h.size() == 1);
    CHECK(h[3]._n == 30);

    int before = h.size();
    h[999]; // lvalue access materializes a default
    CHECK(h.size() == before + 1);
    CHECK(h[999]._n == -1); // obj() default
  }

  // erase(): single-element bucket, repeated erase of the same key.
  {
    HashTable<int, obj> h(7, intHash);
    h.insert(0, obj(0));
    CHECK(h.erase(0));
    CHECK(h.size() == 0);
    CHECK(!h.exists(0));
    CHECK(!h.erase(0)); // second erase of the same key: false, size intact
    CHECK(h.size() == 0);
  }

  // erase across one long chain: a constant hash forces a single bucket,
  // covering the head / middle / tail unlink cases.
  {
    HashTable<int, obj> h(8, constHash); // constant hash => one chain

    for (int i = 0; i < 6; i++)
      h.insert(i, obj(i * 10));
    CHECK(h.size() == 6);

    CHECK(h.erase(0)); // head of the chain
    CHECK(!h.exists(0));
    CHECK(h.find(1)._n == 10); // chain still traversable

    CHECK(h.erase(3)); // middle
    CHECK(!h.exists(3));
    CHECK(h.find(2)._n == 20);
    CHECK(h.find(4)._n == 40);

    CHECK(h.erase(5)); // tail
    CHECK(!h.exists(5));

    CHECK(h.size() == 3);
    CHECK(h.erase(99) == false); // never inserted
    CHECK(h.size() == 3);
  }

  // exists() reports accurately for present and absent keys.
  {
    HashTable<string, float> h(50, hashFunc);
    for (int i = 0; i < 26; i++) {
      string key(1, char('a' + i));
      CHECK(h.exists(key) == false);
      h.insert(key, i);
      CHECK(h.exists(key));
    }
    CHECK(h.size() == 26);
    CHECK(h.exists("") == false); // empty string key
  }

  // Negative hash values must not index a negative bucket (regression:
  // the original _hash(k) % _size on a negative int produced an
  // out-of-bounds bucket).
  {
    HashTable<int, obj> hn(17, intHash);
    for (int k = -1; k >= -20; k--)
      hn.insert(k, obj(k));
    CHECK(hn.size() == 20);
    CHECK(hn.find(-1)._n == -1);
    CHECK(hn.find(-20)._n == -20);
    CHECK(hn.exists(-9));
    CHECK(!hn.exists(9));

    HashTable<int, obj> hc(5, constHash); // constant negative hash
    for (int k = 0; k < 8; k++)
      hc.insert(k, obj(k));
    CHECK(hc.size() == 8); // one bucket, eight-deep chain
    CHECK(hc.find(0)._n == 0);
    CHECK(hc.find(7)._n == 7);
    CHECK(hc.erase(3));
    CHECK(hc.find(6)._n == 6); // chain intact around the erased node

    // djb2 overflows negative on a long string; exercise that directly.
    string longKey(200, 'z');
    HashTable<string, float> hl(17, hashFunc);
    hl.insert(longKey, 4.5);
    CHECK(hl.find(longKey) == 4.5);
    CHECK(hl.exists(longKey));
  }

  // clear(): empties but keeps buckets and hash, so the table is
  // immediately reusable (regression: the original zeroed _hash and
  // _size, making every later call undefined).
  {
    HashTable<string, float> h(20, hashFunc);
    h.insert("a", 1);
    h.insert("b", 2);
    h.clear();
    CHECK(h.size() == 0);
    CHECK(h.empty());
    CHECK(h.buckets() == 20); // capacity retained

    h.insert("c", 3); // works again without rehash setup
    CHECK(h.find("c") == 3);
    CHECK(h.size() == 1);
    bool threw = false;
    try { h.find("a"); } catch (Exception &) { threw = true; }
    CHECK(threw); // "a" really gone
  }

  // Copy construction is deep and independent.
  {
    HashTable<string, float> a(40, hashFunc);
    a.insert("x", 1);
    a.insert("y", 2);

    HashTable<string, float> b(a);
    CHECK(b.size() == 2);
    CHECK(b.find("x") == 1);
    CHECK(b.find("y") == 2);
    CHECK(b.buckets() == 40);
    CHECK(b.erase("x"));
    CHECK(b.size() == 1);
    CHECK(a.size() == 2); // erasing in b left a intact
    CHECK(a.exists("x"));

    b.insert("x", 9);
    b.find("x") = 10; // write through find's reference
    CHECK(b.find("x") == 10);
    CHECK(a.find("x") == 1); // a's x separate from b's x
  }

  // Copy assignment replaces contents; old destination is destroyed.
  {
    HashTable<int, obj> a(16, intHash), b(16, intHash);
    a.insert(1, obj(1));
    a.insert(2, obj(2));

    b.insert(50, obj(50));
    b = a;
    CHECK(b.size() == 2);
    CHECK(b.find(1)._n == 1);
    CHECK(b.find(2)._n == 2);
    bool threw = false;
    try { b.find(50); } catch (Exception &) { threw = true; }
    CHECK(threw); // replaced contents, no residue

    b.insert(3, obj(3));
    CHECK(a.size() == 2); // a unaffected
  }

  // Self-assignment is a no-op.
  {
    HashTable<int, obj> a(8, intHash);
    a.insert(1, obj(1));
    HashTable<int, obj> &alias = a;
    a = alias;
    CHECK(a.size() == 1);
    CHECK(a.find(1)._n == 1);
  }

  // Move construction steals; source is bucket-less and NOT reusable,
  // matching the documented moved-from contract.
  {
    HashTable<int, obj> a(10, intHash);
    for (int i = 0; i < 5; i++)
      a.insert(i, obj(i));
    HashTable<int, obj> m(std::move(a));
    CHECK(m.size() == 5);
    CHECK(m.find(0)._n == 0);
    CHECK(m.find(4)._n == 4);

    CHECK(a.size() == 0);
    bool threw = false;
    try { a.insert(1, obj(1)); } catch (Exception &) { threw = true; }
    CHECK(threw); // moved-from table refuses inserts
  }

  // Move assignment frees destination contents and steals the source.
  {
    HashTable<int, obj> a(10, intHash);
    a.insert(1, obj(11));
    a.insert(2, obj(22));

    HashTable<int, obj> b(5, intHash);
    b.insert(77, obj(77));
    b = std::move(a);
    CHECK(b.size() == 2);
    CHECK(b.find(1)._n == 11);
    CHECK(b.find(2)._n == 22);
    bool threw = false;
    try { b.find(77); } catch (Exception &) { threw = true; }
    CHECK(threw); // old contents gone

    CHECK(a.size() == 0); // source emptied
    threw = false;
    try { a.insert(9, obj(9)); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // Self-move is a no-op.
  {
    HashTable<int, obj> a(8, intHash);
    a.insert(1, obj(1));
    HashTable<int, obj> &alias = a;
    a = std::move(alias);
    CHECK(a.size() == 1);
    CHECK(a.find(1)._n == 1);
  }

  // swap exchanges contents, bucket counts, and hash functions.
  {
    HashTable<string, float> s1(10, hashFunc), s2(64, hashFunc);
    s1.insert("one", 1);
    s2.insert("two", 2);
    s2.insert("three", 3);
    s1.swap(s2);
    CHECK(s1.size() == 2 && s2.size() == 1);
    CHECK(s1.buckets() == 64 && s2.buckets() == 10);
    CHECK(s1.find("two") == 2);
    CHECK(s2.find("one") == 1);
  }

  // A table with a single bucket (maximum chaining).
  {
    HashTable<int, obj> h1(1, intHash);
    for (int i = 0; i < 10; i++)
      h1.insert(i, obj(i * 2));
    CHECK(h1.size() == 10);
    CHECK(h1.find(7)._n == 14); // chain traversal is correct
    CHECK(h1.erase(4));
    CHECK(h1.size() == 9);
    CHECK(!h1.exists(4));
    CHECK(h1.find(3)._n == 6); // chain unlinked correctly around 4
    CHECK(h1.find(5)._n == 10);
  }

  // A zero-bucket table is safe: queries refuse politely or report false.
  {
    HashTable<int, obj> h(0, intHash);
    CHECK(h.empty());
    CHECK(!h.exists(1));
    CHECK(!h.erase(1));
    bool threw = false;
    try { h.find(1); } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { h.insert(1, obj(1)); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // Randomized stress: every insert/erase/exists/find must agree with a
  // naive array-based model. Catches bucket indexing, chain unlinking,
  // and counting errors together.
  {
    bool allOk = true;
    const int KEYS = 200;
    for (int trial = 0; trial < 30; trial++) {
      unsigned b = 1 + rnd() % 64; // bucket count varies per trial

      HashTable<int, int> h(b, intHash);

      // Reference model: parallel arrays holding live entries only.
      int modelKey[KEYS];
      int modelVal[KEYS];
      int cnt = 0;

      for (int op = 0; op < 400; op++) {
        int k = static_cast<int>(rnd() % KEYS);
        int roll = static_cast<int>(rnd() % 3);
        bool present = false;
        for (int i = 0; i < cnt && !present; i++)
          present = (modelKey[i] == k);

        if (roll == 0) { // insert
          int v = static_cast<int>(rnd() % 100000);
          h.insert(k, v);
          if (present) { // replace in model
            for (int i = 0; i < cnt; i++)
              if (modelKey[i] == k) {
                modelVal[i] = v;
                break;
              }
          } else {
            modelKey[cnt] = k;
            modelVal[cnt] = v;
            cnt++;
          }
        } else if (roll == 1) { // erase
          bool erased = h.erase(k);
          if (erased != present)
            allOk = false;
          if (present) {
            for (int i = 0; i < cnt; i++)
              if (modelKey[i] == k) {
                modelKey[i] = modelKey[cnt - 1];
                modelVal[i] = modelVal[cnt - 1];
                cnt--;
                break;
              }
          }
        } else { // exists/find
          if (h.exists(k) != present)
            allOk = false;
          if (present) {
            int want = 0;
            for (int i = 0; i < cnt; i++)
              if (modelKey[i] == k) {
                want = modelVal[i];
                break;
              }
            if (h.find(k) != want)
              allOk = false;
          }
        }

        if (static_cast<unsigned>(cnt) != h.size())
          allOk = false;
      }
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
