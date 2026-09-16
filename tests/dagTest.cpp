// Tests for DAG<W>. Prints one line per check and exits nonzero on the
// first failure. Build and run from tests/:  make dagTest && ./dagTest

#include "DAG.h"
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

static unsigned seed = 1414213562u;
static unsigned rnd() { return seed = seed * 1103515245u + 12345u; }

int main() {
  // Empty and edgeless graphs.
  {
    DAG<int> g;
    CHECK(g.vertexCount() == 0);
    CHECK(g.edgeCount() == 0);
    CHECK(g.empty());

    DAG<int> g3(3);
    CHECK(g3.vertexCount() == 3);
    CHECK(g3.edgeCount() == 0);

    // topological order of an edgeless graph: ascending vertices
    Vec<u32> order = g3.topologicalOrder();
    CHECK(order.len() == 3);
    CHECK(order[0] == 0 && order[1] == 1 && order[2] == 2);

    CHECK(g3.longestPathEdges() == 0); // no edges: longest path 0
    CHECK(g3.isSource(0) && g3.isSink(0)); // no edges: a vertex is both
  }

  // addEdge: direction, weights, rejections.
  {
    DAG<int> g(3);
    CHECK(g.addEdge(0, 1, 5));
    CHECK(g.edgeCount() == 1);
    CHECK(g.hasEdge(0, 1));
    CHECK(!g.hasEdge(1, 0)); // DIRECTION matters
    CHECK(g.weight(0, 1) == 5);
    CHECK(g.weight(1, 0) == 0); // absent: default

    CHECK(!g.addEdge(1, 0, 9)); // would close the 2-cycle with (0,1)
    CHECK(g.edgeCount() == 1);
    CHECK(!g.hasEdge(1, 0)); // nothing mutated

    CHECK(!g.addEdge(0, 1)); // parallel edge
    CHECK(!g.addEdge(1, 1)); // self-loop
    CHECK(!g.addEdge(0, 3)); // out of range

    CHECK(g.addEdge(2, 0)); // fine: 2 -> 0
    // now adding 0 -> 2 would close a cycle; already rejected above
  }

  // Acyclicity enforcement: attempt to build each cycle shape.
  {
    // 3-cycle: 0 -> 1 -> 2 -> 0
    DAG<int> g(3);
    CHECK(g.addEdge(0, 1));
    CHECK(g.addEdge(1, 2));
    CHECK(!g.addEdge(2, 0)); // rejected

    // backward edge over a chain 0->1->2->3
    DAG<int> chain(4);
    chain.addEdge(0, 1);
    chain.addEdge(1, 2);
    chain.addEdge(2, 3);
    CHECK(!chain.addEdge(3, 1)); // 1 reaches 3 already: cycle edge
    CHECK(!chain.addEdge(3, 0));
    CHECK(chain.addEdge(0, 3));  // forward jump is fine
    CHECK(chain.edgeCount() == 4);

    // diamond (no cycle): 0->1, 0->2, 1->3, 2->3
    DAG<int> diamond(4);
    CHECK(diamond.addEdge(0, 1));
    CHECK(diamond.addEdge(0, 2));
    CHECK(diamond.addEdge(1, 3));
    CHECK(diamond.addEdge(2, 3));
    CHECK(diamond.edgeCount() == 4);
    // acyclicity proven statically: addEdge succeeded => no cycle, and
    // a topological order exists
    Vec<u32> dorder = diamond.topologicalOrder();
    CHECK(dorder.len() == 4);
  }

  // Degree and neighbor views.
  {
    DAG<int> g(4);
    g.addEdge(0, 1);
    g.addEdge(0, 2);
    g.addEdge(1, 3);
    g.addEdge(2, 3);

    CHECK(g.outDegree(0) == 2);
    CHECK(g.outDegree(3) == 0);
    CHECK(g.inDegree(3) == 2);
    CHECK(g.inDegree(0) == 0);

    Vec<u32> out0 = g.outNeighbors(0);
    CHECK(out0.len() == 2 && out0[0] == 1 && out0[1] == 2); // ascending

    Vec<u32> in3 = g.inNeighbors(3);
    CHECK(in3.len() == 2 && in3[0] == 1 && in3[1] == 2); // ascending sources

    bool threw = false;
    try { g.outDegree(4); } catch (Exception &) { threw = true; }
    CHECK(threw);
    threw = false;
    try { g.inDegree(4); } catch (Exception &) { threw = true; }
    CHECK(threw);

    CHECK(g.isSink(3));
    CHECK(g.isSource(0));
    CHECK(!g.isSink(0));
  }

  // Reachability: direct, transitive, trivial, impossible.
  {
    DAG<int> g(5);
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.addEdge(2, 3);

    CHECK(g.reachable(0, 3));      // three hops
    CHECK(g.reachable(0, 1));      // direct
    CHECK(g.reachable(2, 3));
    CHECK(!g.reachable(3, 0));     // direction forbids
    CHECK(!g.reachable(1, 4));     // disconnected
    CHECK(g.reachable(4, 4));      // trivial u == v
    CHECK(!g.reachableStrict(4, 4)); // excluded by the strict form
    CHECK(g.reachableStrict(0, 3));

    // after adding 3 -> 4, the reachability extends
    CHECK(g.addEdge(3, 4));
    CHECK(g.reachable(0, 4));
    CHECK(!g.reachable(4, 0));

    bool threw = false;
    try { g.reachable(0, 5); } catch (Exception &) { threw = true; }
    CHECK(threw);
  }

  // Topological order on a diamond: sources before sinks.
  {
    DAG<int> g(4);
    g.addEdge(0, 1);
    g.addEdge(0, 2);
    g.addEdge(1, 3);
    g.addEdge(2, 3);

    Vec<u32> order = g.topologicalOrder();
    CHECK(order.len() == 4);

    // validity: for every edge u->v, u precedes v
    bool valid = true;
    int pos[4];
    for (u32 i = 0; i < 4; i++)
      pos[order[i]] = int(i);
    // edge 0->1: pos[0] < pos[1], etc.
    valid = pos[0] < pos[1] && pos[0] < pos[2] && pos[1] < pos[3] &&
            pos[2] < pos[3];
    CHECK(valid);

    // Kahn with smallest-first: 0 first, 3 last
    CHECK(order[0] == 0 && order[3] == 3);
  }

  // Longest path (edge count).
  {
    DAG<int> g(5);
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.addEdge(1, 3);
    g.addEdge(3, 4);
    CHECK(g.longestPathEdges() == 3); // 0->1->3->4

    DAG<int> single(2);
    single.addEdge(0, 1);
    CHECK(single.longestPathEdges() == 1);
  }

  // Transitive closure bitvector.
  {
    DAG<int> g(3);
    g.addEdge(0, 1);
    g.addEdge(1, 2);

    BitVector tc = g.transitiveClosure();
    CHECK(tc.size() == 9);
    CHECK(tc.getBit(0 * 3 + 1)); // 0 reaches 1
    CHECK(tc.getBit(0 * 3 + 2)); // 0 reaches 2 (transitively)
    CHECK(tc.getBit(1 * 3 + 2)); // 1 reaches 2
    CHECK(!tc.getBit(2 * 3 + 0)); // 2 reaches nothing
    CHECK(!tc.getBit(0 * 3 + 0)); // no self-reachability recorded
  }

  // removeEdge: fine in a DAG (removal cannot create a cycle).
  {
    DAG<int> g(3);
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    CHECK(g.removeEdge(0, 1));
    CHECK(g.edgeCount() == 1);
    CHECK(!g.hasEdge(0, 1));
    CHECK(!g.removeEdge(0, 1)); // already gone
    CHECK(g.reachable(1, 2));   // remaining edge intact

    CHECK(!g.removeEdge(0, 9)); // out of range reports false (bounds
                                // rejection, per the addEdge style)
  }

  // clearEdges keeps vertices.
  {
    DAG<int> g(4);
    g.addEdge(0, 1);
    g.addEdge(1, 2);
    g.clearEdges();
    CHECK(g.edgeCount() == 0 && g.vertexCount() == 4);
    Vec<u32> order = g.topologicalOrder();
    CHECK(order.len() == 4 && order[0] == 0);
    g.addEdge(3, 0); // reusable; 3 -> 0 is acyclic
    CHECK(g.edgeCount() == 1);
  }

  // operator== / !=.
  {
    DAG<int> a(3), b(3);
    a.addEdge(0, 1, 5);
    b.addEdge(0, 1, 5);
    CHECK(a == b);

    b.removeEdge(0, 1);
    b.addEdge(1, 0, 5); // different direction
    CHECK(a != b);

    DAG<int> c(2);
    c.addEdge(0, 1, 5);
    CHECK(a != c); // vertex counts differ
  }

  // Copy construction deep; assignment replaces; self-assign.
  {
    DAG<int> src(4);
    src.addEdge(0, 1, 1);
    src.addEdge(1, 2, 2);

    DAG<int> cp(src);
    CHECK(cp.edgeCount() == 2);
    cp.addEdge(2, 3);
    cp.removeEdge(0, 1);
    CHECK(cp.edgeCount() == 2);
    CHECK(src.edgeCount() == 2);
    CHECK(src.hasEdge(0, 1));
    CHECK(!src.hasEdge(2, 3));

    DAG<int> big(20);
    big = src;
    CHECK(big.vertexCount() == 4);

    DAG<int> &alias = src;
    src = alias;
    CHECK(src.edgeCount() == 2);
    CHECK(src.weight(1, 2) == 2);
  }

  // Move construction steals; source reusable.
  {
    DAG<int> src(4);
    src.addEdge(0, 1);
    DAG<int> dst(std::move(src));
    CHECK(dst.vertexCount() == 4 && dst.edgeCount() == 1);
    CHECK(src.vertexCount() == 0);

    DAG<int> redo(2);
    src = redo;
    CHECK(src.addEdge(0, 1));
  }

  // Self-move no-op; swap member/free.
  {
    DAG<int> m(3);
    m.addEdge(0, 1);
    DAG<int> &alias = m;
    m = std::move(alias);
    CHECK(m.vertexCount() == 3 && m.hasEdge(0, 1));

    DAG<int> s1(2), s2(3);
    s1.addEdge(0, 1);
    s2.addEdge(1, 2);
    s1.swap(s2);
    CHECK(s1.vertexCount() == 3 && s2.vertexCount() == 2);
    CHECK(s1.hasEdge(1, 2) && s2.hasEdge(0, 1));
    swap(s1, s2);
    CHECK(s1.vertexCount() == 2 && s2.vertexCount() == 3);
  }

  // operator<< prints out-adjacency with weights when nonzero.
  {
    DAG<int> g(3);
    g.addEdge(0, 1, 4);
    g.addEdge(0, 2);
    ostringstream oss;
    oss << g;
    CHECK(oss.str().find("0: 1(4) 2") != string::npos);
    CHECK(oss.str().find("2:") != string::npos);

    DAG<int> e;
    ostringstream ossE;
    ossE << e;
    CHECK(ossE.str() == "");
  }

  // Randomized stress: random safe DAG growth against a brute-force
  // reachability model; verify addEdge's cycle rejection is exactly
  // right and topological order stays valid.
  {
    bool allOk = true;
    for (int trial = 0; trial < 30; trial++) {
      const u32 V = 2 + rnd() % 9;
      DAG<int> g(V);

      bool adj[10][10];
      bool reach[10][10]; // transitive closure model
      for (u32 i = 0; i < 10; i++)
        for (u32 j = 0; j < 10; j++) {
          adj[i][j] = false;
          reach[i][j] = (i == j);
        }

      for (int op = 0; op < 60; op++) {
        u32 u = rnd() % V, v = rnd() % V;
        if (u == v)
          continue;

        bool want = !adj[u][v] && !reach[v][u]; // may add iff acyclic
        bool got = g.addEdge(u, v);
        if (want != got)
          allOk = false;
        if (got) {
          adj[u][v] = true;
          // update closure: all a with reach[a][u], all b with reach[v][b]
          for (u32 a2 = 0; a2 < V; a2++)
            for (u32 b2 = 0; b2 < V; b2++)
              if (reach[a2][u] && reach[v][b2])
                reach[a2][b2] = true;
        }
      }

      u64 edgeTotal = 0;
      for (u32 i = 0; i < V; i++)
        for (u32 j = 0; j < V; j++)
          if (adj[i][j])
            edgeTotal++;
      if (g.edgeCount() != edgeTotal)
        allOk = false;

      // model the reachability DFS on the closure and compare
      for (u32 u = 0; u < V && allOk; u++)
        for (u32 v = 0; v < V; v++)
          if (g.reachable(u, v) != reach[u][v])
            allOk = false;

      // topological order validity: positions must respect edges
      try {
        Vec<u32> order = g.topologicalOrder();
        if (order.len() != V)
          allOk = false;
        int pos[10];
        for (u32 i = 0; i < V; i++)
          pos[order[i]] = int(i);
        for (u32 u = 0; u < V; u++)
          for (u32 v = 0; v < V; v++)
            if (adj[u][v] && pos[u] >= pos[v])
              allOk = false;
      } catch (Exception &) {
        allOk = false; // a well-formed DAG always has an order
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
