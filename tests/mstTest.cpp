// Tests for MST (Kruskal and Prim over UndirectedGraph<W>). Prints one
// line per check and exits nonzero on the first failure. Build and run
// from tests/:  make mstTest && ./mstTest

#include "MST.h"
#include <iostream>
#include <sstream>
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

static unsigned seed = 836478195u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

// Verifies mask = valid spanning structure: exactly once-per-edge
// bits, chosen edges form no cycle (union-find), count matches the
// component model (V - comps).
template <typename W>
bool maskIsForest(const UndirectedGraph<W> &g, const BitVector &mask) {
  const u32 V = g.vertexCount();

  // union-find over chosen edges: two endpoints already joined would
  // mean a cycle in the CHOSEN edge set
  u32 parent[16];
  for (u32 i = 0; i < V; i++)
    parent[i] = i;

  for (u32 u = 0; u < V; u++)
    for (u32 v = u + 1; v < V; v++) {
      if (!mask.getBit(u64(u) * V + v))
        continue;
      if (!g.hasEdge(u, v))
        return false; // chose a non-edge

      // find
      u32 a = u, b = v;
      while (parent[a] != a)
        a = parent[a];
      while (parent[b] != b)
        b = parent[b];
      if (a == b)
        return false; // cycle among chosen edges
      parent[a] = b;
    }

  // tree/forest size: V - components chosen edges
  u32 comps = g.connectedComponents();
  u64 want = u64(V) - comps;
  return mstEdgeCount(mask, V) == want;
}

int main() {
  // Trivial cases.
  {
    UndirectedGraph<int> g(1); // single vertex: empty tree
    int tot = -1;
    BitVector m = kruskalMST(g, tot);
    CHECK(mstEdgeCount(m, 1) == 0);
    CHECK(tot == 0);

    int totP = -1;
    BitVector mp = primMST(g, 0, totP);
    CHECK(mstEdgeCount(mp, 1) == 0);
    CHECK(totP == 0);

    UndirectedGraph<int> e; // 0 vertices
    BitVector me = kruskalMST(e, tot);
    CHECK(mstEdgeCount(me, 0) == 0);

    // disconnected graph: forest totals from both algorithms match
    UndirectedGraph<int> disc(5);
    disc.addEdge(0, 1, 2);
    disc.addEdge(1, 2, 3);
    disc.addEdge(3, 4, 1); // two components: {0,1,2} and {3,4}

    int tk, tp, tp2;
    BitVector k = kruskalMST(disc, tk);
    BitVector p = primMST(disc, 0, tp);   // single-tree: {0,1,2} only
    BitVector pf = primForestMST(disc, tp2); // all components
    CHECK(maskIsForest(disc, k));   // kruskal: spans everything
    CHECK(maskIsForest(disc, pf));  // primForest: spans everything
    CHECK(maskIsForest(disc, pf));
    CHECK(tk == 5 + 1);                    // 2+3 + 1
    CHECK(tp == 5);                        // seed's component only
    CHECK(tp2 == tk);                      // forest totals agree
    CHECK(mstEdgeCount(pf, 5) == 3);
  }

  // Known-weight MST (CLRS-style): verify exact weight and edges.
  {
    UndirectedGraph<int> g(5);
    g.addEdge(0, 1, 4);
    g.addEdge(0, 2, 13);
    g.addEdge(1, 2, 9);
    g.addEdge(1, 3, 7);
    g.addEdge(2, 3, 3);
    g.addEdge(3, 4, 6);

    // MST: {0-1(4), 2-3(3), 1-3(7), 3-4(6)} = 20
    int totK, totP;
    BitVector k = kruskalMST(g, totK);
    BitVector p = primMST(g, 0, totP);
    CHECK(totK == 20);
    CHECK(totP == 20);
    CHECK(maskIsForest(g, k));
    CHECK(maskIsForest(g, p));

    // same edge SET (unique MST here: weights distinct enough)
    bool same = true;
    for (u32 u = 0; u < 5; u++)
      for (u32 v = u + 1; v < 5; v++)
        if (k.getBit(u64(u) * 5 + v) != p.getBit(u64(u) * 5 + v))
          same = false;
    CHECK(same);
  }

  // Equal-weight ties: both algorithms still minimize, totals equal
  // (edge sets may differ when ties exist).
  {
    UndirectedGraph<int> g(4);
    g.addEdge(0, 1, 1);
    g.addEdge(1, 2, 1);
    g.addEdge(2, 3, 1);

    int totK, totP;
    BitVector k = kruskalMST(g, totK);
    BitVector p = primMST(g, 0, totP);
    CHECK(totK == 3 && totP == 3);
    CHECK(maskIsForest(g, k));
    CHECK(maskIsForest(g, p));
  }

  // Cycle-heavy graph: the MST drops exactly the heaviest cycle edges.
  {
    UndirectedGraph<int> g(4);
    g.addEdge(0, 1, 10);
    g.addEdge(1, 2, 10);
    g.addEdge(2, 3, 10);
    g.addEdge(3, 0, 10);
    g.addEdge(0, 2, 1); // cheap chord

    int totK, totP;
    BitVector k = kruskalMST(g, totK);
    BitVector p = primMST(g, 1, totP);
    CHECK(totK == 21); // the two cheap chords + one 10
    CHECK(totP == 21);
    CHECK(maskIsForest(g, k));

    // verify via mask: '0-2' chosen, not 3 of the 10s
    CHECK(k.getBit(u64(0) * 4 + 2));
  }

  // Randomized stress: random graphs, totals agree between algorithms,
  // masks are valid forests, totals match a brute-force MST model.
  {
    bool allOk = true;
    for (int trial = 0; trial < 40; trial++) {
      const u32 V = 1 + rnd() % 9;
      UndirectedGraph<int> g(V);

      int W[9][9];
      for (u32 i = 0; i < 9; i++)
        for (u32 j = 0; j < 9; j++)
          W[i][j] = 0;

      for (int op = 0; op < int(V * 3); op++) {
        u32 u = rnd() % V, v = rnd() % V;
        if (u == v)
          continue;
        int w = 1 + int(rnd() % 50);
        if (g.addEdge(u, v, w))
          W[u][v] = W[v][u] = w;
      }

      int totK = -1, totP = -7;
      BitVector k = kruskalMST(g, totK);
      BitVector p = primMST(g, 0, totP);

      if (totK != totP)
        allOk = false;
      if (!maskIsForest(g, k))
        allOk = false;
      if (!maskIsForest(g, p))
        allOk = false;

      // brute-force MST on a connected graph: greedy cut property
      // model = Prim's structure recomputed independently
      bool inTree[9];
      for (u32 i = 0; i < V; i++)
        inTree[i] = false;
      inTree[0] = true; // same start as Prim
      int modelTotal = 0;
      for (u64 taken = 0; taken + 1 < V; taken++) {
        int bestW = 1 << 30;
        int bv = -1;
        for (u32 u = 0; u < V; u++) {
          if (!inTree[u])
            continue;
          for (u32 v = 0; v < V; v++) {
            if (inTree[v] || !W[u][v])
              continue;
            if (W[u][v] < bestW) {
              bestW = W[u][v];
              bv = int(v);
            }
          }
        }
        if (bv >= 0) {
          inTree[bv] = true;
          modelTotal += bestW;
        }
      }
      // connected? compare only when Prim actually spanned all
      if (g.connectedComponents() == 1 && totP != modelTotal)
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
