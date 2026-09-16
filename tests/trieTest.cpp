// Tests for Trie<V>. Prints one line per check and exits nonzero on the
// first failure. Build and run from tests/:  make trieTest && ./trieTest

#include "Trie.h"
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
static unsigned seed = 161803398u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Fresh trie.
  {
    Trie<int> t;
    CHECK(t.empty());
    CHECK(t.size() == 0);
    CHECK(!t.exists(""));
    CHECK(!t.hasPrefix("a"));
    CHECK(!t.erase("anything"));
    t.clear(); // clear on empty: safe
    CHECK(t.empty());
  }

  // insert/find/exists: store, retrieve, replace.
  {
    Trie<int> t;
    CHECK(t.insert("cat", 1));   // new
    CHECK(!t.insert("cat", 11)); // replace reports false
    CHECK(t.insert("car", 2));
    CHECK(t.insert("dog", 3));
    CHECK(t.size() == 3);

    CHECK(t.find("cat") == 11); // replaced value
    CHECK(t.find("car") == 2);
    CHECK(t.find("dog") == 3);

    bool threw = false;
    try { t.find("cow"); } catch (Exception &) { threw = true; }
    CHECK(threw); // shares the prefix c but is not mapped
  }

  // Nesting: a key that is a prefix of another key ("in" vs "inn");
  // erasing the shorter one must leave the longer one intact.
  {
    Trie<string> t;
    t["in"] = "in";
    t["inn"] = "inn";
    t["into"] = "into";
    CHECK(t.size() == 3);
    CHECK(t.find("in") == "in");
    CHECK(t.find("inn") == "inn");
    CHECK(t.find("into") == "into");

    CHECK(t.erase("in"));
    CHECK(t.size() == 2);
    CHECK(!t.exists("in"));
    CHECK(t.exists("inn"));   // untouched
    CHECK(t.exists("into"));  // untouched
    CHECK(t.find("inn") == "inn");

    // reinsert after erase
    t.insert("in", "back");
    CHECK(t.find("in") == "back");
    CHECK(t.size() == 3);
  }

  // operator[] read/write: absent inserts a default; const throws.
  {
    Trie<int> t;
    t["apple"] = 5;
    CHECK(t.size() == 1);
    CHECK(t["apple"] == 5);

    int before = t.size();
    t["banana"]; // read materializes a default
    CHECK(t.size() == before + 1);
    CHECK(t["banana"] == 0);

    const Trie<int> &ct = t;
    bool threw = false;
    try { ct["cherry"]; } catch (Exception &) { threw = true; }
    CHECK(threw);

    // writing through operator[] reference
    t["appl"] = 9; // sibling prefix of "apple"
    CHECK(t["appl"] == 9);
    CHECK(t["apple"] == 5); // untouched
  }

  // Levenshtein-ish adjacency: "a", "ab", "abc" chains and prunes.
  {
    Trie<int> t;
    t["a"] = 1;
    t["ab"] = 2;
    t["abc"] = 3;
    t["abcd"] = 4;
    CHECK(t.size() == 4);
    CHECK(t.hasPrefix("a") && t.hasPrefix("ab") && t.hasPrefix("abcd"));
    CHECK(!t.hasPrefix("e"));

    CHECK(t.erase("abcd"));
    CHECK(t.exists("abc")); // chain preserved
    CHECK(!t.hasPrefix("abcd"));

    CHECK(t.erase("abc"));
    CHECK(t.exists("ab"));
    CHECK(t.erase("ab"));
    CHECK(t.exists("a"));
    CHECK(t.erase("a"));
    CHECK(t.empty());
    CHECK(!t.hasPrefix("a")); // fully pruned back to the root

    t["z"] = 26; // reusable after everything pruned
    CHECK(t.size() == 1 && t.find("z") == 26);
  }

  // Empty-string key: a valid key that maps at the root.
  {
    Trie<int> t;
    t[""] = 0;
    CHECK(t.size() == 1);
    CHECK(t.exists(""));
    CHECK(t.find("") == 0);
    CHECK(t.hasPrefix(""));   // "" mapped: the root itself terminates a key
    CHECK(!t.hasPrefix("x")); // no key extends past ""

    CHECK(t.erase(""));
    CHECK(t.size() == 0);
    CHECK(!t.exists(""));

    // "" as a prefix of real keys
    Trie<int> u;
    u["k"] = 1;
    CHECK(u.hasPrefix("")); // the root always holds at least one child
    u[""] = 2;
    CHECK(u.size() == 2);
  }

  // hasPrefix vs exists distinction (the trie's raison d'etre).
  {
    Trie<int> t;
    t["team"] = 1;
    t["tea"] = 2;

    CHECK(t.hasPrefix("te"));
    CHECK(t.hasPrefix("tea"));
    CHECK(!t.exists("te")); // prefix without a key
    CHECK(t.exists("tea"));


  }

  // for_each: lexicographic order.
  {
    Trie<int> t;
    t["banana"] = 1;
    t["apple"] = 2;
    t["cherry"] = 3;
    t["app"] = 0;

    bool sorted = true;
    string prev;
    bool first = true;
    int count = 0;
    t.for_each([&](const string &k, int v) {
      if (!first && k <= prev)
        sorted = false;
      prev = k;
      first = false;
      count++;
      if ((k == "app" && v != 0) || (k == "apple" && v != 2) ||
          (k == "banana" && v != 1) || (k == "cherry" && v != 3))
        sorted = false;
    });
    CHECK(sorted); // app, apple, banana, cherry
    CHECK(count == 4);
  }

  // prefix_each: traversal restricted to a prefix.
  {
    Trie<int> t;
    t["car"] = 1;
    t["cart"] = 2;
    t["cat"] = 3;
    t["dog"] = 4;

    string seen;
    int count = 0;
    t.prefix_each("car", [&](const string &k, int) {
      seen += k + ";";
      count++;
    });
    CHECK(seen == "car;cart;");   // exactly the two car* keys
    CHECK(count == 2);

    // prefix with no matches emits nothing
    count = 0;
    t.prefix_each("x", [&](const string &, int) { count++; });
    CHECK(count == 0);

    // exhaustive prefix
    count = 0;
    t.prefix_each("", [&](const string &, int) { count++; });
    CHECK(count == 4);
  }

  // erase() prunes internal nodes: after erasing the only long key,
  // hasPrefix on its path goes false (tested above); here with two
  // keys sharing a prefix, pruning keeps the shared path alive.
  {
    Trie<int> t;
    t["abcd"] = 1;
    t["abce"] = 2;
    CHECK(t.hasPrefix("abc"));

    t.erase("abcd");
    CHECK(t.hasPrefix("abc"));  // abce still walks through ab c
    CHECK(t.exists("abce"));
    CHECK(!t.hasPrefix("abcd")); // 'd' pruned

    t.erase("abce");
    CHECK(!t.hasPrefix("ab"));   // whole branch pruned
    CHECK(t.empty());
  }

  // operator== / != (key-value equality, shape irrelevant).
  {
    Trie<int> a, b;
    a["x"] = 1;
    a["xy"] = 2;
    b["xy"] = 2; // different shape, same mapping
    b["x"] = 1;
    CHECK(a == b);

    b["xyz"] = 3;
    CHECK(a != b);

    Trie<int> c;
    c["x"] = 9; // same key, different value
    c["xy"] = 2;
    CHECK(a != c);

    Trie<int> e1, e2;
    CHECK(e1 == e2);
  }

  // Copy construction is deep and independent.
  {
    Trie<int> a;
    a["one"] = 1;
    a["one2"] = 11;
    a["two"] = 2;

    Trie<int> b(a);
    CHECK(b.size() == 3);
    CHECK(b.find("one") == 1);

    b["one"] = 100;
    b["three"] = 3;
    b.erase("two");
    CHECK(b.size() == 3);
    CHECK(a.size() == 3);        // a untouched
    CHECK(a.find("one") == 1);   // a's values unchanged
    CHECK(a.exists("two"));      // a still has two
    CHECK(!b.exists("two"));     // b lost it
    CHECK(b.find("one") == 100);
  }

  // Copy assignment replaces contents; self-assignment no-op.
  {
    Trie<int> a;
    a["k1"] = 1;
    Trie<int> b;
    b["stale"] = 9;
    b = a;
    CHECK(b.size() == 1);
    CHECK(b.find("k1") == 1);
    CHECK(!b.exists("stale"));

    Trie<int> &alias = a;
    a = alias;
    CHECK(a.size() == 1);
    CHECK(a.find("k1") == 1);
  }

  // Move construction steals; source empty and reusable.
  {
    Trie<int> a;
    a["move"] = 1;
    a["movee"] = 2;
    Trie<int> m(std::move(a));
    CHECK(m.size() == 2);
    CHECK(m.find("move") == 1);

    CHECK(a.empty());
    a["fresh"] = 9; // reusable
    CHECK(a.size() == 1 && a.find("fresh") == 9);
  }

  // Move assignment frees destination and steals; self-move safe.
  {
    Trie<int> a;
    a["src"] = 1;
    Trie<int> b;
    b["dst-stale"] = 5;
    b = std::move(a);
    CHECK(b.size() == 1);
    CHECK(b.find("src") == 1);
    CHECK(!b.exists("dst-stale"));

    CHECK(a.empty());
    a["again"] = 2;
    CHECK(a.size() == 1);

    Trie<int> s;
    s["x"] = 1;
    Trie<int> &sAlias = s;
    s = std::move(sAlias); // self-move no-op
    CHECK(s.size() == 1 && s.find("x") == 1);
  }

  // swap exchanges (member and free).
  {
    Trie<int> s1, s2;
    s1["a"] = 1;
    s2["b"] = 2; s2["c"] = 3;
    s1.swap(s2);
    CHECK(s1.size() == 2 && s2.size() == 1);
    CHECK(s1.find("b") == 2 && s2.find("a") == 1);

    swap(s1, s2);
    CHECK(s1.size() == 1 && s2.size() == 2);
    CHECK(s1.find("a") == 1);
  }

  // clear() empties and stays reusable.
  {
    Trie<int> c;
    c["aa"] = 1;
    c["ab"] = 2;
    c.clear();
    CHECK(c.empty());
    CHECK(!c.exists("aa"));
    CHECK(!c.hasPrefix("a"));
    c["new"] = 9; // reusable
    CHECK(c.size() == 1 && c.find("new") == 9);
    c.clear();
    c.clear(); // idempotent
    CHECK(c.empty());
  }

  // operator<< prints "key -> value" lines lexicographically; an empty
  // trie prints nothing.
  {
    Trie<int> t;
    t["b"] = 2;
    t["a"] = 1;
    ostringstream oss;
    oss << t;
    CHECK(oss.str() == "a -> 1\nb -> 2\n");
    CHECK(t.size() == 2);

    Trie<int> e;
    ostringstream ossE;
    ossE << e;
    CHECK(ossE.str() == "");
  }

  // Randomized stress: Trie vs std::map<string,int> model; random keys
  // over a small alphabet keep the trie branching realistic. Every
  // insert/replace/erase/exists/find must agree with the model.
  {
    bool allOk = true;
    const char letters[4] = {'a', 'b', 'c', 'd'};

    for (int trial = 0; trial < 20; trial++) {
      Trie<int> mine;
      map<string, int> model;

      for (int op = 0; op < 800; op++) {
        // random key: 1..6 chars over a,b,c,d
        int len = 1 + int(rnd() % 6);
        string k;
        for (int i = 0; i < len; i++)
          k += letters[rnd() % 4];

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
          // prefix sanity: if any key in the model starts with k,
          // hasPrefix(k) must be true
          bool anyPrefix = false;
          for (auto &kv : model)
            if (kv.first.compare(0, k.size(), k) == 0) {
              anyPrefix = true;
              break;
            }
          if (mine.hasPrefix(k) != anyPrefix)
            allOk = false;
        }

        if (mine.size() != model.size())
          allOk = false;
      }

      // final traversal: exact lexicographic match with the model
      bool same = true;
      auto it = model.begin();
      mine.for_each([&](const string &k, int v) {
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

  // Deep-chain smoke: a 1000-char key does not recurse into trouble on
  // insert/find (walks are iterative); erase prunes it leaves-first.
  {
    Trie<int> t;
    string k(1000, 'x');
    t[k] = 7;
    CHECK(t.size() == 1);
    CHECK(t.find(k) == 7);
    CHECK(t.erase(k));
    CHECK(t.empty());
  }

  if (failures == 0) {
    cout << "all checks passed" << endl;
    return 0;
  }
  cout << failures << " check(s) failed" << endl;
  return 1;
}
