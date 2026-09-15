// Tests for DisjointSet<T>. Prints one line per check and exits nonzero
// on the first failure. Build and run from tests/:
//   make disjointSetTest && ./disjointSetTest

#include <iostream>
#include <set>
#include <string>
#include <utility>

using namespace std;

#include "DisjointSet.h"

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
static unsigned seed = 123456789u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Fresh set: nothing in it yet.
  {
    DisjointSet<int> ds;
    CHECK(ds.numSets() == 0);
  }

  // make_set creates singleton sets; a repeat make_set is a no-op.
  {
    DisjointSet<int> ds;
    ds.make_set(1);
    ds.make_set(2);
    ds.make_set(3);
    CHECK(ds.numSets() == 3);
    CHECK(ds.size(1) == 1);
    CHECK(ds.size(2) == 1);

    ds.make_set(2); // duplicate
    CHECK(ds.numSets() == 3);
    CHECK(ds.find(2) == 2); // membership unchanged
  }

  // find returns the element itself for a singleton...
  {
    DisjointSet<int> ds;
    ds.make_set(7);
    CHECK(ds.find(7) == 7);
  }

  // ...and the representative after a union.
  {
    DisjointSet<int> ds;
    ds.make_set(1);
    ds.make_set(2);
    ds.join(1, 2);
    CHECK(ds.numSets() == 1);
    int root = ds.find(1);
    CHECK(root == ds.find(2)); // same representative
    CHECK(root == 1 || root == 2);
  }

  // join works when called through NON-root elements: unions must be
  // transitive (regression: the original join attached the passed
  // elements instead of the roots, so joining through a middle element
  // split the set).
  {
    DisjointSet<int> ds;
    ds.make_set(1);
    ds.make_set(2);
    ds.make_set(3);
    ds.join(1, 2); // 1-2 merged
    ds.join(2, 3); // through 2, which may not be the representative
    CHECK(ds.numSets() == 1);
    CHECK(ds.find(1) == ds.find(3));
    CHECK(ds.size(1) == 3);
    CHECK(ds.size(3) == 3); // size readable from ANY member
  }

  // join(x, x) and re-joining an already-united pair are no-ops and do
  // NOT decrement the set count (regression: the original decremented
  // num unconditionally).
  {
    DisjointSet<int> ds;
    ds.make_set(1);
    ds.make_set(2);
    ds.join(1, 1);
    CHECK(ds.numSets() == 2); // self-union changed nothing
    ds.join(1, 2);
    CHECK(ds.numSets() == 1);
    ds.join(2, 1); // already united, argument order flipped
    ds.join(1, 2); // and in original order
    CHECK(ds.numSets() == 1);
    CHECK(ds.size(1) == 2); // sizes not double-counted
  }

  // Union by rank: equal ranks bump the new root's rank so subsequent
  // unions stay balanced. Observable through correctness under load.
  {
    DisjointSet<int> ds;
    ds.make_set(0);
    ds.make_set(1);
    ds.join(0, 1);      // ranks equal: one rank bump
    ds.make_set(2);
    ds.make_set(3);
    ds.join(2, 3);
    ds.join(0, 2);      // two rank-equal pairs merge
    CHECK(ds.numSets() == 1);
    CHECK(ds.size(0) == 4);
  }

  // size() reflects the merged count and stays correct after more joins.
  {
    DisjointSet<int> ds;
    for (int i = 0; i < 10; i++)
      ds.make_set(i);
    CHECK(ds.numSets() == 10);
    ds.join(0, 9);
    CHECK(ds.size(9) == 2); // queried from the absorbed end
    ds.join(9, 5);          // chain through a non-root
    CHECK(ds.size(5) == 3);
    CHECK(ds.numSets() == 8);
  }

  // Operations on an element that was never made throw Exception.
  {
    DisjointSet<int> ds;
    ds.make_set(1);
    bool threw;
    threw = false;
    try { ds.find(42); } catch (Exception &) { threw = true; }
    CHECK(threw); // find of a missing element

    threw = false;
    try { ds.size(42); } catch (Exception &) { threw = true; }
    CHECK(threw); // size of a missing element

    threw = false;
    try { ds.join(1, 42); } catch (Exception &) { threw = true; }
    CHECK(threw); // join with a missing element changed nothing
    CHECK(ds.numSets() == 1);
  }

  // String elements: T needs only hash + equality.
  {
    DisjointSet<string> ds;
    ds.make_set("alice");
    ds.make_set("bob");
    ds.make_set("carol");
    ds.join("alice", "bob");
    ds.join("bob", "carol");
    CHECK(ds.numSets() == 1);
    CHECK(ds.find("alice") == ds.find("carol"));
    CHECK(ds.size("bob") == 3);
  }

  // Copy construction is deep: the copy's unions leave the original
  // untouched, and vice versa.
  {
    DisjointSet<int> a;
    a.make_set(1);
    a.make_set(2);
    DisjointSet<int> b(a);
    CHECK(b.numSets() == 2);
    b.join(1, 2);
    CHECK(b.numSets() == 1);
    CHECK(a.numSets() == 2); // original unaffected
    CHECK(a.find(1) == 1);   // still its own representative

    a.join(1, 2);
    CHECK(a.numSets() == 1);
    CHECK(b.size(1) == 2); // b reached the same partition independently
  }

  // Copy assignment replaces contents and is independent afterward.
  {
    DisjointSet<int> a;
    a.make_set(1);
    a.make_set(2);
    a.make_set(3);
    a.join(1, 2);

    DisjointSet<int> b;
    b.make_set(99);
    b = a;
    CHECK(b.numSets() == 2);
    CHECK(b.find(1) == b.find(2));
    bool threw = false;
    try { b.find(99); } catch (Exception &) { threw = true; }
    CHECK(threw); // replaced contents, no residue

    b.join(2, 3);
    CHECK(b.numSets() == 1);
    CHECK(a.numSets() == 2); // assignment target does not alias
  }

  // Self-assignment is a no-op.
  {
    DisjointSet<int> a;
    a.make_set(1);
    a.make_set(2);
    a.join(1, 2);
    DisjointSet<int> &alias = a;
    a = alias;
    CHECK(a.numSets() == 1);
    CHECK(a.size(2) == 2);
  }

  // Move construction steals; the source is empty and reusable.
  {
    DisjointSet<int> a;
    for (int i = 0; i < 5; i++)
      a.make_set(i);
    a.join(0, 1);
    DisjointSet<int> m(std::move(a));
    CHECK(m.numSets() == 4);
    CHECK(m.size(0) == 2);
    CHECK(a.numSets() == 0); // emptied by the move

    a.make_set(50); // reusable
    CHECK(a.numSets() == 1);
    CHECK(a.find(50) == 50);
  }

  // Move assignment frees the destination contents and steals the source.
  {
    DisjointSet<int> a;
    for (int i = 0; i < 4; i++)
      a.make_set(i);
    a.join(0, 3);

    DisjointSet<int> b;
    b.make_set(77);
    b = std::move(a);
    CHECK(b.numSets() == 3);
    CHECK(b.find(0) == b.find(3));
    CHECK(b.size(0) == 2);
    bool threw = false;
    try { b.find(77); } catch (Exception &) { threw = true; }
    CHECK(threw); // old contents gone

    CHECK(a.numSets() == 0); // source emptied
    a.make_set(9);
    CHECK(a.find(9) == 9);   // and reusable
  }

  // Self-move is guarded.
  {
    DisjointSet<int> a;
    a.make_set(1);
    a.make_set(2);
    DisjointSet<int> &alias = a;
    a = std::move(alias);
    CHECK(a.numSets() == 2);
    CHECK(a.find(1) == 1);
  }

  // Randomized stress: every random union must keep numSets in sync
  // with the number of distinct representatives, and the per-set sizes
  // must always sum to the element count.
  {
    bool allOk = true;
    for (int trial = 0; trial < 50; trial++) {
      const int n = 1 + static_cast<int>(rnd() % 80);
      DisjointSet<int> ds;
      for (int i = 0; i < n; i++)
        ds.make_set(i);

      int unions = 1 + static_cast<int>(rnd() % n);
      for (int u = 0; u < unions; u++)
        ds.join(static_cast<int>(rnd() % n), static_cast<int>(rnd() % n));

      set<int> roots;
      for (int i = 0; i < n; i++)
        roots.insert(ds.find(i));

      if (static_cast<int>(roots.size()) != ds.numSets())
        allOk = false;

      int total = 0;
      for (set<int>::const_iterator it = roots.begin(); it != roots.end(); ++it)
        total += ds.size(*it);
      if (total != n)
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
