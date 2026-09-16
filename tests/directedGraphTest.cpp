// Tests for DirectedGraph<W>. Prints one line per check and exits
// nonzero on the first failure. Build and run from tests/:
//   make directedGraphTest && ./directedGraphTest

#include "DirectedGraph.h"
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

static unsigned seed = 3141592654u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Empty and edgeless.
  {
    DirectedGraph<int> g;
    CHECK(g.vertexCount() == 0 && g.edgeCount() == 0 && g.empty());
    CHECK(!g.hasCycle()); // no edges: no cycle
    CHECK(g.isDAG());

    DirectedGraph<int> g3(3);
    CHECK(!g3.hasCycle());
    // an edgeless digraph has a valid topological order
    Vec<u32> order = g3.topologicalOrder();
    CHECK(order.len() == 3);
  }

  // Direction: addEdge stores ONE direction (vs UndirectedGraph's two).
  {
    DirectedGraph<int> g(3);
    CHECK(g.addEdge(0, 1, 5));
    CHECK(g.hasEdge(0, 1));
    CHECK(!g.hasEdge(1, 0)); // direction kept
    CHECK(g.weight(0, 1) == 5);
    CHECK(g.weight(1, 0) == 0); // default on absence

    CHECK(!g.addEdge(0, 1));  // parallel
    CHECK(!g.addEdge(1, 1));  // self-loop
    CHECK(!g.addEdge(0, 3));  // range
    CHECK(g.edgeCount() == 1);
  }

  // Cycles are ACCEPTED (the class contract).
  {
    DirectedGraph<int> g(3);
    CHECK(g.addEdge(0, 1));
    CHECK(g.addEdge(1, 2));
    CHECK(g.addEdge(2, 0)); // accepted here; DAG would reject
    CHECK(g.edgeCount() == 3);
    CHECK(g.hasCycle());
    CHECK(!g.isDAG());

    // self-loop via a real edge is impossible, but a 2-cycle works
    DirectedGraph<int> two(2);
    two.addEdge(0, 1);
    two.addEdge(1, 0);
    CHECK(two.hasCycle());
    CHECK(two.isBidirectionalPair(0, 1));
    CHECK(!two.isBidirectionalPair(0, 0)); // anti-parallel only

    bool threw = false;
    try { two.topologicalOrder(); } catch (Exception &) { threw = true; }
    CHECK(threw); // cyclic: no order exists
  }

  // hasCycle across shapes (positive cases).
  {
    // deep cycle 0->1->2->3->1
    DirectedGraph<int> g(4);
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.addEdge(2, 3);
    CHECK(!g.hasCycle());
    g.addEdge(3, 1); // closes it
    CHECK(g.hasCycle());

    // cycle NOT through vertex 0's subtree: 4 isolated edges forming a
    // separate cycle among 1,2,3 while 0 dangles
    DirectedGraph<int> sep(4);
    sep.addEdge(0, 1);
    sep.addEdge(1, 2);
    sep.addEdge(2, 3);
    sep.addEdge(3, 1);
    CHECK(sep.hasCycle());
  }

  // DFS reachability with cycles: terminates (no recursion after the
  // mark), and reachability is cycle-safe.
  {
    DirectedGraph<int> g(3);
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.addEdge(2, 0);

    CHECK(g.reachable(0, 2));
    CHECK(g.reachable(2, 1)); // the cycle makes everything reachable
    CHECK(g.reachable(1, 0));
    CHECK(g.reachable(2, 0));
  }

  // SCC: classic shapes.
  {
    // two SCCs: {0,1,2} cyclic, {3} alone, edge 2->3
    DirectedGraph<int> g(4);
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.addEdge(2, 0);
    g.addEdge(2, 3);

    Vec<u32> comps = g.stronglyConnectedComponents();
    CHECK(comps[0] == comps[1]); // in one SCC
    CHECK(comps[1] == comps[2]);
    CHECK(comps[3] != comps[0]); // {3} separate

    CHECK(g.stronglyConnected(0, 2));
    CHECK(!g.stronglyConnected(0, 3)); // no path back
    CHECK(g.stronglyConnected(3, 3));  // trivial self-reachability

    // all-singletons DAG: each vertex its own SCC
    DirectedGraph<int> dag(4);
    dag.addEdge(0, 1);
    dag.addEdge(1, 2);
    dag.addEdge(2, 3);
    Vec<u32> singles = dag.stronglyConnectedComponents();
    bool distinct = singles[0] != singles[1] && singles[1] != singles[2] &&
                    singles[2] != singles[3];
    CHECK(distinct);
    CHECK(dag.isDAG());

    // everything in one SCC: a strongly connected component pair
    DirectedGraph<int> bi(2);
    bi.addEdge(0, 1);
    bi.addEdge(1, 0);
    CHECK(bi.stronglyConnected(0, 1));
  }

  // SCC in the textbook example (CLRS Fig 22.9 adapted): verify
  // component PARTITION equality rather than labels.
  {
    // vertices: a(0) b(1) c(2) d(3) e(4) f(5) g(6) h(7)
    // SCCs: {0,1,2}, {3,4}, {5}, {6}, {7}
    DirectedGraph<int> g(8);
    g.addEdge(0, 1); g.addEdge(1, 2); g.addEdge(2, 0);
    g.addEdge(2, 3);                      // {0,1,2} -> {3,4}
    g.addEdge(3, 4); g.addEdge(4, 3);     // pair SCC
    g.addEdge(4, 5);
    g.addEdge(5, 6); g.addEdge(6, 7);
    g.addEdge(7, 5);                      // wait: 5->6->7->5 = cycle!

    // recompute expectation: {5,6,7} is an SCC
    Vec<u32> comps = g.stronglyConnectedComponents();
    CHECK(comps[5] == comps[6]);
    CHECK(comps[6] == comps[7]);
    CHECK(comps[0] == comps[1] && comps[1] == comps[2]);
    CHECK(comps[3] == comps[4]);
    // distinct SCCs differ
    CHECK(comps[0] != comps[3]);
    CHECK(comps[3] != comps[5]);
    CHECK(g.hasCycle());
  }

  // topologicalOrder on the interesting NON-cyclic digraph works.
  {
    DirectedGraph<int> g(4);
    g.addEdge(3, 0);
    g.addEdge(3, 1);
    g.addEdge(1, 2);

    Vec<u32> order = g.topologicalOrder();
    CHECK(order.len() == 4);
    int pos[4];
    for (u32 i = 0; i < 4; i++)
      pos[order[i]] = int(i);
    CHECK(pos[3] < pos[0] && pos[3] < pos[1] && pos[1] < pos[2]);
  }

  // in/out neighbors and degrees.
  {
    DirectedGraph<int> g(4);
    g.addEdge(0, 1);
    g.addEdge(0, 2);
    g.addEdge(3, 0);

    CHECK(g.outDegree(0) == 2 && g.inDegree(0) == 1);

    Vec<u32> out0 = g.outNeighbors(0);
    CHECK(out0.len() == 2 && out0[0] == 1 && out0[1] == 2);
    Vec<u32> in0 = g.inNeighbors(0);
    CHECK(in0.len() == 1 && in0[0] == 3);

    bool threw = false;
    try { g.outDegree(4); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // transitiveClosure across cycles.
  {
    DirectedGraph<int> g(3);
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.addEdge(2, 0);

    BitVector tc = g.transitiveClosure();
    // everything reaches everything (strict)
    for (u32 u = 0; u < 3; u++)
      for (u32 v = 0; v < 3; v++)
        if (u != v && !tc.getBit(u * 3 + v))
          CHECK(false);
    CHECK(true);
    CHECK(!tc.getBit(0 * 3 + 0)); // diagonal unrecorded
  }

  // removeEdge and clearEdges.
  {
    DirectedGraph<int> g(3);
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    CHECK(g.removeEdge(0, 1));
    CHECK(!g.removeEdge(0, 1));
    CHECK(g.edgeCount() == 1);
    g.clearEdges();
    CHECK(g.edgeCount() == 0);
    CHECK(!g.hasCycle());
    g.addEdge(2, 0); // reusable
    CHECK(g.edgeCount() == 1);
  }

  // operator== / !=.
  {
    DirectedGraph<int> a(3), b(3);
    a.addEdge(0, 1, 5);
    b.addEdge(0, 1, 5);
    CHECK(a == b);

    b.addEdge(1, 0); // extra reverse edge
    CHECK(a != b);

    DirectedGraph<int> c(2);
    c.addEdge(0, 1, 5);
    CHECK(a != c); // vertex counts
  }

  // Copy construction deep; assignment; self-assign; move; self-move; swap.
  {
    DirectedGraph<int> src(4);
    src.addEdge(0, 1, 1);
    src.addEdge(1, 2, 2);
    src.addEdge(2, 0); // a cycle: copies must be cycle-faithful

    DirectedGraph<int> cp(src);
    CHECK(cp.edgeCount() == 3);
    CHECK(cp.hasCycle()); // structural property COPIED too
    cp.addEdge(0, 3);
    cp.removeEdge(0, 1);
    CHECK(cp.edgeCount() == 3);
    CHECK(src.edgeCount() == 3);
    CHECK(src.hasEdge(0, 1));
    CHECK(!src.hasEdge(0, 3));

    DirectedGraph<int> big(30);
    big = src;
    CHECK(big.vertexCount() == 4);

    DirectedGraph<int> &alias = src;
    src = alias;
    CHECK(src.hasCycle() && src.edgeCount() == 3);

    DirectedGraph<int> mv(std::move(src));
    CHECK(mv.edgeCount() == 3 && mv.hasCycle());
    CHECK(src.vertexCount() == 0);

    DirectedGraph<int> redo(2);
    src = redo;
    CHECK(src.addEdge(0, 1));

    DirectedGraph<int> &alias2 = src;
    src = std::move(alias2);
    CHECK(src.vertexCount() == 2 && src.hasEdge(0, 1));

    DirectedGraph<int> s1(3), s2(5);
    s1.addEdge(0, 1);
    s2.addEdge(4, 2);
    s1.swap(s2);
    CHECK(s1.vertexCount() == 5 && s1.hasEdge(4, 2));
    swap(s1, s2);
    CHECK(s1.vertexCount() == 3 && s1.hasEdge(0, 1));
  }

  // operator<< prints out-adjacency ascending.
  {
    DirectedGraph<int> g(3);
    g.addEdge(2, 0);
    g.addEdge(0, 1, 7);
    ostringstream oss;
    oss << g;
    CHECK(oss.str().find("0: 1(7)") != string::npos);
    CHECK(oss.str().find("2: 0") != string::npos);

    DirectedGraph<int> e;
    ostringstream ossE;
    ossE << e;
    CHECK(ossE.str() == "");
  }

  // Randomized stress: Tarjan SCC vs brute-force mutual reachability;
  // hasCycle vs the SCC predictor (cycle exists iff some SCC has 2+
  // vertices OR any self-loop exists).
  {
    bool allOk = true;
    for (int trial = 0; trial < 30; trial++) {
      const u32 V = 2 + rnd() % 8;
      DirectedGraph<int> g(V);
      bool adj[10][10];
      for (u32 i = 0; i < 10; i++)
        for (u32 j = 0; j < 10; j++)
          adj[i][j] = false;

      for (int op = 0; op < 40; op++) {
        u32 u = rnd() % V, v = rnd() % V;
        if (g.addEdge(u, v))
          adj[u][v] = true;
      }

      // brute force reachability (edges seeded, then Floyd-Warshall)
      bool reach[10][10];
      for (u32 i = 0; i < V; i++)
        for (u32 j = 0; j < V; j++)
          reach[i][j] = (i == j) || adj[i][j];
      for (u32 k = 0; k < V; k++)          // Floyd-Warshall transitive
        for (u32 i = 0; i < V; i++)
          for (u32 j = 0; j < V; j++)
            if (reach[i][k] && reach[k][j])
              reach[i][j] = true;

      Vec<u32> comps = g.stronglyConnectedComponents();
      for (u32 u = 0; u < V && allOk; u++)
        for (u32 v = 0; v < V; v++)
          if ((comps[u] == comps[v]) != (reach[u][v] && reach[v][u]))
            allOk = false;

      // cycle predictor
      bool modelCycle = false;
      for (u32 u = 0; u < V && !modelCycle; u++)
        for (u32 v = 0; v < V; v++)
          if (u != v && reach[u][v] && reach[v][u]) {
            modelCycle = true;
            break;
          }
      if (modelCycle != g.hasCycle())
        allOk = false;

      // reachable() vs model
      for (u32 u = 0; u < V && allOk; u++)
        for (u32 v = 0; v < V; v++)
          if (g.reachable(u, v) != reach[u][v])
            allOk = false;

      // topo order inversion: cyclic graphs throw, acyclic succeed
      if (modelCycle) {
        bool threw = false;
        try { g.topologicalOrder(); } catch (Exception &) { threw = true; }
        if (!threw)
          allOk = false;
      } else {
        try {
          Vec<u32> order = g.topologicalOrder();
          if (order.len() != V)
            allOk = false;
        } catch (Exception &) {
          allOk = false;
        }
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
